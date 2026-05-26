// Runtime/nodedefs/BuiltinNodeDefs_AI.cpp
// Dispatcher for AI node-def registration. The actual reg(...) calls are split
// across four sub-files mirroring the handlers/ split:
//
//   BuiltinNodeDefs_AI_JSON.cpp   — JSON.*  + Schema.*                            (10 nodes)
//   BuiltinNodeDefs_AI_LLM.cpp    — LLM.*   + String.Template + Intent + Context  ( 9 nodes)
//   BuiltinNodeDefs_AI_Tool.cpp   — Tool.*  + MCP.Call + HTTP.Retry               ( 8 nodes)
//   BuiltinNodeDefs_AI_Agent.cpp  — Agent.* + Memory.* + Trigger.* + UserInput    (12 nodes)
//
// Total: 39 AI nodes. Order of registration is preserved per group.
#include "BuiltinNodeDefs_Internal.h"

namespace NodeEditor {
namespace Runtime {

void RegisterNodeDefs_AI(INodeRegistry& registry)
{
    RegisterNodeDefs_AI_JSON(registry);
    RegisterNodeDefs_AI_LLM(registry);
    RegisterNodeDefs_AI_Tool(registry);
    RegisterNodeDefs_AI_Agent(registry);
}

} // namespace Runtime
} // namespace NodeEditor
