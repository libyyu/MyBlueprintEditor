// Handlers_Houdini.cpp -- Houdini 节点处理器注册
#include "../BlueprintEditor.h"

void BlueprintEditor::RegisterHandlers_Houdini()
{
    m_HandlerRegistry["HoudiniTransform"] = [](RTContext& ctx) {
        ctx.Log("  [Houdini] Transform applied");
        return true;
    };

    m_HandlerRegistry["HoudiniGroup"] = [](RTContext& ctx) {
        ctx.Log("  [Houdini] Group created");
        return true;
    };
}
