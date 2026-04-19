// Runtime/handlers/BuiltinHandlers_Server.cpp
// HTTP Server 和 WebSocket 节点实现
//
// 设计原则：
//   - httplib Server / WebSocketClient 跑在后台线程
//   - 收到消息/请求时 push 到线程安全队列
//   - Handler 通过 ctx.SetTimer 每帧轮询队列，有消息则激活下游 exec
//   - HTTP.Listen 使用 promise/future 让 HTTP 线程等待蓝图回复
//
// 节点列表：
//   HTTP.Listen   — 启动 HTTP Server，每次请求触发 onRequest
//   HTTP.Respond  — 回复一个 HTTP 请求
//   HTTP.Stop     — 停止 HTTP Server
//   WS.Connect    — 作为客户端连接 WebSocket
//   WS.Send       — 发送 WebSocket 消息
//   WS.Close      — 关闭 WebSocket 连接
//   WS.Server     — 启动 WebSocket Server（接受连接）

#include "BuiltinHandlers_Server.h"
#include "../BlueprintRunner.h"
#include "../../Utils/Json/crude_json.h"

#ifndef __EMSCRIPTEN__

#define CPPHTTPLIB_NO_EXCEPTIONS
#include "../Http/httplib.h"

#include <thread>
#include <mutex>
#include <atomic>
#include <future>
#include <memory>
#include <unordered_map>
#include <string>
#include <functional>
#include <chrono>
#include <queue>
#include <sstream>

