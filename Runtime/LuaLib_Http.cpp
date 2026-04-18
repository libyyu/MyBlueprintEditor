// Runtime/LuaLib_Http.cpp
// http.* Lua 全局库实现（异步回调式）
//
//   http.get(url [, headers_table], callback)
//   http.post(url, body [, headers_table], callback)
//   http.request(method, url, body, headers_table_or_str, callback)
//
//   callback(body: string, statusCode: int, error: string)
//
// 跨平台：native + WebGL（底层走 IHttpClient::SendAsync）
//
// 注意：pending 工作计数由 Lua 脚本自行管理：
//   在发起请求前调用 Blueprint.AcquireAsync()
//   在回调完成后调用 Blueprint.ReleaseAsync()
//   这样 runner.HasPendingWork() 才能感知到 Lua 层的异步活动。

#ifdef BLUEPRINT_HAS_LUA

#include "LuaLib_Internal.h"
#include "LuaBindings.h"
#include "Http/IHttpClient.h"
#include "MainThreadDispatcher.h"

#include <cstdio>

namespace NodeEditor {
namespace Runtime {

// 通用异步请求分发：构造 HttpRequest，调用 SendAsync；
// 回调由 MainThreadDispatcher::Post 派回主线程后调用 Lua callback（cbRef）。
static void asyncRequest(lua_State* L,
                         const std::string& method,
                         const std::string& url,
                         const std::string& body,
                         const std::string& headersJson,
                         int cbRef)
{
    auto* client = BP_GetHttpClient();
    if (!client) {
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

    client->SendAsync(req, [L, cbRef](HttpResponse resp) {
        std::string respBody  = resp.body;
        int         respCode  = resp.statusCode;
        std::string respError = resp.error;

        MainThreadDispatcher::Get().Post([L, cbRef,
                                          respBody  = std::move(respBody),
                                          respCode,
                                          respError = std::move(respError)]() {
            lua_rawgeti(L, LUA_REGISTRYINDEX, cbRef);
            luaL_unref(L, LUA_REGISTRYINDEX, cbRef);
            lua_pushstring(L, respBody.c_str());
            lua_pushinteger(L, respCode);
            lua_pushstring(L, respError.c_str());
            if (lua_pcall(L, 3, 0, 0) != LUA_OK) {
                const char* err = lua_tostring(L, -1);
                fprintf(stderr, "[http callback] Lua error: %s\n", err ? err : "(unknown)");
                lua_pop(L, 1);
            }
        });
    });
}

void RegisterLuaHttpLib(lua_State* L)
{
    lua_newtable(L);

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

    lua_setglobal(L, "http");
}

} // namespace Runtime
} // namespace NodeEditor

#endif // BLUEPRINT_HAS_LUA
