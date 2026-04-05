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


// ── FromJSON: 嵌套对象 ────────────────────────────────────────────────────────

TEST_F(HandlersTest, FromJSON_NestedObject)
{
    BlueprintData bp;
    bp.metadata.name = "FromJSONNestedTest";

    NodeInstance node;
    node.id = 1; node.definitionId = "FromJSON";
    auto addPin = [&](uint64_t id, PinKind k, PinDataType dt, const char* name, Variant dv = {}) {
        PinInfo p; p.id = id; p.kind = k; p.dataType = dt; p.name = name; p.defaultValue = dv; node.pins.push_back(p);
    };
    addPin(10, PinKind::Input,  PinDataType::String, "JSON",
           Variant(std::string(R"({"name":"Alice","score":42,"active":true})")));
    addPin(11, PinKind::Output, PinDataType::Any,    "Value");
    addPin(12, PinKind::Output, PinDataType::Boolean,"Valid");
    bp.nodes.push_back(node);
    bp.rebuildIndices();
    ASSERT_TRUE(runner.Load(bp));

    auto result = runner.ExecuteNode(1);
    EXPECT_TRUE(result.success);
    EXPECT_TRUE(runner.GetPinValue(12).asBool()) << "Valid should be true for valid JSON";
    // Value 应该是 Map 类型
    Variant val = runner.GetPinValue(11);
    EXPECT_EQ(val.type, PinDataType::Map) << "Parsed object should be Map type";
}

// ── FromJSON: 数组 ────────────────────────────────────────────────────────────

TEST_F(HandlersTest, FromJSON_Array)
{
    BlueprintData bp;
    bp.metadata.name = "FromJSONArrayTest";

    NodeInstance node;
    node.id = 1; node.definitionId = "FromJSON";
    auto addPin = [&](uint64_t id, PinKind k, PinDataType dt, const char* name, Variant dv = {}) {
        PinInfo p; p.id = id; p.kind = k; p.dataType = dt; p.name = name; p.defaultValue = dv; node.pins.push_back(p);
    };
    addPin(10, PinKind::Input,  PinDataType::String, "JSON",
           Variant(std::string(R"([1, 2, 3])")));
    addPin(11, PinKind::Output, PinDataType::Any,    "Value");
    addPin(12, PinKind::Output, PinDataType::Boolean,"Valid");
    bp.nodes.push_back(node);
    bp.rebuildIndices();
    ASSERT_TRUE(runner.Load(bp));

    auto result = runner.ExecuteNode(1);
    EXPECT_TRUE(result.success);
    EXPECT_TRUE(runner.GetPinValue(12).asBool());
    Variant val = runner.GetPinValue(11);
    EXPECT_EQ(val.type, PinDataType::Array) << "Parsed array should be Array type";
    EXPECT_EQ(val.arraySize(), 3u);
    EXPECT_EQ(val.arrayGet(1).asInt(), 2);
}

// ── GetField: 嵌套对象字段 ────────────────────────────────────────────────────

TEST_F(HandlersTest, GetField_NestedObject)
{
    BlueprintData bp;
    bp.metadata.name = "GetFieldTest";

    NodeInstance node;
    node.id = 1; node.definitionId = "GetField";
    auto addPin = [&](uint64_t id, PinKind k, PinDataType dt, const char* name, Variant dv = {}) {
        PinInfo p; p.id = id; p.kind = k; p.dataType = dt; p.name = name; p.defaultValue = dv; node.pins.push_back(p);
    };
    addPin(10, PinKind::Input,  PinDataType::String, "JSON",
           Variant(std::string(R"({"choices":[{"message":{"content":"Hello AI"}}]})")));
    addPin(11, PinKind::Input,  PinDataType::String, "Key",
           Variant(std::string("choices")));
    addPin(12, PinKind::Output, PinDataType::String, "Value");
    addPin(13, PinKind::Output, PinDataType::Boolean,"Found");
    bp.nodes.push_back(node);
    bp.rebuildIndices();
    ASSERT_TRUE(runner.Load(bp));

    auto result = runner.ExecuteNode(1);
    EXPECT_TRUE(result.success);
    EXPECT_TRUE(runner.GetPinValue(13).asBool()) << "choices key should be found";
    // GetField 返回顶层字段，choices 是数组，Value 应被序列化为 JSON 字符串
    std::string val = runner.GetPinValue(12).asString();
    EXPECT_FALSE(val.empty()) << "choices value should not be empty";
}

// ── JSON.GetPath: 深层路径 ────────────────────────────────────────────────────

TEST_F(HandlersTest, JSONGetPath_DeepPath)
{
    BlueprintData bp;
    bp.metadata.name = "JSONGetPathTest";

    NodeInstance node;
    node.id = 1; node.definitionId = "JSON.GetPath";
    auto addPin = [&](uint64_t id, PinKind k, PinDataType dt, const char* name, Variant dv = {}) {
        PinInfo p; p.id = id; p.kind = k; p.dataType = dt; p.name = name; p.defaultValue = dv; node.pins.push_back(p);
    };
    addPin(10, PinKind::Input,  PinDataType::String, "JSON",
           Variant(std::string(R"({"choices":[{"message":{"content":"Hello AI"}}]})")));
    addPin(11, PinKind::Input,  PinDataType::String, "Path",
           Variant(std::string("choices[0].message.content")));
    addPin(12, PinKind::Output, PinDataType::String, "Value");
    addPin(13, PinKind::Output, PinDataType::Boolean,"Found");
    bp.nodes.push_back(node);
    bp.rebuildIndices();
    ASSERT_TRUE(runner.Load(bp));

    auto result = runner.ExecuteNode(1);
    EXPECT_TRUE(result.success);
    EXPECT_TRUE(runner.GetPinValue(13).asBool()) << "Path should be found";
    EXPECT_EQ(runner.GetPinValue(12).asString(), "Hello AI")
        << "choices[0].message.content should equal 'Hello AI'";
}

// ============================================================================
// AI 节点测试
// ============================================================================

// JSON.Build
TEST_F(HandlersTest, JSONBuild_BasicObject)
{
    BlueprintData bp;
    bp.metadata.name = "JSONBuildTest";
    NodeInstance node;
    node.id = 1; node.definitionId = "JSON.Build";
    auto addPin = [&](uint64_t id, PinKind k, PinDataType dt, const char* name, Variant dv = {}) {
        PinInfo p; p.id = id; p.kind = k; p.dataType = dt; p.name = name; p.defaultValue = dv;
        node.pins.push_back(p);
    };
    // Keys = ["model", "temperature"]
    std::vector<Variant> keys = { Variant(std::string("model")), Variant(std::string("temperature")) };
    // Values = ["gpt-4o", 0.7]
    std::vector<Variant> vals = { Variant(std::string("gpt-4o")), Variant(0.7) };
    addPin(10, PinKind::Input,  PinDataType::Array, "Keys",   Variant(keys));
    addPin(11, PinKind::Input,  PinDataType::Array, "Values", Variant(vals));
    addPin(12, PinKind::Output, PinDataType::String,"JSON");
    bp.nodes.push_back(node);
    bp.rebuildIndices();
    ASSERT_TRUE(runner.Load(bp));

    auto result = runner.ExecuteNode(1);
    EXPECT_TRUE(result.success);
    std::string json = runner.GetPinValue(12).asString();
    EXPECT_FALSE(json.empty()) << "JSON.Build should produce non-empty JSON";
    EXPECT_NE(json.find("gpt-4o"), std::string::npos) << "JSON should contain model value";
    EXPECT_NE(json.find("model"),  std::string::npos) << "JSON should contain 'model' key";
}

// JSON.SetPath
TEST_F(HandlersTest, JSONSetPath_CreateNestedField)
{
    BlueprintData bp;
    bp.metadata.name = "JSONSetPathTest";
    NodeInstance node;
    node.id = 1; node.definitionId = "JSON.SetPath";
    auto addPin = [&](uint64_t id, PinKind k, PinDataType dt, const char* name, Variant dv = {}) {
        PinInfo p; p.id = id; p.kind = k; p.dataType = dt; p.name = name; p.defaultValue = dv;
        node.pins.push_back(p);
    };
    addPin(10, PinKind::Input,  PinDataType::String, "JSON",  Variant(std::string("{}")));
    addPin(11, PinKind::Input,  PinDataType::String, "Path",  Variant(std::string("message.content")));
    addPin(12, PinKind::Input,  PinDataType::Any,    "Value", Variant(std::string("Hello")));
    addPin(13, PinKind::Output, PinDataType::String, "JSON");
    bp.nodes.push_back(node);
    bp.rebuildIndices();
    ASSERT_TRUE(runner.Load(bp));

    auto result = runner.ExecuteNode(1);
    EXPECT_TRUE(result.success);
    std::string out = runner.GetPinValue(13).asString();
    EXPECT_NE(out.find("Hello"),   std::string::npos) << "JSON.SetPath should set value";
    EXPECT_NE(out.find("content"), std::string::npos) << "JSON.SetPath should create nested key";
}

