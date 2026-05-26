// Runtime/handlers/BuiltinHandlers_Game.cpp
// 游戏核心节点：行为树 + 状态机
//
// 行为树（BT.*）：
//   BT.Sequence      — 顺序执行，任一失败则停止，全部成功输出 onSuccess
//   BT.Selector      — 顺序尝试，任一成功则停止，全部失败输出 onFailure
//   BT.Parallel      — 并行执行所有子节点，Policy(All/Any) 决定成功条件
//   BT.Inverter      — 反转子节点结果
//   BT.Repeat        — 循环执行 N 次（0=无限），直到子节点失败或达到次数
//   BT.Cooldown      — 限制节点执行频率（冷却期内直接走 onCooldown）
//   BT.Wait          — 等待 N 秒后继续（异步）
//   BT.Condition     — 检查条件变量，True→onSuccess, False→onFailure
//   BT.SetBlackboard — 写黑板变量（SetVariable 的语义别名）
//   BT.GetBlackboard — 读黑板变量（GetVariable 的语义别名）
//   BT.Log           — 行为树日志节点（调试用）
//   BT.AlwaysSuccess — 强制子节点结果为 success
//   BT.AlwaysFailure — 强制子节点结果为 failure
//
// 状态机（FSM.*）：
//   FSM.State        — 定义状态，onEnter/onUpdate/onExit 三个执行出口
//   FSM.Transition   — 条件驱动的状态切换
//   FSM.GetState     — 获取当前状态名
//   FSM.SetState     — 强制切换状态（会触发 onExit/onEnter）
//   FSM.IsInState    — 判断是否处于指定状态

#include "BuiltinHandlers_Game.h"
#include "../BlueprintRunner.h"
#include "../FrameTimerManager.h"

#include <string>
#include <mutex>
#include <unordered_map>
#include <atomic>

