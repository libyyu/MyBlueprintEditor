// Runtime/LuaLib_JsonHttp.cpp
// Lua 全局库注册入口：依次注册 json.* / http.* / file.*
//
// 各模块实现分别在：
//   LuaLib_Json.cpp  — json.parse / stringify / get / set
//   LuaLib_Http.cpp  — http.get / post / request
//   LuaLib_File.cpp  — file.read / write / append / exists / delete /
//                       size / listdir / mkdir / basename / dirname / join

#ifdef BLUEPRINT_HAS_LUA

#include "LuaBindings.h"
#include <lua.hpp>

#ifdef WITH_LUASOCKET
	// 额外注册 luasocket 库（如果编译时包含了）
extern int luaopen_socket_core(lua_State* L);
extern int luaopen_mime_core(lua_State* L);
#endif

namespace NodeEditor {
namespace Runtime {

// 各模块子注册函数（由对应 .cpp 实现）
void RegisterLuaJsonLib(lua_State* L);
void RegisterLuaHttpLib(lua_State* L);
void RegisterLuaFileLib(lua_State* L);

void RegisterLuaJsonHttpLibs(lua_State* L)
{
    RegisterLuaJsonLib(L);
    RegisterLuaHttpLib(L);
    RegisterLuaFileLib(L);

#ifdef WITH_LUASOCKET
    // 额外注册 luasocket 库（如果编译时包含了）
	luaL_requiref(L, "luasocket", luaopen_socket_core, 0);
	luaL_requiref(L, "luasocket.core", luaopen_socket_core, 0);
	luaL_requiref(L, "luasocket.mime", luaopen_mime_core, 0);
#endif

}

} // namespace Runtime
} // namespace NodeEditor

#endif // BLUEPRINT_HAS_LUA