// JSON.ArrayPush
TEST_F(HandlersTest, JSONArrayPush_AppendsElement)
{
    BlueprintData bp;
    bp.metadata.name = "JSONArrayPushTest";
    NodeInstance node;
    node.id = 1; node.definitionId = "JSON.ArrayPush";
    auto addPin = [&](uint64_t id, PinKind k, PinDataType dt, const char* name, Variant dv = {}) {
        PinInfo p; p.id = id; p.kind = k; p.dataType = dt; p.name = name; p.defaultValue = dv;
        node.pins.push_back(p);
    };
    // exec flow pins (required for exec nodes)
    { PinInfo p; p.id = 9;  p.kind = PinKind::Input;  p.isExec = true;  p.name = ""; node.pins.push_back(p); }
    { PinInfo p; p.id = 99; p.kind = PinKind::Output; p.isExec = true;  p.name = ""; node.pins.push_back(p); }
    addPin(10, PinKind::Input,  PinDataType::String,  "JSON",    Variant(std::string("[\"a\"]")));
    addPin(11, PinKind::Input,  PinDataType::Any,     "Element", Variant(std::string("b")));
    addPin(12, PinKind::Output, PinDataType::String,  "JSON");
    addPin(13, PinKind::Output, PinDataType::Integer, "Length");
    bp.nodes.push_back(node);
    bp.rebuildIndices();
    ASSERT_TRUE(runner.Load(bp));

    auto result = runner.ExecuteNode(1);
    EXPECT_TRUE(result.success);
    EXPECT_EQ(runner.GetPinValue(13).asInt(), 2) << "Length should be 2 after push";
    std::string out = runner.GetPinValue(12).asString();
    EXPECT_NE(out.find("\"b\""), std::string::npos) << "Pushed element should appear in JSON";
}

// JSON.MakeMessage
TEST_F(HandlersTest, JSONMakeMessage_BuildsChatMessage)
{
    BlueprintData bp;
    bp.metadata.name = "JSONMakeMessageTest";
    NodeInstance node;
    node.id = 1; node.definitionId = "JSON.MakeMessage";
    auto addPin = [&](uint64_t id, PinKind k, PinDataType dt, const char* name, Variant dv = {}) {
        PinInfo p; p.id = id; p.kind = k; p.dataType = dt; p.name = name; p.defaultValue = dv;
        node.pins.push_back(p);
    };
    addPin(10, PinKind::Input,  PinDataType::String, "Role",    Variant(std::string("user")));
    addPin(11, PinKind::Input,  PinDataType::String, "Content", Variant(std::string("Hello AI")));
    addPin(12, PinKind::Output, PinDataType::String, "JSON");
    bp.nodes.push_back(node);
    bp.rebuildIndices();
    ASSERT_TRUE(runner.Load(bp));

    auto result = runner.ExecuteNode(1);
    EXPECT_TRUE(result.success);
    std::string out = runner.GetPinValue(12).asString();
    EXPECT_NE(out.find("\"user\""),     std::string::npos) << "role should be 'user'";
    EXPECT_NE(out.find("\"Hello AI\""), std::string::npos) << "content should be 'Hello AI'";
}

// String.Template
TEST_F(HandlersTest, StringTemplate_ReplacesPlaceholders)
{
    BlueprintData bp;
    bp.metadata.name = "StringTemplateTest";
    NodeInstance node;
    node.id = 1; node.definitionId = "String.Template";
    auto addPin = [&](uint64_t id, PinKind k, PinDataType dt, const char* name, Variant dv = {}) {
        PinInfo p; p.id = id; p.kind = k; p.dataType = dt; p.name = name; p.defaultValue = dv;
        node.pins.push_back(p);
    };
    addPin(10, PinKind::Input,  PinDataType::String, "Template",
           Variant(std::string("Hello {{name}}, today is {{day}}!")));
    std::vector<Variant> keys = { Variant(std::string("name")), Variant(std::string("day")) };
    std::vector<Variant> vals = { Variant(std::string("Alice")), Variant(std::string("Monday")) };
    addPin(11, PinKind::Input,  PinDataType::Array,  "Keys",   Variant(keys));
    addPin(12, PinKind::Input,  PinDataType::Array,  "Values", Variant(vals));
    addPin(13, PinKind::Output, PinDataType::String, "Result");
    bp.nodes.push_back(node);
    bp.rebuildIndices();
    ASSERT_TRUE(runner.Load(bp));

    auto result = runner.ExecuteNode(1);
    EXPECT_TRUE(result.success);
    EXPECT_EQ(runner.GetPinValue(13).asString(), "Hello Alice, today is Monday!");
}

// ============================================================================
// JSON.ParseToolCall
// ============================================================================
TEST_F(HandlersTest, ParseToolCall_ExtractsNameAndArgs)
{
    const std::string toolCallsJson = R"([
        {
            "id": "call_abc123",
            "type": "function",
            "function": {
                "name": "get_weather",
                "arguments": "{\"location\":\"Beijing\",\"unit\":\"celsius\"}"
            }
        }
    ])";

    BlueprintData bp;
    bp.metadata.name = "ParseToolCallTest";
    NodeInstance node;
    node.id = 1; node.definitionId = "JSON.ParseToolCall";
    auto addPin = [&](uint64_t id, PinKind k, PinDataType dt, const char* name, Variant dv = {}) {
        PinInfo p; p.id = id; p.kind = k; p.dataType = dt; p.name = name; p.defaultValue = dv;
        node.pins.push_back(p);
    };
    addPin(10, PinKind::Input,  PinDataType::String,  "ToolCallsJSON", Variant(toolCallsJson));
    addPin(11, PinKind::Input,  PinDataType::Integer, "Index",         Variant((int64_t)0));
    addPin(12, PinKind::Output, PinDataType::String,  "Name");
    addPin(13, PinKind::Output, PinDataType::String,  "ArgumentsJSON");
    addPin(14, PinKind::Output, PinDataType::String,  "ID");
    bp.nodes.push_back(node);
    bp.rebuildIndices();
    ASSERT_TRUE(runner.Load(bp));

    auto result = runner.ExecuteNode(1);
    EXPECT_TRUE(result.success);
    EXPECT_EQ(runner.GetPinValue(12).asString(), "get_weather");
    EXPECT_NE(runner.GetPinValue(13).asString().find("Beijing"), std::string::npos);
    EXPECT_EQ(runner.GetPinValue(14).asString(), "call_abc123");
}

TEST_F(HandlersTest, ParseToolCall_OutOfBoundsReturnsEmpty)
{
    BlueprintData bp;
    bp.metadata.name = "ParseToolCallOOB";
    NodeInstance node;
    node.id = 1; node.definitionId = "JSON.ParseToolCall";
    auto addPin = [&](uint64_t id, PinKind k, PinDataType dt, const char* name, Variant dv = {}) {
        PinInfo p; p.id = id; p.kind = k; p.dataType = dt; p.name = name; p.defaultValue = dv;
        node.pins.push_back(p);
    };
    addPin(10, PinKind::Input,  PinDataType::String,  "ToolCallsJSON", Variant(std::string("[]")));
    addPin(11, PinKind::Input,  PinDataType::Integer, "Index",         Variant((int64_t)0));
    addPin(12, PinKind::Output, PinDataType::String,  "Name");
    addPin(13, PinKind::Output, PinDataType::String,  "ArgumentsJSON");
    addPin(14, PinKind::Output, PinDataType::String,  "ID");
    bp.nodes.push_back(node);
    bp.rebuildIndices();
    ASSERT_TRUE(runner.Load(bp));

    auto result = runner.ExecuteNode(1);
    EXPECT_TRUE(result.success);
    EXPECT_EQ(runner.GetPinValue(12).asString(), "");
    EXPECT_EQ(runner.GetPinValue(13).asString(), "{}");
}

