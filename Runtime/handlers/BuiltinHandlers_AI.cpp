// Runtime/handlers/BuiltinHandlers_AI.cpp
// AI 相关节点处理器 — 共享辅助函数实现 + 入口注册函数
// Handler 实现分布在：
//   BuiltinHandlers_AI_JSON.cpp  — JSON.* / String.Template
//   BuiltinHandlers_AI_LLM.cpp   — LLM.Chat / Auto / AutoStream / StreamChat
//   BuiltinHandlers_AI_Tool.cpp  — Tool.* / MCP.Call / Memory.* / UserInput.Wait
//   BuiltinHandlers_AI_Agent.cpp — Agent.* / Context.Compress / LLM.StructuredOutput / Schema.Validate / LLM.Route / Intent.Classify

#include "BuiltinHandlers_AI.h"
#include "BuiltinHandlers_AI_Internal.h"

namespace NodeEditor {
namespace Runtime {

// ============================================================================
// 内部工具：Variant ↔ crude_json（与 BuiltinHandlers_Data 保持一致）
// ============================================================================

crude_json::value variantToJson_AI(const Variant& v)
{
    switch (v.type)
    {
    case PinDataType::Boolean: return crude_json::value(v.asBool());
    case PinDataType::Integer: return crude_json::value(static_cast<double>(v.asInt()));
    case PinDataType::Float:   return crude_json::value(v.asFloat());
    case PinDataType::String:
    {
        const std::string& s = v.asString();
        // 如果字符串看起来是 JSON object 或 array，尝试解析；
        // 这样 JSON.MakeMessage 输出的消息对象能正确被 JSON.ArrayPush 当对象处理
        if (!s.empty() && (s.front() == '{' || s.front() == '['))
        {
            crude_json::value parsed = crude_json::value::parse(s);
            if (!parsed.is_null())
                return parsed;
        }
        return crude_json::value(s);
    }
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

Variant jsonToVariant_AI(const crude_json::value& j)
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
std::vector<std::string> parsePath(const std::string& path)
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
void setAtPath(crude_json::value& node,
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
// GetFirstChoice — 安全提取 choices[0]（返回指针，不存在返回 nullptr）
// ============================================================================
const crude_json::value* GetFirstChoice(const crude_json::value& resp)
{
    if (!resp.is_object() || !resp.contains("choices")) return nullptr;
    const auto& choices = resp["choices"];
    if (!choices.is_array() || choices.get<crude_json::array>().empty()) return nullptr;
    const auto& first = choices.get<crude_json::array>()[0];
    return first.is_object() ? &first : nullptr;
}

// ============================================================================
// ExtractToolCall — 从单个 tool_call 对象中提取 id/name/arguments
// ============================================================================

ToolCallInfo ExtractToolCall(const crude_json::value& tc)
{
    ToolCallInfo info;
    if (!tc.is_object()) return info;
    if (tc.contains("id") && tc["id"].is_string())
        info.id = tc["id"].get<std::string>();
    if (tc.contains("function") && tc["function"].is_object()) {
        const auto& fn = tc["function"];
        if (fn.contains("name") && fn["name"].is_string())
            info.name = fn["name"].get<std::string>();
        if (fn.contains("arguments") && fn["arguments"].is_string())
            info.arguments = fn["arguments"].get<std::string>();
        else if (fn.contains("arguments"))
            info.arguments = fn["arguments"].dump();
    }
    return info;
}

// ============================================================================
// BuildLLMRequest — LLM.Chat 和 LLM.StreamChat 共用的请求构建逻辑
// 从 ExecutionContext 读取所有 LLM 参数，构建并返回 HttpRequest。
// ============================================================================
// 核心重载：显式传入 baseURL/apiKey/model，其余参数从 ctx 读取
HttpRequest BuildLLMRequest(ExecutionContext& ctx,
    const std::string& baseURL,
    const std::string& apiKey,
    const std::string& model)
{
    std::string messagesStr  = ctx.GetInputValue("Messages").asString();
    std::string systemPrompt = ctx.GetInputValue("SystemPrompt").asString();
    std::string toolsStr     = ctx.GetInputValue("Tools").asString();

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

    // 构建 messages 数组
    crude_json::array messages;
    if (!systemPrompt.empty()) {
        crude_json::object sys;
        sys["role"]    = crude_json::value(std::string("system"));
        sys["content"] = crude_json::value(systemPrompt);
        messages.push_back(crude_json::value(std::move(sys)));
    }
    if (!messagesStr.empty()) {
        crude_json::value parsed = crude_json::value::parse(messagesStr);
        if (parsed.is_array()) {
            for (const auto& m : parsed.get<crude_json::array>())
                messages.push_back(m);
        } else if (parsed.is_object()) {
            messages.push_back(parsed);
        } else if (parsed.is_string()) {
            crude_json::object um;
            um["role"]    = crude_json::value(std::string("user"));
            um["content"] = crude_json::value(messagesStr);
            messages.push_back(crude_json::value(std::move(um)));
        }
    }

    // 构建 body
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
    req.headers["User-Agent"]   = "Mozilla/5.0 BlueprintRuntime/1.0";
    if (!apiKey.empty())
        req.headers["Authorization"] = "Bearer " + apiKey;
    req.timeoutSeconds = 120;
    return req;
}

// 原接口保持不变：从 ctx 引脚读取 baseURL/apiKey/model
HttpRequest BuildLLMRequest(ExecutionContext& ctx)
{
    std::string baseURL = ctx.GetInputValue("BaseURL").asString();
    std::string apiKey  = ctx.GetInputValue("ApiKey").asString();
    std::string model   = ctx.GetInputValue("Model").asString();
    if (baseURL.empty()) baseURL = "https://api.openai.com/v1";
    if (model.empty())   model   = "gpt-4o";
    return BuildLLMRequest(ctx, baseURL, apiKey, model);
}

// ============================================================================
// RegisterHandlers_AI — 入口：分发到 4 个子注册函数
// ============================================================================
void RegisterHandlers_AI(
    std::unordered_map<std::string, NodeHandler>& handlers,
    BlueprintRunner& runner)
{
    RegisterHandlers_AI_JSON(handlers, runner);
    RegisterHandlers_AI_LLM(handlers, runner);
    RegisterHandlers_AI_Tool(handlers, runner);
    RegisterHandlers_AI_Agent(handlers, runner);
}

} // namespace Runtime
} // namespace NodeEditor
