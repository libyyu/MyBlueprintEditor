// tests/test_agent_demo.cpp
// 加载 AgentDemo.bjson，用 Mock HttpClient 模拟 OpenAI 响应，验证完整 Agent 流程：
//   OnBeginPlay
//     → String.Template（拼 prompt）
//     → JSON.MakeMessage（打包消息）
//     → JSON.ArrayPush（构建 messages 数组）
//     → LLM.Chat（mock HTTP，返回假回复）
//         → onReply → FormatString → PrintString（打印 [AI] 前缀的回复）
//         → onError → FormatString → PrintString（打印 [ERR] 前缀的错误）

#include <gtest/gtest.h>
#include "../Runtime/BlueprintRunner.h"
#include "../Runtime/BlueprintExporter.h"
#include "../Runtime/BuiltinHandlers.h"
#include "../Runtime/Http/IHttpClient.h"
#include "../Runtime/MainThreadDispatcher.h"
#include "test_helpers.h"
#include <fstream>
#include <thread>
#include <chrono>

using namespace NodeEditor::Runtime;
using namespace TestHelpers;

// ============================================================================
// Mock HttpClient：后台线程延迟回调，模拟真实异步行为
// ============================================================================
class MockHttpClient : public IHttpClient
{
public:
    struct Config {
        int         statusCode = 200;
        std::string body;
        std::string error;          // 非空 → 网络层错误
    };

    explicit MockHttpClient(Config cfg) : m_cfg(std::move(cfg)) {}

    void SendAsync(const HttpRequest& /*req*/, HttpCallback cb) override
    {
        // 后台线程延迟回调，模拟网络往返
        std::thread([cfg = m_cfg, cb = std::move(cb)]() mutable {
            std::this_thread::sleep_for(std::chrono::milliseconds(20));
            HttpResponse resp;
            resp.statusCode = cfg.statusCode;
            resp.body       = cfg.body;
            resp.error      = cfg.error;
            cb(std::move(resp));
        }).detach();
    }

private:
    Config m_cfg;
};

// ============================================================================
// Test fixture
// ============================================================================
class AgentDemoTest : public ::testing::Test
{
protected:
    BlueprintRunner runner;
    std::vector<std::string> logs;
    std::vector<std::string> prints;

    void SetUp() override {
        RegisterBuiltinHandlers(runner, ".");
        runner.SetLogCallback([this](LogLevel, const std::string& msg) {
            logs.push_back(msg);
        });
        runner.SetPrintCallback([this](LogLevel, const std::string& msg) {
            prints.push_back(msg);
        });
    }

    bool loadDemo() {
        JsonBlueprintExporter exp;
        auto result = exp.importRuntimeFromFile(demoPath());
        if (!result.success) {
            ADD_FAILURE() << "importRuntimeFromFile failed: " << result.errorMessage;
            return false;
        }
        if (!runner.Load(result.data)) {
            ADD_FAILURE() << "runner.Load failed";
            return false;
        }
        return true;
    }

    // RunWithBeginPlay：Execute + DispatchEvent("OnBeginPlay") + HasPendingAsync Tick 循环
    // 比固定 ms 等待更可靠：只要 async 计数归零就退出
    ExecutionResult run() {
        return RunWithBeginPlay(runner);
    }

    static std::string demoPath() {
        return std::string(TEST_DATA_DIR) + "/examples/AgentDemo.bjson";
    }
};

// ============================================================================
// 成功路径：Mock 返回正常 OpenAI 格式响应
// ============================================================================
TEST_F(AgentDemoTest, FullAgentFlow_ReplyPath)
{
    std::string fakeBody = R"({
        "id": "chatcmpl-mock",
        "object": "chat.completion",
        "choices": [{
            "index": 0,
            "message": {
                "role": "assistant",
                "content": "大型语言模型（LLM）是基于Transformer架构、在海量文本上预训练的神经网络，能够理解和生成自然语言。"
            },
            "finish_reason": "stop"
        }],
        "usage": { "prompt_tokens": 50, "completion_tokens": 40, "total_tokens": 90 }
    })";

    BP_SetHttpClient(std::make_shared<MockHttpClient>(
        MockHttpClient::Config{ 200, fakeBody, "" }));

    std::ifstream f(demoPath());
    ASSERT_TRUE(f.good()) << "AgentDemo.bjson not found at: " << demoPath();
    f.close();
    ASSERT_TRUE(loadDemo());

    run();

    bool foundReply = false;
    for (const auto& p : prints) {
        if (p.find("[AI]") != std::string::npos) {
            foundReply = true;
            EXPECT_NE(p.find("大型语言模型"), std::string::npos)
                << "Reply should contain mock LLM response. Got: " << p;
            break;
        }
    }

    if (!foundReply) {
        // 输出所有 log/print 辅助诊断
        for (const auto& l : logs)   std::cerr << "[LOG]   " << l << "\n";
        for (const auto& p : prints) std::cerr << "[PRINT] " << p << "\n";
    }
    EXPECT_TRUE(foundReply) << "Should have printed [AI] reply";
}