// ============================================================================
// JSON.MakeToolResult
// ============================================================================
TEST_F(HandlersTest, MakeToolResult_BuildsCorrectJSON)
{
    BlueprintData bp;
    bp.metadata.name = "MakeToolResultTest";
    NodeInstance node;
    node.id = 1; node.definitionId = "JSON.MakeToolResult";
    auto addPin = [&](uint64_t id, PinKind k, PinDataType dt, const char* name, Variant dv = {}) {
        PinInfo p; p.id = id; p.kind = k; p.dataType = dt; p.name = name; p.defaultValue = dv;
        node.pins.push_back(p);
    };
    addPin(10, PinKind::Input,  PinDataType::String, "ToolCallID", Variant(std::string("call_abc123")));
    addPin(11, PinKind::Input,  PinDataType::String, "Content",    Variant(std::string("28°C, sunny")));
    addPin(12, PinKind::Output, PinDataType::String, "JSON");
    bp.nodes.push_back(node);
    bp.rebuildIndices();
    ASSERT_TRUE(runner.Load(bp));

    auto result = runner.ExecuteNode(1);
    EXPECT_TRUE(result.success);
    const std::string out = runner.GetPinValue(12).asString();
    EXPECT_NE(out.find("\"tool\""),        std::string::npos) << "role should be 'tool'";
    EXPECT_NE(out.find("\"call_abc123\""), std::string::npos) << "tool_call_id mismatch";
    EXPECT_NE(out.find("28"),              std::string::npos) << "content mismatch";
}

// ============================================================================
// Memory.LoadHistory / Memory.SaveHistory
// ============================================================================
#include <fstream>
#include <filesystem>
#include <cstdio>

namespace {
// 生成一个进程内唯一的临时文件路径（不创建文件）
std::string TempHistoryPath(const char* suffix = "") {
    std::filesystem::path tmp = std::filesystem::temp_directory_path();
    return (tmp / ("bp_test_history_" + std::to_string(
        std::hash<std::string>{}(std::string(suffix) +
            std::to_string(reinterpret_cast<uintptr_t>(&suffix)))) + ".json")).string();
}
} // namespace

// ── SaveHistory ──────────────────────────────────────────────────────────────
TEST_F(HandlersTest, MemorySaveHistory_WritesFile)
{
    std::string path = TempHistoryPath("save_basic");
    std::filesystem::remove(path); // 确保不存在

    BlueprintData bp; bp.metadata.name = "MemSaveTest";
    NodeInstance node; node.id = 1; node.definitionId = "Memory.SaveHistory";
    auto addPin = [&](uint64_t id, PinKind k, PinDataType dt, const char* name, Variant dv = {}) {
        PinInfo p; p.id=id; p.kind=k; p.dataType=dt; p.name=name; p.defaultValue=dv;
        node.pins.push_back(p);
    };
    const std::string msgs = R"([{"role":"user","content":"hi"}])";
    addPin(1, PinKind::Input,  PinDataType::Any,     "",            {});
    addPin(2, PinKind::Input,  PinDataType::String,  "Path",        Variant(path));
    addPin(3, PinKind::Input,  PinDataType::String,  "Messages",    Variant(msgs));
    addPin(4, PinKind::Input,  PinDataType::Integer, "MaxMessages", Variant((int64_t)0));
    addPin(5, PinKind::Output, PinDataType::Any,     "onSuccess",   {});
    addPin(6, PinKind::Output, PinDataType::Any,     "onError",     {});
    addPin(7, PinKind::Output, PinDataType::String,  "ErrorMessage",{});
    bp.nodes.push_back(node); bp.rebuildIndices();
    ASSERT_TRUE(runner.Load(bp));

    auto res = runner.ExecuteNode(1);
    EXPECT_TRUE(res.success);
    EXPECT_EQ(runner.GetPinValue(7).asString(), ""); // no error

    // 验证文件内容（ifstream 放独立作用域，确保析构后文件锁释放再 remove）
    {
        std::ifstream f(path);
        ASSERT_TRUE(f.is_open()) << "file should have been created";
        std::string content((std::istreambuf_iterator<char>(f)), {});
        EXPECT_NE(content.find("user"), std::string::npos);
        EXPECT_NE(content.find("hi"),   std::string::npos);
    } // f 析构，Windows 文件锁释放
    std::filesystem::remove(path);
}

TEST_F(HandlersTest, MemorySaveHistory_TrimsToMaxMessages)
{
    std::string path = TempHistoryPath("save_trim");
    std::filesystem::remove(path);

    // 5 条消息，MaxMessages=2，期望只保留最后 2 条
    std::string msgs = R"([
        {"role":"user","content":"1"},
        {"role":"assistant","content":"2"},
        {"role":"user","content":"3"},
        {"role":"assistant","content":"4"},
        {"role":"user","content":"5"}
    ])";

    BlueprintData bp; bp.metadata.name = "MemSaveTrimTest";
    NodeInstance node; node.id = 1; node.definitionId = "Memory.SaveHistory";
    auto addPin = [&](uint64_t id, PinKind k, PinDataType dt, const char* name, Variant dv = {}) {
        PinInfo p; p.id=id; p.kind=k; p.dataType=dt; p.name=name; p.defaultValue=dv;
        node.pins.push_back(p);
    };
    addPin(1, PinKind::Input,  PinDataType::Any,     "",            {});
    addPin(2, PinKind::Input,  PinDataType::String,  "Path",        Variant(path));
    addPin(3, PinKind::Input,  PinDataType::String,  "Messages",    Variant(msgs));
    addPin(4, PinKind::Input,  PinDataType::Integer, "MaxMessages", Variant((int64_t)2));
    addPin(5, PinKind::Output, PinDataType::Any,     "onSuccess",   {});
    addPin(6, PinKind::Output, PinDataType::Any,     "onError",     {});
    addPin(7, PinKind::Output, PinDataType::String,  "ErrorMessage",{});
    bp.nodes.push_back(node); bp.rebuildIndices();
    ASSERT_TRUE(runner.Load(bp));

    auto res = runner.ExecuteNode(1);
    EXPECT_TRUE(res.success);

    // 解析写入的文件，期望恰好 2 条
    {
        std::ifstream f(path);
        ASSERT_TRUE(f.is_open());
        std::string content((std::istreambuf_iterator<char>(f)), {});
        // 最后两条是 "4" 和 "5"
        EXPECT_NE(content.find("\"4\""), std::string::npos) << "should contain msg 4";
        EXPECT_NE(content.find("\"5\""), std::string::npos) << "should contain msg 5";
        EXPECT_EQ(content.find("\"1\""), std::string::npos) << "msg 1 should be trimmed";
    } // f 析构，Windows 文件锁释放
    std::filesystem::remove(path);
}

// ── LoadHistory ──────────────────────────────────────────────────────────────
TEST_F(HandlersTest, MemoryLoadHistory_ReturnsOnNewWhenFileMissing)
{
    std::string path = TempHistoryPath("load_missing");
    std::filesystem::remove(path); // 确保不存在

    BlueprintData bp; bp.metadata.name = "MemLoadMissingTest";
    NodeInstance node; node.id = 1; node.definitionId = "Memory.LoadHistory";
    auto addPin = [&](uint64_t id, PinKind k, PinDataType dt, const char* name, Variant dv = {}) {
        PinInfo p; p.id=id; p.kind=k; p.dataType=dt; p.name=name; p.defaultValue=dv;
        node.pins.push_back(p);
    };
    addPin(1, PinKind::Input,  PinDataType::Any,     "In",          {});
    addPin(2, PinKind::Input,  PinDataType::String,  "Path",        Variant(path));
    addPin(3, PinKind::Input,  PinDataType::Integer, "MaxMessages", Variant((int64_t)0));
    addPin(4, PinKind::Output, PinDataType::Any,     "onSuccess",   {});
    addPin(5, PinKind::Output, PinDataType::Any,     "onNew",       {});
    addPin(6, PinKind::Output, PinDataType::Any,     "onError",     {});
    addPin(7, PinKind::Output, PinDataType::String,  "Messages",    {});
    addPin(8, PinKind::Output, PinDataType::Integer, "Count",       {});
    addPin(9, PinKind::Output, PinDataType::String,  "ErrorMessage",{});
    bp.nodes.push_back(node); bp.rebuildIndices();
    ASSERT_TRUE(runner.Load(bp));

    auto res = runner.ExecuteNode(1);
    EXPECT_TRUE(res.success);
    EXPECT_EQ(runner.GetPinValue(7).asString(), "[]");
    EXPECT_EQ(runner.GetPinValue(8).asInt(), 0);
}

