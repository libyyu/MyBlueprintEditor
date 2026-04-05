// Runtime/handlers/BuiltinHandlers_Network.h
#pragma once
#include <unordered_map>
#include <string>
#include <functional>

namespace NodeEditor {
namespace Runtime {

class BlueprintRunner;
class ExecutionContext;
using NodeHandler = std::function<bool(ExecutionContext&)>;

void RegisterHandlers_Network(
    std::unordered_map<std::string, NodeHandler>& handlers,
    BlueprintRunner& runner);

} // namespace Runtime
} // namespace NodeEditor
