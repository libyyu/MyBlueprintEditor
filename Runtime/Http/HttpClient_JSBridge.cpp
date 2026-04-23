// Runtime/Http/HttpClient_JSBridge.cpp
// ─────────────────────────────────────────────────────────────────────────────
// JS 回调驱动的 HttpClient（用于微信小游戏、抖音小游戏、浏览器等 JS 宿主）
//
// 为什么需要它？
//   emscripten_fetch 在标准浏览器工作，但在微信小游戏等受限环境里：
//     - 必须走 wx.request（沙箱安全限制）
//     - 必须经过 request 白名单域名
//     - 不能直接 XHR/fetch
//   这个 HttpClient 把请求通过导出的 C API 回传给 JS 侧，由 JS 侧用宿主
//   环境的 API（wx.request / tt.request / dy.httpRequest / fetch）来发请求，
//   完成后再通过 BP_Http_OnResponse 回注结果。
//
// JS 侧接入示例（小游戏）：
//     Module.onRuntimeInitialized = () => {
//         // 1. 注册请求派发回调（由 WASM 调用）
//         Module._BP_Http_SetRequestDispatcher(Module.addFunction(dispatcher, 'vi'));
//         // 2. 注入自己作为 HTTP 后端
//         Module._BP_InstallJSBridgeHttpClient();
//     };
//     function dispatcher(reqId) {
//         const url    = UTF8ToString(Module._BP_Http_GetReqUrl(reqId));
//         const method = UTF8ToString(Module._BP_Http_GetReqMethod(reqId));
//         const body   = UTF8ToString(Module._BP_Http_GetReqBody(reqId));
//         wx.request({ url, method, data: body, success(res) {
//             const bodyStr = typeof res.data === 'string' ? res.data : JSON.stringify(res.data);
//             const buf = Module._malloc(lengthBytesUTF8(bodyStr)+1);
//             stringToUTF8(bodyStr, buf, lengthBytesUTF8(bodyStr)+1);
//             Module._BP_Http_OnResponse(reqId, res.statusCode, buf, "");
//             Module._free(buf);
//         }, fail(err) {
//             Module._BP_Http_OnResponse(reqId, 0, 0, err.errMsg || "fail");
//         }});
//     }
//
// 设计要点：
//   - 每个请求分配一个递增的 reqId（线程安全，原子递增）
//   - HttpRequest / HttpCallback 存在全局 map 里，JS 回调时通过 reqId 找回
//   - JS 通过 getter 读取请求字段（无需复杂序列化，内存零拷贝）
//   - JS 回调完成时调 BP_Http_OnResponse，内部 invoke callback
//     cb 本身已经是 Post 到 MainThreadDispatcher 的 resolve()，线程安全
// ─────────────────────────────────────────────────────────────────────────────

#include "IHttpClient.h"
#include "../BlueprintCAPI.h"

#include <atomic>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <utility>

namespace NodeEditor {
namespace Runtime {

namespace {

// 等待 JS 回调的挂起请求
struct PendingReq {
    HttpRequest      req;
    HttpCallback     cb;
};

std::mutex                                         g_mtx;
std::unordered_map<uint32_t, std::unique_ptr<PendingReq>> g_pending;
std::atomic<uint32_t>                              g_nextId{1};

// JS 侧注册的派发回调：void dispatcher(uint32_t reqId)
using JSRequestDispatcher = void (*)(uint32_t);
std::atomic<JSRequestDispatcher>                   g_dispatcher{nullptr};

class HttpClient_JSBridge : public IHttpClient
{
public:
    void SendAsync(const HttpRequest& req, HttpCallback cb) override
    {
        auto disp = g_dispatcher.load();
        if (!disp) {
            // JS 侧还没注册 dispatcher：立即失败
            HttpResponse resp;
            resp.statusCode = 0;
            resp.error      = "JSBridge: no dispatcher registered (call BP_Http_SetRequestDispatcher first)";
            cb(std::move(resp));
            return;
        }

        uint32_t id = g_nextId.fetch_add(1, std::memory_order_relaxed);

        {
            std::lock_guard<std::mutex> lk(g_mtx);
            auto pr = std::unique_ptr<PendingReq>(new PendingReq);
            pr->req = req;
            pr->cb  = std::move(cb);
            g_pending.emplace(id, std::move(pr));
        }

        // 回调到 JS：由 JS 侧发起实际 HTTP 请求
        disp(id);
    }

