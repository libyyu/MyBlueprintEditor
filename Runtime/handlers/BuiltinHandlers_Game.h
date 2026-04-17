// Runtime/handlers/BuiltinHandlers_Game.h
#pragma once
#include <unordered_map>
#include <string>
#include "../BlueprintRunner.h"

namespace NodeEditor {
namespace Runtime {

// 行为树（BT.*）、状态机（FSM.*）节点
void RegisterHandlers_Game(
    std::unordered_map<std::string, NodeHandler>& handlers,
    BlueprintRunner& runner);

} // namespace Runtime
} // namespace NodeEditor
