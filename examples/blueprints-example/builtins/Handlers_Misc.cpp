// Handlers_Misc.cpp -- Misc 节点处理器注册
#include "../BlueprintEditor.h"

void BlueprintEditor::RegisterHandlers_Misc()
{
    m_HandlerRegistry["GetVariable"] = [](RTContext& ctx) {
        auto name = ctx.GetInputValue("Name").asString();
        ctx.SetOutputValue("Value", ctx.GetVariable(name));
        return true;
    };

    m_HandlerRegistry["SetVariable"] = [](RTContext& ctx) {
        auto name = ctx.GetInputValue("Name").asString();
        auto val = ctx.GetInputValue("Value");
        ctx.SetVariable(name, val);
        ctx.Log("  Set '" + name + "' = '" + val.asString() + "'");
        return true;
    };
}