TEST_F(HandlersTest, MemoryLoadHistory_LoadsExistingFile)
{
    std::string path = TempHistoryPath("load_existing");
    // 先写一个历史文件
    {
        std::ofstream f(path);
        f << R"([{"role":"user","content":"hello"},{"role":"assistant","content":"hi there"}])";
    }

    BlueprintData bp; bp.metadata.name = "MemLoadExistTest";
    NodeInstance node; node.id = 1; node.definitionId = "Memory.LoadHistory";
    auto addPin = [&](uint64_t id, PinKind k, PinDataType dt, const char* name, Variant dv = {}) {
        PinInfo p; p.id=id; p.kind=k; p.dataType=dt; p.name=name; p.defaultValue=dv;
        node.pins.push_back(p);
    };
    addPin(1, PinKind::Input,  PinDataType::Any,     "In",          {});
    addPin(2, PinKind::Input,  PinDataType::String,  "Path",        Variant(path));
    addPin(3, PinKind::Input,  PinDataType::Integer, "MaxMessages", Variant((int64_t)0));
    addPin(4, PinKind::Output, PinDataType::Any,     "onSuccess",   {});
    addPin(5, PinKind::Output, PinDataType::Any,     "onNew",       {});
    addPin(6, PinKind::Output, PinDataType::Any,     "onError",     {});
    addPin(7, PinKind::Output, PinDataType::String,  "Messages",    {});
    addPin(8, PinKind::Output, PinDataType::Integer, "Count",       {});
    addPin(9, PinKind::Output, PinDataType::String,  "ErrorMessage",{});
    bp.nodes.push_back(node); bp.rebuildIndices();
    ASSERT_TRUE(runner.Load(bp));

    auto res = runner.ExecuteNode(1);
    EXPECT_TRUE(res.success);
    EXPECT_EQ(runner.GetPinValue(8).asInt(), 2);
    EXPECT_NE(runner.GetPinValue(7).asString().find("hello"), std::string::npos);

    std::filesystem::remove(path);
}

TEST_F(HandlersTest, MemoryLoadHistory_TrimsToMaxMessages)
{
    std::string path = TempHistoryPath("load_trim");
    {
        std::ofstream f(path);
        f << R"([
            {"role":"user","content":"A"},
            {"role":"assistant","content":"B"},
            {"role":"user","content":"C"},
            {"role":"assistant","content":"D"}
        ])";
    }

    BlueprintData bp; bp.metadata.name = "MemLoadTrimTest";
    NodeInstance node; node.id = 1; node.definitionId = "Memory.LoadHistory";
    auto addPin = [&](uint64_t id, PinKind k, PinDataType dt, const char* name, Variant dv = {}) {
        PinInfo p; p.id=id; p.kind=k; p.dataType=dt; p.name=name; p.defaultValue=dv;
        node.pins.push_back(p);
    };
    addPin(1, PinKind::Input,  PinDataType::Any,     "In",          {});
    addPin(2, PinKind::Input,  PinDataType::String,  "Path",        Variant(path));
    addPin(3, PinKind::Input,  PinDataType::Integer, "MaxMessages", Variant((int64_t)2));
    addPin(4, PinKind::Output, PinDataType::Any,     "onSuccess",   {});
    addPin(5, PinKind::Output, PinDataType::Any,     "onNew",       {});
    addPin(6, PinKind::Output, PinDataType::Any,     "onError",     {});
    addPin(7, PinKind::Output, PinDataType::String,  "Messages",    {});
    addPin(8, PinKind::Output, PinDataType::Integer, "Count",       {});
    addPin(9, PinKind::Output, PinDataType::String,  "ErrorMessage",{});
    bp.nodes.push_back(node); bp.rebuildIndices();
    ASSERT_TRUE(runner.Load(bp));

    auto res = runner.ExecuteNode(1);
    EXPECT_TRUE(res.success);
    EXPECT_EQ(runner.GetPinValue(8).asInt(), 2);                              // 截断到 2
    EXPECT_NE(runner.GetPinValue(7).asString().find("\"C\""), std::string::npos); // 保留最近
    EXPECT_NE(runner.GetPinValue(7).asString().find("\"D\""), std::string::npos);
    EXPECT_EQ(runner.GetPinValue(7).asString().find("\"A\""), std::string::npos); // 最旧被丢弃

    std::filesystem::remove(path);
}

TEST_F(HandlersTest, MemoryLoadHistory_CorruptFileReturnsOnNew)
{
    std::string path = TempHistoryPath("load_corrupt");
    { std::ofstream f(path); f << "not valid json {{{{"; }

    BlueprintData bp; bp.metadata.name = "MemLoadCorruptTest";
    NodeInstance node; node.id = 1; node.definitionId = "Memory.LoadHistory";
    auto addPin = [&](uint64_t id, PinKind k, PinDataType dt, const char* name, Variant dv = {}) {
        PinInfo p; p.id=id; p.kind=k; p.dataType=dt; p.name=name; p.defaultValue=dv;
        node.pins.push_back(p);
    };
    addPin(1, PinKind::Input,  PinDataType::Any,     "In",          {});
    addPin(2, PinKind::Input,  PinDataType::String,  "Path",        Variant(path));
    addPin(3, PinKind::Input,  PinDataType::Integer, "MaxMessages", Variant((int64_t)0));
    addPin(4, PinKind::Output, PinDataType::Any,     "onSuccess",   {});
    addPin(5, PinKind::Output, PinDataType::Any,     "onNew",       {});
    addPin(6, PinKind::Output, PinDataType::Any,     "onError",     {});
    addPin(7, PinKind::Output, PinDataType::String,  "Messages",    {});
    addPin(8, PinKind::Output, PinDataType::Integer, "Count",       {});
    addPin(9, PinKind::Output, PinDataType::String,  "ErrorMessage",{});
    bp.nodes.push_back(node); bp.rebuildIndices();
    ASSERT_TRUE(runner.Load(bp));

    auto res = runner.ExecuteNode(1);
    EXPECT_TRUE(res.success);
    EXPECT_EQ(runner.GetPinValue(7).asString(), "[]"); // 降级为空数组

    std::filesystem::remove(path);
}

// ── RoundTrip ────────────────────────────────────────────────────────────────
TEST_F(HandlersTest, Memory_RoundTrip_SaveThenLoad)
{
    std::string path = TempHistoryPath("roundtrip");
    std::filesystem::remove(path);

    const std::string msgs = R"([{"role":"user","content":"ping"},{"role":"assistant","content":"pong"}])";

    // Save
    {
        BlueprintData bp; bp.metadata.name = "SaveRT";
        NodeInstance node; node.id = 1; node.definitionId = "Memory.SaveHistory";
        auto addPin = [&](uint64_t id, PinKind k, PinDataType dt, const char* name, Variant dv = {}) {
            PinInfo p; p.id=id; p.kind=k; p.dataType=dt; p.name=name; p.defaultValue=dv;
            node.pins.push_back(p);
        };
        addPin(1, PinKind::Input,  PinDataType::Any,     "",            {});
        addPin(2, PinKind::Input,  PinDataType::String,  "Path",        Variant(path));
        addPin(3, PinKind::Input,  PinDataType::String,  "Messages",    Variant(msgs));
        addPin(4, PinKind::Input,  PinDataType::Integer, "MaxMessages", Variant((int64_t)0));
        addPin(5, PinKind::Output, PinDataType::Any,     "onSuccess",   {});
        addPin(6, PinKind::Output, PinDataType::Any,     "onError",     {});
        addPin(7, PinKind::Output, PinDataType::String,  "ErrorMessage",{});
        bp.nodes.push_back(node); bp.rebuildIndices();
        BlueprintRunner saver; RegisterBuiltinHandlers(saver, ".");
        ASSERT_TRUE(saver.Load(bp));
        EXPECT_TRUE(saver.ExecuteNode(1).success);
    }

    // Load
    {
        BlueprintData bp; bp.metadata.name = "LoadRT";
        NodeInstance node; node.id = 1; node.definitionId = "Memory.LoadHistory";
        auto addPin = [&](uint64_t id, PinKind k, PinDataType dt, const char* name, Variant dv = {}) {
            PinInfo p; p.id=id; p.kind=k; p.dataType=dt; p.name=name; p.defaultValue=dv;
            node.pins.push_back(p);
        };
        addPin(1, PinKind::Input,  PinDataType::Any,     "In",          {});
        addPin(2, PinKind::Input,  PinDataType::String,  "Path",        Variant(path));
        addPin(3, PinKind::Input,  PinDataType::Integer, "MaxMessages", Variant((int64_t)0));
        addPin(4, PinKind::Output, PinDataType::Any,     "onSuccess",   {});
        addPin(5, PinKind::Output, PinDataType::Any,     "onNew",       {});
        addPin(6, PinKind::Output, PinDataType::Any,     "onError",     {});
        addPin(7, PinKind::Output, PinDataType::String,  "Messages",    {});
        addPin(8, PinKind::Output, PinDataType::Integer, "Count",       {});
        addPin(9, PinKind::Output, PinDataType::String,  "ErrorMessage",{});
        bp.nodes.push_back(node); bp.rebuildIndices();
        BlueprintRunner loader; RegisterBuiltinHandlers(loader, ".");
        ASSERT_TRUE(loader.Load(bp));
        auto res = loader.ExecuteNode(1);
        EXPECT_TRUE(res.success);
        EXPECT_EQ(loader.GetPinValue(8).asInt(), 2);
        EXPECT_NE(loader.GetPinValue(7).asString().find("ping"), std::string::npos);
        EXPECT_NE(loader.GetPinValue(7).asString().find("pong"), std::string::npos);
    }

    std::filesystem::remove(path);
}

