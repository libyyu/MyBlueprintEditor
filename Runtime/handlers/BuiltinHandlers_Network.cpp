// Runtime/handlers/BuiltinHandlers_Network.cpp
// 网络相关节点处理器：HTTP.Request, HTTP.Get, HTTP.Post, HTTP.Download,
//                    JSON.GetPath, Web.Search, Code.Run
#include "BuiltinHandlers_Network.h"
#include "../BlueprintRunner.h"
#include "../Http/IHttpClient.h"
#include "../../Utils/Json/crude_json.h"

#include <thread>
#include <cstdio>
#include <cctype>

#ifdef _WIN32
#  ifndef WIN32_LEAN_AND_MEAN
#    define WIN32_LEAN_AND_MEAN
#  endif
#  include <windows.h>
#elif !defined(__EMSCRIPTEN__)
#  include <unistd.h>
#  include <sys/wait.h>
#  include <signal.h>
#  include <fcntl.h>
#endif

namespace NodeEditor {
namespace Runtime {

void RegisterHandlers_Network(
    std::unordered_map<std::string, NodeHandler>& handlers,
    BlueprintRunner& runner)
{
    // ========================================================================
    // HTTP.Request
    // 输入：  URL(String), Method(String), Body(String), Headers(String/JSON),
    //         TimeoutSeconds(Integer)
    // 输出：  → onSuccess, → onError
    //         StatusCode(Integer), ResponseBody(String), ErrorMessage(String)
    //
    // 异步机制：通过 RunAsync(dispatcher, onComplete) 实现跨平台异步。
    //   - dispatcher(resolve)：调用 IHttpClient::SendAsync 发起请求，
    //     回调里调 resolve() 通知完成。
    //   - 桌面：dispatcher 在后台线程执行；WebGL：dispatcher 在主线程执行，
    //     emscripten_fetch 在浏览器异步完成后调 resolve()。
    //   - 两个平台 onComplete 都在 Tick → DrainQueue 上下文执行，安全访问 ctx。
    // ========================================================================
    handlers["HTTP.Request"] = [&runner](ExecutionContext& ctx) -> bool {

        IHttpClient* client = BP_GetHttpClient();
        if (!client) {
            ctx.LogError("[HTTP.Request] No HttpClient registered. Call BP_SetHttpClient() first.");
            ctx.SetOutputValue("StatusCode",    Variant(static_cast<int64_t>(0)));
            ctx.SetOutputValue("ResponseBody",  Variant(std::string("")));
            ctx.SetOutputValue("ErrorMessage",  Variant(std::string("No HttpClient registered")));
            ctx.ActivateOutputFlow("onError");
            return true;
        }

        HttpRequest req;
        req.url    = ctx.GetInputValue("URL").asString();
        req.method = ctx.GetInputValue("Method").asString();
        req.body   = ctx.GetInputValue("Body").asString();
        if (req.method.empty()) req.method = "POST";

        // Headers：JSON 格式字符串 {"Key": "Value", ...}
        std::string headersJson = ctx.GetInputValue("Headers").asString();
        if (!headersJson.empty()) {
            crude_json::value hj = crude_json::value::parse(headersJson);
            if (hj.is_object()) {
                for (const auto& kv : hj.get<crude_json::object>())
                    if (kv.second.is_string())
                        req.headers[kv.first] = kv.second.get<std::string>();
            }
        }

        auto timeoutVar = ctx.GetInputValue("TimeoutSeconds");
        if (timeoutVar.type == PinDataType::Integer || timeoutVar.type == PinDataType::Float)
            req.timeoutSeconds = static_cast<int>(timeoutVar.asInt());

        PinId successPinId = ctx.GetPinId("onSuccess");
        PinId errorPinId   = ctx.GetPinId("onError");

        ctx.MarkDownstreamAsHandled("onSuccess");
        ctx.MarkDownstreamAsHandled("onError");

        // 把 response 存在 shared_ptr 里，dispatcher 和 onComplete 之间传递
        auto sharedResp = std::make_shared<HttpResponse>();

        // dispatcher：发起异步 HTTP 请求，完成后调 resolve()
        auto dispatcher = [client, req, sharedResp](ExecutionContext::AsyncResolve resolve) mutable
        {
            client->SendAsync(req,
                [sharedResp, resolve = std::move(resolve)](HttpResponse resp) mutable
                {
                    *sharedResp = std::move(resp);
                    resolve();   // 通知 RunAsync：异步操作完成，可执行 onComplete
                });
        };

        // onComplete：在 Tick 上下文安全执行，访问 ctx 激活下游
        auto onComplete = [sharedResp, successPinId, errorPinId](ExecutionContext& c) mutable
        {
            const HttpResponse& resp = *sharedResp;
            c.SetOutputValue("StatusCode",   Variant(static_cast<int64_t>(resp.statusCode)));
            c.SetOutputValue("ResponseBody", Variant(resp.body));
            c.SetOutputValue("ErrorMessage", Variant(resp.error));

            if (resp.ok()) {
                c.Log("[HTTP.Request] " + std::to_string(resp.statusCode)
                      + " OK (" + std::to_string(resp.body.size()) + " bytes)");
                c.ActivateOutputFlow(successPinId);
            } else {
                c.LogError("[HTTP.Request] " + std::to_string(resp.statusCode)
                           + " " + resp.error);
                c.ActivateOutputFlow(errorPinId);
            }
        };

        ctx.RunAsync(std::move(dispatcher), std::move(onComplete));
        return true;
    };

    // ========================================================================
    // HTTP.Download
    // 输入：  URL(String), Headers(String/JSON), TimeoutSeconds(Integer)
    // 输出：  → onSuccess, → onError
    //         Data(String 原始字节), Size(Integer), StatusCode(Integer), ErrorMessage(String)
    //
    // 实现：强制 GET 请求，复用 IHttpClient::SendAsync。
    // 跨平台：native = cpp-httplib 后台线程；WebGL = emscripten_fetch（MEMFS 内存）
    // 注意：二进制数据存在 std::string（按字节长度，非 null-terminated 语义），
    //       如需写文件请在蓝图中用 File.Write 节点衔接。
    // ========================================================================
    handlers["HTTP.Download"] = [&runner](ExecutionContext& ctx) -> bool {

        IHttpClient* client = BP_GetHttpClient();
        if (!client) {
            ctx.LogError("[HTTP.Download] No HttpClient registered. Call BP_InitDefaultHttpClient() first.");
            ctx.SetOutputValue("Data",         Variant(std::string("")));
            ctx.SetOutputValue("Size",         Variant(static_cast<int64_t>(0)));
            ctx.SetOutputValue("StatusCode",   Variant(static_cast<int64_t>(0)));
            ctx.SetOutputValue("ErrorMessage", Variant(std::string("No HttpClient registered")));
            ctx.ActivateOutputFlow("onError");
            return true;
        }

        HttpRequest req;
        req.url    = ctx.GetInputValue("URL").asString();
        req.method = "GET";

        std::string headersJson = ctx.GetInputValue("Headers").asString();
        if (!headersJson.empty()) {
            crude_json::value hj = crude_json::value::parse(headersJson);
            if (hj.is_object()) {
                for (const auto& kv : hj.get<crude_json::object>())
                    if (kv.second.is_string())
                        req.headers[kv.first] = kv.second.get<std::string>();
            }
        }

        auto timeoutVar = ctx.GetInputValue("TimeoutSeconds");
        if (timeoutVar.type == PinDataType::Integer || timeoutVar.type == PinDataType::Float)
            req.timeoutSeconds = static_cast<int>(timeoutVar.asInt());

        PinId successPinId = ctx.GetPinId("onSuccess");
        PinId errorPinId   = ctx.GetPinId("onError");

        ctx.MarkDownstreamAsHandled("onSuccess");
        ctx.MarkDownstreamAsHandled("onError");

        auto sharedResp = std::make_shared<HttpResponse>();

        auto dispatcher = [client, req, sharedResp](ExecutionContext::AsyncResolve resolve) mutable
        {
            client->SendAsync(req,
                [sharedResp, resolve = std::move(resolve)](HttpResponse resp) mutable
                {
                    *sharedResp = std::move(resp);
                    resolve();
                });
        };

        auto onComplete = [sharedResp, successPinId, errorPinId](ExecutionContext& c) mutable
        {
            const HttpResponse& resp = *sharedResp;
            int64_t size = static_cast<int64_t>(resp.body.size());
            c.SetOutputValue("Data",         Variant(resp.body));
            c.SetOutputValue("Size",         Variant(size));
            c.SetOutputValue("StatusCode",   Variant(static_cast<int64_t>(resp.statusCode)));
            c.SetOutputValue("ErrorMessage", Variant(resp.error));

            if (resp.ok()) {
                c.Log("[HTTP.Download] " + std::to_string(resp.statusCode)
                      + " OK (" + std::to_string(size) + " bytes)");
                c.ActivateOutputFlow(successPinId);
            } else {
                c.LogError("[HTTP.Download] " + std::to_string(resp.statusCode)
                           + " " + resp.error);
                c.ActivateOutputFlow(errorPinId);
            }
        };

        ctx.RunAsync(std::move(dispatcher), std::move(onComplete));
        return true;
    };

    // ========================================================================
    // JSON.GetPath — 支持路径：choices[0].message.content
    // 输入：JSON(String), Path(String)
    // 输出：Value(String), Found(Boolean)
    // ========================================================================
    handlers["JSON.GetPath"] = [](ExecutionContext& ctx) -> bool {
        std::string json = ctx.GetInputValue("JSON").asString();
        std::string path = ctx.GetInputValue("Path").asString();

        // 解析路径段：choices[0].message.content → ["choices","0","message","content"]
        std::vector<std::string> segments;
        std::string current;
        for (size_t i = 0; i < path.size(); ++i) {
            char c = path[i];
            if (c == '.') {
                if (!current.empty()) { segments.push_back(current); current.clear(); }
            } else if (c == '[') {
                if (!current.empty()) { segments.push_back(current); current.clear(); }
                ++i;
                while (i < path.size() && path[i] != ']') { current += path[i]; ++i; }
                segments.push_back(current); current.clear();
            } else {
                current += c;
            }
        }
        if (!current.empty()) segments.push_back(current);

        crude_json::value root = crude_json::value::parse(json);
        crude_json::value* cur = &root;
        bool found = !root.is_null();

        for (const auto& seg : segments) {
            if (!found) break;
            bool isIndex = !seg.empty();
            for (char c : seg) if (!std::isdigit((unsigned char)c)) { isIndex = false; break; }

            if (isIndex && cur->is_array()) {
                size_t idx = static_cast<size_t>(std::stoul(seg));
                if (idx < cur->get<crude_json::array>().size())
                    cur = &(cur->get<crude_json::array>()[idx]);
                else
                    found = false;
            } else if (cur->is_object()) {
                auto& obj = cur->get<crude_json::object>();
                auto it = obj.find(seg);
                if (it != obj.end()) cur = &(it->second);
                else                 found = false;
            } else {
                found = false;
            }
        }

        std::string resultStr;
        if (found && cur) {
            if      (cur->is_string())  resultStr = cur->get<std::string>();
            else if (cur->is_number())  resultStr = std::to_string(cur->get<double>());
            else if (cur->is_boolean()) resultStr = cur->get<bool>() ? "true" : "false";
            else                        resultStr = cur->dump();
        }

        ctx.SetOutputValue("Value", Variant(resultStr));
        ctx.SetOutputValue("Found", Variant(found));
        return true;
    };

    // ========================================================================
    // HTTP.Get — 快捷 GET 节点
    // ========================================================================
    handlers["HTTP.Get"] = [&runner](ExecutionContext& ctx) -> bool {
        IHttpClient* client = BP_GetHttpClient();
        if (!client) {
            ctx.SetOutputValue("StatusCode",   Variant(static_cast<int64_t>(0)));
            ctx.SetOutputValue("ResponseBody", Variant(std::string("")));
            ctx.SetOutputValue("ErrorMessage", Variant(std::string("No HttpClient registered")));
            ctx.ActivateOutputFlow("onError");
            return true;
        }
        HttpRequest req;
        req.url    = ctx.GetInputValue("URL").asString();
        req.method = "GET";
        std::string headersJson = ctx.GetInputValue("Headers").asString();
        if (!headersJson.empty()) {
            crude_json::value hj = crude_json::value::parse(headersJson);
            if (hj.is_object())
                for (const auto& kv : hj.get<crude_json::object>())
                    if (kv.second.is_string()) req.headers[kv.first] = kv.second.get<std::string>();
        }
        auto tv = ctx.GetInputValue("TimeoutSeconds");
        if (tv.type == PinDataType::Integer || tv.type == PinDataType::Float)
            req.timeoutSeconds = static_cast<int>(tv.asInt());

        PinId successPinId = ctx.GetPinId("onSuccess");
        PinId errorPinId   = ctx.GetPinId("onError");
        ctx.MarkDownstreamAsHandled("onSuccess");
        ctx.MarkDownstreamAsHandled("onError");
        auto sharedResp = std::make_shared<HttpResponse>();
        ctx.RunAsync(
            [client, req, sharedResp](ExecutionContext::AsyncResolve resolve) mutable {
                client->SendAsync(req, [sharedResp, resolve=std::move(resolve)](HttpResponse r) mutable {
                    *sharedResp = std::move(r); resolve();
                });
            },
            [sharedResp, successPinId, errorPinId](ExecutionContext& c) mutable {
                const HttpResponse& r = *sharedResp;
                c.SetOutputValue("StatusCode",   Variant(static_cast<int64_t>(r.statusCode)));
                c.SetOutputValue("ResponseBody", Variant(r.body));
                c.SetOutputValue("ErrorMessage", Variant(r.error));
                c.ActivateOutputFlow(r.ok() ? successPinId : errorPinId);
            });
        return true;
    };

    // ========================================================================
    // HTTP.Post — 快捷 POST 节点
    // ========================================================================
    handlers["HTTP.Post"] = [&runner](ExecutionContext& ctx) -> bool {
        IHttpClient* client = BP_GetHttpClient();
        if (!client) {
            ctx.SetOutputValue("StatusCode",   Variant(static_cast<int64_t>(0)));
            ctx.SetOutputValue("ResponseBody", Variant(std::string("")));
            ctx.SetOutputValue("ErrorMessage", Variant(std::string("No HttpClient registered")));
            ctx.ActivateOutputFlow("onError");
            return true;
        }
        HttpRequest req;
        req.url    = ctx.GetInputValue("URL").asString();
        req.method = "POST";
        req.body   = ctx.GetInputValue("Body").asString();
        std::string headersJson = ctx.GetInputValue("Headers").asString();
        if (!headersJson.empty()) {
            crude_json::value hj = crude_json::value::parse(headersJson);
            if (hj.is_object())
                for (const auto& kv : hj.get<crude_json::object>())
                    if (kv.second.is_string()) req.headers[kv.first] = kv.second.get<std::string>();
        }
        auto tv = ctx.GetInputValue("TimeoutSeconds");
        if (tv.type == PinDataType::Integer || tv.type == PinDataType::Float)
            req.timeoutSeconds = static_cast<int>(tv.asInt());

        PinId successPinId = ctx.GetPinId("onSuccess");
        PinId errorPinId   = ctx.GetPinId("onError");
        ctx.MarkDownstreamAsHandled("onSuccess");
        ctx.MarkDownstreamAsHandled("onError");
        auto sharedResp = std::make_shared<HttpResponse>();
        ctx.RunAsync(
            [client, req, sharedResp](ExecutionContext::AsyncResolve resolve) mutable {
                client->SendAsync(req, [sharedResp, resolve=std::move(resolve)](HttpResponse r) mutable {
                    *sharedResp = std::move(r); resolve();
                });
            },
            [sharedResp, successPinId, errorPinId](ExecutionContext& c) mutable {
                const HttpResponse& r = *sharedResp;
                c.SetOutputValue("StatusCode",   Variant(static_cast<int64_t>(r.statusCode)));
                c.SetOutputValue("ResponseBody", Variant(r.body));
                c.SetOutputValue("ErrorMessage", Variant(r.error));
                c.ActivateOutputFlow(r.ok() ? successPinId : errorPinId);
            });
        return true;
    };

    // ========================================================================
    // Web.Search — DuckDuckGo Lite 无 Key 搜索（HTML scraping）
    //
    // 实现：GET https://lite.duckduckgo.com/lite/?q=<query>
    //   解析 HTML 中 <a class="result-link"> 提取标题+URL，
    //   <td class="result-snippet"> 提取摘要。
    // 跨平台：native + WebGL（同 HTTP.Get，底层走 IHttpClient）
    // ========================================================================
    handlers["Web.Search"] = [&runner](ExecutionContext& ctx) -> bool {
        IHttpClient* client = BP_GetHttpClient();
        if (!client) {
            ctx.SetOutputValue("Results",     Variant(std::string("[]")));
            ctx.SetOutputValue("ResultText",  Variant(std::string("")));
            ctx.SetOutputValue("ErrorMessage",Variant(std::string("No HttpClient registered")));
            ctx.ActivateOutputFlow("onError");
            return true;
        }

        std::string query = ctx.GetInputValue("Query").asString();
        auto maxVar = ctx.GetInputValue("MaxResults");
        int maxResults = (maxVar.type == PinDataType::Integer) ? static_cast<int>(maxVar.asInt()) : 5;
        if (maxResults <= 0) maxResults = 5;

        // URL 编码 query
        std::string encoded;
        for (unsigned char c : query) {
            if (std::isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
                encoded += c;
            } else {
                char buf[4];
                snprintf(buf, sizeof(buf), "%%%02X", c);
                encoded += buf;
            }
        }

        HttpRequest req;
        req.url    = "https://lite.duckduckgo.com/lite/?q=" + encoded;
        req.method = "GET";
        req.headers["User-Agent"] = "Mozilla/5.0 (compatible; BlueprintRuntime/1.0)";
        req.timeoutSeconds = 15;

        PinId successPinId = ctx.GetPinId("onSuccess");
        PinId errorPinId   = ctx.GetPinId("onError");
        ctx.MarkDownstreamAsHandled("onSuccess");
        ctx.MarkDownstreamAsHandled("onError");

        auto sharedResp    = std::make_shared<HttpResponse>();
        int  capturedMax   = maxResults;

        ctx.RunAsync(
            [client, req, sharedResp](ExecutionContext::AsyncResolve resolve) mutable {
                client->SendAsync(req, [sharedResp, resolve=std::move(resolve)](HttpResponse r) mutable {
                    *sharedResp = std::move(r); resolve();
                });
            },
            [sharedResp, successPinId, errorPinId, capturedMax](ExecutionContext& c) mutable {
                const HttpResponse& r = *sharedResp;
                if (!r.ok()) {
                    c.SetOutputValue("Results",      Variant(std::string("[]")));
                    c.SetOutputValue("ResultText",   Variant(std::string("")));
                    c.SetOutputValue("ErrorMessage", Variant(r.error.empty()
                        ? "HTTP " + std::to_string(r.statusCode) : r.error));
                    c.ActivateOutputFlow(errorPinId);
                    return;
                }

                // 简单 HTML 解析：提取 <a class="result-link" href="...">title</a>
                // 和 <td class="result-snippet">snippet</td>
                const std::string& html = r.body;
                struct SRItem { std::string title, url, snippet; };
                std::vector<SRItem> items;

                auto extractBetween = [](const std::string& s, const std::string& open,
                                          const std::string& close, size_t from) -> std::pair<std::string, size_t> {
                    size_t p = s.find(open, from);
                    if (p == std::string::npos) return {"", std::string::npos};
                    p += open.size();
                    size_t e = s.find(close, p);
                    if (e == std::string::npos) return {"", std::string::npos};
                    return {s.substr(p, e - p), e + close.size()};
                };

                auto stripTags = [](const std::string& s) -> std::string {
                    std::string out; bool inTag = false;
                    for (char ch : s) {
                        if (ch == '<') { inTag = true; continue; }
                        if (ch == '>') { inTag = false; continue; }
                        if (!inTag) out += ch;
                    }
                    return out;
                };

                size_t pos = 0;
                while ((int)items.size() < capturedMax) {
                    // 找 href
                    size_t hrefP = html.find("class=\"result-link\"", pos);
                    if (hrefP == std::string::npos) break;
                    // 往前找 href="..."
                    size_t tagStart = html.rfind('<', hrefP);
                    auto [href, p1] = extractBetween(html, "href=\"", "\"", tagStart);
                    // 标题为 >...</a>
                    auto [titleRaw, p2] = extractBetween(html, ">", "</a>", hrefP);
                    std::string title = stripTags(titleRaw);
                    // 找 snippet
                    auto [snipRaw, p3] = extractBetween(html, "class=\"result-snippet\">", "</td>", hrefP);
                    std::string snippet = stripTags(snipRaw);
                    // 去首尾空白
                    auto trim = [](std::string& str) {
                        size_t s = str.find_first_not_of(" \t\r\n");
                        size_t e = str.find_last_not_of(" \t\r\n");
                        str = (s == std::string::npos) ? "" : str.substr(s, e - s + 1);
                    };
                    trim(title); trim(href); trim(snippet);

                    if (!href.empty() && !title.empty()) {
                        SRItem item;
                        item.title   = title;
                        item.url     = href;
                        item.snippet = snippet;
                        items.push_back(std::move(item));
                    }

                    pos = (p2 != std::string::npos) ? p2 : hrefP + 1;
                    if (pos == std::string::npos) break;
                }

                // 构建 JSON 数组
                std::string json = "[";
                std::string text;
                for (size_t i = 0; i < items.size(); ++i) {
                    if (i) json += ",";
                    auto escape = [](const std::string& s) {
                        std::string out;
                        for (char ch : s) {
                            if      (ch == '"')  out += "\\\"";
                            else if (ch == '\\') out += "\\\\";
                            else if (ch == '\n') out += "\\n";
                            else if (ch == '\r') out += "\\r";
                            else                 out += ch;
                        }
                        return out;
                    };
                    json += "{\"title\":\"" + escape(items[i].title) + "\","
                             "\"url\":\""   + escape(items[i].url) + "\","
                             "\"snippet\":\"" + escape(items[i].snippet) + "\"}";
                    text += std::to_string(i+1) + ". " + items[i].title + "\n"
                          + "   " + items[i].url + "\n"
                          + "   " + items[i].snippet + "\n\n";
                }
                json += "]";

                c.SetOutputValue("Results",      Variant(json));
                c.SetOutputValue("ResultText",   Variant(text));
                c.SetOutputValue("ErrorMessage", Variant(std::string("")));
                c.Log("[Web.Search] " + std::to_string(items.size()) + " results");
                c.ActivateOutputFlow(successPinId);
            });
        return true;
    };

    // ========================================================================
    // Code.Run — 执行子进程命令（native only，WebGL 走 onError）
    // ========================================================================
    handlers["Code.Run"] = [&runner](ExecutionContext& ctx) -> bool {
#ifdef __EMSCRIPTEN__
        ctx.SetOutputValue("Stdout",       Variant(std::string("")));
        ctx.SetOutputValue("Stderr",       Variant(std::string("")));
        ctx.SetOutputValue("ExitCode",     Variant(static_cast<int64_t>(-1)));
        ctx.SetOutputValue("ErrorMessage", Variant(std::string("Code.Run not supported on WebGL")));
        ctx.ActivateOutputFlow("onError");
        return true;
#else
        std::string command = ctx.GetInputValue("Command").asString();
        std::string workDir = ctx.GetInputValue("WorkDir").asString();
        auto tv = ctx.GetInputValue("TimeoutSeconds");
        int timeoutSec = (tv.type == PinDataType::Integer) ? static_cast<int>(tv.asInt()) : 30;
        if (timeoutSec <= 0) timeoutSec = 30;

        if (command.empty()) {
            ctx.SetOutputValue("Stdout",       Variant(std::string("")));
            ctx.SetOutputValue("Stderr",       Variant(std::string("")));
            ctx.SetOutputValue("ExitCode",     Variant(static_cast<int64_t>(-1)));
            ctx.SetOutputValue("ErrorMessage", Variant(std::string("Command is empty")));
            ctx.ActivateOutputFlow("onError");
            return true;
        }

        PinId successPinId = ctx.GetPinId("onSuccess");
        PinId errorPinId   = ctx.GetPinId("onError");
        ctx.MarkDownstreamAsHandled("onSuccess");
        ctx.MarkDownstreamAsHandled("onError");

        struct RunResult {
            std::string stdoutStr, stderrStr, errorMsg;
            int exitCode = -1;
        };
        auto sharedResult = std::make_shared<RunResult>();

        ctx.RunAsync(
            [command, workDir, timeoutSec, sharedResult](ExecutionContext::AsyncResolve resolve) mutable {
                std::thread([command, workDir, timeoutSec, sharedResult, resolve=std::move(resolve)]() mutable {

#ifdef _WIN32
                    // ── Windows：CreateProcess + WaitForSingleObject + TerminateProcess ──
                    std::string fullCmd;
                    if (!workDir.empty())
                        fullCmd = "cmd /c \"cd /d \"" + workDir + "\" && " + command + "\" 2>&1";
                    else
                        fullCmd = "cmd /c \"" + command + "\" 2>&1";

                    // 创建匿名管道捕获 stdout
                    HANDLE hReadPipe = nullptr, hWritePipe = nullptr;
                    SECURITY_ATTRIBUTES sa{sizeof(SECURITY_ATTRIBUTES), nullptr, TRUE};
                    if (!CreatePipe(&hReadPipe, &hWritePipe, &sa, 0)) {
                        sharedResult->errorMsg = "CreatePipe failed";
                        resolve(); return;
                    }
                    SetHandleInformation(hReadPipe, HANDLE_FLAG_INHERIT, 0);

                    STARTUPINFOA si{};
                    si.cb          = sizeof(si);
                    si.hStdOutput  = hWritePipe;
                    si.hStdError   = hWritePipe;
                    si.dwFlags     = STARTF_USESTDHANDLES;

                    PROCESS_INFORMATION pi{};
                    std::vector<char> cmdBuf(fullCmd.begin(), fullCmd.end());
                    cmdBuf.push_back('\0');

                    if (!CreateProcessA(nullptr, cmdBuf.data(), nullptr, nullptr,
                                        TRUE, CREATE_NO_WINDOW, nullptr,
                                        workDir.empty() ? nullptr : workDir.c_str(),
                                        &si, &pi)) {
                        CloseHandle(hWritePipe); CloseHandle(hReadPipe);
                        sharedResult->errorMsg = "CreateProcess failed: " + command;
                        resolve(); return;
                    }
                    CloseHandle(hWritePipe);  // 子进程拥有写端

                    // 读 stdout（在独立线程，避免 pipe 缓冲区满死锁）
                    std::string captured;
                    std::thread reader([&]() {
                        char buf[4096];
                        DWORD read = 0;
                        while (ReadFile(hReadPipe, buf, sizeof(buf)-1, &read, nullptr) && read > 0) {
                            buf[read] = '\0';
                            captured += buf;
                        }
                    });

                    DWORD waitMs = static_cast<DWORD>(timeoutSec) * 1000;
                    DWORD wr = WaitForSingleObject(pi.hProcess, waitMs);
                    bool timedOut = (wr == WAIT_TIMEOUT);

                    if (timedOut) {
                        TerminateProcess(pi.hProcess, 1);
                        WaitForSingleObject(pi.hProcess, 3000);
                        sharedResult->errorMsg = "Process killed: timeout (" + std::to_string(timeoutSec) + "s)";
                        sharedResult->exitCode = -1;
                    } else {
                        DWORD code = 0;
                        GetExitCodeProcess(pi.hProcess, &code);
                        sharedResult->exitCode = static_cast<int>(code);
                    }

                    CloseHandle(pi.hProcess);
                    CloseHandle(pi.hThread);
                    CloseHandle(hReadPipe);
                    reader.join();
                    sharedResult->stdoutStr = captured;

#else
                    // ── POSIX：fork + execvp + waitpid(WNOHANG) 轮询超时 + kill ──
                    // 用 popen 方式同样可实现，但无法可靠 kill；改用 pipe+fork
                    std::string fullCmd;
                    if (!workDir.empty())
                        fullCmd = "cd \"" + workDir + "\" && " + command + " 2>&1";
                    else
                        fullCmd = command + " 2>&1";

                    int pipeFd[2];
                    if (pipe(pipeFd) != 0) {
                        sharedResult->errorMsg = "pipe() failed";
                        resolve(); return;
                    }

                    pid_t pid = fork();
                    if (pid < 0) {
                        close(pipeFd[0]); close(pipeFd[1]);
                        sharedResult->errorMsg = "fork() failed";
                        resolve(); return;
                    }

                    if (pid == 0) {
                        // 子进程
                        close(pipeFd[0]);
                        dup2(pipeFd[1], STDOUT_FILENO);
                        dup2(pipeFd[1], STDERR_FILENO);
                        close(pipeFd[1]);
                        execl("/bin/sh", "sh", "-c", fullCmd.c_str(), nullptr);
                        _exit(127);
                    }

                    // 父进程：读 pipe + 超时监控
                    close(pipeFd[1]);

                    auto deadline = std::chrono::steady_clock::now()
                                  + std::chrono::seconds(timeoutSec);
                    std::string captured;
                    bool timedOut = false;

                    // 设 pipe 为非阻塞
                    int flags = fcntl(pipeFd[0], F_GETFL, 0);
                    fcntl(pipeFd[0], F_SETFL, flags | O_NONBLOCK);

                    while (true) {
                        // 读取可用数据
                        char buf[4096];
                        ssize_t n = read(pipeFd[0], buf, sizeof(buf)-1);
                        if (n > 0) { buf[n] = '\0'; captured += buf; }

                        // 检查子进程是否结束
                        int status = 0;
                        pid_t ret = waitpid(pid, &status, WNOHANG);
                        if (ret == pid) {
                            // 进程已结束，把剩余数据读完
                            while ((n = read(pipeFd[0], buf, sizeof(buf)-1)) > 0) {
                                buf[n] = '\0'; captured += buf;
                            }
                            sharedResult->exitCode = WIFEXITED(status) ? WEXITSTATUS(status) : -1;
                            break;
                        }

                        if (std::chrono::steady_clock::now() >= deadline) {
                            timedOut = true;
                            kill(pid, SIGKILL);
                            waitpid(pid, nullptr, 0);
                            sharedResult->errorMsg = "Process killed: timeout ("
                                                   + std::to_string(timeoutSec) + "s)";
                            sharedResult->exitCode = -1;
                            break;
                        }

                        std::this_thread::sleep_for(std::chrono::milliseconds(50));
                    }
                    close(pipeFd[0]);
                    sharedResult->stdoutStr = captured;
                    (void)timedOut;
#endif
                    resolve();
                }).detach();
            },
            [sharedResult, successPinId, errorPinId](ExecutionContext& c) mutable {
                c.SetOutputValue("Stdout",       Variant(sharedResult->stdoutStr));
                c.SetOutputValue("Stderr",       Variant(sharedResult->stderrStr));
                c.SetOutputValue("ExitCode",     Variant(static_cast<int64_t>(sharedResult->exitCode)));
                c.SetOutputValue("ErrorMessage", Variant(sharedResult->errorMsg));
                if (sharedResult->errorMsg.empty() && sharedResult->exitCode == 0) {
                    c.Log("[Code.Run] exit=0");
                    c.ActivateOutputFlow(successPinId);
                } else {
                    c.LogError("[Code.Run] exit=" + std::to_string(sharedResult->exitCode)
                               + " " + sharedResult->errorMsg);
                    c.ActivateOutputFlow(errorPinId);
                }
            });
        return true;
#endif // __EMSCRIPTEN__
    };
}

} // namespace Runtime
} // namespace NodeEditor
