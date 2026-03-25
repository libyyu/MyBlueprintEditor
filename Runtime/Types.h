// Runtime/Types.h - 蓝图运行时基础类型定义
// 该文件定义了蓝图数据的核心类型，可独立于编辑器使用

#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <unordered_map>

namespace NodeEditor {
namespace Runtime {

// ============================================================================
// ID 类型定义
// ============================================================================

using NodeId = uint64_t;
using PinId = uint64_t;
using LinkId = uint64_t;

constexpr NodeId InvalidNodeId = 0;
constexpr PinId InvalidPinId = 0;
constexpr LinkId InvalidLinkId = 0;

// ============================================================================
// 枚举类型
// ============================================================================

// 引脚类型
enum class PinKind
{
    Input,
    Output
};

// 引脚数据类型
enum class PinDataType
{
    Unknown,
    Boolean,
    Integer,
    Float,
    String,
    Object,
    Array,
    Map,    // 键值对映射（字符串键 → Variant 值）
    Any,
    Custom  // 用户自定义类型
};

// 节点类型 —— 编辑器与运行时共用
enum class NodeType
{
    Blueprint,  // 蓝图节点（默认，带标题栏+引脚）
    Simple,     // 简洁节点（精简显示）
    Tree,       // 行为树节点
    Comment,    // 注释节点
    Houdini,    // Houdini 风格节点
    Group,      // 组节点
    Reroute,    // 重定向节点
    Custom      // 自定义节点
};

// ============================================================================
// 数据值类型
// ============================================================================

struct Variant
{
    PinDataType type = PinDataType::Unknown;

    // 使用 union 节省内存，同一时刻只存储一种类型的值
    union
    {
        bool        boolValue;
        int64_t     intValue;
        double      floatValue;
    };
    // std::string 含非平凡析构，不能放入 union，单独存储
    std::string stringValue;
    // Array 值 —— 用于 PinDataType::Array
    std::vector<Variant> arrayValue;
    // Map 值 —— 用于 PinDataType::Map（存储键为 string，运行时任意类型通过 asString() 转换）
    std::unordered_map<std::string, Variant> mapValue;

    // 默认构造
    Variant() : intValue(0) {}

    // 析构
    ~Variant() = default;

    // 拷贝构造
    Variant(const Variant& other)
        : type(other.type), intValue(0)
    {
        switch (type)
        {
        case PinDataType::Boolean: boolValue  = other.boolValue;  break;
        case PinDataType::Integer: intValue   = other.intValue;   break;
        case PinDataType::Float:   floatValue = other.floatValue; break;
        case PinDataType::String:  stringValue = other.stringValue; break;
        case PinDataType::Object:  stringValue = other.stringValue; break;
        case PinDataType::Array:   arrayValue = other.arrayValue; break;
        case PinDataType::Map:     mapValue = other.mapValue; break;
        default: break;
        }
    }

    // 移动构造
    Variant(Variant&& other) noexcept
        : type(other.type), intValue(0)
    {
        switch (type)
        {
        case PinDataType::Boolean: boolValue  = other.boolValue;  break;
        case PinDataType::Integer: intValue   = other.intValue;   break;
        case PinDataType::Float:   floatValue = other.floatValue; break;
        case PinDataType::String:  stringValue = std::move(other.stringValue); break;
        case PinDataType::Object:  stringValue = std::move(other.stringValue); break;
        case PinDataType::Array:   arrayValue = std::move(other.arrayValue); break;
        case PinDataType::Map:     mapValue = std::move(other.mapValue); break;
        default: break;
        }
    }

    // 拷贝赋值
    Variant& operator=(const Variant& other)
    {
        if (this != &other)
        {
            // 如果旧类型使用 stringValue 但新类型不使用，清空 stringValue
            if ((type == PinDataType::String || type == PinDataType::Object)
                && other.type != PinDataType::String && other.type != PinDataType::Object)
                stringValue.clear();
            if (type == PinDataType::Array && other.type != PinDataType::Array)
                arrayValue.clear();
            if (type == PinDataType::Map && other.type != PinDataType::Map)
                mapValue.clear();
            type = other.type;
            switch (type)
            {
            case PinDataType::Boolean: boolValue  = other.boolValue;  break;
            case PinDataType::Integer: intValue   = other.intValue;   break;
            case PinDataType::Float:   floatValue = other.floatValue; break;
            case PinDataType::String:  stringValue = other.stringValue; break;
            case PinDataType::Object:  stringValue = other.stringValue; break;
            case PinDataType::Array:   arrayValue = other.arrayValue; break;
            case PinDataType::Map:     mapValue = other.mapValue; break;
            default: intValue = 0; break;
            }
        }
        return *this;
    }

