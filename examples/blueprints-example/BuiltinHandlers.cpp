// BuiltinHandlers.cpp -- 内置节点运行时处理器注册
#include "BlueprintEditor.h"
#include <cstdlib>
#include <fstream>
#include <sstream>

void BlueprintEditor::RegisterBuiltinHandlers()
{
    // Default handler: pass-through
    m_DefaultHandler = [this](RTContext& ctx) {
        auto node = ctx.GetCurrentNode();
        if (!node) return true;
        ctx.Log("  [Default Handler] pass-through");
        return true;
    };

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

    m_HandlerRegistry["PrintString"] = [](RTContext& ctx) {
        auto str = ctx.GetInputValue("In String").asString();
        ctx.Log("  >>> Print: \"" + str + "\"");
        return true;
    };

    m_HandlerRegistry["SetTimer"] = [](RTContext& ctx) {
        double time = ctx.GetInputValue("Time").asFloat();
        bool looping = ctx.GetInputValue("Looping").asBool();
        ctx.Log("  Timer: " + std::to_string(time) + "s, Looping=" + (looping ? "true" : "false"));
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

    // Comparison operators
    m_HandlerRegistry["Less"] = [](RTContext& ctx) {
        auto node = ctx.GetCurrentNode();
        if (node && node->pins.size() >= 3)
        {
            double a = ctx.GetInputValue(node->pins[0].id).asFloat();
            double b = ctx.GetInputValue(node->pins[1].id).asFloat();
            double result = (a < b) ? 1.0 : 0.0;
            ctx.SetOutputValue(node->pins[2].id, RTVariant(result));
            ctx.Log("  " + std::to_string(a) + " < " + std::to_string(b) + " = " + std::to_string(result));
        }
        return true;
    };

    m_HandlerRegistry["Greater"] = [](RTContext& ctx) {
        auto node = ctx.GetCurrentNode();
        if (node && node->pins.size() >= 3)
        {
            double a = ctx.GetInputValue(node->pins[0].id).asFloat();
            double b = ctx.GetInputValue(node->pins[1].id).asFloat();
            double result = (a > b) ? 1.0 : 0.0;
            ctx.SetOutputValue(node->pins[2].id, RTVariant(result));
        }
        return true;
    };

    m_HandlerRegistry["Weird"] = [](RTContext& ctx) {
        auto node = ctx.GetCurrentNode();
        if (node && node->pins.size() >= 3)
        {
            double input = ctx.GetInputValue(node->pins[0].id).asFloat();
            ctx.SetOutputValue(node->pins[1].id, RTVariant(input));
            ctx.SetOutputValue(node->pins[2].id, RTVariant(input));
        }
        return true;
    };

    // Math operators
    m_HandlerRegistry["Add"] = [](RTContext& ctx) {
        double a = ctx.GetInputValue("A").asFloat();
        double b = ctx.GetInputValue("B").asFloat();
        ctx.SetOutputValue("Result", RTVariant(a + b));
        return true;
    };

    m_HandlerRegistry["Subtract"] = [](RTContext& ctx) {
        double a = ctx.GetInputValue("A").asFloat();
        double b = ctx.GetInputValue("B").asFloat();
        ctx.SetOutputValue("Result", RTVariant(a - b));
        return true;
    };

    m_HandlerRegistry["Multiply"] = [](RTContext& ctx) {
        double a = ctx.GetInputValue("A").asFloat();
        double b = ctx.GetInputValue("B").asFloat();
        ctx.SetOutputValue("Result", RTVariant(a * b));
        return true;
    };

    m_HandlerRegistry["Divide"] = [](RTContext& ctx) {
        double a = ctx.GetInputValue("A").asFloat();
        double b = ctx.GetInputValue("B").asFloat();
        if (b == 0.0) { ctx.Log("  [WARN] Division by zero!"); b = 1.0; }
        ctx.SetOutputValue("Result", RTVariant(a / b));
        return true;
    };

    m_HandlerRegistry["Equal"] = [](RTContext& ctx) {
        double a = ctx.GetInputValue("A").asFloat();
        double b = ctx.GetInputValue("B").asFloat();
        ctx.SetOutputValue("Result", RTVariant(a == b));
        return true;
    };

    // Logic operators
    m_HandlerRegistry["And"] = [](RTContext& ctx) {
        bool a = ctx.GetInputValue("A").asBool();
        bool b = ctx.GetInputValue("B").asBool();
        ctx.SetOutputValue("Result", RTVariant(a && b));
        return true;
    };

    m_HandlerRegistry["Or"] = [](RTContext& ctx) {
        bool a = ctx.GetInputValue("A").asBool();
        bool b = ctx.GetInputValue("B").asBool();
        ctx.SetOutputValue("Result", RTVariant(a || b));
        return true;
    };

    m_HandlerRegistry["Not"] = [](RTContext& ctx) {
        bool v = ctx.GetInputValue("Value").asBool();
        ctx.SetOutputValue("Result", RTVariant(!v));
        return true;
    };

    // Type conversion
    m_HandlerRegistry["IntToFloat"] = [](RTContext& ctx) {
        int64_t v = ctx.GetInputValue("Value").asInt();
        ctx.SetOutputValue("Result", RTVariant(static_cast<double>(v)));
        return true;
    };

    m_HandlerRegistry["FloatToInt"] = [](RTContext& ctx) {
        double v = ctx.GetInputValue("Value").asFloat();
        ctx.SetOutputValue("Result", RTVariant(static_cast<int64_t>(v)));
        return true;
    };

    m_HandlerRegistry["FloatToString"] = [](RTContext& ctx) {
        double v = ctx.GetInputValue("Value").asFloat();
        ctx.SetOutputValue("Result", RTVariant(std::to_string(v)));
        return true;
    };

    // Math functions
    m_HandlerRegistry["Abs"] = [](RTContext& ctx) {
        double v = ctx.GetInputValue("Value").asFloat();
        ctx.SetOutputValue("Result", RTVariant(v < 0 ? -v : v));
        return true;
    };

    m_HandlerRegistry["Clamp"] = [](RTContext& ctx) {
        double v = ctx.GetInputValue("Value").asFloat();
        double lo = ctx.GetInputValue("Min").asFloat();
        double hi = ctx.GetInputValue("Max").asFloat();
        if (v < lo) v = lo;
        if (v > hi) v = hi;
        ctx.SetOutputValue("Result", RTVariant(v));
        return true;
    };

    m_HandlerRegistry["Min"] = [](RTContext& ctx) {
        double a = ctx.GetInputValue("A").asFloat();
        double b = ctx.GetInputValue("B").asFloat();
        ctx.SetOutputValue("Result", RTVariant(a < b ? a : b));
        return true;
    };

    m_HandlerRegistry["Max"] = [](RTContext& ctx) {
        double a = ctx.GetInputValue("A").asFloat();
        double b = ctx.GetInputValue("B").asFloat();
        ctx.SetOutputValue("Result", RTVariant(a > b ? a : b));
        return true;
    };

    // String operations
    m_HandlerRegistry["AppendString"] = [](RTContext& ctx) {
        auto a = ctx.GetInputValue("A").asString();
        auto b = ctx.GetInputValue("B").asString();
        ctx.SetOutputValue("Result", RTVariant(a + b));
        return true;
    };

    m_HandlerRegistry["StringLength"] = [](RTContext& ctx) {
        auto s = ctx.GetInputValue("String").asString();
        ctx.SetOutputValue("Length", RTVariant(static_cast<int64_t>(s.size())));
        return true;
    };

    m_HandlerRegistry["MakeString"] = [](RTContext& ctx) {
        auto v = ctx.GetInputValue("Value").asString();
        ctx.SetOutputValue("String", RTVariant(v));
        return true;
    };

    m_HandlerRegistry["Log"] = [](RTContext& ctx) {
        auto msg = ctx.GetInputValue("Message").asString();
        ctx.Log("  [LOG] " + msg);
        return true;
    };

    // Variable access
    m_HandlerRegistry["GetVariable"] = [](RTContext& ctx) {
        auto name = ctx.GetInputValue("Name").asString();
        ctx.SetOutputValue("Value", ctx.GetVariable(name));
        return true;
    };

    m_HandlerRegistry["SetVariable"] = [](RTContext& ctx) {
        auto name = ctx.GetInputValue("Name").asString();
        auto val = ctx.GetInputValue("Value");
        ctx.SetVariable(name, val);
        ctx.Log("  Set '" + name + "' = '" + val.asString() + "'");
        return true;
    };

    // Flow control
    m_HandlerRegistry["Sequence"] = [](RTContext& ctx) {
        ctx.Log("  [Sequence] executing all outputs");
        return true;
    };

    m_HandlerRegistry["MoveTo"] = [](RTContext& ctx) {
        ctx.Log("  [Task] Moving to target...");
        return true;
    };

    m_HandlerRegistry["RandomWait"] = [](RTContext& ctx) {
        ctx.Log("  [Task] Waiting random time...");
        return true;
    };

    // Houdini
    m_HandlerRegistry["HoudiniTransform"] = [](RTContext& ctx) {
        ctx.Log("  [Houdini] Transform applied");
        return true;
    };

    m_HandlerRegistry["HoudiniGroup"] = [](RTContext& ctx) {
        ctx.Log("  [Houdini] Group created");
        return true;
    };

    // Single Line Trace by Channel
    m_HandlerRegistry["TraceByChannel"] = [](RTContext& ctx) {
        ctx.Log("  [Trace] Line trace performed");
        ctx.SetOutputValue("Return Value", RTVariant(true));
        return true;
    };

    // For Loop
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

    // While Loop
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

    // Delay
    m_HandlerRegistry["Delay"] = [](RTContext& ctx) {
        double dur = ctx.GetInputValue("Duration").asFloat();
        ctx.Log("  [Delay] " + std::to_string(dur) + "s");
        return true;
    };

    // Flip Flop
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

    // Gate
    m_HandlerRegistry["Gate"] = [](RTContext& ctx) {
        ctx.Log("  [Gate] pass through");
        return true;
    };

    // Random Float
    m_HandlerRegistry["Random"] = [](RTContext& ctx) {
        double v = static_cast<double>(rand()) / RAND_MAX;
        ctx.SetOutputValue("Value", RTVariant(v));
        return true;
    };

    // Random In Range
    m_HandlerRegistry["RandomInRange"] = [](RTContext& ctx) {
        double lo = ctx.GetInputValue("Min").asFloat();
        double hi = ctx.GetInputValue("B").asFloat();
        double v = lo + (static_cast<double>(rand()) / RAND_MAX) * (hi - lo);
        ctx.SetOutputValue("Value", RTVariant(v));
        return true;
    };
}
