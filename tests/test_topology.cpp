// tests/test_topology.cpp -- 拓扑排序 / 执行顺序 / 端到端集成测试
// buildTopologicalOrder 是 BlueprintRunner 的私有方法，通过执行结果间接验证顺序正确性
#include <gtest/gtest.h>
#include "BlueprintRunner.h"
#include "BlueprintData.h"
#include "BuiltinHandlers.h"
#include "test_helpers.h"

using namespace NodeEditor::Runtime;
using namespace TestHelpers;

// ── 辅助 ─────────────────────────────────────────────────────────────────────

static NodeInstance makeNode(uint64_t id, const char* defId = "Nop")
{
    NodeInstance n; n.id = id; n.definitionId = defId; return n;
}
static LinkInstance makeLink(uint64_t id, uint64_t start, uint64_t end)
{
    LinkInstance l; l.id = id; l.startPinId = start; l.endPinId = end; return l;
}
static PinInfo makePin(uint64_t id, PinKind k, bool exec, PinDataType dt = PinDataType::Unknown, const char* name = "")
{
    PinInfo p; p.id = id; p.kind = k; p.isExec = exec; p.dataType = dt; p.name = name; return p;
}

// ── 线性链：SetVariable → ForLoop 类型的顺序验证 ─────────────────────────────

TEST(TopologyTest, LinearChainExecutionOrder)
{
    // 用两个 SetVariable 节点 + exec 链，验证执行顺序（A先B后）
    BlueprintData bp;
    bp.metadata.name = "OrderTest";

    // Node A: SetVariable("a", 1)  execOut=10
    {
        NodeInstance n; n.id = 1; n.definitionId = "SetVariable";
        n.pins.push_back(makePin(10, PinKind::Input,  true));   // exec in
        n.pins.push_back(makePin(11, PinKind::Input,  false, PinDataType::String, "Name"));
        n.pins.push_back(makePin(12, PinKind::Input,  false, PinDataType::Any,    "Value"));
        n.pins.push_back(makePin(13, PinKind::Output, true));   // exec out
        n.pins[1].defaultValue = Variant(std::string("a"));
        n.pins[2].defaultValue = Variant(int64_t(1));
        bp.nodes.push_back(n);
    }
    // Node B: SetVariable("a", 2)  execIn=20
    {
        NodeInstance n; n.id = 2; n.definitionId = "SetVariable";
        n.pins.push_back(makePin(20, PinKind::Input,  true));
        n.pins.push_back(makePin(21, PinKind::Input,  false, PinDataType::String, "Name"));
        n.pins.push_back(makePin(22, PinKind::Input,  false, PinDataType::Any,    "Value"));
        n.pins.push_back(makePin(23, PinKind::Output, true));
        n.pins[1].defaultValue = Variant(std::string("a"));
        n.pins[2].defaultValue = Variant(int64_t(2));
        bp.nodes.push_back(n);
    }
    bp.links = { makeLink(1, 13, 20) };  // A.execOut → B.execIn

    VariableDefinition var; var.name = "a"; var.dataType = PinDataType::Integer;
    var.defaultValue = Variant(int64_t(0));
    bp.variables.push_back(var);

    bp.rebuildIndices();

    BlueprintRunner runner;
    RegisterBuiltinHandlers(runner, ".");
    ASSERT_TRUE(runner.Load(bp));

    auto result = RunWithTick(runner);
    EXPECT_TRUE(result.success) << runner.GetLastError();

    // A 先执行（a=1），B 后执行（a=2），最终 a==2
    Variant a = runner.GetVariable("a");
    EXPECT_EQ(a.asInt(), 2) << "B should execute after A, final a should be 2";
}

// ── 无环图：exec 悬空节点不执行 ──────────────────────────────────────────────