namespace NodeEditor {
namespace Runtime {

// ============================================================================
// 工具
// ============================================================================
static std::string generateServerId(const std::string& prefix = "id")
{
    static std::atomic<uint64_t> counter{1};
    return prefix + "_" + std::to_string(counter.fetch_add(1));
}

// ============================================================================
// HTTP Server：每个请求通过 promise 等待蓝图回复
// ============================================================================

struct HttpPendingReq {
    std::string method, path, body, headersJson, requestId;
};

struct HttpServerState {
    std::shared_ptr<httplib::Server>           svr;
    std::thread                                thread;
    // 请求队列（HTTP线程写，主线程读）
    std::mutex                                 queueMutex;
    std::queue<HttpPendingReq>                 queue;
    // pending响应：requestId → promise（HTTP线程等主线程填充）
    std::mutex                                 pendingMutex;
    std::unordered_map<std::string,
        std::shared_ptr<std::promise<
            std::tuple<int,std::string,std::string>>>> pending;
    std::atomic<bool>                          stopped{false};
};

static std::mutex                                              s_httpMutex;
static std::unordered_map<std::string, HttpServerState*>       s_httpServers;

// ============================================================================
// WS Client 状态
// ============================================================================
struct WsMessage { std::string connId, text; bool isClosed = false; };

struct WsClientState {
    std::unique_ptr<httplib::ws::WebSocketClient>  cli;
    std::thread                                thread;
    std::mutex                                 sendMutex;
    std::vector<std::string>                   sendQueue;
    std::mutex                                 recvMutex;
    std::queue<WsMessage>                      recvQueue;
    std::atomic<bool>                          running{true};
    std::atomic<bool>                          connected{false};
    std::string                                connId;
    std::string                                errorMsg;
    bool                                       openFired = false;
};

static std::mutex                                              s_wsMutex;
static std::unordered_map<std::string, WsClientState*>         s_wsClients;

// ============================================================================
// WS Server 状态（复用 httplib Server，在 WebSocket handler 线程收发）
// ============================================================================
struct WsServerMsg { std::string connId, text; bool isConnect = false; bool isClose = false; };

struct WsServerState {
    std::shared_ptr<httplib::Server>  svr;
    std::thread                       thread;
    std::mutex                        queueMutex;
    std::queue<WsServerMsg>           queue;
    // connId → WebSocket* (httplib 管理生命周期，仅在 handler 线程使用)
    std::mutex                        wsMutex;
    std::unordered_map<std::string, httplib::ws::WebSocket*> wsMap;
};

static std::mutex                                              s_wsServerMutex;
static std::unordered_map<std::string, WsServerState*>         s_wsServers;

// ============================================================================
// RegisterHandlers_Server
// ============================================================================
void RegisterHandlers_Server(
    std::unordered_map<std::string, NodeHandler>& handlers,
    BlueprintRunner& /*runner*/)
{
    // ========================================================================
    // HTTP.Listen
    // ========================================================================
    handlers["HTTP.Listen"] = [](ExecutionContext& ctx) -> bool {
        int64_t     port     = ctx.GetInputValue("Port").asInt();
        std::string host     = ctx.GetInputValue("Host").asString();

        if (port <= 0)    port = 7788;
        if (host.empty()) host = "0.0.0.0";

        std::string listenKey = host + ":" + std::to_string(port);

        // 停止已有
        {
            std::lock_guard<std::mutex> lk(s_httpMutex);
            auto it = s_httpServers.find(listenKey);
            if (it != s_httpServers.end()) {
                it->second->stopped = true;
                it->second->svr->stop();
                if (it->second->thread.joinable()) it->second->thread.join();
                delete it->second;
                s_httpServers.erase(it);
            }
        }

        auto* state = new HttpServerState();
        state->svr = std::make_shared<httplib::Server>();
        {
            std::lock_guard<std::mutex> lk(s_httpMutex);
            s_httpServers[listenKey] = state;
        }

        auto svr = state->svr;

        // 通用请求处理：push 到队列，然后 promise 等待主线程响应
        auto handleReq = [state](const httplib::Request& req, httplib::Response& res) {
            crude_json::object hdrsObj;
            for (const auto& h : req.headers)
                hdrsObj[h.first] = crude_json::value(h.second);

            std::string reqId = generateServerId("req");
            HttpPendingReq pending;
            pending.method      = req.method;
            pending.path        = req.path;
            pending.body        = req.body;
            pending.headersJson = crude_json::value(hdrsObj).dump();
            pending.requestId   = reqId;

            // 创建 promise，HTTP 线程在此阻塞等待主线程调 HTTP.Respond
            auto prom = std::make_shared<std::promise<std::tuple<int,std::string,std::string>>>();
            auto fut  = prom->get_future();
            {
                std::lock_guard<std::mutex> plk(state->pendingMutex);
                state->pending[reqId] = prom;
            }
            {
                std::lock_guard<std::mutex> qlk(state->queueMutex);
                state->queue.push(pending);
            }

            // 等待（最多 30s）
            if (fut.wait_for(std::chrono::seconds(30)) == std::future_status::ready) {
                auto [code, body, ct] = fut.get();
                res.status = code;
                res.set_content(body, ct.empty() ? "application/json; charset=utf-8" : ct);
            } else {
                res.status = 504;
                res.set_content("Blueprint timeout", "text/plain");
                std::lock_guard<std::mutex> plk(state->pendingMutex);
                state->pending.erase(reqId);
            }
            res.set_header("Access-Control-Allow-Origin", "*");
        };

        svr->Get   (".*", handleReq);
        svr->Post  (".*", handleReq);
        svr->Put   (".*", handleReq);
        svr->Delete(".*", handleReq);
        svr->Options(".*", [](const httplib::Request&, httplib::Response& res) {
            res.set_header("Access-Control-Allow-Origin",  "*");
            res.set_header("Access-Control-Allow-Methods", "GET,POST,PUT,DELETE,OPTIONS");
            res.set_header("Access-Control-Allow-Headers", "Content-Type,Authorization");
            res.status = 204;
            res.set_content("", "text/plain");
        });

        state->thread = std::thread([svr, host, port]() {
            svr->listen(host.c_str(), (int)port);
        });

        ctx.Log("[HTTP.Listen] Listening on http://" + host + ":" + std::to_string(port));
        ctx.SetVariable("__http_listen_key", Variant(listenKey));

        ctx.MarkDownstreamAsHandled("onRequest");
        ctx.MarkDownstreamAsHandled("onError");

        // SetTimer 每帧轮询请求队列
        ExecutionContext* pCtx = &ctx;
        ctx.SetTimer(0.016f, -1, [pCtx, listenKey]() -> bool {
            // 检查 server 是否还活着
            HttpServerState* st = nullptr;
            {
                std::lock_guard<std::mutex> lk(s_httpMutex);
                auto it = s_httpServers.find(listenKey);
                if (it == s_httpServers.end()) return false; // 已停止
                st = it->second;
            }
            if (st->stopped) return false;

            // 取出所有待处理请求
            std::queue<HttpPendingReq> batch;
            {
                std::lock_guard<std::mutex> qlk(st->queueMutex);
                std::swap(batch, st->queue);
            }
            while (!batch.empty()) {
                auto& req = batch.front();
                pCtx->SetVariable("__http_method",     Variant(req.method));
                pCtx->SetVariable("__http_path",       Variant(req.path));
                pCtx->SetVariable("__http_body",       Variant(req.body));
                pCtx->SetVariable("__http_headers",    Variant(req.headersJson));
                pCtx->SetVariable("__http_request_id", Variant(req.requestId));
                pCtx->ActivateOutputFlow("onRequest");
                batch.pop();
            }
            return true; // 继续轮询
        });

        return true;
    };

    // ========================================================================
    // HTTP.Respond
    // ========================================================================
    handlers["HTTP.Respond"] = [](ExecutionContext& ctx) -> bool {
        std::string reqId = ctx.GetInputValue("RequestId").asString();
        if (reqId.empty()) reqId = ctx.GetVariable("__http_request_id").asString();

        int64_t     status = ctx.GetInputValue("StatusCode").asInt();
        std::string body   = ctx.GetInputValue("Body").asString();
        std::string ct     = ctx.GetInputValue("ContentType").asString();
        if (status <= 0) status = 200;
        if (ct.empty())  ct = "application/json; charset=utf-8";

        if (reqId.empty()) {
            ctx.LogError("[HTTP.Respond] RequestId is empty");
            ctx.ActivateOutputFlow("");
            return true;
        }

        // 找到对应的 server state
        std::lock_guard<std::mutex> lk(s_httpMutex);
        for (auto& [key, st] : s_httpServers) {
            std::lock_guard<std::mutex> plk(st->pendingMutex);
            auto it = st->pending.find(reqId);
            if (it != st->pending.end()) {
                it->second->set_value({ (int)status, body, ct });
                st->pending.erase(it);
                break;
            }
        }
        ctx.ActivateOutputFlow("");
        return true;
    };

    // ========================================================================
    // HTTP.Stop
    // ========================================================================
    handlers["HTTP.Stop"] = [](ExecutionContext& ctx) -> bool {
        std::string key = ctx.GetInputValue("ListenKey").asString();
        if (key.empty()) key = ctx.GetVariable("__http_listen_key").asString();

        std::lock_guard<std::mutex> lk(s_httpMutex);
        auto stop = [](HttpServerState* st) {
            st->stopped = true;
            st->svr->stop();
            if (st->thread.joinable()) st->thread.join();
            delete st;
        };
        if (key.empty()) {
            for (auto& kv : s_httpServers) stop(kv.second);
            s_httpServers.clear();
        } else {
            auto it = s_httpServers.find(key);
            if (it != s_httpServers.end()) {
                stop(it->second);
                s_httpServers.erase(it);
            }
        }
        ctx.ActivateOutputFlow("");
        return true;
    };

    // ========================================================================
    // WS.Connect
    //   后台线程循环 read()，push 到 recvQueue，SetTimer 轮询激活 onMessage
    // ========================================================================
    handlers["WS.Connect"] = [](ExecutionContext& ctx) -> bool {
        std::string url     = ctx.GetInputValue("URL").asString();
        std::string hdrsStr = ctx.GetInputValue("Headers").asString();

        if (url.empty()) {
            ctx.SetOutputValue("ErrorMessage", Variant(std::string("URL is empty")));
            ctx.ActivateOutputFlow("onError");
            return true;
        }

        std::string connId = generateServerId("wsc");
        ctx.SetOutputValue("ConnectionId", Variant(connId));

        httplib::Headers hdrs;
        if (!hdrsStr.empty()) {
            auto jh = crude_json::value::parse(hdrsStr);
            if (jh.is_object())
                for (const auto& kv : jh.get<crude_json::object>())
                    hdrs.emplace(kv.first,
                        kv.second.is_string() ? kv.second.get<std::string>() : kv.second.dump());
        }

        auto* state = new WsClientState();
        state->connId = connId;
        {
            std::lock_guard<std::mutex> lk(s_wsMutex);
            s_wsClients[connId] = state;
        }

        // 后台线程：连接并轮询消息
        state->thread = std::thread([state, url, hdrs]() mutable {
            state->cli = std::make_unique<httplib::ws::WebSocketClient>(url, hdrs);
            if (!state->cli->is_valid() || !state->cli->connect()) {
                state->errorMsg = "WS connect failed: " + url;
                state->running  = false;
                return;
            }
            state->connected = true;
            state->cli->set_read_timeout(0, 50000); // 50ms 超时，轮询用

            while (state->running) {
                // 发送队列
                {
                    std::lock_guard<std::mutex> slk(state->sendMutex);
                    for (const auto& msg : state->sendQueue)
                        state->cli->send(msg);
                    state->sendQueue.clear();
                }
                // 读消息
                std::string msg;
                auto result = state->cli->read(msg);
                if (result == httplib::ws::ReadResult::Text ||
                    result == httplib::ws::ReadResult::Binary) {
                    std::lock_guard<std::mutex> rlk(state->recvMutex);
                    state->recvQueue.push({ state->connId, msg, false });
                } else if (result == httplib::ws::ReadResult::Fail) {
                    // 连接断开
                    std::lock_guard<std::mutex> rlk(state->recvMutex);
                    state->recvQueue.push({ state->connId, "", true });
                    break;
                }
                // Timeout: 继续轮询
            }
            state->connected = false;
        });

        ctx.MarkDownstreamAsHandled("onOpen");
        ctx.MarkDownstreamAsHandled("onMessage");
        ctx.MarkDownstreamAsHandled("onClose");
        ctx.MarkDownstreamAsHandled("onError");

        ExecutionContext* pCtx = &ctx;
        ctx.SetTimer(0.016f, -1, [pCtx, connId]() -> bool {
            WsClientState* st = nullptr;
            {
                std::lock_guard<std::mutex> lk(s_wsMutex);
                auto it = s_wsClients.find(connId);
                if (it == s_wsClients.end()) return false;
                st = it->second;
            }

            // onOpen（只触发一次）
            if (st->connected && !st->openFired) {
                st->openFired = true;
                pCtx->SetVariable("__ws_conn_id", Variant(connId));
                pCtx->ActivateOutputFlow("onOpen");
            }

            // onError
            if (!st->running && !st->connected && !st->errorMsg.empty()) {
                pCtx->SetOutputValue("ErrorMessage", Variant(st->errorMsg));
                pCtx->ActivateOutputFlow("onError");
                // 清理
                if (st->thread.joinable()) st->thread.detach();
                std::lock_guard<std::mutex> lk(s_wsMutex);
                s_wsClients.erase(connId);
                delete st;
                return false;
            }

            // 消息队列
            std::queue<WsMessage> batch;
            {
                std::lock_guard<std::mutex> rlk(st->recvMutex);
                std::swap(batch, st->recvQueue);
            }
            while (!batch.empty()) {
                auto& m = batch.front();
                if (m.isClosed) {
                    pCtx->SetVariable("__ws_conn_id",      Variant(connId));
                    pCtx->SetVariable("__ws_close_code",   Variant((int64_t)1000));
                    pCtx->SetVariable("__ws_close_reason", Variant(std::string("")));
                    pCtx->ActivateOutputFlow("onClose");
                    // 清理
                    if (st->thread.joinable()) st->thread.detach();
                    std::lock_guard<std::mutex> lk(s_wsMutex);
                    s_wsClients.erase(connId);
                    delete st;
                    return false;
                } else {
                    pCtx->SetVariable("__ws_conn_id", Variant(connId));
                    pCtx->SetVariable("__ws_message",  Variant(m.text));
                    pCtx->ActivateOutputFlow("onMessage");
                }
                batch.pop();
            }
            return true;
        });

        ctx.Log("[WS.Connect] Connecting to " + url + " (id=" + connId + ")");
        return true;
    };

    // ========================================================================
    // WS.Send
    // ========================================================================
    handlers["WS.Send"] = [](ExecutionContext& ctx) -> bool {
        std::string connId  = ctx.GetInputValue("ConnectionId").asString();
        std::string message = ctx.GetInputValue("Message").asString();
        if (connId.empty()) connId = ctx.GetVariable("__ws_conn_id").asString();

        if (connId.empty()) {
            ctx.SetOutputValue("ErrorMessage", Variant(std::string("ConnectionId is empty")));
            ctx.ActivateOutputFlow("onError");
            return true;
        }

        std::lock_guard<std::mutex> lk(s_wsMutex);
        auto it = s_wsClients.find(connId);
        if (it == s_wsClients.end()) {
            ctx.SetOutputValue("ErrorMessage",
                Variant(std::string("Connection not found: " + connId)));
            ctx.ActivateOutputFlow("onError");
            return true;
        }
        {
            std::lock_guard<std::mutex> slk(it->second->sendMutex);
            it->second->sendQueue.push_back(message);
        }
        ctx.SetOutputValue("ErrorMessage", Variant(std::string("")));
        ctx.ActivateOutputFlow("onSuccess");
        return true;
    };

    // ========================================================================
    // WS.Close
    // ========================================================================
    handlers["WS.Close"] = [](ExecutionContext& ctx) -> bool {
        std::string connId = ctx.GetInputValue("ConnectionId").asString();
        if (connId.empty()) connId = ctx.GetVariable("__ws_conn_id").asString();

        if (!connId.empty()) {
            std::lock_guard<std::mutex> lk(s_wsMutex);
            auto it = s_wsClients.find(connId);
            if (it != s_wsClients.end()) {
                it->second->running = false;
                if (it->second->cli)
                    it->second->cli->close(httplib::ws::CloseStatus::Normal);
            }
        }
        ctx.ActivateOutputFlow("");
        return true;
    };

    // ========================================================================
    // WS.Server
    //   每个新连接在 httplib WebSocket handler 线程中运行
    //   消息 push 到队列，SetTimer 轮询激活 onConnect/onMessage/onClose
    // ========================================================================
    handlers["WS.Server"] = [](ExecutionContext& ctx) -> bool {
        int64_t     port = ctx.GetInputValue("Port").asInt();
        std::string host = ctx.GetInputValue("Host").asString();
        std::string path = ctx.GetInputValue("Path").asString();
        if (port <= 0)    port = 7790;
        if (host.empty()) host = "0.0.0.0";
        if (path.empty()) path = "/ws";

        std::string serverKey = "wssvr_" + host + ":" + std::to_string(port);

        // 停止已有
        {
            std::lock_guard<std::mutex> lk(s_wsServerMutex);
            auto it = s_wsServers.find(serverKey);
            if (it != s_wsServers.end()) {
                it->second->svr->stop();
                if (it->second->thread.joinable()) it->second->thread.join();
                delete it->second;
                s_wsServers.erase(it);
            }
        }

        auto* state = new WsServerState();
        state->svr = std::make_shared<httplib::Server>();
        {
            std::lock_guard<std::mutex> lk(s_wsServerMutex);
            s_wsServers[serverKey] = state;
        }

        auto svr = state->svr;

        // 注册 WebSocket 路由（httplib 0.41 API）
        svr->WebSocket(path,
            [state](const httplib::Request& /*req*/, httplib::ws::WebSocket& ws)
        {
            std::string connId = generateServerId("wsconn");

            // onConnect
            {
                std::lock_guard<std::mutex> qlk(state->queueMutex);
                state->queue.push({ connId, "", true, false });
            }
            // 保存 ws 指针（仅在本线程使用，发送用）
            {
                std::lock_guard<std::mutex> wlk(state->wsMutex);
                state->wsMap[connId] = &ws;
            }

            // 消息循环
            while (ws.is_open()) {
                std::string msg;
                auto result = ws.read(msg);
                if (result == httplib::ws::ReadResult::Text ||
                    result == httplib::ws::ReadResult::Binary) {
                    std::lock_guard<std::mutex> qlk(state->queueMutex);
                    state->queue.push({ connId, msg, false, false });
                } else {
                    break; // Fail = 连接断开
                }
            }

            // onClose
            {
                std::lock_guard<std::mutex> qlk(state->queueMutex);
                state->queue.push({ connId, "", false, true });
            }
            {
                std::lock_guard<std::mutex> wlk(state->wsMutex);
                state->wsMap.erase(connId);
            }
        });

        state->thread = std::thread([svr, host, port]() {
            svr->listen(host.c_str(), (int)port);
        });

        ctx.Log("[WS.Server] Listening on ws://" + host + ":" +
                std::to_string(port) + path);
        ctx.SetVariable("__ws_server_key", Variant(serverKey));

        ctx.MarkDownstreamAsHandled("onConnect");
        ctx.MarkDownstreamAsHandled("onMessage");
        ctx.MarkDownstreamAsHandled("onClose");
        ctx.MarkDownstreamAsHandled("onError");

        ExecutionContext* pCtx = &ctx;
        ctx.SetTimer(0.016f, -1, [pCtx, serverKey]() -> bool {
            WsServerState* st = nullptr;
            {
                std::lock_guard<std::mutex> lk(s_wsServerMutex);
                auto it = s_wsServers.find(serverKey);
                if (it == s_wsServers.end()) return false;
                st = it->second;
            }

            std::queue<WsServerMsg> batch;
            {
                std::lock_guard<std::mutex> qlk(st->queueMutex);
                std::swap(batch, st->queue);
            }
            while (!batch.empty()) {
                auto& m = batch.front();
                pCtx->SetVariable("__ws_conn_id", Variant(m.connId));
                if (m.isConnect) {
                    pCtx->ActivateOutputFlow("onConnect");
                } else if (m.isClose) {
                    pCtx->SetVariable("__ws_close_code", Variant((int64_t)1000));
                    pCtx->ActivateOutputFlow("onClose");
                } else {
                    pCtx->SetVariable("__ws_message", Variant(m.text));
                    pCtx->ActivateOutputFlow("onMessage");
                }
                batch.pop();
            }
            return true;
        });

        return true;
    };
}

} // namespace Runtime
} // namespace NodeEditor

