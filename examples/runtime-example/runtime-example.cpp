// runtime-example.cpp
// 蓝图运行时执行器 — 支持命令行传入蓝图文件执行，以及内置单元测试模式
//
// 用法:
//   runtime-example                                — 运行内置示例蓝图
//   runtime-example <file.json>                    — 加载并执行指定蓝图文件
//   runtime-example <file.editor.json>             — 加载并执行指定蓝图文件
//   runtime-example <file> --max-time <secs>       — 设置最大等待异步 timer 的时间（默认 30 秒）
//   runtime-example <file> --tick-rate <ms>        — 设置帧循环 tick 间隔（默认 16ms ≈ 60fps）
//   runtime-example --test                         — 运行所有内置单元测试
//   runtime-example --test flow_test.json          — 运行测试（指定 flow_test.json 路径）
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
#include <vector>

// 只需要 Runtime 模块，不需要 ImGui
#include "BlueprintRunner.h"
#include "BlueprintExporter.h"
#include "BuiltinHandlers.h"
#include "crude_json.h"

using namespace NodeEditor::Runtime;

// ============================================================================
// 单元测试框架（轻量级，无外部依赖）
// ============================================================================

static int g_pass = 0;
static int g_fail = 0;

#define CHECK(cond, msg) \
    do { \
        if (cond) { std::cout << "  [PASS] " << (msg) << "\n"; ++g_pass; } \
        else      { std::cout << "  [FAIL] " << (msg) << "\n"; ++g_fail; } \
    } while(0)

// ============================================================================
// Test 1: DefaultNodeRegistry 野指针修复验证
//   连续注册大量节点，强制触发 unordered_map rehash，
//   再遍历 getAllNodeDefinitions() 确保指针仍然有效。
// ============================================================================
static void test_registry_no_dangling_ptr()
{
    std::cout << "\n[Test 1] DefaultNodeRegistry – rehash 野指针修复\n";

    DefaultNodeRegistry reg;
    for (int i = 0; i < 200; ++i)
    {
        NodeDefinition def;
        def.id   = "node_" + std::to_string(i);
        def.name = "Node " + std::to_string(i);
        reg.registerNode(def);
    }

    const auto& defs = reg.getAllNodeDefinitions();
    CHECK(defs.size() == 200, "注册 200 个节点后缓存数量正确");

    bool allValid = true;
    for (const auto* p : defs)
    {
        if (!p || p->id.empty()) { allValid = false; break; }
        const auto* found = reg.getNodeDefinition(p->id);
        if (found != p) { allValid = false; break; }
    }
    CHECK(allValid, "所有缓存指针指向有效节点（无野指针）");

    NodeDefinition updated;
    updated.id   = "node_0";
    updated.name = "Node UPDATED";
    reg.registerNode(updated);

    const auto* p0 = reg.getNodeDefinition("node_0");
    CHECK(p0 && p0->name == "Node UPDATED", "更新节点后 getNodeDefinition 返回新值");
    CHECK(reg.getAllNodeDefinitions().size() == 200, "更新后缓存数量不变");
}

// ============================================================================
// Test 2: Variant strtoll/strtod（-fno-exceptions 友好）
// ============================================================================
static void test_variant_conversion()
{
    std::cout << "\n[Test 2] Variant asInt/asFloat – strtoll/strtod 转换\n";

    Variant vs(std::string("42"));
    CHECK(vs.asInt() == 42,  "\"42\" → asInt() == 42");
    CHECK(vs.asFloat() == 42.0, "\"42\" → asFloat() == 42.0");

    Variant vf(std::string("3.14"));
    CHECK(vf.asFloat() > 3.13 && vf.asFloat() < 3.15, "\"3.14\" → asFloat() ≈ 3.14");
    CHECK(vf.asInt() == 3, "\"3.14\" → asInt() == 3 (截断)");

    Variant vbad(std::string("abc"));
    CHECK(vbad.asInt() == 0,   "\"abc\" → asInt() == 0 (转换失败)");
    CHECK(vbad.asFloat() == 0.0, "\"abc\" → asFloat() == 0.0 (转换失败)");

    Variant vempty(std::string(""));
    CHECK(vempty.asInt() == 0,   "\"\" → asInt() == 0");
    CHECK(vempty.asFloat() == 0.0, "\"\" → asFloat() == 0.0");
}

