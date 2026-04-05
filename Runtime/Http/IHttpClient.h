// Runtime/Http/IHttpClient.h
// 跨平台 HTTP 客户端抽象接口
//
// 用法：
//   启动时注入实现：BP_SetHttpClient(CreateDefaultHttpClient());
//   节点内使用：    BP_GetHttpClient()->SendAsync(req, cb);
//
// 平台实现：
//   HttpClient_Default    — cpp-httplib（Windows/macOS/Linux/Android/iOS）
//   HttpClient_Emscripten — emscripten_fetch（WebGL）
//   外部注入              — 通过 BP_SetHttpClient 注入自定义实现（Unity C# 侧）
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

struct BLUEPRINT_API HttpRequest {
    std::string url;
    std::string method      = "POST";   // "GET" | "POST" | "PUT" | "DELETE" ...
    std::string body;
    std::map<std::string, std::string> headers;
    int timeoutSeconds      = 30;
};

struct BLUEPRINT_API HttpResponse {
    int         statusCode  = 0;
    std::string body;
    std::string error;      // 非空 = 网络/连接错误
    bool        ok() const { return error.empty() && statusCode >= 200 && statusCode < 300; }
};

using HttpCallback = std::function<void(HttpResponse)>;

// ============================================================================
// IHttpClient — 纯虚接口
// ============================================================================

class BLUEPRINT_API IHttpClient {
public:
    virtual ~IHttpClient() = default;

    // 异步发送请求，回调在"安全上下文"触发（由 Tick → DrainQueue 消费）：
    //   - 桌面：后台线程发送，回调 Post 到 MainThreadDispatcher
    //   - WebGL：emscripten_fetch 回调 Post 到 MainThreadDispatcher
    virtual void SendAsync(const HttpRequest& req, HttpCallback cb) = 0;
};

// ============================================================================
// 全局注册 / 获取（线程安全：仅在初始化阶段写，运行时只读）
// ============================================================================

BLUEPRINT_API void BP_SetHttpClient(std::shared_ptr<IHttpClient> client);
BLUEPRINT_API IHttpClient* BP_GetHttpClient();   // 可能返回 nullptr（未注册时）

// 工厂函数：按当前平台创建默认实现
// - 非 Emscripten：HttpClient_Default（cpp-httplib）
// - Emscripten：HttpClient_Emscripten（emscripten_fetch）
BLUEPRINT_API std::shared_ptr<IHttpClient> CreateDefaultHttpClient();

} // namespace Runtime
} // namespace NodeEditor
