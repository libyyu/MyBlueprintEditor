// Runtime/BuiltinHandlers_Houdini.cpp -- Houdini 节点处理器
#include "BuiltinHandlers_Houdini.h"

namespace NodeEditor {
namespace Runtime {

void RegisterHandlers_Houdini(std::unordered_map<std::string, NodeHandler>& handlers)
{
    handlers["HoudiniTransform"] = [](ExecutionContext& ctx) {
        ctx.Log("  [Houdini] Transform applied");
        return true;
    };

    handlers["HoudiniGroup"] = [](ExecutionContext& ctx) {
        ctx.Log("  [Houdini] Group created");
        return true;
    };
}

// ============================================================================
// Time 处理器
// ============================================================================

} // namespace Runtime
} // namespace NodeEditor
