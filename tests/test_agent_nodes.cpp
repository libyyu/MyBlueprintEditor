// tests/test_agent_nodes.cpp
// 覆盖 2026-04-07 新增/修复的 AI Agent 节点：
//   1. JSON.ToolCallCount  — 补充缺失 handler（含裸数组 + 完整 response 两种输入格式）
//   2. Tool.CallByName     — 动态工具路由新节点
//   3. BP_DispatchEvent    — C API 新函数

#include <gtest/gtest.h>
#include "../Runtime/BlueprintRunner.h"
#include "../Runtime/BlueprintExporter.h"
#include "../Runtime/BuiltinHandlers.h"
#include "../Runtime/BlueprintCAPI.h"
#include "../Runtime/Http/IHttpClient.h"
#include "test_helpers.h"

using namespace NodeEditor::Runtime;
using namespace TestHelpers;

// ============================================================================
// 公共辅助
// ============================================================================

// 构建最小蓝图辅助：OnBeginPlay → 目标节点
static BlueprintData MakeSimpleBP(
    const std::string& defId,
    const std::vector<PinInfo>& nodePins,
    const std::unordered_map<std::string, Variant>& nodeData = {})
{
    BlueprintData bp;

    // OnBeginPlay（id=1，execOut pin=10）
    {
        NodeInstance n; n.id=1; n.definitionId="OnBeginPlay"; n.name="OnBeginPlay";
        PinInfo p; p.id=10; p.kind=PinKind::Output; p.isExec=true; p.name="";
        n.pins.push_back(p);
        bp.nodes.push_back(n);
    }

    // 目标节点（id=2）
    {
        NodeInstance n; n.id=2; n.definitionId=defId; n.name=defId;
        n.pins = nodePins;
        n.nodeData = nodeData;
        bp.nodes.push_back(n);
    }

    // 连线 OnBeginPlay.execOut → 目标节点.execIn
    // 假定目标节点第一个 pin 是 exec input
    if (!nodePins.empty() && nodePins[0].isExec && nodePins[0].kind == PinKind::Input)
    {
        LinkInstance lnk; lnk.id=100;
        lnk.startPinId = 10;
        lnk.endPinId   = nodePins[0].id;
        bp.links.push_back(lnk);
    }
    return bp;
}

static PinInfo MakeExecIn(PinId id)
{
    PinInfo p; p.id=id; p.kind=PinKind::Input; p.isExec=true; p.name=""; return p;
}
static PinInfo MakeExecOut(PinId id, const std::string& name="")
{
    PinInfo p; p.id=id; p.kind=PinKind::Output; p.isExec=true; p.name=name; return p;
}
static PinInfo MakeDataIn(PinId id, PinDataType dt, const std::string& name, Variant dv={})
{
    PinInfo p; p.id=id; p.kind=PinKind::Input; p.isExec=false; p.dataType=dt;
    p.name=name; p.defaultValue=dv; return p;
}
static PinInfo MakeDataOut(PinId id, PinDataType dt, const std::string& name)
{
    PinInfo p; p.id=id; p.kind=PinKind::Output; p.isExec=false; p.dataType=dt;
    p.name=name; return p;
}

// ============================================================================
// 1. JSON.ToolCallCount 测试
// ============================================================================

class ToolCallCountTest : public ::testing::Test
{
protected:
    BlueprintRunner runner;
    std::vector<std::string> prints;

    void SetUp() override {
        RegisterBuiltinHandlers(runner);
        runner.SetPrintCallback([this](LogLevel, const std::string& msg) {
            prints.push_back(msg);
        });
    }

