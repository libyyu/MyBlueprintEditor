// Utils/Serialization/Json.h - JSON 序列化库
// 包装原有的 crude_json，提供统一的接口

#pragma once

// 包含原有的 JSON 库
#include "../Json/crude_json.h"

namespace NodeEditor {
namespace Utils {
namespace Serialization {

// 使用原有的 JSON 类型
using JsonValue = ::crude_json::value;
using JsonObject = ::crude_json::object;
using JsonArray = ::crude_json::array;
using JsonString = ::crude_json::string;
using JsonNumber = ::crude_json::number;
using JsonBoolean = ::crude_json::boolean;

// JSON 值类型枚举
using JsonType = ::crude_json::type_t;

// 便捷函数
inline JsonValue parseJson(const std::string& data)
{
    return JsonValue::parse(data);
}

inline std::string dumpJson(const JsonValue& value, int indent = -1, char indent_char = ' ')
{
    return value.dump(indent, indent_char);
}

inline std::pair<JsonValue, bool> loadJson(const std::string& path)
{
    return JsonValue::load(path);
}

inline bool saveJson(const JsonValue& value, const std::string& path, int indent = -1, char indent_char = ' ')
{
    return value.save(path, indent, indent_char);
}

} // namespace Serialization
} // namespace Utils
} // namespace NodeEditor
