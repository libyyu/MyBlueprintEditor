// tests/test_topology.cpp -- 拓扑排序 / 执行顺序测试
// buildTopologicalOrder 是 BlueprintRunner 的私有方法，通过执行结果间接验证顺序正确性
#include <gtest/gtest.h>
#include "BlueprintRunner.h"
#include "BlueprintData.h"
#include "BuiltinHandlers.h"

using namespace NodeEditor::Runtime;

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

    auto result = runner.Execute();
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

    std::vector<std::string> logs;
    runner.SetLogCallback([&](LogLevel, const std::string& m){ logs.push_back(m); });

    ASSERT_TRUE(runner.Load(bp));
    auto result = runner.Execute();
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
    auto result = runner.Execute();
    EXPECT_TRUE(result.success);
}

// ── 端到端：ForLoop 累加（从 JSON 文件）────────────────────────────────────────

TEST(TopologyTest, ForLoopExecution)
{
    BlueprintRunner runner;
    RegisterBuiltinHandlers(runner, "assets");

    // 收集 Print 输出
    std::vector<std::string> logs;
    runner.SetPrintCallback([&](LogLevel, const std::string& m){ logs.push_back(m); });

    // Main.bjson: ForLoop 0..10 → PrintString each index, then Branch/PrintString/ExecuteBlueprint
    bool loaded = runner.LoadFromFile("assets/Main.bjson");
    ASSERT_TRUE(loaded) << "Failed to load assets/Main.bjson: " << runner.GetLastError();

    auto result = runner.Execute();
    EXPECT_TRUE(result.success) << runner.GetLastError();
    EXPECT_GT(result.nodesExecuted, 0u);

    // ForLoop 0..10 prints 11 lines ("Loop Index: 0" .. "Loop Index: 10")
    // plus "Index Matched 5" (when index==5) and "Loop Finished"
    EXPECT_GE(logs.size(), 11u) << "Expected at least 11 log entries from ForLoop";
}

