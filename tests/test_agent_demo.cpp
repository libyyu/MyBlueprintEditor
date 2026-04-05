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
        return std::string(TEST_PROJECT_DIR) + "/examples/AgentDemo.bjson";
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
            for (const auto& tok : cfg.chunks) {
                std::this_thread::sleep_for(std::chrono::milliseconds(cfg.delayMs));
                onChunk(tok);
            }
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
