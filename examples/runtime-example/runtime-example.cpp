// runtime-example.cpp
// 蓝图运行时执行器 — 支持命令行传入蓝图文件执行
//
// 用法:
//   runtime-example.exe                           — 运行内置示例蓝图
//   runtime-example.exe <file.json>               — 加载并执行指定蓝图文件
//   runtime-example.exe <file.editor.json>         — 加载并执行指定蓝图文件
//   runtime-example.exe <file> --max-time <secs>   — 设置最大等待异步 timer 的时间（默认 30 秒）
//   runtime-example.exe <file> --tick-rate <ms>     — 设置帧循环 tick 间隔（默认 16ms ≈ 60fps）
//
// 设计理念:
//   - Runtime 文件 (.json):          只包含执行所需的最小数据集
//   - Editor 附加文件 (.editor.json): 只包含编辑器独有数据（位置、尺寸、注释、视图等）
//   - Editor 完整数据 = Runtime 文件 + Editor 附加文件
//   - 运行时只需加载 Runtime 文件即可执行

#include <iostream>
#include <fstream>
#include <string>
#include <cmath>
#include <filesystem>

// 只需要 Runtime 模块，不需要 ImGui
#include "BlueprintRunner.h"
#include "BlueprintExporter.h"
#include "BuiltinHandlers.h"

using namespace NodeEditor::Runtime;

// ============================================================================
// 示例: 手动构建一个蓝图数据，模拟编辑器导出的结果
// ============================================================================

static BlueprintData createSampleBlueprint()
{
    BlueprintData bp;

    // 元数据
    bp.metadata.name        = "MathPipeline";
    bp.metadata.description = "A simple math computation pipeline";
    bp.metadata.version     = "1.0";
    bp.metadata.author      = "Runtime Example";
    bp.metadata.createdAt   = "2026-03-14T10:00:00Z";
    bp.metadata.updatedAt   = "2026-03-14T12:30:00Z";

    // --- 节点 1: 输入常量 A (值 = 10) ---
    {
        NodeInstance node;
        node.id           = 1;
        node.definitionId = "constant_float";
        node.name         = "Constant A";
        node.position     = NodePosition(100, 200);
        node.size         = NodeSize(150, 80);

        PinInfo outPin;
        outPin.id       = 101;
        outPin.name     = "Value";
        outPin.kind     = PinKind::Output;
        outPin.dataType = PinDataType::Float;
        node.pins.push_back(outPin);

        // 节点配置数据
        node.nodeData["value"] = Variant(10.0);

        bp.nodes.push_back(std::move(node));
    }

    // --- 节点 2: 输入常量 B (值 = 3) ---
    {
        NodeInstance node;
        node.id           = 2;
        node.definitionId = "constant_float";
        node.name         = "Constant B";
        node.position     = NodePosition(100, 350);
        node.size         = NodeSize(150, 80);

        PinInfo outPin;
        outPin.id       = 201;
        outPin.name     = "Value";
        outPin.kind     = PinKind::Output;
        outPin.dataType = PinDataType::Float;
        node.pins.push_back(outPin);

        node.nodeData["value"] = Variant(3.0);

        bp.nodes.push_back(std::move(node));
    }

    // --- 节点 3: 加法器 (A + B) ---
    {
        NodeInstance node;
        node.id           = 3;
        node.definitionId = "math_add";
        node.name         = "Add";
        node.position     = NodePosition(350, 275);
        node.size         = NodeSize(120, 100);

        PinInfo inA;
        inA.id       = 301;
        inA.name     = "A";
        inA.kind     = PinKind::Input;
        inA.dataType = PinDataType::Float;
        node.pins.push_back(inA);

        PinInfo inB;
        inB.id       = 302;
        inB.name     = "B";
        inB.kind     = PinKind::Input;
        inB.dataType = PinDataType::Float;
        node.pins.push_back(inB);

        PinInfo out;
        out.id       = 303;
        out.name     = "Result";
        out.kind     = PinKind::Output;
        out.dataType = PinDataType::Float;
        node.pins.push_back(out);

        bp.nodes.push_back(std::move(node));
    }

    // --- 节点 4: 输出显示 ---
    {
        NodeInstance node;
        node.id           = 4;
        node.definitionId = "output_display";
        node.name         = "Display Result";
        node.position     = NodePosition(550, 275);
        node.size         = NodeSize(160, 80);

        PinInfo inVal;
        inVal.id       = 401;
        inVal.name     = "Input";
        inVal.kind     = PinKind::Input;
        inVal.dataType = PinDataType::Float;
        node.pins.push_back(inVal);

        bp.nodes.push_back(std::move(node));
    }

    // --- 链接 ---
    {
        LinkInstance link;
        link.id         = 1001;
        link.startPinId = 101;
        link.endPinId   = 301;
        bp.links.push_back(link);
    }
    {
        LinkInstance link;
        link.id         = 1002;
        link.startPinId = 201;
        link.endPinId   = 302;
        bp.links.push_back(link);
    }
    {
        LinkInstance link;
        link.id         = 1003;
        link.startPinId = 303;
        link.endPinId   = 401;
        bp.links.push_back(link);
    }

    // 蓝图变量
    VariableDefinition speedVar;
    speedVar.name         = "Speed";
    speedVar.dataType     = PinDataType::Float;
    speedVar.defaultValue = Variant(1.5);
    speedVar.category     = "Physics";
    speedVar.tooltip      = "Movement speed multiplier";
    bp.variables.push_back(speedVar);

    // 注释区域
    CommentRegion comment;
    comment.id    = "comment_1";
    comment.text  = "Math Pipeline - Input Section";
    comment.position = NodePosition(50, 150);
    comment.size     = NodeSize(250, 300);
    comment.color    = "#4488FF";
    comment.alpha    = 0.3f;
    bp.comments.push_back(comment);

    // 视图信息
    bp.viewInfo.viewPosition = NodePosition(0, 0);
    bp.viewInfo.viewScale    = 1.0f;

    return bp;
}

