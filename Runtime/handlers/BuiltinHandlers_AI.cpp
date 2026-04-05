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
        std::string role    = ctx.GetInputValue("Role").asString();
        std::string content = ctx.GetInputValue("Content").asString();
        if (role.empty()) role = "user";

        crude_json::object obj;
        obj["role"]    = crude_json::value(role);
        obj["content"] = crude_json::value(content);
        ctx.SetOutputValue("JSON", Variant(crude_json::value(std::move(obj)).dump()));
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

        HttpRequest req;
        req.url    = baseURL + "/chat/completions";
        req.method = "POST";
        req.body   = crude_json::value(std::move(body)).dump();
        req.headers["Content-Type"]  = "application/json";
        if (!apiKey.empty())
            req.headers["Authorization"] = "Bearer " + apiKey;
        req.timeoutSeconds = 120;  // LLM 可能慢

        PinId replyPinId = ctx.GetPinId("onReply");
        PinId errorPinId = ctx.GetPinId("onError");

        ctx.MarkDownstreamAsHandled("onReply");
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
        auto onComplete = [sharedResp, replyPinId, errorPinId](ExecutionContext& c) mutable {
            const HttpResponse& resp = *sharedResp;

            c.SetOutputValue("FullResponse", Variant(resp.body));

            if (!resp.ok()) {
                std::string errMsg = resp.error.empty()
                    ? ("HTTP " + std::to_string(resp.statusCode))
                    : resp.error;
                // 尝试从响应体中提取 error.message
                if (!resp.body.empty()) {
                    crude_json::value j = crude_json::value::parse(resp.body);
                    if (j.is_object() && j.contains("error")) {
                        const auto& e = j["error"];
                        if (e.is_object() && e.contains("message"))
                            errMsg = e["message"].get<std::string>();
                    }
                }
                c.SetOutputValue("Reply",        Variant(std::string("")));
                c.SetOutputValue("ErrorMessage", Variant(errMsg));
                c.LogError("[LLM.Chat] " + errMsg);
                c.ActivateOutputFlow(errorPinId);
                return;
            }

            // 解析 choices[0].message.content
            std::string reply;
            crude_json::value j = crude_json::value::parse(resp.body);
            if (j.is_object() && j.contains("choices")) {
                const auto& choices = j["choices"];
                if (choices.is_array() && !choices.get<crude_json::array>().empty()) {
                    const auto& first = choices.get<crude_json::array>()[0];
                    if (first.is_object() && first.contains("message")) {
                        const auto& msg = first["message"];
                        if (msg.is_object() && msg.contains("content"))
                            reply = msg["content"].get<std::string>();
                    }
                }
            }

            c.SetOutputValue("Reply",        Variant(reply));
            c.SetOutputValue("ErrorMessage", Variant(std::string("")));
            c.Log("[LLM.Chat] reply length=" + std::to_string(reply.size()));
            c.ActivateOutputFlow(replyPinId);
        };

        ctx.RunAsync(std::move(dispatcher), std::move(onComplete));
        return true;
    };

    // ================================================================
    // Memory.LoadHistory
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
}

} // namespace Runtime
} // namespace NodeEditor