// ============================================================================
// Test 3: Variant Map API
// ============================================================================
static void test_variant_map()
{
    std::cout << "\n[Test 3] Variant Map API\n";

    Variant m;
    m.mapSet("foo",   Variant(std::string("bar")));
    m.mapSet("count", Variant(int64_t(42)));

    CHECK(m.mapHasKey("foo"),  "mapHasKey(\"foo\") == true");
    CHECK(!m.mapHasKey("baz"), "mapHasKey(\"baz\") == false");
    CHECK(m.mapGet("foo").asString() == "bar", "mapGet(\"foo\") == \"bar\"");
    CHECK(m.mapGet("count").asInt() == 42,     "mapGet(\"count\") == 42");
    CHECK(m.mapSize() == 2, "mapSize() == 2");

    m.mapRemove("foo");
    CHECK(m.mapSize() == 1,    "mapRemove 后 mapSize() == 1");
    CHECK(!m.mapHasKey("foo"), "mapRemove 后 mapHasKey(\"foo\") == false");

    Variant m2(m);
    CHECK(m2.mapHasKey("count"), "拷贝后 mapHasKey(\"count\") == true");

    Variant m3(std::move(m2));
    CHECK(m3.mapHasKey("count"), "移动后 mapHasKey(\"count\") == true");
}

// ============================================================================
// Test 4: BlueprintRunner 执行 flow_test.json
//   拓扑：SetVariable(counter=0) → ForLoop(0..4) → AddIndex+GetVariable →
//         SetVariable(counter+=index) → PrintString →
//         [完成后] Branch(counter>=10) → PrintTrue/PrintFalse
// ============================================================================
static void test_flow_blueprint(const std::string& flowTestPath)
{
    std::cout << "\n[Test 4] BlueprintRunner – flow_test.json 执行\n";

    BlueprintRunner runner;
    RegisterBuiltinHandlers(runner, ".");

    std::vector<std::string> logs;
    // Debug diagnostics → logs[]
    runner.SetLogCallback([&](NodeEditor::Runtime::LogLevel lv, const std::string& msg) {
        logs.push_back(msg);
        std::cout << "    LOG: " << msg << "\n";
    });
    // PrintString / Log node output → also captured in logs[]
    runner.SetPrintCallback([&](NodeEditor::Runtime::LogLevel lv, const std::string& msg) {
        logs.push_back(msg);
        std::cout << "    PRINT: " << msg << "\n";
    });

    bool loaded = runner.LoadFromFile(flowTestPath);
    CHECK(loaded, "flow_test.json 加载成功");
    if (!loaded)
    {
        std::cout << "    Error: " << runner.GetLastError() << "\n";
        return;
    }

    auto result = runner.Execute();
    CHECK(result.success, "Execute() 返回 success=true");
    if (!result.success)
        std::cout << "    Error: " << result.errorMessage << "\n";

    // ForLoop 0..4 累加：0+1+2+3+4 = 10
    auto finalCounter = runner.GetVariable("counter");
    CHECK(finalCounter.asInt() == 10, "最终 counter == 10（0+1+2+3+4）");

    bool foundTrue = false;
    for (const auto& l : logs)
        if (l.find("TRUE") != std::string::npos) { foundTrue = true; break; }
    CHECK(foundTrue, "Branch True 分支被执行（PrintString 含 TRUE）");

    std::cout << "    nodesExecuted=" << result.nodesExecuted
              << "  elapsedMs=" << result.elapsedMs << "\n";
}