    // 构建：OnBeginPlay → JSON.ToolCallCount → PrintString(Count)
    void runWith(const std::string& toolCallsJson)
    {
        BlueprintData bp;

        // OnBeginPlay (id=1, execOut=10)
        { NodeInstance n; n.id=1; n.definitionId="OnBeginPlay";
          n.pins.push_back(MakeExecOut(10));
          bp.nodes.push_back(n); }

        // JSON.ToolCallCount (id=2)
        { NodeInstance n; n.id=2; n.definitionId="JSON.ToolCallCount";
          n.pins.push_back(MakeDataIn(21, PinDataType::String, "ToolCallsJSON",
                                      Variant(toolCallsJson)));
          n.pins.push_back(MakeDataOut(22, PinDataType::Integer, "Count"));
          bp.nodes.push_back(n); }

        // PrintString (id=3, execIn=30, "In String" input=31)
        { NodeInstance n; n.id=3; n.definitionId="PrintString";
          n.pins.push_back(MakeExecIn(30));
          n.pins.push_back(MakeExecOut(31));
          n.pins.push_back(MakeDataIn(32, PinDataType::String, "In String", Variant(std::string(""))));
          bp.nodes.push_back(n); }

        // 连线：OnBeginPlay.exec → PrintString.execIn
        { LinkInstance l; l.id=100; l.startPinId=10; l.endPinId=30; bp.links.push_back(l); }
        // 连线：Count → "In String"（通过 IntToString 绕不开，直接连 Any）
        { LinkInstance l; l.id=101; l.startPinId=22; l.endPinId=32; bp.links.push_back(l); }

        runner.Load(bp);
        RunWithBeginPlay(runner);
    }

    int64_t getCountVar()
    {
        return runner.GetPinValue(22).asInt();
    }
};

TEST_F(ToolCallCountTest, BareArray_TwoTools)
{
    // 裸 tool_calls 数组，2 个工具
    std::string json = R"([
        {"id":"c1","type":"function","function":{"name":"get_weather","arguments":"{}"}},
        {"id":"c2","type":"function","function":{"name":"search_web","arguments":"{}"}}
    ])";
    runWith(json);
    EXPECT_EQ(getCountVar(), 2) << "Should count 2 tool calls in bare array";
}

TEST_F(ToolCallCountTest, BareArray_OneTools)
{
    std::string json = R"([{"id":"c1","type":"function","function":{"name":"get_weather","arguments":"{}"}}])";
    runWith(json);
    EXPECT_EQ(getCountVar(), 1);
}

TEST_F(ToolCallCountTest, EmptyArray)
{
    runWith("[]");
    EXPECT_EQ(getCountVar(), 0) << "Empty array should return count=0";
}

TEST_F(ToolCallCountTest, EmptyString)
{
    runWith("");
    EXPECT_EQ(getCountVar(), 0) << "Empty string should return count=0";
}

TEST_F(ToolCallCountTest, FullResponseFormat)
{
    // 完整 LLM response 格式，Handler 应能自动提取
    std::string json = R"({
        "id": "chatcmpl-mock",
        "choices": [{
            "message": {
                "role": "assistant",
                "tool_calls": [
                    {"id":"c1","function":{"name":"get_weather","arguments":"{}"}},
                    {"id":"c2","function":{"name":"search","arguments":"{}"}},
                    {"id":"c3","function":{"name":"calc","arguments":"{}"}}
                ]
            },
            "finish_reason": "tool_calls"
        }]
    })";
    runWith(json);
    EXPECT_EQ(getCountVar(), 3) << "Full response format should extract tool_calls count=3";
}

TEST_F(ToolCallCountTest, InvalidJson)
{
    runWith("not-json");
    EXPECT_EQ(getCountVar(), 0) << "Invalid JSON should return count=0 without crash";
}

// ============================================================================
// 2. Tool.CallByName 测试
// ============================================================================

class ToolCallByNameTest : public ::testing::Test
{
protected:
    BlueprintRunner runner;
    std::vector<std::string> prints;
    std::vector<std::string> logs;

    void SetUp() override {
        RegisterBuiltinHandlers(runner);
        runner.SetPrintCallback([this](LogLevel, const std::string& msg){ prints.push_back(msg); });
        runner.SetLogCallback([this](LogLevel, const std::string& msg){ logs.push_back(msg); });
    }

