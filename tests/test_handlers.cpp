// tests/test_handlers.cpp -- 内置节点处理器逻辑测试
#include <gtest/gtest.h>
#include "BlueprintRunner.h"
#include "BlueprintData.h"
#include "BuiltinNodeDefs.h"
#include "BuiltinHandlers.h"
#include "test_helpers.h"

using namespace NodeEditor::Runtime;
using namespace TestHelpers;

// ── 辅助 ─────────────────────────────────────────────────────────────────────

class HandlersTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        RegisterBuiltinHandlers(runner, ".");
        runner.SetLogCallback([this](LogLevel, const std::string& m){ logs.push_back(m); });
        runner.SetPrintCallback([this](LogLevel, const std::string& m){ logs.push_back(m); });
    }

    // 构建并执行单节点蓝图（inline），走 Tick 循环
    bool loadAndRun(BlueprintData& bp)
    {
        bp.rebuildIndices();
        if (!runner.Load(bp)) return false;
        return RunWithTick(runner).success;
    }

    BlueprintRunner runner;
    std::vector<std::string> logs;
};

// ── Branch 节点 ──────────────────────────────────────────────────────────────

TEST_F(HandlersTest, BranchTrue)
{
    BlueprintData bp;
    bp.metadata.name = "BranchTest";

    auto addPin = [](NodeInstance& n, uint64_t id, PinKind k, bool exec, PinDataType dt = PinDataType::Unknown, const char* name = "") {
        PinInfo p; p.id = id; p.kind = k; p.isExec = exec; p.dataType = dt; p.name = name; n.pins.push_back(p);
    };

    // OnBeginPlay: execOut=1000 → Branch.execIn=10
    AddBeginPlayEntry(bp, 100, 1000, 5000, 10);

    // Node 1: Branch (exec-in=10, condition=11, true-out=12, false-out=13)
    NodeInstance branch;
    branch.id = 1; branch.definitionId = "Branch";
    addPin(branch, 10, PinKind::Input,  true);
    addPin(branch, 11, PinKind::Input,  false, PinDataType::Boolean, "Condition");
    addPin(branch, 12, PinKind::Output, true,  PinDataType::Unknown, "True");
    addPin(branch, 13, PinKind::Output, true,  PinDataType::Unknown, "False");
    branch.pins[1].defaultValue = Variant(true);
    bp.nodes.push_back(branch);

    bp.rebuildIndices();
    ASSERT_TRUE(runner.Load(bp));

    auto result = RunWithBeginPlay(runner);
    EXPECT_TRUE(result.success);
    EXPECT_GE(result.nodesExecuted, 1u) << "Branch should execute via OnBeginPlay";
}

// ── Math: Add ────────────────────────────────────────────────────────────────

TEST_F(HandlersTest, AddIntegers)
{
    // 直接通过 ExecutionContext 测试 Add handler
    BlueprintData bp;
    bp.metadata.name = "AddTest";

    NodeInstance add;
    add.id = 1; add.definitionId = "Add";
    auto addPin = [&](uint64_t id, PinKind k, PinDataType dt, const char* name, Variant dv = {}) {
        PinInfo p; p.id = id; p.kind = k; p.dataType = dt; p.name = name; p.defaultValue = dv; add.pins.push_back(p);
    };
    addPin(10, PinKind::Input,  PinDataType::Integer, "A", Variant(int64_t(3)));
    addPin(11, PinKind::Input,  PinDataType::Integer, "B", Variant(int64_t(4)));
    addPin(12, PinKind::Output, PinDataType::Integer, "Result");
    bp.nodes.push_back(add);

    bp.rebuildIndices();
    ASSERT_TRUE(runner.Load(bp));

    auto result = runner.ExecuteNode(1);
    EXPECT_TRUE(result.success);

    Variant out = runner.GetPinValue(12);
    EXPECT_EQ(out.asInt(), 7) << "3 + 4 should equal 7";
}

// ── Variant 转换：String → Int ───────────────────────────────────────────────

TEST_F(HandlersTest, VariantStringToInt)
{
    Variant v(std::string("42"));
    EXPECT_EQ(v.asInt(), 42);
    EXPECT_DOUBLE_EQ(v.asFloat(), 42.0);
    EXPECT_TRUE(v.asBool());
}

TEST_F(HandlersTest, VariantBoolConversion)
{
    EXPECT_TRUE(Variant(true).asBool());
    EXPECT_FALSE(Variant(false).asBool());
    EXPECT_TRUE(Variant(int64_t(1)).asBool());
    EXPECT_FALSE(Variant(int64_t(0)).asBool());
    EXPECT_FALSE(Variant(std::string("")).asBool());
    EXPECT_TRUE(Variant(std::string("hello")).asBool());
}

// ── SetVariable / GetVariable ────────────────────────────────────────────────

TEST_F(HandlersTest, SetGetVariable)
{
    BlueprintData bp;
    bp.metadata.name = "VarTest";

    VariableDefinition var;
    var.name = "x"; var.dataType = PinDataType::Integer;
    var.defaultValue = Variant(int64_t(0));
    bp.variables.push_back(var);

    bp.rebuildIndices();
    ASSERT_TRUE(runner.Load(bp));

    runner.SetVariable("x", Variant(int64_t(99)));
    Variant v = runner.GetVariable("x");
    EXPECT_EQ(v.asInt(), 99);
}

// ── ForLoop 累加验证 ─────────────────────────────────────────────────────────

TEST_F(HandlersTest, ForLoopAccumulation)
{
    // TestBP.01.bjson: OnBeginPlay → ForLoop 0..4 累加 counter = 0+1+2+3+4 = 10
    BlueprintRunner fr;
    RegisterBuiltinHandlers(fr, "assets");

    bool loaded = fr.LoadFromFile("assets/TestBP.01.bjson");
    ASSERT_TRUE(loaded) << "Failed to load assets/TestBP.01.bjson: " << fr.GetLastError();

    auto result = RunWithBeginPlay(fr);
    EXPECT_TRUE(result.success) << fr.GetLastError();
    EXPECT_GT(result.nodesExecuted, 0u);

    Variant counter = fr.GetVariable("counter");
    EXPECT_NE(counter.type, PinDataType::Unknown) << "Variable 'counter' not found";
    EXPECT_EQ(counter.asInt(), 10) << "counter should be 10 after accumulating 0..4";
}

// ── PrintString 产生日志 ──────────────────────────────────────────────────────

TEST_F(HandlersTest, PrintStringProducesLog)
{
    BlueprintData bp;
    bp.metadata.name = "PrintTest";

    NodeInstance print;
    print.id = 1; print.definitionId = "PrintString";
    PinInfo execIn; execIn.id = 10; execIn.kind = PinKind::Input; execIn.isExec = true;
    PinInfo strIn;  strIn.id  = 11; strIn.kind  = PinKind::Input; strIn.dataType = PinDataType::String;
    strIn.name = "In String"; strIn.defaultValue = Variant(std::string("HELLO_TEST"));
    print.pins.push_back(execIn);
    print.pins.push_back(strIn);
    bp.nodes.push_back(print);

    bp.rebuildIndices();
    ASSERT_TRUE(runner.Load(bp));

    runner.ExecuteNode(1);

    bool found = false;
    for (const auto& l : logs)
        if (l.find("HELLO_TEST") != std::string::npos) { found = true; break; }
    EXPECT_TRUE(found) << "PrintString should produce a log containing 'HELLO_TEST'";
}

