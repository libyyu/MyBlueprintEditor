// Runtime/LuaBindings.h -- Lua ↔ C++ 绑定（ExecutionContext / Variant / Blueprint API）
#pragma once

#ifdef BLUEPRINT_HAS_LUA

#include "BlueprintExport.h"

struct lua_State;

namespace NodeEditor {
namespace Runtime {

class BlueprintRunner;

// 注册 Variant/ExecutionContext metatable + Blueprint 全局表
BLUEPRINT_API void RegisterLuaBindings(lua_State* L, BlueprintRunner* runner);

// 注册 json.* 、http.* 和 file.* Lua 全局库（需在 RegisterLuaBindings 之后调用）
BLUEPRINT_API void RegisterLuaJsonHttpLibs(lua_State* L);

// ----------------------------------------------------------------------
// Any Variant 的 lua_State 生命周期通告
// ----------------------------------------------------------------------
// 必须在 lua_close(L) 之前调用 UnregisterLuaState(L)，否则残留的
// Any Variant 在 VM 关闭后析构会因 luaL_unref 访问野指针而 UAF。
//
// Register 是可选的：首次执行 ctx:SetOutputAny / makeAnyFromLuaIndex 时
// 内部会自动注册；提前显式注册仅用于线程安全场景的预热。

BLUEPRINT_API void RegisterLuaState(lua_State* L);
BLUEPRINT_API void UnregisterLuaState(lua_State* L);

} // namespace Runtime
} // namespace NodeEditor

#endif // BLUEPRINT_HAS_LUA
