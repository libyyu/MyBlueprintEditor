// Runtime/Http/HttpClient_Emscripten.cpp
// emscripten_fetch 实现（WebGL）
//
// 特点：
//   - 浏览器沙盒内唯一合法的网络方式
//   - emscripten_fetch 本身是异步的，回调在主线程触发 → 不需要 MainThreadDispatcher
//   - 需要在 emcc 链接参数加上 -sFETCH

#ifdef __EMSCRIPTEN__

#include "IHttpClient.h"
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
// 回调
// ============================================================================

static void onFetchSuccess(emscripten_fetch_t* fetch)
{
    auto* ctx = static_cast<FetchContext*>(fetch->userData);
    HttpResponse resp;
    resp.statusCode = fetch->status;
    resp.body.assign(fetch->data, static_cast<size_t>(fetch->numBytes));
    ctx->cb(std::move(resp));
    delete ctx;
    emscripten_fetch_close(fetch);
}

static void onFetchError(emscripten_fetch_t* fetch)
{
    auto* ctx = static_cast<FetchContext*>(fetch->userData);
    HttpResponse resp;
    resp.statusCode = fetch->status;
    resp.error = "fetch error (status " + std::to_string(fetch->status) + ")";
    ctx->cb(std::move(resp));
    delete ctx;
    emscripten_fetch_close(fetch);
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

        // method
        strncpy(attr.requestMethod, req.method.c_str(), sizeof(attr.requestMethod) - 1);
        attr.requestMethod[sizeof(attr.requestMethod) - 1] = '\0';

        attr.attributes = EMSCRIPTEN_FETCH_LOAD_TO_MEMORY;
        attr.onsuccess  = onFetchSuccess;
        attr.onerror    = onFetchError;
        attr.userData   = ctx;

        // headers：emscripten_fetch 需要 null-terminated 字符串数组 [k, v, k, v, ..., nullptr]
        std::vector<std::string> hdrStorage;
        std::vector<const char*> hdrPtrs;
        // 默认 Content-Type
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

        // body
        if (!req.body.empty()) {
            attr.requestData     = req.body.c_str();
            attr.requestDataSize = req.body.size();
        }

        emscripten_fetch(&attr, req.url.c_str());
        // hdrStorage / hdrPtrs 在 fetch 发起后可以销毁（emscripten_fetch 内部已复制）
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
