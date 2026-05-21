// Runtime/Types.h - 蓝图运行时基础类型定义
// 该文件定义了蓝图数据的核心类型，可独立于编辑器使用

#pragma once

#include "BlueprintExport.h"
#include <cstdint>
#include <cstdlib>
#include <cstdio>    // std::snprintf — explicit for MSVC
#include <string>
#include <vector>
#include <unordered_map>
#include <variant>
#include <utility>   // std::pair
#include <memory>    // std::shared_ptr — for Variant::opaqueRef (Any 类型)

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
    Unknown,   // 0
    Boolean,   // 1
    Integer,   // 2
    Float,     // 3
    String,    // 4
    Object,    // 5
    Array,     // 6
    Map,       // 7  键值对映射
    Set,       // 8  集合（无重复，内部用 arrayValue 存储）
    Any,       // 9  通配类型
    Custom     // 10 用户自定义类型
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

// 容器类型（UE4 风格）
enum class ContainerType : int
{
    Single = 0,
    Array  = 1,
    Map    = 2,
    Set    = 3,
};

// ============================================================================
// 数据值类型
// ============================================================================

struct Variant
{
    PinDataType type = PinDataType::Unknown;

    // C++17 std::variant 替代 union，完全消除 type-punning UB
    // monostate = Unknown/未初始化状态
    std::variant<std::monostate, bool, int64_t, double> numericValue;

    // std::string 含非平凡析构，不能放入 variant，单独存储
    std::string stringValue;
    // Array 值 —— 用于 PinDataType::Array
    // 同时也用于 Set（PinDataType::Set）的内部存储
    std::vector<Variant> arrayValue;
    // Map 值 —— 用于 PinDataType::Map
    // 改为有序 vector<pair<Variant,Variant>> 以支持任意键类型
    std::vector<std::pair<Variant, Variant>> mapValue;

    // ============================================================
    // Opaque 跨语言对象引用（仅 PinDataType::Any 使用）
    // ============================================================
    // 用于在 Lua 节点之间透传不透明对象（Lua table / userdata / xLua object 等）。
    // C++ Runtime 永远不解释 opaqueRef 指向的内容，仅做搬运。
    //
    // shared_ptr<void> 的优势：
    //   · 类型擦除 + 引用计数（拷贝 Variant 时自动 +1）
    //   · 支持自定义 deleter（C# GCHandle / Lua luaL_unref / 纯 C 资源释放）
    //   · 不依赖 Lua 头文件，纯 C++ 标准库
    //   · 释放路径由创建者决定（shared_ptr 析构时调 deleter）
    //
    // 序列化、跨 VM、C++ handler 消费等场景下都不应使用 Any，
    // 设计语义请参考方案文档。
    std::shared_ptr<void> opaqueRef;

    // 默认构造
    Variant() : numericValue(std::monostate{}) {}

    // 析构
    ~Variant() = default;

    // 拷贝构造 / 移动构造 / 拷贝赋值 / 移动赋值 —— 全部默认即可
    Variant(const Variant&)            = default;
    Variant(Variant&&) noexcept        = default;
    Variant& operator=(const Variant&) = default;
    Variant& operator=(Variant&&) noexcept = default;

    // 类型转换构造函数
    explicit Variant(bool v)               : type(PinDataType::Boolean), numericValue(v) {}
    explicit Variant(int v)                : type(PinDataType::Integer), numericValue(static_cast<int64_t>(v)) {}
    explicit Variant(int64_t v)            : type(PinDataType::Integer), numericValue(v) {}
    explicit Variant(float v)              : type(PinDataType::Float),   numericValue(static_cast<double>(v)) {}
    explicit Variant(double v)             : type(PinDataType::Float),   numericValue(v) {}
    explicit Variant(const char* v)        : type(PinDataType::String),  stringValue(v) {}
    explicit Variant(const std::string& v) : type(PinDataType::String),  stringValue(v) {}
    explicit Variant(std::vector<Variant> v) : type(PinDataType::Array), arrayValue(std::move(v)) {}

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

