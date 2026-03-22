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
        GetTimerManager().SetTimer(interval, repeat, [this, funcPinId, looping]() {
            m_ExecutionLog.push_back("[Timer] fired! looping=" + std::string(looping ? "true" : "false"));
            m_ExecutionLogDirty = true;

            // Fire the node connected to the "Function Name" pin
            // InvalidPinId == 0 in NodeEditor::Runtime
            if (funcPinId != 0)
                m_PersistentRunner.FireConnectedNode(funcPinId);

            return looping; // return true to keep looping
        });

        // Activate the output flow pin to continue downstream execution
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
