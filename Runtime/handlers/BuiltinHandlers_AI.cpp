// Runtime/handlers/BuiltinHandlers_AI.cpp
// AI / LLM 相关节点处理器
//
// 节点列表：
//   JSON.Build        — 从键值对动态构建 JSON 对象字符串
//   JSON.SetPath      — 向 JSON 对象设置（支持嵌套路径）
//   JSON.ArrayPush    — 向 JSON 数组末尾追加元素
//   String.Template   — {{variable}} 风格占位符替换
//   LLM.Chat          — OpenAI 兼容 Chat 调用（封装 HTTP.Request）

#include "BuiltinHandlers_AI.h"
#include "../BlueprintRunner.h"
#include "../Http/IHttpClient.h"
#include "../../Utils/Json/crude_json.h"
#include <sstream>
#include <regex>
#ifndef __EMSCRIPTEN__
#  include <fstream>
#  include <filesystem>
#endif

namespace NodeEditor {
namespace Runtime {

// ============================================================================
// 内部工具：Variant ↔ crude_json（与 BuiltinHandlers_Data 保持一致）
// ============================================================================

static crude_json::value variantToJson_AI(const Variant& v)
{
    switch (v.type)
    {
    case PinDataType::Boolean: return crude_json::value(v.asBool());
    case PinDataType::Integer: return crude_json::value(static_cast<double>(v.asInt()));
    case PinDataType::Float:   return crude_json::value(v.asFloat());
    case PinDataType::String:  return crude_json::value(v.asString());
    case PinDataType::Array:
    {
        crude_json::array arr;
        for (size_t i = 0; i < v.arraySize(); ++i)
            arr.push_back(variantToJson_AI(v.arrayGet(i)));
        return crude_json::value(std::move(arr));
    }
    case PinDataType::Map:
    {
        crude_json::object obj;
        for (const auto& kv : v.asMap())
            obj[kv.first.asString()] = variantToJson_AI(kv.second);
        return crude_json::value(std::move(obj));
    }
    default: return crude_json::value(); // null
    }
}

static Variant jsonToVariant_AI(const crude_json::value& j)
{
    switch (j.type())
    {
    case crude_json::type_t::boolean: return Variant(j.get<bool>());
    case crude_json::type_t::number:
    {
        double d = j.get<double>();
        if (d == static_cast<double>(static_cast<int64_t>(d)))
            return Variant(static_cast<int64_t>(d));
        return Variant(d);
    }
    case crude_json::type_t::string: return Variant(j.get<std::string>());
    case crude_json::type_t::array:
    {
        std::vector<Variant> arr;
        for (const auto& e : j.get<crude_json::array>())
            arr.push_back(jsonToVariant_AI(e));
        return Variant(std::move(arr));
    }
    case crude_json::type_t::object:
    {
        Variant result;
        result.type = PinDataType::Map;
        for (const auto& kv : j.get<crude_json::object>())
            result.mapSet(Variant(kv.first), jsonToVariant_AI(kv.second));
        return result;
    }
    default: return Variant();
    }
}

// ============================================================================
// JSON 路径工具：解析 "a.b[0].c" → ["a","b","0","c"]
// ============================================================================
static std::vector<std::string> parsePath(const std::string& path)
{
    std::vector<std::string> segs;
    std::string cur;
    for (size_t i = 0; i < path.size(); ++i) {
        char c = path[i];
        if (c == '.') {
            if (!cur.empty()) { segs.push_back(cur); cur.clear(); }
        } else if (c == '[') {
            if (!cur.empty()) { segs.push_back(cur); cur.clear(); }
            ++i;
            while (i < path.size() && path[i] != ']') cur += path[i++];
            segs.push_back(cur); cur.clear();
        } else {
            cur += c;
        }
    }
    if (!cur.empty()) segs.push_back(cur);
    return segs;
}

// ============================================================================
// JSON.SetPath 核心：递归写入
// ============================================================================
static void setAtPath(crude_json::value& node,
                      const std::vector<std::string>& segs,
                      size_t idx,
                      const crude_json::value& val)
{
    if (idx >= segs.size()) return;
    const std::string& seg = segs[idx];
    bool isLast = (idx + 1 == segs.size());

    // 判断下一级是否是数组索引
    auto isNum = [](const std::string& s) {
        if (s.empty()) return false;
        for (char c : s) if (!std::isdigit((unsigned char)c)) return false;
        return true;
    };

    if (isNum(seg)) {
        // 数组索引
        if (!node.is_array()) node = crude_json::value(crude_json::array{});
        size_t i = std::stoul(seg);
        auto& arr = node.get<crude_json::array>();
        while (arr.size() <= i) arr.emplace_back(crude_json::value{});
        if (isLast) arr[i] = val;
        else        setAtPath(arr[i], segs, idx + 1, val);
    } else {
        // 对象键
        if (!node.is_object()) node = crude_json::value(crude_json::object{});
        auto& obj = node.get<crude_json::object>();
        if (isLast) { obj[seg] = val; }
        else {
            if (obj.find(seg) == obj.end()) {
                // 预判下一级：数字→array，否则→object
                bool nextIsIdx = (idx + 2 <= segs.size()) && isNum(segs[idx + 1]);
                obj[seg] = nextIsIdx
                    ? crude_json::value(crude_json::array{})
                    : crude_json::value(crude_json::object{});
            }
            setAtPath(obj[seg], segs, idx + 1, val);
        }
    }
}

// ============================================================================
// RegisterHandlers_AI
// ============================================================================
void RegisterHandlers_AI(
    std::unordered_map<std::string, NodeHandler>& handlers,
    BlueprintRunner& runner)
{
    // ========================================================================
    // JSON.Build
    // 输入：Keys(Array<String>), Values(Array<Any>)
    // 输出：JSON(String)
    //
    // 从两个等长数组构建 JSON 对象字符串。
    // 例：Keys=["model","temperature"], Values=["gpt-4o", 0.7]
    //   → {"model":"gpt-4o","temperature":0.7}
    // ========================================================================
    handlers["JSON.Build"] = [](ExecutionContext& ctx) -> bool {
        Variant keys   = ctx.GetInputValue("Keys");
        Variant values = ctx.GetInputValue("Values");

        crude_json::object obj;
        size_t n = std::min(keys.arraySize(), values.arraySize());
        for (size_t i = 0; i < n; ++i) {
            std::string k = keys.arrayGet(i).asString();
            if (!k.empty())
                obj[k] = variantToJson_AI(values.arrayGet(i));
        }

        ctx.SetOutputValue("JSON", Variant(crude_json::value(std::move(obj)).dump()));
        return true;
    };

    // ========================================================================
    // JSON.SetPath
    // 输入：JSON(String), Path(String), Value(Any)
    // 输出：JSON(String)
    //
    // 向 JSON 字符串的指定路径写入值，不存在的中间节点自动创建。
    // 例：JSON="{}", Path="choices[0].message.content", Value="Hello"
    //   → {"choices":[{"message":{"content":"Hello"}}]}
    // ========================================================================
    handlers["JSON.SetPath"] = [](ExecutionContext& ctx) -> bool {
        std::string jsonStr = ctx.GetInputValue("JSON").asString();
        std::string path    = ctx.GetInputValue("Path").asString();
        Variant     val     = ctx.GetInputValue("Value");

        crude_json::value root = jsonStr.empty()
            ? crude_json::value(crude_json::object{})
            : crude_json::value::parse(jsonStr);

        if (root.is_null() && !jsonStr.empty())
            root = crude_json::value(crude_json::object{});

        auto segs = parsePath(path);
        if (!segs.empty())
            setAtPath(root, segs, 0, variantToJson_AI(val));

        ctx.SetOutputValue("JSON", Variant(root.dump()));
        return true;
    };

    // ========================================================================
    // JSON.ArrayPush
    // 输入：JSON(String), Element(Any)
    // 输出：JSON(String), Length(Integer)
    //
    // 向 JSON 数组末尾追加元素。
    // 若输入不是数组字符串，自动初始化为 []。
    // ========================================================================
    handlers["JSON.ArrayPush"] = [](ExecutionContext& ctx) -> bool {
        std::string jsonStr = ctx.GetInputValue("JSON").asString();
        Variant     elem    = ctx.GetInputValue("Element");

        crude_json::value root = jsonStr.empty()
            ? crude_json::value(crude_json::array{})
            : crude_json::value::parse(jsonStr);

        if (!root.is_array())
            root = crude_json::value(crude_json::array{});

        root.get<crude_json::array>().push_back(variantToJson_AI(elem));

        int64_t len = static_cast<int64_t>(root.get<crude_json::array>().size());
        ctx.SetOutputValue("JSON",   Variant(root.dump()));
        ctx.SetOutputValue("Length", Variant(len));
        return true;
    };

    // ========================================================================
    // JSON.MakeMessage
    // 输入：Role(String), Content(String)
    // 输出：JSON(String)
    //
    // 快速构造 {"role":"user","content":"..."} 消息对象。
    // ========================================================================
    handlers["JSON.MakeMessage"] = [](ExecutionContext& ctx) -> bool {
        std::string role       = ctx.GetInputValue("Role").asString();
        std::string content    = ctx.GetInputValue("Content").asString();
        std::string toolCallId = ctx.GetInputValue("ToolCallId").asString();
        if (role.empty()) role = "user";

        crude_json::object obj;
        obj["role"]    = crude_json::value(role);
        obj["content"] = crude_json::value(content);
        // Add tool_call_id when present (required for role="tool" messages)
        if (!toolCallId.empty())
            obj["tool_call_id"] = crude_json::value(toolCallId);
        ctx.SetOutputValue("Message", Variant(crude_json::value(std::move(obj)).dump()));
        return true;
    };

    // ========================================================================
    // String.Template
    // 输入：Template(String), Keys(Array<String>), Values(Array<String>)
    // 输出：Result(String)
    //
    // 将模板字符串中的 {{key}} 占位符替换为对应的值。
    // 例：Template="你好，{{name}}！今天是{{day}}。"
    //     Keys=["name","day"], Values=["Alice","周一"]
    //   → "你好，Alice！今天是周一。"
    // ========================================================================
    handlers["String.Template"] = [](ExecutionContext& ctx) -> bool {
        std::string tmpl   = ctx.GetInputValue("Template").asString();
        Variant     keys   = ctx.GetInputValue("Keys");
        Variant     values = ctx.GetInputValue("Values");

        size_t n = std::min(keys.arraySize(), values.arraySize());
        std::string result = tmpl;
        for (size_t i = 0; i < n; ++i) {
            std::string k = keys.arrayGet(i).asString();
            std::string v = values.arrayGet(i).asString();
            if (k.empty()) continue;
            std::string placeholder = "{{" + k + "}}";
            size_t pos = 0;
            while ((pos = result.find(placeholder, pos)) != std::string::npos) {
                result.replace(pos, placeholder.size(), v);
                pos += v.size();
            }
        }
        ctx.SetOutputValue("Result", Variant(result));
        return true;
    };

    // ========================================================================
    // LLM.Chat
    // 输入：
    //   BaseURL(String)   — API 基础 URL，默认 "https://api.openai.com/v1"
    //   ApiKey(String)    — Bearer token
    //   Model(String)     — 模型名，如 "gpt-4o"
    //   Messages(String)  — JSON 数组字符串 [{"role":"user","content":"..."}]
    //   MaxTokens(Integer)— 最大输出 token 数，默认 1024
    //   Temperature(Float)— 温度，默认 0.7
    //   SystemPrompt(String) — 可选，自动拼到 messages 第一条 system 消息
    // 输出：
    //   → onReply        — 成功时走这条线
    //   → onError        — 失败时走这条线
    //   Reply(String)    — 模型回复文本（choices[0].message.content）
    //   FullResponse(String) — 完整响应 JSON
    //   ErrorMessage(String) — 错误描述
    //
    // 异步：通过 RunAsync(dispatcher, onComplete) 实现跨平台异步。
    // ========================================================================
    handlers["LLM.Chat"] = [&runner](ExecutionContext& ctx) -> bool {

        IHttpClient* client = BP_GetHttpClient();
        if (!client) {
            ctx.LogError("[LLM.Chat] No HttpClient registered. Call BP_SetHttpClient() first.");
            ctx.SetOutputValue("Reply",        Variant(std::string("")));
            ctx.SetOutputValue("FullResponse", Variant(std::string("")));
            ctx.SetOutputValue("ErrorMessage", Variant(std::string("No HttpClient registered")));
            ctx.ActivateOutputFlow("onError");
            return true;
        }

        // ── 读取输入 ─────────────────────────────────────────────────────
        std::string baseURL     = ctx.GetInputValue("BaseURL").asString();
        std::string apiKey      = ctx.GetInputValue("ApiKey").asString();
        std::string model       = ctx.GetInputValue("Model").asString();
        std::string messagesStr = ctx.GetInputValue("Messages").asString();
        std::string systemPrompt= ctx.GetInputValue("SystemPrompt").asString();

        if (baseURL.empty()) baseURL = "https://api.openai.com/v1";
        if (model.empty())   model   = "gpt-4o";

        int64_t maxTokens   = 1024;
        double  temperature = 0.7;
        {
            auto mt = ctx.GetInputValue("MaxTokens");
            if (mt.type == PinDataType::Integer || mt.type == PinDataType::Float)
                maxTokens = mt.asInt();
            auto tp = ctx.GetInputValue("Temperature");
            if (tp.type == PinDataType::Float || tp.type == PinDataType::Integer)
                temperature = tp.asFloat();
        }

        // ── 构建 messages 数组 ──────────────────────────────────────────
        crude_json::array messages;

        // 插入 system prompt（若有）
        if (!systemPrompt.empty()) {
            crude_json::object sys;
            sys["role"]    = crude_json::value(std::string("system"));
            sys["content"] = crude_json::value(systemPrompt);
            messages.push_back(crude_json::value(std::move(sys)));
        }

        // 追加用户传入的 messages
        if (!messagesStr.empty()) {
            crude_json::value parsed = crude_json::value::parse(messagesStr);
            if (parsed.is_array()) {
                for (const auto& m : parsed.get<crude_json::array>())
                    messages.push_back(m);
            } else if (parsed.is_object()) {
                // 单条消息对象也支持
                messages.push_back(parsed);
            } else if (parsed.is_string()) {
                // 纯字符串：包成 user 消息
                crude_json::object um;
                um["role"]    = crude_json::value(std::string("user"));
                um["content"] = crude_json::value(messagesStr);
                messages.push_back(crude_json::value(std::move(um)));
            }
        }

        // ── 构建请求 body ───────────────────────────────────────────────
        crude_json::object body;
        body["model"]       = crude_json::value(model);
        body["messages"]    = crude_json::value(std::move(messages));
        body["max_tokens"]  = crude_json::value(static_cast<double>(maxTokens));
        body["temperature"] = crude_json::value(temperature);

        // ── Tools（function calling schema）──────────────────────────
        std::string toolsStr = ctx.GetInputValue("Tools").asString();
        if (!toolsStr.empty()) {
            crude_json::value toolsJson = crude_json::value::parse(toolsStr);
            if (toolsJson.is_array())
                body["tools"] = toolsJson;
        }

        HttpRequest req;
        req.url    = baseURL + "/chat/completions";
        req.method = "POST";
        req.body   = crude_json::value(std::move(body)).dump();
        req.headers["Content-Type"]  = "application/json";
        if (!apiKey.empty())
            req.headers["Authorization"] = "Bearer " + apiKey;
        req.timeoutSeconds = 120;  // LLM 可能慢

        PinId replyPinId    = ctx.GetPinId("onReply");
        PinId toolPinId     = ctx.GetPinId("onToolCall");
        PinId errorPinId    = ctx.GetPinId("onError");

        ctx.MarkDownstreamAsHandled("onReply");
        ctx.MarkDownstreamAsHandled("onToolCall");
        ctx.MarkDownstreamAsHandled("onError");

        auto sharedResp = std::make_shared<HttpResponse>();

        // dispatcher：发起请求，完成后 resolve
        auto dispatcher = [client, req, sharedResp](ExecutionContext::AsyncResolve resolve) mutable {
            client->SendAsync(req,
                [sharedResp, resolve = std::move(resolve)](HttpResponse resp) mutable {
                    *sharedResp = std::move(resp);
                    resolve();
                });
        };

        // onComplete：主线程 Tick 上下文，解析回复并激活下游
        auto onComplete = [sharedResp, replyPinId, toolPinId, errorPinId](ExecutionContext& c) mutable {
            const HttpResponse& resp = *sharedResp;

            c.SetOutputValue("FullResponse", Variant(resp.body));

            if (!resp.ok()) {
                std::string errMsg = resp.error.empty()
                    ? ("HTTP " + std::to_string(resp.statusCode))
                    : resp.error;
                if (!resp.body.empty()) {
                    crude_json::value j = crude_json::value::parse(resp.body);
                    if (j.is_object() && j.contains("error")) {
                        const auto& e = j["error"];
                        if (e.is_object() && e.contains("message"))
                            errMsg = e["message"].get<std::string>();
                    }
                }
                c.SetOutputValue("Reply",         Variant(std::string("")));
                c.SetOutputValue("ToolCallsJSON", Variant(std::string("[]")));
                c.SetOutputValue("FinishReason",  Variant(std::string("error")));
                c.SetOutputValue("ErrorMessage",  Variant(errMsg));
                c.LogError("[LLM.Chat] " + errMsg);
                c.ActivateOutputFlow(errorPinId);
                return;
            }

            crude_json::value j = crude_json::value::parse(resp.body);

            // finish_reason
            std::string finishReason;
            if (j.is_object() && j.contains("choices")) {
                const auto& choices = j["choices"];
                if (choices.is_array() && !choices.get<crude_json::array>().empty()) {
                    const auto& first = choices.get<crude_json::array>()[0];
                    if (first.is_object() && first.contains("finish_reason")) {
                        const auto& fr = first["finish_reason"];
                        if (fr.is_string()) finishReason = fr.get<std::string>();
                    }
                }
            }
            c.SetOutputValue("FinishReason", Variant(finishReason));

            // ── tool_calls 分支 ──────────────────────────────────────
            if (finishReason == "tool_calls") {
                std::string toolCallsJson = "[]";
                if (j.is_object() && j.contains("choices")) {
                    const auto& choices = j["choices"];
                    if (choices.is_array() && !choices.get<crude_json::array>().empty()) {
                        const auto& first = choices.get<crude_json::array>()[0];
                        if (first.is_object() && first.contains("message")) {
                            const auto& msg = first["message"];
                            if (msg.is_object() && msg.contains("tool_calls")) {
                                toolCallsJson = msg["tool_calls"].dump();
                            }
                        }
                    }
                }
                c.SetOutputValue("Reply",         Variant(std::string("")));
                c.SetOutputValue("ToolCallsJSON", Variant(toolCallsJson));
                c.SetOutputValue("ErrorMessage",  Variant(std::string("")));
                c.Log("[LLM.Chat] tool_calls: " + toolCallsJson.substr(0, 120));
                c.ActivateOutputFlow(toolPinId);
                return;
            }

            // ── 普通文本回复 ─────────────────────────────────────────
            std::string reply;
            if (j.is_object() && j.contains("choices")) {
                const auto& choices = j["choices"];
                if (choices.is_array() && !choices.get<crude_json::array>().empty()) {
                    const auto& first = choices.get<crude_json::array>()[0];
                    if (first.is_object() && first.contains("message")) {
                        const auto& msg = first["message"];
                        if (msg.is_object() && msg.contains("content")) {
                            const auto& content = msg["content"];
                            if (content.is_string())
                                reply = content.get<std::string>();
                        }
                    }
                }
            }
            c.SetOutputValue("Reply",         Variant(reply));
            c.SetOutputValue("ToolCallsJSON", Variant(std::string("[]")));
            c.SetOutputValue("ErrorMessage",  Variant(std::string("")));
            c.Log("[LLM.Chat] reply length=" + std::to_string(reply.size()));
            c.ActivateOutputFlow(replyPinId);
        };

        ctx.RunAsync(std::move(dispatcher), std::move(onComplete));
        return true;
    };

    // ================================================================
    // LLM.StreamChat
    //   流式 OpenAI 兼容 Chat（SSE，批量聚合模式）
    //   in:  BaseURL / ApiKey / Model / Messages / SystemPrompt /
    //        MaxTokens / Temperature / Tools（同 LLM.Chat）
    //   exec out:
    //     onChunk(Token)  — 后台流结束后在主线程逐 token 批量激活
    //     onDone          — 全部 chunk 激活完毕后触发
    //     onError(ErrorMessage)
    //   out: Token(String)    — 当前 chunk 文本（onChunk 时有效）
    //        FullText(String) — 完整拼合文本（onDone 时有效）
    //        ErrorMessage(String)
    //
    // 实现说明：
    //   用 RunAsync dispatcher/onComplete 模型，后台线程通过 StreamAsync
    //   收集所有 SSE chunks，流结束后 resolve()，onComplete（主线程）批量
    //   逐 token 激活 onChunk，最后激活 onDone。完全复用已有 RunAsync 机制。
    // ================================================================
    handlers["LLM.StreamChat"] = [](ExecutionContext& ctx) {
        auto* client = BP_GetHttpClient();
        if (!client) {
            ctx.SetOutputValue("Token",        Variant(std::string("")));
            ctx.SetOutputValue("FullText",     Variant(std::string("")));
            ctx.SetOutputValue("ErrorMessage", Variant(std::string("No HTTP client registered")));
            ctx.ActivateOutputFlow("onError");
            return true;
        }

        // ── 读取参数 ────────────────────────────────────────────────
        std::string baseURL      = ctx.GetInputValue("BaseURL").asString();
        std::string apiKey       = ctx.GetInputValue("ApiKey").asString();
        std::string model        = ctx.GetInputValue("Model").asString();
        std::string messagesStr  = ctx.GetInputValue("Messages").asString();
        std::string systemPrompt = ctx.GetInputValue("SystemPrompt").asString();
        std::string toolsStr     = ctx.GetInputValue("Tools").asString();

        if (baseURL.empty()) baseURL = "https://api.openai.com/v1";
        if (model.empty())   model   = "gpt-4o";

        int64_t maxTokens   = 1024;
        double  temperature = 0.7;
        {
            auto mt = ctx.GetInputValue("MaxTokens");
            if (mt.type == PinDataType::Integer || mt.type == PinDataType::Float)
                maxTokens = mt.asInt();
            auto tp = ctx.GetInputValue("Temperature");
            if (tp.type == PinDataType::Float || tp.type == PinDataType::Integer)
                temperature = tp.asFloat();
        }

        crude_json::array messages;
        if (!systemPrompt.empty()) {
            crude_json::object sys;
            sys["role"]    = crude_json::value(std::string("system"));
            sys["content"] = crude_json::value(systemPrompt);
            messages.push_back(crude_json::value(std::move(sys)));
        }
        if (!messagesStr.empty()) {
            crude_json::value parsed = crude_json::value::parse(messagesStr);
            if (parsed.is_array())
                for (const auto& m : parsed.get<crude_json::array>()) messages.push_back(m);
            else if (parsed.is_object())
                messages.push_back(parsed);
        }

        crude_json::object body;
        body["model"]       = crude_json::value(model);
        body["messages"]    = crude_json::value(std::move(messages));
        body["max_tokens"]  = crude_json::value(static_cast<double>(maxTokens));
        body["temperature"] = crude_json::value(temperature);
        if (!toolsStr.empty()) {
            crude_json::value tv = crude_json::value::parse(toolsStr);
            if (tv.is_array()) body["tools"] = tv;
        }

        HttpRequest req;
        req.url    = baseURL + "/chat/completions";
        req.method = "POST";
        req.body   = crude_json::value(std::move(body)).dump();
        req.headers["Content-Type"] = "application/json";
        if (!apiKey.empty())
            req.headers["Authorization"] = "Bearer " + apiKey;
        req.timeoutSeconds = 120;

        PinId chunkPinId = ctx.GetPinId("onChunk");
        PinId donePinId  = ctx.GetPinId("onDone");
        PinId errorPinId = ctx.GetPinId("onError");

        ctx.MarkDownstreamAsHandled("onChunk");
        ctx.MarkDownstreamAsHandled("onDone");
        ctx.MarkDownstreamAsHandled("onError");

        // 共享状态：后台线程收集 chunks
        struct StreamResult {
            std::vector<std::string> chunks;
            std::string              errorMsg;
        };
        auto shared = std::make_shared<StreamResult>();

        // dispatcher：StreamAsync 收集所有 chunk，结束后 resolve
        auto dispatcher = [client, req, shared](ExecutionContext::AsyncResolve resolve) mutable {
            client->StreamAsync(req,
                // onChunk — 可能在后台线程（Default）或主线程（Emscripten），
                // 此处仅追加到 vector，不直接访问蓝图状态
                [shared](const std::string& token) {
                    shared->chunks.push_back(token);
                },
                // onDone — Default实现在主线程（MainThreadDispatcher），
                // Emscripten 在 fetch 回调中，均可安全 resolve()
                [shared, resolve = std::move(resolve)](const std::string& error) mutable {
                    shared->errorMsg = error;
                    resolve();
                });
        };

        // onComplete：主线程，批量激活
        auto onComplete = [shared, chunkPinId, donePinId, errorPinId](ExecutionContext& c) mutable {
            if (!shared->errorMsg.empty()) {
                c.SetOutputValue("Token",        Variant(std::string("")));
                c.SetOutputValue("FullText",     Variant(std::string("")));
                c.SetOutputValue("ErrorMessage", Variant(shared->errorMsg));
                c.LogError("[LLM.StreamChat] " + shared->errorMsg);
                c.ActivateOutputFlow(errorPinId);
                return;
            }

            std::string fullText;
            for (const auto& t : shared->chunks) fullText += t;

            c.SetOutputValue("FullText",     Variant(fullText));
            c.SetOutputValue("ErrorMessage", Variant(std::string("")));
            c.Log("[LLM.StreamChat] " + std::to_string(shared->chunks.size())
                  + " chunks, total=" + std::to_string(fullText.size()) + " chars");

            // 逐 token 激活 onChunk
            for (const auto& tok : shared->chunks) {
                c.SetOutputValue("Token", Variant(tok));
                c.ActivateOutputFlow(chunkPinId);
            }
            // onDone
            c.SetOutputValue("Token", Variant(std::string("")));
            c.ActivateOutputFlow(donePinId);
        };

        ctx.RunAsync(std::move(dispatcher), std::move(onComplete));
        return true;
    };

    // ================================================================
    // JSON.ParseToolCall
    //   从 LLM 返回的 tool_calls 数组中取出指定位置的调用信息
    //   in:  ToolCallsJSON(String), Index(Integer, default=0)
    //   out: Name(String), ArgumentsJSON(String), ID(String)
    //   纯数据节点（无 exec flow）
    // ================================================================
    handlers["JSON.ParseToolCall"] = [](ExecutionContext& ctx) {
        std::string json = ctx.GetInputValue("ToolCallsJSON").asString();
        int index = (int)ctx.GetInputValue("Index").asInt();

        auto v = crude_json::value::parse(json);
        if (!v.is_array()) {
            ctx.SetOutputValue("Name",          Variant(std::string("")));
            ctx.SetOutputValue("ArgumentsJSON", Variant(std::string("{}")));
            ctx.SetOutputValue("ID",            Variant(std::string("")));
            return true;
        }
        const auto& arr = v.get<crude_json::array>();
        if (index < 0 || index >= (int)arr.size()) {
            ctx.SetOutputValue("Name",          Variant(std::string("")));
            ctx.SetOutputValue("ArgumentsJSON", Variant(std::string("{}")));
            ctx.SetOutputValue("ID",            Variant(std::string("")));
            return true;
        }
        const auto& tc = arr[index];
        std::string name, argsJson, id;
        if (tc.is_object()) {
            if (tc.contains("id") && tc["id"].is_string())
                id = tc["id"].get<std::string>();
            if (tc.contains("function") && tc["function"].is_object()) {
                const auto& fn = tc["function"];
                if (fn.contains("name") && fn["name"].is_string())
                    name = fn["name"].get<std::string>();
                if (fn.contains("arguments") && fn["arguments"].is_string())
                    argsJson = fn["arguments"].get<std::string>();
                else if (fn.contains("arguments") && fn["arguments"].is_object())
                    argsJson = fn["arguments"].dump();
            }
        }
        ctx.SetOutputValue("Name",          Variant(name));
        ctx.SetOutputValue("ArgumentsJSON", Variant(argsJson.empty() ? std::string("{}") : argsJson));
        ctx.SetOutputValue("ID",            Variant(id));
        return true;
    };

    // ================================================================
    // JSON.MakeToolResult
    //   构造一条 tool role 的消息，用于把工具执行结果返回给 LLM
    //   in:  ToolCallID(String), Content(String)
    //   out: JSON(String)  — {"role":"tool","tool_call_id":"...","content":"..."}
    //   纯数据节点（无 exec flow）
    // ================================================================
    handlers["JSON.MakeToolResult"] = [](ExecutionContext& ctx) {
        std::string toolCallId = ctx.GetInputValue("ToolCallID").asString();
        std::string content    = ctx.GetInputValue("Content").asString();

        crude_json::object obj;
        obj["role"]         = crude_json::value(std::string("tool"));
        obj["tool_call_id"] = crude_json::value(toolCallId);
        obj["content"]      = crude_json::value(content);
        ctx.SetOutputValue("JSON", Variant(crude_json::value(std::move(obj)).dump()));
        return true;
    };

    // ================================================================
    //   从文件加载对话历史（JSON 数组字符串）
    //   exec in → onSuccess / onError / onNew（文件不存在时）
    //   in:  Path(String)        — 历史文件路径
    //        MaxMessages(Integer)— 最多保留最近 N 条，0=不限制
    //   out: Messages(String)    — JSON 数组字符串
    //        Count(Integer)      — 实际消息条数
    //        ErrorMessage(String)
    //
    // WebGL：直接走 onNew，返回空数组（无磁盘可用）
    // ================================================================
    handlers["Memory.LoadHistory"] = [](ExecutionContext& ctx) {
        std::string path = ctx.GetInputValue("Path").asString();
        int maxMsg = (int)ctx.GetInputValue("MaxMessages").asInt();

#ifdef __EMSCRIPTEN__
        ctx.SetOutputValue("Messages",     Variant(std::string("[]")));
        ctx.SetOutputValue("Count",        Variant((int64_t)0));
        ctx.SetOutputValue("ErrorMessage", Variant(std::string("")));
        ctx.ActivateOutputFlow("onNew");
        return true;
#else
        if (path.empty()) {
            ctx.SetOutputValue("Messages",     Variant(std::string("[]")));
            ctx.SetOutputValue("Count",        Variant((int64_t)0));
            ctx.SetOutputValue("ErrorMessage", Variant(std::string("Path is empty")));
            ctx.ActivateOutputFlow("onError");
            return true;
        }

        // 文件不存在 → onNew（正常首次启动）
        {
            std::error_code ec;
            if (!std::filesystem::exists(std::filesystem::path(path), ec)) {
                ctx.SetOutputValue("Messages",     Variant(std::string("[]")));
                ctx.SetOutputValue("Count",        Variant((int64_t)0));
                ctx.SetOutputValue("ErrorMessage", Variant(std::string("")));
                ctx.ActivateOutputFlow("onNew");
                return true;
            }
        }

        // 读取文件
        std::ifstream f(path, std::ios::binary);
        if (!f.is_open()) {
            ctx.SetOutputValue("Messages",     Variant(std::string("[]")));
            ctx.SetOutputValue("Count",        Variant((int64_t)0));
            ctx.SetOutputValue("ErrorMessage", Variant(std::string(
                "Cannot open history file: " + path)));
            ctx.ActivateOutputFlow("onError");
            return true;
        }
        std::ostringstream ss; ss << f.rdbuf();
        std::string raw = ss.str();

        // 解析 JSON 数组
        auto v = crude_json::value::parse(raw);
        if (!v.is_array()) {
            // 文件损坏，当新建处理
            ctx.SetOutputValue("Messages",     Variant(std::string("[]")));
            ctx.SetOutputValue("Count",        Variant((int64_t)0));
            ctx.SetOutputValue("ErrorMessage", Variant(std::string(
                "History file is not a JSON array, starting fresh")));
            ctx.ActivateOutputFlow("onNew");
            return true;
        }

        auto& arr = v.get<crude_json::array>();

        // MaxMessages 截断（保留最近 N 条）
        if (maxMsg > 0 && (int)arr.size() > maxMsg) {
            crude_json::array trimmed(arr.end() - maxMsg, arr.end());
            arr = std::move(trimmed);
        }

        std::string result = v.dump();
        int64_t count = (int64_t)arr.size();
        ctx.SetOutputValue("Messages",     Variant(result));
        ctx.SetOutputValue("Count",        Variant(count));
        ctx.SetOutputValue("ErrorMessage", Variant(std::string("")));
        ctx.ActivateOutputFlow("onSuccess");
        return true;
#endif
    };

    // ================================================================
    // Memory.SaveHistory
    //   把 messages JSON 数组写回文件
    //   exec in → onSuccess / onError
    //   in:  Path(String)           — 文件路径
    //        Messages(String)       — JSON 数组字符串
    //        MaxMessages(Integer)   — 写入前截断，0=不限
    //   out: ErrorMessage(String)
    //
    // WebGL：直接走 onSuccess（no-op）
    // ================================================================
    handlers["Memory.SaveHistory"] = [](ExecutionContext& ctx) {
        std::string path     = ctx.GetInputValue("Path").asString();
        std::string messages = ctx.GetInputValue("Messages").asString();
        int maxMsg = (int)ctx.GetInputValue("MaxMessages").asInt();

#ifdef __EMSCRIPTEN__
        ctx.SetOutputValue("ErrorMessage", Variant(std::string("")));
        ctx.ActivateOutputFlow("onSuccess");
        return true;
#else
        if (path.empty()) {
            ctx.SetOutputValue("ErrorMessage", Variant(std::string("Path is empty")));
            ctx.ActivateOutputFlow("onError");
            return true;
        }

        // 可选截断
        std::string toWrite = messages;
        if (maxMsg > 0) {
            auto v = crude_json::value::parse(messages);
            if (v.is_array()) {
                auto& arr = v.get<crude_json::array>();
                if ((int)arr.size() > maxMsg) {
                    crude_json::array trimmed(arr.end() - maxMsg, arr.end());
                    arr = std::move(trimmed);
                }
                toWrite = v.dump();
            }
        }

        // 自动建父目录
        {
            std::error_code ec2;
            std::filesystem::path p(path);
            if (p.has_parent_path())
                std::filesystem::create_directories(p.parent_path(), ec2);
        }

        std::ofstream f(path, std::ios::out | std::ios::trunc | std::ios::binary);
        if (!f.is_open()) {
            ctx.SetOutputValue("ErrorMessage", Variant(std::string(
                "Cannot write history file: " + path)));
            ctx.ActivateOutputFlow("onError");
            return true;
        }
        f << toWrite;
        ctx.SetOutputValue("ErrorMessage", Variant(std::string("")));
        ctx.ActivateOutputFlow("onSuccess");
        return true;
#endif
    };

    // ========================================================================
    // Tool.ForEach — 遍历 tool_calls JSON 数组，逐个激活 onTool
    // 输入：  ToolCallsJSON (string)  — LLM.Chat 输出的 tool_calls JSON 数组
    // 输出：  onTool   exec           — 每个 tool call 触发一次
    //         onDone   exec           — 全部遍历完毕
    //         ToolName  string        — 当前工具名
    //         Arguments string        — 当前工具参数 JSON
    //         ToolCallId string       — 当前 tool_call id
    //         Index     integer       — 0-based 索引
    // ========================================================================
    handlers["Tool.ForEach"] = [](ExecutionContext& ctx) {
        auto tcJson = ctx.GetInputValue("ToolCallsJSON").asString();
        if (tcJson.empty()) {
            ctx.ActivateOutputFlow("onDone");
            return true;
        }

        crude_json::value root = crude_json::value::parse(tcJson);
        if (!root.is_array()) {
            // 可能是完整 response，尝试提取 choices[0].message.tool_calls
            if (root.is_object() && root.contains("choices")) {
                const auto& choices = root["choices"];
                if (choices.is_array() && !choices.get<crude_json::array>().empty()) {
                    const auto& msg = choices.get<crude_json::array>()[0];
                    if (msg.is_object() && msg.contains("message")) {
                        const auto& m = msg["message"];
                        if (m.is_object() && m.contains("tool_calls"))
                            root = m["tool_calls"];
                    }
                }
            }
        }

        if (!root.is_array()) {
            ctx.ActivateOutputFlow("onDone");
            return true;
        }

        const auto& arr = root.get<crude_json::array>();
        for (int i = 0; i < (int)arr.size(); ++i) {
            const auto& tc = arr[i];
            std::string name, args, id;
            if (tc.is_object()) {
                if (tc.contains("id") && tc["id"].is_string())
                    id = tc["id"].get<std::string>();
                if (tc.contains("function") && tc["function"].is_object()) {
                    const auto& fn = tc["function"];
                    if (fn.contains("name") && fn["name"].is_string())
                        name = fn["name"].get<std::string>();
                    if (fn.contains("arguments") && fn["arguments"].is_string())
                        args = fn["arguments"].get<std::string>();
                    else if (fn.contains("arguments"))
                        args = fn["arguments"].dump();
                }
            }
            ctx.SetOutputValue("ToolName",   Variant(name));
            ctx.SetOutputValue("Arguments",  Variant(args));
            ctx.SetOutputValue("ToolCallId", Variant(id));
            ctx.SetOutputValue("Index",      Variant((int64_t)i));
            ctx.ActivateOutputFlow("onTool");
        }
        ctx.ActivateOutputFlow("onDone");
        return true;
    };

    // ========================================================================
    // JSON.ToolCallCount — 返回 tool_calls 数组长度
    // 输入：  ToolCallsJSON(String)
    // 输出：  Count(Integer)
    // 纯数据节点（无 exec flow）
    // ========================================================================
    handlers["JSON.ToolCallCount"] = [](ExecutionContext& ctx) {
        auto json = ctx.GetInputValue("ToolCallsJSON").asString();
        int64_t count = 0;
        if (!json.empty()) {
            auto v = crude_json::value::parse(json);
            if (v.is_array())
                count = static_cast<int64_t>(v.get<crude_json::array>().size());
            // 兼容完整 response 格式：尝试提取 choices[0].message.tool_calls
            else if (v.is_object() && v.contains("choices")) {
                const auto& choices = v["choices"];
                if (choices.is_array() && !choices.get<crude_json::array>().empty()) {
                    const auto& msg = choices.get<crude_json::array>()[0];
                    if (msg.is_object() && msg.contains("message")) {
                        const auto& m = msg["message"];
                        if (m.is_object() && m.contains("tool_calls")) {
                            const auto& tc = m["tool_calls"];
                            if (tc.is_array())
                                count = static_cast<int64_t>(tc.get<crude_json::array>().size());
                        }
                    }
                }
            }
        }
        ctx.SetOutputValue("Count", Variant(count));
        return true;
    };

    // ========================================================================
    // Tool.CallByName — 按工具名动态路由到同名 FuncLib 函数
    //
    // 功能：根据 ToolName 在当前蓝图的外部函数库中查找同名函数并调用，
    //       无需 Tool.Match 枚举，实现无上限的动态工具路由。
    //
    // 输入：  exec in
    //         ToolName(String)    — 工具名，与 FuncLib 中的函数名一致
    //         Arguments(String)   — 工具参数 JSON 字符串（传给函数的变量 args）
    //         ToolCallId(String)  — tool_call id（用于组装 MakeToolResult）
    // 输出：  onSuccess exec      — 调用成功
    //         onNotFound exec     — 找不到同名函数
    //         Result(String)      — 函数返回的 "Result" 变量值
    //         ToolCallId(String)  — 透传输入的 ToolCallId（便于直接接 MakeToolResult）
    //
    // 约定：被调用的 FuncLib 函数需有一个名为 "Result" 的输出变量；
    //       Arguments JSON 对象的 key 会作为同名变量注入函数。
    // ========================================================================
    handlers["Tool.CallByName"] = [&runner](ExecutionContext& ctx) {
        std::string toolName   = ctx.GetInputValue("ToolName").asString();
        std::string argsJson   = ctx.GetInputValue("Arguments").asString();
        std::string toolCallId = ctx.GetInputValue("ToolCallId").asString();

        ctx.SetOutputValue("ToolCallId", Variant(toolCallId));

        if (toolName.empty()) {
            ctx.SetOutputValue("Result", Variant(std::string("")));
            ctx.ActivateOutputFlow("onNotFound");
            return true;
        }

        // 在已注册的外部函数中查找同名函数
        // GetExternalFunctions() 返回 vector<FunctionDefinition>
        auto extFuncs = runner.GetExternalFunctions();
        const ::NodeEditor::Runtime::FunctionDefinition* funcDefPtr = nullptr;
        for (const auto& fd : extFuncs) {
            if (fd.name == toolName || fd.id == toolName) {
                funcDefPtr = &fd;
                break;
            }
        }

        if (!funcDefPtr) {
            ctx.Log("[Tool.CallByName] function '" + toolName + "' not found in libraries",
                    ::NodeEditor::Runtime::LogLevel::Warning);
            ctx.SetOutputValue("Result", Variant(std::string("")));
            ctx.ActivateOutputFlow("onNotFound");
            return true;
        }

        // 构建函数子图
        const auto& extLibs = runner.GetExternalLibraries();
        auto libIt = extLibs.find(funcDefPtr->id);
        if (libIt == extLibs.end() || !libIt->second) {
            ctx.Log("[Tool.CallByName] library data not found for '" + toolName + "'",
                    ::NodeEditor::Runtime::LogLevel::Warning);
            ctx.SetOutputValue("Result", Variant(std::string("")));
            ctx.ActivateOutputFlow("onNotFound");
            return true;
        }

        // 解析 Arguments JSON（稍后作为 FuncLib 节点输入引脚默认值注入）
        std::vector<std::pair<std::string, Variant>> argVars;
        if (!argsJson.empty()) {
            auto argsVal = crude_json::value::parse(argsJson);
            if (argsVal.is_object()) {
                for (const auto& kv : argsVal.get<crude_json::object>()) {
                    const auto& v = kv.second;
                    Variant var;
                    if (v.is_string())
                        var = Variant(v.get<std::string>());
                    else if (v.is_number()) {
                        double d = v.get<double>();
                        var = (d == static_cast<double>(static_cast<int64_t>(d)))
                            ? Variant(static_cast<int64_t>(d)) : Variant(d);
                    } else if (v.is_boolean())
                        var = Variant(v.get<bool>());
                    else
                        var = Variant(v.dump());
                    argVars.emplace_back(kv.first, std::move(var));
                }
            }
        }

        // 构造一个微型 Actor 蓝图，用 FuncLib.<funcId> 节点调用目标函数。
        // FuncLib.* handler 内部创建子 runner 时，会遍历节点的数据输入引脚，
        // 将引脚值（或默认值）通过 SetVariable 注入函数子 runner。
        // 因此，Arguments 中的参数需要以数据输入引脚的形式挂在 FuncLib 节点上。

        BlueprintData miniActor;
        miniActor.metadata.blueprintClass = BlueprintClass::Actor;
        miniActor.metadata.name = "ToolCallByName_" + toolName;

        // OnBeginPlay 节点（id=1，exec 输出 pin=10）
        {
            NodeInstance n;
            n.id = 1; n.definitionId = "OnBeginPlay"; n.name = "OnBeginPlay";
            PinInfo ep; ep.id=10; ep.kind=PinKind::Output; ep.isExec=true; ep.name="";
            n.pins.push_back(ep);
            miniActor.nodes.push_back(n);
        }

        // FuncLib.<funcId> 节点（id=2）
        // 为每个 Argument 添加数据输入引脚（带默认值），FuncLib handler 会将其传入函数
        std::string funcLibDefId = "FuncLib." + libIt->second->metadata.name + "." + funcDefPtr->id;
        {
            NodeInstance n;
            n.id = 2; n.definitionId = funcLibDefId; n.name = toolName;
            // exec in/out
            PinInfo ei; ei.id=20; ei.kind=PinKind::Input; ei.isExec=true; ei.name="";
            n.pins.push_back(ei);
            PinInfo eo; eo.id=29; eo.kind=PinKind::Output; eo.isExec=true; eo.name="";
            n.pins.push_back(eo);
            // 每个参数作为数据输入引脚（pin id 从 200 开始）
            PinId argPinId = 200;
            for (const auto& av : argVars) {
                PinInfo ap;
                ap.id = argPinId++;
                ap.kind = PinKind::Input;
                ap.isExec = false;
                ap.dataType = PinDataType::String;  // 统一 String，FuncLib handler 用名称匹配
                ap.name = av.first;
                ap.defaultValue = av.second;
                n.pins.push_back(ap);
            }
            // Result 输出引脚
            PinInfo rp; rp.id=22; rp.kind=PinKind::Output; rp.isExec=false;
            rp.dataType=PinDataType::String; rp.name="Result";
            n.pins.push_back(rp);
            miniActor.nodes.push_back(n);
        }

        // 连线 OnBeginPlay.exec → FuncLib.exec
        {
            LinkInstance lnk; lnk.id=100;
            lnk.startPinId=10; lnk.endPinId=20;
            miniActor.links.push_back(lnk);
        }

        // 构造子 runner，继承父 runner 的全部配置
        ::NodeEditor::Runtime::BlueprintRunner subRunner;
        subRunner.RegisterHandlers(runner.GetHandlers());
        if (ctx.OnLog)   subRunner.SetLogCallback(ctx.OnLog);
        if (ctx.OnPrint) subRunner.SetPrintCallback(ctx.OnPrint);
        subRunner.RegisterExternalFunctions(runner.GetExternalFunctions());
        subRunner.InheritExternalLibraries(runner.GetExternalLibraries());

        // 加载并执行
        if (subRunner.Load(miniActor)) {
            subRunner.Execute();
            subRunner.DispatchEvent("OnBeginPlay");
            auto result = subRunner.GetVariable("Result");
            if (result.type == ::NodeEditor::Runtime::PinDataType::Unknown)
                result = subRunner.GetPinValue(22);  // fallback：从输出引脚读
            ctx.SetOutputValue("Result",
                result.type != ::NodeEditor::Runtime::PinDataType::Unknown
                    ? result : Variant(std::string("")));
        } else {
            ctx.SetOutputValue("Result", Variant(std::string("")));
        }

        ctx.ActivateOutputFlow("onSuccess");
        return true;
    };

    // ========================================================================
    // Tool.Match — 按工具名路由到对应 exec 分支（最多 8 个 Case）
    // 输入：  ToolName string
    //         Case0..Case7 string（空 = 不使用）
    // 输出：  Match0..Match7 exec — 对应 Case 匹配时激活
    //         Default exec        — 无匹配时激活
    //         MatchedIndex integer — 匹配到的索引，-1=无匹配
    // ========================================================================
    handlers["Tool.Match"] = [](ExecutionContext& ctx) {
        auto toolName = ctx.GetInputValue("ToolName").asString();
        int matched = -1;
        for (int i = 0; i < 8; ++i) {
            std::string caseKey = "Case" + std::to_string(i);
            auto caseVal = ctx.GetInputValue(caseKey).asString();
            if (!caseVal.empty() && caseVal == toolName) {
                matched = i;
                break;
            }
        }
        ctx.SetOutputValue("MatchedIndex", Variant((int64_t)matched));
        if (matched >= 0) {
            ctx.ActivateOutputFlow("Match" + std::to_string(matched));
        } else {
            ctx.ActivateOutputFlow("Default");
        }
        return true;
    };

    // ========================================================================
    // JSON.Extract — 从文本中提取 ```json ... ``` 代码块内容
    // 输入：  Text string
    // 输出：  JSON string   — 提取到的 JSON（若无代码块则尝试直接 parse）
    //         Found boolean — 是否找到代码块
    // ========================================================================
    handlers["JSON.Extract"] = [](ExecutionContext& ctx) {
        auto text = ctx.GetInputValue("Text").asString();

        // 尝试提取 ```json ... ``` 或 ``` ... ```
        auto tryExtract = [&](const std::string& fence) -> std::string {
            auto pos = text.find(fence);
            if (pos == std::string::npos) return "";
            pos += fence.size();
            // skip newline
            if (pos < text.size() && (text[pos] == '\n' || text[pos] == '\r')) pos++;
            auto end = text.find("```", pos);
            if (end == std::string::npos) return "";
            // trim trailing whitespace
            while (end > pos && (text[end-1] == '\n' || text[end-1] == '\r' || text[end-1] == ' '))
                --end;
            return text.substr(pos, end - pos);
        };

        std::string extracted = tryExtract("```json");
        if (extracted.empty()) extracted = tryExtract("```JSON");
        if (extracted.empty()) extracted = tryExtract("```");

        if (!extracted.empty()) {
            // 验证是合法 JSON
            auto v = crude_json::value::parse(extracted);
            if (!v.is_null() || extracted.find("null") != std::string::npos) {
                ctx.SetOutputValue("JSON",  Variant(extracted));
                ctx.SetOutputValue("Found", Variant(true));
                return true;
            }
        }

        // 没有代码块，尝试直接把整段文本当 JSON parse
        auto trimmed = text;
        while (!trimmed.empty() && (trimmed.front() == ' ' || trimmed.front() == '\n' || trimmed.front() == '\r'))
            trimmed.erase(trimmed.begin());
        while (!trimmed.empty() && (trimmed.back() == ' ' || trimmed.back() == '\n' || trimmed.back() == '\r'))
            trimmed.pop_back();

        auto v2 = crude_json::value::parse(trimmed);
        bool looksJson = !trimmed.empty() && (trimmed.front() == '{' || trimmed.front() == '[');
        if (looksJson && !v2.is_null()) {
            ctx.SetOutputValue("JSON",  Variant(trimmed));
            ctx.SetOutputValue("Found", Variant(true));
        } else {
            ctx.SetOutputValue("JSON",  Variant(std::string("")));
            ctx.SetOutputValue("Found", Variant(false));
        }
        return true;
    };

    // ========================================================================
    // JSON.Validate — 验证 JSON 字符串是否合法，可选按 key 列表校验必填字段
    // 输入：  JSON string
    //         RequiredKeys string  — 逗号分隔，如 "name,age"（空=只验证格式）
    // 输出：  onValid  exec
    //         onInvalid exec
    //         IsValid  boolean
    //         ErrorMessage string
    // ========================================================================
    handlers["JSON.Validate"] = [](ExecutionContext& ctx) {
        auto jsonStr     = ctx.GetInputValue("JSON").asString();
        auto requiredRaw = ctx.GetInputValue("RequiredKeys").asString();

        if (jsonStr.empty()) {
            ctx.SetOutputValue("IsValid",      Variant(false));
            ctx.SetOutputValue("ErrorMessage", Variant(std::string("Empty JSON string")));
            ctx.ActivateOutputFlow("onInvalid");
            return true;
        }

        auto v = crude_json::value::parse(jsonStr);
        bool isNull = v.is_null();
        // crude_json returns null for parse errors, but "null" is valid JSON
        bool looksJson = jsonStr.find_first_not_of(" \t\r\n") != std::string::npos &&
                         (jsonStr[jsonStr.find_first_not_of(" \t\r\n")] == '{' ||
                          jsonStr[jsonStr.find_first_not_of(" \t\r\n")] == '[' ||
                          jsonStr.find("null") != std::string::npos ||
                          jsonStr.find("true") != std::string::npos ||
                          jsonStr.find("false") != std::string::npos);

        if (isNull && jsonStr.find("null") == std::string::npos) {
            ctx.SetOutputValue("IsValid",      Variant(false));
            ctx.SetOutputValue("ErrorMessage", Variant(std::string("JSON parse error")));
            ctx.ActivateOutputFlow("onInvalid");
            return true;
        }

        // 检查必填 keys（仅对 object 有意义）
        if (!requiredRaw.empty() && v.is_object()) {
            std::vector<std::string> missing;
            size_t pos = 0;
            while (pos < requiredRaw.size()) {
                auto comma = requiredRaw.find(',', pos);
                auto key = requiredRaw.substr(pos, comma == std::string::npos ? std::string::npos : comma - pos);
                // trim
                while (!key.empty() && key.front() == ' ') key.erase(key.begin());
                while (!key.empty() && key.back()  == ' ') key.pop_back();
                if (!key.empty() && !v.contains(key))
                    missing.push_back(key);
                if (comma == std::string::npos) break;
                pos = comma + 1;
            }
            if (!missing.empty()) {
                std::string errMsg = "Missing required keys: ";
                for (size_t i = 0; i < missing.size(); ++i) {
                    if (i) errMsg += ", ";
                    errMsg += missing[i];
                }
                ctx.SetOutputValue("IsValid",      Variant(false));
                ctx.SetOutputValue("ErrorMessage", Variant(errMsg));
                ctx.ActivateOutputFlow("onInvalid");
                return true;
            }
        }

        ctx.SetOutputValue("IsValid",      Variant(true));
        ctx.SetOutputValue("ErrorMessage", Variant(std::string("")));
        ctx.ActivateOutputFlow("onValid");
        return true;
    };

}

} // namespace Runtime
} // namespace NodeEditor