// ============================================================================
// Pin 快照（调试器）测试
//   验证 SetSnapshotEnabled + GetNodePinSnapshot / GetSnapshotPinValue
// ============================================================================
TEST_F(HandlersTest, PinSnapshot_DisabledByDefault_NoRecording)
{
    // 默认关闭，执行后 snapshot 为空
    BlueprintData bp; bp.metadata.name = "SnapDisabledTest";
    NodeInstance node; node.id = 1; node.definitionId = "Add";
    auto addP = [&](uint64_t id, PinKind k, PinDataType dt, const char* name, Variant dv = {}) {
        PinInfo p; p.id=id; p.kind=k; p.dataType=dt; p.name=name; p.defaultValue=dv;
        node.pins.push_back(p);
    };
    addP(1, PinKind::Input,  PinDataType::Float, "A", Variant(3.0));
    addP(2, PinKind::Input,  PinDataType::Float, "B", Variant(4.0));
    addP(3, PinKind::Output, PinDataType::Float, "Result");
    bp.nodes.push_back(node); bp.rebuildIndices();

    ASSERT_FALSE(runner.IsSnapshotEnabled());
    ASSERT_TRUE(runner.Load(bp));
    runner.Execute();

    auto snap = runner.GetNodePinSnapshot(1);
    EXPECT_TRUE(snap.empty()) << "Snapshot should be empty when disabled";
}

TEST_F(HandlersTest, PinSnapshot_Enabled_RecordsOutputValue)
{
    // 启用后，Add 节点的 Result 应该被快照
    BlueprintData bp; bp.metadata.name = "SnapEnabledTest";
    NodeInstance node; node.id = 1; node.definitionId = "Add";
    auto addP = [&](uint64_t id, PinKind k, PinDataType dt, const char* name, Variant dv = {}) {
        PinInfo p; p.id=id; p.kind=k; p.dataType=dt; p.name=name; p.defaultValue=dv;
        node.pins.push_back(p);
    };
    addP(1, PinKind::Input,  PinDataType::Float, "A", Variant(3.0));
    addP(2, PinKind::Input,  PinDataType::Float, "B", Variant(4.0));
    addP(3, PinKind::Output, PinDataType::Float, "Result");
    bp.nodes.push_back(node); bp.rebuildIndices();

    runner.SetSnapshotEnabled(true);
    ASSERT_TRUE(runner.Load(bp));
    runner.ExecuteNode(1);

    auto snap = runner.GetNodePinSnapshot(1);
    EXPECT_FALSE(snap.empty()) << "Snapshot should contain entries when enabled";

    // A、B 输入引脚应被记录
    EXPECT_EQ(snap.count("A"),      1u);
    EXPECT_EQ(snap.count("B"),      1u);
    EXPECT_EQ(snap.count("Result"), 1u);

    EXPECT_DOUBLE_EQ(snap["A"].value.asFloat(),      3.0);
    EXPECT_DOUBLE_EQ(snap["B"].value.asFloat(),      4.0);
    EXPECT_DOUBLE_EQ(snap["Result"].value.asFloat(), 7.0);
}

TEST_F(HandlersTest, PinSnapshot_GetSnapshotPinValue_FastAccess)
{
    BlueprintData bp; bp.metadata.name = "SnapFastTest";
    NodeInstance node; node.id = 42; node.definitionId = "Multiply";
    auto addP = [&](uint64_t id, PinKind k, PinDataType dt, const char* name, Variant dv = {}) {
        PinInfo p; p.id=id; p.kind=k; p.dataType=dt; p.name=name; p.defaultValue=dv;
        node.pins.push_back(p);
    };
    addP(1, PinKind::Input,  PinDataType::Float, "A", Variant(6.0));
    addP(2, PinKind::Input,  PinDataType::Float, "B", Variant(7.0));
    addP(3, PinKind::Output, PinDataType::Float, "Result");
    bp.nodes.push_back(node); bp.rebuildIndices();

    runner.SetSnapshotEnabled(true);
    ASSERT_TRUE(runner.Load(bp));
    runner.ExecuteNode(42);

    // 快速单引脚访问
    EXPECT_DOUBLE_EQ(runner.GetSnapshotPinValue(42, "Result").asFloat(), 42.0);
    // 不存在的节点/引脚返回空 Variant
    EXPECT_EQ(runner.GetSnapshotPinValue(99,  "Result").type, PinDataType::Unknown);
    EXPECT_EQ(runner.GetSnapshotPinValue(42,  "NoPin").type,  PinDataType::Unknown);
}

TEST_F(HandlersTest, PinSnapshot_ClearSnapshots)
{
    BlueprintData bp; bp.metadata.name = "SnapClearTest";
    NodeInstance node; node.id = 1; node.definitionId = "Add";
    auto addP = [&](uint64_t id, PinKind k, PinDataType dt, const char* name, Variant dv = {}) {
        PinInfo p; p.id=id; p.kind=k; p.dataType=dt; p.name=name; p.defaultValue=dv;
        node.pins.push_back(p);
    };
    addP(1, PinKind::Input,  PinDataType::Float, "A", Variant(1.0));
    addP(2, PinKind::Input,  PinDataType::Float, "B", Variant(2.0));
    addP(3, PinKind::Output, PinDataType::Float, "Result");
    bp.nodes.push_back(node); bp.rebuildIndices();

    runner.SetSnapshotEnabled(true);
    ASSERT_TRUE(runner.Load(bp));
    runner.ExecuteNode(1);

    EXPECT_FALSE(runner.GetNodePinSnapshot(1).empty());

    runner.ClearSnapshots();
    EXPECT_TRUE(runner.GetNodePinSnapshot(1).empty()) << "After clear, snapshot should be empty";

    // GetSnapshotNodeIds 也应为空
    EXPECT_TRUE(runner.GetSnapshotNodeIds().empty());
}

TEST_F(HandlersTest, PinSnapshot_GetSnapshotNodeIds_ListsExecutedNodes)
{
    // 两个节点的蓝图，两个都应该出现在 snapshot ids 里
    BlueprintData bp; bp.metadata.name = "SnapNodeIdsTest";
    auto addPin = [&](NodeInstance& n, uint64_t id, PinKind k, PinDataType dt,
                      const char* name, Variant dv = {}) {
        PinInfo p; p.id=id; p.kind=k; p.dataType=dt; p.name=name; p.defaultValue=dv;
        n.pins.push_back(p);
    };

    NodeInstance n1; n1.id=1; n1.definitionId="Add";
    addPin(n1, 10, PinKind::Input,  PinDataType::Float, "A", Variant(1.0));
    addPin(n1, 11, PinKind::Input,  PinDataType::Float, "B", Variant(2.0));
    addPin(n1, 12, PinKind::Output, PinDataType::Float, "Result");
    bp.nodes.push_back(n1);

    NodeInstance n2; n2.id=2; n2.definitionId="Negate";
    addPin(n2, 20, PinKind::Input,  PinDataType::Float, "A", Variant(5.0));
    addPin(n2, 21, PinKind::Output, PinDataType::Float, "Result");
    bp.nodes.push_back(n2);

    bp.rebuildIndices();

    runner.SetSnapshotEnabled(true);
    ASSERT_TRUE(runner.Load(bp));
    runner.ExecuteNode(1);
    runner.ExecuteNode(2);

    auto ids = runner.GetSnapshotNodeIds();
    EXPECT_EQ(ids.size(), 2u) << "Both nodes should appear in snapshot";

    bool has1 = std::find(ids.begin(), ids.end(), NodeId(1)) != ids.end();
    bool has2 = std::find(ids.begin(), ids.end(), NodeId(2)) != ids.end();
    EXPECT_TRUE(has1);
    EXPECT_TRUE(has2);
}

// ============================================================================
// 循环节点测试：WhileLoop / ForLoopWithBreak / ForEachLoop
// ============================================================================

