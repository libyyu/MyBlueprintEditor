// Handlers_Flow.cpp -- Flow Control 节点处理器注册
#include "../BlueprintEditor.h"
#include <fstream>
#include <sstream>

void BlueprintEditor::RegisterHandlers_Flow()
{
    m_HandlerRegistry["Branch"] = [](RTContext& ctx) {
        bool condition = ctx.GetInputValue("Condition").asBool();
        ctx.Log("  Condition = " + std::string(condition ? "True" : "False"));
        if (condition)
            ctx.ActivateOutputFlow("True");
        else
            ctx.ActivateOutputFlow("False");
        return true;
    };

    m_HandlerRegistry["DoN"] = [](RTContext& ctx) {
        int64_t n = ctx.GetInputValue("N").asInt();
        ctx.Log("  N = " + std::to_string(n));
        ctx.SetOutputValue("Counter", RTVariant(n));
        return true;
    };

    // ==================================================================
    // Execute Blueprint — 加载并执行另一个蓝图文件
    // ==================================================================
    m_HandlerRegistry["ExecuteBlueprint"] = [this](RTContext& ctx) {
        auto filePath = ctx.GetInputValue("File").asString();
        ctx.Log("  [ExecuteBlueprint] File: \"" + filePath + "\"");

        if (filePath.empty())
        {
            ctx.Log("  [ExecuteBlueprint] ERROR: File path is empty");
            ctx.SetOutputValue("Success", RTVariant(false));
            ctx.SetOutputValue("Output", RTVariant(std::string("Error: empty file path")));
            ctx.ActivateOutputFlow("Done");
            return true;
        }

        // 如果路径是相对路径，基于当前文件路径解析
        std::string resolvedPath = filePath;
        if (!m_CurrentFilePath.empty() && 
            filePath.find(':') == std::string::npos && 
            filePath[0] != '/' && filePath[0] != '\\')
        {
            size_t lastSlash = m_CurrentFilePath.find_last_of("/\\");
            if (lastSlash != std::string::npos)
                resolvedPath = m_CurrentFilePath.substr(0, lastSlash + 1) + filePath;
        }

        ctx.Log("  [ExecuteBlueprint] Resolved: \"" + resolvedPath + "\"");

        // 加载子蓝图
        ::NodeEditor::Runtime::JsonBlueprintExporter exporter;
        auto importResult = exporter.importRuntimeFromFile(resolvedPath);

        if (!importResult.success)
        {
            ctx.Log("  [ExecuteBlueprint] ERROR: " + importResult.errorMessage);
            ctx.SetOutputValue("Success", RTVariant(false));
            ctx.SetOutputValue("Output", RTVariant(std::string("Load error: ") + importResult.errorMessage));
            ctx.ActivateOutputFlow("Done");
            return true;
        }

        ctx.Log("  [ExecuteBlueprint] Loaded " + std::to_string(importResult.data.nodes.size()) + " nodes");

        // 创建子 Runner 执行
        RTBlueprintRunner subRunner;
        
        // 收集子蓝图的执行日志
        std::vector<std::string> subLog;
        subRunner.SetLogCallback([&subLog, &ctx](const std::string& msg) {
            subLog.push_back(msg);
            ctx.Log("    | " + msg);
        });
        subRunner.SetWaitTimeHandler([&ctx](float seconds, const std::function<void()>& run) {
            ctx.Delay(seconds, run);
		});

        if (!subRunner.Load(importResult.data))
        {
            ctx.Log("  [ExecuteBlueprint] ERROR: Failed to load sub-blueprint");
            ctx.SetOutputValue("Success", RTVariant(false));
            ctx.SetOutputValue("Output", RTVariant(std::string("Load failed")));
            ctx.ActivateOutputFlow("Done");
            return true;
        }

        // 注册处理器（复用当前编辑器的所有处理器）
        if (m_DefaultHandler)
            subRunner.SetDefaultHandler(m_DefaultHandler);
        subRunner.RegisterHandlers(m_HandlerRegistry);

        // 执行
        auto execResult = subRunner.Execute();

        ctx.Log("  [ExecuteBlueprint] Result: " + std::string(execResult.success ? "SUCCESS" : "FAILED") +
                " (" + std::to_string(execResult.nodesExecuted) + " nodes executed)");

        ctx.SetOutputValue("Success", RTVariant(execResult.success));

        // 合并子日志作为输出
        std::string outputText;
        for (const auto& line : subLog)
        {
            if (!outputText.empty()) outputText += "\n";
            outputText += line;
        }
        ctx.SetOutputValue("Output", RTVariant(outputText));

        ctx.ActivateOutputFlow("Done");
        return true;
    };

    m_HandlerRegistry["ForLoop"] = [](RTContext& ctx) {
        int64_t first = ctx.GetInputValue("First Index").asInt();
        int64_t last = ctx.GetInputValue("Last Index").asInt();
        ctx.Log("  [ForLoop] " + std::to_string(first) + " to " + std::to_string(last));
        for (int64_t i = first; i <= last; ++i)
        {
            ctx.SetOutputValue("Index", RTVariant(i));
            if (!ctx.ActivateOutputFlow("Loop Body"))
                return false;
        }
        ctx.ActivateOutputFlow("Completed");
        return true;
    };

    m_HandlerRegistry["WhileLoop"] = [](RTContext& ctx) {
        ctx.Log("  [WhileLoop] Starting");
        int iterations = 0;
        const int maxIterations = 10000; // 安全上限
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

    m_HandlerRegistry["Delay"] = [this](RTContext& ctx) {
        double dur = ctx.GetInputValue("Duration").asFloat();
        float duration = static_cast<float>(dur);
        ctx.Log("  [Delay] " + std::to_string(duration) + "s; started:" + std::to_string(time_t()));
        ctx.ActivateOutputFlow("Exec");
        // 使用主线程计时器代替阻塞式 sleep
        m_TimerManager.SetTimerByName("Delay", duration, 1, [this, &ctx]() {
            m_ExecutionLog.push_back("[Delay] Completed; finished:" + std::to_string(time_t()));
            m_ExecutionLogDirty = true;
            ctx.ActivateOutputFlow("Completed");
            return false;
        });

        return true;
    };

    m_HandlerRegistry["FlipFlop"] = [](RTContext& ctx) {
        bool isA = !ctx.GetVariable("__flipflop_state").asBool();
        ctx.SetVariable("__flipflop_state", RTVariant(isA));
        ctx.SetOutputValue("Is A", RTVariant(isA));
        ctx.Log("  [FlipFlop] -> " + std::string(isA ? "A" : "B"));
        if (isA)
            ctx.ActivateOutputFlow("A");
        else
            ctx.ActivateOutputFlow("B");
        return true;
    };

    m_HandlerRegistry["Gate"] = [](RTContext& ctx) {
        ctx.Log("  [Gate] pass through");
        return true;
    };
}
