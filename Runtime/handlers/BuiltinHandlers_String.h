// Runtime/BuiltinHandlers_String.h
#pragma once
#include "../BlueprintRunner.h"
#include <unordered_map>
#include <string>

namespace NodeEditor {
namespace Runtime {

void RegisterHandlers_String(
    std::unordered_map<std::string, NodeHandler>& handlers);

} // namespace Runtime
} // namespace NodeEditor
