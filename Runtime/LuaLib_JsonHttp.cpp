// Runtime/LuaLib_JsonHttp.cpp
// json.* 和 http.* Lua 全局库实现
//
// json.parse(str)              → table | nil, errMsg
// json.stringify(val)          → string
// json.get(str, path)          → value | nil   ("choices[0].message.content")
// json.set(str, key, val)      → string         (顶层 key 写入)
//
// http.get(url [, hdrs])                → body, statusCode, error
// http.post(url, body [, hdrs])         → body, statusCode, error
// http.request(method, url, body, hdrs) → body, statusCode, error
//
// Emscripten 下 http.* 返回 nil, 0, "not supported on WebGL"
// NoLua（未定义 BLUEPRINT_HAS_LUA）时整个文件不编译

#ifdef BLUEPRINT_HAS_LUA

#include "LuaBindings.h"
#include "Http/IHttpClient.h"
#include "MainThreadDispatcher.h"
#include "../../Utils/Json/crude_json.h"

#include <lua.hpp>
#include <string>
#include <map>

namespace NodeEditor {
namespace Runtime {

// ============================================================================
// crude_json ↔ Lua 转换
// ============================================================================
static void pushJsonValue(lua_State* L, const crude_json::value& v);
static crude_json::value luaToJson(lua_State* L, int idx);

static void pushJsonValue(lua_State* L, const crude_json::value& v)
{
    if (v.is_null())    { lua_pushnil(L);  return; }
    if (v.is_boolean()) { lua_pushboolean(L, (int)v.get<bool>());  return; }
    if (v.is_number())  { lua_pushnumber(L, v.get<double>());       return; }
    if (v.is_string())  { lua_pushstring(L, v.get<std::string>().c_str()); return; }
    if (v.is_array()) {
        const auto& arr = v.get<crude_json::array>();
        lua_createtable(L, (int)arr.size(), 0);
        for (int i = 0; i < (int)arr.size(); ++i) {
            pushJsonValue(L, arr[i]);
            lua_rawseti(L, -2, i + 1);
        }
        return;
    }
    if (v.is_object()) {
        const auto& obj = v.get<crude_json::object>();
        lua_createtable(L, 0, (int)obj.size());
        for (const auto& kv : obj) {
            lua_pushstring(L, kv.first.c_str());
            pushJsonValue(L, kv.second);
            lua_rawset(L, -3);
        }
        return;
    }
    lua_pushnil(L);
}

static crude_json::value luaToJson(lua_State* L, int idx)
{
    // 统一转为绝对索引，避免递归时相对索引失效
    int abs = (idx > 0 || idx <= LUA_REGISTRYINDEX) ? idx : lua_gettop(L) + idx + 1;
    int t = lua_type(L, abs);
    if (t == LUA_TNIL)      return crude_json::value();
    if (t == LUA_TBOOLEAN)  return crude_json::value((bool)lua_toboolean(L, abs));
    if (t == LUA_TNUMBER) {
        if (lua_isinteger(L, abs))
            return crude_json::value((double)lua_tointeger(L, abs));
        return crude_json::value(lua_tonumber(L, abs));
    }
    if (t == LUA_TSTRING)   return crude_json::value(std::string(lua_tostring(L, abs)));
    if (t == LUA_TTABLE) {
        // 检测是否纯整数键（1..n）→ array
        lua_pushnil(L);
        bool isArr = true; int n = 0;
        while (lua_next(L, abs)) {
            if (!lua_isinteger(L, -2)) { isArr = false; lua_pop(L, 2); break; }
            ++n; lua_pop(L, 1);
        }
        if (isArr && n > 0) {
            crude_json::array arr; arr.reserve(n);
            for (int i = 1; i <= n; ++i) {
                lua_rawgeti(L, abs, i);
                arr.push_back(luaToJson(L, -1));
                lua_pop(L, 1);
            }
            return crude_json::value(std::move(arr));
        }
        crude_json::object obj;
        lua_pushnil(L);
        while (lua_next(L, abs)) {
            std::string key;
            if (lua_type(L, -2) == LUA_TSTRING)      key = lua_tostring(L, -2);
            else if (lua_isinteger(L, -2)) key = std::to_string(lua_tointeger(L, -2));
            obj[key] = luaToJson(L, -1);
            lua_pop(L, 1);
        }
        return crude_json::value(std::move(obj));
    }
    return crude_json::value();
}

// 路径遍历：支持 "a.b[0].c"
static const crude_json::value* getJsonPath(const crude_json::value& root,
                                             const std::string& path)
{
    const crude_json::value* cur = &root;
    std::string token;
    for (size_t i = 0; i <= path.size(); ++i) {
        char c = (i < path.size()) ? path[i] : '.';
        if (c == '.' || c == '[') {
            if (!token.empty()) {
                if (!cur->is_object()) return nullptr;
                const auto& obj = cur->get<crude_json::object>();
                auto it = obj.find(token);
                if (it == obj.end()) return nullptr;
                cur = &it->second;
                token.clear();
            }
            if (c == '[') {
                std::string idxStr;
                ++i;
                while (i < path.size() && path[i] != ']') idxStr += path[i++];
                if (!cur->is_array()) return nullptr;
                int idx = std::stoi(idxStr);
                const auto& arr = cur->get<crude_json::array>();
                if (idx < 0 || idx >= (int)arr.size()) return nullptr;
                cur = &arr[idx];
            }
        } else {
            token += c;
        }
    }
    return cur;
}

// ============================================================================
// http 异步辅助
//
// 设计说明：
//   所有 http.* 均为异步回调式，调用后立即返回 0。
//   HTTP 完成后由 MainThreadDispatcher::Post 回到主线程，再调用 Lua callback。
//
//   Lua 用法：
//     http.get(url [, headers_table], function(body, status, err) ... end)
//     http.post(url, body [, headers_table], function(body, status, err) ... end)
//     http.request(method, url, body, headers_table_or_str, function(...) end)
// ============================================================================

// 把 Lua table（栈上 idx）转为 JSON headers 字符串 {"K":"V",...}
static std::string luaTableToHeadersJson(lua_State* L, int idx)
{
    int abs = (idx > 0 || idx <= LUA_REGISTRYINDEX) ? idx : lua_gettop(L) + idx + 1;
    std::string h = "{"; bool first = true;
    lua_pushnil(L);
    while (lua_next(L, abs)) {
        if (!first) h += ","; first = false;
        h += "\"";
        if (lua_type(L, -2) == LUA_TSTRING)
            for (char c : std::string(lua_tostring(L, -2)))
                h += (c == '"' ? "\\\"" : std::string(1, c));
        h += "\":\"";
        if (lua_type(L, -1) == LUA_TSTRING)
            for (char c : std::string(lua_tostring(L, -1)))
                h += (c == '"' ? "\\\"" : std::string(1, c));
        h += "\"";
        lua_pop(L, 1);
    }
    return h + "}";
}

// 通用异步分发：构造 HttpRequest，调用 SendAsync；
// 回调由 MainThreadDispatcher::Post 派回主线程，再 lua_rawcall callback。
// cbRef：LUA_REGISTRY 里的 callback 函数引用（调用后自动 unref）。
static void asyncRequest(lua_State* L,
                         const std::string& method,
                         const std::string& url,
                         const std::string& body,
                         const std::string& headersJson,
                         int cbRef)
{
    auto* client = BP_GetHttpClient();
    if (!client) {
        // 立即以错误调用 callback
        lua_rawgeti(L, LUA_REGISTRYINDEX, cbRef);
        luaL_unref(L, LUA_REGISTRYINDEX, cbRef);
        lua_pushnil(L);
        lua_pushinteger(L, 0);
        lua_pushstring(L, "No HttpClient registered");
        lua_pcall(L, 3, 0, 0);
        return;
    }

    HttpRequest req;
    req.method = method; req.url = url; req.body = body;
    {
        auto hj = crude_json::value::parse(headersJson);
        if (hj.is_object())
            for (const auto& kv : hj.get<crude_json::object>())
                if (kv.second.is_string())
                    req.headers[kv.first] = kv.second.get<std::string>();
    }

    // 把 lua_State* 和 cbRef 捕获进 SendAsync 回调，
    // 回调经 MainThreadDispatcher::Post 在主线程执行 → 安全调用 Lua
    client->SendAsync(req, [L, cbRef](HttpResponse resp) {
        MainThreadDispatcher::Get().Post([L, cbRef, resp = std::move(resp)]() {
            lua_rawgeti(L, LUA_REGISTRYINDEX, cbRef);
            luaL_unref(L, LUA_REGISTRYINDEX, cbRef);
            lua_pushstring(L, resp.body.c_str());
            lua_pushinteger(L, resp.statusCode);
            lua_pushstring(L, resp.error.c_str());
            if (lua_pcall(L, 3, 0, 0) != LUA_OK) {
                // Lua 回调出错，打印到 stderr，不崩溃
                const char* err = lua_tostring(L, -1);
                fprintf(stderr, "[http callback] Lua error: %s\n", err ? err : "(unknown)");
                lua_pop(L, 1);
            }
        });
    });
}

// ============================================================================
// 公开入口：注册 json.* 和 http.* 全局表
// ============================================================================
void RegisterLuaJsonHttpLibs(lua_State* L)
{
    // ── json ─────────────────────────────────────────────────────────────────
    lua_newtable(L);  // json 表

    lua_pushcfunction(L, [](lua_State* Lx) -> int {
        const char* s = luaL_checkstring(Lx, 1);
        auto v = crude_json::value::parse(s);
        pushJsonValue(Lx, v);
        if (lua_isnil(Lx, -1) && std::string(s).find("null") == std::string::npos) {
            lua_pop(Lx, 1);
            lua_pushnil(Lx);
            lua_pushstring(Lx, "JSON parse error");
            return 2;
        }
        return 1;
    });
    lua_setfield(L, -2, "parse");

    lua_pushcfunction(L, [](lua_State* Lx) -> int {
        lua_pushstring(Lx, luaToJson(Lx, 1).dump().c_str());
        return 1;
    });
    lua_setfield(L, -2, "stringify");

    lua_pushcfunction(L, [](lua_State* Lx) -> int {
        const char* s    = luaL_checkstring(Lx, 1);
        const char* path = luaL_checkstring(Lx, 2);
        auto root = crude_json::value::parse(s);
        const crude_json::value* found = getJsonPath(root, path);
        if (!found) { lua_pushnil(Lx); return 1; }
        pushJsonValue(Lx, *found);
        return 1;
    });
    lua_setfield(L, -2, "get");

    lua_pushcfunction(L, [](lua_State* Lx) -> int {
        const char* s   = luaL_checkstring(Lx, 1);
        const char* key = luaL_checkstring(Lx, 2);
        auto root = crude_json::value::parse(s);
        if (!root.is_object()) root = crude_json::value(crude_json::object{});
        root.get<crude_json::object>()[key] = luaToJson(Lx, 3);
        lua_pushstring(Lx, root.dump().c_str());
        return 1;
    });
    lua_setfield(L, -2, "set");

    lua_setglobal(L, "json");

    // ── http ─────────────────────────────────────────────────────────────────
    // 全部异步回调式，调用后立即返回，HTTP 完成时在主线程调用 callback。
    //
    //   http.get(url [, headers], callback)
    //   http.post(url, body [, headers], callback)
    //   http.request(method, url, body, headers, callback)
    //
    //   callback(body: string, statusCode: int, error: string)
    //
    lua_newtable(L);  // http 表

#ifdef __EMSCRIPTEN__
    auto stub = [](lua_State* Lx) -> int {
        // Emscripten 不支持同步/异步 http.*，需用 HTTP.Request 节点
        int cbIdx = lua_gettop(Lx);
        if (lua_isfunction(Lx, cbIdx)) {
            lua_pushvalue(Lx, cbIdx);
            lua_pushnil(Lx);
            lua_pushinteger(Lx, 0);
            lua_pushstring(Lx, "http.* not supported on WebGL; use HTTP.Request node instead");
            lua_pcall(Lx, 3, 0, 0);
        }
        return 0;
    };
    lua_pushcfunction(L, stub); lua_setfield(L, -2, "get");
    lua_pushcfunction(L, stub); lua_setfield(L, -2, "post");
    lua_pushcfunction(L, stub); lua_setfield(L, -2, "request");
#else
    // http.get(url [, headers_table], callback)
    lua_pushcfunction(L, [](lua_State* Lx) -> int {
        const char* url = luaL_checkstring(Lx, 1);
        std::string hdrs = "{}";
        int cbIdx = 2;
        if (lua_istable(Lx, 2)) { hdrs = luaTableToHeadersJson(Lx, 2); cbIdx = 3; }
        luaL_checktype(Lx, cbIdx, LUA_TFUNCTION);
        lua_pushvalue(Lx, cbIdx);
        int ref = luaL_ref(Lx, LUA_REGISTRYINDEX);
        asyncRequest(Lx, "GET", url, "", hdrs, ref);
        return 0;
    });
    lua_setfield(L, -2, "get");

    // http.post(url, body [, headers_table], callback)
    lua_pushcfunction(L, [](lua_State* Lx) -> int {
        const char* url  = luaL_checkstring(Lx, 1);
        const char* body = luaL_optstring(Lx, 2, "");
        std::string hdrs = "{}";
        int cbIdx = 3;
        if (lua_istable(Lx, 3)) { hdrs = luaTableToHeadersJson(Lx, 3); cbIdx = 4; }
        luaL_checktype(Lx, cbIdx, LUA_TFUNCTION);
        lua_pushvalue(Lx, cbIdx);
        int ref = luaL_ref(Lx, LUA_REGISTRYINDEX);
        asyncRequest(Lx, "POST", url, body, hdrs, ref);
        return 0;
    });
    lua_setfield(L, -2, "post");

    // http.request(method, url, body, headers_table_or_jsonstr, callback)
    lua_pushcfunction(L, [](lua_State* Lx) -> int {
        const char* method = luaL_checkstring(Lx, 1);
        const char* url    = luaL_checkstring(Lx, 2);
        const char* body   = luaL_optstring(Lx, 3, "");
        std::string hdrs   = "{}";
        if (lua_istable(Lx, 4))       hdrs = luaTableToHeadersJson(Lx, 4);
        else if (lua_isstring(Lx, 4)) hdrs = lua_tostring(Lx, 4);
        luaL_checktype(Lx, 5, LUA_TFUNCTION);
        lua_pushvalue(Lx, 5);
        int ref = luaL_ref(Lx, LUA_REGISTRYINDEX);
        asyncRequest(Lx, method, url, body, hdrs, ref);
        return 0;
    });
    lua_setfield(L, -2, "request");
#endif

    lua_setglobal(L, "http");
}

} // namespace Runtime
} // namespace NodeEditor

#endif // BLUEPRINT_HAS_LUA
