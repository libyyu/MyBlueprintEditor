// Runtime/BuiltinHandlers_Action.cpp -- Action 类型节点处理器
#include "BuiltinHandlers_Action.h"

namespace NodeEditor {
namespace Runtime {

void RegisterHandlers_Action(
    std::unordered_map<std::string, NodeHandler>& handlers,
    BlueprintRunner& runner)
{
    // SetTimer — 依赖：runner（timer 管理器）
    // 注意：FireConnectedNode 通过 ctx.FireConnectedNode() 调用，
    //       这样在子蓝图中使用时会在正确的 runner 上查找节点
    handlers["SetTimer"] = [](ExecutionContext& ctx) {
        double time = ctx.GetInputValue("Time").asFloat();
        bool looping = ctx.GetInputValue("Looping").asBool();
        float interval = static_cast<float>(time);
        int repeat = looping ? -1 : 1;

        auto funcPinId = ctx.GetPinId("Function Name");

        ctx.Log("  [SetTimer] interval=" + std::to_string(interval) + "s, looping=" + (looping ? "true" : "false"));

        ExecutionContext* pCtx = &ctx;
        auto timerHandle = ctx.SetTimer(interval, repeat, [pCtx, funcPinId, looping]() {
            pCtx->Log("[Timer] fired! looping=" + std::string(looping ? "true" : "false"));
            if (funcPinId != 0)
                pCtx->FireConnectedNode(funcPinId);

            return looping;
        });

        ctx.SetOutputValue("TimerHandle", Variant(static_cast<int64_t>(timerHandle)));
        ctx.Log("  [SetTimer] TimerHandle=" + std::to_string(timerHandle));
        ctx.ActivateOutputFlow("Exec");

        return true;
    };

    // RemoveTimer — 依赖：runner（timer 管理器）
    handlers["RemoveTimer"] = [&runner](ExecutionContext& ctx) {
        int64_t handleVal = ctx.GetInputValue("TimerHandle").asInt();
        auto handle = static_cast<TimerHandle>(handleVal);

        ctx.Log("  [RemoveTimer] TimerHandle=" + std::to_string(handle));

        bool success = false;
        if (handle != 0)
            success = runner.GetTimerManager().ClearTimer(handle);

        ctx.SetOutputValue("Success", Variant(success));
        ctx.Log("  [RemoveTimer] " + std::string(success ? "Removed" : "Not found or invalid"));

        ctx.ActivateOutputFlow("Exec");
        return true;
    };

    // PauseTimer — 依赖：runner（timer 管理器）
    handlers["PauseTimer"] = [&runner](ExecutionContext& ctx) {
        int64_t handleVal = ctx.GetInputValue("TimerHandle").asInt();
        auto handle = static_cast<TimerHandle>(handleVal);

        ctx.Log("  [PauseTimer] TimerHandle=" + std::to_string(handle));

        bool success = false;
        if (handle != 0)
            success = runner.GetTimerManager().PauseTimer(handle);

        ctx.SetOutputValue("Success", Variant(success));
        ctx.Log("  [PauseTimer] " + std::string(success ? "Paused" : "Not found or invalid"));

        ctx.ActivateOutputFlow("Exec");
        return true;
    };

    // ResumeTimer — 依赖：runner（timer 管理器）
    handlers["ResumeTimer"] = [&runner](ExecutionContext& ctx) {
        int64_t handleVal = ctx.GetInputValue("TimerHandle").asInt();
        auto handle = static_cast<TimerHandle>(handleVal);

        ctx.Log("  [ResumeTimer] TimerHandle=" + std::to_string(handle));

        bool success = false;
        if (handle != 0)
            success = runner.GetTimerManager().ResumeTimer(handle);

        ctx.SetOutputValue("Success", Variant(success));
        ctx.Log("  [ResumeTimer] " + std::string(success ? "Resumed" : "Not found or invalid"));

        ctx.ActivateOutputFlow("Exec");
        return true;
    };

    handlers["OutputAction"] = [](ExecutionContext& ctx) {
        double sample = ctx.GetInputValue("Sample").asFloat();
        ctx.Log("  Sample = " + std::to_string(sample));
        ctx.SetOutputValue("Condition", Variant(sample > 0.5));
        return true;
    };

    handlers["InputActionFire"] = [](ExecutionContext& ctx) {
        ctx.Log("  [InputAction] Fire triggered");
        return true;
    };

    handlers["CustomEvent"] = [](ExecutionContext& ctx) {
        ctx.Log("  [CustomEvent] triggered");
        ctx.ActivateOutputFlow("Exec");
        return true;
    };

    handlers["TraceByChannel"] = [](ExecutionContext& ctx) {
        ctx.Log("  [Trace] Line trace performed");
        ctx.SetOutputValue("Return Value", Variant(true));
        return true;
    };
}

} // namespace Runtime
} // namespace NodeEditor
