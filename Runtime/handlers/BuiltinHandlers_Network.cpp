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
    // 输入：  URL(String), Method(String), Body(String), Headers(String/JSON)
    // 输出：  → onSuccess, → onError
    //         StatusCode(Integer), ResponseBody(String), ErrorMessage(String)
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

        // 默认 method
        if (req.method.empty()) req.method = "POST";

        // Headers：可选，JSON 格式字符串 {"Key": "Value", ...}
        std::string headersJson = ctx.GetInputValue("Headers").asString();
        if (!headersJson.empty()) {
            // 用 crude_json 解析 headers JSON
            // 简单手动解析：找 "key": "value" 对
            // 利用已有的 GetField 逻辑风格做轻量解析
            // 此处用 crude_json（Runtime 内部已有依赖）
            // 实现：遍历所有 "xxx": "yyy" 对
            size_t pos = 0;
            while (pos < headersJson.size()) {
                // 找下一个 "key"
                auto keyStart = headersJson.find('"', pos);
                if (keyStart == std::string::npos) break;
                auto keyEnd = headersJson.find('"', keyStart + 1);
                if (keyEnd == std::string::npos) break;
                std::string key = headersJson.substr(keyStart + 1, keyEnd - keyStart - 1);
                // 找 :
                auto colon = headersJson.find(':', keyEnd + 1);
                if (colon == std::string::npos) break;
                // 找 value
                auto valStart = headersJson.find('"', colon + 1);
                if (valStart == std::string::npos) break;
                auto valEnd = headersJson.find('"', valStart + 1);
                if (valEnd == std::string::npos) break;
                std::string val = headersJson.substr(valStart + 1, valEnd - valStart - 1);
                req.headers[key] = val;
                pos = valEnd + 1;
            }
        }

        // 超时（可选，默认 30s）
        auto timeoutVar = ctx.GetInputValue("TimeoutSeconds");
        if (timeoutVar.type == PinDataType::Integer || timeoutVar.type == PinDataType::Float)
            req.timeoutSeconds = static_cast<int>(timeoutVar.asInt());

        // 预先拿好 PinId（避免异步回调时 ctx 状态可能变化）
        PinId successPinId = ctx.GetPinId("onSuccess");
        PinId errorPinId   = ctx.GetPinId("onError");

        // 获取 alive 标志
        auto alive = runner.GetAliveFlag();

        // 标记下游已处理（阻止主流程继续走这两条线）
        ctx.MarkDownstreamAsHandled("onSuccess");
        ctx.MarkDownstreamAsHandled("onError");

        // AcquireAsync：告知 runner 有挂起的异步操作
        runner.AcquireAsync();

        // 保存 ctx 指针（SendAsync 回调时使用）
        ExecutionContext* pCtx = &ctx;

        client->SendAsync(req,
            [pCtx, successPinId, errorPinId, alive, &runner](HttpResponse resp) mutable
            {
                if (!alive->load(std::memory_order_acquire)) {
                    runner.ReleaseAsync();
                    return;
                }

                pCtx->SetOutputValue("StatusCode",   Variant(static_cast<int64_t>(resp.statusCode)));
                pCtx->SetOutputValue("ResponseBody", Variant(resp.body));
                pCtx->SetOutputValue("ErrorMessage", Variant(resp.error));

                if (resp.ok()) {
                    pCtx->Log("[HTTP.Request] " + std::to_string(resp.statusCode)
                              + " OK (" + std::to_string(resp.body.size()) + " bytes)");
                    pCtx->ActivateOutputFlow(successPinId);
                } else {
                    pCtx->LogError("[HTTP.Request] " + std::to_string(resp.statusCode)
                                   + " " + resp.error);
                    pCtx->ActivateOutputFlow(errorPinId);
                }

                runner.ReleaseAsync();
            });

        return true;
    };

    // ========================================================================
    // JSON.GetPath
    // 支持路径：choices[0].message.content
    // 输入：JSON(String), Path(String)
    // 输出：Value(String), Found(Boolean)
    // ========================================================================
    handlers["JSON.GetPath"] = [](ExecutionContext& ctx) -> bool {
        std::string json = ctx.GetInputValue("JSON").asString();
        std::string path = ctx.GetInputValue("Path").asString();

        // 解析路径：按 '.' 和 '[n]' 分段
        // 例：choices[0].message.content → ["choices", "0", "message", "content"]
        std::vector<std::string> segments;
        std::string current;
        for (size_t i = 0; i < path.size(); ++i) {
            char c = path[i];
            if (c == '.') {
                if (!current.empty()) { segments.push_back(current); current.clear(); }
            } else if (c == '[') {
                if (!current.empty()) { segments.push_back(current); current.clear(); }
                // 读数字直到 ']'
                ++i;
                while (i < path.size() && path[i] != ']') { current += path[i]; ++i; }
                segments.push_back(current); current.clear();
            } else {
                current += c;
            }
        }
        if (!current.empty()) segments.push_back(current);

        // 沿路径遍历 JSON 字符串（用 crude_json 解析）
        // 简单实现：先用 crude_json 完整解析，然后按路径取值
        // crude_json::value 支持 operator[]
        crude_json::value root = crude_json::value::parse(json);
        crude_json::value* cur = &root;

        bool found = !root.is_null();
        for (const auto& seg : segments) {
            if (!found) break;
            // 判断是数组索引（纯数字）还是对象键
            bool isIndex = !seg.empty();
            for (char c : seg) if (!std::isdigit((unsigned char)c)) { isIndex = false; break; }

            if (isIndex && cur->is_array()) {
                size_t idx = static_cast<size_t>(std::stoul(seg));
                if (idx < cur->get<crude_json::array>().size()) {
                    cur = &(cur->get<crude_json::array>()[idx]);
                } else {
                    found = false;
                }
            } else if (cur->is_object()) {
                auto& obj = cur->get<crude_json::object>();
                auto it = obj.find(seg);
                if (it != obj.end()) {
                    cur = &(it->second);
                } else {
                    found = false;
                }
            } else {
                found = false;
            }
        }

        std::string resultStr;
        if (found && cur) {
            if (cur->is_string())       resultStr = cur->get<std::string>();
            else if (cur->is_number())  resultStr = std::to_string(cur->get<double>());
            else if (cur->is_boolean()) resultStr = cur->get<bool>() ? "true" : "false";
            else                        resultStr = cur->dump(); // 对象/数组序列化
        }

        ctx.SetOutputValue("Value", Variant(resultStr));
        ctx.SetOutputValue("Found", Variant(found));
        return true;
    };
}

} // namespace Runtime
} // namespace NodeEditor