TEST(TopologyTest, AcyclicGraphExecDanglingSkipped)
{
    BlueprintData bp;
    bp.metadata.name = "AcyclicTest";

    // 单个 PrintString 节点（exec 输入悬空 → 新机制下不执行）
    NodeInstance n; n.id = 1; n.definitionId = "PrintString";
    n.pins.push_back(makePin(10, PinKind::Input, true));   // exec in（悬空）
    n.pins.push_back(makePin(11, PinKind::Input, false, PinDataType::String, "In String"));
    n.pins[1].defaultValue = Variant(std::string("topology_test"));
    bp.nodes.push_back(n);
    bp.rebuildIndices();

    BlueprintRunner runner;
    RegisterBuiltinHandlers(runner, ".");

    ASSERT_TRUE(runner.Load(bp));
    auto result = RunWithTick(runner);
    EXPECT_TRUE(result.success);
    // exec 输入悬空 → 可达性过滤后不执行
    EXPECT_EQ(result.nodesExecuted, 0u) << "Dangling exec-input node should be skipped";
}

// ── 末端悬空节点（execIn悬空，无execOut）不执行 ─────────────────────────────────

TEST(TopologyTest, DanglingExecNodeNotExecuted)
{
    // PrintString 只有 exec 输入（悬空），无 exec 输出（末端节点）→ 不执行
    // 注意：若节点有 execOut（控制流节点），则视为隐式入口，会执行（见 ImplicitEntryChainExecutes）
    BlueprintData bp;
    bp.metadata.name = "DanglingExecTest";

    NodeInstance n; n.id = 1; n.definitionId = "PrintString";
    n.pins.push_back(makePin(10, PinKind::Input,  true));   // exec in（悬空）
    // 故意不加 exec out，使其成为末端悬空节点
    n.pins.push_back(makePin(12, PinKind::Input,  false, PinDataType::String, "In String"));
    n.pins[1].defaultValue = Variant(std::string("should_not_print"));
    bp.nodes.push_back(n);
    bp.rebuildIndices();

    std::vector<std::string> prints;
    BlueprintRunner runner;
    RegisterBuiltinHandlers(runner, ".");
    runner.SetPrintCallback([&](LogLevel, const std::string& m){ prints.push_back(m); });
    ASSERT_TRUE(runner.Load(bp));

    auto result = RunWithTick(runner);
    EXPECT_TRUE(result.success);
    EXPECT_EQ(result.nodesExecuted, 0u) << "Terminal dangling exec node (no execOut) should not execute";
    EXPECT_TRUE(prints.empty()) << "Should not print anything";
}

// ── 控制流隐式入口（exec悬空+有execOut）→ 执行；末端悬空节点（无execOut）→ 不执行 ──────

TEST(TopologyTest, DanglingChainNotExecuted)
{
    // 场景：末端悬空节点（exec输入悬空，无 exec 输出）不执行
    // PrintString 只有 exec 输入（悬空），无 exec 输出 → 不执行
    BlueprintData bp;
    bp.metadata.name = "TerminalDanglingTest";

    NodeInstance n; n.id = 1; n.definitionId = "PrintString";
    n.pins.push_back(makePin(10, PinKind::Input,  true));   // exec in（悬空，无 exec out）
    n.pins.push_back(makePin(12, PinKind::Input,  false, PinDataType::String, "In String"));
    n.pins[1].defaultValue = Variant(std::string("should_not_print_terminal"));
    bp.nodes.push_back(n);
    bp.rebuildIndices();

    std::vector<std::string> prints;
    BlueprintRunner runner;
    RegisterBuiltinHandlers(runner, ".");
    runner.SetPrintCallback([&](LogLevel, const std::string& m){ prints.push_back(m); });
    ASSERT_TRUE(runner.Load(bp));

    auto result = RunWithTick(runner);
    EXPECT_TRUE(result.success);
    EXPECT_EQ(result.nodesExecuted, 0u) << "Terminal dangling exec node (no execOut) should not execute";
    EXPECT_TRUE(prints.empty()) << "Should not print anything";
}

