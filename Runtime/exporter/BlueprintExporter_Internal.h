// Runtime/exporter/BlueprintExporter_Internal.h
// Shared helpers for BlueprintExporter_*.cpp split files.
// Not part of the public API — only included by Runtime/BlueprintExporter*.cpp.

#pragma once

#include "../BlueprintData.h"
#include "../../Utils/Json/crude_json.h"
#include <iomanip>
#include <sstream>
#include <string>

namespace NodeEditor {
namespace Runtime {
namespace exporter_internal {

// ----------------------------------------------------------------------------
// JSON 字符串构造 / 缩进
// ----------------------------------------------------------------------------

inline std::string escapeJson(const std::string& str)
{
    std::ostringstream oss;
    for (char c : str)
    {
        switch (c)
        {
        case '"':  oss << "\\\""; break;
        case '\\': oss << "\\\\"; break;
        case '\b': oss << "\\b";  break;
        case '\f': oss << "\\f";  break;
        case '\n': oss << "\\n";  break;
        case '\r': oss << "\\r";  break;
        case '\t': oss << "\\t";  break;
        default:
            if (static_cast<unsigned char>(c) <= 0x1f)
            {
                oss << "\\u" << std::hex << std::setw(4) << std::setfill('0')
                    << static_cast<int>(static_cast<unsigned char>(c));
            }
            else
            {
                oss << c;
            }
        }
    }
    return oss.str();
}

inline std::string indentJson(int level, int spaces = 4)
{
    return std::string(level * spaces, ' ');
}

// ----------------------------------------------------------------------------
// JSON 字段安全读取
// ----------------------------------------------------------------------------

inline double getJsonNumber(const crude_json::value& obj, const char* key, double defaultVal = 0.0)
{
    if (obj.contains(key) && obj[key].type() == crude_json::type_t::number)
        return obj[key].get<double>();
    return defaultVal;
}

inline std::string getJsonString(const crude_json::value& obj, const char* key)
{
    if (obj.contains(key) && obj[key].type() == crude_json::type_t::string)
        return obj[key].get<std::string>();
    return "";
}

inline bool getJsonBool(const crude_json::value& obj, const char* key, bool defaultVal = false)
{
    if (obj.contains(key) && obj[key].type() == crude_json::type_t::boolean)
        return obj[key].get<bool>();
    return defaultVal;
}

// ----------------------------------------------------------------------------
// Schema 迁移占位（当前 schema 版本一致，无需迁移）
// ----------------------------------------------------------------------------

inline void ApplySchemaMigrations(BlueprintData& /*data*/, int /*fromVersion*/)
{
    // 当前 schema 版本为 BLUEPRINT_CURRENT_SCHEMA_VERSION，无历史格式需要迁移
}

} // namespace exporter_internal
} // namespace Runtime
} // namespace NodeEditor