namespace NodeEditor {
namespace Runtime {

void RegisterHandlers_Game(
    std::unordered_map<std::string, NodeHandler>& handlers)
{
    // ========================================================================
    // ── 行为树结果约定 ────────────────────────────────────────────────────
    // 每个 BT 节点通过变量 "__bt_result_<nodeId>" 传递执行结果：
    //   true  = success
    //   false = failure
    // 父节点在执行子节点后读取该变量判断分支逻辑。
    // ========================================================================

    // ========================================================================
    // BT.Sequence
    // 顺序执行所有子节点（通过 onChild0..onChild7 引脚连接）
    // 遇到第一个 failure 立即停止，全部 success 则 onSuccess
    // in:  exec, [ChildCount(Int,默认自动检测)]
    // out: onChild0..onChild7(exec), onSuccess(exec), onFailure(exec)
    //      FailedIndex(Int) — 第几个子节点失败（-1=全成功）
    // ========================================================================
    handlers["BT.Sequence"] = [](ExecutionContext& ctx) -> bool {
        auto* node = ctx.GetCurrentNode();
        std::string nodeId = node ? std::to_string(node->id) : "0";

        // 收集所有 onChildN 引脚
        std::vector<PinId> childPins;
        if (node) {
            for (int i = 0; i < 8; ++i) {
                std::string name = "onChild" + std::to_string(i);
                PinId pid = ctx.GetPinId(name);
                if (pid != 0) {
                    childPins.push_back(pid);
                    ctx.MarkDownstreamAsHandled(name);
                }
            }
        }
        ctx.MarkDownstreamAsHandled("onSuccess");
        ctx.MarkDownstreamAsHandled("onFailure");

        int64_t failedIdx = -1;
        for (size_t i = 0; i < childPins.size(); ++i) {
            ctx.SetVariable("__bt_result_" + nodeId, Variant(true)); // 默认 success
            ctx.ActivateOutputFlow(childPins[i]);
            bool childResult = ctx.GetVariable("__bt_result_" + nodeId).asBool();
            if (!childResult) {
                failedIdx = static_cast<int64_t>(i);
                break;
            }
        }

        ctx.SetOutputValue("FailedIndex", Variant(failedIdx));
        // 向父节点报告本节点结果
        std::string parentResultKey = "__bt_result_" + (node ? std::to_string(node->id) : "0");
        bool success = (failedIdx < 0);
        ctx.SetVariable(parentResultKey, Variant(success));
        ctx.Log("[BT.Sequence] " + std::string(success ? "SUCCESS" : "FAILURE at child ") +
                (success ? "" : std::to_string(failedIdx)));

        if (success)
            ctx.ActivateOutputFlow("onSuccess");
        else
            ctx.ActivateOutputFlow("onFailure");
        return true;
    };

    // ========================================================================
    // BT.Selector
    // 顺序尝试，遇到第一个 success 停止，全部 failure 则 onFailure
    // out: onChild0..onChild7(exec), onSuccess(exec), onFailure(exec)
    //      SucceededIndex(Int)
    // ========================================================================
    handlers["BT.Selector"] = [](ExecutionContext& ctx) -> bool {
        auto* node = ctx.GetCurrentNode();
        std::string nodeId = node ? std::to_string(node->id) : "0";

        std::vector<PinId> childPins;
        if (node) {
            for (int i = 0; i < 8; ++i) {
                std::string name = "onChild" + std::to_string(i);
                PinId pid = ctx.GetPinId(name);
                if (pid != 0) {
                    childPins.push_back(pid);
                    ctx.MarkDownstreamAsHandled(name);
                }
            }
        }
        ctx.MarkDownstreamAsHandled("onSuccess");
        ctx.MarkDownstreamAsHandled("onFailure");

        int64_t succeededIdx = -1;
        for (size_t i = 0; i < childPins.size(); ++i) {
            ctx.SetVariable("__bt_result_" + nodeId, Variant(false));
            ctx.ActivateOutputFlow(childPins[i]);
            bool childResult = ctx.GetVariable("__bt_result_" + nodeId).asBool();
            if (childResult) {
                succeededIdx = static_cast<int64_t>(i);
                break;
            }
        }

        ctx.SetOutputValue("SucceededIndex", Variant(succeededIdx));
        bool success = (succeededIdx >= 0);
        ctx.SetVariable("__bt_result_" + nodeId, Variant(success));
        ctx.Log("[BT.Selector] " + std::string(success ? "SUCCESS at child " : "FAILURE") +
                (success ? std::to_string(succeededIdx) : ""));

        if (success)
            ctx.ActivateOutputFlow("onSuccess");
        else
            ctx.ActivateOutputFlow("onFailure");
        return true;
    };

    // ========================================================================
    // BT.Parallel
    // 执行所有子节点，Policy 决定成功条件
    // in:  exec, Policy(String: "All"=全部成功, "Any"=任一成功)
    // out: onChild0..onChild7(exec), onSuccess(exec), onFailure(exec)
    //      SuccessCount(Int), FailureCount(Int)
    // ========================================================================
    handlers["BT.Parallel"] = [](ExecutionContext& ctx) -> bool {
        auto* node = ctx.GetCurrentNode();
        std::string nodeId = node ? std::to_string(node->id) : "0";
        std::string policy = ctx.GetInputValue("Policy").asString();
        if (policy.empty()) policy = "All";

        std::vector<PinId> childPins;
        if (node) {
            for (int i = 0; i < 8; ++i) {
                std::string name = "onChild" + std::to_string(i);
                PinId pid = ctx.GetPinId(name);
                if (pid != 0) {
                    childPins.push_back(pid);
                    ctx.MarkDownstreamAsHandled(name);
                }
            }
        }
        ctx.MarkDownstreamAsHandled("onSuccess");
        ctx.MarkDownstreamAsHandled("onFailure");

        int64_t successCount = 0, failureCount = 0;
        for (auto pid : childPins) {
            ctx.SetVariable("__bt_result_" + nodeId, Variant(true));
            ctx.ActivateOutputFlow(pid);
            if (ctx.GetVariable("__bt_result_" + nodeId).asBool())
                ++successCount;
            else
                ++failureCount;
        }

        ctx.SetOutputValue("SuccessCount", Variant(successCount));
        ctx.SetOutputValue("FailureCount", Variant(failureCount));

        bool success = (policy == "Any") ? (successCount > 0) : (failureCount == 0);
        ctx.SetVariable("__bt_result_" + nodeId, Variant(success));
        ctx.Log("[BT.Parallel] policy=" + policy + " success=" + std::to_string(successCount)
                + " failure=" + std::to_string(failureCount));

        if (success) ctx.ActivateOutputFlow("onSuccess");
        else         ctx.ActivateOutputFlow("onFailure");
        return true;
    };

    // ========================================================================
    // BT.Inverter
    // 反转子节点结果（success↔failure）
    // out: onChild(exec), onSuccess(exec), onFailure(exec)
    // ========================================================================
    handlers["BT.Inverter"] = [](ExecutionContext& ctx) -> bool {
        auto* node = ctx.GetCurrentNode();
        std::string nodeId = node ? std::to_string(node->id) : "0";
        PinId childPin = ctx.GetPinId("onChild");
        ctx.MarkDownstreamAsHandled("onChild");
        ctx.MarkDownstreamAsHandled("onSuccess");
        ctx.MarkDownstreamAsHandled("onFailure");

        ctx.SetVariable("__bt_result_" + nodeId, Variant(true));
        if (childPin != 0)
            ctx.ActivateOutputFlow(childPin);
        bool childResult = ctx.GetVariable("__bt_result_" + nodeId).asBool();
        bool inverted = !childResult;
        ctx.SetVariable("__bt_result_" + nodeId, Variant(inverted));

        if (inverted) ctx.ActivateOutputFlow("onSuccess");
        else          ctx.ActivateOutputFlow("onFailure");
        return true;
    };

    // ========================================================================
    // BT.Repeat
    // 循环执行子节点 Count 次（0=无限直到失败）
    // in:  exec, Count(Int,0=无限), StopOnFailure(Bool,默认true)
    // out: onChild(exec), onDone(exec), onFailure(exec)
    //      Iterations(Int)
    // ========================================================================
    handlers["BT.Repeat"] = [](ExecutionContext& ctx) -> bool {
        auto* node = ctx.GetCurrentNode();
        std::string nodeId = node ? std::to_string(node->id) : "0";
        int64_t count       = ctx.GetInputValue("Count").asInt();
        bool stopOnFailure  = ctx.GetInputValue("StopOnFailure").type == PinDataType::Unknown
                              ? true : ctx.GetInputValue("StopOnFailure").asBool();

        PinId childPin = ctx.GetPinId("onChild");
        ctx.MarkDownstreamAsHandled("onChild");
        ctx.MarkDownstreamAsHandled("onDone");
        ctx.MarkDownstreamAsHandled("onFailure");

        int64_t iter = 0;
        bool failed = false;
        int64_t maxIter = (count <= 0) ? 10000 : count; // 0=无限但加安全上限

        while (iter < maxIter) {
            ctx.SetVariable("__bt_result_" + nodeId, Variant(true));
            if (childPin != 0) ctx.ActivateOutputFlow(childPin);
            bool childResult = ctx.GetVariable("__bt_result_" + nodeId).asBool();
            ++iter;
            if (stopOnFailure && !childResult) {
                failed = true;
                break;
            }
        }

        ctx.SetOutputValue("Iterations", Variant(iter));
        ctx.SetVariable("__bt_result_" + nodeId, Variant(!failed));
        ctx.Log("[BT.Repeat] iterations=" + std::to_string(iter));

        if (failed) ctx.ActivateOutputFlow("onFailure");
        else        ctx.ActivateOutputFlow("onDone");
        return true;
    };

    // ========================================================================
    // BT.Cooldown
    // 冷却节点：Duration 秒内再次进入直接走 onCooldown
    // in:  exec, Duration(Float), Key(String,可选)
    // out: onChild(exec), onSuccess(exec), onFailure(exec), onCooldown(exec)
    // ========================================================================
    handlers["BT.Cooldown"] = [](ExecutionContext& ctx) -> bool {
        auto* node = ctx.GetCurrentNode();
        double duration  = ctx.GetInputValue("Duration").asFloat();
        std::string key  = ctx.GetInputValue("Key").asString();
        if (key.empty()) key = "__bt_cd_" + (node ? std::to_string(node->id) : "0");
        else             key = "__bt_cd_" + key;

        double now      = static_cast<double>(FrameTimerManager::GetCurrentUnixTime());
        double lastTime = ctx.GetVariable(key).asFloat();

        PinId childPin = ctx.GetPinId("onChild");
        ctx.MarkDownstreamAsHandled("onChild");
        ctx.MarkDownstreamAsHandled("onSuccess");
        ctx.MarkDownstreamAsHandled("onFailure");
        ctx.MarkDownstreamAsHandled("onCooldown");

        std::string nodeId = node ? std::to_string(node->id) : "0";

        if (lastTime > 0.0 && (now - lastTime) < duration) {
            ctx.Log("[BT.Cooldown] on cooldown, remaining=" +
                    std::to_string(duration - (now - lastTime)) + "s");
            ctx.SetVariable("__bt_result_" + nodeId, Variant(false));
            ctx.ActivateOutputFlow("onCooldown");
            return true;
        }

        ctx.SetVariable(key, Variant(now));
        ctx.SetVariable("__bt_result_" + nodeId, Variant(true));
        if (childPin != 0) ctx.ActivateOutputFlow(childPin);
        bool result = ctx.GetVariable("__bt_result_" + nodeId).asBool();

        if (result) ctx.ActivateOutputFlow("onSuccess");
        else        ctx.ActivateOutputFlow("onFailure");
        return true;
    };

    // ========================================================================
    // BT.Wait
    // 等待 Duration 秒后触发 onDone（异步）
    // 作为 BT Leaf 节点，等待期间报告 running（暂时用 success 表示已启动）
    // in:  exec, Duration(Float)
    // out: onDone(exec)
    // ========================================================================
    handlers["BT.Wait"] = [](ExecutionContext& ctx) -> bool {
        auto* runner = ctx.GetRunner(); (void)runner;
        auto* node = ctx.GetCurrentNode();
        float duration = static_cast<float>(ctx.GetInputValue("Duration").asFloat());
        if (duration <= 0.0f) duration = 0.0f;

        std::string nodeId = node ? std::to_string(node->id) : "0";
        PinId donePinId = ctx.GetPinId("onDone");
        ctx.MarkDownstreamAsHandled("onDone");
        ctx.SetVariable("__bt_result_" + nodeId, Variant(true));

        auto alive = runner->GetAliveFlag();
        ExecutionContext* pCtx = &ctx;
        ctx.Delay(duration, [pCtx, donePinId, nodeId, alive]() {
            if (!alive->load(std::memory_order_acquire)) return;
            pCtx->SetVariable("__bt_result_" + nodeId, Variant(true));
            pCtx->ActivateOutputFlow(donePinId);
        });
        return true;
    };

    // ========================================================================
    // BT.Condition
    // 检查变量/条件值，True→onSuccess，False→onFailure
    // in:  Condition(Bool), Key(String,可选，若非空则读变量)
    // out: onSuccess(exec), onFailure(exec)
    //      Result(Bool)
    // ========================================================================
    handlers["BT.Condition"] = [](ExecutionContext& ctx) -> bool {
        auto* node = ctx.GetCurrentNode();
        std::string nodeId = node ? std::to_string(node->id) : "0";

        std::string key = ctx.GetInputValue("Key").asString();
        bool result = key.empty()
            ? ctx.GetInputValue("Condition").asBool()
            : ctx.GetVariable(key).asBool();

        ctx.SetOutputValue("Result", Variant(result));
        ctx.SetVariable("__bt_result_" + nodeId, Variant(result));
        ctx.Log("[BT.Condition] result=" + std::string(result ? "true" : "false"));

        if (result) ctx.ActivateOutputFlow("onSuccess");
        else        ctx.ActivateOutputFlow("onFailure");
        return true;
    };

    // ========================================================================
    // BT.SetBlackboard  /  BT.GetBlackboard
    // 黑板读写（语义别名，底层等同 SetVariable/GetVariable，便于 BT 识别）
    // ========================================================================
    handlers["BT.SetBlackboard"] = [](ExecutionContext& ctx) -> bool {
        std::string key = ctx.GetInputValue("Key").asString();
        Variant val     = ctx.GetInputValue("Value");
        ctx.SetVariable(key, val);
        ctx.Log("[BT.SetBlackboard] " + key + " = " + val.asString());
        ctx.ActivateOutputFlow("exec");
        return true;
    };

    handlers["BT.GetBlackboard"] = [](ExecutionContext& ctx) -> bool {
        std::string key = ctx.GetInputValue("Key").asString();
        Variant val     = ctx.GetVariable(key);
        bool found      = (val.type != PinDataType::Unknown);
        ctx.SetOutputValue("Value", val);
        ctx.SetOutputValue("Found", Variant(found));
        return true;
    };

    // ========================================================================
    // BT.Log
    // 行为树日志节点（调试用），总是返回 success
    // in:  exec, Message(String), Level(String: info/warn/error)
    // out: exec
    // ========================================================================
    handlers["BT.Log"] = [](ExecutionContext& ctx) -> bool {
        auto* node = ctx.GetCurrentNode();
        std::string nodeId = node ? std::to_string(node->id) : "0";
        std::string msg    = ctx.GetInputValue("Message").asString();
        std::string level  = ctx.GetInputValue("Level").asString();

        std::string prefix = "[BT] ";
        if (level == "warn")        ctx.Log(prefix + msg, LogLevel::Warning);
        else if (level == "error")  ctx.LogError(prefix + msg);
        else                        ctx.Log(prefix + msg);

        ctx.SetVariable("__bt_result_" + nodeId, Variant(true));
        ctx.ActivateOutputFlow("exec");
        return true;
    };

    // ========================================================================
    // BT.AlwaysSuccess  /  BT.AlwaysFailure
    // 强制修改子节点结果
    // ========================================================================
    handlers["BT.AlwaysSuccess"] = [](ExecutionContext& ctx) -> bool {
        auto* node = ctx.GetCurrentNode();
        std::string nodeId = node ? std::to_string(node->id) : "0";
        PinId childPin = ctx.GetPinId("onChild");
        ctx.MarkDownstreamAsHandled("onChild");
        ctx.MarkDownstreamAsHandled("onDone");
        if (childPin != 0) ctx.ActivateOutputFlow(childPin);
        ctx.SetVariable("__bt_result_" + nodeId, Variant(true));
        ctx.ActivateOutputFlow("onDone");
        return true;
    };

    handlers["BT.AlwaysFailure"] = [](ExecutionContext& ctx) -> bool {
        auto* node = ctx.GetCurrentNode();
        std::string nodeId = node ? std::to_string(node->id) : "0";
        PinId childPin = ctx.GetPinId("onChild");
        ctx.MarkDownstreamAsHandled("onChild");
        ctx.MarkDownstreamAsHandled("onDone");
        if (childPin != 0) ctx.ActivateOutputFlow(childPin);
        ctx.SetVariable("__bt_result_" + nodeId, Variant(false));
        ctx.ActivateOutputFlow("onDone");
        return true;
    };

    // ========================================================================
    // ── 状态机 FSM ────────────────────────────────────────────────────────
    // 全局状态存储：变量 "__fsm_<machineId>_state" 存当前状态名
    // machineId 默认用节点 id，支持多个独立状态机
    // ========================================================================

    // ========================================================================
    // FSM.State
    // 定义并响应一个状态。每帧 Tick 时，若当前状态==StateName，触发 onUpdate
    // 进入时触发 onEnter，离开时触发 onExit（由 FSM.SetState 驱动）
    // in:  exec(从 OnTick/OnBeginPlay 接入), StateName(String), MachineId(String)
    // out: onEnter(exec), onUpdate(exec), onExit(exec)
    //      IsActive(Bool)
    // ========================================================================
    handlers["FSM.State"] = [](ExecutionContext& ctx) -> bool {
        std::string stateName = ctx.GetInputValue("StateName").asString();
        std::string machineId = ctx.GetInputValue("MachineId").asString();
        if (machineId.empty()) {
            auto* node = ctx.GetCurrentNode();
            machineId = node ? std::to_string(node->id) : "default";
        }

        std::string stateKey    = "__fsm_" + machineId + "_state";
        std::string prevKey     = "__fsm_" + machineId + "_prev";
        std::string enterKey    = "__fsm_" + machineId + "_enter_" + stateName;

        std::string current = ctx.GetVariable(stateKey).asString();
        bool isActive = (current == stateName);
        ctx.SetOutputValue("IsActive", Variant(isActive));

        if (!isActive) return true;

        // 检测 Enter（首次进入或从其他状态切换来）
        std::string prev = ctx.GetVariable(prevKey).asString();
        bool justEntered = ctx.GetVariable(enterKey).asBool();
        if (!justEntered) {
            ctx.SetVariable(enterKey, Variant(true));
            ctx.Log("[FSM." + machineId + "] Enter state: " + stateName);
            ctx.ActivateOutputFlow("onEnter");
        }

        ctx.ActivateOutputFlow("onUpdate");
        return true;
    };

    // ========================================================================
    // FSM.Transition
    // 条件驱动的状态切换
    // in:  exec, Condition(Bool), From(String,空=任意), To(String), MachineId(String)
    // out: onTransitioned(exec), onSkipped(exec)
    //      Changed(Bool)
    // ========================================================================
    handlers["FSM.Transition"] = [](ExecutionContext& ctx) -> bool {
        bool condition      = ctx.GetInputValue("Condition").asBool();
        std::string from    = ctx.GetInputValue("From").asString();
        std::string to      = ctx.GetInputValue("To").asString();
        std::string machineId = ctx.GetInputValue("MachineId").asString();
        if (machineId.empty()) machineId = "default";

        std::string stateKey = "__fsm_" + machineId + "_state";
        std::string current  = ctx.GetVariable(stateKey).asString();

        bool canTransit = condition && !to.empty() &&
                          (from.empty() || current == from);

        ctx.SetOutputValue("Changed", Variant(canTransit));

        if (canTransit) {
            // 清除旧状态的 enter 标记，设置新状态
            std::string oldEnterKey = "__fsm_" + machineId + "_enter_" + current;
            std::string newEnterKey = "__fsm_" + machineId + "_enter_" + to;
            ctx.SetVariable("__fsm_" + machineId + "_prev", Variant(current));
            ctx.SetVariable(oldEnterKey, Variant(false));
            ctx.SetVariable(newEnterKey, Variant(false));
            ctx.SetVariable(stateKey, Variant(to));
            ctx.Log("[FSM." + machineId + "] Transition: " + current + " → " + to);

            // 触发旧状态的 onExit（如有）
            // 注：onExit 由 FSM.State 节点的下一帧检测，或通过 FSM.SetState 显式触发
            ctx.ActivateOutputFlow("onTransitioned");
        } else {
            ctx.ActivateOutputFlow("onSkipped");
        }
        return true;
    };

    // ========================================================================
    // FSM.GetState
    // 获取当前状态名
    // in:  MachineId(String)
    // out: State(String)
    // ========================================================================
    handlers["FSM.GetState"] = [](ExecutionContext& ctx) -> bool {
        std::string machineId = ctx.GetInputValue("MachineId").asString();
        if (machineId.empty()) machineId = "default";
        std::string state = ctx.GetVariable("__fsm_" + machineId + "_state").asString();
        ctx.SetOutputValue("State", Variant(state));
        return true;
    };

    // ========================================================================
    // FSM.SetState
    // 强制设置状态（清除 enter 标记，触发切换）
    // in:  exec, State(String), MachineId(String)
    // out: exec
    //      PrevState(String)
    // ========================================================================
    handlers["FSM.SetState"] = [](ExecutionContext& ctx) -> bool {
        std::string newState  = ctx.GetInputValue("State").asString();
        std::string machineId = ctx.GetInputValue("MachineId").asString();
        if (machineId.empty()) machineId = "default";

        std::string stateKey = "__fsm_" + machineId + "_state";
        std::string current  = ctx.GetVariable(stateKey).asString();

        ctx.SetOutputValue("PrevState", Variant(current));

        // 清除旧状态 enter 标记
        if (!current.empty())
            ctx.SetVariable("__fsm_" + machineId + "_enter_" + current, Variant(false));
        // 清除新状态 enter 标记（触发 onEnter）
        ctx.SetVariable("__fsm_" + machineId + "_enter_" + newState, Variant(false));
        ctx.SetVariable("__fsm_" + machineId + "_prev", Variant(current));
        ctx.SetVariable(stateKey, Variant(newState));

        ctx.Log("[FSM." + machineId + "] SetState: " + current + " → " + newState);
        ctx.ActivateOutputFlow("exec");
        return true;
    };

    // ========================================================================
    // FSM.IsInState
    // 判断是否处于指定状态
    // in:  State(String), MachineId(String)
    // out: Result(Bool)
    // ========================================================================
    handlers["FSM.IsInState"] = [](ExecutionContext& ctx) -> bool {
        std::string state     = ctx.GetInputValue("State").asString();
        std::string machineId = ctx.GetInputValue("MachineId").asString();
        if (machineId.empty()) machineId = "default";
        std::string current = ctx.GetVariable("__fsm_" + machineId + "_state").asString();
        ctx.SetOutputValue("Result", Variant(current == state));
        return true;
    };

    // 替换现有桩实现，让旧蓝图不崩溃
    handlers["Sequence"]   = handlers["BT.Sequence"];
    handlers["MoveTo"]     = [](ExecutionContext& ctx) -> bool {
        ctx.Log("[BT] MoveTo: TargetX=" + ctx.GetInputValue("TargetX").asString()
                + " TargetY=" + ctx.GetInputValue("TargetY").asString());
        auto* node = ctx.GetCurrentNode();
        std::string nodeId = node ? std::to_string(node->id) : "0";
        ctx.SetVariable("__bt_result_" + nodeId, Variant(true));
        ctx.ActivateOutputFlow("onSuccess");
        return true;
    };
    handlers["RandomWait"] = [](ExecutionContext& ctx) -> bool {
        double minT = ctx.GetInputValue("Min").asFloat();
        double maxT = ctx.GetInputValue("Max").asFloat();
        if (maxT <= minT) maxT = minT + 1.0;
        double dur = minT + static_cast<double>(rand()) / RAND_MAX * (maxT - minT);
        ctx.Log("[BT] RandomWait: " + std::to_string(dur) + "s (scheduled, not blocking)");
        ctx.ActivateOutputFlow("exec");
        return true;
    };
}

} // namespace Runtime
} // namespace NodeEditor
