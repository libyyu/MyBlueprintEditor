// Runtime/handlers/BuiltinHandlers_Socket.h
#pragma once
#include <unordered_map>
#include <string>
#include "../BlueprintRunner.h"

namespace NodeEditor {
namespace Runtime {

// TCP/UDP 节点：
//   TCP.Listen  / TCP.Connect / TCP.Send / TCP.Receive / TCP.Disconnect / TCP.Stop
//   UDP.Bind    / UDP.Send    / UDP.Receive / UDP.Close
void RegisterHandlers_Socket(
    std::unordered_map<std::string, NodeHandler>& handlers,
    BlueprintRunner& runner);

} // namespace Runtime
} // namespace NodeEditor
