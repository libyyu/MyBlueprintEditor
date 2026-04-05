// Runtime/Http/HttpClient_Emscripten.cpp
// emscripten_fetch 实现（WebGL）
//
// 关键设计：
//   fetch 回调触发在浏览器事件循环（主线程），但不是 BlueprintRunner::Tick() 上下文。
//   直接在回调里调 ActivateOutputFlow 会绕过 Tick 调度，导致重入风险。
//   修复：回调里把 HttpCallback Post 到 MainThreadDispatcher，
//   由 Tick() → DrainQueue() 在安全上下文里统一消费。
//
// Emscripten 链接参数需加：-sFETCH

#ifdef __EMSCRIPTEN__

#include "IHttpClient.h"
#include "../MainThreadDispatcher.h"
#include <emscripten/fetch.h>
#include <cstring>
#include <vector>
#include <memory>

namespace NodeEditor {
namespace Runtime {

// ============================================================================
// 请求上下文（传给 fetch 回调）
// ============================================================================

struct FetchContext {
    HttpRequest  req;
    HttpCallback cb;
};

// ============================================================================
// 统一派发：把响应 Post 到 MainThreadDispatcher，由 Tick/DrainQueue 消费
// ============================================================================

static void dispatchResponse(FetchContext* ctx, HttpResponse resp)
{
    HttpCallback cb = std::move(ctx->cb);
    delete ctx;
    MainThreadDispatcher::Get().Post(
        [cb = std::move(cb), resp = std::move(resp)]() mutable {
            cb(std::move(resp));
        });
}

static void onFetchSuccess(emscripten_fetch_t* fetch)
{
    auto* ctx = static_cast<FetchContext*>(fetch->userData);
    HttpResponse resp;
    resp.statusCode = fetch->status;
    resp.body.assign(fetch->data, static_cast<size_t>(fetch->numBytes));
    emscripten_fetch_close(fetch);
    dispatchResponse(ctx, std::move(resp));
}

static void onFetchError(emscripten_fetch_t* fetch)
{
    auto* ctx = static_cast<FetchContext*>(fetch->userData);
    HttpResponse resp;
    resp.statusCode = fetch->status;
    resp.error = "fetch error (status " + std::to_string(fetch->status) + ")";
    emscripten_fetch_close(fetch);
    dispatchResponse(ctx, std::move(resp));
}

// ============================================================================
// HttpClient_Emscripten
// ============================================================================

class HttpClient_Emscripten : public IHttpClient
{
public:
    void SendAsync(const HttpRequest& req, HttpCallback cb) override
    {
        auto* ctx = new FetchContext{req, std::move(cb)};

        emscripten_fetch_attr_t attr;
        emscripten_fetch_attr_init(&attr);

        strncpy(attr.requestMethod, req.method.c_str(), sizeof(attr.requestMethod) - 1);
        attr.requestMethod[sizeof(attr.requestMethod) - 1] = '\0';

        attr.attributes = EMSCRIPTEN_FETCH_LOAD_TO_MEMORY;
        attr.onsuccess  = onFetchSuccess;
        attr.onerror    = onFetchError;
        attr.userData   = ctx;

        // headers：null-terminated 字符串数组 [k, v, k, v, ..., nullptr]
        std::vector<std::string> hdrStorage;
        std::vector<const char*> hdrPtrs;
        bool hasContentType = false;
        for (const auto& kv : req.headers) {
            if (kv.first == "Content-Type" || kv.first == "content-type")
                hasContentType = true;
            hdrStorage.push_back(kv.first);
            hdrStorage.push_back(kv.second);
        }
        if (!hasContentType && !req.body.empty()) {
            hdrStorage.push_back("Content-Type");
            hdrStorage.push_back("application/json");
        }
        for (const auto& s : hdrStorage)
            hdrPtrs.push_back(s.c_str());
        hdrPtrs.push_back(nullptr);
        attr.requestHeaders = hdrPtrs.data();

        if (!req.body.empty()) {
            attr.requestData     = req.body.c_str();
            attr.requestDataSize = req.body.size();
        }

        // emscripten_fetch 立即返回，请求在浏览器事件循环里异步执行
        emscripten_fetch(&attr, req.url.c_str());
        // hdrStorage/hdrPtrs 的生命周期：emscripten_fetch 内部会复制 headers，安全销毁
    }
};

// ============================================================================
// 工厂函数
// ============================================================================

std::shared_ptr<IHttpClient> CreateDefaultHttpClient()
{
    return std::make_shared<HttpClient_Emscripten>();
}

} // namespace Runtime
} // namespace NodeEditor

#endif // __EMSCRIPTEN__
