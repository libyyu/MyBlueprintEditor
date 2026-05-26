// Auto-generated from BuiltinHandlers_AI.cpp split
#include "BuiltinHandlers_AI_Internal.h"

namespace NodeEditor {
namespace Runtime {
void RegisterHandlers_AI_JSON(
    std::unordered_map<std::string, NodeHandler>& handlers)
{
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
        Variant     keysV  = ctx.GetInputValue("Keys");
        Variant     valsV  = ctx.GetInputValue("Values");

        // 把 Keys/Values 统一拉成 vector<string>（支持 Array variant 或 JSON 字符串）
        auto toStringVec = [](const Variant& v) -> std::vector<std::string> {
            std::vector<std::string> out;
            if (v.type == PinDataType::Array) {
                for (size_t i = 0; i < v.arraySize(); ++i)
                    out.push_back(v.arrayGet(i).asString());
            } else if (v.type == PinDataType::String) {
                const std::string& s = v.asString();
                if (!s.empty() && s.front() == '[') {
                    crude_json::value parsed = crude_json::value::parse(s);
                    if (parsed.is_array()) {
                        for (const auto& elem : parsed.get<crude_json::array>())
                            out.push_back(elem.is_string() ? elem.get<std::string>() : elem.dump());
                    }
                } else if (!s.empty()) {
                    out.push_back(s);  // 单值视为长度1的数组
                }
            }
            return out;
        };

        auto keys   = toStringVec(keysV);
        auto values = toStringVec(valsV);

        size_t n = std::min(keys.size(), values.size());
        std::string result = tmpl;
        for (size_t i = 0; i < n; ++i) {
            if (keys[i].empty()) continue;
            std::string placeholder = "{{" + keys[i] + "}}";
            size_t pos = 0;
            while ((pos = result.find(placeholder, pos)) != std::string::npos) {
                result.replace(pos, placeholder.size(), values[i]);
                pos += values[i].size();
            }
        }
        ctx.SetOutputValue("Result", Variant(result));
        return true;
    };
}
}
} // namespace Runtime
} // namespace NodeEditor
