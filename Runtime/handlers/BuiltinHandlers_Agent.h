// Runtime/handlers/BuiltinHandlers_Agent.h
#pragma once
#include <unordered_map>
#include <string>
#include "../BlueprintRunner.h"

namespace NodeEditor {
namespace Runtime {

// Memory.Store / Memory.Recall / Tool.Register
// Trigger.Cron / Trigger.FileWatch
void RegisterHandlers_Agent(
    std::unordered_map<std::string, NodeHandler>& handlers);

} // namespace Runtime
} // namespace NodeEditor
