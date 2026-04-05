// Runtime/handlers/BuiltinHandlers_AI.h
#pragma once
#include <unordered_map>
#include <string>
#include "../BlueprintRunner.h"

namespace NodeEditor {
namespace Runtime {

void RegisterHandlers_AI(
    std::unordered_map<std::string, NodeHandler>& handlers,
    BlueprintRunner& runner);

} // namespace Runtime
} // namespace NodeEditor
