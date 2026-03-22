// Handlers_Debug.cpp -- Debug 节点处理器注册（PrintString, Log）
#include "../BlueprintEditor.h"

void BlueprintEditor::RegisterHandlers_Debug()
{
    m_HandlerRegistry["PrintString"] = [](RTContext& ctx) {
        auto str = ctx.GetInputValue("In String").asString();
        ctx.Log("  >>> Print: \"" + str + "\"");
        return true;
    };

    m_HandlerRegistry["Log"] = [](RTContext& ctx) {
        auto msg = ctx.GetInputValue("Message").asString();
        ctx.Log("  [LOG] " + msg);
        return true;
    };
}
