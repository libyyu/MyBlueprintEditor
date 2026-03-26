// Runtime/BuiltinHandlers_Data.h
#pragma once
#include "../BlueprintRunner.h"
#include <unordered_map>
#include <string>

namespace NodeEditor {
namespace Runtime {

void RegisterHandlers_Data(
    std::unordered_map<std::string, NodeHandler>& handlers);

} // namespace Runtime
} // namespace NodeEditor