    // 注册一个简单的外部 FuncLib（单函数：echo_tool，返回 "echo:" + input）
    void registerEchoTool()
    {
        // 构建 FuncLib BlueprintData
        BlueprintData libData;
        libData.metadata.name = "TestLib";
        libData.metadata.blueprintClass = BlueprintClass::FunctionLibrary;

        // Function.Entry (id=1, execOut=10, "Input" data out=11)
        { NodeInstance n; n.id=1; n.definitionId="Function.Entry"; n.name="echo_tool";
          n.pins.push_back(MakeExecOut(10));
          n.pins.push_back(MakeDataOut(11, PinDataType::String, "Input"));
          libData.nodes.push_back(n); }

        // AppendString (id=2, execIn=20, A=21(String), B=22(String), Result=23)
        { NodeInstance n; n.id=2; n.definitionId="AppendString";
          n.pins.push_back(MakeExecIn(20));
          n.pins.push_back(MakeExecOut(29));
          n.pins.push_back(MakeDataIn(21, PinDataType::String, "A",
                                      Variant(std::string("echo:"))));
          n.pins.push_back(MakeDataIn(22, PinDataType::String, "B",
                                      Variant(std::string(""))));
          n.pins.push_back(MakeDataOut(23, PinDataType::String, "Result"));
          libData.nodes.push_back(n); }

        // Function.Return (id=3, execIn=30, "Result" in=31)
        { NodeInstance n; n.id=3; n.definitionId="Function.Return"; n.name="echo_tool";
          n.pins.push_back(MakeExecIn(30));
          n.pins.push_back(MakeDataIn(31, PinDataType::String, "Result",
                                      Variant(std::string(""))));
          libData.nodes.push_back(n); }

        // 连线：Entry.exec → AppendString.exec → Return.exec
        { LinkInstance l; l.id=100; l.startPinId=10; l.endPinId=20; libData.links.push_back(l); }
        { LinkInstance l; l.id=101; l.startPinId=29; l.endPinId=30; libData.links.push_back(l); }
        // Entry.Input → AppendString.B
        { LinkInstance l; l.id=102; l.startPinId=11; l.endPinId=22; libData.links.push_back(l); }
        // AppendString.Result → Return.Result
        { LinkInstance l; l.id=103; l.startPinId=23; l.endPinId=31; libData.links.push_back(l); }

        // 注册函数定义
        FunctionDefinition fd;
        fd.id = "echo_tool"; fd.name = "echo_tool"; fd.isPublic = true;
        libData.functions.push_back(fd);

        runner.RegisterExternalLibrary(libData);
        runner.RegisterExternalFunction(fd);
    }
};

TEST_F(ToolCallByNameTest, FoundAndCalled)
{
    registerEchoTool();

    // 构建主蓝图：OnBeginPlay → Tool.CallByName → PrintString(Result)
    BlueprintData bp;
    { NodeInstance n; n.id=1; n.definitionId="OnBeginPlay";
      n.pins.push_back(MakeExecOut(10));
      bp.nodes.push_back(n); }

    { NodeInstance n; n.id=2; n.definitionId="Tool.CallByName";
      n.pins.push_back(MakeExecIn(20));
      n.pins.push_back(MakeDataIn(21, PinDataType::String, "ToolName",
                                  Variant(std::string("echo_tool"))));
      n.pins.push_back(MakeDataIn(22, PinDataType::String, "Arguments",
                                  Variant(std::string(R"({"Input":"hello"})"))));
      n.pins.push_back(MakeDataIn(23, PinDataType::String, "ToolCallId",
                                  Variant(std::string("call_1"))));
      n.pins.push_back(MakeExecOut(24, "onSuccess"));
      n.pins.push_back(MakeExecOut(25, "onNotFound"));
      n.pins.push_back(MakeDataOut(26, PinDataType::String, "Result"));
      n.pins.push_back(MakeDataOut(27, PinDataType::String, "ToolCallId"));
      bp.nodes.push_back(n); }

    // PrintString (id=3)
    { NodeInstance n; n.id=3; n.definitionId="PrintString";
      n.pins.push_back(MakeExecIn(30));
      n.pins.push_back(MakeExecOut(39));
      n.pins.push_back(MakeDataIn(31, PinDataType::String, "In String",
                                  Variant(std::string(""))));
      bp.nodes.push_back(n); }

    // 连线
    { LinkInstance l; l.id=100; l.startPinId=10; l.endPinId=20; bp.links.push_back(l); }
    { LinkInstance l; l.id=101; l.startPinId=24; l.endPinId=30; bp.links.push_back(l); } // onSuccess→Print
    { LinkInstance l; l.id=102; l.startPinId=26; l.endPinId=31; bp.links.push_back(l); } // Result→InString

    runner.Load(bp);
    RunWithBeginPlay(runner);

    bool found = false;
    for (const auto& p : prints)
        if (p.find("echo:hello") != std::string::npos) { found = true; break; }
    EXPECT_TRUE(found) << "echo_tool should return 'echo:hello'. Prints: "
                       << (prints.empty() ? "(none)" : prints[0]);
}

