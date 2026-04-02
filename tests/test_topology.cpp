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

// ── 无环图正常执行 ────────────────────────────────────────────────────────────

TEST(TopologyTest, AcyclicGraphExecutes)
{
    BlueprintData bp;
    bp.metadata.name = "AcyclicTest";

    // 单个 PrintString 节点
    NodeInstance n; n.id = 1; n.definitionId = "PrintString";
    n.pins.push_back(makePin(10, PinKind::Input, true));
    n.pins.push_back(makePin(11, PinKind::Input, false, PinDataType::String, "In String"));
    n.pins[1].defaultValue = Variant(std::string("topology_test"));
    bp.nodes.push_back(n);
    bp.rebuildIndices();

    BlueprintRunner runner;
    RegisterBuiltinHandlers(runner, ".");

    ASSERT_TRUE(runner.Load(bp));
    auto result = RunWithTick(runner);
    EXPECT_TRUE(result.success);
    EXPECT_GE(result.nodesExecuted, 1u);
}

// ── 孤立节点（无连线）也能执行 ───────────────────────────────────────────────

TEST(TopologyTest, IsolatedNodeExecutes)
{
    BlueprintData bp;
    bp.metadata.name = "IsolatedTest";

    NodeInstance n; n.id = 1; n.definitionId = "PrintString";
    n.pins.push_back(makePin(10, PinKind::Input, true));
    n.pins.push_back(makePin(11, PinKind::Input, false, PinDataType::String, "In String"));
    n.pins[1].defaultValue = Variant(std::string("isolated"));
    bp.nodes.push_back(n);
    bp.rebuildIndices();

    BlueprintRunner runner;
    RegisterBuiltinHandlers(runner, ".");
    ASSERT_TRUE(runner.Load(bp));
    auto result = RunWithTick(runner);
    EXPECT_TRUE(result.success);
}

// ── 端到端：Main.bjson 完整集成（ForLoop + Branch + FuncLib + ExecuteBlueprint）─

TEST(TopologyTest, ForLoopExecution)
{
    BlueprintRunner runner;
    RegisterBuiltinHandlers(runner, "assets");

    std::vector<std::string> logs;
    runner.SetPrintCallback([&](LogLevel, const std::string& m){ logs.push_back(m); });

    // LoadFromFileWithDeps 自动加载 CommonLib（metadata.dependencies）
    bool loaded = runner.LoadFromFileWithDeps("assets/Main.bjson");
    ASSERT_TRUE(loaded) << "Failed to load assets/Main.bjson: " << runner.GetLastError();

    RunWithTick(runner);

    // ── ForLoop 0..10 产生 11 条 "Loop Index:N" ──────────────────────────────
    int loopCount = 0;
    for (const auto& l : logs)
        if (l.rfind("Loop Index:", 0) == 0) ++loopCount;
    EXPECT_EQ(loopCount, 11) << "ForLoop 0..10 should print 11 'Loop Index:N' lines";

    // ── Branch: Index==5 时额外打印 ──────────────────────────────────────────
    bool branchHit = false;
    for (const auto& l : logs)
        if (l.find("Index Matched 5") != std::string::npos) { branchHit = true; break; }
    EXPECT_TRUE(branchHit) << "Branch should print 'Index Matched 5' when index==5";

    // ── ForLoop 完成出口 ──────────────────────────────────────────────────────
    bool loopFinished = false;
    for (const auto& l : logs)
        if (l.find("Loop Finished") != std::string::npos) { loopFinished = true; break; }
    EXPECT_TRUE(loopFinished) << "ForLoop.Completed should print 'Loop Finished'";

    // ── Sub Continue: ExecuteBlueprint 完成后回到主流程 ───────────────────────
    bool subContinue = false;
    for (const auto& l : logs)
        if (l.find("Sub Continue") != std::string::npos) { subContinue = true; break; }
    EXPECT_TRUE(subContinue) << "After ExecuteBlueprint, should print 'Sub Continue'";

    // ── FuncLib.TestPrint 打印 Message 参数 ──────────────────────────────────
    bool libMsg = false;
    for (const auto& l : logs)
        if (l.find("test lib") != std::string::npos) { libMsg = true; break; }
    EXPECT_TRUE(libMsg) << "FuncLib.TestPrint should print the Message input 'test lib'";

    // ── FuncLib 返回值正确（TestPrint 返回 MakeLiteralFloat(99)）─────────────
    bool libReturn = false;
    for (const auto& l : logs)
        if (l.find("Lib Return:") != std::string::npos &&
            l.find("99") != std::string::npos) { libReturn = true; break; }
    EXPECT_TRUE(libReturn) << "FuncLib.TestPrint return value should be 99 (got: Lib Return:99.xxx)";
}

// ── 端到端：Sub/Sub.bjson 含 Delay 异步节点，Tick 循环驱动 ────────────────────

TEST(TopologyTest, AsyncDelayInSubBlueprint)
{
    BlueprintRunner runner;
    RegisterBuiltinHandlers(runner, "assets");

    std::vector<std::string> logs;
    runner.SetPrintCallback([&](LogLevel, const std::string& m){ logs.push_back(m); });

    // Sub/Sub.bjson: PrintString → Delay(2s) → PrintString × 2
    bool loaded = runner.LoadFromFile("assets/Sub/Sub.bjson");
    ASSERT_TRUE(loaded) << "Failed to load assets/Sub/Sub.bjson: " << runner.GetLastError();

    // RunWithTick 使用实时时钟，Delay(2s) 完成后 HasPendingAsync() 变 false
    RunWithTick(runner);

    // ── Delay 同步出口（Exec）：立即输出 ─────────────────────────────────────
    bool enterSub = false, delayContinue = false, delayReached = false;
    for (const auto& l : logs) {
        if (l.find("Enter Sub")      != std::string::npos) enterSub      = true;
        if (l.find("Delay Continue") != std::string::npos) delayContinue = true;
        if (l.find("Delay Reached")  != std::string::npos) delayReached  = true;
    }
    EXPECT_TRUE(enterSub)      << "Should print 'Enter Sub' at start";
    EXPECT_TRUE(delayContinue) << "Delay.Exec (sync) should print 'Delay Continue'";
    // Delay(2s).Completed 是异步回调，Tick 循环驱动后应到达
    EXPECT_TRUE(delayReached)  << "Delay.Completed (async, 2s) should print 'Delay Reached' after Tick loop";
}
