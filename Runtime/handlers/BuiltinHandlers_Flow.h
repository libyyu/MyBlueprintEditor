// Runtime/BuiltinHandlers_Flow.h
#pragma once
#include "../BlueprintRunner.h"
#include <unordered_map>
#include <string>

namespace NodeEditor {
namespace Runtime {

void RegisterHandlers_Flow(
    std::unordered_map<std::string, NodeHandler>& handlers,
    BlueprintRunner& runner,
    const std::string& basePath);

} // namespace Runtime
} // namespace NodeEditor