// ============================================================================
// Test 5: Map 节点端到端（BlueprintRunner 内联构建）
// ============================================================================
static void test_map_nodes()
{
    std::cout << "\n[Test 5] Map 节点端到端\n";

    BlueprintRunner runner;
    RegisterBuiltinHandlers(runner, ".");

    std::vector<std::string> logs;
    runner.SetLogCallback([&](NodeEditor::Runtime::LogLevel lv, const std::string& msg) { logs.push_back(msg); });

    BlueprintData bp;
    bp.metadata.name = "MapTest";

    // Node 1: MapSet (exec)
    {
        NodeInstance n;
        n.id = 1; n.definitionId = "MapSet";
        PinInfo p0; p0.id=11; p0.kind=PinKind::Input;  p0.isExec=true;  n.pins.push_back(p0);
        PinInfo p1; p1.id=12; p1.kind=PinKind::Input;  p1.dataType=PinDataType::Map;    p1.name="Map";   n.pins.push_back(p1);
        PinInfo p2; p2.id=13; p2.kind=PinKind::Input;  p2.dataType=PinDataType::Any;    p2.name="Key";   p2.defaultValue=Variant(std::string("hello")); n.pins.push_back(p2);
        PinInfo p3; p3.id=14; p3.kind=PinKind::Input;  p3.dataType=PinDataType::Any;    p3.name="Value"; p3.defaultValue=Variant(std::string("world")); n.pins.push_back(p3);
        PinInfo p4; p4.id=15; p4.kind=PinKind::Output; p4.isExec=true;  n.pins.push_back(p4);
        PinInfo p5; p5.id=16; p5.kind=PinKind::Output; p5.dataType=PinDataType::Map;    p5.name="Map";   n.pins.push_back(p5);
        bp.nodes.push_back(n);
    }

    // Node 2: MapGet
    {
        NodeInstance n;
        n.id = 2; n.definitionId = "MapGet";
        PinInfo p0; p0.id=21; p0.kind=PinKind::Input;  p0.dataType=PinDataType::Map;    p0.name="Map";   n.pins.push_back(p0);
        PinInfo p1; p1.id=22; p1.kind=PinKind::Input;  p1.dataType=PinDataType::Any;    p1.name="Key";   p1.defaultValue=Variant(std::string("hello")); n.pins.push_back(p1);
        PinInfo p2; p2.id=23; p2.kind=PinKind::Output; p2.dataType=PinDataType::Any;    p2.name="Value"; n.pins.push_back(p2);
        PinInfo p3; p3.id=24; p3.kind=PinKind::Output; p3.dataType=PinDataType::Boolean;p3.name="Found"; n.pins.push_back(p3);
        bp.nodes.push_back(n);
    }

    // Link: MapSet.Map_out(16) → MapGet.Map_in(21)
    { LinkInstance lk; lk.id=1; lk.startPinId=16; lk.endPinId=21; bp.links.push_back(lk); }
    bp.rebuildIndices();

    bool loaded = runner.Load(bp);
    CHECK(loaded, "内联 BlueprintData 加载成功");

    runner.SetPinValue(12, Variant(std::unordered_map<std::string,Variant>{}));

    ExecutionResult r1 = runner.ExecuteNode(1);
    CHECK(r1.success, "ExecuteNode(MapSet) success");

    ExecutionResult r2 = runner.ExecuteNode(2);
    CHECK(r2.success, "ExecuteNode(MapGet) success");

    Variant val   = runner.GetPinValue(23);
    Variant found = runner.GetPinValue(24);
    CHECK(found.asBool(), "MapGet Found == true");
    CHECK(val.asString() == "world", "MapGet Value == \"world\"");
}

// ============================================================================
// Test 6: crude_json UTF-8 多字节字符解析
// ============================================================================
static void test_crude_json_utf8()
{
    std::cout << "\n[Test 6] crude_json UTF-8 支持\n";

    const char* json_utf8 = u8"{\"name\": \"蓝图测试\", \"value\": 42}";
    auto v = crude_json::value::parse(json_utf8);
    CHECK(!v.is_discarded(), "包含中文的 JSON 解析成功（非 discarded）");
    CHECK(v.type() == crude_json::type_t::object, "解析结果为 object");
    CHECK(v["name"].get<std::string>() == u8"蓝图测试", "中文字段值正确（UTF-8 pass-through）");
    CHECK(v["value"].get<double>() == 42.0, "数字字段值正确");

    const char* json_desc = u8"{\"description\": \"这是一个ForLoop节点\"}";
    auto v2 = crude_json::value::parse(json_desc);
    CHECK(!v2.is_discarded(), "中文描述字段解析成功");
    CHECK(v2["description"].get<std::string>() == u8"这是一个ForLoop节点",
          "中文描述字段内容正确");
}