// ============================================================================
// 节点处理器实现
// ============================================================================

bool handler_ConstantFloat(ExecutionContext& ctx)
{
    Variant value = ctx.GetNodeData("value");
    ctx.SetOutputValue("Value", value);
    ctx.Log("  -> Output: " + std::to_string(value.asFloat()));
    return true;
}

bool handler_MathAdd(ExecutionContext& ctx)
{
    double a = ctx.GetInputValue("A").asFloat();
    double b = ctx.GetInputValue("B").asFloat();
    double result = a + b;

    ctx.SetOutputValue("Result", Variant(result));
    ctx.Log("  -> " + std::to_string(a) + " + " + std::to_string(b) + " = " + std::to_string(result));
    return true;
}

bool handler_OutputDisplay(ExecutionContext& ctx)
{
    Variant input = ctx.GetInputValue("Input");
    std::cout << "  [DISPLAY] Final result = " << input.asFloat() << std::endl;
    return true;
}

// ============================================================================
// 命令行模式: 加载并执行指定蓝图文件
// ============================================================================

static int runBlueprintFromFile(const std::string& filePath, float maxTimeSec, int tickRateMs)
{
    namespace fs = std::filesystem;

    // 检查文件是否存在
    if (!fs::exists(filePath))
    {
        std::cerr << "ERROR: File not found: " << filePath << std::endl;
        return 1;
    }

    // 获取文件所在目录作为 basePath（用于解析 ExecuteBlueprint 等节点的相对路径）
    std::string basePath = fs::path(filePath).parent_path().string();
    std::string fileName = fs::path(filePath).filename().string();

    std::cout << "========================================" << std::endl;
    std::cout << "  Blueprint Runtime Executor" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << "File:      " << filePath << std::endl;
    std::cout << "BasePath:  " << basePath << std::endl;
    std::cout << "MaxTime:   " << maxTimeSec << "s" << std::endl;
    std::cout << "TickRate:  " << tickRateMs << "ms" << std::endl;
    std::cout << std::endl;

    // 加载蓝图
    // 支持两种格式：
    //   1. .editor.json — 编辑器导出的完整文件（通过 importEditorFromFile 加载合并）
    //   2. .json        — 纯运行时文件（直接 LoadFromFile）
    BlueprintRunner runner;
    runner.SetLogCallback([](const std::string& msg) {
        std::cout << msg << std::endl;
    });

    bool isEditorFile = (fileName.size() > 12 && fileName.substr(fileName.size() - 12) == ".editor.json");

    if (isEditorFile)
    {
        // .editor.json 文件：通过 exporter 加载完整编辑器数据（包含内嵌的 runtime 数据）
        JsonBlueprintExporter exporter;
        auto importResult = exporter.importFromEditorFile(filePath);
        if (!importResult.success)
        {
            std::cerr << "ERROR: Failed to load editor file: " << importResult.errorMessage << std::endl;
            return 1;
        }

        if (!runner.Load(importResult.data))
        {
            std::cerr << "ERROR: Failed to load blueprint data: " << runner.GetLastError() << std::endl;
            return 1;
        }
    }
    else
    {
        // 普通 .json 文件：直接加载
        if (!runner.LoadFromFile(filePath))
        {
            std::cerr << "ERROR: " << runner.GetLastError() << std::endl;
            return 1;
        }
    }

    const auto& bp = runner.GetBlueprintData();
    std::cout << "Blueprint: " << (bp.metadata.name.empty() ? "(unnamed)" : bp.metadata.name) << std::endl;
    std::cout << "Nodes:     " << bp.nodes.size() << std::endl;
    std::cout << "Links:     " << bp.links.size() << std::endl;
    std::cout << std::endl;

    // 注册所有内置处理器
    RegisterBuiltinHandlers(runner, basePath);

    // 设置默认处理器（未注册的节点类型会走 pass-through）
    runner.SetDefaultHandler([](ExecutionContext& ctx) {
        auto node = ctx.GetCurrentNode();
        std::string nodeName = node ? node->name : "(unknown)";
        ctx.Log("  [Default Handler] pass-through for: " + nodeName);
        return true;
    });

    // 同步执行蓝图
    std::cout << "========================================" << std::endl;
    std::cout << "  Execution Started" << std::endl;
    std::cout << "========================================" << std::endl;

    auto startTime = std::chrono::high_resolution_clock::now();
    auto result = runner.Execute();
    auto endTime = std::chrono::high_resolution_clock::now();
    double elapsed = std::chrono::duration<double, std::milli>(endTime - startTime).count();

    std::cout << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << "  Execution " << (result.success ? "Completed Successfully!" : "FAILED!") << std::endl;
    std::cout << "  Nodes executed: " << result.nodesExecuted << std::endl;
    std::cout << "  Elapsed: " << elapsed << " ms" << std::endl;
    std::cout << "========================================" << std::endl;

    // 帧循环：驱动异步 timer（Delay、SetTimer 等）
    // 持续 tick 直到没有活跃 timer 或超过最大等待时间
    if (runner.GetTimerManager().GetActiveTimerCount() > 0)
    {
        std::cout << std::endl;
        std::cout << "[Timer Loop] Active timers: " << runner.GetTimerManager().GetActiveTimerCount()
                  << ", entering frame loop..." << std::endl;

        auto loopStart = std::chrono::high_resolution_clock::now();
        auto lastTick  = loopStart;

        while (runner.GetTimerManager().GetActiveTimerCount() > 0)
        {
            auto now = std::chrono::high_resolution_clock::now();

            // 检查超时
            double totalElapsed = std::chrono::duration<double>(now - loopStart).count();
            if (totalElapsed > maxTimeSec)
            {
                std::cout << "[Timer Loop] Max time (" << maxTimeSec << "s) exceeded, stopping." << std::endl;
                break;
            }

            // 计算 deltaTime
            float deltaTime = std::chrono::duration<float>(now - lastTick).count();
            lastTick = now;

            // Tick timer manager
            runner.Tick(deltaTime);

            // Sleep 到下一帧
            std::this_thread::sleep_for(std::chrono::milliseconds(tickRateMs));
        }

        auto loopEnd = std::chrono::high_resolution_clock::now();
        double loopElapsed = std::chrono::duration<double, std::milli>(loopEnd - loopStart).count();

        std::cout << "[Timer Loop] Finished. Loop time: " << loopElapsed << " ms" << std::endl;
        std::cout << "  Remaining active timers: " << runner.GetTimerManager().GetActiveTimerCount() << std::endl;
    }

    std::cout << std::endl;
    std::cout << "=== Done ===" << std::endl;
    return result.success ? 0 : 1;
}

