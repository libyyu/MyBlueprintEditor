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
// 多 Runner 共享 lua_State 时的当前 Runner 路由
// ----------------------------------------------------------------------
// 当多个 BlueprintRunner 共享同一个 lua_State 时，Blueprint.RegisterHandler、
// print/warn/printerror 等 binding 通过 LUA_REGISTRYINDEX["__blueprint_runner"]
// 找当前 Runner。共享场景下这个字段必须由各 Runner 在做 Lua 操作前更新为自己。
//
// RegisterLuaBindings 也会写入此字段（首次初始化），但之后不会自动维护。
// BlueprintRunner 内部在 LoadExtensionScript / Execute 等入口前调 BindRunnerToLuaState。

BLUEPRINT_API void BindRunnerToLuaState(lua_State* L, BlueprintRunner* runner);

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

// 内部使用：强制 LuaStateRegistry singleton 在调用方之前构造，
// 用于解决进程退出时的析构顺序问题（让 default Lua engine 比 registry 先死）。
BLUEPRINT_API void TouchLuaStateRegistry();

} // namespace Runtime
} // namespace NodeEditor

#endif // BLUEPRINT_HAS_LUA
