// Runtime/handlers/BuiltinHandlers_Socket.cpp
// TCP / UDP 节点实现
//
// 设计原则（与 WS/HTTP.Listen 保持一致）：
//   - socket 操作在后台线程执行
//   - 收发数据通过线程安全队列传给主线程
//   - 主线程通过 SetTimer 每帧轮询队列，激活下游 exec
//   - WebGL/Emscripten 不支持原生 socket，全部提供降级 stub
//
// TCP 节点：
//   TCP.Listen     — 启动 TCP Server，每个新连接/消息触发 onAccept/onData
//   TCP.Connect    — 作为客户端连接到远端 TCP Server（异步）
//   TCP.Send       — 向指定连接发送数据
//   TCP.Disconnect — 断开指定连接
//   TCP.Stop       — 停止 TCP Server
//
// UDP 节点：
//   UDP.Bind       — 绑定 UDP 端口，收到数据触发 onData
//   UDP.Send       — 向指定地址/端口发送 UDP 数据报
//   UDP.Close      — 关闭 UDP 套接字

#include "BuiltinHandlers_Socket.h"
#include "../BlueprintRunner.h"

#ifndef __EMSCRIPTEN__

#include <thread>
#include <mutex>
#include <atomic>
#include <queue>
#include <memory>
#include <unordered_map>
#include <string>
#include <functional>
#include <sstream>
#include <cstring>

// ── 跨平台 socket ────────────────────────────────────────────────────────────
#if defined(_WIN32) || defined(_WIN64)
#  ifndef WIN32_LEAN_AND_MEAN
#    define WIN32_LEAN_AND_MEAN
#  endif
#  ifndef _WINSOCK_DEPRECATED_NO_WARNINGS
#   define _WINSOCK_DEPRECATED_NO_WARNINGS
#  endif 
#  ifndef NOMINMAX
#    define NOMINMAX
#  endif
#  include <winsock2.h>
#  include <ws2tcpip.h>
#  pragma comment(lib, "ws2_32.lib")
   using SockFd = SOCKET;
#  define INVALID_SOCK INVALID_SOCKET
#  define SOCK_CLOSE(s) closesocket(s)
#  define SOCK_ERRNO WSAGetLastError()
   static void initWSA() {
       static std::once_flag flag;
       std::call_once(flag, []{
           WSADATA wd;
           WSAStartup(MAKEWORD(2,2), &wd);
       });
   }
#else
#  include <sys/socket.h>
#  include <netinet/in.h>
#  include <arpa/inet.h>
#  include <netdb.h>
#  include <unistd.h>
#  include <fcntl.h>
#  include <errno.h>
   using SockFd = int;
#  define INVALID_SOCK (-1)
#  define SOCK_CLOSE(s) ::close(s)
#  define SOCK_ERRNO errno
   static void initWSA() {}
#endif

#include <mutex>
namespace NodeEditor {
namespace Runtime {

// ============================================================================
// 内部工具
// ============================================================================
static std::string makeConnId(const std::string& prefix = "conn")
{
    static std::atomic<uint64_t> cnt{1};
    return prefix + "_" + std::to_string(cnt.fetch_add(1));
}

// ── TCP Server 状态 ──────────────────────────────────────────────────────────
struct TcpClientConn {
    SockFd     fd     = INVALID_SOCK;
    std::string connId;
    std::string peerAddr;
    int         peerPort = 0;
    bool        closed   = false;
};

struct TcpMessage {
    enum class Type { Accept, Data, Disconnect } type;
    std::string connId;
    std::string peerAddr;
    int         peerPort = 0;
    std::string data;
};

struct TcpServerState {
    SockFd                       listenFd = INVALID_SOCK;
    std::thread                  acceptThread;
    std::atomic<bool>            stopped{false};

    std::mutex                   connMutex;
    std::unordered_map<std::string, std::shared_ptr<TcpClientConn>> conns;

