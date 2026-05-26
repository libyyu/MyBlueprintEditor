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
        auto* runner = ctx.GetRunner(); (void)runner;
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
    handlers["RemoveTimer"] = [](ExecutionContext& ctx) {
        auto* runner = ctx.GetRunner(); (void)runner;
        int64_t handleVal = ctx.GetInputValue("TimerHandle").asInt();
        auto handle = static_cast<TimerHandle>(handleVal);

        ctx.Log("  [RemoveTimer] TimerHandle=" + std::to_string(handle));

        bool success = false;
        if (handle != 0)
            success = runner->GetTimerManager().ClearTimer(handle);

        ctx.SetOutputValue("Success", Variant(success));
        ctx.Log("  [RemoveTimer] " + std::string(success ? "Removed" : "Not found or invalid"));

        ctx.ActivateOutputFlow("Exec");
        return true;
    };

    // PauseTimer — 依赖：runner（timer 管理器）
    handlers["PauseTimer"] = [](ExecutionContext& ctx) {
        auto* runner = ctx.GetRunner(); (void)runner;
        int64_t handleVal = ctx.GetInputValue("TimerHandle").asInt();
        auto handle = static_cast<TimerHandle>(handleVal);

        ctx.Log("  [PauseTimer] TimerHandle=" + std::to_string(handle));

        bool success = false;
        if (handle != 0)
            success = runner->GetTimerManager().PauseTimer(handle);

        ctx.SetOutputValue("Success", Variant(success));
        ctx.Log("  [PauseTimer] " + std::string(success ? "Paused" : "Not found or invalid"));

        ctx.ActivateOutputFlow("Exec");
        return true;
    };

    // ResumeTimer — 依赖：runner（timer 管理器）
    handlers["ResumeTimer"] = [](ExecutionContext& ctx) {
        auto* runner = ctx.GetRunner(); (void)runner;
        int64_t handleVal = ctx.GetInputValue("TimerHandle").asInt();
        auto handle = static_cast<TimerHandle>(handleVal);

        ctx.Log("  [ResumeTimer] TimerHandle=" + std::to_string(handle));

        bool success = false;
        if (handle != 0)
            success = runner->GetTimerManager().ResumeTimer(handle);

        ctx.SetOutputValue("Success", Variant(success));
        ctx.Log("  [ResumeTimer] " + std::string(success ? "Resumed" : "Not found or invalid"));

        ctx.ActivateOutputFlow("Exec");
        return true;
    };

    handlers["OutputAction"] = [](ExecutionContext& ctx) {
        auto* runner = ctx.GetRunner(); (void)runner;
        double sample = ctx.GetInputValue("Sample").asFloat();
        ctx.Log("  Sample = " + std::to_string(sample));
        ctx.SetOutputValue("Condition", Variant(sample > 0.5));
        return true;
    };

    handlers["InputActionFire"] = [](ExecutionContext& ctx) {
        auto* runner = ctx.GetRunner(); (void)runner;
        ctx.Log("  [InputAction] Fire triggered");
        return true;
    };

    handlers["CustomEvent"] = [](ExecutionContext& ctx) {
        auto* runner = ctx.GetRunner(); (void)runner;
        ctx.Log("  [CustomEvent] triggered");
        ctx.ActivateOutputFlow("Exec");
        return true;
    };

    handlers["TraceByChannel"] = [](ExecutionContext& ctx) {
        auto* runner = ctx.GetRunner(); (void)runner;
        ctx.Log("  [Trace] Line trace performed");
        ctx.SetOutputValue("Return Value", Variant(true));
        return true;
    };

    // ============================================================================
    // Event 节点处理器
    // ============================================================================

    handlers["OnBeginPlay"] = [](ExecutionContext& ctx) {
        auto* runner = ctx.GetRunner(); (void)runner;
        ctx.Log("  [OnBeginPlay] triggered");
        ctx.ActivateOutputFlow("");
        return true;
    };

    handlers["OnTick"] = [](ExecutionContext& ctx) {
        auto* runner = ctx.GetRunner(); (void)runner;
        // 从 __DeltaTime 变量读取编辑器注入的真实帧时间
        double dt = ctx.GetVariable("__DeltaTime").asFloat();
        ctx.SetOutputValue("DeltaTime", Variant(dt));
        ctx.ActivateOutputFlow("");
        return true;
    };

    handlers["CustomEventNode"] = [](ExecutionContext& ctx) {
        auto* runner = ctx.GetRunner(); (void)runner;
        ctx.Log("  [CustomEvent] triggered");
        ctx.SetOutputValue("EventName", Variant(std::string("")));
        ctx.ActivateOutputFlow("");
        return true;
    };

    // FireEvent：触发指定名称的 CustomEvent，用于打破 exec 环（异步循环入口）
    // 使用 MainThreadDispatcher::Post 异步投递，同时通过 AcquireAsync/ReleaseAsync
    // 维持 HasPendingAsync()==true，确保编辑器 OnFrame 继续驱动 DrainQueue，
    // 直到事件真正被消费。这样既避免同步重入（拓扑有环时会递归爆栈），
    // 又不会因 isExecuting 提前变 false 导致 DrainQueue 停止调用。
    handlers["FireEvent"] = [](ExecutionContext& ctx) -> bool {
        auto* runner = ctx.GetRunner(); (void)runner;
        std::string eventName = ctx.GetInputValue("EventName").asString();
        ctx.Log("  [FireEvent] triggering event: " + eventName);
        ctx.ActivateOutputFlow("");
        if (!eventName.empty()) {
            // 先 Acquire，让 isExecuting 保持 true（OnFrame 继续调 DrainQueue）
            runner->AcquireAsync();
            ctx.Log("  [FireEvent] async acquired, posting to dispatcher");
            auto* r = runner;
            MainThreadDispatcher::Get().Post([r, eventName]() {
                r->DispatchEvent(eventName);
                r->ReleaseAsync();
            });
        }
        return true;
    };

} // RegisterHandlers_Action

} // namespace Runtime
} // namespace NodeEditor