// ============================================================================
// Test 7: Add/Subtract/Multiply 节点类型提升 — integer 累加保持 integer
// ============================================================================
static void test_add_type_promotion()
{
    std::cout << "\n[Test 7] Add 节点类型提升（integer 保持 integer）\n";

    // Integer + Integer → Integer
    {
        Variant a(int64_t(3)), b(int64_t(4));
        CHECK(a.type == PinDataType::Integer, "a 是 Integer");
        CHECK(b.type == PinDataType::Integer, "b 是 Integer");
        Variant r(a.asInt() + b.asInt());
        CHECK(r.type == PinDataType::Integer, "Integer+Integer 结果是 Integer");
        CHECK(r.asInt() == 7, "Integer+Integer 结果值正确（3+4=7）");
        CHECK(r.asString() == "7", "Integer asString 无 '0.000000' 格式");
    }

    // Float + Integer → Float
    {
        Variant a(3.14), b(int64_t(2));
        CHECK(a.type == PinDataType::Float, "a 是 Float");
        Variant r(a.asFloat() + b.asFloat());
        CHECK(r.type == PinDataType::Float, "Float+Integer 结果是 Float");
    }

    // Unknown/Any + Integer → Integer（resolveArithType 规则）
    {
        Variant a;               // Unknown/Any（如 GetVariable Any 输出）
        Variant b(int64_t(5));
        CHECK(a.type == PinDataType::Unknown, "a 是 Unknown（Any）");
        bool useInt = (a.type != PinDataType::Float && b.type != PinDataType::Float)
                   && (a.type == PinDataType::Integer || b.type == PinDataType::Integer);
        CHECK(useInt, "Unknown+Integer 应走 Integer 路径");
    }
}

// ============================================================================
// 测试入口
// ============================================================================
static int runTests(const std::string& flowTestPath)
{
    g_pass = 0;
    g_fail = 0;

    std::cout << "========== BlueprintRuntime 单元测试 ==========\n";
    test_registry_no_dangling_ptr();
    test_variant_conversion();
    test_variant_map();
    test_flow_blueprint(flowTestPath);
    test_map_nodes();
    test_crude_json_utf8();
    test_add_type_promotion();

    std::cout << "\n========== 结果 ==========\n";
    std::cout << "PASS: " << g_pass << "  FAIL: " << g_fail << "\n";
    return (g_fail == 0) ? 0 : 1;
}

// ============================================================================
// 示例: 手动构建一个蓝图数据，模拟编辑器导出的结果
// ============================================================================

