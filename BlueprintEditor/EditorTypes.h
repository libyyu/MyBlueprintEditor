// EditorTypes.h -- 蓝图编辑器 UI 层数据类型定义
// 这些结构包含 ImGui 特有字段（ImColor/ImVec2），是编辑器的 UI 模型
#pragma once

#include <imgui.h>
#include <imgui_node_editor.h>

#include "Types.h"          // Runtime::PinKind, Runtime::NodeType

#include <string>
#include <vector>
#include <sstream>

namespace ed = ax::NodeEditor;

// Runtime 类型别名 —— 必须在 "using namespace ax" 之前声明
// 因为 "using namespace ax" 会将 ax::NodeEditor 引入全局作用域，
// 导致 NodeEditor 产生歧义(全局 NodeEditor vs ax::NodeEditor)
typedef NodeEditor::Runtime::PinKind           PinKind;
typedef NodeEditor::Runtime::NodeType          NodeType;

// 引脚数据类型 —— 编辑器 UI 层特有（含 Flow/Function/Delegate 等 UI 概念）
enum class PinType
{
    Flow,
    Bool,
    Int,
    Float,
    String,
    Object,
    Function,
    Delegate,
    Array,
    Map,        // 键值对映射类型
    Set,        // 集合类型（无重复元素）
    Any,        // 通配类型：可与任何数据类型连接（用于 ArrayGet 等泛型节点）
};

struct Node;

struct Pin
{
    ed::PinId   ID;
    ::Node*     Node;
    std::string Name;
    PinType     Type;
    PinKind     Kind;

    // 引脚默认值（未连线时可在节点上直接编辑）
    bool        BoolValue   = false;
    int64_t     IntValue    = 0;
    double      FloatValue  = 0.0;  // float 默认值（用 double 存储与 Runtime Variant 精度对齐）
    std::string StringValue;
    std::string ObjectValue;    // Object 类型引脚的对象引用 ID

    // 错误状态（UE4 风格）
    bool        IsOrphaned  = false;   // 孤立引脚：NodeDef 中已删除但旧 JSON 中仍存在
    bool        IsRequired  = false;   // 必须连接的引脚（如 Delegate 类型）
    bool        IsHidden    = false;   // 隐藏引脚：不渲染，不参与连线交互（仍存在于数据中）

    // 声明式可见性规则（来自 PinDefinition::customProperties["hiddenWhen"]）
    // 格式: "<引脚名>==<值>"  例: "Sync==true" 表示当同节点的 Sync 引脚值为 true 时隐藏此引脚
    //        被引用的引脚若被连线，则视为条件不满足（保守显示）
    std::string HiddenWhen;

    // ── 枚举条目（来自 PinDefinition::customProperties["enumValues"]）──
    // 支持两种格式（可混用）：
    //   纯字符串:  "GET,POST,PUT"       → label="GET",  value="GET"
    //   Name=val:  "Monday=1,Tuesday=2" → label="Monday", value="1"
    // - String 引脚：StringValue 存 value 字段
    // - Int 引脚：IntValue 存 stoll(value)，显示 label
    // - Float 引脚：FloatValue 存 stof(value)，显示 label
    struct EnumEntry {
        std::string label;   // 显示文本（如 "Monday"）
        std::string value;   // 存储值（如 "1"；无 = 号时 value == label）
    };
    std::vector<EnumEntry> EnumValues;
    // true = 严格模式：只能从下拉选，不允许手动输入
    bool EnumStrict = false;

    Pin(int id, const char* name, PinType type):
        ID(id), Node(nullptr), Name(name), Type(type), Kind(PinKind::Input)
    {
    }
};

struct Node
{
    ed::NodeId ID;
    std::string Name;
    std::string DefinitionId;   // 注册表中的定义 ID（用于序列化）
    std::vector<Pin> Inputs;
    std::vector<Pin> Outputs;
    ImColor Color;
    NodeType Type;
    ImVec2 Size;

    std::string State;
    std::string SavedState;

    // Dynamic input pins support (like UE's Append node)
    // When non-empty, the node supports adding/removing input pins of this type.
    // The first N pins (defined in the node definition) are fixed; extra ones are dynamic.
    PinType     DynamicInputPinType = PinType::Flow;  // type of dynamic pins
    int         DynamicInputFixedCount = 0;           // number of fixed (non-removable) input pins
    bool        HasDynamicInputs = false;             // whether dynamic inputs are enabled

    // 折叠状态（双击标题栏或点折叠按钮切换）
    bool        isCollapsed = false;                  // 折叠时只显示标题栏，隐藏所有引脚

    // 错误状态（UE4 风格）
    bool        HasError = false;                     // 节点是否有错误
    std::string ErrorMessage;                         // 错误描述信息

    Node(int id, const char* name, ImColor color = ImColor(255, 255, 255)):
        ID(id), Name(name), Color(color), Type(NodeType::Blueprint), Size(0, 0)
    {
    }
};

struct Link
{
    ed::LinkId ID;

    ed::PinId StartPinID;
    ed::PinId EndPinID;

    ImColor Color;

    Link(ed::LinkId id, ed::PinId startPinId, ed::PinId endPinId):
        ID(id), StartPinID(startPinId), EndPinID(endPinId), Color(255, 255, 255)
    {
    }
};

// ── 共享工具：解析枚举字符串 → vector<EnumEntry> ───────────────────────────
// 格式（可混用）：
//   "GET,POST,PUT"            → [{label="GET",value="GET"}, ...]
//   "Monday=1,Tuesday=2"      → [{label="Monday",value="1"}, ...]
//   " A = 10 , B = 20 "       → [{label="A",value="10"}, ...] （自动 trim）
// 被 BlueprintEditor.cpp / FileOperations.cpp / ClipboardOps.cpp 共同使用
inline std::vector<Pin::EnumEntry> ParseEnumValues(const std::string& raw)
{
    std::vector<Pin::EnumEntry> out;
    if (raw.empty()) return out;

    // 按逗号分割
    std::string item;
    for (size_t i = 0; i <= raw.size(); ++i)
    {
        if (i == raw.size() || raw[i] == ',')
        {
            // trim item
            size_t s = item.find_first_not_of(' ');
            size_t e = item.find_last_not_of(' ');
            if (s == std::string::npos) { item.clear(); continue; }
            std::string tok = item.substr(s, e - s + 1);
            item.clear();

            // 解析 "Label=Value" 或纯 "Value"
            auto eq = tok.find('=');
            Pin::EnumEntry entry;
            if (eq != std::string::npos)
            {
                entry.label = tok.substr(0, eq);
                entry.value = tok.substr(eq + 1);
                // trim label/value
                auto trimStr = [](std::string& str) {
                    size_t ls = str.find_first_not_of(' ');
                    size_t le = str.find_last_not_of(' ');
                    str = (ls == std::string::npos) ? "" : str.substr(ls, le - ls + 1);
                };
                trimStr(entry.label);
                trimStr(entry.value);
            }
            else
            {
                entry.label = tok;
                entry.value = tok;
            }
            if (!entry.label.empty())
                out.push_back(std::move(entry));
        }
        else item += raw[i];
    }
    return out;
}

struct NodeIdLess
{
    bool operator()(const ed::NodeId& lhs, const ed::NodeId& rhs) const
    {
        return lhs.AsPointer() < rhs.AsPointer();
    }
};
