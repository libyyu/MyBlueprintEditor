// Runtime/handlers/BuiltinHandlers_File.h
#pragma once
#include <unordered_map>
#include <string>
#include <functional>

namespace NodeEditor {
namespace Runtime {

class ExecutionContext;
using NodeHandler = std::function<bool(ExecutionContext&)>;

void RegisterHandlers_File(
    std::unordered_map<std::string, NodeHandler>& handlers);

} // namespace Runtime
} // namespace NodeEditor