    // ============================================================
    // Any（不透明跨语言对象）工厂与访问器
    // ============================================================

    // 持有所有权（shared_ptr 析构时调用 deleter 释放原始资源）
    // 调用者构造 shared_ptr 时把生命周期管理逻辑放到 deleter 里：
    //   auto sp = std::shared_ptr<void>(rawPtr, [](void* p){ /* 释放 p */ });
    //   Variant v = Variant::MakeAny(sp);
    static Variant MakeAny(std::shared_ptr<void> ref)
    {
        Variant v;
        v.type      = PinDataType::Any;
        v.opaqueRef = std::move(ref);
        return v;
    }

    // 借用语义：外部托管生命周期，Variant 不释放（deleter 为 no-op）
    // 适合短生命周期的临时引用（如单次 handler 调用内）
    static Variant MakeAnyBorrow(void* rawPtr)
    {
        Variant v;
        v.type      = PinDataType::Any;
        v.opaqueRef = std::shared_ptr<void>(rawPtr, [](void*){});
        return v;
    }

    // 取出原始指针（不影响引用计数）。type != Any 时返回 nullptr。
    void* asAny() const
    {
        if (type != PinDataType::Any) return nullptr;
        return opaqueRef.get();
    }

    // 取出 shared_ptr（增加引用计数）。type != Any 时返回空 shared_ptr。
    std::shared_ptr<void> asAnyShared() const
    {
        if (type != PinDataType::Any) return nullptr;
        return opaqueRef;
    }

    bool hasAny() const
    {
        return type == PinDataType::Any && opaqueRef != nullptr;
    }


    // ============================================================
    // 相等性比较（用于 Set 去重和 Map 键比较）
    // ============================================================
    // 安全访问 numericValue：variant 处于 monostate 或类型不匹配时返回默认值
    // 避免 std::get 抛 std::bad_variant_access 导致进程崩溃
    bool   numAsBool()   const { auto* p = std::get_if<bool>(&numericValue);    return p ? *p : false; }
    int64_t numAsInt()   const { auto* p = std::get_if<int64_t>(&numericValue); return p ? *p : 0; }
    double numAsDouble() const { auto* p = std::get_if<double>(&numericValue);  return p ? *p : 0.0; }

    bool operator==(const Variant& other) const
    {
        if (type != other.type) return false;
        switch (type)
        {
        case PinDataType::Boolean: return numAsBool()   == other.numAsBool();
        case PinDataType::Integer: return numAsInt()    == other.numAsInt();
        case PinDataType::Float:   return numAsDouble() == other.numAsDouble();
        case PinDataType::String:
        case PinDataType::Object:  return stringValue == other.stringValue;
        case PinDataType::Any:     return opaqueRef.get() == other.opaqueRef.get();
        default:                   return false;
        }
    }
    bool operator!=(const Variant& other) const { return !(*this == other); }

    // Set/Map 键匹配：Float 使用 epsilon 比较，避免浮点精度导致集合操作失效。
    // 不修改 operator== 以保留 Equal 节点的精确语义。
    bool matchesKey(const Variant& other) const
    {
        if (type != other.type) return false;
        if (type == PinDataType::Float)
        {
            double a = numAsDouble();
            double b = other.numAsDouble();
            // ULP-based epsilon：相对误差 1e-9 或绝对误差 1e-12
            double diff = a - b;
            if (diff < 0) diff = -diff;
            double mag = a < 0 ? -a : a;
            double magB = b < 0 ? -b : b;
            double largest = mag > magB ? mag : magB;
            const double kRelEps = 1e-9;
            const double kAbsEps = 1e-12;
            return diff <= (largest * kRelEps + kAbsEps);
        }
        return *this == other;
    }

