// Runtime/LuaLib_Json.cpp
// json.* Lua 全局库实现
//
//   json.parse(str)           → table | nil, errMsg
//   json.stringify(val)       → string
//   json.get(str, path)       → value | nil   ("choices[0].message.content")
//   json.set(str, key, val)   → string         (顶层 key 写入)

#ifdef BLUEPRINT_HAS_LUA

#include "LuaLib_Internal.h"
#include "LuaBindings.h"

namespace NodeEditor {
namespace Runtime {

void RegisterLuaJsonLib(lua_State* L)
{
    lua_newtable(L);

    // json.parse(str) → table | nil, errMsg
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

    // json.stringify(val) → string
    lua_pushcfunction(L, [](lua_State* Lx) -> int {
        lua_pushstring(Lx, luaToJson(Lx, 1).dump().c_str());
        return 1;
    });
    lua_setfield(L, -2, "stringify");

    // json.get(str, path) → value | nil
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

    // json.set(str, key, val) → string
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
}

} // namespace Runtime
} // namespace NodeEditor

#endif // BLUEPRINT_HAS_LUA