    // 移动赋值
    Variant& operator=(Variant&& other) noexcept
    {
        if (this != &other)
        {
            if ((type == PinDataType::String || type == PinDataType::Object)
                && other.type != PinDataType::String && other.type != PinDataType::Object)
                stringValue.clear();
            if (type == PinDataType::Array && other.type != PinDataType::Array)
                arrayValue.clear();
            if (type == PinDataType::Map && other.type != PinDataType::Map)
                mapValue.clear();
            type = other.type;
            switch (type)
            {
            case PinDataType::Boolean: boolValue  = other.boolValue;  break;
            case PinDataType::Integer: intValue   = other.intValue;   break;
            case PinDataType::Float:   floatValue = other.floatValue; break;
            case PinDataType::String:  stringValue = std::move(other.stringValue); break;
            case PinDataType::Object:  stringValue = std::move(other.stringValue); break;
            case PinDataType::Array:   arrayValue = std::move(other.arrayValue); break;
            case PinDataType::Map:     mapValue = std::move(other.mapValue); break;
            default: intValue = 0; break;
            }
        }
        return *this;
    }

    // 类型转换构造函数
    explicit Variant(bool v)               : type(PinDataType::Boolean), boolValue(v) {}
    explicit Variant(int v)                : type(PinDataType::Integer), intValue(v) {}
    explicit Variant(int64_t v)            : type(PinDataType::Integer), intValue(v) {}
    explicit Variant(float v)              : type(PinDataType::Float),   floatValue(static_cast<double>(v)) {}
    explicit Variant(double v)             : type(PinDataType::Float),   floatValue(v) {}
    explicit Variant(const char* v)        : type(PinDataType::String),  intValue(0), stringValue(v) {}
    explicit Variant(const std::string& v) : type(PinDataType::String),  intValue(0), stringValue(v) {}
    explicit Variant(std::vector<Variant> v) : type(PinDataType::Array), intValue(0), arrayValue(std::move(v)) {}
    explicit Variant(std::unordered_map<std::string, Variant> m) : type(PinDataType::Map), intValue(0), mapValue(std::move(m)) {}

    // Object 工厂方法（用字符串 ID 表示对象引用）
    static Variant MakeObject(const std::string& objectId)
    {
        Variant v;
        v.type = PinDataType::Object;
        v.stringValue = objectId;
        return v;
    }

    // 获取对象引用 ID
    std::string asObjectId() const
    {
        if (type == PinDataType::Object) return stringValue;
        if (type == PinDataType::String) return stringValue;
        return "";
    }

    // 获取值（带自动类型转换）
    bool asBool() const
    {
        switch (type)
        {
        case PinDataType::Boolean: return boolValue;
        case PinDataType::Integer: return intValue != 0;
        case PinDataType::Float:   return floatValue != 0.0;
        case PinDataType::String:  return !stringValue.empty();
        case PinDataType::Object:  return !stringValue.empty();
        case PinDataType::Array:   return !arrayValue.empty();
        case PinDataType::Map:     return !mapValue.empty();
        default:                   return false;
        }
    }

    int64_t asInt() const
    {
        switch (type)
        {
        case PinDataType::Integer: return intValue;
        case PinDataType::Boolean: return boolValue ? 1 : 0;
        case PinDataType::Float:   return static_cast<int64_t>(floatValue);
        case PinDataType::String:
            try { return std::stoll(stringValue); }
            catch (...) { return 0; }
        case PinDataType::Array:   return static_cast<int64_t>(arrayValue.size());
        case PinDataType::Map:     return static_cast<int64_t>(mapValue.size());
        default: return 0;
        }
    }

    double asFloat() const
    {
        switch (type)
        {
        case PinDataType::Float:   return floatValue;
        case PinDataType::Integer: return static_cast<double>(intValue);
        case PinDataType::Boolean: return boolValue ? 1.0 : 0.0;
        case PinDataType::String:
            try { return std::stod(stringValue); }
            catch (...) { return 0.0; }
        default: return 0.0;
        }
    }

