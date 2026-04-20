// Runtime/BuiltinHandlers_Tree.cpp -- Tree 节点处理器（历史占位，未实现）
//
// 注意：这些节点是早期遗留存根，handler 无实际逻辑。
//   Sequence / MoveTo / RandomWait 均只打印警告并返回 success。
//   请使用完整的 Game/BehaviorTree 系列节点（BT.Sequence / BT.Wait 等）。
//
#include "BuiltinHandlers_Tree.h"

namespace NodeEditor {
namespace Runtime {

void RegisterHandlers_Tree(std::unordered_map<std::string, NodeHandler>& handlers)
{
    handlers["Sequence"] = [](ExecutionContext& ctx) {
        ctx.LogWarning("[Legacy] Sequence: not implemented. Use BT.Sequence instead.");
        ctx.ActivateOutputFlow("");
        return true;
    };

    handlers["MoveTo"] = [](ExecutionContext& ctx) {
        ctx.LogWarning("[Legacy] MoveTo: not implemented. Implement via Lua or C++ handler.");
        ctx.ActivateOutputFlow("");
        return true;
    };

    handlers["RandomWait"] = [](ExecutionContext& ctx) {
        ctx.LogWarning("[Legacy] RandomWait: not implemented. Use BT.Wait + Random.Float instead.");
        ctx.ActivateOutputFlow("");
        return true;
    };
}

} // namespace Runtime
} // namespace NodeEditor