// WhileLoop — Condition 为 false → 0 次迭代，直接走 Completed，设置变量 done=1
TEST_F(HandlersTest, WhileLoop_FalseCondition_ZeroIterations)
{
    // 蓝图：OnBeginPlay → WhileLoop(Condition=false) → [Completed] → SetVariable(done,1)
    BlueprintData bp; bp.metadata.name = "WhileFalseTest";

    // WhileLoop node
    NodeInstance wl; wl.id=1; wl.definitionId="WhileLoop";
    auto addP = [](NodeInstance& n, uint64_t id, PinKind k, PinDataType dt,
                   const char* name, bool isExec=false, Variant dv={}) {
        PinInfo p; p.id=id; p.kind=k; p.dataType=dt; p.name=name;
        p.isExec=isExec; p.defaultValue=dv; n.pins.push_back(p);
    };
    addP(wl, 10, PinKind::Input,  PinDataType::Unknown, "", true);         // exec in
    addP(wl, 11, PinKind::Input,  PinDataType::Boolean, "Condition", false,
         Variant(false));
    addP(wl, 12, PinKind::Output, PinDataType::Unknown, "Loop Body", true);
    addP(wl, 13, PinKind::Output, PinDataType::Unknown, "Completed", true);
    bp.nodes.push_back(wl);

    // SetVariable(done, 1)
    NodeInstance sv; sv.id=2; sv.definitionId="SetVariable";
    addP(sv, 20, PinKind::Input,  PinDataType::Unknown, "", true);
    addP(sv, 21, PinKind::Input,  PinDataType::String,  "Name",
         false, Variant(std::string("done")));
    addP(sv, 22, PinKind::Input,  PinDataType::Integer, "Value",
         false, Variant((int64_t)1));
    addP(sv, 23, PinKind::Output, PinDataType::Unknown, "", true);
    bp.nodes.push_back(sv);

    // Links: OnBeginPlay→WhileLoop, WhileLoop.Completed→SetVariable
    AddBeginPlayEntry(bp, 100, 1000, 2000, 10);   // 100:OnBeginPlay, pin1000→10
    LinkInstance lk1; lk1.id=3000; lk1.startPinId=13; lk1.endPinId=20;
    bp.links.push_back(lk1);
    bp.rebuildIndices();

    BlueprintRunner r; RegisterBuiltinHandlers(r, ".");
    ASSERT_TRUE(r.Load(bp));
    RunWithBeginPlay(r);

    EXPECT_EQ(r.GetVariable("done").asInt(), 1)
        << "Completed branch should fire; SetVariable should set done=1";
}

// WhileLoop — 计数 0→N：用 ForLoop 来简单验证 WhileLoop 能正确跑 N 次
// 通过 Less 节点做条件，每次 LoopBody 中 SetVariable(counter, counter+1)
// 结构稍复杂，改用直接测试"迭代次数"的简单版本：
// Condition 先 true 跑 3 次再变 false（借助 ForLoop 更简单，此处跳过复杂版）
// 只测 Completed 分支 + 0次迭代已在上面，补充：WhileLoop maxIterations 保护
TEST_F(HandlersTest, WhileLoop_TrueCondition_MaxIterGuard)
{
    // Condition 硬编码 true → 应该跑满 maxIterations(10000) 次然后退出（不崩溃）
    // Completed 分支随后激活 → done=1
    BlueprintData bp; bp.metadata.name = "WhileMaxIterTest";

    NodeInstance wl; wl.id=1; wl.definitionId="WhileLoop";
    auto addP = [](NodeInstance& n, uint64_t id, PinKind k, PinDataType dt,
                   const char* name, bool isExec=false, Variant dv={}) {
        PinInfo p; p.id=id; p.kind=k; p.dataType=dt; p.name=name;
        p.isExec=isExec; p.defaultValue=dv; n.pins.push_back(p);
    };
    addP(wl, 10, PinKind::Input,  PinDataType::Unknown, "", true);
    addP(wl, 11, PinKind::Input,  PinDataType::Boolean, "Condition", false,
         Variant(true));                                           // 永远 true
    addP(wl, 12, PinKind::Output, PinDataType::Unknown, "Loop Body", true);
    addP(wl, 13, PinKind::Output, PinDataType::Unknown, "Completed", true);
    bp.nodes.push_back(wl);

    NodeInstance sv; sv.id=2; sv.definitionId="SetVariable";
    addP(sv, 20, PinKind::Input,  PinDataType::Unknown, "", true);
    addP(sv, 21, PinKind::Input,  PinDataType::String,  "Name",  false, Variant(std::string("done")));
    addP(sv, 22, PinKind::Input,  PinDataType::Integer, "Value", false, Variant((int64_t)1));
    addP(sv, 23, PinKind::Output, PinDataType::Unknown, "", true);
    bp.nodes.push_back(sv);

    AddBeginPlayEntry(bp, 100, 1000, 2000, 10);
    LinkInstance lk1; lk1.id=3000; lk1.startPinId=13; lk1.endPinId=20;
    bp.links.push_back(lk1);
    bp.rebuildIndices();

    BlueprintRunner r; RegisterBuiltinHandlers(r, ".");
    ASSERT_TRUE(r.Load(bp));
    RunWithBeginPlay(r);   // must not hang

    // Completed 一定被激活（超限后 break → Completed）
    EXPECT_EQ(r.GetVariable("done").asInt(), 1)
        << "After maxIterations guard, Completed branch must still fire";
}

// ForLoopWithBreak — 跑 0..9，LoopBody 每次 counter++，在 index==3 触发 Break
// Break 是由 Branch 节点路由实现的（index==3 → Break pin）
// 简化版：Break 引脚不通过运行时路由，直接在 handler 里用变量标志模拟
// 实际测试 ForLoopWithBreak 的完整路径：不连 Break 引脚，验证全跑 0..4
TEST_F(HandlersTest, ForLoopWithBreak_NoBreak_RunsFull)
{
    // OnBeginPlay → ForLoopWithBreak(0..4, no break) → LoopBody: counter += Index
    // 预期 counter = 0+1+2+3+4 = 10
    BlueprintData bp; bp.metadata.name = "FLBreakNoBreakTest";

    NodeInstance fl; fl.id=1; fl.definitionId="ForLoopWithBreak";
    auto addP = [](NodeInstance& n, uint64_t id, PinKind k, PinDataType dt,
                   const char* name, bool isExec=false, Variant dv={}) {
        PinInfo p; p.id=id; p.kind=k; p.dataType=dt; p.name=name;
        p.isExec=isExec; p.defaultValue=dv; n.pins.push_back(p);
    };
    addP(fl, 10, PinKind::Input,  PinDataType::Unknown, "",           true);   // exec
    addP(fl, 11, PinKind::Input,  PinDataType::Unknown, "Break",      true);   // break pin
    addP(fl, 12, PinKind::Input,  PinDataType::Integer, "First Index",false, Variant((int64_t)0));
    addP(fl, 13, PinKind::Input,  PinDataType::Integer, "Last Index", false, Variant((int64_t)4));
    addP(fl, 14, PinKind::Output, PinDataType::Unknown, "Loop Body",  true);
    addP(fl, 15, PinKind::Output, PinDataType::Integer, "Index");
    addP(fl, 16, PinKind::Output, PinDataType::Unknown, "Completed",  true);
    bp.nodes.push_back(fl);

    // SetVariable: counter = counter + Index
    // 简化：用 SetVariable name="counter", value=Index（GetVariable 拿不到 counter 初始值时用 0）
    // 用 ForLoop 已覆盖 counter 累加，这里只验证 Completed 和节点不崩
    NodeInstance sv; sv.id=2; sv.definitionId="SetVariable";
    addP(sv, 20, PinKind::Input,  PinDataType::Unknown, "",       true);
    addP(sv, 21, PinKind::Input,  PinDataType::String,  "Name",  false, Variant(std::string("loopDone")));
    addP(sv, 22, PinKind::Input,  PinDataType::Integer, "Value", false, Variant((int64_t)1));
    addP(sv, 23, PinKind::Output, PinDataType::Unknown, "",       true);
    bp.nodes.push_back(sv);

    AddBeginPlayEntry(bp, 100, 1000, 2000, 10);
    LinkInstance lk1; lk1.id=3000; lk1.startPinId=16; lk1.endPinId=20;  // Completed→SetVariable
    bp.links.push_back(lk1);
    bp.rebuildIndices();

    BlueprintRunner r; RegisterBuiltinHandlers(r, ".");
    ASSERT_TRUE(r.Load(bp));
    auto result = RunWithBeginPlay(r);
    EXPECT_TRUE(result.success);
    EXPECT_EQ(r.GetVariable("loopDone").asInt(), 1)
        << "Completed should fire after ForLoopWithBreak runs 0..4 without break";
}

