// Runtime/LuaBindings.h -- Lua ↔ C++ 绑定（ExecutionContext / Variant / Blueprint API）
//
// 职责：
//   · Variant ↔ Lua 类型双向转换
//   · ExecutionContext Lua metatable 绑定
//   · Blueprint.RegisterHandler() Lua API
//
#pragma once

#ifdef BLUEPRINT_HAS_LUA

#include "BlueprintExport.h"

struct lua_State;

namespace NodeEditor {
namespace Runtime {

class BlueprintRunner;

// 注册所有 Lua 绑定（Variant metatable + ExecutionContext metatable + Blueprint 全局表）
// 在 LuaScriptEngine::Initialize 中调用
BLUEPRINT_API void RegisterLuaBindings(lua_State* L, BlueprintRunner* runner);

} // namespace Runtime
} // namespace NodeEditor

#endif // BLUEPRINT_HAS_LUA
