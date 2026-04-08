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
}

} // namespace Runtime
} // namespace NodeEditor

#endif // BLUEPRINT_HAS_LUA
