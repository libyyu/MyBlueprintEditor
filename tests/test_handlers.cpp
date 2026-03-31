// tests/test_handlers.cpp -- 内置节点处理器逻辑测试
#include <gtest/gtest.h>
#include "BlueprintRunner.h"
#include "BlueprintData.h"
#include "BuiltinNodeDefs.h"
#include "BuiltinHandlers.h"

using namespace NodeEditor::Runtime;

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

    // 构建并执行单节点蓝图（inline）
    bool loadAndRun(BlueprintData& bp)
    {
        bp.rebuildIndices();
        return runner.Load(bp) && runner.Execute().success;
    }

    BlueprintRunner runner;
    std::vector<std::string> logs;
};

// ── Branch 节点 ──────────────────────────────────────────────────────────────

TEST_F(HandlersTest, BranchTrue)
{
    BlueprintData bp;
    bp.metadata.name = "BranchTest";

    // Node 1: Branch (exec-in=10, condition=11, true-out=12, false-out=13)
    NodeInstance branch;
    branch.id = 1; branch.definitionId = "Branch";
    auto addPin = [&](NodeInstance& n, uint64_t id, PinKind k, bool exec, PinDataType dt = PinDataType::Unknown, const char* name = "") {
        PinInfo p; p.id = id; p.kind = k; p.isExec = exec; p.dataType = dt; p.name = name; n.pins.push_back(p);
    };
    addPin(branch, 10, PinKind::Input,  true);   // exec in
    addPin(branch, 11, PinKind::Input,  false, PinDataType::Boolean, "Condition");
    addPin(branch, 12, PinKind::Output, true,  PinDataType::Unknown, "True");
    addPin(branch, 13, PinKind::Output, true,  PinDataType::Unknown, "False");
    branch.pins[1].defaultValue = Variant(true);  // Condition = true
    bp.nodes.push_back(branch);

    bp.rebuildIndices();
    ASSERT_TRUE(runner.Load(bp));

    auto result = runner.Execute();
    EXPECT_TRUE(result.success);
    // Branch 节点本身被执行
    EXPECT_GE(result.nodesExecuted, 1u);
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
    // 若测试 JSON 文件存在则执行端到端测试
    bool loaded = runner.LoadFromFile("TestBP.01.bjson");
    if (!loaded)
    {
        GTEST_SKIP() << "TestBP.01.bjson not found in working directory";
        return;
    }

    auto result = runner.Execute();
    EXPECT_TRUE(result.success) << runner.GetLastError();

    // TestBP.01 执行 ForLoop 0..4 累加 → counter = 10
    Variant counter = runner.GetVariable("counter");
    if (counter.type != PinDataType::Unknown)
        EXPECT_EQ(counter.asInt(), 10);
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