TEST(TopologyTest, ImplicitEntryChainExecutes)
{
    // 场景：PrintA 有 execIn悬空 + execOut（隐式入口）→ PrintA 和 PrintB 都执行
    // 这对应于 Main.bjson 中 ForLoop 等直接放在画布上的控制流节点
    BlueprintData bp;
    bp.metadata.name = "ImplicitEntryChainTest";

    // PrintA: execIn=10（悬空）, execOut=11
    {
        NodeInstance n; n.id = 1; n.definitionId = "PrintString";
        n.pins.push_back(makePin(10, PinKind::Input,  true));
        n.pins.push_back(makePin(11, PinKind::Output, true));
        n.pins.push_back(makePin(12, PinKind::Input,  false, PinDataType::String, "In String"));
        n.pins[2].defaultValue = Variant(std::string("PrintA"));
        bp.nodes.push_back(n);
    }
    // PrintB: execIn=20（连着PrintA）
    {
        NodeInstance n; n.id = 2; n.definitionId = "PrintString";
        n.pins.push_back(makePin(20, PinKind::Input,  true));
        n.pins.push_back(makePin(21, PinKind::Output, true));
        n.pins.push_back(makePin(22, PinKind::Input,  false, PinDataType::String, "In String"));
        n.pins[2].defaultValue = Variant(std::string("PrintB"));
        bp.nodes.push_back(n);
    }
    bp.links = { makeLink(1, 11, 20) };
    bp.rebuildIndices();

    std::vector<std::string> prints;
    BlueprintRunner runner;
    RegisterBuiltinHandlers(runner, ".");
    runner.SetPrintCallback([&](LogLevel, const std::string& m){ prints.push_back(m); });
    ASSERT_TRUE(runner.Load(bp));

    auto result = RunWithTick(runner);
    EXPECT_TRUE(result.success);
    EXPECT_GE(result.nodesExecuted, 2u) << "Implicit entry chain (execIn dangling + execOut) should execute";
    bool foundA = false, foundB = false;
    for (const auto& p : prints) {
        if (p == "PrintA") foundA = true;
        if (p == "PrintB") foundB = true;
    }
    EXPECT_TRUE(foundA) << "PrintA should execute";
    EXPECT_TRUE(foundB) << "PrintB should execute via ActivateOutputFlow";
}

// ── 纯数据节点（无 exec 引脚）正常执行 ──────────────────────────────────────────

TEST(TopologyTest, PureDataNodeExecutes)
{
    // SetVariable 只有 data 引脚（无 exec），应正常执行
    BlueprintData bp;
    bp.metadata.name = "PureDataTest";

    NodeInstance n; n.id = 1; n.definitionId = "SetVariable";
    n.pins.push_back(makePin(11, PinKind::Input, false, PinDataType::String, "Name"));
    n.pins.push_back(makePin(12, PinKind::Input, false, PinDataType::Any,    "Value"));
    n.pins[0].defaultValue = Variant(std::string("x"));
    n.pins[1].defaultValue = Variant(int64_t(42));
    bp.nodes.push_back(n);

    VariableDefinition var; var.name = "x"; var.dataType = PinDataType::Integer;
    var.defaultValue = Variant(int64_t(0));
    bp.variables.push_back(var);
    bp.rebuildIndices();

    BlueprintRunner runner;
    RegisterBuiltinHandlers(runner, ".");
    ASSERT_TRUE(runner.Load(bp));

    auto result = RunWithTick(runner);
    EXPECT_TRUE(result.success);
    EXPECT_GE(result.nodesExecuted, 1u) << "Pure data node (no exec pins) should execute";
}

// ── DispatchEvent("OnBeginPlay") 触发事件链 ──────────────────────────────────

