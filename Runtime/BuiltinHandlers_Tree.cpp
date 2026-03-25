// Runtime/BuiltinHandlers_Tree.cpp -- Tree 节点处理器
#include "BuiltinHandlers_Tree.h"

namespace NodeEditor {
namespace Runtime {

void RegisterHandlers_Tree(std::unordered_map<std::string, NodeHandler>& handlers)
{
    handlers["Sequence"] = [](ExecutionContext& ctx) {
        ctx.Log("  [Sequence] executing all outputs");
        return true;
    };

    handlers["MoveTo"] = [](ExecutionContext& ctx) {
        ctx.Log("  [Task] Moving to target...");
        return true;
    };

    handlers["RandomWait"] = [](ExecutionContext& ctx) {
        ctx.Log("  [Task] Waiting random time...");
        return true;
    };
}

// ============================================================================
// Houdini 处理器
// ============================================================================

} // namespace Runtime
} // namespace NodeEditor
