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
        default: break;
        }
    }

    // 拷贝赋值
    Variant& operator=(const Variant& other)
    {
        if (this != &other)
        {
            // 如果旧类型是 string 但新类型不是，清空 stringValue
            if (type == PinDataType::String && other.type != PinDataType::String)
                stringValue.clear();
            type = other.type;
            switch (type)
            {
            case PinDataType::Boolean: boolValue  = other.boolValue;  break;
            case PinDataType::Integer: intValue   = other.intValue;   break;
            case PinDataType::Float:   floatValue = other.floatValue; break;
            case PinDataType::String:  stringValue = other.stringValue; break;
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
            if (type == PinDataType::String && other.type != PinDataType::String)
                stringValue.clear();
            type = other.type;
            switch (type)
            {
            case PinDataType::Boolean: boolValue  = other.boolValue;  break;
            case PinDataType::Integer: intValue   = other.intValue;   break;
            case PinDataType::Float:   floatValue = other.floatValue; break;
            case PinDataType::String:  stringValue = std::move(other.stringValue); break;
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

    // 获取值（带自动类型转换）
    bool asBool() const
    {
        switch (type)
        {
        case PinDataType::Boolean: return boolValue;
        case PinDataType::Integer: return intValue != 0;
        case PinDataType::Float:   return floatValue != 0.0;
        case PinDataType::String:  return !stringValue.empty();
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
        default:                   return std::string();
        }
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
