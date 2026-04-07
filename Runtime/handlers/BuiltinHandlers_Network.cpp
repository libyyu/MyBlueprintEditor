// Runtime/handlers/BuiltinHandlers_Network.cpp
// 网络相关节点处理器：HTTP.Request, JSON.GetPath
#include "BuiltinHandlers_Network.h"
#include "../BlueprintRunner.h"
#include "../Http/IHttpClient.h"
#include "../../Utils/Json/crude_json.h"

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
}

} // namespace Runtime
} // namespace NodeEditor
