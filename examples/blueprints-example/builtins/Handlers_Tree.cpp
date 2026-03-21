// Handlers_Tree.cpp -- Behavior Tree 节点处理器注册
#include "../BlueprintEditor.h"

void BlueprintEditor::RegisterHandlers_Tree()
{
    m_HandlerRegistry["Sequence"] = [](RTContext& ctx) {
        ctx.Log("  [Sequence] executing all outputs");
        return true;
    };

    m_HandlerRegistry["MoveTo"] = [](RTContext& ctx) {
        ctx.Log("  [Task] Moving to target...");
        return true;
    };

    m_HandlerRegistry["RandomWait"] = [](RTContext& ctx) {
        ctx.Log("  [Task] Waiting random time...");
        return true;
    };
}