    std::string asString() const
    {
        switch (type)
        {
        case PinDataType::String:  return stringValue;
        case PinDataType::Boolean: return boolValue ? "True" : "False";
        case PinDataType::Integer: return std::to_string(intValue);
        case PinDataType::Float:   return std::to_string(floatValue);
        case PinDataType::Object:  return stringValue.empty() ? "(none)" : stringValue;
        case PinDataType::Array:
        {
            std::string result = "[";
            for (size_t i = 0; i < arrayValue.size(); ++i)
            {
                if (i > 0) result += ", ";
                result += arrayValue[i].asString();
            }
            result += "]";
            return result;
        }
        case PinDataType::Map:
        {
            std::string result = "{";
            bool first = true;
            for (const auto& kv : mapValue)
            {
                if (!first) result += ", ";
                first = false;
                result += "\"" + kv.first + "\": " + kv.second.asString();
            }
            result += "}";
            return result;
        }
        default:                   return std::string();
        }
    }

    // Array 访问方法
    const std::vector<Variant>& asArray() const { return arrayValue; }
    size_t arraySize() const { return arrayValue.size(); }
    const Variant& arrayGet(size_t index) const
    {
        static Variant empty;
        return (index < arrayValue.size()) ? arrayValue[index] : empty;
    }

    // 设置数组指定索引处的元素，索引越界时自动扩展
    void arraySet(size_t index, const Variant& value)
    {
        if (type != PinDataType::Array)
        {
            type = PinDataType::Array;
            arrayValue.clear();
        }
        if (index >= arrayValue.size())
            arrayValue.resize(index + 1);
        arrayValue[index] = value;
    }

    // 移除数组指定索引处的元素，返回是否成功
    bool arrayRemoveAt(size_t index)
    {
        if (index >= arrayValue.size())
            return false;
        arrayValue.erase(arrayValue.begin() + static_cast<std::ptrdiff_t>(index));
        return true;
    }

    // 清空数组
    void arrayClear()
    {
        arrayValue.clear();
    }

    // Map 访问方法
    const std::unordered_map<std::string, Variant>& asMap() const { return mapValue; }
    size_t mapSize() const { return mapValue.size(); }

    bool mapHasKey(const std::string& key) const
    {
        return mapValue.find(key) != mapValue.end();
    }

    const Variant& mapGet(const std::string& key) const
    {
        static Variant empty;
        auto it = mapValue.find(key);
        return (it != mapValue.end()) ? it->second : empty;
    }

    void mapSet(const std::string& key, const Variant& value)
    {
        if (type != PinDataType::Map)
        {
            type = PinDataType::Map;
            mapValue.clear();
        }
        mapValue[key] = value;
    }

    bool mapRemove(const std::string& key)
    {
        return mapValue.erase(key) > 0;
    }

    void mapClear()
    {
        mapValue.clear();
    }

    std::vector<std::string> mapKeys() const
    {
        std::vector<std::string> keys;
        keys.reserve(mapValue.size());
        for (const auto& kv : mapValue)
            keys.push_back(kv.first);
        return keys;
    }

    std::vector<Variant> mapValues() const
    {
        std::vector<Variant> values;
        values.reserve(mapValue.size());
        for (const auto& kv : mapValue)
            values.push_back(kv.second);
        return values;
    }
};

// ============================================================================
// 引脚信息
// ============================================================================

struct PinInfo
{
    PinId           id = InvalidPinId;
    std::string     name;
    PinKind         kind = PinKind::Input;
    PinDataType     dataType = PinDataType::Unknown;
    Variant         defaultValue;
    bool            allowMultiple = false;  // 是否允许连接多个链接
    bool            isExec = false;         // 是否是执行引脚
};

// ============================================================================
// 节点位置信息
// ============================================================================

struct NodePosition
{
    float x = 0.0f;
    float y = 0.0f;
    
    NodePosition() = default;
    NodePosition(float _x, float _y) : x(_x), y(_y) {}
};

// ============================================================================
// 节点尺寸信息
// ============================================================================

struct NodeSize
{
    float width = 0.0f;
    float height = 0.0f;
    
    NodeSize() = default;
    NodeSize(float w, float h) : width(w), height(h) {}
};

} // namespace Runtime
} // namespace NodeEditor