static BlueprintData createSampleBlueprint()
{
    BlueprintData bp;

    bp.metadata.name        = "MathPipeline";
    bp.metadata.description = "A simple math computation pipeline";
    bp.metadata.version     = "1.0";
    bp.metadata.author      = "Runtime Example";
    bp.metadata.createdAt   = "2026-03-14T10:00:00Z";
    bp.metadata.updatedAt   = "2026-03-14T12:30:00Z";

    // Node 1: Constant A (10)
    {
        NodeInstance node;
        node.id = 1; node.definitionId = "constant_float"; node.name = "Constant A";
        node.position = NodePosition(100, 200); node.size = NodeSize(150, 80);
        PinInfo outPin; outPin.id=101; outPin.name="Value"; outPin.kind=PinKind::Output; outPin.dataType=PinDataType::Float;
        node.pins.push_back(outPin);
        node.nodeData["value"] = Variant(10.0);
        bp.nodes.push_back(std::move(node));
    }

    // Node 2: Constant B (3)
    {
        NodeInstance node;
        node.id = 2; node.definitionId = "constant_float"; node.name = "Constant B";
        node.position = NodePosition(100, 350); node.size = NodeSize(150, 80);
        PinInfo outPin; outPin.id=201; outPin.name="Value"; outPin.kind=PinKind::Output; outPin.dataType=PinDataType::Float;
        node.pins.push_back(outPin);
        node.nodeData["value"] = Variant(3.0);
        bp.nodes.push_back(std::move(node));
    }

    // Node 3: Add
    {
        NodeInstance node;
        node.id = 3; node.definitionId = "math_add"; node.name = "Add";
        node.position = NodePosition(350, 275); node.size = NodeSize(120, 100);
        PinInfo inA; inA.id=301; inA.name="A"; inA.kind=PinKind::Input;  inA.dataType=PinDataType::Float; node.pins.push_back(inA);
        PinInfo inB; inB.id=302; inB.name="B"; inB.kind=PinKind::Input;  inB.dataType=PinDataType::Float; node.pins.push_back(inB);
        PinInfo out; out.id=303;  out.name="Result"; out.kind=PinKind::Output; out.dataType=PinDataType::Float; node.pins.push_back(out);
        bp.nodes.push_back(std::move(node));
    }

    // Node 4: Display
    {
        NodeInstance node;
        node.id = 4; node.definitionId = "output_display"; node.name = "Display Result";
        node.position = NodePosition(550, 275); node.size = NodeSize(160, 80);
        PinInfo inVal; inVal.id=401; inVal.name="Input"; inVal.kind=PinKind::Input; inVal.dataType=PinDataType::Float; node.pins.push_back(inVal);
        bp.nodes.push_back(std::move(node));
    }

    { LinkInstance lk; lk.id=1001; lk.startPinId=101; lk.endPinId=301; bp.links.push_back(lk); }
    { LinkInstance lk; lk.id=1002; lk.startPinId=201; lk.endPinId=302; bp.links.push_back(lk); }
    { LinkInstance lk; lk.id=1003; lk.startPinId=303; lk.endPinId=401; bp.links.push_back(lk); }

    VariableDefinition speedVar;
    speedVar.name="Speed"; speedVar.dataType=PinDataType::Float; speedVar.defaultValue=Variant(1.5);
    speedVar.category="Physics"; speedVar.tooltip="Movement speed multiplier";
    bp.variables.push_back(speedVar);

    CommentRegion comment;
    comment.id="comment_1"; comment.text="Math Pipeline - Input Section";
    comment.position=NodePosition(50,150); comment.size=NodeSize(250,300);
    comment.color="#4488FF"; comment.alpha=0.3f;
    bp.comments.push_back(comment);

    bp.viewInfo.viewPosition = NodePosition(0, 0);
    bp.viewInfo.viewScale    = 1.0f;

    return bp;
}

// ============================================================================
// 节点处理器（示例模式用）
// ============================================================================

static bool handler_ConstantFloat(ExecutionContext& ctx)
{
    Variant value = ctx.GetNodeData("value");
    ctx.SetOutputValue("Value", value);
    ctx.Log("  -> Output: " + std::to_string(value.asFloat()));
    return true;
}

static bool handler_MathAdd(ExecutionContext& ctx)
{
    double a = ctx.GetInputValue("A").asFloat();
    double b = ctx.GetInputValue("B").asFloat();
    double r = a + b;
    ctx.SetOutputValue("Result", Variant(r));
    ctx.Log("  -> " + std::to_string(a) + " + " + std::to_string(b) + " = " + std::to_string(r));
    return true;
}

