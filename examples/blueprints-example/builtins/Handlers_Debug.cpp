// Handlers_Debug.cpp -- Debug 节点处理器注册
#include "../BlueprintEditor.h"

void BlueprintEditor::RegisterHandlers_Debug()
{
    m_HandlerRegistry["PrintString"] = [](RTContext& ctx) {
        auto str = ctx.GetInputValue("In String").asString();
        ctx.Log("  >>> Print: \"" + str + "\"");
        return true;
    };

    m_HandlerRegistry["AppendString"] = [](RTContext& ctx) {
        auto a = ctx.GetInputValue("A").asString();
        auto b = ctx.GetInputValue("B").asString();
        ctx.SetOutputValue("Result", RTVariant(a + b));
        return true;
    };

    m_HandlerRegistry["StringLength"] = [](RTContext& ctx) {
        auto s = ctx.GetInputValue("String").asString();
        ctx.SetOutputValue("Length", RTVariant(static_cast<int64_t>(s.size())));
        return true;
    };

    m_HandlerRegistry["MakeString"] = [](RTContext& ctx) {
        auto v = ctx.GetInputValue("Value").asString();
        ctx.SetOutputValue("String", RTVariant(v));
        return true;
    };

    m_HandlerRegistry["Log"] = [](RTContext& ctx) {
        auto msg = ctx.GetInputValue("Message").asString();
        ctx.Log("  [LOG] " + msg);
        return true;
    };
}