    // ============================================================
    // 值转换
    // ============================================================
    bool asBool() const
    {
        switch (type)
        {
        case PinDataType::Boolean: return numAsBool();
        case PinDataType::Integer: return numAsInt() != 0;
        case PinDataType::Float:   return numAsDouble() != 0.0;
        case PinDataType::String:  return !stringValue.empty();
        case PinDataType::Object:  return !stringValue.empty();
        case PinDataType::Array:   return !arrayValue.empty();
        case PinDataType::Map:     return !mapValue.empty();
        case PinDataType::Set:     return !arrayValue.empty();
        case PinDataType::Any:     return opaqueRef != nullptr;
        default:                   return false;
        }
    }

    int64_t asInt() const
    {
        switch (type)
        {
        case PinDataType::Integer: return numAsInt();
        case PinDataType::Boolean: return numAsBool() ? 1 : 0;
        case PinDataType::Float:   return static_cast<int64_t>(numAsDouble());
        case PinDataType::String:
        {
            if (stringValue.empty()) return 0;
            char* end = nullptr;
            auto v = std::strtoll(stringValue.c_str(), &end, 10);
            return (end != stringValue.c_str()) ? v : 0;
        }
        case PinDataType::Array: return static_cast<int64_t>(arrayValue.size());
        case PinDataType::Map:   return static_cast<int64_t>(mapValue.size());
        case PinDataType::Set:   return static_cast<int64_t>(arrayValue.size());
        default: return 0;
        }
    }

    double asFloat() const
    {
        switch (type)
        {
        case PinDataType::Float:   return numAsDouble();
        case PinDataType::Integer: return static_cast<double>(numAsInt());
        case PinDataType::Boolean: return numAsBool() ? 1.0 : 0.0;
        case PinDataType::String:
        {
            if (stringValue.empty()) return 0.0;
            char* end = nullptr;
            auto v = std::strtod(stringValue.c_str(), &end);
            return (end != stringValue.c_str()) ? v : 0.0;
        }
        default: return 0.0;
        }
    }