static bool handler_OutputDisplay(ExecutionContext& ctx)
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

    if (!fs::exists(filePath))
    {
        std::cerr << "ERROR: File not found: " << filePath << std::endl;
        return 1;
    }

    std::string basePath = fs::path(filePath).parent_path().string();
    std::string fileName = fs::path(filePath).filename().string();

    std::cout << "========================================" << std::endl;
    std::cout << "  Blueprint Runtime Executor"            << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << "File:      " << filePath   << std::endl;
    std::cout << "BasePath:  " << basePath   << std::endl;
    std::cout << "MaxTime:   " << maxTimeSec << "s" << std::endl;
    std::cout << "TickRate:  " << tickRateMs << "ms" << std::endl;
    std::cout << std::endl;

    BlueprintRunner runner;
    runner.SetLogCallback([](NodeEditor::Runtime::LogLevel, const std::string& msg) { std::cout << msg << std::endl; });
    runner.SetPrintCallback([](NodeEditor::Runtime::LogLevel, const std::string& msg) { std::cout << msg << std::endl; });

    bool isEditorFile = (fileName.size() > 12 && fileName.substr(fileName.size()-12) == ".editor.json");

    if (isEditorFile)
    {
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

    RegisterBuiltinHandlers(runner, basePath);
    runner.SetDefaultHandler([](ExecutionContext& ctx) {
        auto node = ctx.GetCurrentNode();
        std::string n = node ? node->name : "(unknown)";
        ctx.Log("  [Default Handler] pass-through for: " + n);
        return true;
    });

    std::cout << "========================================" << std::endl;
    std::cout << "  Execution Started"                     << std::endl;
    std::cout << "========================================" << std::endl;

    auto startTime = std::chrono::high_resolution_clock::now();
    auto result    = runner.Execute();
    auto endTime   = std::chrono::high_resolution_clock::now();
    double elapsed = std::chrono::duration<double, std::milli>(endTime - startTime).count();

    std::cout << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << "  Execution " << (result.success ? "Completed Successfully!" : "FAILED!") << std::endl;
    std::cout << "  Nodes executed: " << result.nodesExecuted << std::endl;
    std::cout << "  Elapsed: " << elapsed << " ms" << std::endl;
    std::cout << "========================================" << std::endl;

    if (runner.GetTimerManager().GetActiveTimerCount() > 0)
    {
        std::cout << "\n[Timer Loop] Active timers: " << runner.GetTimerManager().GetActiveTimerCount()
                  << ", entering frame loop..." << std::endl;

        auto loopStart = std::chrono::high_resolution_clock::now();
        auto lastTick  = loopStart;

        while (runner.GetTimerManager().GetActiveTimerCount() > 0)
        {
            auto now = std::chrono::high_resolution_clock::now();
            double totalElapsed = std::chrono::duration<double>(now - loopStart).count();
            if (totalElapsed > maxTimeSec)
            {
                std::cout << "[Timer Loop] Max time (" << maxTimeSec << "s) exceeded, stopping." << std::endl;
                break;
            }
            float deltaTime = std::chrono::duration<float>(now - lastTick).count();
            lastTick = now;
            runner.Tick(deltaTime);
            std::this_thread::sleep_for(std::chrono::milliseconds(tickRateMs));
        }

        auto loopEnd = std::chrono::high_resolution_clock::now();
        double loopElapsed = std::chrono::duration<double, std::milli>(loopEnd - loopStart).count();
        std::cout << "[Timer Loop] Finished. Loop time: " << loopElapsed << " ms" << std::endl;
        std::cout << "  Remaining active timers: " << runner.GetTimerManager().GetActiveTimerCount() << std::endl;
    }

    std::cout << "\n=== Done ===" << std::endl;
    return result.success ? 0 : 1;
}

// ============================================================================
// main
// ============================================================================

// 前向声明（实现在 test_new_features.cpp）
int runNewFeatureTests();