// ============================================================================
// 失败路径：Mock 返回 HTTP 401 + 错误 JSON
// ============================================================================
TEST_F(AgentDemoTest, FullAgentFlow_ErrorPath)
{
    std::string errBody = R"({
        "error": {
            "message": "Invalid API key.",
            "type": "invalid_request_error",
            "code": "invalid_api_key"
        }
    })";

    BP_SetHttpClient(std::make_shared<MockHttpClient>(
        MockHttpClient::Config{ 401, errBody, "" }));

    std::ifstream f(demoPath());
    ASSERT_TRUE(f.good());
    f.close();
    ASSERT_TRUE(loadDemo());

    run();

    bool foundErr = false;
    for (const auto& p : prints) {
        if (p.find("[ERR]") != std::string::npos) {
            foundErr = true;
            EXPECT_NE(p.find("Invalid API key"), std::string::npos)
                << "Error should mention API key. Got: " << p;
            break;
        }
    }

    if (!foundErr) {
        for (const auto& l : logs)   std::cerr << "[LOG]   " << l << "\n";
        for (const auto& p : prints) std::cerr << "[PRINT] " << p << "\n";
    }
    EXPECT_TRUE(foundErr) << "Should have printed [ERR] error";
}

// ============================================================================
// 网络层失败：连接拒绝
// ============================================================================
TEST_F(AgentDemoTest, FullAgentFlow_NetworkError)
{
    BP_SetHttpClient(std::make_shared<MockHttpClient>(
        MockHttpClient::Config{ 0, "", "Connection refused" }));

    std::ifstream f(demoPath());
    ASSERT_TRUE(f.good());
    f.close();
    ASSERT_TRUE(loadDemo());

    run();

    bool foundErr = false;
    for (const auto& p : prints)
        if (p.find("[ERR]") != std::string::npos) { foundErr = true; break; }

    if (!foundErr) {
        for (const auto& l : logs)   std::cerr << "[LOG]   " << l << "\n";
        for (const auto& p : prints) std::cerr << "[PRINT] " << p << "\n";
    }
    EXPECT_TRUE(foundErr) << "Network error should route to [ERR] print";
}


// ============================================================================
// MockStreamHttpClient
//   支持 StreamAsync：把 chunks 逐个模拟 SSE delta，最后调 onDone
// ============================================================================
class MockStreamHttpClient : public IHttpClient
{
public:
    struct Config {
        std::vector<std::string> chunks;
        std::string              error;
        int                      delayMs = 5;
    };

    explicit MockStreamHttpClient(Config cfg) : m_cfg(std::move(cfg)) {}

    void SendAsync(const HttpRequest& /*req*/, HttpCallback cb) override
    {
        std::thread([cfg = m_cfg, cb = std::move(cb)]() mutable {
            std::this_thread::sleep_for(std::chrono::milliseconds(cfg.delayMs));
            HttpResponse resp;
            if (!cfg.error.empty()) {
                resp.error = cfg.error;
            } else {
                resp.statusCode = 200;
                for (const auto& c : cfg.chunks) resp.body += c;
            }
            cb(std::move(resp));
        }).detach();
    }

    void StreamAsync(const HttpRequest& /*req*/,
                     HttpChunkCallback onChunk,
                     HttpDoneCallback  onDone) override
    {
        std::thread([cfg = m_cfg,
                     onChunk = std::move(onChunk),
                     onDone  = std::move(onDone)]() mutable
        {
            if (!cfg.error.empty()) {
                std::this_thread::sleep_for(std::chrono::milliseconds(cfg.delayMs));
                onDone(cfg.error);
                return;
            }
            // 将每个文本 chunk 包装为 SSE JSON（与 HttpClient_Default 的 parseSseLine 输出一致）
            for (size_t i = 0; i < cfg.chunks.size(); ++i) {
                std::this_thread::sleep_for(std::chrono::milliseconds(cfg.delayMs));
                // 最后一个 chunk 附带 finish_reason=stop
                std::string fr = (i + 1 == cfg.chunks.size()) ? "\"stop\"" : "null";
                // 转义 content 中的引号
                std::string content = cfg.chunks[i];
                std::string escaped;
                for (char c : content) {
                    if (c == '"') escaped += "\\\"";
                    else if (c == '\\') escaped += "\\\\";
                    else escaped += c;
                }
                std::string sseJson =
                    "{\"choices\":[{\"delta\":{\"content\":\"" + escaped + "\"},"
                    "\"finish_reason\":" + fr + "}]}";
                onChunk(sseJson);
            }
            onChunk("[DONE]");
            onDone("");
        }).detach();
    }

private:
    Config m_cfg;
};

// ============================================================================
// StreamChatTest fixture
// ============================================================================
class StreamChatTest : public ::testing::Test
{
protected:
    BlueprintRunner runner;
    std::vector<std::string> prints;

    std::vector<std::string> logs;
    void SetUp() override {
        RegisterBuiltinHandlers(runner, ".");
        runner.SetLogCallback([this](LogLevel, const std::string& msg) {
            logs.push_back(msg);
        });
        runner.SetPrintCallback([this](LogLevel, const std::string& msg) {
            prints.push_back(msg);
        });
    }

