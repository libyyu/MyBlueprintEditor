// Runtime/Http/IHttpClient.h
// 跨平台 HTTP 客户端抽象接口
//
// 用法：
//   启动时注入实现：BP_SetHttpClient(CreateDefaultHttpClient());
//   节点内使用：    BP_GetHttpClient()->SendAsync(req, cb);
//                  BP_GetHttpClient()->StreamAsync(req, onChunk, onDone);
//
// 平台实现：
//   HttpClient_Default    — cpp-httplib（Windows/macOS/Linux/Android/iOS）
//   HttpClient_Emscripten — emscripten_fetch（WebGL，Streaming 降级为非流式）
//   外部注入              — 通过 BP_SetHttpClient 注入自定义实现
#pragma once

#include "../BlueprintExport.h"
#include <string>
#include <map>
#include <functional>
#include <memory>

namespace NodeEditor {
namespace Runtime {

// ============================================================================
// HttpRequest / HttpResponse
// ============================================================================
// 注意：不加 BLUEPRINT_API — 纯数据结构，避免 MSVC C4251

struct HttpRequest {
    std::string url;
    std::string method      = "POST";
    std::string body;
    std::map<std::string, std::string> headers;
    int timeoutSeconds      = 120;
};

struct HttpResponse {
    int         statusCode  = 0;
    std::string body;
    std::string error;
    bool ok() const { return error.empty() && statusCode >= 200 && statusCode < 300; }
};

using HttpCallback = std::function<void(HttpResponse)>;

// SSE Streaming 回调类型
//   onChunk(token)  — 每收到一个 delta.content 就调一次，在主线程（MainThreadDispatcher）
//   onDone(error)   — 流结束或出错；error 为空表示正常结束
using HttpChunkCallback = std::function<void(const std::string& token)>;
using HttpDoneCallback  = std::function<void(const std::string& error)>;

// ============================================================================
// IHttpClient
// ============================================================================

class BLUEPRINT_API IHttpClient {
public:
    virtual ~IHttpClient() = default;

    // 非流式异步请求（原有接口，保持不变）
    virtual void SendAsync(const HttpRequest& req, HttpCallback cb) = 0;

    // 流式 SSE 请求
    //   onChunk: 每个 delta.content token（主线程回调）
    //   onDone:  流结束或出错（主线程回调，error=""表示正常）
    // 默认实现：降级为非流式 SendAsync，onDone 时把整个 body 当作一个 chunk
    virtual void StreamAsync(const HttpRequest& req,
                             HttpChunkCallback onChunk,
                             HttpDoneCallback  onDone)
    {
        SendAsync(req, [onChunk = std::move(onChunk),
                        onDone  = std::move(onDone)](HttpResponse resp) mutable {
            if (!resp.ok()) {
                onDone(resp.error.empty()
                    ? ("HTTP " + std::to_string(resp.statusCode))
                    : resp.error);
                return;
            }
            // 尝试解析 choices[0].message.content 作为单个 chunk
            // 如果无法解析就把整个 body 当 chunk
            onChunk(resp.body);
            onDone("");
        });
    }
};

// ============================================================================
// 全局注册 / 获取
// ============================================================================

BLUEPRINT_API void BP_SetHttpClient(std::shared_ptr<IHttpClient> client);
BLUEPRINT_API IHttpClient* BP_GetHttpClient();

BLUEPRINT_API std::shared_ptr<IHttpClient> CreateDefaultHttpClient();

} // namespace Runtime
} // namespace NodeEditor