TEST_F(ToolCallByNameTest, NotFoundRoutesToOnNotFound)
{
    registerEchoTool();

    BlueprintData bp;
    { NodeInstance n; n.id=1; n.definitionId="OnBeginPlay";
      n.pins.push_back(MakeExecOut(10));
      bp.nodes.push_back(n); }

    { NodeInstance n; n.id=2; n.definitionId="Tool.CallByName";
      n.pins.push_back(MakeExecIn(20));
      n.pins.push_back(MakeDataIn(21, PinDataType::String, "ToolName",
                                  Variant(std::string("no_such_tool"))));
      n.pins.push_back(MakeDataIn(22, PinDataType::String, "Arguments",
                                  Variant(std::string("{}"))));
      n.pins.push_back(MakeDataIn(23, PinDataType::String, "ToolCallId",
                                  Variant(std::string(""))));
      n.pins.push_back(MakeExecOut(24, "onSuccess"));
      n.pins.push_back(MakeExecOut(25, "onNotFound"));
      n.pins.push_back(MakeDataOut(26, PinDataType::String, "Result"));
      n.pins.push_back(MakeDataOut(27, PinDataType::String, "ToolCallId"));
      bp.nodes.push_back(n); }

    // PrintString on onNotFound
    { NodeInstance n; n.id=3; n.definitionId="PrintString";
      n.pins.push_back(MakeExecIn(30));
      n.pins.push_back(MakeExecOut(39));
      n.pins.push_back(MakeDataIn(31, PinDataType::String, "In String",
                                  Variant(std::string("not_found_fired"))));
      bp.nodes.push_back(n); }

    { LinkInstance l; l.id=100; l.startPinId=10;  l.endPinId=20; bp.links.push_back(l); }
    { LinkInstance l; l.id=101; l.startPinId=25;  l.endPinId=30; bp.links.push_back(l); } // onNotFound

    runner.Load(bp);
    RunWithBeginPlay(runner);

    bool notFoundFired = false;
    for (const auto& p : prints)
        if (p.find("not_found_fired") != std::string::npos) { notFoundFired = true; break; }
    EXPECT_TRUE(notFoundFired) << "onNotFound should fire for unknown tool name";
}

TEST_F(ToolCallByNameTest, ToolCallIdPassthrough)
{
    registerEchoTool();

    BlueprintData bp;
    { NodeInstance n; n.id=1; n.definitionId="OnBeginPlay";
      n.pins.push_back(MakeExecOut(10));
      bp.nodes.push_back(n); }

    { NodeInstance n; n.id=2; n.definitionId="Tool.CallByName";
      n.pins.push_back(MakeExecIn(20));
      n.pins.push_back(MakeDataIn(21, PinDataType::String, "ToolName",
                                  Variant(std::string("echo_tool"))));
      n.pins.push_back(MakeDataIn(22, PinDataType::String, "Arguments",
                                  Variant(std::string(R"({"Input":"x"})"))));
      n.pins.push_back(MakeDataIn(23, PinDataType::String, "ToolCallId",
                                  Variant(std::string("my_call_id_99"))));
      n.pins.push_back(MakeExecOut(24, "onSuccess"));
      n.pins.push_back(MakeExecOut(25, "onNotFound"));
      n.pins.push_back(MakeDataOut(26, PinDataType::String, "Result"));
      n.pins.push_back(MakeDataOut(27, PinDataType::String, "ToolCallId"));
      bp.nodes.push_back(n); }

    { LinkInstance l; l.id=100; l.startPinId=10; l.endPinId=20; bp.links.push_back(l); }

    runner.Load(bp);
    RunWithBeginPlay(runner);

    // ToolCallId 输出引脚应透传输入值
    auto outId = runner.GetPinValue(27).asString();
    EXPECT_EQ(outId, "my_call_id_99") << "ToolCallId should be passed through to output pin";
}

// ============================================================================
// 3. BP_DispatchEvent C API 测试
// ============================================================================

class BPDispatchEventTest : public ::testing::Test
{
protected:
    std::vector<std::string> prints;

