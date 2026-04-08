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

} // namespace Runtime
} // namespace NodeEditor

#endif // BLUEPRINT_HAS_LUA