    std::string asString() const
    {
        switch (type)
        {
        case PinDataType::String:  return stringValue;
        case PinDataType::Boolean: return numAsBool() ? "True" : "False";
        case PinDataType::Integer: return std::to_string(numAsInt());
        case PinDataType::Float:
        {
            char buf[64];
            std::snprintf(buf, sizeof(buf), "%.17g", numAsDouble());
            return std::string(buf);
        }
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
                result += kv.first.asString() + ": " + kv.second.asString();
            }
            result += "}";
            return result;
        }
        case PinDataType::Set:
        {
            std::string result = "{";
            for (size_t i = 0; i < arrayValue.size(); ++i)
            {
                if (i > 0) result += ", ";
                result += arrayValue[i].asString();
            }
            result += "}";
            return result;
        }
        case PinDataType::Any:
        {
            // Any 类型只能给出指针级别的调试字符串（C++ 不解析具体内容）
            if (!opaqueRef) return "(any:null)";
            char buf[40];
            std::snprintf(buf, sizeof(buf), "(any:%p)", opaqueRef.get());
            return std::string(buf);
        }
        default:                   return std::string();
        }
    }

    // ============================================================
    // Array 访问方法
    // ============================================================
    const std::vector<Variant>& asArray() const { return arrayValue; }
    size_t arraySize() const { return arrayValue.size(); }
    const Variant& arrayGet(size_t index) const
    {
        static Variant empty;
        return (index < arrayValue.size()) ? arrayValue[index] : empty;
    }

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

    bool arrayRemoveAt(size_t index)
    {
        if (index >= arrayValue.size())
            return false;
        arrayValue.erase(arrayValue.begin() + static_cast<std::ptrdiff_t>(index));
        return true;
    }

    void arrayClear()
    {
        arrayValue.clear();
    }

    // ============================================================
    // Map 访问方法（主版本：Variant key）
    // ============================================================
    const std::vector<std::pair<Variant, Variant>>& asMap() const { return mapValue; }
    size_t mapSize() const { return mapValue.size(); }

    bool mapHasKey(const Variant& key) const
    {
        for (const auto& kv : mapValue)
            if (kv.first.matchesKey(key)) return true;
        return false;
    }

    const Variant& mapGet(const Variant& key) const
    {
        static Variant empty;
        for (const auto& kv : mapValue)
            if (kv.first.matchesKey(key)) return kv.second;
        return empty;
    }

    void mapSet(const Variant& key, const Variant& value)
    {
        if (type != PinDataType::Map)
        {
            type = PinDataType::Map;
            mapValue.clear();
        }
        for (auto& kv : mapValue)
        {
            if (kv.first.matchesKey(key))
            {
                kv.second = value;
                return;
            }
        }
        mapValue.push_back({key, value});
    }

    bool mapRemove(const Variant& key)
    {
        for (auto it = mapValue.begin(); it != mapValue.end(); ++it)
        {
            if (it->first.matchesKey(key))
            {
                mapValue.erase(it);
                return true;
            }
        }
        return false;
    }

    // Map 访问方法（兼容旧 string key 版本）
    bool mapHasKey(const std::string& key) const  { return mapHasKey(Variant(key)); }
    const Variant& mapGet(const std::string& key) const { return mapGet(Variant(key)); }
    void mapSet(const std::string& key, const Variant& value) { mapSet(Variant(key), value); }
    bool mapRemove(const std::string& key) { return mapRemove(Variant(key)); }

    void mapClear()
    {
        mapValue.clear();
    }

    std::vector<Variant> mapKeys() const
    {
        std::vector<Variant> keys;
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

    // ============================================================
    // Set 操作（内部用 arrayValue 存储，setAdd 去重）
    // ============================================================
    void setAdd(const Variant& value)
    {
        if (type != PinDataType::Set)
        {
            type = PinDataType::Set;
            arrayValue.clear();
        }
        for (const auto& v : arrayValue)
            if (v.matchesKey(value)) return;
        arrayValue.push_back(value);
    }

    bool setRemove(const Variant& value)
    {
        for (auto it = arrayValue.begin(); it != arrayValue.end(); ++it)
        {
            if (it->matchesKey(value))
            {
                arrayValue.erase(it);
                return true;
            }
        }
        return false;
    }

    bool setContains(const Variant& value) const
    {
        for (const auto& v : arrayValue)
            if (v.matchesKey(value)) return true;
        return false;
    }

    size_t setSize() const { return arrayValue.size(); }
    void setClear() { arrayValue.clear(); }
    std::vector<Variant> setToArray() const { return arrayValue; }

    // 驻留当前字符串值，返回稳定指针（适合频繁比较的场景）
    BLUEPRINT_API const char* internedString() const;
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
    ContainerType   containerType = ContainerType::Single;
    PinDataType     itemType = PinDataType::Any;
    PinDataType     mapKeyType = PinDataType::String;
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

// ---------------------------------------------------------------------------
// Log / Print level
// ---------------------------------------------------------------------------

/// Severity level for both debug log messages (OnLog) and
/// application-level print messages (OnPrint / PrintString node).
enum class LogLevel : int
{
    Verbose = 0,   ///< Detailed trace information (e.g. per-node execution steps)
    Info    = 1,   ///< Normal informational output (PrintString default)
    Warning = 2,   ///< Non-fatal issues (e.g. missing optional pin, type coercion)
    Error   = 3,   ///< Fatal errors that stop or skip execution
};

/// Returns a short prefix string for a given level, e.g. "[W] ".
inline const char* LogLevelPrefix(LogLevel level)
{
    switch (level)
    {
        case LogLevel::Verbose: return "[V] ";
        case LogLevel::Info:    return "[I] ";
        case LogLevel::Warning: return "[W] ";
        case LogLevel::Error:   return "[E] ";
        default:                return "    ";
    }
}

} // namespace Runtime
} // namespace NodeEditor
