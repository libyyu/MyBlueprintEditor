// Runtime/BuiltinHandlers_Houdini.h
#pragma once
#include "../BlueprintRunner.h"
#include <unordered_map>
#include <string>

namespace NodeEditor {
namespace Runtime {

void RegisterHandlers_Houdini(
    std::unordered_map<std::string, NodeHandler>& handlers);

} // namespace Runtime
} // namespace NodeEditor