    // Streaming 在小游戏环境统一降级为整体 fetch（微信小游戏不支持真·流式）
    void StreamAsync(const HttpRequest& req,
                     HttpChunkCallback onChunk,
                     HttpDoneCallback  onDone) override
    {
        SendAsync(req, [onChunk = std::move(onChunk),
                        onDone  = std::move(onDone)](HttpResponse resp) mutable {
            if (!resp.ok()) {
                onDone(resp.error.empty()
                    ? ("HTTP " + std::to_string(resp.statusCode))
                    : resp.error);
                return;
            }
            onChunk(resp.body);
            onDone("");
        });
    }
};

PendingReq* TakePending(uint32_t id)
{
    std::lock_guard<std::mutex> lk(g_mtx);
    auto it = g_pending.find(id);
    if (it == g_pending.end()) return nullptr;
    PendingReq* raw = it->second.release();
    g_pending.erase(it);
    return raw;
}

PendingReq* PeekPending(uint32_t id)
{
    std::lock_guard<std::mutex> lk(g_mtx);
    auto it = g_pending.find(id);
    return it == g_pending.end() ? nullptr : it->second.get();
}

} // namespace

std::shared_ptr<IHttpClient> CreateJSBridgeHttpClient()
{
    return std::make_shared<HttpClient_JSBridge>();
}

} // namespace Runtime
} // namespace NodeEditor


// ============================================================================
// C API（导出给 JS 调用）
// ============================================================================
extern "C" {

// 安装 JSBridge 作为全局 HttpClient（覆盖已有的）
BLUEPRINT_CAPI_EXPORT void BLUEPRINT_CAPI_CALL BP_InstallJSBridgeHttpClient(void)
{
    ::NodeEditor::Runtime::BP_SetHttpClient(
        ::NodeEditor::Runtime::CreateJSBridgeHttpClient());
}

// JS 侧注册派发函数指针（通过 Module.addFunction 获得）
BLUEPRINT_CAPI_EXPORT void BLUEPRINT_CAPI_CALL BP_Http_SetRequestDispatcher(void* dispatcher)
{
    ::NodeEditor::Runtime::g_dispatcher.store(
        reinterpret_cast<::NodeEditor::Runtime::JSRequestDispatcher>(dispatcher));
}

// ── JS 侧读取挂起请求字段的 getter ──────────────────────────────────────────
BLUEPRINT_CAPI_EXPORT const char* BLUEPRINT_CAPI_CALL BP_Http_GetReqUrl(uint32_t reqId)
{
    auto* p = ::NodeEditor::Runtime::PeekPending(reqId);
    return p ? p->req.url.c_str() : "";
}

BLUEPRINT_CAPI_EXPORT const char* BLUEPRINT_CAPI_CALL BP_Http_GetReqMethod(uint32_t reqId)
{
    auto* p = ::NodeEditor::Runtime::PeekPending(reqId);
    return p ? p->req.method.c_str() : "";
}

BLUEPRINT_CAPI_EXPORT const char* BLUEPRINT_CAPI_CALL BP_Http_GetReqBody(uint32_t reqId)
{
    auto* p = ::NodeEditor::Runtime::PeekPending(reqId);
    return p ? p->req.body.c_str() : "";
}

// 返回 "key1\nvalue1\nkey2\nvalue2\n..." 由 JS 切分
BLUEPRINT_CAPI_EXPORT const char* BLUEPRINT_CAPI_CALL BP_Http_GetReqHeaders(uint32_t reqId)
{
    thread_local std::string buf;
    buf.clear();
    auto* p = ::NodeEditor::Runtime::PeekPending(reqId);
    if (!p) return "";
    for (const auto& kv : p->req.headers) {
        buf += kv.first;  buf.push_back('\n');
        buf += kv.second; buf.push_back('\n');
    }
    return buf.c_str();
}

// JS 回调：请求完成
//   statusCode: HTTP status；0 表示网络/平台错误
//   bodyPtr:    响应体 UTF-8 字符串（可为 NULL）
//   errPtr:     错误信息（可为 NULL 或 ""）
BLUEPRINT_CAPI_EXPORT void BLUEPRINT_CAPI_CALL BP_Http_OnResponse(
    uint32_t reqId, int statusCode, const char* bodyPtr, const char* errPtr)
{
    auto* raw = ::NodeEditor::Runtime::TakePending(reqId);
    if (!raw) return;  // 重复回调或未知 id，忽略
    std::unique_ptr<::NodeEditor::Runtime::PendingReq> p(raw);

    ::NodeEditor::Runtime::HttpResponse resp;
    resp.statusCode = statusCode;
    if (bodyPtr) resp.body.assign(bodyPtr);
    if (errPtr)  resp.error.assign(errPtr);

    // 调 callback（本身已走 MainThreadDispatcher.Post）
    if (p->cb) p->cb(std::move(resp));
}

} // extern "C"