// ForEachLoop — JSON 数组 ["x","y","z"] → 迭代 3 次，count==3
TEST_F(HandlersTest, ForEachLoop_IteratesAllElements)
{
    BlueprintData bp; bp.metadata.name = "ForEachTest";

    NodeInstance fe; fe.id=1; fe.definitionId="ForEachLoop";
    auto addP = [](NodeInstance& n, uint64_t id, PinKind k, PinDataType dt,
                   const char* name, bool isExec=false, Variant dv={}) {
        PinInfo p; p.id=id; p.kind=k; p.dataType=dt; p.name=name;
        p.isExec=isExec; p.defaultValue=dv; n.pins.push_back(p);
    };
    addP(fe, 10, PinKind::Input,  PinDataType::Unknown, "",          true);
    addP(fe, 11, PinKind::Input,  PinDataType::Array,   "Array",     false,
         Variant(std::vector<Variant>{
             Variant(std::string("x")),
             Variant(std::string("y")),
             Variant(std::string("z"))}));
    addP(fe, 12, PinKind::Output, PinDataType::Unknown, "Loop Body", true);
    addP(fe, 13, PinKind::Output, PinDataType::String,  "Array Element");
    addP(fe, 14, PinKind::Output, PinDataType::Integer, "Array Index");
    addP(fe, 15, PinKind::Output, PinDataType::Unknown, "Completed", true);
    bp.nodes.push_back(fe);

    // SetVariable(count, count+1) — 简化：只用 SetVariable("iterCount", Index+1) 但 ForEach 没 index
    // 方案：每次 LoopBody → SetVariable("iterCount", iterCount+1) 需要 GetVariable+Add
    // 更简：LoopBody → SetVariable("last", Element) 最后验证 last=="z"
    // 并另一个 Completed → SetVariable("completed", 1)
    NodeInstance sv1; sv1.id=2; sv1.definitionId="SetVariable";
    addP(sv1, 20, PinKind::Input,  PinDataType::Unknown, "", true);
    addP(sv1, 21, PinKind::Input,  PinDataType::String,  "Name",  false, Variant(std::string("last")));
    addP(sv1, 22, PinKind::Input,  PinDataType::Any,     "Value", false);  // 由 Element 连线传入
    addP(sv1, 23, PinKind::Output, PinDataType::Unknown, "", true);
    bp.nodes.push_back(sv1);

    NodeInstance sv2; sv2.id=3; sv2.definitionId="SetVariable";
    addP(sv2, 30, PinKind::Input,  PinDataType::Unknown, "", true);
    addP(sv2, 31, PinKind::Input,  PinDataType::String,  "Name",  false, Variant(std::string("done")));
    addP(sv2, 32, PinKind::Input,  PinDataType::Integer, "Value", false, Variant((int64_t)1));
    addP(sv2, 33, PinKind::Output, PinDataType::Unknown, "", true);
    bp.nodes.push_back(sv2);

    AddBeginPlayEntry(bp, 100, 1000, 2000, 10);
    // LoopBody → SetVariable(last, Element)
    LinkInstance lk1; lk1.id=3000; lk1.startPinId=12; lk1.endPinId=20;
    // Element → sv1.Value
    LinkInstance lk2; lk2.id=3001; lk2.startPinId=13; lk2.endPinId=22;
    // Completed → SetVariable(done,1)
    LinkInstance lk3; lk3.id=3002; lk3.startPinId=15; lk3.endPinId=30;
    bp.links.insert(bp.links.end(), {lk1, lk2, lk3});
    bp.rebuildIndices();

    BlueprintRunner r; RegisterBuiltinHandlers(r, ".");
    ASSERT_TRUE(r.Load(bp));
    RunWithBeginPlay(r);

    // 最后一个 Element 是 "z"
    EXPECT_EQ(r.GetVariable("last").asString(), "z")
        << "Last element should be 'z'";
    EXPECT_EQ(r.GetVariable("done").asInt(), 1)
        << "Completed branch must fire after all elements processed";
}

// ============================================================================
// Retry.Backoff 节点测试
// ============================================================================

TEST_F(HandlersTest, RetryBackoff_SucceedOnFirstAttempt)
{
    // OnBeginPlay → Retry.Backoff(MaxRetries=3)
    //   onTry → SetVariable(__retry_succeeded, true)  # 立即成功
    //   onSuccess → SetVariable(result, "ok")
    //   onExceeded → SetVariable(result, "fail")
    BlueprintData bp; bp.metadata.name = "RetrySucceedFirstTest";

    auto addP = [](NodeInstance& n, uint64_t id, PinKind k, PinDataType dt,
                   const char* name, bool isExec=false, Variant dv={}) {
        PinInfo p; p.id=id; p.kind=k; p.dataType=dt; p.name=name;
        p.isExec=isExec; p.defaultValue=dv; n.pins.push_back(p);
    };

    // Retry.Backoff node (id=1)
    NodeInstance rb; rb.id=1; rb.definitionId="Retry.Backoff";
    addP(rb, 10, PinKind::Input,  PinDataType::Unknown, "", true);
    addP(rb, 11, PinKind::Input,  PinDataType::Integer, "MaxRetries",        false, Variant((int64_t)3));
    addP(rb, 12, PinKind::Input,  PinDataType::Float,   "InitialDelayMs",    false, Variant(0.0));
    addP(rb, 13, PinKind::Input,  PinDataType::Float,   "BackoffMultiplier", false, Variant(2.0));
    addP(rb, 14, PinKind::Input,  PinDataType::Float,   "MaxDelayMs",        false, Variant(10000.0));
    addP(rb, 15, PinKind::Output, PinDataType::Unknown, "onTry",     true);
    addP(rb, 16, PinKind::Output, PinDataType::Integer, "AttemptIndex");
    addP(rb, 17, PinKind::Output, PinDataType::Float,   "DelayMs");
    addP(rb, 18, PinKind::Output, PinDataType::Unknown, "onSuccess", true);
    addP(rb, 19, PinKind::Output, PinDataType::Unknown, "onExceeded",true);
    bp.nodes.push_back(rb);

    // SetVariable(__retry_succeeded, true)  id=2
    NodeInstance sv1; sv1.id=2; sv1.definitionId="SetVariable";
    addP(sv1, 20, PinKind::Input,  PinDataType::Unknown, "", true);
    addP(sv1, 21, PinKind::Input,  PinDataType::String,  "Name",  false, Variant(std::string("__retry_succeeded")));
    addP(sv1, 22, PinKind::Input,  PinDataType::Boolean, "Value", false, Variant(true));
    addP(sv1, 23, PinKind::Output, PinDataType::Unknown, "", true);
    bp.nodes.push_back(sv1);

    // SetVariable(result, "ok")  id=3  (onSuccess branch)
    NodeInstance sv2; sv2.id=3; sv2.definitionId="SetVariable";
    addP(sv2, 30, PinKind::Input,  PinDataType::Unknown, "", true);
    addP(sv2, 31, PinKind::Input,  PinDataType::String,  "Name",  false, Variant(std::string("result")));
    addP(sv2, 32, PinKind::Input,  PinDataType::String,  "Value", false, Variant(std::string("ok")));
    addP(sv2, 33, PinKind::Output, PinDataType::Unknown, "", true);
    bp.nodes.push_back(sv2);

    // SetVariable(result, "fail")  id=4  (onExceeded branch)
    NodeInstance sv3; sv3.id=4; sv3.definitionId="SetVariable";
    addP(sv3, 40, PinKind::Input,  PinDataType::Unknown, "", true);
    addP(sv3, 41, PinKind::Input,  PinDataType::String,  "Name",  false, Variant(std::string("result")));
    addP(sv3, 42, PinKind::Input,  PinDataType::String,  "Value", false, Variant(std::string("fail")));
    addP(sv3, 43, PinKind::Output, PinDataType::Unknown, "", true);
    bp.nodes.push_back(sv3);

    AddBeginPlayEntry(bp, 100, 1000, 2000, 10);            // OnBeginPlay→Retry
    LinkInstance lk1; lk1.id=3001; lk1.startPinId=15; lk1.endPinId=20;  // onTry→SetSucceeded
    LinkInstance lk2; lk2.id=3002; lk2.startPinId=18; lk2.endPinId=30;  // onSuccess→SetOk
    LinkInstance lk3; lk3.id=3003; lk3.startPinId=19; lk3.endPinId=40;  // onExceeded→SetFail
    bp.links.insert(bp.links.end(), {lk1, lk2, lk3});
    bp.rebuildIndices();

    BlueprintRunner r; RegisterBuiltinHandlers(r, ".");
    ASSERT_TRUE(r.Load(bp));
    RunWithBeginPlay(r);

    EXPECT_EQ(r.GetVariable("result").asString(), "ok")
        << "Should succeed on first attempt → result='ok'";
}

