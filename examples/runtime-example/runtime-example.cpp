// runtime-example.cpp
// 演示蓝图数据的双文件导出架构
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

// 只需要 Runtime 模块，不需要 ImGui
#include "BlueprintRunner.h"
#include "BlueprintExporter.h"

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
// main
// ============================================================================

int main()
{
    std::cout << "=== Blueprint Dual-File Export Example ===" << std::endl;
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