    // 构建极简蓝图：OnBeginPlay → LLM.StreamChat（三路出口接 PrintString）
    BlueprintData buildBP() {
        BlueprintData bp;
        bp.metadata.name = "StreamChatTestBP";

        auto mkExec = [](uint64_t id, PinKind k, const char* name) {
            PinInfo p; p.id=id; p.kind=k; p.dataType=PinDataType::Any;
            p.name=name; p.isExec=true; return p;
        };
        auto mkPin = [](uint64_t id, PinKind k, PinDataType dt, const char* name,
                        Variant dv = {}) {
            PinInfo p; p.id=id; p.kind=k; p.dataType=dt;
            p.name=name; p.defaultValue=dv; return p;
        };
        auto addLink = [&](uint64_t id, uint64_t from, uint64_t to) {
            LinkInstance lnk; lnk.id=(LinkId)id;
            lnk.startPinId=(PinId)from; lnk.endPinId=(PinId)to;
            bp.links.push_back(lnk);
        };

        // Node 1: OnBeginPlay
        { NodeInstance n; n.id=1; n.definitionId="OnBeginPlay";
          n.pins.push_back(mkExec(10, PinKind::Output, ""));
          bp.nodes.push_back(n); }

        // Node 2: LLM.StreamChat
        { NodeInstance n; n.id=2; n.definitionId="LLM.StreamChat";
          n.pins.push_back(mkExec(20, PinKind::Input,  ""));
          n.pins.push_back(mkPin (21, PinKind::Input,  PinDataType::String,  "BaseURL",
                                   Variant(std::string("https://api.openai.com/v1"))));
          n.pins.push_back(mkPin (22, PinKind::Input,  PinDataType::String,  "ApiKey",
                                   Variant(std::string("sk-test"))));
          n.pins.push_back(mkPin (23, PinKind::Input,  PinDataType::String,  "Model",
                                   Variant(std::string("gpt-4o"))));
          n.pins.push_back(mkPin (24, PinKind::Input,  PinDataType::String,  "Messages",
                                   Variant(std::string(R"([{"role":"user","content":"hi"}])"))));
          n.pins.push_back(mkPin (25, PinKind::Input,  PinDataType::String,  "SystemPrompt",
                                   Variant(std::string(""))));
          n.pins.push_back(mkPin (26, PinKind::Input,  PinDataType::Integer, "MaxTokens",
                                   Variant((int64_t)256)));
          n.pins.push_back(mkPin (27, PinKind::Input,  PinDataType::Float,   "Temperature",
                                   Variant((double)0.7)));
          n.pins.push_back(mkPin (28, PinKind::Input,  PinDataType::String,  "Tools",
                                   Variant(std::string(""))));
          n.pins.push_back(mkExec(29, PinKind::Output, "onChunk"));
          n.pins.push_back(mkExec(30, PinKind::Output, "onDone"));
          n.pins.push_back(mkExec(31, PinKind::Output, "onError"));
          n.pins.push_back(mkPin (32, PinKind::Output, PinDataType::String,  "Token"));
          n.pins.push_back(mkPin (33, PinKind::Output, PinDataType::String,  "FullText"));
          n.pins.push_back(mkPin (34, PinKind::Output, PinDataType::String,  "ErrorMessage"));
          bp.nodes.push_back(n); }

        // Node 3: PrintString（onChunk → Token）
        { NodeInstance n; n.id=3; n.definitionId="PrintString";
          n.pins.push_back(mkExec(40, PinKind::Input,  ""));
          n.pins.push_back(mkExec(41, PinKind::Output, ""));
          n.pins.push_back(mkPin (42, PinKind::Input,  PinDataType::String, "In String",
                                   Variant(std::string(""))));
          bp.nodes.push_back(n); }

        // Node 4: PrintString（onDone → FullText）
        { NodeInstance n; n.id=4; n.definitionId="PrintString";
          n.pins.push_back(mkExec(50, PinKind::Input,  ""));
          n.pins.push_back(mkExec(51, PinKind::Output, ""));
          n.pins.push_back(mkPin (52, PinKind::Input,  PinDataType::String, "In String",
                                   Variant(std::string(""))));
          bp.nodes.push_back(n); }

        // Node 5: PrintString（onError → ErrorMessage）
        { NodeInstance n; n.id=5; n.definitionId="PrintString";
          n.pins.push_back(mkExec(60, PinKind::Input,  ""));
          n.pins.push_back(mkExec(61, PinKind::Output, ""));
          n.pins.push_back(mkPin (62, PinKind::Input,  PinDataType::String, "In String",
                                   Variant(std::string(""))));
          bp.nodes.push_back(n); }

        // ── 连线 ──
        addLink(100, 10, 20);  // OnBeginPlay → LLM.StreamChat.in
        addLink(101, 29, 40);  // onChunk     → PrintString(3).in
        addLink(102, 32, 42);  // Token       → PrintString(3).Message
        addLink(103, 30, 50);  // onDone      → PrintString(4).in
        addLink(104, 33, 52);  // FullText    → PrintString(4).Message
        addLink(105, 31, 60);  // onError     → PrintString(5).in
        addLink(106, 34, 62);  // ErrorMessage→ PrintString(5).Message

        return bp;
    }

    // 运行并驱动 Tick 循环（与 AgentDemoTest 保持一致）
    void runAndDrain(BlueprintData& bp) {
        runner.Load(bp);
        RunWithBeginPlay(runner);  // Execute() + DispatchEvent + Tick loop
    }
};

// ── Test 1：正常流式，3 chunk ────────────────────────────────────────────────
TEST_F(StreamChatTest, StreamChat_ChunksArriveThenDone)
{
    std::vector<std::string> chunks = {"Hello", ", ", "world!"};
    BP_SetHttpClient(std::make_shared<MockStreamHttpClient>(
        MockStreamHttpClient::Config{ chunks, "", 5 }));

    auto bp = buildBP();
    runAndDrain(bp);

    // onChunk 激活 3 次 → prints 包含 3 个 token
    std::string combined;
    int chunkPrints = 0;
    for (const auto& p : prints) {
        if (p == "Hello" || p == ", " || p == "world!") {
            chunkPrints++;
            combined += p;
        }
    }
    EXPECT_EQ(chunkPrints, 3) << "Should have 3 chunk prints";
    EXPECT_EQ(combined, "Hello, world!");

    // onDone → PrintString(4) 打印 FullText
    bool foundDone = false;
    for (const auto& p : prints)
        if (p == "Hello, world!") { foundDone = true; break; }
    EXPECT_TRUE(foundDone) << "FullText should equal assembled chunks";

    // onError 不触发
    for (const auto& p : prints)
        EXPECT_EQ(p.find("Connection"), std::string::npos);
}

