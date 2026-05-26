// Runtime/handlers/BuiltinHandlers_GameMath.h
#pragma once
#include <unordered_map>
#include <string>
#include "../BlueprintRunner.h"

namespace NodeEditor {
namespace Runtime {

// Vec2/Vec3 数学、物理碰撞辅助、数值系统 Stat/Cooldown
void RegisterHandlers_GameMath(
    std::unordered_map<std::string, NodeHandler>& handlers);

} // namespace Runtime
} // namespace NodeEditor
