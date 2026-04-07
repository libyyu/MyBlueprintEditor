// Runtime/Http/HttpClient_Default.cpp
// cpp-httplib 实现（Windows / macOS / Linux / Android / iOS）
//
// 策略：
//   SendAsync  — 后台线程同步 HTTP，结果 dispatch 到主线程
//   StreamAsync— cpp-httplib ContentReceiver 解析 SSE，每个 delta.content
//                通过 MainThreadDispatcher.Post() 派回主线程触发 onChunk

#ifndef __EMSCRIPTEN__

#include "IHttpClient.h"
#include "../BlueprintRunner.h"
#include "../MainThreadDispatcher.h"

#define CPPHTTPLIB_NO_EXCEPTIONS
#include "httplib.h"

// crude_json 用于解析 SSE data 行中的 delta.content
#include "../../Utils/Json/crude_json.h"

#include <thread>
#include <sstream>

namespace NodeEditor {
namespace Runtime {

// ============================================================================
// URL 解析辅助
// ============================================================================

struct ParsedUrl {
    std::string scheme;
    std::string host;
    int         port = 0;
    std::string path;
};

static ParsedUrl parseUrl(const std::string& url)
{
    ParsedUrl result;
    auto schemeEnd = url.find("://");
    if (schemeEnd == std::string::npos) {
        result.scheme = "http";
        result.host   = url;
    } else {
        result.scheme = url.substr(0, schemeEnd);
        result.host   = url.substr(schemeEnd + 3);
    }
    auto pathPos = result.host.find('/');
    if (pathPos != std::string::npos) {
        result.path = result.host.substr(pathPos);
        result.host = result.host.substr(0, pathPos);
    } else {
        result.path = "/";
    }
    auto portPos = result.host.rfind(':');
    if (portPos != std::string::npos) {
        result.port = std::stoi(result.host.substr(portPos + 1));
        result.host = result.host.substr(0, portPos);
    } else {
        result.port = (result.scheme == "https") ? 443 : 80;
    }
    return result;
}

// ============================================================================
// SSE 行解析辅助
//   从 "data: {...}" 行提取原始 JSON 字符串（不做内容解析）
//   "[DONE]" 行返回 "" 并设置 isDone=true
//   由上层调用方（handler）负责解析 delta.content / delta.tool_calls 等字段
// ============================================================================

static std::string parseSseLine(const std::string& line, bool& isDone)
{
    isDone = false;
    if (line.empty() || line[0] == ':') return ""; // comment or empty
    if (line.rfind("data:", 0) != 0) return "";

    std::string data = line.substr(5);
    // 去前导空格
    size_t s = data.find_first_not_of(' ');
    if (s != std::string::npos) data = data.substr(s);

    if (data == "[DONE]") { isDone = true; return "[DONE]"; }

    // 快速检查是否合法 JSON（以 { 开头）
    if (data.empty() || data[0] != '{') return "";

    // 提取 finish_reason 判断流是否结束（不修改 data，仅检查）
    crude_json::value j = crude_json::value::parse(data);
    if (j.is_object() && j.contains("choices")) {
        const auto& choices = j["choices"];
        if (choices.is_array() && !choices.get<crude_json::array>().empty()) {
            const auto& first = choices.get<crude_json::array>()[0];
            if (first.is_object() && first.contains("finish_reason")) {
                const auto& fr = first["finish_reason"];
                if (!fr.is_null() && fr.is_string() && !fr.get<std::string>().empty())
                    isDone = true;
            }
        }
    }

    return data;  // 返回原始 JSON 字符串，由 handler 解析
}

// ============================================================================
// HttpClient_Default
// ============================================================================

class HttpClient_Default : public IHttpClient
{
public:
    // ── 非流式 ────────────────────────────────────────────────────────────
    void SendAsync(const HttpRequest& req, HttpCallback cb) override
    {
        std::thread([req, cb]() mutable {
            HttpResponse resp = doSend(req);
            MainThreadDispatcher::Get().Post(
                [cb = std::move(cb), resp = std::move(resp)]() mutable {
                    cb(std::move(resp));
                });
        }).detach();
    }