    std::mutex                   msgMutex;
    std::queue<TcpMessage>       msgQueue;
};

static std::mutex                                            s_tcpServersMutex;
static std::unordered_map<std::string, TcpServerState*>      s_tcpServers;

// ── TCP Client 状态 ──────────────────────────────────────────────────────────
struct TcpClientState {
    SockFd      fd = INVALID_SOCK;
    std::string connId;
    std::string remoteAddr;
    int         remotePort = 0;
    std::thread recvThread;
    std::atomic<bool> closed{false};

    std::mutex          msgMutex;
    std::queue<TcpMessage> msgQueue;
};

static std::mutex                                              s_tcpClientsMutex;
static std::unordered_map<std::string, TcpClientState*>        s_tcpClients;

// ── UDP 状态 ─────────────────────────────────────────────────────────────────
struct UdpMessage {
    std::string fromAddr;
    int         fromPort = 0;
    std::string data;
};

struct UdpState {
    SockFd      fd = INVALID_SOCK;
    std::string bindKey;
    std::thread recvThread;
    std::atomic<bool> closed{false};

    std::mutex           msgMutex;
    std::queue<UdpMessage> msgQueue;
};

static std::mutex                                        s_udpMutex;
static std::unordered_map<std::string, UdpState*>        s_udpSockets;

// ── 解析地址 ─────────────────────────────────────────────────────────────────
static SockFd connectTcp(const std::string& host, int port, int timeoutMs = 5000)
{
    initWSA();
    addrinfo hints{};
    hints.ai_family   = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    addrinfo* res = nullptr;
    std::string portStr = std::to_string(port);
    if (getaddrinfo(host.c_str(), portStr.c_str(), &hints, &res) != 0)
        return INVALID_SOCK;

    SockFd fd = INVALID_SOCK;
    for (addrinfo* p = res; p; p = p->ai_next) {
        fd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (fd == INVALID_SOCK) continue;
        if (connect(fd, p->ai_addr, (int)p->ai_addrlen) == 0) break;
        SOCK_CLOSE(fd);
        fd = INVALID_SOCK;
    }
    freeaddrinfo(res);
    return fd;
}

// ============================================================================
// RegisterHandlers_Socket
// ============================================================================
void RegisterHandlers_Socket(
    std::unordered_map<std::string, NodeHandler>& handlers)
{
    // ========================================================================
    // TCP.Listen
    // 启动 TCP Server，接受新连接并接收数据，通过 SetTimer 轮询推送事件
    // in:  exec, Host(String,"0.0.0.0"), Port(Integer), BufferSize(Integer,4096)
    // out: exec
    //      onAccept(exec)    — 新连接时触发
    //      onData(exec)      — 收到数据时触发
    //      onDisconnect(exec)— 连接断开时触发
    //      ConnId(String), PeerAddr(String), PeerPort(Int), Data(String)
    // ========================================================================
    handlers["TCP.Listen"] = [](ExecutionContext& ctx) -> bool {
        auto* runner = ctx.GetRunner(); (void)runner;
        std::string host  = ctx.GetInputValue("Host").asString();
        int port          = static_cast<int>(ctx.GetInputValue("Port").asInt());
        int bufSize       = static_cast<int>(ctx.GetInputValue("BufferSize").asInt());
        if (host.empty()) host = "0.0.0.0";
        if (bufSize <= 0) bufSize = 4096;
                
        initWSA();
        SockFd listenFd = socket(AF_INET, SOCK_STREAM, 0);
        if (listenFd == INVALID_SOCK) {
            ctx.LogError("[TCP.Listen] socket() failed");
            ctx.SetOutputValue("Data", Variant("[TCP.Listen] socket() failed"));
            ctx.SetOutputValue("Success", Variant(false));
            ctx.ActivateOutputFlow("");
            return true;
        }
        int opt = 1;
        setsockopt(listenFd, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(opt));

        sockaddr_in addr{};
        addr.sin_family      = AF_INET;
        addr.sin_port        = htons((uint16_t)port);
        addr.sin_addr.s_addr = inet_addr(host.c_str());
        if (bind(listenFd, (sockaddr*)&addr, sizeof(addr)) != 0 ||
            listen(listenFd, SOMAXCONN) != 0)
        {
            SOCK_CLOSE(listenFd);
            ctx.LogError("[TCP.Listen] bind/listen failed on port " + std::to_string(port));
			ctx.SetOutputValue("Data", Variant("[TCP.Listen] bind/listen failed on port " + std::to_string(port)));
			ctx.SetOutputValue("Success", Variant(false));
			ctx.ActivateOutputFlow("");
            return true;
        }

        std::string serverKey = makeConnId("tcp_srv");
        ctx.SetVariable("__tcp_server_key", Variant(serverKey));

        auto* state = new TcpServerState();
        state->listenFd = listenFd;
        {
            std::lock_guard<std::mutex> lk(s_tcpServersMutex);
            s_tcpServers[serverKey] = state;
        }

        // Accept 线程
        state->acceptThread = std::thread([state, bufSize]() {
            while (!state->stopped) {
                sockaddr_in cli{};
                socklen_t clilen = sizeof(cli);
                SockFd clientFd = accept(state->listenFd, (sockaddr*)&cli, &clilen);
                if (clientFd == INVALID_SOCK) break;

                char ipStr[INET_ADDRSTRLEN] = {};
                inet_ntop(AF_INET, &cli.sin_addr, ipStr, sizeof(ipStr));
                int peerPort = ntohs(cli.sin_port);

                auto conn = std::make_shared<TcpClientConn>();
                conn->fd       = clientFd;
                conn->connId   = makeConnId("tcp_conn");
                conn->peerAddr = ipStr;
                conn->peerPort = peerPort;

                {
                    std::lock_guard<std::mutex> lk(state->connMutex);
                    state->conns[conn->connId] = conn;
                }
                // Push Accept 事件
                {
                    std::lock_guard<std::mutex> lk(state->msgMutex);
                    state->msgQueue.push({TcpMessage::Type::Accept,
                        conn->connId, ipStr, peerPort, ""});
                }

                // 为每个连接启动接收线程
                std::string cid   = conn->connId;
                std::thread recvT([state, conn, bufSize, cid]() {
                    std::vector<char> buf(bufSize);
                    while (!state->stopped && !conn->closed) {
                        int n = recv(conn->fd, buf.data(), bufSize, 0);
                        if (n <= 0) {
                            conn->closed = true;
                            std::lock_guard<std::mutex> lk(state->msgMutex);
                            state->msgQueue.push({TcpMessage::Type::Disconnect,
                                cid, conn->peerAddr, conn->peerPort, ""});
                            break;
                        }
                        std::string data(buf.data(), n);
                        std::lock_guard<std::mutex> lk(state->msgMutex);
                        state->msgQueue.push({TcpMessage::Type::Data,
                            cid, conn->peerAddr, conn->peerPort, data});
                    }
                });
                recvT.detach();
            }
        });
        state->acceptThread.detach();

        ctx.Log("[TCP.Listen] listening on " + host + ":" + std::to_string(port));

        // 预取 PinId
        PinId acceptPin = ctx.GetPinId("onAccept");
        PinId dataPin   = ctx.GetPinId("onData");
        PinId discPin   = ctx.GetPinId("onDisconnect");
        ctx.MarkDownstreamAsHandled("onAccept");
        ctx.MarkDownstreamAsHandled("onData");
        ctx.MarkDownstreamAsHandled("onDisconnect");

        ExecutionContext* pCtx = &ctx;
        auto alive = runner->GetAliveFlag();

        ctx.SetTimer(0.016f, -1, [pCtx, serverKey, acceptPin, dataPin, discPin, alive]() -> bool {
            if (!alive->load(std::memory_order_acquire)) return false;
            TcpServerState* st = nullptr;
            {
                std::lock_guard<std::mutex> lk(s_tcpServersMutex);
                auto it = s_tcpServers.find(serverKey);
                if (it == s_tcpServers.end()) return false;
                st = it->second;
            }
            if (st->stopped) return false;

            std::queue<TcpMessage> batch;
            {
                std::lock_guard<std::mutex> lk(st->msgMutex);
                std::swap(batch, st->msgQueue);
            }
            while (!batch.empty()) {
                const auto& msg = batch.front();
                pCtx->SetOutputValue("ConnId",   Variant(msg.connId));
                pCtx->SetOutputValue("PeerAddr", Variant(msg.peerAddr));
                pCtx->SetOutputValue("PeerPort", Variant(static_cast<int64_t>(msg.peerPort)));
                pCtx->SetOutputValue("Data",     Variant(msg.data));
                switch (msg.type) {
                case TcpMessage::Type::Accept:     pCtx->ActivateOutputFlow(acceptPin); break;
                case TcpMessage::Type::Data:       pCtx->ActivateOutputFlow(dataPin);   break;
                case TcpMessage::Type::Disconnect: pCtx->ActivateOutputFlow(discPin);   break;
                }
                batch.pop();
            }
            return true;
        });

		ctx.SetOutputValue("Success", Variant(true));
		ctx.ActivateOutputFlow("");

        return true;
    };

    // ========================================================================
    // TCP.Connect
    // 作为客户端连接到远端 TCP Server（后台线程异步建立，成功后推送 onConnected）
    // in:  exec, Host(String), Port(Integer), BufferSize(Integer,4096)
    // out: onConnected(exec), onData(exec), onDisconnect(exec), onError(exec)
    //      ConnId(String), Data(String), ErrorMessage(String)
    // ========================================================================
    handlers["TCP.Connect"] = [](ExecutionContext& ctx) -> bool {
        auto* runner = ctx.GetRunner(); (void)runner;
        std::string host = ctx.GetInputValue("Host").asString();
        int port         = static_cast<int>(ctx.GetInputValue("Port").asInt());
        int bufSize      = static_cast<int>(ctx.GetInputValue("BufferSize").asInt());
        if (bufSize <= 0) bufSize = 4096;

        PinId connectedPin = ctx.GetPinId("onConnected");
        PinId dataPin      = ctx.GetPinId("onData");
        PinId discPin      = ctx.GetPinId("onDisconnect");
        PinId errorPin     = ctx.GetPinId("onError");
        ctx.MarkDownstreamAsHandled("onConnected");
        ctx.MarkDownstreamAsHandled("onData");
        ctx.MarkDownstreamAsHandled("onDisconnect");
        ctx.MarkDownstreamAsHandled("onError");

        std::string connId = makeConnId("tcp_cli");

        auto* state = new TcpClientState();
        state->connId      = connId;
        state->remoteAddr  = host;
        state->remotePort  = port;

        {
            std::lock_guard<std::mutex> lk(s_tcpClientsMutex);
            s_tcpClients[connId] = state;
        }

        ExecutionContext* pCtx = &ctx;
        auto alive = runner->GetAliveFlag();

        // 后台连接线程
        state->recvThread = std::thread([state, host, port, bufSize]() {
            SockFd fd = connectTcp(host, port);
            if (fd == INVALID_SOCK) {
                state->closed = true;
                std::lock_guard<std::mutex> lk(state->msgMutex);
                state->msgQueue.push({TcpMessage::Type::Disconnect,
                    state->connId, "", 0, "connect failed"});
                return;
            }
            state->fd = fd;
            // 触发 onConnected（用 Type::Accept 复用）
            {
                std::lock_guard<std::mutex> lk(state->msgMutex);
                state->msgQueue.push({TcpMessage::Type::Accept,
                    state->connId, host, port, ""});
            }
            // 接收循环
            std::vector<char> buf(bufSize);
            while (!state->closed) {
                int n = recv(fd, buf.data(), bufSize, 0);
                if (n <= 0) {
                    state->closed = true;
                    std::lock_guard<std::mutex> lk(state->msgMutex);
                    state->msgQueue.push({TcpMessage::Type::Disconnect,
                        state->connId, host, port, ""});
                    break;
                }
                std::lock_guard<std::mutex> lk(state->msgMutex);
                state->msgQueue.push({TcpMessage::Type::Data,
                    state->connId, host, port, std::string(buf.data(), n)});
            }
            SOCK_CLOSE(fd);
        });
        state->recvThread.detach();

        // 主线程轮询
        ctx.SetTimer(0.016f, -1, [pCtx, connId, connectedPin, dataPin, discPin, errorPin, alive]() -> bool {
            if (!alive->load(std::memory_order_acquire)) return false;
            TcpClientState* st = nullptr;
            {
                std::lock_guard<std::mutex> lk(s_tcpClientsMutex);
                auto it = s_tcpClients.find(connId);
                if (it == s_tcpClients.end()) return false;
                st = it->second;
            }
            std::queue<TcpMessage> batch;
            {
                std::lock_guard<std::mutex> lk(st->msgMutex);
                std::swap(batch, st->msgQueue);
            }
            while (!batch.empty()) {
                const auto& msg = batch.front();
                pCtx->SetOutputValue("ConnId",       Variant(msg.connId));
                pCtx->SetOutputValue("Data",         Variant(msg.data));
                switch (msg.type) {
                case TcpMessage::Type::Accept:
                    pCtx->ActivateOutputFlow(connectedPin);
                    break;
                case TcpMessage::Type::Data:
                    pCtx->ActivateOutputFlow(dataPin);
                    break;
                case TcpMessage::Type::Disconnect:
                    if (!msg.data.empty()) {
                        pCtx->ActivateOutputFlow(errorPin);
                    } else {
                        pCtx->ActivateOutputFlow(discPin);
                    }
                    return false; // 停止轮询
                }
                batch.pop();
            }
            return !st->closed;
        });

		ctx.ActivateOutputFlow("");
        return true;
    };

    // ========================================================================
    // TCP.Send
    // 向指定 ConnId 发送数据（TCP Server 和 Client 均可使用）
    // in:  exec, ConnId(String), Data(String)
    // out: exec, Success(Bool), ErrorMessage(String)
    // ========================================================================
    handlers["TCP.Send"] = [](ExecutionContext& ctx) -> bool {
        auto* runner = ctx.GetRunner(); (void)runner;
        std::string connId = ctx.GetInputValue("ConnId").asString();
        std::string data   = ctx.GetInputValue("Data").asString();

        bool sent = false;
        std::string errMsg;

        // 先查 Server 连接
        {
            std::lock_guard<std::mutex> lk(s_tcpServersMutex);
            for (auto& kv : s_tcpServers) {
                std::lock_guard<std::mutex> clk(kv.second->connMutex);
                auto it = kv.second->conns.find(connId);
                if (it != kv.second->conns.end() && !it->second->closed) {
                    int n = send(it->second->fd, data.c_str(), (int)data.size(), 0);
                    sent = (n == (int)data.size());
                    if (!sent) errMsg = "send() partial or failed";
                    break;
                }
            }
        }
        // 再查 Client 连接
        if (!sent && errMsg.empty()) {
            std::lock_guard<std::mutex> lk(s_tcpClientsMutex);
            auto it = s_tcpClients.find(connId);
            if (it != s_tcpClients.end() && !it->second->closed &&
                it->second->fd != INVALID_SOCK) {
                int n = send(it->second->fd, data.c_str(), (int)data.size(), 0);
                sent = (n == (int)data.size());
                if (!sent) errMsg = "send() partial or failed";
            } else if (!sent) {
                errMsg = "ConnId not found: " + connId;
            }
        }

        ctx.SetOutputValue("Success",      Variant(sent));
        ctx.SetOutputValue("ErrorMessage", Variant(errMsg));
        ctx.ActivateOutputFlow("");
        return true;
    };

    // ========================================================================
    // TCP.Disconnect
    // 断开指定连接
    // in:  exec, ConnId(String)
    // out: exec
    // ========================================================================
    handlers["TCP.Disconnect"] = [](ExecutionContext& ctx) -> bool {
        auto* runner = ctx.GetRunner(); (void)runner;
        std::string connId = ctx.GetInputValue("ConnId").asString();

        // Server 侧
        {
            std::lock_guard<std::mutex> lk(s_tcpServersMutex);
            for (auto& kv : s_tcpServers) {
                std::lock_guard<std::mutex> clk(kv.second->connMutex);
                auto it = kv.second->conns.find(connId);
                if (it != kv.second->conns.end()) {
                    it->second->closed = true;
                    SOCK_CLOSE(it->second->fd);
                    break;
                }
            }
        }
        // Client 侧
        {
            std::lock_guard<std::mutex> lk(s_tcpClientsMutex);
            auto it = s_tcpClients.find(connId);
            if (it != s_tcpClients.end()) {
                it->second->closed = true;
                if (it->second->fd != INVALID_SOCK)
                    SOCK_CLOSE(it->second->fd);
            }
        }

        ctx.Log("[TCP.Disconnect] connId=" + connId);
        ctx.ActivateOutputFlow("");
        return true;
    };

    // ========================================================================
    // TCP.Stop
    // 停止 TCP Server（关闭 listen socket，终止所有连接）
    // in:  exec, ServerKey(String,可空=最后一个)
    // out: exec
    // ========================================================================
    handlers["TCP.Stop"] = [](ExecutionContext& ctx) -> bool {
        auto* runner = ctx.GetRunner(); (void)runner;
        std::string serverKey = ctx.GetInputValue("ServerKey").asString();
        if (serverKey.empty())
            serverKey = ctx.GetVariable("__tcp_server_key").asString();

        std::lock_guard<std::mutex> lk(s_tcpServersMutex);
        auto it = s_tcpServers.find(serverKey);
        if (it != s_tcpServers.end()) {
            it->second->stopped = true;
            SOCK_CLOSE(it->second->listenFd);
            // 关闭所有客户端连接
            std::lock_guard<std::mutex> clk(it->second->connMutex);
            for (auto& conn : it->second->conns)
                SOCK_CLOSE(conn.second->fd);
            delete it->second;
            s_tcpServers.erase(it);
        }
        ctx.Log("[TCP.Stop] key=" + serverKey);
        ctx.ActivateOutputFlow("");
        return true;
    };

    // ========================================================================
    // UDP.Bind
    // 绑定 UDP 端口，后台线程持续接收，通过 SetTimer 轮询推送 onData
    // in:  exec, Host(String,"0.0.0.0"), Port(Integer), BufferSize(Integer,65507)
    // out: exec
    //      onData(exec) — 收到数据时触发
    //      FromAddr(String), FromPort(Int), Data(String)
    // ========================================================================
    handlers["UDP.Bind"] = [](ExecutionContext& ctx) -> bool {
        auto* runner = ctx.GetRunner(); (void)runner;
        std::string host = ctx.GetInputValue("Host").asString();
        int port         = static_cast<int>(ctx.GetInputValue("Port").asInt());
        int bufSize      = static_cast<int>(ctx.GetInputValue("BufferSize").asInt());
        if (host.empty())  host    = "0.0.0.0";
        if (bufSize <= 0)  bufSize = 65507;
 
        initWSA();
        SockFd fd = socket(AF_INET, SOCK_DGRAM, 0);
        if (fd == INVALID_SOCK) {
            ctx.LogError("[UDP.Bind] socket() failed");
			ctx.SetOutputValue("Data", Variant("[UDP.Bind] socket() failed"));
			ctx.SetOutputValue("Success", Variant(false));
			ctx.ActivateOutputFlow("");
            return true;
        }
        sockaddr_in addr{};
        addr.sin_family      = AF_INET;
        addr.sin_port        = htons((uint16_t)port);
        addr.sin_addr.s_addr = inet_addr(host.c_str());
        if (bind(fd, (sockaddr*)&addr, sizeof(addr)) != 0) {
            SOCK_CLOSE(fd);
            ctx.LogError("[UDP.Bind] bind failed on port " + std::to_string(port));
			ctx.SetOutputValue("Data", Variant("[UDP.Bind] bind failed on port " + std::to_string(port)));
			ctx.SetOutputValue("Success", Variant(false));
			ctx.ActivateOutputFlow("");
            return true;
        }

        std::string bindKey = makeConnId("udp");
        ctx.SetVariable("__udp_key_" + std::to_string(port), Variant(bindKey));

        auto* state = new UdpState();
        state->fd      = fd;
        state->bindKey = bindKey;
        {
            std::lock_guard<std::mutex> lk(s_udpMutex);
            s_udpSockets[bindKey] = state;
        }

        // 接收线程
        state->recvThread = std::thread([state, bufSize]() {
            std::vector<char> buf(bufSize);
            while (!state->closed) {
                sockaddr_in from{};
                socklen_t fromLen = sizeof(from);
                int n = recvfrom(state->fd, buf.data(), bufSize, 0,
                                 (sockaddr*)&from, &fromLen);
                if (n <= 0) break;
                char ipStr[INET_ADDRSTRLEN] = {};
                inet_ntop(AF_INET, &from.sin_addr, ipStr, sizeof(ipStr));
                int fromPort = ntohs(from.sin_port);
                std::lock_guard<std::mutex> lk(state->msgMutex);
                state->msgQueue.push({ipStr, fromPort, std::string(buf.data(), n)});
            }
        });
        state->recvThread.detach();

        ctx.Log("[UDP.Bind] listening on " + host + ":" + std::to_string(port));

        PinId dataPin = ctx.GetPinId("onData");
        ctx.MarkDownstreamAsHandled("onData");
        ExecutionContext* pCtx = &ctx;
        auto alive = runner->GetAliveFlag();

        ctx.SetTimer(0.016f, -1, [pCtx, bindKey, dataPin, alive]() -> bool {
            if (!alive->load(std::memory_order_acquire)) return false;
            UdpState* st = nullptr;
            {
                std::lock_guard<std::mutex> lk(s_udpMutex);
                auto it = s_udpSockets.find(bindKey);
                if (it == s_udpSockets.end()) return false;
                st = it->second;
            }
            if (st->closed) return false;

            std::queue<UdpMessage> batch;
            {
                std::lock_guard<std::mutex> lk(st->msgMutex);
                std::swap(batch, st->msgQueue);
            }
            while (!batch.empty()) {
                const auto& msg = batch.front();
                pCtx->SetOutputValue("FromAddr", Variant(msg.fromAddr));
                pCtx->SetOutputValue("FromPort", Variant(static_cast<int64_t>(msg.fromPort)));
                pCtx->SetOutputValue("Data",     Variant(msg.data));
                pCtx->ActivateOutputFlow(dataPin);
                batch.pop();
            }
            return true;
        });

		ctx.SetOutputValue("Success", Variant(true));
		ctx.ActivateOutputFlow("");

        return true;
    };

    // ========================================================================
    // UDP.Send
    // 向指定地址/端口发送 UDP 数据报（不需要预先连接）
    // in:  exec, Host(String), Port(Integer), Data(String)
    // out: exec, Success(Bool), ErrorMessage(String)
    // ========================================================================
    handlers["UDP.Send"] = [](ExecutionContext& ctx) -> bool {
        auto* runner = ctx.GetRunner(); (void)runner;
        std::string host = ctx.GetInputValue("Host").asString();
        int port         = static_cast<int>(ctx.GetInputValue("Port").asInt());
        std::string data = ctx.GetInputValue("Data").asString();
        
        initWSA();
        SockFd fd = socket(AF_INET, SOCK_DGRAM, 0);
        bool sent = false;
        std::string errMsg;

        if (fd != INVALID_SOCK) {
            sockaddr_in dst{};
            dst.sin_family = AF_INET;
            dst.sin_port   = htons((uint16_t)port);
            inet_pton(AF_INET, host.c_str(), &dst.sin_addr);

            int n = sendto(fd, data.c_str(), (int)data.size(), 0,
                           (sockaddr*)&dst, sizeof(dst));
            sent = (n == (int)data.size());
            if (!sent) errMsg = "sendto() failed, errno=" + std::to_string(SOCK_ERRNO);
            SOCK_CLOSE(fd);
        } else {
            errMsg = "socket() failed";
        }

        ctx.SetOutputValue("Success",      Variant(sent));
        ctx.SetOutputValue("ErrorMessage", Variant(errMsg));
        ctx.ActivateOutputFlow("");
        return true;
    };

    // ========================================================================
    // UDP.Close
    // 关闭已绑定的 UDP 套接字
    // in:  exec, Port(Integer)  — 用于查找对应的 UDP socket
    // out: exec
    // ========================================================================
    handlers["UDP.Close"] = [](ExecutionContext& ctx) -> bool {
        auto* runner = ctx.GetRunner(); (void)runner;
        int port = static_cast<int>(ctx.GetInputValue("Port").asInt());
        std::string bindKey = ctx.GetVariable("__udp_key_" + std::to_string(port)).asString();
        
        if (!bindKey.empty()) {
            std::lock_guard<std::mutex> lk(s_udpMutex);
            auto it = s_udpSockets.find(bindKey);
            if (it != s_udpSockets.end()) {
                it->second->closed = true;
                SOCK_CLOSE(it->second->fd);
                delete it->second;
                s_udpSockets.erase(it);
            }
        }
        ctx.Log("[UDP.Close] port=" + std::to_string(port));
        ctx.ActivateOutputFlow("");
        return true;
    };
}

} // namespace Runtime
} // namespace NodeEditor