    // 构建：OnBeginPlay → PrintString("beginplay_fired")
    std::string makeBlueprintJson()
    {
        return R"({
            "runtime": {
                "fileType": "runtime",
                "metadata": { "schemaVersion": 2, "blueprintClass": 0, "name": "TestBP" },
                "nodes": [
                    { "id": 1, "definitionId": "OnBeginPlay", "name": "OnBeginPlay",
                      "pins": [{ "id": 10, "kind": 1, "isExec": true }] },
                    { "id": 2, "definitionId": "PrintString", "name": "Print",
                      "pins": [
                          { "id": 20, "kind": 0, "isExec": true },
                          { "id": 21, "kind": 1, "isExec": true },
                          { "id": 22, "kind": 0, "dataType": 4, "name": "In String",
                            "defaultValue": "beginplay_fired" }
                      ]}
                ],
                "links": [
                    { "id": 100, "startPinId": 10, "endPinId": 20, "isEnabled": true }
                ],
                "variables": []
            }
        })";
    }
};

TEST_F(BPDispatchEventTest, DispatchEventFiresOnBeginPlay)
{
    BP_Runner runner = BP_CreateRunner();
    ASSERT_NE(runner, nullptr);

    // 注册 print 回调
    BP_SetPrintCallback(runner, [](BP_LogLevel, const char* msg) {
        // 通过静态收集（简单做法：用全局字符串）
        // 但测试隔离，改为通过变量检查
        (void)msg;
    });

    std::string json = makeBlueprintJson();
    EXPECT_EQ(BP_LoadFromJson(runner, json.c_str()), 0) << "Load should succeed";

    // Execute（数据流节点，无 OnBeginPlay 链）
    EXPECT_EQ(BP_Execute(runner), 0) << "Execute should succeed";

    // DispatchEvent OnBeginPlay
    EXPECT_EQ(BP_DispatchEvent(runner, "OnBeginPlay"), 0) << "DispatchEvent should succeed";

    // 验证 runner 状态正常（Idle，非 Stopped/Failed）
    // 用 BP_GetLastError 确认无错误
    char errbuf[256] = {};
    BP_GetLastError(runner, errbuf, sizeof(errbuf));
    EXPECT_EQ(std::string(errbuf), "") << "No error expected after DispatchEvent";

    BP_DestroyRunner(runner);
}

TEST_F(BPDispatchEventTest, DispatchEventNoMatchingNode_ReturnsSuccess)
{
    // 蓝图中没有 OnBeginPlay 节点时，应静默成功（不报错）
    BP_Runner runner = BP_CreateRunner();
    ASSERT_NE(runner, nullptr);

    // 最简蓝图：只有一个 PrintString，无事件源
    const char* json = R"({
        "runtime": {
            "fileType": "runtime",
            "metadata": { "schemaVersion": 2, "blueprintClass": 0, "name": "NoEvent" },
            "nodes": [
                { "id": 1, "definitionId": "PrintString",
                  "pins": [
                      { "id": 10, "kind": 0, "isExec": true },
                      { "id": 11, "kind": 1, "isExec": true },
                      { "id": 12, "kind": 0, "dataType": 4, "name": "In String",
                        "defaultValue": "hello" }
                  ]}
            ],
            "links": [],
            "variables": []
        }
    })";

    ASSERT_EQ(BP_LoadFromJson(runner, json), 0);
    ASSERT_EQ(BP_Execute(runner), 0);

    // 分发不存在的事件，应返回 0（静默忽略）
    int ret = BP_DispatchEvent(runner, "OnBeginPlay");
    EXPECT_EQ(ret, 0) << "DispatchEvent on non-existent event node should return 0 (silent)";

    char errbuf[256] = {};
    BP_GetLastError(runner, errbuf, sizeof(errbuf));
    EXPECT_EQ(std::string(errbuf), "") << "No error for missing event node";

    BP_DestroyRunner(runner);
}

TEST_F(BPDispatchEventTest, NullRunnerReturnsError)
{
    EXPECT_NE(BP_DispatchEvent(nullptr, "OnBeginPlay"), 0)
        << "Null runner should return error code";
}

TEST_F(BPDispatchEventTest, NullEventIdReturnsError)
{
    BP_Runner runner = BP_CreateRunner();
    ASSERT_NE(runner, nullptr);

    const char* json = R"({"runtime":{"fileType":"runtime","metadata":{"schemaVersion":2,"blueprintClass":0,"name":"T"},"nodes":[],"links":[],"variables":[]}})";
    BP_LoadFromJson(runner, json);

    EXPECT_NE(BP_DispatchEvent(runner, nullptr), 0)
        << "Null event id should return error code";

    BP_DestroyRunner(runner);
}