    // ── SSE 流式 ──────────────────────────────────────────────────────────
    void StreamAsync(const HttpRequest& baseReq,
                     HttpChunkCallback onChunk,
                     HttpDoneCallback  onDone) override
    {
        // 修改 body 加 "stream": true
        HttpRequest req = baseReq;
        {
            crude_json::value body = crude_json::value::parse(req.body);
            if (body.is_object())
                body.get<crude_json::object>()["stream"] = crude_json::value(true);
            req.body = body.dump();
        }

        std::thread([req,
                     onChunk = std::move(onChunk),
                     onDone  = std::move(onDone)]() mutable
        {
            auto parsed = parseUrl(req.url);

            httplib::Headers headers;
            for (const auto& kv : req.headers)
                headers.emplace(kv.first, kv.second);
            if (req.headers.find("Content-Type") == req.headers.end() &&
                req.headers.find("content-type") == req.headers.end())
                headers.emplace("Content-Type", "application/json");

            // SSE 缓冲区（跨 ContentReceiver 调用累积不完整行）
            std::string buf;
            std::string errorMsg;
            bool streamDone = false;

            auto contentReceiver = [&](const char* data, size_t len) -> bool {
                buf.append(data, len);
                // 按行处理
                size_t pos = 0;
                while (true) {
                    size_t nl = buf.find('\n', pos);
                    if (nl == std::string::npos) break;
                    std::string line = buf.substr(pos, nl - pos);
                    // 去掉末尾 \r
                    if (!line.empty() && line.back() == '\r')
                        line.pop_back();
                    pos = nl + 1;

                    bool isDone = false;
                    std::string sseData = parseSseLine(line, isDone);
                    if (!sseData.empty()) {
                        // 将原始 SSE data JSON（或"[DONE]"）dispatch 到主线程
                        MainThreadDispatcher::Get().Post(
                            [onChunk, tok = std::move(sseData)]() {
                                onChunk(tok);
                            });
                    }
                    if (isDone) { streamDone = true; }
                }
                buf.erase(0, pos);
                return true; // 继续接收
            };

            auto doStream = [&](auto& cli) {
                cli.set_connection_timeout(req.timeoutSeconds);
                cli.set_read_timeout(req.timeoutSeconds);
                cli.set_write_timeout(req.timeoutSeconds);
#ifdef CPPHTTPLIB_OPENSSL_SUPPORT
                cli.enable_server_certificate_verification(false);
#endif
                auto res = cli.Post(parsed.path.c_str(), headers,
                                    req.body, "application/json",
                                    contentReceiver);
                if (!res) {
                    errorMsg = httplib::to_string(res.error());
                } else if (res->status < 200 || res->status >= 300) {
                    errorMsg = "HTTP " + std::to_string(res->status);
                    if (!res->body.empty()) errorMsg += ": " + res->body.substr(0, 200);
                }
            };

#ifdef CPPHTTPLIB_OPENSSL_SUPPORT
            if (parsed.scheme == "https") {
                httplib::SSLClient cli(parsed.host, parsed.port);
                doStream(cli);
            } else
#endif
            {
                httplib::Client cli(parsed.host, parsed.port);
                doStream(cli);
            }

            // 流结束 → 主线程通知 onDone
            MainThreadDispatcher::Get().Post(
                [onDone, errorMsg]() { onDone(errorMsg); });

        }).detach();
    }

private:
    static HttpResponse doSend(const HttpRequest& req)
    {
        HttpResponse resp;
        auto parsed = parseUrl(req.url);

        httplib::Headers headers;
        for (const auto& kv : req.headers)
            headers.emplace(kv.first, kv.second);
        if (req.headers.find("Content-Type") == req.headers.end() &&
            req.headers.find("content-type") == req.headers.end())
            headers.emplace("Content-Type", "application/json");

        auto doRequest = [&](auto& cli) {
            cli.set_connection_timeout(req.timeoutSeconds);
            cli.set_read_timeout(req.timeoutSeconds);
            cli.set_write_timeout(req.timeoutSeconds);
#ifdef CPPHTTPLIB_OPENSSL_SUPPORT
            cli.enable_server_certificate_verification(false);
#endif
            httplib::Result res;
            std::string method = req.method;
            for (auto& c : method) c = static_cast<char>(toupper((unsigned char)c));
            if (method == "GET" || method == "DELETE") {
                res = (method == "GET")
                    ? cli.Get(parsed.path.c_str(), headers)
                    : cli.Delete(parsed.path.c_str(), headers);
            } else {
                res = cli.Post(parsed.path.c_str(), headers, req.body, "application/json");
                if (method == "PUT")
                    res = cli.Put(parsed.path.c_str(), headers, req.body, "application/json");
            }
            if (res) {
                resp.statusCode = res->status;
                resp.body       = res->body;
            } else {
                resp.error = httplib::to_string(res.error());
            }
        };

#ifdef CPPHTTPLIB_OPENSSL_SUPPORT
        if (parsed.scheme == "https") {
            httplib::SSLClient cli(parsed.host, parsed.port);
            doRequest(cli);
            return resp;
        }
#endif
        httplib::Client cli(parsed.host, parsed.port);
        doRequest(cli);
        return resp;
    }
};

// ============================================================================
// 工厂函数
// ============================================================================

std::shared_ptr<IHttpClient> CreateDefaultHttpClient()
{
    return std::make_shared<HttpClient_Default>();
}

} // namespace Runtime
} // namespace NodeEditor

#endif // !__EMSCRIPTEN__
