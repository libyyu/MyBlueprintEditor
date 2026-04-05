// Runtime/Http/HttpClient_Emscripten.cpp
// emscripten_fetch 实现（WebGL）
//
// SendAsync 的 callback 就是 RunAsync dispatcher 传入的 resolve()。
// resolve() 内部已经把 onComplete Post 到 MainThreadDispatcher，
// 由 Tick() → DrainQueue() 消费，所以这里直接调 cb 即可。
//
// Emscripten 链接参数需加：-sFETCH

#ifdef __EMSCRIPTEN__

#include "IHttpClient.h"
#include <emscripten/fetch.h>
#include <cstring>
#include <vector>
#include <memory>

namespace NodeEditor {
namespace Runtime {

struct FetchContext {
    HttpRequest  req;
    HttpCallback cb;
};

static void onFetchSuccess(emscripten_fetch_t* fetch)
{
    auto* ctx = static_cast<FetchContext*>(fetch->userData);
    HttpResponse resp;
    resp.statusCode = fetch->status;
    resp.body.assign(fetch->data, static_cast<size_t>(fetch->numBytes));
    emscripten_fetch_close(fetch);
    HttpCallback cb = std::move(ctx->cb);
    delete ctx;
    cb(std::move(resp));   // 即 resolve()，内部 Post 到 MainThreadDispatcher
}

static void onFetchError(emscripten_fetch_t* fetch)
{
    auto* ctx = static_cast<FetchContext*>(fetch->userData);
    HttpResponse resp;
    resp.statusCode = fetch->status;
    resp.error = "fetch error (status " + std::to_string(fetch->status) + ")";
    emscripten_fetch_close(fetch);
    HttpCallback cb = std::move(ctx->cb);
    delete ctx;
    cb(std::move(resp));   // 即 resolve()
}

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

        emscripten_fetch(&attr, req.url.c_str());
    }
};

std::shared_ptr<IHttpClient> CreateDefaultHttpClient()
{
    return std::make_shared<HttpClient_Emscripten>();
}

} // namespace Runtime
} // namespace NodeEditor

#endif // __EMSCRIPTEN__