// ── Test 2：单 chunk ─────────────────────────────────────────────────────────
TEST_F(StreamChatTest, StreamChat_SingleChunk)
{
    BP_SetHttpClient(std::make_shared<MockStreamHttpClient>(
        MockStreamHttpClient::Config{ {"Hello"}, "", 5 }));

    auto bp = buildBP();
    runAndDrain(bp);

    bool found = false;
    for (const auto& p : prints)
        if (p == "Hello") { found = true; break; }
    EXPECT_TRUE(found) << "Single chunk token should be printed";
}

// ── Test 3：空 chunks（直接 done）───────────────────────────────────────────
TEST_F(StreamChatTest, StreamChat_EmptyChunks_NoCrash)
{
    BP_SetHttpClient(std::make_shared<MockStreamHttpClient>(
        MockStreamHttpClient::Config{ {}, "", 5 }));

    auto bp = buildBP();
    EXPECT_NO_THROW(runAndDrain(bp));

    // onError 不触发
    for (const auto& p : prints)
        EXPECT_EQ(p.find("ERR"), std::string::npos);
}

// ── Test 4：网络错误路由到 onError ───────────────────────────────────────────
TEST_F(StreamChatTest, StreamChat_NetworkError_RoutesToOnError)
{
    BP_SetHttpClient(std::make_shared<MockStreamHttpClient>(
        MockStreamHttpClient::Config{ {}, "Connection refused", 5 }));

    auto bp = buildBP();
    runAndDrain(bp);

    bool foundErr = false;
    for (const auto& p : prints)
        if (p.find("Connection refused") != std::string::npos) { foundErr = true; break; }
    EXPECT_TRUE(foundErr) << "Error message should propagate to onError PrintString";
}

// ── Test 5：无 HttpClient 注册（同步报错）────────────────────────────────────
TEST_F(StreamChatTest, StreamChat_NoClient_SyncError)
{
    BP_SetHttpClient(nullptr);

    auto bp = buildBP();
    runAndDrain(bp);

    bool foundErr = false;
    for (const auto& p : prints)
        if (p.find("No HTTP client") != std::string::npos) { foundErr = true; break; }
    EXPECT_TRUE(foundErr) << "Should report missing client synchronously";
}

