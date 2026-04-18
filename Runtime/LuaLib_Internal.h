// Runtime/LuaLib_Internal.h
// Lua 库模块内部共享工具函数（仅供 LuaLib_*.cpp 使用）
//
// 提供：
//   pushJsonValue  — crude_json::value → Lua 栈
//   luaToJson      — Lua 栈 → crude_json::value
//   getJsonPath    — JSON 路径遍历 "a.b[0].c"
//   luaTableToHeadersJson — Lua table → JSON headers 字符串
#pragma once

#ifdef BLUEPRINT_HAS_LUA

#include "../../Utils/Json/crude_json.h"
#include <lua.hpp>
#include <string>

namespace NodeEditor {
namespace Runtime {

// ── crude_json ↔ Lua 转换 ──────────────────────────────────────────────────

inline void pushJsonValue(lua_State* L, const crude_json::value& v)
{
    if (v.is_null())    { lua_pushnil(L);  return; }
    if (v.is_boolean()) { lua_pushboolean(L, (int)v.get<bool>());           return; }
    if (v.is_number())  { lua_pushnumber(L, v.get<double>());               return; }
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

inline crude_json::value luaToJson(lua_State* L, int idx)
{
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
        lua_pushnil(L);
        bool isArr = true; int n = 0;
        while (lua_next(L, abs)) {
            if (!lua_isinteger(L, -2)) { isArr = false; lua_pop(L, 2); break; }
            ++n; lua_pop(L, 1);
        }
        if (isArr) {
            // 空表 {} 也序列化为 JSON array []，而非 object {}
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
            if (lua_type(L, -2) == LUA_TSTRING)  key = lua_tostring(L, -2);
            else if (lua_isinteger(L, -2))        key = std::to_string(lua_tointeger(L, -2));
            obj[key] = luaToJson(L, -1);
            lua_pop(L, 1);
        }
        return crude_json::value(std::move(obj));
    }
    return crude_json::value();
}

// 路径遍历：支持 "a.b[0].c"
inline const crude_json::value* getJsonPath(const crude_json::value& root,
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

// ── HTTP headers 辅助 ──────────────────────────────────────────────────────

// 把 Lua table（栈上 idx）转为 JSON headers 字符串 {"K":"V",...}
inline std::string luaTableToHeadersJson(lua_State* L, int idx)
{
    // JSON 字符串最小转义（" 和 \）
    auto jsonEscape = [](const std::string& s) -> std::string {
        std::string out;
        out.reserve(s.size());
        for (char c : s) {
            if      (c == '"')  out += "\\\"";
            else if (c == '\\') out += "\\\\";
            else if (c == '\n') out += "\\n";
            else if (c == '\r') out += "\\r";
            else if (c == '\t') out += "\\t";
            else                out += c;
        }
        return out;
    };

    int abs = (idx > 0 || idx <= LUA_REGISTRYINDEX) ? idx : lua_gettop(L) + idx + 1;
    std::string h = "{"; bool first = true;
    lua_pushnil(L);
    while (lua_next(L, abs)) {
        if (!first) h += ",";
        first = false;
        h += "\"";
        if (lua_type(L, -2) == LUA_TSTRING)
            h += jsonEscape(lua_tostring(L, -2));
        h += "\":\"";
        if (lua_type(L, -1) == LUA_TSTRING)
            h += jsonEscape(lua_tostring(L, -1));
        h += "\"";
        lua_pop(L, 1);
    }
    return h + "}";
}

} // namespace Runtime
} // namespace NodeEditor

#endif // BLUEPRINT_HAS_LUA