TEST(TopologyTest, DispatchEventOnBeginPlay)
{
    // OnBeginPlay → PrintString：
    //   Execute() 不触发，DispatchEvent("OnBeginPlay") 才触发
    BlueprintData bp;
    bp.metadata.name = "BeginPlayTest";

    // OnBeginPlay：无 exec 输入，有 exec 输出（事件源节点）
    {
        NodeInstance n; n.id = 1; n.definitionId = "OnBeginPlay";
        n.name = "On Begin Play";
        n.pins.push_back(makePin(11, PinKind::Output, true));  // exec out
        bp.nodes.push_back(n);
    }
    // PrintString：execIn=20
    {
        NodeInstance n; n.id = 2; n.definitionId = "PrintString";
        n.pins.push_back(makePin(20, PinKind::Input,  true));
        n.pins.push_back(makePin(21, PinKind::Output, true));
        n.pins.push_back(makePin(22, PinKind::Input,  false, PinDataType::String, "In String"));
        n.pins[2].defaultValue = Variant(std::string("BeginPlay_fired"));
        bp.nodes.push_back(n);
    }
    bp.links = { makeLink(1, 11, 20) };  // OnBeginPlay.execOut → PrintString.execIn
    bp.rebuildIndices();

    std::vector<std::string> prints;
    BlueprintRunner runner;
    RegisterBuiltinHandlers(runner, ".");
    runner.SetPrintCallback([&](LogLevel, const std::string& m){ prints.push_back(m); });
    ASSERT_TRUE(runner.Load(bp));

    // Execute() 不应触发 BeginPlay 链
    auto execResult = RunWithTick(runner);
    EXPECT_TRUE(execResult.success);
    EXPECT_TRUE(prints.empty()) << "Execute() should NOT fire OnBeginPlay chain";

    // 重置后用 DispatchEvent 触发
    runner.ResetState();
    ASSERT_TRUE(runner.Load(bp));
    prints.clear();
    auto dispResult = runner.DispatchEvent("OnBeginPlay");
    EXPECT_TRUE(dispResult.success);
    EXPECT_FALSE(prints.empty()) << "DispatchEvent should fire OnBeginPlay chain";
    bool found = false;
    for (const auto& p : prints)
        if (p.find("BeginPlay_fired") != std::string::npos) { found = true; break; }
    EXPECT_TRUE(found) << "PrintString should have printed 'BeginPlay_fired'";
}

// ── 反向数据可达性：末端悬空节点的数据上游也不执行 ─────────────────────────────

TEST(TopologyTest, DanglingExecChainDataUpstreamAlsoSkipped)
{
    // 末端悬空节点（execIn悬空，无execOut，如 PrintString）的数据上游 IntToString 也不执行
    // IntToString（纯数据）→ PrintString（execIn悬空，无execOut）
    // IntToString 虽然是纯数据节点（无 exec 引脚），但它只被 PrintString 引用
    // PrintString 不可达 → IntToString 不在任何 exec 节点的数据需求中 → 结果只取决于独立执行
    //
    // 注意：IntToString 无 exec 输入，是独立的数据入口，会被执行
    // 但 PrintString 不执行，SetVariable 也不执行，变量 v 保持默认值
    BlueprintData bp;
    bp.metadata.name = "DataReachableTest";

    // Node1: IntToString，纯数据（无 exec 引脚）
    {
        NodeInstance n; n.id = 1; n.definitionId = "IntToString";
        n.pins.push_back(makePin(100, PinKind::Input,  false, PinDataType::Integer, "Value"));
        n.pins.push_back(makePin(101, PinKind::Output, false, PinDataType::String,  "Result"));
        n.pins[0].defaultValue = Variant(int64_t(77));
        bp.nodes.push_back(n);
    }
    // Node2: PrintString，execIn悬空，无 execOut（末端悬空节点）
    {
        NodeInstance n; n.id = 2; n.definitionId = "PrintString";
        n.pins.push_back(makePin(200, PinKind::Input,  true));   // exec in（悬空，无 execOut）
        n.pins.push_back(makePin(201, PinKind::Input,  false, PinDataType::String, "In String"));
        // 注意：故意不加 exec out，使 PrintString 成为末端悬空节点
        bp.nodes.push_back(n);
    }
    // IntToString.Result(101) → PrintString.In String(201)
    bp.links = { makeLink(1, 101, 201) };
    bp.rebuildIndices();

    std::vector<std::string> prints;
    BlueprintRunner runner;
    RegisterBuiltinHandlers(runner, ".");
    runner.SetPrintCallback([&](LogLevel, const std::string& m){ prints.push_back(m); });
    ASSERT_TRUE(runner.Load(bp));

    auto result = RunWithTick(runner);
    EXPECT_TRUE(result.success);
    // PrintString 末端悬空 → 不执行（其数据上游 IntToString 虽独立执行，但无输出被消费）
    EXPECT_TRUE(prints.empty()) << "Terminal dangling PrintString should not print";
}

// ── DispatchEvent：不存在的事件名静默返回成功 ────────────────────────────────

