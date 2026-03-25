// Runtime/BuiltinHandlers_Data.cpp -- Data 节点处理器
#include "BuiltinHandlers_Data.h"
#include <sstream>
#include <algorithm>
#include <cctype>

namespace NodeEditor {
namespace Runtime {

void RegisterHandlers_Data(std::unordered_map<std::string, NodeHandler>& handlers)
{
    handlers["ToJSON"] = [](ExecutionContext& ctx) {
        auto val = ctx.GetInputValue("Value");
        std::string json;
        switch (val.type)
        {
        case PinDataType::Boolean:
            json = std::get<bool>(val.numericValue) ? "true" : "false";
            break;
        case PinDataType::Integer:
            json = std::to_string(std::get<int64_t>(val.numericValue));
            break;
        case PinDataType::Float:
            json = std::to_string(std::get<double>(val.numericValue));
            break;
        case PinDataType::String:
            json = "\"" + val.stringValue + "\"";
            break;
        case PinDataType::Array:
        {
            json = "[";
            for (size_t i = 0; i < val.arraySize(); ++i)
            {
                if (i > 0) json += ", ";
                auto elem = val.arrayGet(i);
                if (elem.type == PinDataType::String)
                    json += "\"" + elem.asString() + "\"";
                else
                    json += elem.asString();
            }
            json += "]";
            break;
        }
        default:
            json = "null";
            break;
        }
        ctx.SetOutputValue("JSON", Variant(json));
        return true;
    };

    handlers["FromJSON"] = [](ExecutionContext& ctx) {
        auto json = ctx.GetInputValue("JSON").asString();
        bool valid = false;

        // 简单 JSON 值解析
        // 去除首尾空白
        size_t start = json.find_first_not_of(" \t\n\r");
        size_t end = json.find_last_not_of(" \t\n\r");
        if (start != std::string::npos && end != std::string::npos)
        {
            std::string trimmed = json.substr(start, end - start + 1);
            if (trimmed == "true")
            {
                ctx.SetOutputValue("Value", Variant(true));
                valid = true;
            }
            else if (trimmed == "false")
            {
                ctx.SetOutputValue("Value", Variant(false));
                valid = true;
            }
            else if (trimmed == "null")
            {
                ctx.SetOutputValue("Value", Variant());
                valid = true;
            }
            else if (trimmed.size() >= 2 && trimmed.front() == '"' && trimmed.back() == '"')
            {
                ctx.SetOutputValue("Value", Variant(trimmed.substr(1, trimmed.size() - 2)));
                valid = true;
            }
            else
            {
                // 尝试解析数字（不使用 try/catch，兼容 Emscripten -fno-exceptions）
                bool hasDigit = !trimmed.empty();
                for (char c : trimmed)
                    if (!std::isdigit((unsigned char)c) && c != '.' && c != '-' && c != '+' && c != 'e' && c != 'E')
                        { hasDigit = false; break; }
                if (hasDigit && !trimmed.empty())
                {
                    if (trimmed.find('.') != std::string::npos ||
                        trimmed.find('e') != std::string::npos ||
                        trimmed.find('E') != std::string::npos)
                    {
                        // Parse float manually via strtod (sets errno, no exceptions)
                        char* endptr = nullptr;
                        double v = std::strtod(trimmed.c_str(), &endptr);
                        if (endptr != trimmed.c_str())
                        {
                            ctx.SetOutputValue("Value", Variant(v));
                            valid = true;
                        }
                    }
                    else
                    {
                        // Parse integer manually via strtoll (sets errno, no exceptions)
                        char* endptr = nullptr;
                        int64_t v = static_cast<int64_t>(std::strtoll(trimmed.c_str(), &endptr, 10));
                        if (endptr != trimmed.c_str())
                        {
                            ctx.SetOutputValue("Value", Variant(v));
                            valid = true;
                        }
                    }
                }
            }
        }
        if (!valid)
            ctx.SetOutputValue("Value", Variant());
        ctx.SetOutputValue("Valid", Variant(valid));
        return true;
    };

    handlers["HasKey"] = [](ExecutionContext& ctx) {
        auto json = ctx.GetInputValue("JSON").asString();
        auto key = ctx.GetInputValue("Key").asString();
        // 简单的字符串搜索（查找 "key": 模式）
        std::string pattern = "\"" + key + "\"";
        bool found = json.find(pattern) != std::string::npos;
        ctx.SetOutputValue("Result", Variant(found));
        return true;
    };

    handlers["GetField"] = [](ExecutionContext& ctx) {
        auto json = ctx.GetInputValue("JSON").asString();
        auto key = ctx.GetInputValue("Key").asString();
        std::string pattern = "\"" + key + "\"";
        auto pos = json.find(pattern);
        bool found = false;
        std::string value;
        if (pos != std::string::npos)
        {
            pos += pattern.size();
            // 跳过 : 和空白
            while (pos < json.size() && (json[pos] == ':' || json[pos] == ' ' || json[pos] == '\t'))
                ++pos;
            if (pos < json.size())
            {
                if (json[pos] == '"')
                {
                    ++pos;
                    size_t end = json.find('"', pos);
                    if (end != std::string::npos)
                    {
                        value = json.substr(pos, end - pos);
                        found = true;
                    }
                }
                else
                {
                    // 数字或布尔值
                    size_t end = json.find_first_of(",]} \t\n\r", pos);
                    if (end == std::string::npos) end = json.size();
                    value = json.substr(pos, end - pos);
                    found = true;
                }
            }
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
}

// ============================================================================
// Misc 处理器
// ============================================================================

} // namespace Runtime
} // namespace NodeEditor
