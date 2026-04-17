// Runtime/handlers/BuiltinHandlers_Save.h
#pragma once
#include <unordered_map>
#include <string>
#include "../BlueprintRunner.h"

namespace NodeEditor {
namespace Runtime {

// 存档系统 Save.* 节点
void RegisterHandlers_Save(
    std::unordered_map<std::string, NodeHandler>& handlers);

} // namespace Runtime
} // namespace NodeEditor