TEST(TopologyTest, DispatchEventNonExistentSilent)
{
    BlueprintData bp;
    bp.metadata.name = "NoEventTest";

    NodeInstance n; n.id = 1; n.definitionId = "PrintString";
    n.pins.push_back(makePin(10, PinKind::Input, false, PinDataType::String, "In String"));
    bp.nodes.push_back(n);
    bp.rebuildIndices();

    BlueprintRunner runner;
    RegisterBuiltinHandlers(runner, ".");
    ASSERT_TRUE(runner.Load(bp));

    auto result = runner.DispatchEvent("OnNonExistentEvent");
    EXPECT_TRUE(result.success) << "DispatchEvent with unknown event should return success silently";
    EXPECT_EQ(result.nodesExecuted, 0u);
}

// ── 端到端：Main.bjson 完整集成（ForLoop + Branch + FuncLib + ExecuteBlueprint）─

TEST(TopologyTest, ForLoopExecution)
{
    BlueprintRunner runner;
    RegisterBuiltinHandlers(runner, "assets");

    std::vector<std::string> logs;
    runner.SetPrintCallback([&](LogLevel, const std::string& m){ logs.push_back(m); });

    bool loaded = runner.LoadFromFileWithDeps("assets/Main.bjson");
    ASSERT_TRUE(loaded) << "Failed to load assets/Main.bjson: " << runner.GetLastError();

    RunWithTick(runner);

    int loopCount = 0;
    for (const auto& l : logs)
        if (l.rfind("Loop Index:", 0) == 0) ++loopCount;
    EXPECT_EQ(loopCount, 11) << "ForLoop 0..10 should print 11 'Loop Index:N' lines";

    bool branchHit = false;
    for (const auto& l : logs)
        if (l.find("Index Matched 5") != std::string::npos) { branchHit = true; break; }
    EXPECT_TRUE(branchHit) << "Branch should print 'Index Matched 5' when index==5";

    bool loopFinished = false;
    for (const auto& l : logs)
        if (l.find("Loop Finished") != std::string::npos) { loopFinished = true; break; }
    EXPECT_TRUE(loopFinished) << "ForLoop.Completed should print 'Loop Finished'";

    bool subContinue = false;
    for (const auto& l : logs)
        if (l.find("Sub Continue") != std::string::npos) { subContinue = true; break; }
    EXPECT_TRUE(subContinue) << "After ExecuteBlueprint, should print 'Sub Continue'";

    bool libMsg = false;
    for (const auto& l : logs)
        if (l.find("test lib") != std::string::npos) { libMsg = true; break; }
    EXPECT_TRUE(libMsg) << "FuncLib.TestPrint should print the Message input 'test lib'";

    bool libReturn = false;
    for (const auto& l : logs)
        if (l.find("Lib Return:") != std::string::npos &&
            l.find("99") != std::string::npos) { libReturn = true; break; }
    EXPECT_TRUE(libReturn) << "FuncLib.TestPrint return value should be 99";
}

// ── 端到端：Sub/Sub.bjson 含 Delay 异步节点 ────────────────────────────────────

TEST(TopologyTest, AsyncDelayInSubBlueprint)
{
    BlueprintRunner runner;
    RegisterBuiltinHandlers(runner, "assets");

    std::vector<std::string> logs;
    runner.SetPrintCallback([&](LogLevel, const std::string& m){ logs.push_back(m); });

    bool loaded = runner.LoadFromFile("assets/Sub/Sub.bjson");
    ASSERT_TRUE(loaded) << "Failed to load assets/Sub/Sub.bjson: " << runner.GetLastError();

    RunWithTick(runner);

    bool enterSub = false, delayContinue = false, delayReached = false;
    for (const auto& l : logs) {
        if (l.find("Enter Sub")      != std::string::npos) enterSub      = true;
        if (l.find("Delay Continue") != std::string::npos) delayContinue = true;
        if (l.find("Delay Reached")  != std::string::npos) delayReached  = true;
    }
    EXPECT_TRUE(enterSub)      << "Should print 'Enter Sub' at start";
    EXPECT_TRUE(delayContinue) << "Delay.Exec (sync) should print 'Delay Continue'";
    EXPECT_TRUE(delayReached)  << "Delay.Completed (async, 2s) should print 'Delay Reached'";
}