// ============================================================================
// 并行工具调用测试：Tool.ForEach 处理多个 tool_calls，收集结果后再调 LLM
//
// 蓝图结构（自驱动循环，无需 ForLoopWithBreak）：
//
//  OnBeginPlay
//    → SetVar(messages="[]")
//    → SetVar(done=false)
//    → JSON.MakeMessage(user)
//    → JSON.ArrayPush(messages)
//    → SetVar(messages)
//    → [LLM_NODE] LLM.Chat
//        onReply  → SetVar(finalReply) → SetVar(done=true)   [终止]
//        onToolCall
//          → Tool.ForEach
//              onTool → Tool.Match
//                Match0(get_weather)
//                  → JSON.GetPath(location)
//                  → FormatString
//                  → JSON.MakeMessage(tool)
//                  → GetVar(messages)
//                  → JSON.ArrayPush
//                  → SetVar(messages)
//              onDone → GetVar(messages) → [LLM_NODE] (递归触发下一轮)
//        onError  → SetVar(done=true)    [终止]
//
// LLM mock：第1次返回 tool_calls(c1,c2)，第2次返回最终答案
// ============================================================================
TEST_F(AgentDemoTest, ParallelToolCalls_AllToolsProcessed)
{
    // ── Mock：第1次返回 2 个 tool_calls，第2次返回最终答案 ───────────────────
    class MultiMock : public IHttpClient {
    public:
        std::atomic<int> callCount{0};
        void SendAsync(const HttpRequest&, HttpCallback cb) override {
            int n = ++callCount;
            std::thread([n, cb = std::move(cb)]() mutable {
                std::this_thread::sleep_for(std::chrono::milliseconds(5));
                HttpResponse r; r.statusCode = 200;
                if (n == 1) {
                    r.body = R"({"choices":[{"message":{"role":"assistant","content":null,)"
                             R"("tool_calls":[)"
                             R"({"id":"c1","type":"function","function":{"name":"get_weather","arguments":"{\"location\":\"Beijing\"}"}},)"
                             R"({"id":"c2","type":"function","function":{"name":"get_weather","arguments":"{\"location\":\"Shanghai\"}"}})"
                             R"(]},"finish_reason":"tool_calls"}]})";
                } else {
                    r.body = R"({"choices":[{"message":{"role":"assistant","content":"Beijing: Sunny 25C; Shanghai: Cloudy 18C"},"finish_reason":"stop"}]})";
                }
                cb(std::move(r));
            }).detach();
        }
    };
    auto multiMock = std::make_shared<MultiMock>();
    BP_SetHttpClient(multiMock);

    // ── 蓝图：CustomEventNode("AgentLoop") + FireEvent("AgentLoop") 无环设计 ──
    //
    // 无 exec cycle：onDone → FireEvent("AgentLoop")，FireEvent 异步 post 到
    // MainThreadDispatcher，在下一帧触发 CustomEventNode("AgentLoop") 的链路。
    //
    // 图结构：
    //
    //  [OnBeginPlay]
    //    → SV(messages="[]")
    //    → SV(tools=...)
    //    → JSON.ArrayPush(userMsg)
    //    → SV(messages)
    //    → FireEvent("AgentLoop")       ← 初始触发
    //
    //  [CustomEventNode "AgentLoop"]   ← 每轮 LLM 的入口
    //    → GetVar(messages) → LLM.Messages
    //    → GetVar(tools)    → LLM.Tools
    //    → LLM.Chat
    //        onReply   → SV(finalReply)
    //        onToolCall → Tool.ForEach
    //                       onTool → Tool.Match
    //                                  Match0(get_weather)
    //                                    → GetPath(location)
    //                                    → FormatString
    //                                    → MakeMessage(tool)
    //                                    → ArrayPush(messages)
    //                                    → SV(messages)
    //                       onDone → FireEvent("AgentLoop")  ← 触发下一轮
    //
    BlueprintData bp; bp.metadata.name = "ParallelToolTest";

    static int gLinkId = 9100;    // 避免与其他测试 link id 冲突
    auto addP = [](NodeInstance& n, uint64_t id, PinKind k, PinDataType dt,
                   const char* nm, bool isExec=false, Variant dv={}) {
        PinInfo p; p.id=id; p.kind=k; p.dataType=dt; p.name=nm;
        p.isExec=isExec; p.defaultValue=dv; n.pins.push_back(p);
    };
    auto SV = [&](int id, uint64_t ei, uint64_t ni, uint64_t vi, uint64_t eo,
                  const char* vn, Variant val={}, PinDataType vt=PinDataType::String) {
        NodeInstance n; n.id=id; n.definitionId="SetVariable";
        addP(n,ei,PinKind::Input, PinDataType::Unknown,"",    true);
        addP(n,ni,PinKind::Input, PinDataType::String, "Name",false,Variant(std::string(vn)));
        addP(n,vi,PinKind::Input, vt,                  "Value",false,val);
        addP(n,eo,PinKind::Output,PinDataType::Unknown,"",    true);
        return n;
    };
    auto GV = [&](int id, uint64_t ni, uint64_t vo,
                  const char* vn, PinDataType vt=PinDataType::String) {
        NodeInstance n; n.id=id; n.definitionId="GetVariable";
        addP(n,ni,PinKind::Input, PinDataType::String,"Name",false,Variant(std::string(vn)));
        addP(n,vo,PinKind::Output,vt,                 "Value");
        return n;
    };
    auto FE = [&](int id, uint64_t ei, uint64_t ni, uint64_t eo, const char* evtName) {
        NodeInstance n; n.id=id; n.definitionId="FireEvent";
        addP(n,ei,PinKind::Input, PinDataType::Unknown,"",         true);
        addP(n,ni,PinKind::Input, PinDataType::String, "EventName",false,Variant(std::string(evtName)));
        addP(n,eo,PinKind::Output,PinDataType::Unknown,"",         true);
        return n;
    };
    auto addLink = [&](uint64_t s, uint64_t e) {
        LinkInstance l; l.id=++gLinkId; l.startPinId=s; l.endPinId=e;
        bp.links.push_back(l);
    };

    // ── 节点 ─────────────────────────────────────────────────────────────────

    // 1 OnBeginPlay
    { NodeInstance n; n.id=1; n.definitionId="OnBeginPlay";
      addP(n,1001,PinKind::Output,PinDataType::Unknown,"",true); bp.nodes.push_back(n); }

    // 2 SetVar(messages="[]")
    bp.nodes.push_back(SV(2,2001,2002,2003,2010,"messages",Variant(std::string("[]"))));

    // 3 SetVar(tools=toolsJson)
    static const std::string kToolsJson =
        R"([{"type":"function","function":{"name":"get_weather","description":"Get weather","parameters":{"type":"object","properties":{"location":{"type":"string"}},"required":["location"]}}}])";
    bp.nodes.push_back(SV(3,3001,3002,3003,3010,"tools",Variant(kToolsJson)));

    // 4 JSON.MakeMessage(user)
    { NodeInstance n; n.id=4; n.definitionId="JSON.MakeMessage";
      addP(n,4001,PinKind::Input, PinDataType::String,"Role",   false,Variant(std::string("user")));
      addP(n,4002,PinKind::Input, PinDataType::String,"Content",false,
           Variant(std::string("Weather in Beijing and Shanghai?")));
      addP(n,4010,PinKind::Output,PinDataType::String,"JSON");
      bp.nodes.push_back(n); }

    // 5 JSON.ArrayPush(push userMsg into messages)
    { NodeInstance n; n.id=5; n.definitionId="JSON.ArrayPush";
      addP(n,5001,PinKind::Input, PinDataType::Unknown,"",     true);
      addP(n,5002,PinKind::Input, PinDataType::String, "JSON", false,Variant(std::string("[]")));
      addP(n,5003,PinKind::Input, PinDataType::Any,    "Element");
      addP(n,5010,PinKind::Output,PinDataType::Unknown,"",     true);
      addP(n,5011,PinKind::Output,PinDataType::String, "JSON");
      bp.nodes.push_back(n); }

    // 6 SetVar(messages = 5.JSON)
    bp.nodes.push_back(SV(6,6001,6002,6003,6010,"messages"));

    // 7 FireEvent("AgentLoop") — 初始触发第一轮
    bp.nodes.push_back(FE(7, 7001, 7002, 7010, "AgentLoop"));

    // ── CustomEventNode("AgentLoop") ──────────────────────────────────────────
    // 10 CustomEventNode — 每轮 LLM 的入口事件
    { NodeInstance n; n.id=10; n.definitionId="CustomEventNode";
      addP(n,10010,PinKind::Output,PinDataType::Unknown,"",        true);
      addP(n,10011,PinKind::Output,PinDataType::String, "EventName");
      n.name = "AgentLoop";
      // nodeData 用于事件匹配
      n.nodeData["EventName"] = Variant(std::string("AgentLoop"));
      bp.nodes.push_back(n); }

    // 11 GetVar(messages) — LLM 数据输入
    bp.nodes.push_back(GV(11,11001,11010,"messages"));

    // 12 GetVar(tools) — LLM 数据输入
    bp.nodes.push_back(GV(12,12001,12010,"tools"));

    // 20 LLM.Chat
    { NodeInstance n; n.id=20; n.definitionId="LLM.Chat";
      addP(n,20001,PinKind::Input, PinDataType::Unknown,"",          true);
      addP(n,20002,PinKind::Input, PinDataType::String, "BaseURL",   false,Variant(std::string("https://api.openai.com/v1")));
      addP(n,20003,PinKind::Input, PinDataType::String, "ApiKey",    false,Variant(std::string("sk-test")));
      addP(n,20004,PinKind::Input, PinDataType::String, "Model",     false,Variant(std::string("gpt-4o")));
      addP(n,20005,PinKind::Input, PinDataType::String, "Messages");
      addP(n,20006,PinKind::Input, PinDataType::String, "SystemPrompt",false,Variant(std::string("")));
      addP(n,20007,PinKind::Input, PinDataType::Integer,"MaxTokens", false,Variant((int64_t)512));
      addP(n,20008,PinKind::Input, PinDataType::Float,  "Temperature",false,Variant(0.7));
      addP(n,20009,PinKind::Input, PinDataType::String, "Tools");
      addP(n,20010,PinKind::Output,PinDataType::Unknown,"onReply",   true);
      addP(n,20011,PinKind::Output,PinDataType::Unknown,"onToolCall",true);
      addP(n,20012,PinKind::Output,PinDataType::Unknown,"onError",   true);
      addP(n,20020,PinKind::Output,PinDataType::String, "Reply");
      addP(n,20021,PinKind::Output,PinDataType::String, "ToolCallsJSON");
      addP(n,20022,PinKind::Output,PinDataType::String, "FinishReason");
      addP(n,20023,PinKind::Output,PinDataType::String, "FullResponse");
      addP(n,20024,PinKind::Output,PinDataType::String, "ErrorMessage");
      bp.nodes.push_back(n); }

    // 21 SetVar(finalReply) — onReply 分支
    bp.nodes.push_back(SV(21,21001,21002,21003,21010,"finalReply"));

    // 30 Tool.ForEach — onToolCall 分支
    { NodeInstance n; n.id=30; n.definitionId="Tool.ForEach";
      addP(n,30001,PinKind::Input, PinDataType::Unknown,"",           true);
      addP(n,30002,PinKind::Input, PinDataType::String, "ToolCallsJSON");
      addP(n,30010,PinKind::Output,PinDataType::Unknown,"onTool",     true);
      addP(n,30011,PinKind::Output,PinDataType::Unknown,"onDone",     true);
      addP(n,30020,PinKind::Output,PinDataType::String, "ToolName");
      addP(n,30021,PinKind::Output,PinDataType::String, "Arguments");
      addP(n,30022,PinKind::Output,PinDataType::String, "ToolCallId");
      addP(n,30023,PinKind::Output,PinDataType::Integer,"Index");
      bp.nodes.push_back(n); }

    // 31 Tool.Match
    { NodeInstance n; n.id=31; n.definitionId="Tool.Match";
      addP(n,31001,PinKind::Input, PinDataType::Unknown,"",    true);
      addP(n,31002,PinKind::Input, PinDataType::String, "ToolName");
      addP(n,31003,PinKind::Input, PinDataType::String, "Case0",false,Variant(std::string("get_weather")));
      for (int i=1;i<8;++i) {
          std::string cn="Case"+std::to_string(i);
          addP(n,31010+i,PinKind::Input,PinDataType::String,cn.c_str(),false,Variant(std::string("")));
      }
      addP(n,31020,PinKind::Output,PinDataType::Unknown,"Match0",true);
      for (int i=1;i<8;++i) {
          std::string mn="Match"+std::to_string(i);
          addP(n,31030+i,PinKind::Output,PinDataType::Unknown,mn.c_str(),true);
      }
      addP(n,31028,PinKind::Output,PinDataType::Unknown,"Default",true);
      addP(n,31029,PinKind::Output,PinDataType::Integer,"MatchedIndex");
      bp.nodes.push_back(n); }

    // 32 JSON.GetPath(location)
    { NodeInstance n; n.id=32; n.definitionId="JSON.GetPath";
      addP(n,32001,PinKind::Input, PinDataType::String,"JSON");
      addP(n,32002,PinKind::Input, PinDataType::String,"Path",false,Variant(std::string("location")));
      addP(n,32010,PinKind::Output,PinDataType::String,"Value");
      addP(n,32011,PinKind::Output,PinDataType::Boolean,"Found");
      bp.nodes.push_back(n); }

    // 33 FormatString
    { NodeInstance n; n.id=33; n.definitionId="FormatString";
      addP(n,33001,PinKind::Input, PinDataType::String,"Format",false,
           Variant(std::string("Weather in {0}: Sunny 25C")));
      addP(n,33002,PinKind::Input, PinDataType::String,"Arg0");
      addP(n,33010,PinKind::Output,PinDataType::String,"Result");
      bp.nodes.push_back(n); }

    // 34 JSON.MakeMessage(role="tool")
    { NodeInstance n; n.id=34; n.definitionId="JSON.MakeMessage";
      addP(n,34001,PinKind::Input, PinDataType::String,"Role",   false,Variant(std::string("tool")));
      addP(n,34002,PinKind::Input, PinDataType::String,"Content");
      addP(n,34010,PinKind::Output,PinDataType::String,"JSON");
      bp.nodes.push_back(n); }

    // 35 GetVar(messages) — 纯数据，供 ArrayPush 读当前 messages
    bp.nodes.push_back(GV(35,35001,35010,"messages"));

    // 36 JSON.ArrayPush(messages, toolMsg)
    { NodeInstance n; n.id=36; n.definitionId="JSON.ArrayPush";
      addP(n,36001,PinKind::Input, PinDataType::Unknown,"",     true);
      addP(n,36002,PinKind::Input, PinDataType::String, "JSON");
      addP(n,36003,PinKind::Input, PinDataType::Any,    "Element");
      addP(n,36010,PinKind::Output,PinDataType::Unknown,"",     true);
      addP(n,36011,PinKind::Output,PinDataType::String, "JSON");
      bp.nodes.push_back(n); }

    // 37 SetVar(messages = 36.JSON)
    bp.nodes.push_back(SV(37,37001,37002,37003,37010,"messages"));

    // 40 FireEvent("AgentLoop") — onDone 触发下一轮
    bp.nodes.push_back(FE(40, 40001, 40002, 40010, "AgentLoop"));

    // ── Links ─────────────────────────────────────────────────────────────────

    // OnBeginPlay → init chain → FireEvent("AgentLoop")
    addLink(1001, 2001);   // OnBeginPlay → SV(messages="[]")
    addLink(2010, 3001);   // → SV(tools=...)
    addLink(3010, 5001);   // → ArrayPush.execIn
    addLink(4010, 5003);   // MakeMessage.JSON → ArrayPush.Element
    addLink(5010, 6001);   // ArrayPush.execOut → SV(messages)
    addLink(5011, 6003);   // ArrayPush.JSON → SV.Value
    addLink(6010, 7001);   // SV(messages) → FireEvent.execIn
    // FireEvent 的 EventName 使用默认值 "AgentLoop"

    // CustomEventNode("AgentLoop") → LLM
    addLink(10010, 20001); // CustomEvent.exec → LLM.execIn
    addLink(11010, 20005); // GetVar(messages) → LLM.Messages
    addLink(12010, 20009); // GetVar(tools) → LLM.Tools

    // LLM.onReply → SetVar(finalReply)
    addLink(20010, 21001); // LLM.onReply → SV.execIn
    addLink(20020, 21003); // LLM.Reply → SV.Value

    // LLM.onToolCall → Tool.ForEach
    addLink(20011, 30001); // LLM.onToolCall → ForEach.execIn
    addLink(20021, 30002); // LLM.ToolCallsJSON → ForEach.ToolCallsJSON

    // ForEach.onTool → Tool.Match
    addLink(30010, 31001); // ForEach.onTool → Match.execIn
    addLink(30020, 31002); // ForEach.ToolName → Match.ToolName

    // Match0(get_weather) → 处理链
    addLink(31020, 36001); // Match0 → ArrayPush.execIn
    addLink(30021, 32001); // ForEach.Arguments → GetPath.JSON
    addLink(32010, 33002); // GetPath.Value → FormatString.Arg0
    addLink(33010, 34002); // FormatString.Result → MakeMessage.Content
    addLink(34010, 36003); // MakeMessage.JSON → ArrayPush.Element
    addLink(35010, 36002); // GetVar(messages) → ArrayPush.JSON
    addLink(36010, 37001); // ArrayPush.execOut → SV(messages).execIn
    addLink(36011, 37003); // ArrayPush.JSON → SV.Value

    // ForEach.onDone → FireEvent("AgentLoop")
    addLink(30011, 40001); // ForEach.onDone → FireEvent.execIn

    bp.rebuildIndices();

    BlueprintRunner r;
    RegisterBuiltinHandlers(r, ".");
    ASSERT_TRUE(r.Load(bp));

    r.Execute();
    r.DispatchEvent("OnBeginPlay");

    // 等待两轮 LLM 异步完成（FireEvent 会异步 post 到 MainThreadDispatcher）
    auto start = std::chrono::steady_clock::now();
    for (int i = 0; i < 2000; ++i) {
        MainThreadDispatcher::Get().DrainQueue();
        if (multiMock->callCount.load() >= 2 &&
            !r.HasPendingAsync() &&
            !r.GetVariable("finalReply").asString().empty())
            break;
        if (std::chrono::steady_clock::now() - start > std::chrono::milliseconds(3000)) break;
        std::this_thread::sleep_for(std::chrono::milliseconds(2));
    }
    MainThreadDispatcher::Get().DrainQueue();

    EXPECT_EQ(multiMock->callCount.load(), 2)
        << "Should have called LLM twice: once for tool_calls, once for final answer";

    auto finalReply = r.GetVariable("finalReply").asString();
    EXPECT_NE(finalReply.find("Beijing"), std::string::npos)
        << "Final reply should mention Beijing, got: " << finalReply;
    EXPECT_NE(finalReply.find("Shanghai"), std::string::npos)
        << "Final reply should mention Shanghai, got: " << finalReply;

    BP_SetHttpClient(nullptr);
}

