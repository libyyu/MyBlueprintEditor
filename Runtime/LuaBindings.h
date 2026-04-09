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

// 只注册 Variant / ExecutionContext metatables，不创建/覆盖 Blueprint 全局表
// 供编辑器侧 LuaNodeRegistrar 使用（编辑器自己管理 Blueprint 表）
BLUEPRINT_API void RegisterLuaMetatables(lua_State* L);

// 注册 json.* 、http.* 和 file.* Lua 全局库（需在 RegisterLuaBindings 之后调用）
BLUEPRINT_API void RegisterLuaJsonHttpLibs(lua_State* L);

} // namespace Runtime
} // namespace NodeEditor

#endif // BLUEPRINT_HAS_LUA
