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
