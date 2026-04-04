// Runtime/BuiltinHandlers_Flow.cpp -- Flow 控制流节点处理器
#include "BuiltinHandlers_Flow.h"
#include "../BlueprintExporter.h"
#include <cstdlib>
#include <algorithm>

namespace NodeEditor {
namespace Runtime {

void RegisterHandlers_Flow(
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

    // ExecuteBlueprint — 加载并执行另一个蓝图文件（完全异步）
    // 依赖：basePath（路径解析）、runner（获取 handlers + timer）
    handlers["ExecuteBlueprint"] = [&runner, basePath](ExecutionContext& ctx) {
        auto filePath = ctx.GetInputValue("File").asString();
        ctx.Log("  [ExecuteBlueprint] File: \"" + filePath + "\"");

        if (filePath.empty())
        {
            ctx.LogError("[ExecuteBlueprint] File path is empty");
            ctx.SetOutputValue("Success", Variant(false));
            ctx.SetOutputValue("Output", Variant(std::string("Error: empty file path")));
            ctx.ActivateOutputFlow("Done");
            return true;
        }

        // 自动补全扩展名：没有 .bjson 后缀时补全
        {
            auto hasExt = [](const std::string& s, const std::string& ext) {
                if (s.size() < ext.size()) return false;
                std::string tail = s.substr(s.size() - ext.size());
                for (auto& c : tail) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
                return tail == ext;
            };
            if (!hasExt(filePath, ".bjson"))
                filePath += ".bjson";
        }

        // 如果路径是相对路径，基于 basePath（目录路径）解析
        std::string resolvedPath = filePath;
        if (!basePath.empty() && 
            filePath.find(':') == std::string::npos && 
            filePath[0] != '/' && filePath[0] != '\\')
        {
            char last = basePath.back();
            if (last == '/' || last == '\\')
                resolvedPath = basePath + filePath;
            else
                resolvedPath = basePath + '/' + filePath;
        }

        ctx.Log("  [ExecuteBlueprint] Resolved: \"" + resolvedPath + "\"");

        // 加载子蓝图
        JsonBlueprintExporter exporter(runner.GetFileSystem());
        auto importResult = exporter.importRuntimeFromFile(resolvedPath);

        if (!importResult.success)
        {
            ctx.LogError("[ExecuteBlueprint] " + importResult.errorMessage);
            ctx.SetOutputValue("Success", Variant(false));
            ctx.SetOutputValue("Output", Variant(std::string("Load error: ") + importResult.errorMessage));
            ctx.ActivateOutputFlow("Done");
            return true;
        }

        ctx.Log("  [ExecuteBlueprint] Loaded " + std::to_string(importResult.data.nodes.size()) + " nodes");

        // 共享 handler 表
        auto currentHandlers = std::make_shared<std::unordered_map<std::string, NodeHandler>>(
            runner.GetHandlers());

        // 异步执行
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

        ExecutionContext* pCtx = &ctx;
        auto alive = runner.GetAliveFlag();
        ctx.Delay(0.0f, [pCtx, &runner, sharedData, currentHandlers, completedPinId, resolvedPath, alive]() {
            if (!alive->load(std::memory_order_acquire)) return;
            pCtx->Log("  [ExecuteBlueprint] Async: executing \"" + resolvedPath + "\"...");

            auto subRunner = std::make_shared<BlueprintRunner>(runner.GetFileSystem());
            subRunner->SetParentTimerManager(runner.GetTimerManagerPtr());

            auto subLog = std::make_shared<std::vector<std::string>>();
            subRunner->SetLogCallback([pCtx](LogLevel lv, const std::string& msg) {
                pCtx->Print("    | " + msg, lv);
            });
            subRunner->SetPrintCallback([subLog, pCtx](LogLevel lv, const std::string& msg) {
                subLog->push_back(msg);
                pCtx->Print("    | " + msg, lv);
                std::string outputText;
                for (const auto& line : *subLog)
                {
                    if (!outputText.empty()) outputText += "\n";
                    outputText += line;
                }
                pCtx->SetOutputValue("Output", Variant(outputText));
            });

            if (!subRunner->Load(*sharedData))
            {
                pCtx->LogError("[ExecuteBlueprint:Async] Failed to load sub-blueprint");
                pCtx->SetOutputValue("Success", Variant(false));
                pCtx->SetOutputValue("Output", Variant(std::string("Async load failed")));
                if (completedPinId != 0)
                    pCtx->ActivateOutputFlow(completedPinId);
                return;
            }

            subRunner->SetDefaultHandler([](ExecutionContext& subCtx) {
                subCtx.Log("  [Default Handler] pass-through");
                return true;
            });
            subRunner->RegisterHandlers(*currentHandlers);

            // 将父 runner 的外部函数库（m_externalFunctions）透传给子 runner，
            // 确保子蓝图中引用的 FuncLib.* 节点能找到对应的函数定义。
            subRunner->RegisterExternalFunctions(runner.GetExternalFunctions());
            // 同时透传完整 Library 数据（shared_ptr 共享，零拷贝），供 FuncLib.* 节点正确执行
            subRunner->InheritExternalLibraries(runner.GetExternalLibraries());

            subRunner->Execute();
            auto execResult = subRunner->DispatchEvent("OnBeginPlay");

            std::string outputText;
            for (const auto& line : *subLog)
            {
                if (!outputText.empty()) outputText += "\n";
                outputText += line;
            }

            pCtx->Print("  [ExecuteBlueprint] Async result: " +
                std::string(execResult.success ? "SUCCESS" : "FAILED") +
                " (" + std::to_string(execResult.nodesExecuted) + " nodes executed)", LogLevel::Verbose);

            pCtx->SetOutputValue("Success", Variant(execResult.success));
            pCtx->SetOutputValue("Output", Variant(outputText));

            runner.KeepAlive(subRunner);

            if (completedPinId != 0)
                pCtx->ActivateOutputFlow(completedPinId);
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
        // Re-evaluate Condition each iteration: re-execute upstream data nodes
        // so that nodes like "Less", "Greater" etc. are recalculated each loop cycle.
        while (ctx.EvaluateConditionPin("Condition"))
        {
            if (++iterations > maxIterations)
            {
                ctx.LogWarning("[WhileLoop] Max iterations reached (" + std::to_string(maxIterations) + "), breaking");
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
    handlers["Delay"] = [&runner](ExecutionContext& ctx) {
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

        ExecutionContext* pCtx = &ctx;
        // 捕获 alive 标志，在回调时检查 runner 是否仍存活
        auto alive = runner.GetAliveFlag();
        auto timerHandle = ctx.Delay(duration, [pCtx, completedPinId, alive]() {
            // 检查 runner 是否已析构
            if (!alive->load(std::memory_order_acquire)) return;
            auto finishTime = FrameTimerManager::GetCurrentUnixTime();
            pCtx->Log("  [Delay] Completed; finished:" + std::to_string(finishTime));
            if (completedPinId != 0)
                pCtx->ActivateOutputFlow(completedPinId);
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

    // ── SwitchOnBool ───────────────────────────────────────────────────────
    handlers["SwitchOnBool"] = [](ExecutionContext& ctx) {
        bool cond = ctx.GetInputValue("Condition").asBool();
        ctx.ActivateOutputFlow(cond ? "True" : "False");
        return true;
    };

    // ── SwitchOnString ─────────────────────────────────────────────────────
    handlers["SwitchOnString"] = [](ExecutionContext& ctx) {
        std::string sel = ctx.GetInputValue("Selection").asString();
        // 动态扫描所有 "Case N" 输入引脚，找到匹配的激活对应输出
        const auto* node = ctx.GetCurrentNode();
        if (node)
        {
            for (const auto& pin : node->pins)
            {
                if (pin.kind == PinKind::Input && !pin.isExec &&
                    pin.name.rfind("Case ", 0) == 0)
                {
                    std::string caseVal = ctx.GetInputValue(pin.id).asString();
                    if (sel == caseVal)
                    {
                        ctx.ActivateOutputFlow(pin.name.c_str());
                        return true;
                    }
                }
            }
        }
        ctx.ActivateOutputFlow("Default");
        return true;
    };
}

} // namespace Runtime
} // namespace NodeEditor
