// Handlers_Action.cpp -- Action 节点处理器注册
#include "../BlueprintEditor.h"

void BlueprintEditor::RegisterHandlers_Action()
{
    m_HandlerRegistry["SetTimer"] = [this](RTContext& ctx) {
        double time = ctx.GetInputValue("Time").asFloat();
        bool looping = ctx.GetInputValue("Looping").asBool();
        float interval = static_cast<float>(time);
        int repeat = looping ? -1 : 1;  // -1 = 无限循环

        ctx.Log("  [SetTimer] interval=" + std::to_string(interval) + "s, looping=" + (looping ? "true" : "false"));

        // 用 FrameTimerManager 注册一个主线程计时器
        GetTimerManager().SetTimerByName("SetTimer", interval, repeat, [this, looping]() {
            m_ExecutionLog.push_back("[Timer] fired! looping=" + std::string(looping ? "true" : "false"));
            m_ExecutionLogDirty = true;
            return true; // 返回 true 继续循环
        });

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