TEST_F(HandlersTest, RetryBackoff_ExhaustsAllRetries)
{
    // __retry_succeeded never set to true → all 3 retries exhausted → onExceeded
    BlueprintData bp; bp.metadata.name = "RetryExhaustedTest";

    auto addP = [](NodeInstance& n, uint64_t id, PinKind k, PinDataType dt,
                   const char* name, bool isExec=false, Variant dv={}) {
        PinInfo p; p.id=id; p.kind=k; p.dataType=dt; p.name=name;
        p.isExec=isExec; p.defaultValue=dv; n.pins.push_back(p);
    };

    NodeInstance rb; rb.id=1; rb.definitionId="Retry.Backoff";
    addP(rb, 10, PinKind::Input,  PinDataType::Unknown, "", true);
    addP(rb, 11, PinKind::Input,  PinDataType::Integer, "MaxRetries",        false, Variant((int64_t)3));
    addP(rb, 12, PinKind::Input,  PinDataType::Float,   "InitialDelayMs",    false, Variant(0.0));
    addP(rb, 13, PinKind::Input,  PinDataType::Float,   "BackoffMultiplier", false, Variant(2.0));
    addP(rb, 14, PinKind::Input,  PinDataType::Float,   "MaxDelayMs",        false, Variant(10000.0));
    addP(rb, 15, PinKind::Output, PinDataType::Unknown, "onTry",     true);
    addP(rb, 16, PinKind::Output, PinDataType::Integer, "AttemptIndex");
    addP(rb, 17, PinKind::Output, PinDataType::Float,   "DelayMs");
    addP(rb, 18, PinKind::Output, PinDataType::Unknown, "onSuccess", true);
    addP(rb, 19, PinKind::Output, PinDataType::Unknown, "onExceeded",true);
    bp.nodes.push_back(rb);

    // onTry → SetVariable(attempts, attempts+1) — just increment counter
    NodeInstance sv1; sv1.id=2; sv1.definitionId="SetVariable";
    addP(sv1, 20, PinKind::Input,  PinDataType::Unknown, "", true);
    addP(sv1, 21, PinKind::Input,  PinDataType::String,  "Name",  false, Variant(std::string("attempts")));
    addP(sv1, 22, PinKind::Input,  PinDataType::Integer, "Value", false, Variant((int64_t)0));  // overwritten by AttemptIndex+1
    addP(sv1, 23, PinKind::Output, PinDataType::Unknown, "", true);
    bp.nodes.push_back(sv1);

    // onExceeded → SetVariable(result, "exceeded")
    NodeInstance sv2; sv2.id=3; sv2.definitionId="SetVariable";
    addP(sv2, 30, PinKind::Input,  PinDataType::Unknown, "", true);
    addP(sv2, 31, PinKind::Input,  PinDataType::String,  "Name",  false, Variant(std::string("result")));
    addP(sv2, 32, PinKind::Input,  PinDataType::String,  "Value", false, Variant(std::string("exceeded")));
    addP(sv2, 33, PinKind::Output, PinDataType::Unknown, "", true);
    bp.nodes.push_back(sv2);

    // AttemptIndex → sv1.Value (each onTry sets attempts = AttemptIndex+1 implicitly)
    AddBeginPlayEntry(bp, 100, 1000, 2000, 10);
    LinkInstance lk1; lk1.id=3001; lk1.startPinId=15; lk1.endPinId=20;  // onTry→sv1
    LinkInstance lk2; lk2.id=3002; lk2.startPinId=16; lk2.endPinId=22;  // AttemptIndex→sv1.Value
    LinkInstance lk3; lk3.id=3003; lk3.startPinId=19; lk3.endPinId=30;  // onExceeded→sv2
    bp.links.insert(bp.links.end(), {lk1, lk2, lk3});
    bp.rebuildIndices();

    BlueprintRunner r; RegisterBuiltinHandlers(r, ".");
    ASSERT_TRUE(r.Load(bp));
    RunWithBeginPlay(r);

    EXPECT_EQ(r.GetVariable("result").asString(), "exceeded")
        << "All 3 retries exhausted → result='exceeded'";
    // Last attempt index should be 2 (0-based, 3 attempts)
    EXPECT_EQ(r.GetVariable("attempts").asInt(), 2)
        << "AttemptIndex at last onTry should be 2";
}

TEST_F(HandlersTest, RetryBackoff_SucceedOnSecondAttempt)
{
    // Set __retry_succeeded=true only after first attempt (attempt index 1)
    // Achieved by: onTry → Branch(AttemptIndex >= 1) → SetSucceeded=true
    // Using: AttemptIndex output + SetVariable pattern
    // Simpler: just use a counter variable
    BlueprintData bp; bp.metadata.name = "RetrySucceedSecondTest";

    auto addP = [](NodeInstance& n, uint64_t id, PinKind k, PinDataType dt,
                   const char* name, bool isExec=false, Variant dv={}) {
        PinInfo p; p.id=id; p.kind=k; p.dataType=dt; p.name=name;
        p.isExec=isExec; p.defaultValue=dv; n.pins.push_back(p);
    };

    NodeInstance rb; rb.id=1; rb.definitionId="Retry.Backoff";
    addP(rb, 10, PinKind::Input,  PinDataType::Unknown, "", true);
    addP(rb, 11, PinKind::Input,  PinDataType::Integer, "MaxRetries",        false, Variant((int64_t)5));
    addP(rb, 12, PinKind::Input,  PinDataType::Float,   "InitialDelayMs",    false, Variant(0.0));
    addP(rb, 13, PinKind::Input,  PinDataType::Float,   "BackoffMultiplier", false, Variant(1.0));  // no growth
    addP(rb, 14, PinKind::Input,  PinDataType::Float,   "MaxDelayMs",        false, Variant(0.0));
    addP(rb, 15, PinKind::Output, PinDataType::Unknown, "onTry",     true);
    addP(rb, 16, PinKind::Output, PinDataType::Integer, "AttemptIndex");
    addP(rb, 17, PinKind::Output, PinDataType::Float,   "DelayMs");
    addP(rb, 18, PinKind::Output, PinDataType::Unknown, "onSuccess", true);
    addP(rb, 19, PinKind::Output, PinDataType::Unknown, "onExceeded",true);
    bp.nodes.push_back(rb);

    // Branch: AttemptIndex >= 1 → true branch → SetSucceeded
    NodeInstance br; br.id=2; br.definitionId="Branch";
    addP(br, 20, PinKind::Input,  PinDataType::Unknown, "",          true);
    addP(br, 21, PinKind::Input,  PinDataType::Boolean, "Condition", false, Variant(false));
    addP(br, 22, PinKind::Output, PinDataType::Unknown, "True",  true);
    addP(br, 23, PinKind::Output, PinDataType::Unknown, "False", true);
    bp.nodes.push_back(br);

    // Greater (AttemptIndex, 0): AttemptIndex > 0 → true from attempt 1
    NodeInstance gt; gt.id=5; gt.definitionId="GreaterEqual";
    addP(gt, 50, PinKind::Input,  PinDataType::Integer, "A", false);  // AttemptIndex
    addP(gt, 51, PinKind::Input,  PinDataType::Integer, "B", false, Variant((int64_t)1));
    addP(gt, 52, PinKind::Output, PinDataType::Boolean, "Result");
    bp.nodes.push_back(gt);

    NodeInstance sv; sv.id=3; sv.definitionId="SetVariable";
    addP(sv, 30, PinKind::Input,  PinDataType::Unknown, "", true);
    addP(sv, 31, PinKind::Input,  PinDataType::String,  "Name",  false, Variant(std::string("__retry_succeeded")));
    addP(sv, 32, PinKind::Input,  PinDataType::Boolean, "Value", false, Variant(true));
    addP(sv, 33, PinKind::Output, PinDataType::Unknown, "", true);
    bp.nodes.push_back(sv);

    NodeInstance sv2; sv2.id=4; sv2.definitionId="SetVariable";
    addP(sv2, 40, PinKind::Input,  PinDataType::Unknown, "", true);
    addP(sv2, 41, PinKind::Input,  PinDataType::String,  "Name",  false, Variant(std::string("result")));
    addP(sv2, 42, PinKind::Input,  PinDataType::String,  "Value", false, Variant(std::string("ok")));
    addP(sv2, 43, PinKind::Output, PinDataType::Unknown, "", true);
    bp.nodes.push_back(sv2);

    AddBeginPlayEntry(bp, 100, 1000, 2000, 10);
    // onTry → Branch
    LinkInstance lk1; lk1.id=3001; lk1.startPinId=15; lk1.endPinId=20;
    // AttemptIndex → GreaterEqual.A
    LinkInstance lk2; lk2.id=3002; lk2.startPinId=16; lk2.endPinId=50;
    // GreaterEqual.Result → Branch.Condition
    LinkInstance lk3; lk3.id=3003; lk3.startPinId=52; lk3.endPinId=21;
    // Branch.True → SetSucceeded
    LinkInstance lk4; lk4.id=3004; lk4.startPinId=22; lk4.endPinId=30;
    // onSuccess → SetOk
    LinkInstance lk5; lk5.id=3005; lk5.startPinId=18; lk5.endPinId=40;
    bp.links.insert(bp.links.end(), {lk1, lk2, lk3, lk4, lk5});
    bp.rebuildIndices();

    BlueprintRunner r; RegisterBuiltinHandlers(r, ".");
    ASSERT_TRUE(r.Load(bp));
    RunWithBeginPlay(r);

    EXPECT_EQ(r.GetVariable("result").asString(), "ok")
        << "Should succeed on attempt index 1 → result='ok'";
}