// ============================================================================
// ReActAgent.bjson 完整流程测试
// 第一轮 LLM：返回 tool_call（get_weather，location=Beijing）
// 第二轮 LLM：返回 final answer
// ============================================================================
class ReActAgentTest : public ::testing::Test
{
protected:
    BlueprintRunner runner;
    std::vector<std::string> logs;
    std::vector<std::string> prints;

    void SetUp() override {
        RegisterBuiltinHandlers(runner, ".");
        runner.SetLogCallback([this](LogLevel, const std::string& msg) {
            logs.push_back(msg);
        });
        runner.SetPrintCallback([this](LogLevel, const std::string& msg) {
            prints.push_back(msg);
        });
    }

    static std::string reactPath() {
        return std::string(TEST_DATA_DIR) + "/examples/ReActAgent.bjson";
    }
};

TEST_F(ReActAgentTest, FullReActLoop_ToolCall_ThenFinalAnswer)
{
    // 轮次1：tool_call（get_weather，location=Beijing）
    std::string round1 = R"({
        "id": "chatcmpl-round1",
        "object": "chat.completion",
        "choices": [{
            "index": 0,
            "message": {
                "role": "assistant",
                "content": null,
                "tool_calls": [{
                    "id": "call_abc1",
                    "type": "function",
                    "function": {
                        "name": "get_weather",
                        "arguments": "{\"location\": \"Beijing\"}"
                    }
                }]
            },
            "finish_reason": "tool_calls"
        }]
    })";

    // 轮次2：final answer
    std::string round2 = R"({
        "id": "chatcmpl-round2",
        "object": "chat.completion",
        "choices": [{
            "index": 0,
            "message": {
                "role": "assistant",
                "content": "根据工具返回结果，Beijing 当前天气晴，25°C，适合出行。"
            },
            "finish_reason": "stop"
        }]
    })";

    // 两轮顺序 mock（新版 ReActAgent 共 3 次 HTTP 调用）
    // call 0: LLM round1 → tool_call(get_weather, Beijing)
    // call 1: wttr.in    → 天气数据
    // call 2: LLM round2 → final answer
    class SeqMock : public IHttpClient {
    public:
        std::vector<std::string> bodies;
        std::atomic<int> callCount{0};
        SeqMock(std::string r1, std::string weather, std::string r2) {
            bodies.push_back(std::move(r1));
            bodies.push_back(std::move(weather));
            bodies.push_back(std::move(r2));
        }
        void SendAsync(const HttpRequest&, HttpCallback cb) override {
            int n = callCount.fetch_add(1);
            std::string body = (n < (int)bodies.size()) ? bodies[n] : bodies.back();
            std::thread([body, cb = std::move(cb)]() mutable {
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
                HttpResponse r; r.statusCode = 200; r.body = body;
                cb(std::move(r));
            }).detach();
        }
    };
    auto multiMock = std::make_shared<SeqMock>(round1, "Beijing: Sunny, 25°C", round2);
    BP_SetHttpClient(multiMock);

    JsonBlueprintExporter exp;
    auto result = exp.importRuntimeFromFile(reactPath());
    ASSERT_TRUE(result.success) << "importRuntimeFromFile failed: " << result.errorMessage;
    ASSERT_TRUE(runner.Load(result.data)) << "runner.Load failed";

    runner.Execute();
    runner.DispatchEvent("OnBeginPlay");

    // 等待三轮 HTTP 异步完成（LLM×2 + weather×1）
    auto start = std::chrono::steady_clock::now();
    for (int i = 0; i < 3000; ++i) {
        MainThreadDispatcher::Get().DrainQueue();
        if (multiMock->callCount.load() >= 3 &&
            !runner.HasPendingAsync())
            break;
        if (std::chrono::steady_clock::now() - start > std::chrono::milliseconds(8000)) break;
        std::this_thread::sleep_for(std::chrono::milliseconds(2));
    }
    MainThreadDispatcher::Get().DrainQueue();

    // 打印所有输出（测试目的：对比编辑器实际输出）
    std::cout << "\n===== ReActAgent execution trace =====\n";
    std::cout << "--- logs (" << logs.size() << ") ---\n";
    for (const auto& l : logs)   std::cout << "  [LOG]   " << l << "\n";
    std::cout << "--- prints (" << prints.size() << ") ---\n";
    for (const auto& p : prints) std::cout << "  [PRINT] " << p << "\n";
    std::cout << "--- LLM call count: " << multiMock->callCount.load() << " ---\n";
    std::cout << "======================================\n";

    EXPECT_EQ(multiMock->callCount.load(), 3)
        << "Should call HTTP 3 times: LLM round1 + wttr.in weather + LLM round2";

    // PrintString 节点（Print Final Answer）应打印含 Beijing 的文字
    bool foundFinalAnswer = false;
    for (const auto& p : prints) {
        if (p.find("Beijing") != std::string::npos ||
            p.find("北京") != std::string::npos ||
            p.find("天气") != std::string::npos) {
            foundFinalAnswer = true;
            break;
        }
    }
    EXPECT_TRUE(foundFinalAnswer)
        << "Final PrintString should contain weather result for Beijing";

    BP_SetHttpClient(nullptr);
}

