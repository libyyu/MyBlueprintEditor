// Runtime/BuiltinHandlers_Action.cpp -- Action 类型节点处理器
#include "BuiltinHandlers_Action.h"
#include "../MainThreadDispatcher.h"

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

    // ============================================================================
    // Event 节点处理器
    // ============================================================================

    handlers["OnBeginPlay"] = [](ExecutionContext& ctx) {
        ctx.Log("  [OnBeginPlay] triggered");
        ctx.ActivateOutputFlow("");
        return true;
    };

    handlers["OnTick"] = [](ExecutionContext& ctx) {
        // 从 __DeltaTime 变量读取编辑器注入的真实帧时间
        double dt = ctx.GetVariable("__DeltaTime").asFloat();
        ctx.SetOutputValue("DeltaTime", Variant(dt));
        ctx.ActivateOutputFlow("");
        return true;
    };

    handlers["CustomEventNode"] = [](ExecutionContext& ctx) {
        ctx.Log("  [CustomEvent] triggered");
        ctx.SetOutputValue("EventName", Variant(std::string("")));
        ctx.ActivateOutputFlow("");
        return true;
    };

    // FireEvent：触发指定名称的 CustomEvent，用于打破 exec 环（异步循环入口）
    // EventName 可来自 pin 连线或默认值
    // 注意：直接同步调用 DispatchEvent，而非 post 到 MainThreadDispatcher 队列。
    // 原因：FireEvent 通常从 LLM.Chat 等异步回调触发，此时当前 exec 帧已结束，
    //       不存在重入风险。Post 方案依赖 OnFrame DrainQueue，而编辑器在
    //       BeginPlay 同步链结束后 isExecuting=false，DrainQueue 不再被调用，
    //       导致事件永远不触发。同步调用可正确驱动 ReAct loop。
    handlers["FireEvent"] = [&runner](ExecutionContext& ctx) -> bool {
        std::string eventName = ctx.GetInputValue("EventName").asString();
        ctx.Log("  [FireEvent] triggering event: " + eventName);
        ctx.ActivateOutputFlow("");
        if (!eventName.empty()) {
            ctx.Log("  [FireEvent] dispatching synchronously");
            runner.DispatchEvent(eventName);
        }
        return true;
    };

} // RegisterHandlers_Action

} // namespace Runtime
} // namespace NodeEditor
