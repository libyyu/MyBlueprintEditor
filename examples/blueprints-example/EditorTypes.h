// EditorTypes.h -- 蓝图编辑器 UI 层数据类型定义
// 这些结构包含 ImGui 特有字段（ImColor/ImVec2），是编辑器的 UI 模型
#pragma once

#include <imgui.h>
#include <imgui_node_editor.h>

#include "Types.h"          // Runtime::PinKind, Runtime::NodeType

#include <string>
#include <vector>

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
    int         IntValue    = 0;
    float       FloatValue  = 0.0f;
    std::string StringValue;

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

struct NodeIdLess
{
    bool operator()(const ed::NodeId& lhs, const ed::NodeId& rhs) const
    {
        return lhs.AsPointer() < rhs.AsPointer();
    }
};
