// Handlers_Action.cpp -- Action 节点处理器注册
#include "../BlueprintEditor.h"

void BlueprintEditor::RegisterHandlers_Action()
{
    m_HandlerRegistry["SetTimer"] = [](RTContext& ctx) {
        double time = ctx.GetInputValue("Time").asFloat();
        bool looping = ctx.GetInputValue("Looping").asBool();
        ctx.Log("  Timer: " + std::to_string(time) + "s, Looping=" + (looping ? "true" : "false"));
        return true;
    };

    m_HandlerRegistry["OutputAction"] = [](RTContext& ctx) {
        double sample = ctx.GetInputValue("Sample").asFloat();
        ctx.Log("  Sample = " + std::to_string(sample));
        ctx.SetOutputValue("Condition", RTVariant(sample > 0.5));
        return true;
    };

    m_HandlerRegistry["InputActionFire"] = [](RTContext& ctx) {
        ctx.Log("  [InputAction] Fire triggered");
        return true;
    };

    m_HandlerRegistry["TraceByChannel"] = [](RTContext& ctx) {
        ctx.Log("  [Trace] Line trace performed");
        ctx.SetOutputValue("Return Value", RTVariant(true));
        return true;
    };
}
