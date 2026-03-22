// Handlers_Action.cpp -- Action 节点处理器注册
#include "../BlueprintEditor.h"

void BlueprintEditor::RegisterHandlers_Action()
{
    m_HandlerRegistry["SetTimer"] = [this](RTContext& ctx) {
        double time = ctx.GetInputValue("Time").asFloat();
        bool looping = ctx.GetInputValue("Looping").asBool();
        float interval = static_cast<float>(time);
        int repeat = looping ? -1 : 1;  // -1 = infinite loop

        // Capture the PinId of "Function Name" input pin at registration time,
        // so the timer callback can fire the connected callback node later.
        // RTContext::GetPinId() returns NodeEditor::Runtime::PinId (uint64_t), not ed::PinId
        auto funcPinId = ctx.GetPinId("Function Name");  // NodeEditor::Runtime::PinId

        ctx.Log("  [SetTimer] interval=" + std::to_string(interval) + "s, looping=" + (looping ? "true" : "false"));

        // Register a main-thread timer via FrameTimerManager
        auto timerHandle = ctx.SetTimer(interval, repeat, [this, funcPinId, looping]() {
            m_ExecutionLog.push_back("[Timer] fired! looping=" + std::string(looping ? "true" : "false"));
            m_ExecutionLogDirty = true;

            // Fire the node connected to the "Function Name" pin
            // InvalidPinId == 0 in NodeEditor::Runtime
            if (funcPinId != 0)
                m_PersistentRunner.FireConnectedNode(funcPinId);

            return looping; // return true to keep looping
        });

        // 输出 TimerHandle，供 RemoveTimer/PauseTimer/ResumeTimer 使用
        ctx.SetOutputValue("TimerHandle", RTVariant(static_cast<int64_t>(timerHandle)));

        ctx.Log("  [SetTimer] TimerHandle=" + std::to_string(timerHandle));

        // Activate the output flow pin to continue downstream execution
        ctx.ActivateOutputFlow("Exec");

        return true;
    };

    // ==================================================================
    // RemoveTimer — 取消指定的计时器
    // ==================================================================
    m_HandlerRegistry["RemoveTimer"] = [this](RTContext& ctx) {
        int64_t handleVal = ctx.GetInputValue("TimerHandle").asInt();
        auto handle = static_cast<RTTimerHandle>(handleVal);

        ctx.Log("  [RemoveTimer] TimerHandle=" + std::to_string(handle));

        bool success = false;
        if (handle != 0)
            success = m_PersistentRunner.GetTimerManager().ClearTimer(handle);

        ctx.SetOutputValue("Success", RTVariant(success));
        ctx.Log("  [RemoveTimer] " + std::string(success ? "Removed" : "Not found or invalid"));

        ctx.ActivateOutputFlow("Exec");
        return true;
    };

    // ==================================================================
    // PauseTimer — 暂停指定的计时器
    // ==================================================================
    m_HandlerRegistry["PauseTimer"] = [this](RTContext& ctx) {
        int64_t handleVal = ctx.GetInputValue("TimerHandle").asInt();
        auto handle = static_cast<RTTimerHandle>(handleVal);

        ctx.Log("  [PauseTimer] TimerHandle=" + std::to_string(handle));

        bool success = false;
        if (handle != 0)
            success = m_PersistentRunner.GetTimerManager().PauseTimer(handle);

        ctx.SetOutputValue("Success", RTVariant(success));
        ctx.Log("  [PauseTimer] " + std::string(success ? "Paused" : "Not found or invalid"));

        ctx.ActivateOutputFlow("Exec");
        return true;
    };

    // ==================================================================
    // ResumeTimer — 恢复指定的计时器
    // ==================================================================
    m_HandlerRegistry["ResumeTimer"] = [this](RTContext& ctx) {
        int64_t handleVal = ctx.GetInputValue("TimerHandle").asInt();
        auto handle = static_cast<RTTimerHandle>(handleVal);

        ctx.Log("  [ResumeTimer] TimerHandle=" + std::to_string(handle));

        bool success = false;
        if (handle != 0)
            success = m_PersistentRunner.GetTimerManager().ResumeTimer(handle);

        ctx.SetOutputValue("Success", RTVariant(success));
        ctx.Log("  [ResumeTimer] " + std::string(success ? "Resumed" : "Not found or invalid"));

        ctx.ActivateOutputFlow("Exec");
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

    // CustomEvent: called by FireConnectedNode when a timer (or other trigger) fires.
    // It activates the "Exec" output flow pin to drive downstream nodes.
    m_HandlerRegistry["CustomEvent"] = [](RTContext& ctx) {
        ctx.Log("  [CustomEvent] triggered");
        ctx.ActivateOutputFlow("Exec");
        return true;
    };

    m_HandlerRegistry["TraceByChannel"] = [](RTContext& ctx) {
        ctx.Log("  [Trace] Line trace performed");
        ctx.SetOutputValue("Return Value", RTVariant(true));
        return true;
    };
}
