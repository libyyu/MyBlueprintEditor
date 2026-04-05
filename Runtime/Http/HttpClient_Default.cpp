// Runtime/Http/HttpClient_Default.cpp
// cpp-httplib 实现（Windows / macOS / Linux / Android / iOS）
//
// 策略：
//   - 在后台线程执行同步 HTTP，结果通过 MainThreadDispatcher 回到主线程触发回调
//   - HTTPS 需要 OpenSSL；如果编译时未定义 CPPHTTPLIB_OPENSSL_SUPPORT，则 HTTPS 不可用
//
// Unity/Android 接入说明：
//   在 Native 初始化时调用：
//   BP_SetHttpClient(std::make_shared<HttpClient_Default>());

#ifndef __EMSCRIPTEN__

#include "IHttpClient.h"
#include "../BlueprintRunner.h"
#include "../MainThreadDispatcher.h"

// 禁用 httplib 的 zlib / brotli 依赖（不需要压缩支持）
#define CPPHTTPLIB_NO_EXCEPTIONS
#include "httplib.h"

#include <thread>
#include <sstream>

namespace NodeEditor {
namespace Runtime {

// ============================================================================
// URL 解析辅助
// ============================================================================

struct ParsedUrl {
    std::string scheme;   // "http" | "https"
    std::string host;
    int         port = 0;
    std::string path;     // 含 query string
};

static ParsedUrl parseUrl(const std::string& url)
{
    ParsedUrl result;
    // scheme
    auto schemeEnd = url.find("://");
    if (schemeEnd == std::string::npos) {
        result.scheme = "http";
        result.host   = url;
    } else {
        result.scheme = url.substr(0, schemeEnd);
        result.host   = url.substr(schemeEnd + 3);
    }
    // path
    auto pathPos = result.host.find('/');
    if (pathPos != std::string::npos) {
        result.path = result.host.substr(pathPos);
        result.host = result.host.substr(0, pathPos);
    } else {
        result.path = "/";
    }
    // port
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
// HttpClient_Default
// ============================================================================

class HttpClient_Default : public IHttpClient
{
public:
    void SendAsync(const HttpRequest& req, HttpCallback cb) override
    {
        // 后台线程执行同步 HTTP
        std::thread([req, cb]() mutable
        {
            HttpResponse resp = doSend(req);

            // 回调 dispatch 到主线程（与 RunAsync 机制保持一致）
            MainThreadDispatcher::Get().Post([cb = std::move(cb), resp = std::move(resp)]() mutable {
                cb(std::move(resp));
            });
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

        // Content-Type 默认 application/json（如果没有显式指定）
        if (req.headers.find("Content-Type") == req.headers.end() &&
            req.headers.find("content-type") == req.headers.end())
        {
            headers.emplace("Content-Type", "application/json");
        }

        auto doRequest = [&](auto& cli) {
            cli.set_connection_timeout(req.timeoutSeconds);
            cli.set_read_timeout(req.timeoutSeconds);
            cli.set_write_timeout(req.timeoutSeconds);
#ifdef CPPHTTPLIB_OPENSSL_SUPPORT
            cli.enable_server_certificate_verification(false); // 简化部署
#endif
            httplib::Result res;
            std::string method = req.method;
            for (auto& c : method) c = static_cast<char>(toupper((unsigned char)c));

            if (method == "GET" || method == "DELETE") {
                res = (method == "GET")
                    ? cli.Get(parsed.path.c_str(), headers)
                    : cli.Delete(parsed.path.c_str(), headers);
            } else {
                // POST / PUT / PATCH
                res = cli.Post(parsed.path.c_str(), headers,
                               req.body, "application/json");
                if (method == "PUT")
                    res = cli.Put(parsed.path.c_str(), headers,
                                  req.body, "application/json");
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
// 工厂函数（供外部调用）
// ============================================================================

std::shared_ptr<IHttpClient> CreateDefaultHttpClient()
{
    return std::make_shared<HttpClient_Default>();
}

} // namespace Runtime
} // namespace NodeEditor

#endif // !__EMSCRIPTEN__
