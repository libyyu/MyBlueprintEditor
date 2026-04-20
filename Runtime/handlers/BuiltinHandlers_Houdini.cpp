// Runtime/BuiltinHandlers_Houdini.cpp -- Houdini 节点处理器（历史占位，未实现）
//
// 注意：这些节点是早期遗留存根，handler 无实际逻辑。
//   HoudiniTransform / HoudiniGroup 均只打印警告并返回 success。
//   如需类似功能，使用 Game/BehaviorTree 系列节点（BT.*）。
//
#include "BuiltinHandlers_Houdini.h"

namespace NodeEditor {
namespace Runtime {

void RegisterHandlers_Houdini(std::unordered_map<std::string, NodeHandler>& handlers)
{
    handlers["HoudiniTransform"] = [](ExecutionContext& ctx) {
        ctx.LogWarning("[Legacy] HoudiniTransform: not implemented. Node passes through.");
        ctx.ActivateOutputFlow("");
        return true;
    };

    handlers["HoudiniGroup"] = [](ExecutionContext& ctx) {
        ctx.LogWarning("[Legacy] HoudiniGroup: not implemented. Node passes through.");
        ctx.ActivateOutputFlow("");
        return true;
    };
}

} // namespace Runtime
} // namespace NodeEditor
