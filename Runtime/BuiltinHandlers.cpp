// Runtime/BuiltinHandlers.cpp -- 内置节点处理器注册实现（独立于编辑器）
//
// 合并了以下 9 个分类的处理器：
//   Flow, Action, Math, Debug, String, Array, Tree, Houdini, Misc
//
// 原 Editor 层的处理器中，约 5 个依赖 BlueprintEditor 成员变量
// （m_ExecutionLog, m_PersistentRunner, m_CurrentFilePath, m_HandlerRegistry）。
// 此文件将它们重构为仅使用 Runtime API：
//   - m_ExecutionLog   → ctx.Log()
//   - m_PersistentRunner → runner 引用（通过参数传入）
//   - m_CurrentFilePath → basePath（通过参数传入）
//   - m_HandlerRegistry → handlers 映射表（通过参数传入）
//
// 该文件完全不依赖 ImGui 或 BlueprintEditor，可在任意环境中使用。

#include "BuiltinHandlers.h"
#include "BlueprintExporter.h"
#include <cstdlib>
#include <algorithm>
#include <cctype>
#include <fstream>
#include <sstream>
#include <cmath>

namespace NodeEditor {
namespace Runtime {

// ============================================================================
// Flow Control 处理器
// ============================================================================

static void RegisterHandlers_Flow(
    std::unordered_map<std::string, NodeHandler>& handlers,
    BlueprintRunner& runner,
    const std::string& basePath)
{
    handlers["Branch"] = [](ExecutionContext& ctx) {
        bool condition = ctx.GetInputValue("Condition").asBool();
        ctx.Log("  Condition = " + std::string(condition ? "True" : "False"));
        if (condition)
            ctx.ActivateOutputFlow("True");
        else
            ctx.ActivateOutputFlow("False");
        return true;
    };

    handlers["DoN"] = [](ExecutionContext& ctx) {
        int64_t n = ctx.GetInputValue("N").asInt();

        // 使用节点 ID 作为变量键的一部分，避免多个 DoN 实例的状态冲突
        auto* node = ctx.GetCurrentNode();
        std::string nodeIdStr = node ? std::to_string(node->id) : "0";
        std::string counterKey = "__don_counter_" + nodeIdStr;

        // 判断是从 Enter 还是 Reset 引脚触发的
        std::string activatedPin = ctx.GetActivatedInputPinName();

        if (activatedPin == "Reset")
        {
            // Reset 引脚触发：重置计数器
            ctx.SetVariable(counterKey, Variant(static_cast<int64_t>(0)));
            ctx.Log("  [DoN] Reset (counter -> 0)");
            return true;
        }

        // Enter 引脚触发（或主循环中直接执行）：递增计数器
        int64_t counter = ctx.GetVariable(counterKey).asInt();
        counter++;
        ctx.SetVariable(counterKey, Variant(counter));
        ctx.SetOutputValue("Counter", Variant(counter));
        ctx.Log("  [DoN] N=" + std::to_string(n) + ", Counter=" + std::to_string(counter));

        if (counter <= n)
        {
            // 计数器未超过 N，激活 Exit 输出流
            ctx.ActivateOutputFlow("Exit");
        }
        else
        {
            ctx.Log("  [DoN] Counter exceeded N, blocked");
        }
        return true;
    };

    // ExecuteBlueprint — 加载并执行另一个蓝图文件
    // 依赖：basePath（路径解析）、runner（获取 handlers + timer）
    // 注意：不能捕获 &handlers（局部变量引用，函数返回后悬垂），
    //       改为运行时从 runner.GetHandlers() 获取
    handlers["ExecuteBlueprint"] = [&runner, basePath](ExecutionContext& ctx) {
        auto filePath = ctx.GetInputValue("File").asString();
        bool isSync = ctx.GetInputValue("Sync").asBool();
        ctx.Log("  [ExecuteBlueprint] File: \"" + filePath + "\"  Sync: " + (isSync ? "true" : "false"));

        if (filePath.empty())
        {
            ctx.Log("  [ExecuteBlueprint] ERROR: File path is empty");
            ctx.SetOutputValue("Success", Variant(false));
            ctx.SetOutputValue("Output", Variant(std::string("Error: empty file path")));
            ctx.ActivateOutputFlow("Done");
            return true;
        }

        // 如果路径是相对路径，基于 basePath 解析
        std::string resolvedPath = filePath;
        if (!basePath.empty() && 
            filePath.find(':') == std::string::npos && 
            filePath[0] != '/' && filePath[0] != '\\')
        {
            size_t lastSlash = basePath.find_last_of("/\\");
            if (lastSlash != std::string::npos)
                resolvedPath = basePath.substr(0, lastSlash + 1) + filePath;
        }

        ctx.Log("  [ExecuteBlueprint] Resolved: \"" + resolvedPath + "\"");

        // 加载子蓝图
        JsonBlueprintExporter exporter(runner.GetFileSystem());
        auto importResult = exporter.importRuntimeFromFile(resolvedPath);

        if (!importResult.success)
        {
            ctx.Log("  [ExecuteBlueprint] ERROR: " + importResult.errorMessage);
            ctx.SetOutputValue("Success", Variant(false));
            ctx.SetOutputValue("Output", Variant(std::string("Load error: ") + importResult.errorMessage));
            ctx.ActivateOutputFlow("Done");
            return true;
        }

        ctx.Log("  [ExecuteBlueprint] Loaded " + std::to_string(importResult.data.nodes.size()) + " nodes");

        // 从 runner 获取当前已注册的 handlers（安全，不依赖局部变量引用）
        auto currentHandlers = runner.GetHandlers();

        // 同步模式
        if (isSync)
        {
            BlueprintRunner subRunner(runner.GetFileSystem());
            
            // 子蓝图以 weak_ptr 持有父的 timerManager；
            // 父析构后 weak_ptr 失效，子自动回退到自身 manager，不会 UAF
            subRunner.SetParentTimerManager(runner.GetTimerManagerPtr());

            std::vector<std::string> subLog;
            subRunner.SetLogCallback([&subLog, &ctx](const std::string& msg) {
                subLog.push_back(msg);
                ctx.Log("    | " + msg);
            });

            if (!subRunner.Load(importResult.data))
            {
                ctx.Log("  [ExecuteBlueprint] ERROR: Failed to load sub-blueprint");
                ctx.SetOutputValue("Success", Variant(false));
                ctx.SetOutputValue("Output", Variant(std::string("Load failed")));
                ctx.ActivateOutputFlow("Done");
                return true;
            }

            // 默认处理器：pass-through
            subRunner.SetDefaultHandler([](ExecutionContext& subCtx) {
                subCtx.Log("  [Default Handler] pass-through");
                return true;
            });
            subRunner.RegisterHandlers(currentHandlers);

            auto execResult = subRunner.Execute();

            ctx.Log("  [ExecuteBlueprint] Sync result: " + std::string(execResult.success ? "SUCCESS" : "FAILED") +
                    " (" + std::to_string(execResult.nodesExecuted) + " nodes executed)");

            ctx.SetOutputValue("Success", Variant(execResult.success));

            std::string outputText;
            for (const auto& line : subLog)
            {
                if (!outputText.empty()) outputText += "\n";
                outputText += line;
            }
            ctx.SetOutputValue("Output", Variant(outputText));

            ctx.ActivateOutputFlow("Done");
            return true;
        }

        // 异步模式（默认）
        ctx.Log("  [ExecuteBlueprint] Async: scheduling sub-blueprint for next frame...");

        ctx.SetOutputValue("Success", Variant(true));
        ctx.SetOutputValue("Output", Variant(std::string("(async: pending...)")));

        auto* node = ctx.GetCurrentNode();
        PinId completedPinId = 0;
        if (node)
        {
            for (const auto& pin : node->pins)
            {
                if (pin.name == "Completed" && pin.kind == PinKind::Output)
                {
                    completedPinId = pin.id;
                    break;
                }
            }
        }

        ctx.MarkDownstreamAsHandled("Completed");
        ctx.ActivateOutputFlow("Done");

        auto sharedData = std::make_shared<BlueprintData>(std::move(importResult.data));

        ctx.Delay(0.0f, [&ctx, &runner, sharedData, currentHandlers, completedPinId, resolvedPath]() {
            ctx.Log("  [ExecuteBlueprint] Async: executing \"" + resolvedPath + "\"...");

            auto subRunner = std::make_shared<BlueprintRunner>(runner.GetFileSystem());
            
            // 子蓝图以 weak_ptr 持有父的 timerManager；
            // 父析构后 weak_ptr 失效，子自动回退到自身 manager，不会 UAF
            subRunner->SetParentTimerManager(runner.GetTimerManagerPtr());

            // 使用 shared_ptr 管理 subLog，保证异步 timer 回调时仍可访问
            auto subLog = std::make_shared<std::vector<std::string>>();
            subRunner->SetLogCallback([subLog, &ctx](const std::string& msg) {
                subLog->push_back(msg);
                ctx.Log("    | " + msg);
            });

            if (!subRunner->Load(*sharedData))
            {
                ctx.Log("  [ExecuteBlueprint] Async ERROR: Failed to load sub-blueprint");
                ctx.SetOutputValue("Success", Variant(false));
                ctx.SetOutputValue("Output", Variant(std::string("Async load failed")));
                if (completedPinId != 0)
                    ctx.ActivateOutputFlow(completedPinId);
                return;
            }

            subRunner->SetDefaultHandler([](ExecutionContext& subCtx) {
                subCtx.Log("  [Default Handler] pass-through");
                return true;
            });
            subRunner->RegisterHandlers(currentHandlers);

            auto execResult = subRunner->Execute();

            std::string outputText;
            for (const auto& line : *subLog)
            {
                if (!outputText.empty()) outputText += "\n";
                outputText += line;
            }

            ctx.Log("  [ExecuteBlueprint] Async result: " +
                std::string(execResult.success ? "SUCCESS" : "FAILED") +
                " (" + std::to_string(execResult.nodesExecuted) + " nodes executed)");

            ctx.SetOutputValue("Success", Variant(execResult.success));
            ctx.SetOutputValue("Output", Variant(outputText));

            // 将 subRunner 注册到父 runner 中保持存活
            // 子蓝图的 Delay/SetTimer 回调引用了 subRunner 的 context，
            // subRunner 必须在所有 timer 回调完成前保持存活
            runner.KeepAlive(subRunner);

            if (completedPinId != 0)
                ctx.ActivateOutputFlow(completedPinId);
        });

        return true;
    };

    handlers["ForLoop"] = [](ExecutionContext& ctx) {
        int64_t first = ctx.GetInputValue("First Index").asInt();
        int64_t last = ctx.GetInputValue("Last Index").asInt();
        ctx.Log("  [ForLoop] " + std::to_string(first) + " to " + std::to_string(last));
        for (int64_t i = first; i <= last; ++i)
        {
            ctx.SetOutputValue("Index", Variant(i));
            if (!ctx.ActivateOutputFlow("Loop Body"))
                return false;
        }
        ctx.ActivateOutputFlow("Completed");
        return true;
    };

    handlers["WhileLoop"] = [](ExecutionContext& ctx) {
        ctx.Log("  [WhileLoop] Starting");
        int iterations = 0;
        const int maxIterations = 10000;
        while (ctx.GetInputValue("Condition").asBool())
        {
            if (++iterations > maxIterations)
            {
                ctx.Log("  [WhileLoop] Max iterations reached (" + std::to_string(maxIterations) + "), breaking");
                break;
            }
            if (!ctx.ActivateOutputFlow("Loop Body"))
                return false;
        }
        ctx.Log("  [WhileLoop] Completed after " + std::to_string(iterations) + " iterations");
        ctx.ActivateOutputFlow("Completed");
        return true;
    };

    // Delay — 依赖：runner 的 timer
    handlers["Delay"] = [](ExecutionContext& ctx) {
        double dur = ctx.GetInputValue("Duration").asFloat();
        float duration = static_cast<float>(dur);
        auto currentTime = FrameTimerManager::GetCurrentUnixTime();
        ctx.Log("  [Delay] " + std::to_string(duration) + "s; started:" + std::to_string(currentTime));

        ctx.ActivateOutputFlow("Exec");

        auto* node = ctx.GetCurrentNode();
        PinId completedPinId = 0;
        if (node)
        {
            for (const auto& pin : node->pins)
            {
                if (pin.name == "Completed" && pin.kind == PinKind::Output)
                {
                    completedPinId = pin.id;
                    break;
                }
            }
        }

        auto timerHandle = ctx.Delay(duration, [&ctx, completedPinId]() {
            auto finishTime = FrameTimerManager::GetCurrentUnixTime();
            ctx.Log("  [Delay] Completed; finished:" + std::to_string(finishTime));
            if (completedPinId != 0)
                ctx.ActivateOutputFlow(completedPinId);
        });

        ctx.SetOutputValue("TimerHandle", Variant(static_cast<int64_t>(timerHandle)));
        ctx.MarkDownstreamAsHandled("Completed");

        return true;
    };

    handlers["FlipFlop"] = [](ExecutionContext& ctx) {
        // 使用节点 ID 作为变量键的一部分，避免多个 FlipFlop 实例的状态冲突
        auto* node = ctx.GetCurrentNode();
        std::string nodeIdStr = node ? std::to_string(node->id) : "0";
        std::string stateKey = "__flipflop_state_" + nodeIdStr;

        bool isA = !ctx.GetVariable(stateKey).asBool();
        ctx.SetVariable(stateKey, Variant(isA));
        ctx.SetOutputValue("Is A", Variant(isA));
        ctx.Log("  [FlipFlop] -> " + std::string(isA ? "A" : "B"));
        if (isA)
            ctx.ActivateOutputFlow("A");
        else
            ctx.ActivateOutputFlow("B");
        return true;
    };

    handlers["Gate"] = [](ExecutionContext& ctx) {
        // 使用节点 ID 作为变量键的一部分，避免多个 Gate 实例的状态冲突
        auto* node = ctx.GetCurrentNode();
        std::string nodeIdStr = node ? std::to_string(node->id) : "0";
        std::string stateKey = "__gate_open_" + nodeIdStr;

        std::string activatedPin = ctx.GetActivatedInputPinName();

        if (activatedPin == "Open")
        {
            ctx.SetVariable(stateKey, Variant(true));
            ctx.Log("  [Gate] Opened");
            return true;
        }
        else if (activatedPin == "Close")
        {
            ctx.SetVariable(stateKey, Variant(false));
            ctx.Log("  [Gate] Closed");
            return true;
        }
        else if (activatedPin == "Toggle")
        {
            bool isOpen = !ctx.GetVariable(stateKey).asBool();
            ctx.SetVariable(stateKey, Variant(isOpen));
            ctx.Log("  [Gate] Toggled -> " + std::string(isOpen ? "Open" : "Closed"));
            return true;
        }

        // Enter 引脚触发（或默认）：如果门是开的则通过
        bool isOpen = ctx.GetVariable(stateKey).asBool();
        ctx.Log("  [Gate] Enter (state=" + std::string(isOpen ? "Open" : "Closed") + ")");
        if (isOpen)
            ctx.ActivateOutputFlow("Exit");
        return true;
    };

    handlers["DoOnce"] = [](ExecutionContext& ctx) {
        auto* node = ctx.GetCurrentNode();
        std::string nodeIdStr = node ? std::to_string(node->id) : "0";
        std::string stateKey = "__doonce_fired_" + nodeIdStr;

        std::string activatedPin = ctx.GetActivatedInputPinName();

        if (activatedPin == "Reset")
        {
            ctx.SetVariable(stateKey, Variant(false));
            ctx.Log("  [DoOnce] Reset");
            return true;
        }

        bool hasFired = ctx.GetVariable(stateKey).asBool();
        if (!hasFired)
        {
            ctx.SetVariable(stateKey, Variant(true));
            ctx.Log("  [DoOnce] First trigger -> Completed");
            ctx.ActivateOutputFlow("Completed");
        }
        else
        {
            ctx.Log("  [DoOnce] Already fired, blocked");
        }
        return true;
    };

    handlers["FlowSequence"] = [](ExecutionContext& ctx) {
        ctx.Log("  [Sequence] Executing all outputs in order");
        auto* node = ctx.GetCurrentNode();
        if (node)
        {
            for (const auto& pin : node->pins)
            {
                if (pin.kind == PinKind::Output && pin.isExec)
                    ctx.ActivateOutputFlow(pin.id);
            }
        }
        return true;
    };

    handlers["Select"] = [](ExecutionContext& ctx) {
        bool cond = ctx.GetInputValue("Condition").asBool();
        auto a = ctx.GetInputValue("A");
        auto b = ctx.GetInputValue("B");
        ctx.SetOutputValue("Result", cond ? a : b);
        return true;
    };

    handlers["SwitchOnInt"] = [](ExecutionContext& ctx) {
        int64_t sel = ctx.GetInputValue("Selection").asInt();
        ctx.Log("  [SwitchOnInt] Selection=" + std::to_string(sel));
        std::string pinName = std::to_string(sel);
        auto* node = ctx.GetCurrentNode();
        bool found = false;
        if (node)
        {
            for (const auto& pin : node->pins)
            {
                if (pin.kind == PinKind::Output && pin.isExec && pin.name == pinName)
                {
                    ctx.ActivateOutputFlow(pin.id);
                    found = true;
                    break;
                }
            }
        }
        if (!found)
            ctx.ActivateOutputFlow("Default");
        return true;
    };

    // MultiGate — 依次激活多个输出（可选循环和随机）
    handlers["MultiGate"] = [](ExecutionContext& ctx) {
        auto* node = ctx.GetCurrentNode();
        std::string nodeIdStr = node ? std::to_string(node->id) : "0";
        std::string indexKey = "__multigate_index_" + nodeIdStr;

        std::string activatedPin = ctx.GetActivatedInputPinName();
        if (activatedPin == "Reset")
        {
            ctx.SetVariable(indexKey, Variant(static_cast<int64_t>(0)));
            ctx.Log("  [MultiGate] Reset");
            return true;
        }

        bool loop = ctx.GetInputValue("Loop").asBool();
        bool random = ctx.GetInputValue("Random").asBool();

        // 收集所有 exec 输出引脚
        std::vector<PinId> execOuts;
        if (node)
        {
            for (const auto& pin : node->pins)
            {
                if (pin.kind == PinKind::Output && pin.isExec)
                    execOuts.push_back(pin.id);
            }
        }

        if (execOuts.empty()) return true;

        int64_t index = ctx.GetVariable(indexKey).asInt();

        if (random)
        {
            int r = rand() % static_cast<int>(execOuts.size());
            ctx.Log("  [MultiGate] Random -> Out " + std::to_string(r));
            ctx.ActivateOutputFlow(execOuts[r]);
        }
        else
        {
            if (index >= static_cast<int64_t>(execOuts.size()))
            {
                if (loop)
                    index = 0;
                else
                {
                    ctx.Log("  [MultiGate] All outputs exhausted");
                    return true;
                }
            }
            ctx.Log("  [MultiGate] -> Out " + std::to_string(index));
            ctx.ActivateOutputFlow(execOuts[static_cast<size_t>(index)]);
            ctx.SetVariable(indexKey, Variant(index + 1));
        }
        return true;
    };

    // ForLoopWithBreak — 可中断的 For 循环
    handlers["ForLoopWithBreak"] = [](ExecutionContext& ctx) {
        auto* node = ctx.GetCurrentNode();
        std::string nodeIdStr = node ? std::to_string(node->id) : "0";
        std::string breakKey = "__forloopbreak_" + nodeIdStr;

        std::string activatedPin = ctx.GetActivatedInputPinName();
        if (activatedPin == "Break")
        {
            ctx.SetVariable(breakKey, Variant(true));
            ctx.Log("  [ForLoopWithBreak] Break requested");
            return true;
        }

        // 重置 break 标志
        ctx.SetVariable(breakKey, Variant(false));

        int64_t first = ctx.GetInputValue("First Index").asInt();
        int64_t last = ctx.GetInputValue("Last Index").asInt();
        ctx.Log("  [ForLoopWithBreak] " + std::to_string(first) + " to " + std::to_string(last));

        for (int64_t i = first; i <= last; ++i)
        {
            // 检查 break 标志
            if (ctx.GetVariable(breakKey).asBool())
            {
                ctx.Log("  [ForLoopWithBreak] Broken at index " + std::to_string(i));
                break;
            }
            ctx.SetOutputValue("Index", Variant(i));
            if (!ctx.ActivateOutputFlow("Loop Body"))
                return false;
        }
        ctx.ActivateOutputFlow("Completed");
        return true;
    };
}

// ============================================================================
// Action 处理器
// ============================================================================

static void RegisterHandlers_Action(
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

        auto timerHandle = ctx.SetTimer(interval, repeat, [&ctx, funcPinId, looping]() {
            ctx.Log("[Timer] fired! looping=" + std::string(looping ? "true" : "false"));
            if (funcPinId != 0)
                ctx.FireConnectedNode(funcPinId);

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

// ============================================================================
// Math 处理器
// ============================================================================

static void RegisterHandlers_Math(std::unordered_map<std::string, NodeHandler>& handlers)
{
    // --- Arithmetic ---
    handlers["Add"] = [](ExecutionContext& ctx) {
        double a = ctx.GetInputValue("A").asFloat();
        double b = ctx.GetInputValue("B").asFloat();
        ctx.SetOutputValue("Result", Variant(a + b));
        return true;
    };

    handlers["Subtract"] = [](ExecutionContext& ctx) {
        double a = ctx.GetInputValue("A").asFloat();
        double b = ctx.GetInputValue("B").asFloat();
        ctx.SetOutputValue("Result", Variant(a - b));
        return true;
    };

    handlers["Multiply"] = [](ExecutionContext& ctx) {
        double a = ctx.GetInputValue("A").asFloat();
        double b = ctx.GetInputValue("B").asFloat();
        ctx.SetOutputValue("Result", Variant(a * b));
        return true;
    };

    handlers["Divide"] = [](ExecutionContext& ctx) {
        double a = ctx.GetInputValue("A").asFloat();
        double b = ctx.GetInputValue("B").asFloat();
        if (b == 0.0) { ctx.Log("  [WARN] Division by zero!"); b = 1.0; }
        ctx.SetOutputValue("Result", Variant(a / b));
        return true;
    };

    // --- Comparison ---
    handlers["Less"] = [](ExecutionContext& ctx) {
        auto node = ctx.GetCurrentNode();
        if (node && node->pins.size() >= 3)
        {
            double a = ctx.GetInputValue(node->pins[0].id).asFloat();
            double b = ctx.GetInputValue(node->pins[1].id).asFloat();
            double result = (a < b) ? 1.0 : 0.0;
            ctx.SetOutputValue(node->pins[2].id, Variant(result));
            ctx.Log("  " + std::to_string(a) + " < " + std::to_string(b) + " = " + std::to_string(result));
        }
        return true;
    };

    handlers["Greater"] = [](ExecutionContext& ctx) {
        auto node = ctx.GetCurrentNode();
        if (node && node->pins.size() >= 3)
        {
            double a = ctx.GetInputValue(node->pins[0].id).asFloat();
            double b = ctx.GetInputValue(node->pins[1].id).asFloat();
            double result = (a > b) ? 1.0 : 0.0;
            ctx.SetOutputValue(node->pins[2].id, Variant(result));
        }
        return true;
    };

    handlers["Equal"] = [](ExecutionContext& ctx) {
        double a = ctx.GetInputValue("A").asFloat();
        double b = ctx.GetInputValue("B").asFloat();
        ctx.SetOutputValue("Result", Variant(a == b));
        return true;
    };

    // --- Logic ---
    handlers["And"] = [](ExecutionContext& ctx) {
        bool a = ctx.GetInputValue("A").asBool();
        bool b = ctx.GetInputValue("B").asBool();
        ctx.SetOutputValue("Result", Variant(a && b));
        return true;
    };

    handlers["Or"] = [](ExecutionContext& ctx) {
        bool a = ctx.GetInputValue("A").asBool();
        bool b = ctx.GetInputValue("B").asBool();
        ctx.SetOutputValue("Result", Variant(a || b));
        return true;
    };

    handlers["Not"] = [](ExecutionContext& ctx) {
        bool v = ctx.GetInputValue("Value").asBool();
        ctx.SetOutputValue("Result", Variant(!v));
        return true;
    };

    // --- Type Conversion ---
    handlers["IntToFloat"] = [](ExecutionContext& ctx) {
        int64_t v = ctx.GetInputValue("Value").asInt();
        ctx.SetOutputValue("Result", Variant(static_cast<double>(v)));
        return true;
    };

    handlers["FloatToInt"] = [](ExecutionContext& ctx) {
        double v = ctx.GetInputValue("Value").asFloat();
        ctx.SetOutputValue("Result", Variant(static_cast<int64_t>(v)));
        return true;
    };

    handlers["FloatToBool"] = [](ExecutionContext& ctx) {
        bool v = ctx.GetInputValue("Value").asBool();
        ctx.SetOutputValue("Result", Variant(v));
        return true;
    };

    handlers["IntToString"] = [](ExecutionContext& ctx) {
        int64_t v = ctx.GetInputValue("Value").asInt();
        ctx.SetOutputValue("Result", Variant(std::to_string(v)));
        return true;
    };

    handlers["FloatToString"] = [](ExecutionContext& ctx) {
        double v = ctx.GetInputValue("Value").asFloat();
        ctx.SetOutputValue("Result", Variant(std::to_string(v)));
        return true;
    };

    // --- Math Functions ---
    handlers["Abs"] = [](ExecutionContext& ctx) {
        double v = ctx.GetInputValue("Value").asFloat();
        ctx.SetOutputValue("Result", Variant(v < 0 ? -v : v));
        return true;
    };

    handlers["Clamp"] = [](ExecutionContext& ctx) {
        double v = ctx.GetInputValue("Value").asFloat();
        double lo = ctx.GetInputValue("Min").asFloat();
        double hi = ctx.GetInputValue("Max").asFloat();
        if (v < lo) v = lo;
        if (v > hi) v = hi;
        ctx.SetOutputValue("Result", Variant(v));
        return true;
    };

    handlers["Min"] = [](ExecutionContext& ctx) {
        double a = ctx.GetInputValue("A").asFloat();
        double b = ctx.GetInputValue("B").asFloat();
        ctx.SetOutputValue("Result", Variant(a < b ? a : b));
        return true;
    };

    handlers["Max"] = [](ExecutionContext& ctx) {
        double a = ctx.GetInputValue("A").asFloat();
        double b = ctx.GetInputValue("B").asFloat();
        ctx.SetOutputValue("Result", Variant(a > b ? a : b));
        return true;
    };

    // --- Random ---
    handlers["Random"] = [](ExecutionContext& ctx) {
        double v = static_cast<double>(rand()) / RAND_MAX;
        ctx.SetOutputValue("Value", Variant(v));
        return true;
    };

    handlers["RandomInRange"] = [](ExecutionContext& ctx) {
        double lo = ctx.GetInputValue("Min").asFloat();
        double hi = ctx.GetInputValue("B").asFloat();
        double v = lo + (static_cast<double>(rand()) / RAND_MAX) * (hi - lo);
        ctx.SetOutputValue("Value", Variant(v));
        return true;
    };

    // --- Misc Math ---
    handlers["Weird"] = [](ExecutionContext& ctx) {
        auto node = ctx.GetCurrentNode();
        if (node && node->pins.size() >= 3)
        {
            double input = ctx.GetInputValue(node->pins[0].id).asFloat();
            ctx.SetOutputValue(node->pins[1].id, Variant(input));
            ctx.SetOutputValue(node->pins[2].id, Variant(input));
        }
        return true;
    };

    // --- More Arithmetic ---
    handlers["Modulo"] = [](ExecutionContext& ctx) {
        double a = ctx.GetInputValue("A").asFloat();
        double b = ctx.GetInputValue("B").asFloat();
        if (b == 0.0) { ctx.Log("  [WARN] Modulo by zero!"); b = 1.0; }
        ctx.SetOutputValue("Result", Variant(std::fmod(a, b)));
        return true;
    };

    handlers["Power"] = [](ExecutionContext& ctx) {
        double base = ctx.GetInputValue("Base").asFloat();
        double exp = ctx.GetInputValue("Exponent").asFloat();
        ctx.SetOutputValue("Result", Variant(std::pow(base, exp)));
        return true;
    };

    handlers["Negate"] = [](ExecutionContext& ctx) {
        double v = ctx.GetInputValue("Value").asFloat();
        ctx.SetOutputValue("Result", Variant(-v));
        return true;
    };

    // --- More Comparison ---
    handlers["NotEqual"] = [](ExecutionContext& ctx) {
        double a = ctx.GetInputValue("A").asFloat();
        double b = ctx.GetInputValue("B").asFloat();
        ctx.SetOutputValue("Result", Variant(a != b));
        return true;
    };

    handlers["LessEqual"] = [](ExecutionContext& ctx) {
        double a = ctx.GetInputValue("A").asFloat();
        double b = ctx.GetInputValue("B").asFloat();
        ctx.SetOutputValue("Result", Variant(a <= b));
        return true;
    };

    handlers["GreaterEqual"] = [](ExecutionContext& ctx) {
        double a = ctx.GetInputValue("A").asFloat();
        double b = ctx.GetInputValue("B").asFloat();
        ctx.SetOutputValue("Result", Variant(a >= b));
        return true;
    };

    // --- More Logic ---
    handlers["Nand"] = [](ExecutionContext& ctx) {
        bool a = ctx.GetInputValue("A").asBool();
        bool b = ctx.GetInputValue("B").asBool();
        ctx.SetOutputValue("Result", Variant(!(a && b)));
        return true;
    };

    handlers["Nor"] = [](ExecutionContext& ctx) {
        bool a = ctx.GetInputValue("A").asBool();
        bool b = ctx.GetInputValue("B").asBool();
        ctx.SetOutputValue("Result", Variant(!(a || b)));
        return true;
    };

    handlers["Xor"] = [](ExecutionContext& ctx) {
        bool a = ctx.GetInputValue("A").asBool();
        bool b = ctx.GetInputValue("B").asBool();
        ctx.SetOutputValue("Result", Variant(a != b));
        return true;
    };

    // --- More Conversion ---
    handlers["StringToInt"] = [](ExecutionContext& ctx) {
        auto str = ctx.GetInputValue("Value").asString();
        bool valid = false;
        int64_t result = 0;
        if (!str.empty())
        {
            char* end = nullptr;
            result = std::strtoll(str.c_str(), &end, 10);
            valid = (end != str.c_str() && *end == '\0');
        }
        ctx.SetOutputValue("Result", Variant(result));
        ctx.SetOutputValue("Valid", Variant(valid));
        return true;
    };

    handlers["StringToFloat"] = [](ExecutionContext& ctx) {
        auto str = ctx.GetInputValue("Value").asString();
        bool valid = false;
        double result = 0.0;
        if (!str.empty())
        {
            char* end = nullptr;
            result = std::strtod(str.c_str(), &end);
            valid = (end != str.c_str() && *end == '\0');
        }
        ctx.SetOutputValue("Result", Variant(result));
        ctx.SetOutputValue("Valid", Variant(valid));
        return true;
    };

    handlers["BoolToString"] = [](ExecutionContext& ctx) {
        bool v = ctx.GetInputValue("Value").asBool();
        ctx.SetOutputValue("Result", Variant(std::string(v ? "True" : "False")));
        return true;
    };

    // --- More Functions ---
    handlers["Sqrt"] = [](ExecutionContext& ctx) {
        auto* node = ctx.GetCurrentNode();
        double v = 0.0;
        if (node && !node->pins.empty())
            v = ctx.GetInputValue(node->pins[0].id).asFloat();
        ctx.SetOutputValue("Result", Variant(std::sqrt(std::abs(v))));
        return true;
    };

    handlers["Sin"] = [](ExecutionContext& ctx) {
        double v = ctx.GetInputValue("Value (Rad)").asFloat();
        ctx.SetOutputValue("Result", Variant(std::sin(v)));
        return true;
    };

    handlers["Cos"] = [](ExecutionContext& ctx) {
        double v = ctx.GetInputValue("Value (Rad)").asFloat();
        ctx.SetOutputValue("Result", Variant(std::cos(v)));
        return true;
    };

    handlers["Tan"] = [](ExecutionContext& ctx) {
        double v = ctx.GetInputValue("Value (Rad)").asFloat();
        ctx.SetOutputValue("Result", Variant(std::tan(v)));
        return true;
    };

    handlers["Atan2"] = [](ExecutionContext& ctx) {
        double y = ctx.GetInputValue("Y").asFloat();
        double x = ctx.GetInputValue("X").asFloat();
        ctx.SetOutputValue("Result (Rad)", Variant(std::atan2(y, x)));
        return true;
    };

    handlers["Lerp"] = [](ExecutionContext& ctx) {
        double a = ctx.GetInputValue("A").asFloat();
        double b = ctx.GetInputValue("B").asFloat();
        double alpha = ctx.GetInputValue("Alpha").asFloat();
        ctx.SetOutputValue("Result", Variant(a + (b - a) * alpha));
        return true;
    };

    handlers["MapRange"] = [](ExecutionContext& ctx) {
        double v = ctx.GetInputValue("Value").asFloat();
        double inMin = ctx.GetInputValue("InMin").asFloat();
        double inMax = ctx.GetInputValue("InMax").asFloat();
        double outMin = ctx.GetInputValue("OutMin").asFloat();
        double outMax = ctx.GetInputValue("OutMax").asFloat();
        double range = inMax - inMin;
        if (range == 0.0) range = 1.0;
        double t = (v - inMin) / range;
        ctx.SetOutputValue("Result", Variant(outMin + (outMax - outMin) * t));
        return true;
    };

    handlers["Ceil"] = [](ExecutionContext& ctx) {
        double v = ctx.GetInputValue("Value").asFloat();
        ctx.SetOutputValue("Result", Variant(std::ceil(v)));
        return true;
    };

    handlers["Floor"] = [](ExecutionContext& ctx) {
        double v = ctx.GetInputValue("Value").asFloat();
        ctx.SetOutputValue("Result", Variant(std::floor(v)));
        return true;
    };

    handlers["Round"] = [](ExecutionContext& ctx) {
        double v = ctx.GetInputValue("Value").asFloat();
        ctx.SetOutputValue("Result", Variant(std::round(v)));
        return true;
    };

    handlers["Sign"] = [](ExecutionContext& ctx) {
        double v = ctx.GetInputValue("Value").asFloat();
        double result = (v > 0.0) ? 1.0 : (v < 0.0 ? -1.0 : 0.0);
        ctx.SetOutputValue("Result", Variant(result));
        return true;
    };

    // --- Constants ---
    handlers["PI"] = [](ExecutionContext& ctx) {
        ctx.SetOutputValue("Value", Variant(3.14159265358979323846));
        return true;
    };

    handlers["E"] = [](ExecutionContext& ctx) {
        ctx.SetOutputValue("Value", Variant(2.71828182845904523536));
        return true;
    };

    // --- Advanced Functions ---
    handlers["InverseLerp"] = [](ExecutionContext& ctx) {
        double a = ctx.GetInputValue("A").asFloat();
        double b = ctx.GetInputValue("B").asFloat();
        double v = ctx.GetInputValue("Value").asFloat();
        double range = b - a;
        if (range == 0.0) range = 1.0;
        ctx.SetOutputValue("Result", Variant((v - a) / range));
        return true;
    };

    handlers["Remap01"] = [](ExecutionContext& ctx) {
        double v = ctx.GetInputValue("Value").asFloat();
        double inMin = ctx.GetInputValue("InMin").asFloat();
        double inMax = ctx.GetInputValue("InMax").asFloat();
        double range = inMax - inMin;
        if (range == 0.0) range = 1.0;
        double result = (v - inMin) / range;
        // Clamp to [0, 1]
        if (result < 0.0) result = 0.0;
        if (result > 1.0) result = 1.0;
        ctx.SetOutputValue("Result", Variant(result));
        return true;
    };

    handlers["DegreesToRadians"] = [](ExecutionContext& ctx) {
        double deg = ctx.GetInputValue("Degrees").asFloat();
        ctx.SetOutputValue("Radians", Variant(deg * 3.14159265358979323846 / 180.0));
        return true;
    };

    handlers["RadiansToDegrees"] = [](ExecutionContext& ctx) {
        double rad = ctx.GetInputValue("Radians").asFloat();
        ctx.SetOutputValue("Degrees", Variant(rad * 180.0 / 3.14159265358979323846));
        return true;
    };

    handlers["Wrap"] = [](ExecutionContext& ctx) {
        double v = ctx.GetInputValue("Value").asFloat();
        double lo = ctx.GetInputValue("Min").asFloat();
        double hi = ctx.GetInputValue("Max").asFloat();
        double range = hi - lo;
        if (range <= 0.0) { ctx.SetOutputValue("Result", Variant(lo)); return true; }
        double result = lo + std::fmod(std::fmod(v - lo, range) + range, range);
        ctx.SetOutputValue("Result", Variant(result));
        return true;
    };

    handlers["Snap"] = [](ExecutionContext& ctx) {
        double v = ctx.GetInputValue("Value").asFloat();
        double grid = ctx.GetInputValue("GridSize").asFloat();
        if (grid <= 0.0) grid = 1.0;
        ctx.SetOutputValue("Result", Variant(std::round(v / grid) * grid));
        return true;
    };

    handlers["Log2"] = [](ExecutionContext& ctx) {
        double v = ctx.GetInputValue("Value").asFloat();
        ctx.SetOutputValue("Result", Variant(v > 0.0 ? std::log2(v) : 0.0));
        return true;
    };

    handlers["Log10"] = [](ExecutionContext& ctx) {
        double v = ctx.GetInputValue("Value").asFloat();
        ctx.SetOutputValue("Result", Variant(v > 0.0 ? std::log10(v) : 0.0));
        return true;
    };

    handlers["Exp"] = [](ExecutionContext& ctx) {
        double v = ctx.GetInputValue("Value").asFloat();
        ctx.SetOutputValue("Result", Variant(std::exp(v)));
        return true;
    };
}

// ============================================================================
// Debug 处理器
// ============================================================================

static void RegisterHandlers_Debug(std::unordered_map<std::string, NodeHandler>& handlers)
{
    handlers["PrintString"] = [](ExecutionContext& ctx) {
        auto str = ctx.GetInputValue("In String").asString();
        ctx.Log("  >>> Print: \"" + str + "\"");
        return true;
    };

    handlers["Log"] = [](ExecutionContext& ctx) {
        auto msg = ctx.GetInputValue("Message").asString();
        ctx.Log("  [LOG] " + msg);
        return true;
    };

    handlers["Assert"] = [](ExecutionContext& ctx) {
        bool condition = ctx.GetInputValue("Condition").asBool();
        auto message = ctx.GetInputValue("Message").asString();
        if (!condition)
        {
            ctx.Log("  [ASSERT FAILED] " + (message.empty() ? "Assertion failed!" : message));
            return false;  // 中断执行
        }
        ctx.Log("  [Assert] Passed");
        return true;
    };

    handlers["FormatLog"] = [](ExecutionContext& ctx) {
        std::string fmt = ctx.GetInputValue("Format").asString();
        const auto* node = ctx.GetCurrentNode();
        if (node)
        {
            std::vector<std::string> args;
            bool skipFirst = true;
            for (const auto& pin : node->pins)
            {
                if (pin.kind == PinKind::Input && !pin.isExec)
                {
                    if (skipFirst) { skipFirst = false; continue; }
                    args.push_back(ctx.GetInputValue(pin.id).asString());
                }
            }
            for (size_t i = 0; i < args.size(); ++i)
            {
                std::string placeholder = "{" + std::to_string(i) + "}";
                size_t pos = 0;
                while ((pos = fmt.find(placeholder, pos)) != std::string::npos)
                {
                    fmt.replace(pos, placeholder.size(), args[i]);
                    pos += args[i].size();
                }
            }
        }
        ctx.Log("  [FORMAT] " + fmt);
        return true;
    };
}

// ============================================================================
// String 处理器
// ============================================================================

static void RegisterHandlers_String(std::unordered_map<std::string, NodeHandler>& handlers)
{
    handlers["MakeString"] = [](ExecutionContext& ctx) {
        auto v = ctx.GetInputValue("Value").asString();
        ctx.SetOutputValue("String", Variant(v));
        return true;
    };

    handlers["AppendString"] = [](ExecutionContext& ctx) {
        std::string result;
        const auto* node = ctx.GetCurrentNode();
        if (node)
        {
            for (const auto& pin : node->pins)
            {
                if (pin.kind == PinKind::Input && !pin.isExec)
                    result += ctx.GetInputValue(pin.id).asString();
            }
        }
        else
        {
            result = ctx.GetInputValue("A").asString() + ctx.GetInputValue("B").asString();
        }
        ctx.SetOutputValue("Result", Variant(result));
        return true;
    };

    handlers["StringLength"] = [](ExecutionContext& ctx) {
        auto s = ctx.GetInputValue("String").asString();
        ctx.SetOutputValue("Length", Variant(static_cast<int64_t>(s.size())));
        return true;
    };

    handlers["StringEquals"] = [](ExecutionContext& ctx) {
        auto a = ctx.GetInputValue("A").asString();
        auto b = ctx.GetInputValue("B").asString();
        ctx.SetOutputValue("Result", Variant(a == b));
        return true;
    };

    handlers["StringEqualsIgnoreCase"] = [](ExecutionContext& ctx) {
        auto a = ctx.GetInputValue("A").asString();
        auto b = ctx.GetInputValue("B").asString();
        bool equal = false;
        if (a.size() == b.size())
        {
            equal = std::equal(a.begin(), a.end(), b.begin(),
                [](char ca, char cb) { return std::tolower(static_cast<unsigned char>(ca)) == std::tolower(static_cast<unsigned char>(cb)); });
        }
        ctx.SetOutputValue("Result", Variant(equal));
        return true;
    };

    handlers["StringContains"] = [](ExecutionContext& ctx) {
        auto str = ctx.GetInputValue("String").asString();
        auto sub = ctx.GetInputValue("Substring").asString();
        bool found = !sub.empty() && str.find(sub) != std::string::npos;
        ctx.SetOutputValue("Result", Variant(found));
        return true;
    };

    handlers["StringStartsWith"] = [](ExecutionContext& ctx) {
        auto str = ctx.GetInputValue("String").asString();
        auto prefix = ctx.GetInputValue("Prefix").asString();
        bool result = false;
        if (prefix.size() <= str.size())
            result = str.compare(0, prefix.size(), prefix) == 0;
        ctx.SetOutputValue("Result", Variant(result));
        return true;
    };

    handlers["StringEndsWith"] = [](ExecutionContext& ctx) {
        auto str = ctx.GetInputValue("String").asString();
        auto suffix = ctx.GetInputValue("Suffix").asString();
        bool result = false;
        if (suffix.size() <= str.size())
            result = str.compare(str.size() - suffix.size(), suffix.size(), suffix) == 0;
        ctx.SetOutputValue("Result", Variant(result));
        return true;
    };

    handlers["StringReplace"] = [](ExecutionContext& ctx) {
        auto str = ctx.GetInputValue("String").asString();
        auto from = ctx.GetInputValue("From").asString();
        auto to = ctx.GetInputValue("To").asString();
        if (!from.empty())
        {
            size_t pos = 0;
            while ((pos = str.find(from, pos)) != std::string::npos)
            {
                str.replace(pos, from.size(), to);
                pos += to.size();
            }
        }
        ctx.SetOutputValue("Result", Variant(str));
        return true;
    };

    handlers["StringToUpper"] = [](ExecutionContext& ctx) {
        auto str = ctx.GetInputValue("String").asString();
        std::transform(str.begin(), str.end(), str.begin(),
            [](unsigned char c) { return static_cast<char>(std::toupper(c)); });
        ctx.SetOutputValue("Result", Variant(str));
        return true;
    };

    handlers["StringToLower"] = [](ExecutionContext& ctx) {
        auto str = ctx.GetInputValue("String").asString();
        std::transform(str.begin(), str.end(), str.begin(),
            [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
        ctx.SetOutputValue("Result", Variant(str));
        return true;
    };

    handlers["StringSubstring"] = [](ExecutionContext& ctx) {
        auto str = ctx.GetInputValue("String").asString();
        int64_t start = ctx.GetInputValue("Start").asInt();
        int64_t count = ctx.GetInputValue("Count").asInt();
        std::string result;
        if (start >= 0 && static_cast<size_t>(start) < str.size())
        {
            if (count < 0)
                result = str.substr(static_cast<size_t>(start));
            else
                result = str.substr(static_cast<size_t>(start), static_cast<size_t>(count));
        }
        ctx.SetOutputValue("Result", Variant(result));
        return true;
    };

    handlers["StringFind"] = [](ExecutionContext& ctx) {
        auto str = ctx.GetInputValue("String").asString();
        auto sub = ctx.GetInputValue("Substring").asString();
        auto pos = str.find(sub);
        bool found = (pos != std::string::npos);
        ctx.SetOutputValue("Index", Variant(found ? static_cast<int64_t>(pos) : static_cast<int64_t>(-1)));
        ctx.SetOutputValue("Found", Variant(found));
        return true;
    };

    handlers["StringSplit"] = [](ExecutionContext& ctx) {
        auto str = ctx.GetInputValue("String").asString();
        auto delim = ctx.GetInputValue("Delimiter").asString();
        std::vector<Variant> parts;
        if (delim.empty())
        {
            for (size_t i = 0; i < str.size(); ++i)
                parts.push_back(Variant(std::string(1, str[i])));
        }
        else
        {
            size_t pos = 0;
            size_t found;
            while ((found = str.find(delim, pos)) != std::string::npos)
            {
                parts.push_back(Variant(str.substr(pos, found - pos)));
                pos = found + delim.size();
            }
            parts.push_back(Variant(str.substr(pos)));
        }
        ctx.SetOutputValue("Count", Variant(static_cast<int64_t>(parts.size())));
        ctx.SetOutputValue("Array", Variant(std::move(parts)));
        return true;
    };

    handlers["StringTrimmed"] = [](ExecutionContext& ctx) {
        auto str = ctx.GetInputValue("String").asString();
        size_t start = 0;
        while (start < str.size() && std::isspace(static_cast<unsigned char>(str[start])))
            ++start;
        size_t end = str.size();
        while (end > start && std::isspace(static_cast<unsigned char>(str[end - 1])))
            --end;
        ctx.SetOutputValue("Result", Variant(str.substr(start, end - start)));
        return true;
    };

    // FormatString — 用 {0}, {1}, ... 替换参数
    handlers["FormatString"] = [](ExecutionContext& ctx) {
        std::string fmt = ctx.GetInputValue("Format").asString();
        const auto* node = ctx.GetCurrentNode();
        if (node)
        {
            // 收集除 "Format" 外的所有输入引脚值
            std::vector<std::string> args;
            bool skipFirst = true;
            for (const auto& pin : node->pins)
            {
                if (pin.kind == PinKind::Input && !pin.isExec)
                {
                    if (skipFirst) { skipFirst = false; continue; } // 跳过 "Format"
                    args.push_back(ctx.GetInputValue(pin.id).asString());
                }
            }

            // 替换 {0}, {1}, ...
            for (size_t i = 0; i < args.size(); ++i)
            {
                std::string placeholder = "{" + std::to_string(i) + "}";
                size_t pos = 0;
                while ((pos = fmt.find(placeholder, pos)) != std::string::npos)
                {
                    fmt.replace(pos, placeholder.size(), args[i]);
                    pos += args[i].size();
                }
            }
        }
        ctx.SetOutputValue("Result", Variant(fmt));
        return true;
    };

    // StringJoin — 用分隔符连接数组
    handlers["StringJoin"] = [](ExecutionContext& ctx) {
        auto arr = ctx.GetInputValue("Array");
        auto sep = ctx.GetInputValue("Separator").asString();
        std::string result;
        for (size_t i = 0; i < arr.arraySize(); ++i)
        {
            if (i > 0) result += sep;
            result += arr.arrayGet(i).asString();
        }
        ctx.SetOutputValue("Result", Variant(result));
        return true;
    };

    // StringRepeat — 重复字符串 N 次
    handlers["StringRepeat"] = [](ExecutionContext& ctx) {
        auto str = ctx.GetInputValue("String").asString();
        int64_t count = ctx.GetInputValue("Count").asInt();
        if (count < 0) count = 0;
        if (count > 10000) count = 10000; // 安全限制
        std::string result;
        result.reserve(str.size() * static_cast<size_t>(count));
        for (int64_t i = 0; i < count; ++i)
            result += str;
        ctx.SetOutputValue("Result", Variant(result));
        return true;
    };

    // StringPadLeft — 左侧填充
    handlers["StringPadLeft"] = [](ExecutionContext& ctx) {
        auto str = ctx.GetInputValue("String").asString();
        int64_t totalWidth = ctx.GetInputValue("TotalWidth").asInt();
        auto padChar = ctx.GetInputValue("PadChar").asString();
        char pad = padChar.empty() ? ' ' : padChar[0];
        while (static_cast<int64_t>(str.size()) < totalWidth)
            str.insert(str.begin(), pad);
        ctx.SetOutputValue("Result", Variant(str));
        return true;
    };

    // StringPadRight — 右侧填充
    handlers["StringPadRight"] = [](ExecutionContext& ctx) {
        auto str = ctx.GetInputValue("String").asString();
        int64_t totalWidth = ctx.GetInputValue("TotalWidth").asInt();
        auto padChar = ctx.GetInputValue("PadChar").asString();
        char pad = padChar.empty() ? ' ' : padChar[0];
        while (static_cast<int64_t>(str.size()) < totalWidth)
            str.push_back(pad);
        ctx.SetOutputValue("Result", Variant(str));
        return true;
    };

    // CharAt — 获取指定位置字符
    handlers["CharAt"] = [](ExecutionContext& ctx) {
        auto str = ctx.GetInputValue("String").asString();
        int64_t index = ctx.GetInputValue("Index").asInt();
        bool valid = (index >= 0 && static_cast<size_t>(index) < str.size());
        if (valid)
            ctx.SetOutputValue("Char", Variant(std::string(1, str[static_cast<size_t>(index)])));
        else
            ctx.SetOutputValue("Char", Variant(std::string()));
        ctx.SetOutputValue("Valid", Variant(valid));
        return true;
    };

    // StringReverse — 反转字符串
    handlers["StringReverse"] = [](ExecutionContext& ctx) {
        auto str = ctx.GetInputValue("String").asString();
        std::reverse(str.begin(), str.end());
        ctx.SetOutputValue("Result", Variant(str));
        return true;
    };
}

// ============================================================================
// Map 处理器
// Key 支持任意简单类型（Boolean/Integer/Float/String），
// 运行时通过 asString() 自动转为字符串键，底层存储仍为 string -> Variant。
// ============================================================================

static void RegisterHandlers_Map(std::unordered_map<std::string, NodeHandler>& handlers)
{
    handlers["MakeMap"] = [](ExecutionContext& ctx) {
        const auto* node = ctx.GetCurrentNode();
        std::unordered_map<std::string, Variant> result;
        if (node)
        {
            // 从输入引脚中按 Key/Value 配对收集
            std::vector<const PinInfo*> dataPins;
            for (const auto& pin : node->pins)
            {
                if (pin.kind == PinKind::Input && !pin.isExec)
                    dataPins.push_back(&pin);
            }
            // 每两个引脚为一组：Key, Value
            // Key 支持任意类型，通过 asString() 统一转为字符串键
            for (size_t i = 0; i + 1 < dataPins.size(); i += 2)
            {
                std::string key = ctx.GetInputValue(dataPins[i]->id).asString();
                Variant val = ctx.GetInputValue(dataPins[i + 1]->id);
                if (!key.empty())
                    result[key] = val;
            }
        }
        ctx.SetOutputValue("Map", Variant(std::move(result)));
        return true;
    };

    handlers["MapGet"] = [](ExecutionContext& ctx) {
        auto map = ctx.GetInputValue("Map");
        auto key = ctx.GetInputValue("Key").asString();
        bool found = map.mapHasKey(key);
        if (found)
            ctx.SetOutputValue("Value", map.mapGet(key));
        else
            ctx.SetOutputValue("Value", Variant());
        ctx.SetOutputValue("Found", Variant(found));
        return true;
    };

    handlers["MapSet"] = [](ExecutionContext& ctx) {
        auto map = ctx.GetInputValue("Map");
        auto key = ctx.GetInputValue("Key").asString();
        auto value = ctx.GetInputValue("Value");
        map.mapSet(key, value);
        ctx.SetOutputValue("Map", map);
        ctx.Log("  [MapSet] Set key \"" + key + "\" -> " + value.asString());
        return true;
    };

    handlers["MapRemove"] = [](ExecutionContext& ctx) {
        auto map = ctx.GetInputValue("Map");
        auto key = ctx.GetInputValue("Key").asString();
        bool removed = map.mapRemove(key);
        ctx.SetOutputValue("Map", map);
        ctx.SetOutputValue("Removed", Variant(removed));
        ctx.Log("  [MapRemove] Key \"" + key + "\" " + (removed ? "removed" : "not found"));
        return true;
    };

    handlers["MapHasKey"] = [](ExecutionContext& ctx) {
        auto map = ctx.GetInputValue("Map");
        auto key = ctx.GetInputValue("Key").asString();
        ctx.SetOutputValue("Result", Variant(map.mapHasKey(key)));
        return true;
    };

    handlers["MapSize"] = [](ExecutionContext& ctx) {
        auto map = ctx.GetInputValue("Map");
        ctx.SetOutputValue("Size", Variant(static_cast<int64_t>(map.mapSize())));
        return true;
    };

    handlers["MapKeys"] = [](ExecutionContext& ctx) {
        auto map = ctx.GetInputValue("Map");
        auto keys = map.mapKeys();
        std::vector<Variant> keyVariants;
        keyVariants.reserve(keys.size());
        for (const auto& k : keys)
            keyVariants.push_back(Variant(k));
        ctx.SetOutputValue("Keys", Variant(std::move(keyVariants)));
        return true;
    };

    handlers["MapValues"] = [](ExecutionContext& ctx) {
        auto map = ctx.GetInputValue("Map");
        auto values = map.mapValues();
        ctx.SetOutputValue("Values", Variant(std::move(values)));
        return true;
    };

    handlers["MapClear"] = [](ExecutionContext& ctx) {
        auto map = ctx.GetInputValue("Map");
        ctx.Log("  [MapClear] Cleared map (was size " + std::to_string(map.mapSize()) + ")");
        map.mapClear();
        ctx.SetOutputValue("Map", map);
        return true;
    };

    handlers["MapMerge"] = [](ExecutionContext& ctx) {
        auto mapA = ctx.GetInputValue("Map A");
        auto mapB = ctx.GetInputValue("Map B");
        // 以 Map A 为基础，Map B 的键值对覆盖/追加
        auto keysB = mapB.mapKeys();
        for (const auto& key : keysB)
            mapA.mapSet(key, mapB.mapGet(key));
        ctx.SetOutputValue("Map", mapA);
        ctx.Log("  [MapMerge] Merged (A size=" + std::to_string(mapA.mapSize()) + ")");
        return true;
    };

    handlers["ForEachMapLoop"] = [](ExecutionContext& ctx) {
        auto map = ctx.GetInputValue("Map");
        auto keys = map.mapKeys();
        ctx.Log("  [ForEachMapLoop] Map size = " + std::to_string(keys.size()));
        for (const auto& key : keys)
        {
            ctx.SetOutputValue("Key", Variant(key));
            ctx.SetOutputValue("Value", map.mapGet(key));
            if (!ctx.ActivateOutputFlow("Loop Body"))
                return false;
        }
        ctx.ActivateOutputFlow("Completed");
        return true;
    };
}

// ============================================================================
// Array 处理器
// ============================================================================

static void RegisterHandlers_Array(std::unordered_map<std::string, NodeHandler>& handlers)
{
    handlers["ArrayLength"] = [](ExecutionContext& ctx) {
        auto arr = ctx.GetInputValue("Array");
        ctx.SetOutputValue("Length", Variant(static_cast<int64_t>(arr.arraySize())));
        return true;
    };

    handlers["ArrayGet"] = [](ExecutionContext& ctx) {
        auto arr = ctx.GetInputValue("Array");
        int64_t index = ctx.GetInputValue("Index").asInt();
        bool valid = (index >= 0 && static_cast<size_t>(index) < arr.arraySize());
        if (valid)
            ctx.SetOutputValue("Element", arr.arrayGet(static_cast<size_t>(index)));
        else
            ctx.SetOutputValue("Element", Variant());
        ctx.SetOutputValue("Valid", Variant(valid));
        return true;
    };

    handlers["ArraySet"] = [](ExecutionContext& ctx) {
        auto arr = ctx.GetInputValue("Array");
        int64_t index = ctx.GetInputValue("Index").asInt();
        auto element = ctx.GetInputValue("Element");

        if (index < 0)
        {
            ctx.Log("  [ArraySet] Error: negative index " + std::to_string(index));
            ctx.SetOutputValue("Array", arr);
            return true;
        }

        arr.arraySet(static_cast<size_t>(index), element);
        ctx.SetOutputValue("Array", arr);
        ctx.Log("  [ArraySet] Set index " + std::to_string(index) + " -> " + element.asString());
        return true;
    };

    handlers["ArrayRemoveAt"] = [](ExecutionContext& ctx) {
        auto arr = ctx.GetInputValue("Array");
        int64_t index = ctx.GetInputValue("Index").asInt();

        bool valid = (index >= 0 && static_cast<size_t>(index) < arr.arraySize());
        if (valid)
        {
            arr.arrayRemoveAt(static_cast<size_t>(index));
            ctx.Log("  [ArrayRemoveAt] Removed index " + std::to_string(index) + ", new size = " + std::to_string(arr.arraySize()));
        }
        else
        {
            ctx.Log("  [ArrayRemoveAt] Invalid index " + std::to_string(index) + " (size=" + std::to_string(arr.arraySize()) + ")");
        }
        ctx.SetOutputValue("Array", arr);
        ctx.SetOutputValue("Valid", Variant(valid));
        return true;
    };

    handlers["ArrayClear"] = [](ExecutionContext& ctx) {
        auto arr = ctx.GetInputValue("Array");
        ctx.Log("  [ArrayClear] Cleared array (was size " + std::to_string(arr.arraySize()) + ")");
        arr.arrayClear();
        ctx.SetOutputValue("Array", arr);
        return true;
    };

    handlers["ForEachLoop"] = [](ExecutionContext& ctx) {
        auto arr = ctx.GetInputValue("Array");
        size_t count = arr.arraySize();
        ctx.Log("  [ForEachLoop] Array size = " + std::to_string(count));
        for (size_t i = 0; i < count; ++i)
        {
            ctx.SetOutputValue("Array Element", arr.arrayGet(i));
            ctx.SetOutputValue("Array Index", Variant(static_cast<int64_t>(i)));
            if (!ctx.ActivateOutputFlow("Loop Body"))
                return false;
        }
        ctx.ActivateOutputFlow("Completed");
        return true;
    };

    handlers["ArrayAdd"] = [](ExecutionContext& ctx) {
        auto arr = ctx.GetInputValue("Array");
        auto element = ctx.GetInputValue("Element");
        // 确保是数组类型
        if (arr.type != PinDataType::Array)
        {
            arr.type = PinDataType::Array;
            arr.arrayValue.clear();
        }
        arr.arrayValue.push_back(element);
        ctx.SetOutputValue("Array", arr);
        ctx.SetOutputValue("New Length", Variant(static_cast<int64_t>(arr.arraySize())));
        ctx.Log("  [ArrayAdd] New size = " + std::to_string(arr.arraySize()));
        return true;
    };

    handlers["ArrayInsert"] = [](ExecutionContext& ctx) {
        auto arr = ctx.GetInputValue("Array");
        int64_t index = ctx.GetInputValue("Index").asInt();
        auto element = ctx.GetInputValue("Element");

        if (arr.type != PinDataType::Array)
        {
            arr.type = PinDataType::Array;
            arr.arrayValue.clear();
        }

        if (index < 0) index = 0;
        if (static_cast<size_t>(index) > arr.arrayValue.size())
            index = static_cast<int64_t>(arr.arrayValue.size());

        arr.arrayValue.insert(arr.arrayValue.begin() + static_cast<std::ptrdiff_t>(index), element);
        ctx.SetOutputValue("Array", arr);
        ctx.Log("  [ArrayInsert] Inserted at " + std::to_string(index) + ", new size = " + std::to_string(arr.arraySize()));
        return true;
    };

    handlers["ArrayContains"] = [](ExecutionContext& ctx) {
        auto arr = ctx.GetInputValue("Array");
        auto element = ctx.GetInputValue("Element");
        std::string searchStr = element.asString();
        bool found = false;
        int64_t foundIndex = -1;
        for (size_t i = 0; i < arr.arraySize(); ++i)
        {
            if (arr.arrayGet(i).asString() == searchStr)
            {
                found = true;
                foundIndex = static_cast<int64_t>(i);
                break;
            }
        }
        ctx.SetOutputValue("Found", Variant(found));
        ctx.SetOutputValue("Index", Variant(foundIndex));
        return true;
    };

    handlers["ArrayReverse"] = [](ExecutionContext& ctx) {
        auto arr = ctx.GetInputValue("Array");
        if (arr.type == PinDataType::Array)
            std::reverse(arr.arrayValue.begin(), arr.arrayValue.end());
        ctx.SetOutputValue("Array", arr);
        ctx.Log("  [ArrayReverse] Reversed array of size " + std::to_string(arr.arraySize()));
        return true;
    };

    handlers["MakeArray"] = [](ExecutionContext& ctx) {
        const auto* node = ctx.GetCurrentNode();
        std::vector<Variant> elements;
        if (node)
        {
            for (const auto& pin : node->pins)
            {
                if (pin.kind == PinKind::Input && !pin.isExec)
                    elements.push_back(ctx.GetInputValue(pin.id));
            }
        }
        ctx.SetOutputValue("Array", Variant(std::move(elements)));
        return true;
    };
}

// ============================================================================
// Tree 处理器
// ============================================================================

static void RegisterHandlers_Tree(std::unordered_map<std::string, NodeHandler>& handlers)
{
    handlers["Sequence"] = [](ExecutionContext& ctx) {
        ctx.Log("  [Sequence] executing all outputs");
        return true;
    };

    handlers["MoveTo"] = [](ExecutionContext& ctx) {
        ctx.Log("  [Task] Moving to target...");
        return true;
    };

    handlers["RandomWait"] = [](ExecutionContext& ctx) {
        ctx.Log("  [Task] Waiting random time...");
        return true;
    };
}

// ============================================================================
// Houdini 处理器
// ============================================================================

static void RegisterHandlers_Houdini(std::unordered_map<std::string, NodeHandler>& handlers)
{
    handlers["HoudiniTransform"] = [](ExecutionContext& ctx) {
        ctx.Log("  [Houdini] Transform applied");
        return true;
    };

    handlers["HoudiniGroup"] = [](ExecutionContext& ctx) {
        ctx.Log("  [Houdini] Group created");
        return true;
    };
}

// ============================================================================
// Time 处理器
// ============================================================================

static void RegisterHandlers_Time(
    std::unordered_map<std::string, NodeHandler>& handlers,
    BlueprintRunner& runner)
{
    handlers["GetTime"] = [](ExecutionContext& ctx) {
        double seconds = static_cast<double>(FrameTimerManager::GetCurrentUnixTime());
        ctx.SetOutputValue("Seconds", Variant(seconds));
        return true;
    };

    handlers["DeltaTime"] = [](ExecutionContext& ctx) {
        // 从 TimerManager 获取上一帧 deltaTime（近似值）
        // 注意：实际精确值需要从外部传入，这里用 timer manager 的最后 tick 间隔
        ctx.SetOutputValue("Seconds", Variant(0.016));  // 默认 ~60fps
        return true;
    };

    handlers["TimeSince"] = [](ExecutionContext& ctx) {
        double timestamp = ctx.GetInputValue("Timestamp").asFloat();
        double now = static_cast<double>(FrameTimerManager::GetCurrentUnixTime());
        ctx.SetOutputValue("Elapsed", Variant(now - timestamp));
        return true;
    };

    handlers["FormatTime"] = [](ExecutionContext& ctx) {
        double seconds = ctx.GetInputValue("Seconds").asFloat();
        int totalSec = static_cast<int>(seconds);
        int hours = totalSec / 3600;
        int mins = (totalSec % 3600) / 60;
        int secs = totalSec % 60;
        int ms = static_cast<int>((seconds - totalSec) * 1000);

        char buf[64];
        if (hours > 0)
            snprintf(buf, sizeof(buf), "%02d:%02d:%02d.%03d", hours, mins, secs, ms);
        else
            snprintf(buf, sizeof(buf), "%02d:%02d.%03d", mins, secs, ms);
        ctx.SetOutputValue("Formatted", Variant(std::string(buf)));
        return true;
    };

    handlers["TimerInfo"] = [&runner](ExecutionContext& ctx) {
        int64_t handleVal = ctx.GetInputValue("TimerHandle").asInt();
        auto handle = static_cast<TimerHandle>(handleVal);

        bool isActive = false;
        bool isPaused = false;
        double elapsed = 0.0;
        double remaining = 0.0;

        if (handle != 0)
        {
            const auto& timers = runner.GetTimerManager().GetAllTimers();
            for (const auto& t : timers)
            {
                if (t.handle == handle && !t.pendingKill)
                {
                    isActive = true;
                    isPaused = t.paused;
                    elapsed = static_cast<double>(t.totalElapsed);
                    remaining = static_cast<double>(t.remaining);
                    if (remaining < 0.0) remaining = 0.0;
                    break;
                }
            }
        }

        ctx.SetOutputValue("IsActive", Variant(isActive));
        ctx.SetOutputValue("IsPaused", Variant(isPaused));
        ctx.SetOutputValue("Elapsed", Variant(elapsed));
        ctx.SetOutputValue("Remaining", Variant(remaining));
        return true;
    };
}

// ============================================================================
// Data 处理器
// ============================================================================

static void RegisterHandlers_Data(std::unordered_map<std::string, NodeHandler>& handlers)
{
    handlers["ToJSON"] = [](ExecutionContext& ctx) {
        auto val = ctx.GetInputValue("Value");
        std::string json;
        switch (val.type)
        {
        case PinDataType::Boolean:
            json = val.boolValue ? "true" : "false";
            break;
        case PinDataType::Integer:
            json = std::to_string(val.intValue);
            break;
        case PinDataType::Float:
            json = std::to_string(val.floatValue);
            break;
        case PinDataType::String:
            json = "\"" + val.stringValue + "\"";
            break;
        case PinDataType::Array:
        {
            json = "[";
            for (size_t i = 0; i < val.arraySize(); ++i)
            {
                if (i > 0) json += ", ";
                auto elem = val.arrayGet(i);
                if (elem.type == PinDataType::String)
                    json += "\"" + elem.asString() + "\"";
                else
                    json += elem.asString();
            }
            json += "]";
            break;
        }
        default:
            json = "null";
            break;
        }
        ctx.SetOutputValue("JSON", Variant(json));
        return true;
    };

    handlers["FromJSON"] = [](ExecutionContext& ctx) {
        auto json = ctx.GetInputValue("JSON").asString();
        bool valid = false;

        // 简单 JSON 值解析
        // 去除首尾空白
        size_t start = json.find_first_not_of(" \t\n\r");
        size_t end = json.find_last_not_of(" \t\n\r");
        if (start != std::string::npos && end != std::string::npos)
        {
            std::string trimmed = json.substr(start, end - start + 1);
            if (trimmed == "true")
            {
                ctx.SetOutputValue("Value", Variant(true));
                valid = true;
            }
            else if (trimmed == "false")
            {
                ctx.SetOutputValue("Value", Variant(false));
                valid = true;
            }
            else if (trimmed == "null")
            {
                ctx.SetOutputValue("Value", Variant());
                valid = true;
            }
            else if (trimmed.size() >= 2 && trimmed.front() == '"' && trimmed.back() == '"')
            {
                ctx.SetOutputValue("Value", Variant(trimmed.substr(1, trimmed.size() - 2)));
                valid = true;
            }
            else
            {
                // 尝试解析数字
                try
                {
                    if (trimmed.find('.') != std::string::npos)
                    {
                        double v = std::stod(trimmed);
                        ctx.SetOutputValue("Value", Variant(v));
                        valid = true;
                    }
                    else
                    {
                        int64_t v = std::stoll(trimmed);
                        ctx.SetOutputValue("Value", Variant(v));
                        valid = true;
                    }
                }
                catch (...) {}
            }
        }
        if (!valid)
            ctx.SetOutputValue("Value", Variant());
        ctx.SetOutputValue("Valid", Variant(valid));
        return true;
    };

    handlers["HasKey"] = [](ExecutionContext& ctx) {
        auto json = ctx.GetInputValue("JSON").asString();
        auto key = ctx.GetInputValue("Key").asString();
        // 简单的字符串搜索（查找 "key": 模式）
        std::string pattern = "\"" + key + "\"";
        bool found = json.find(pattern) != std::string::npos;
        ctx.SetOutputValue("Result", Variant(found));
        return true;
    };

    handlers["GetField"] = [](ExecutionContext& ctx) {
        auto json = ctx.GetInputValue("JSON").asString();
        auto key = ctx.GetInputValue("Key").asString();
        std::string pattern = "\"" + key + "\"";
        auto pos = json.find(pattern);
        bool found = false;
        std::string value;
        if (pos != std::string::npos)
        {
            pos += pattern.size();
            // 跳过 : 和空白
            while (pos < json.size() && (json[pos] == ':' || json[pos] == ' ' || json[pos] == '\t'))
                ++pos;
            if (pos < json.size())
            {
                if (json[pos] == '"')
                {
                    ++pos;
                    size_t end = json.find('"', pos);
                    if (end != std::string::npos)
                    {
                        value = json.substr(pos, end - pos);
                        found = true;
                    }
                }
                else
                {
                    // 数字或布尔值
                    size_t end = json.find_first_of(",]} \t\n\r", pos);
                    if (end == std::string::npos) end = json.size();
                    value = json.substr(pos, end - pos);
                    found = true;
                }
            }
        }
        ctx.SetOutputValue("Value", Variant(value));
        ctx.SetOutputValue("Found", Variant(found));
        return true;
    };

    handlers["ArrayToString"] = [](ExecutionContext& ctx) {
        auto arr = ctx.GetInputValue("Array");
        std::string result = "[";
        for (size_t i = 0; i < arr.arraySize(); ++i)
        {
            if (i > 0) result += ", ";
            result += arr.arrayGet(i).asString();
        }
        result += "]";
        ctx.SetOutputValue("String", Variant(result));
        return true;
    };
}

// ============================================================================
// Misc 处理器
// ============================================================================

static void RegisterHandlers_Misc(std::unordered_map<std::string, NodeHandler>& handlers)
{
    handlers["GetVariable"] = [](ExecutionContext& ctx) {
        auto name = ctx.GetInputValue("Name").asString();
        ctx.SetOutputValue("Value", ctx.GetVariable(name));
        return true;
    };

    handlers["SetVariable"] = [](ExecutionContext& ctx) {
        auto name = ctx.GetInputValue("Name").asString();
        auto val = ctx.GetInputValue("Value");
        ctx.SetVariable(name, val);
        ctx.Log("  Set '" + name + "' = '" + val.asString() + "'");
        return true;
    };

    handlers["IsValid"] = [](ExecutionContext& ctx) {
        auto val = ctx.GetInputValue("Value");
        bool isValid = false;
        switch (val.type)
        {
        case PinDataType::String:  isValid = !val.stringValue.empty(); break;
        case PinDataType::Object:  isValid = !val.stringValue.empty(); break;
        case PinDataType::Array:   isValid = !val.arrayValue.empty(); break;
        case PinDataType::Integer: isValid = val.intValue != 0; break;
        case PinDataType::Float:   isValid = val.floatValue != 0.0; break;
        case PinDataType::Boolean: isValid = val.boolValue; break;
        default: break;
        }
        ctx.SetOutputValue("Is Valid", Variant(isValid));
        return true;
    };

    handlers["MakeLiteralBool"] = [](ExecutionContext& ctx) {
        ctx.SetOutputValue("Result", Variant(ctx.GetInputValue("Value").asBool()));
        return true;
    };

    handlers["MakeLiteralInt"] = [](ExecutionContext& ctx) {
        ctx.SetOutputValue("Result", Variant(ctx.GetInputValue("Value").asInt()));
        return true;
    };

    handlers["MakeLiteralFloat"] = [](ExecutionContext& ctx) {
        ctx.SetOutputValue("Result", Variant(ctx.GetInputValue("Value").asFloat()));
        return true;
    };

    handlers["MakeLiteralString"] = [](ExecutionContext& ctx) {
        ctx.SetOutputValue("Result", Variant(ctx.GetInputValue("Value").asString()));
        return true;
    };
}

// ============================================================================
// 入口函数
// ============================================================================

void RegisterBuiltinHandlers(
    BlueprintRunner& runner,
    const std::string& basePath,
    std::unordered_map<std::string, NodeHandler>* outHandlers)
{
    std::unordered_map<std::string, NodeHandler> allHandlers;

    // 注册各分类的处理器
    RegisterHandlers_Flow(allHandlers, runner, basePath);
    RegisterHandlers_Action(allHandlers, runner);
    RegisterHandlers_Math(allHandlers);
    RegisterHandlers_Debug(allHandlers);
    RegisterHandlers_String(allHandlers);
    RegisterHandlers_Array(allHandlers);
    RegisterHandlers_Map(allHandlers);
    RegisterHandlers_Time(allHandlers, runner);
    RegisterHandlers_Data(allHandlers);
    RegisterHandlers_Tree(allHandlers);
    RegisterHandlers_Houdini(allHandlers);
    RegisterHandlers_Misc(allHandlers);

    // 设置默认处理器
    runner.SetDefaultHandler([](ExecutionContext& ctx) {
        ctx.Log("  [Default Handler] pass-through");
        return true;
    });

    // 批量注册到 runner
    runner.RegisterHandlers(allHandlers);

    // 如果调用者需要 handlers 映射表（供 Editor 层保存或传递给子蓝图）
    if (outHandlers)
        *outHandlers = std::move(allHandlers);
}

} // namespace Runtime
} // namespace NodeEditor
