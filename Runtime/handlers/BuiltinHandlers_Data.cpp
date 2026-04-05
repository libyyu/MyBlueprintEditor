// Runtime/BuiltinHandlers_Data.cpp -- Data 节点处理器
#include "BuiltinHandlers_Data.h"
#include "../../Utils/Json/crude_json.h"
#include <sstream>
#include <algorithm>
#include <cctype>

namespace NodeEditor {
namespace Runtime {

// ============================================================================
// Variant ↔ crude_json::value 双向转换
// ============================================================================

static crude_json::value variantToJson(const Variant& v)
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
            arr.push_back(variantToJson(v.arrayGet(i)));
        return crude_json::value(std::move(arr));
    }
    case PinDataType::Map:
    {
        crude_json::object obj;
        for (const auto& kv : v.asMap())
            obj[kv.first.asString()] = variantToJson(kv.second);
        return crude_json::value(std::move(obj));
    }
    default:
        return crude_json::value(); // null
    }
}

static Variant jsonToVariant(const crude_json::value& j)
{
    switch (j.type())
    {
    case crude_json::type_t::boolean: return Variant(j.get<bool>());
    case crude_json::type_t::number:
    {
        double d = j.get<double>();
        // 如果值是整数则转 int64，否则保留 float
        if (d == static_cast<double>(static_cast<int64_t>(d)))
            return Variant(static_cast<int64_t>(d));
        return Variant(d);
    }
    case crude_json::type_t::string: return Variant(j.get<std::string>());
    case crude_json::type_t::array:
    {
        std::vector<Variant> arr;
        for (const auto& elem : j.get<crude_json::array>())
            arr.push_back(jsonToVariant(elem));
        return Variant(std::move(arr));
    }
    case crude_json::type_t::object:
    {
        Variant result;
        result.type = PinDataType::Map;
        for (const auto& kv : j.get<crude_json::object>())
            result.mapSet(Variant(kv.first), jsonToVariant(kv.second));
        return result;
    }
    default:
        return Variant(); // null
    }
}

void RegisterHandlers_Data(std::unordered_map<std::string, NodeHandler>& handlers)
{
    // ========================================================================
    // ToJSON — Variant → JSON 字符串（支持嵌套对象/数组）
    // ========================================================================
    handlers["ToJSON"] = [](ExecutionContext& ctx) {
        auto val = ctx.GetInputValue("Value");
        crude_json::value j = variantToJson(val);
        ctx.SetOutputValue("JSON", Variant(j.dump()));
        return true;
    };

    // ========================================================================
    // FromJSON — JSON 字符串 → Variant（支持嵌套对象/数组/基础类型）
    // ========================================================================
    handlers["FromJSON"] = [](ExecutionContext& ctx) {
        auto json = ctx.GetInputValue("JSON").asString();
        crude_json::value j = crude_json::value::parse(json);
        bool valid = (j.type() != crude_json::type_t::null || json == "null");
        ctx.SetOutputValue("Value", jsonToVariant(j));
        ctx.SetOutputValue("Valid", Variant(valid));
        return true;
    };

    // ========================================================================
    // HasKey — 检查 JSON 对象是否含有指定键（用 crude_json 真正解析）
    // ========================================================================
    handlers["HasKey"] = [](ExecutionContext& ctx) {
        auto json = ctx.GetInputValue("JSON").asString();
        auto key  = ctx.GetInputValue("Key").asString();
        crude_json::value j = crude_json::value::parse(json);
        bool found = j.is_object() && j.contains(key);
        ctx.SetOutputValue("Result", Variant(found));
        return true;
    };

    // ========================================================================
    // GetField — 从 JSON 对象取顶层字段，返回值序列化为字符串
    // 注意：深层路径请用 JSON.GetPath 节点
    // ========================================================================
    handlers["GetField"] = [](ExecutionContext& ctx) {
        auto json  = ctx.GetInputValue("JSON").asString();
        auto key   = ctx.GetInputValue("Key").asString();
        crude_json::value j = crude_json::value::parse(json);
        bool found = false;
        std::string value;
        if (j.is_object() && j.contains(key))
        {
            found = true;
            const auto& field = j[key];
            if (field.is_string())       value = field.get<std::string>();
            else if (field.is_number())  value = std::to_string(field.get<double>());
            else if (field.is_boolean()) value = field.get<bool>() ? "true" : "false";
            else                         value = field.dump(); // 对象/数组序列化
        }
        ctx.SetOutputValue("Value", Variant(value));
        ctx.SetOutputValue("Found", Variant(found));
        return true;
    };

    handlers["ArrayToString"] = [](ExecutionContext& ctx) {
        auto arr = ctx.GetInputValue("Array");
        std::string result = "[";
        for (size_t i = 0; i < arr.arraySize(); ++i)
        {
            if (i > 0) result += ", ";
            result += arr.arrayGet(i).asString();
        }
        result += "]";
        ctx.SetOutputValue("String", Variant(result));
        return true;
    };

    // ============================================================================
    // Conversion 节点处理器
    // ============================================================================

    handlers["IntToFloat"] = [](ExecutionContext& ctx) {
        int64_t v = ctx.GetInputValue("Value").asInt();
        ctx.SetOutputValue("Result", Variant(static_cast<double>(v)));
        return true;
    };

    handlers["FloatToInt"] = [](ExecutionContext& ctx) {
        double v = ctx.GetInputValue("Value").asFloat();
        ctx.SetOutputValue("Result", Variant(static_cast<int64_t>(v)));
        return true;
    };

    handlers["IntToString"] = [](ExecutionContext& ctx) {
        int64_t v = ctx.GetInputValue("Value").asInt();
        ctx.SetOutputValue("Result", Variant(std::to_string(v)));
        return true;
    };

    handlers["FloatToString"] = [](ExecutionContext& ctx) {
        double v = ctx.GetInputValue("Value").asFloat();
        ctx.SetOutputValue("Result", Variant(std::to_string(v)));
        return true;
    };

    handlers["StringToInt"] = [](ExecutionContext& ctx) {
        std::string s = ctx.GetInputValue("Value").asString();
        char* endptr = nullptr;
        int64_t v = static_cast<int64_t>(std::strtoll(s.c_str(), &endptr, 10));
        if (endptr == s.c_str()) v = 0;
        ctx.SetOutputValue("Result", Variant(v));
        return true;
    };

    handlers["StringToFloat"] = [](ExecutionContext& ctx) {
        std::string s = ctx.GetInputValue("Value").asString();
        char* endptr = nullptr;
        double v = std::strtod(s.c_str(), &endptr);
        if (endptr == s.c_str()) v = 0.0;
        ctx.SetOutputValue("Result", Variant(v));
        return true;
    };

    handlers["BoolToInt"] = [](ExecutionContext& ctx) {
        bool b = ctx.GetInputValue("Value").asBool();
        ctx.SetOutputValue("Result", Variant(static_cast<int64_t>(b ? 1 : 0)));
        return true;
    };

    handlers["IntToBool"] = [](ExecutionContext& ctx) {
        int64_t v = ctx.GetInputValue("Value").asInt();
        ctx.SetOutputValue("Result", Variant(v != 0));
        return true;
    };

    handlers["ToString"] = [](ExecutionContext& ctx) {
        auto val = ctx.GetInputValue("Value");
        ctx.SetOutputValue("Result", Variant(val.asString()));
        return true;
    };
}

// ============================================================================
// Misc 处理器
// ============================================================================

} // namespace Runtime
} // namespace NodeEditor