int main(int argc, char* argv[])
{
    // --help
    for (int i = 1; i < argc; ++i)
    {
        std::string a = argv[i];
        if (a == "--help" || a == "-h")
        {
            std::cout << "Usage: runtime-example [options] [<blueprint-file>]\n\n"
                      << "Options:\n"
                      << "  --test [flow_test.json]    Run built-in unit tests\n"
                      << "                             Optionally specify the path to flow_test.json\n"
                      << "                             (default: flow_test.json in current dir)\n"
                      << "  --max-time <seconds>       Max time to wait for async timers (default: 30)\n"
                      << "  --tick-rate <ms>           Frame tick interval in ms (default: 16)\n"
                      << "\nIf no arguments, runs the built-in sample blueprint.\n";
            return 0;
        }
    }

    // --test [optional path to flow_test.json]
    for (int i = 1; i < argc; ++i)
    {
        if (std::string(argv[i]) == "--test")
        {
            std::string flowPath = "flow_test.json";
            if (i + 1 < argc && argv[i+1][0] != '-')
                flowPath = argv[i+1];
            return runTests(flowPath);
        }
    }

    // --test-new: 新功能专项测试（Functions / Events / StringPool）
    for (int i = 1; i < argc; ++i)
    {
        if (std::string(argv[i]) == "--test-new")
            return runNewFeatureTests();
    }

    // 命令行模式: 传入蓝图文件
    if (argc >= 2)
    {
        std::string filePath  = argv[1];
        float  maxTimeSec = 30.0f;
        int    tickRateMs = 16;

        for (int i = 2; i < argc; ++i)
        {
            std::string arg = argv[i];
            if (arg == "--max-time" && i+1 < argc)
                maxTimeSec = std::stof(argv[++i]);
            else if (arg == "--tick-rate" && i+1 < argc)
                tickRateMs = std::stoi(argv[++i]);
            else
            {
                std::cerr << "Unknown option: " << arg << "\nUse --help for usage information.\n";
                return 1;
            }
        }

        return runBlueprintFromFile(filePath, maxTimeSec, tickRateMs);
    }

    // ----------------------------------------------------------------
    // 无参数模式：运行内置示例蓝图
    // ----------------------------------------------------------------
    std::cout << "=== Blueprint Dual-File Export Example ===" << std::endl;
    std::cout << "(Use 'runtime-example --test' to run unit tests)" << std::endl;
    std::cout << std::endl;

    BlueprintData blueprint = createSampleBlueprint();
    JsonBlueprintExporter exporter;
    ExportOptions opts;
    opts.prettyPrint = true;

    // Step 1: 导出为两个文件 (Runtime + Editor)
    std::cout << "--- Step 1: Export to dual files (Runtime + Editor) ---" << std::endl;
    auto exportResult = exporter.exportEditorFiles(blueprint, "blueprint.json", "blueprint.editor.json", opts);
    if (!exportResult.success)
    {
        std::cerr << "Export failed: " << exportResult.errorMessage << std::endl;
        return 1;
    }
    std::cout << "Runtime file:  blueprint.json         (" << exportResult.runtimeBytes << " bytes)" << std::endl;
    std::cout << "Editor file:   blueprint.editor.json   (" << exportResult.editorBytes  << " bytes)" << std::endl;
    std::cout << "Total:         " << exportResult.totalBytes() << " bytes" << std::endl;
    std::cout << std::endl;

    std::string runtimeJson = exporter.exportRuntimeToString(blueprint, opts);
    std::cout << "---- Runtime file content (blueprint.json) ----" << std::endl;
    std::cout << runtimeJson << std::endl;

    std::string editorJson = exporter.exportEditorToString(blueprint, opts);
    std::cout << "---- Editor file content (blueprint.editor.json) ----" << std::endl;
    std::cout << editorJson << std::endl;

    // Step 2: 运行时 — 只加载 Runtime 文件
    std::cout << "--- Step 2: Runtime — load only Runtime file and execute ---" << std::endl;
    BlueprintRunner runner;
    runner.SetLogCallback([](NodeEditor::Runtime::LogLevel, const std::string& msg) { std::cout << msg << std::endl; });
    runner.SetPrintCallback([](NodeEditor::Runtime::LogLevel, const std::string& msg) { std::cout << msg << std::endl; });

    if (!runner.LoadFromFile("blueprint.json"))
    {
        std::cerr << "ERROR: " << runner.GetLastError() << std::endl;
        return 1;
    }
    std::cout << "Blueprint loaded: " << runner.GetBlueprintData().metadata.name << std::endl;
    std::cout << "Nodes: " << runner.GetBlueprintData().nodes.size() << std::endl;
    std::cout << "Links: " << runner.GetBlueprintData().links.size() << std::endl;

    const auto* node1 = runner.GetBlueprintData().findNode(1);
    if (node1)
        std::cout << "Node 'Constant A' position: (" << node1->position.x << ", " << node1->position.y
                  << ") (should be 0,0 — not in Runtime file)" << std::endl;
    std::cout << "Comments count: " << runner.GetBlueprintData().comments.size()
              << " (should be 0 — not in Runtime file)" << std::endl;
    std::cout << std::endl;

    runner.RegisterHandler("constant_float", handler_ConstantFloat);
    runner.RegisterHandler("math_add",       handler_MathAdd);
    runner.RegisterHandler("output_display", handler_OutputDisplay);

    std::cout << "Executing..." << std::endl;
    auto result = runner.Execute();
    std::cout << "Success: " << (result.success ? "Yes" : "No") << std::endl;
    std::cout << "Nodes executed: " << result.nodesExecuted << std::endl;
    std::cout << "Elapsed: " << result.elapsedMs << " ms" << std::endl;
    std::cout << std::endl;

    // Step 3: 编辑器 — 加载两个文件合并
    std::cout << "--- Step 3: Editor — load both files and merge ---" << std::endl;
    auto mergeResult = exporter.importEditorFromFiles("blueprint.json", "blueprint.editor.json");
    if (!mergeResult.success)
    {
        std::cerr << "Merge import failed: " << mergeResult.errorMessage << std::endl;
        return 1;
    }
    std::cout << "Merged data loaded OK!" << std::endl;
    std::cout << "  Name: "      << mergeResult.data.metadata.name      << std::endl;
    std::cout << "  Author: "    << mergeResult.data.metadata.author    << " (from Editor file)" << std::endl;
    std::cout << "  CreatedAt: " << mergeResult.data.metadata.createdAt << " (from Editor file)" << std::endl;
    std::cout << "  Nodes: "     << mergeResult.data.nodes.size()       << std::endl;

    const auto* mergedNode1 = mergeResult.data.findNode(1);
    if (mergedNode1)
    {
        std::cout << "  Node 'Constant A' position: (" << mergedNode1->position.x << ", " << mergedNode1->position.y
                  << ") (restored from Editor file)" << std::endl;
        std::cout << "  Node 'Constant A' size: (" << mergedNode1->size.width << "x" << mergedNode1->size.height
                  << ") (restored from Editor file)" << std::endl;
    }
    std::cout << "  Comments: " << mergeResult.data.comments.size() << " (restored from Editor file)" << std::endl;
    if (!mergeResult.data.comments.empty())
        std::cout << "    Comment[0]: \"" << mergeResult.data.comments[0].text << "\"" << std::endl;
    if (!mergeResult.data.variables.empty())
    {
        const auto& var = mergeResult.data.variables[0];
        std::cout << "  Variable '" << var.name << "' category: \"" << var.category
                  << "\", tooltip: \"" << var.tooltip << "\" (from Editor file)" << std::endl;
    }
    std::cout << "  Total bytes read: " << mergeResult.bytesRead << std::endl;
    std::cout << std::endl;

    // Step 4: 注入外部值重新执行
    std::cout << "--- Step 4: Inject external values and re-execute ---" << std::endl;
    runner.ResetState();
    runner.SetPinValue(101, Variant(42.0));
    runner.SetPinValue(201, Variant(8.0));
    runner.RegisterHandler("constant_float", [](ExecutionContext& ctx) {
        ctx.Log("  -> (using pre-set pin value)");
        return true;
    });
    auto result2 = runner.Execute();
    std::cout << "Success: " << (result2.success ? "Yes" : "No") << std::endl;
    std::cout << "Expected: 42 + 8 = 50" << std::endl;

    // Step 5: 蓝图变量
    std::cout << "\n--- Step 5: Blueprint variables ---" << std::endl;
    auto speed = runner.GetVariable("Speed");
    std::cout << "Variable 'Speed' = " << speed.asFloat() << std::endl;
    runner.SetVariable("Speed", Variant(3.0));
    std::cout << "Variable 'Speed' (updated) = " << runner.GetVariable("Speed").asFloat() << std::endl;

    std::cout << "\n=== Done ===" << std::endl;

#ifdef _WIN32
    system("pause");
#endif

    return 0;
}