// ============================================================================
// main
// ============================================================================

int main(int argc, char* argv[])
{
    // ----------------------------------------------------------------
    // 命令行模式：传入蓝图文件路径
    // ----------------------------------------------------------------
    if (argc >= 2)
    {
        std::string firstArg = argv[1];

        // --help 可以出现在任意位置
        if (firstArg == "--help" || firstArg == "-h")
        {
            std::cout << "Usage: runtime-example [<blueprint-file>] [options]" << std::endl;
            std::cout << "  <blueprint-file>         Path to .json or .editor.json blueprint file" << std::endl;
            std::cout << "  --max-time <seconds>     Max time to wait for async timers (default: 30)" << std::endl;
            std::cout << "  --tick-rate <ms>         Frame tick interval in ms (default: 16)" << std::endl;
            std::cout << std::endl;
            std::cout << "If no file is provided, runs the built-in sample blueprint." << std::endl;
            return 0;
        }

        std::string filePath = firstArg;
        float  maxTimeSec = 30.0f;  // 默认最大等待异步 timer 30 秒
        int    tickRateMs = 16;     // 默认 ~60fps

        // 解析可选参数
        for (int i = 2; i < argc; ++i)
        {
            std::string arg = argv[i];
            if (arg == "--max-time" && i + 1 < argc)
            {
                maxTimeSec = std::stof(argv[++i]);
            }
            else if (arg == "--tick-rate" && i + 1 < argc)
            {
                tickRateMs = std::stoi(argv[++i]);
            }
            else
            {
                std::cerr << "Unknown option: " << arg << std::endl;
                std::cerr << "Use --help for usage information." << std::endl;
                return 1;
            }
        }

        return runBlueprintFromFile(filePath, maxTimeSec, tickRateMs);
    }

    // ----------------------------------------------------------------
    // 无参数模式：运行内置示例蓝图
    // ----------------------------------------------------------------
    std::cout << "=== Blueprint Dual-File Export Example ===" << std::endl;
    std::cout << "(Use 'runtime-example <file>' to execute a blueprint file)" << std::endl;
    std::cout << std::endl;

    BlueprintData blueprint = createSampleBlueprint();
    JsonBlueprintExporter exporter;
    ExportOptions opts;
    opts.prettyPrint = true;

    // ==================================================================
    // Step 1: 导出为两个文件 (Runtime + Editor)
    // ==================================================================
    std::cout << "--- Step 1: Export to dual files (Runtime + Editor) ---" << std::endl;

    auto exportResult = exporter.exportEditorFiles(
        blueprint,
        "blueprint.json",           // Runtime 数据
        "blueprint.editor.json",    // Editor 附加数据
        opts
    );

    if (!exportResult.success)
    {
        std::cerr << "Export failed: " << exportResult.errorMessage << std::endl;
        return 1;
    }

    std::cout << "Runtime file:  blueprint.json         (" << exportResult.runtimeBytes << " bytes)" << std::endl;
    std::cout << "Editor file:   blueprint.editor.json   (" << exportResult.editorBytes << " bytes)" << std::endl;
    std::cout << "Total:         " << exportResult.totalBytes() << " bytes" << std::endl;
    std::cout << std::endl;

    // 打印 Runtime 文件内容
    std::string runtimeJson = exporter.exportRuntimeToString(blueprint, opts);
    std::cout << "---- Runtime file content (blueprint.json) ----" << std::endl;
    std::cout << runtimeJson << std::endl;
    std::cout << std::endl;

    // 打印 Editor 附加文件内容
    std::string editorJson = exporter.exportEditorToString(blueprint, opts);
    std::cout << "---- Editor file content (blueprint.editor.json) ----" << std::endl;
    std::cout << editorJson << std::endl;
    std::cout << std::endl;

    // ==================================================================
    // Step 2: 运行时 — 只加载 Runtime 文件并执行
    // ==================================================================
    std::cout << "--- Step 2: Runtime — load only Runtime file and execute ---" << std::endl;

    BlueprintRunner runner;
    runner.SetLogCallback([](const std::string& msg) {
        std::cout << msg << std::endl;
    });

    // 运行时只需要 Runtime 文件
    if (!runner.LoadFromFile("blueprint.json"))
    {
        std::cerr << "ERROR: " << runner.GetLastError() << std::endl;
        return 1;
    }

    std::cout << "Blueprint loaded: " << runner.GetBlueprintData().metadata.name << std::endl;
    std::cout << "Nodes: " << runner.GetBlueprintData().nodes.size() << std::endl;
    std::cout << "Links: " << runner.GetBlueprintData().links.size() << std::endl;

    // 验证运行时数据不包含编辑器信息
    const auto* node1 = runner.GetBlueprintData().findNode(1);
    if (node1)
    {
        std::cout << "Node 'Constant A' position: (" << node1->position.x << ", " << node1->position.y << ")"
                  << " (should be 0,0 — not in Runtime file)" << std::endl;
    }
    std::cout << "Comments count: " << runner.GetBlueprintData().comments.size()
              << " (should be 0 — not in Runtime file)" << std::endl;
    std::cout << std::endl;

    // 注册处理器并执行
    runner.RegisterHandler("constant_float", handler_ConstantFloat);
    runner.RegisterHandler("math_add",       handler_MathAdd);
    runner.RegisterHandler("output_display", handler_OutputDisplay);

    std::cout << "Executing..." << std::endl;
    auto result = runner.Execute();

    std::cout << std::endl;
    std::cout << "Success: " << (result.success ? "Yes" : "No") << std::endl;
    std::cout << "Nodes executed: " << result.nodesExecuted << std::endl;
    std::cout << "Elapsed: " << result.elapsedMs << " ms" << std::endl;
    std::cout << std::endl;

    // ==================================================================
    // Step 3: 编辑器 — 加载两个文件，合并恢复完整数据
    // ==================================================================
    std::cout << "--- Step 3: Editor — load both files and merge ---" << std::endl;

    auto mergeResult = exporter.importEditorFromFiles("blueprint.json", "blueprint.editor.json");

    if (!mergeResult.success)
    {
        std::cerr << "Merge import failed: " << mergeResult.errorMessage << std::endl;
        return 1;
    }

    std::cout << "Merged data loaded OK!" << std::endl;
    std::cout << "  Name: " << mergeResult.data.metadata.name << std::endl;
    std::cout << "  Author: " << mergeResult.data.metadata.author << " (from Editor file)" << std::endl;
    std::cout << "  CreatedAt: " << mergeResult.data.metadata.createdAt << " (from Editor file)" << std::endl;
    std::cout << "  Nodes: " << mergeResult.data.nodes.size() << std::endl;

    // 验证位置信息已恢复
    const auto* mergedNode1 = mergeResult.data.findNode(1);
    if (mergedNode1)
    {
        std::cout << "  Node 'Constant A' position: (" << mergedNode1->position.x << ", " << mergedNode1->position.y << ")"
                  << " (restored from Editor file)" << std::endl;
        std::cout << "  Node 'Constant A' size: (" << mergedNode1->size.width << "x" << mergedNode1->size.height << ")"
                  << " (restored from Editor file)" << std::endl;
    }

    std::cout << "  Comments: " << mergeResult.data.comments.size() << " (restored from Editor file)" << std::endl;
    if (!mergeResult.data.comments.empty())
    {
        std::cout << "    Comment[0]: \"" << mergeResult.data.comments[0].text << "\"" << std::endl;
    }

    std::cout << "  View scale: " << mergeResult.data.viewInfo.viewScale << std::endl;

    // 验证变量附加信息已恢复
    if (!mergeResult.data.variables.empty())
    {
        const auto& var = mergeResult.data.variables[0];
        std::cout << "  Variable '" << var.name << "' category: \"" << var.category
                  << "\", tooltip: \"" << var.tooltip << "\" (from Editor file)" << std::endl;
    }

    std::cout << std::endl;
    std::cout << "  Total bytes read: " << mergeResult.bytesRead << std::endl;
    std::cout << std::endl;

    // ==================================================================
    // Step 4: 修改输入并重新执行
    // ==================================================================
    std::cout << "--- Step 4: Inject external values and re-execute ---" << std::endl;

    runner.ResetState();
    runner.SetPinValue(101, Variant(42.0));
    runner.SetPinValue(201, Variant(8.0));

    runner.RegisterHandler("constant_float", [](ExecutionContext& ctx) {
        ctx.Log("  -> (using pre-set pin value)");
        return true;
    });

    auto result2 = runner.Execute();
    std::cout << std::endl;
    std::cout << "Success: " << (result2.success ? "Yes" : "No") << std::endl;
    std::cout << "Expected: 42 + 8 = 50" << std::endl;

    // ==================================================================
    // Step 5: 蓝图变量
    // ==================================================================
    std::cout << std::endl;
    std::cout << "--- Step 5: Blueprint variables ---" << std::endl;
    auto speed = runner.GetVariable("Speed");
    std::cout << "Variable 'Speed' = " << speed.asFloat() << std::endl;

    runner.SetVariable("Speed", Variant(3.0));
    std::cout << "Variable 'Speed' (updated) = " << runner.GetVariable("Speed").asFloat() << std::endl;

    std::cout << std::endl;
    std::cout << "=== Done ===" << std::endl;

#ifdef _WIN32
    system("pause");
#endif

    return 0;
}