#else // __EMSCRIPTEN__

namespace NodeEditor {
namespace Runtime {

void RegisterHandlers_Socket(
    std::unordered_map<std::string, NodeHandler>& handlers,
    BlueprintRunner&)
{
    auto webglStub = [](const std::string& name) {
        return [name](ExecutionContext& ctx) -> bool {
            ctx.LogError("[" + name + "] not supported on WebGL");
            ctx.SetOutputValue("Success",      Variant(false));
            ctx.SetOutputValue("Data", Variant(std::string("Not supported on WebGL")));
            ctx.ActivateOutputFlow("");
            return true;
        };
    };
    auto webglStub2 = [](const std::string& name) {
        return [name](ExecutionContext& ctx) -> bool {
            ctx.LogError("[" + name + "] not supported on WebGL");
            ctx.SetOutputValue("Success",      Variant(false));
            ctx.SetOutputValue("ErrorMessage", Variant(std::string("Not supported on WebGL")));
            ctx.ActivateOutputFlow("");
            return true;
        };
    };
    auto webglStub3 = [](const std::string& name) {
        return [name](ExecutionContext& ctx) -> bool {
            ctx.LogError("[" + name + "] not supported on WebGL");
            ctx.ActivateOutputFlow("");
            return true;
        };
    };
    handlers["TCP.Listen"]     = webglStub("TCP.Listen");
    handlers["TCP.Connect"]    = webglStub("TCP.Connect");
    handlers["TCP.Send"]       = webglStub2("TCP.Send");
    handlers["TCP.Disconnect"] = webglStub3("TCP.Disconnect");
    handlers["TCP.Stop"]       = webglStub3("TCP.Stop");
    handlers["UDP.Bind"]       = webglStub("UDP.Bind");
    handlers["UDP.Send"]       = webglStub2("UDP.Send");
    handlers["UDP.Close"]      = webglStub3("UDP.Close");
}

} // namespace Runtime
} // namespace NodeEditor

#endif // __EMSCRIPTEN__