#else // __EMSCRIPTEN__

namespace NodeEditor { namespace Runtime {
void RegisterHandlers_Server(
    std::unordered_map<std::string, NodeHandler>& handlers,
    BlueprintRunner& /*runner*/)
{
    auto webglErr = [](const std::string& name) {
        return [name](ExecutionContext& ctx) -> bool {
            ctx.SetOutputValue("ErrorMessage",
                Variant(name + " not supported on WebGL"));
            ctx.ActivateOutputFlow("onError");
            return true;
        };
    };
    handlers["HTTP.Listen"]  = webglErr("HTTP.Listen");
    handlers["HTTP.Respond"] = [](ExecutionContext& ctx)->bool{ ctx.ActivateOutputFlow(""); return true; };
    handlers["HTTP.Stop"]    = [](ExecutionContext& ctx)->bool{ ctx.ActivateOutputFlow(""); return true; };
    handlers["WS.Connect"]   = webglErr("WS.Connect");
    handlers["WS.Send"]      = webglErr("WS.Send");
    handlers["WS.Close"]     = [](ExecutionContext& ctx)->bool{ ctx.ActivateOutputFlow(""); return true; };
    handlers["WS.Server"]    = webglErr("WS.Server");
}
} }

#endif // __EMSCRIPTEN__
