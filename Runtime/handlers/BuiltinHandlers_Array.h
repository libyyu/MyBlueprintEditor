// Runtime/BuiltinHandlers_Array.h
#pragma once
#include "../BlueprintRunner.h"
#include <unordered_map>
#include <string>

namespace NodeEditor {
namespace Runtime {

void RegisterHandlers_Array(
    std::unordered_map<std::string, NodeHandler>& handlers);

} // namespace Runtime
} // namespace NodeEditor
