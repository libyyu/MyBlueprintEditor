// BlueprintEditor.h -- 蓝图编辑器主类声明
#pragma once

#ifndef IMGUI_DEFINE_MATH_OPERATORS
#define IMGUI_DEFINE_MATH_OPERATORS
#endif
#include <application.h>
#include "utilities/builders.h"
#include "utilities/widgets.h"
#include "EditorTypes.h"

#include <imgui_node_editor.h>
#include <imgui_internal.h>

// Runtime module
#include "BlueprintRunner.h"
#include "BlueprintExporter.h"
#include "NodeDefinition.h"

#include <string>
#include <vector>
#include <map>
#include <unordered_map>
#include <array>
#include <algorithm>
#include <utility>
#include <sstream>
#include <chrono>

namespace ed   = ax::NodeEditor;
namespace util = ax::NodeEditor::Utilities;

// Runtime 类型别名
typedef NodeEditor::Runtime::BlueprintData     RTBlueprintData;
typedef NodeEditor::Runtime::BlueprintRunner   RTBlueprintRunner;
typedef NodeEditor::Runtime::ExecutionContext   RTContext;
typedef NodeEditor::Runtime::Variant           RTVariant;
typedef NodeEditor::Runtime::NodeInstance       RTNodeInstance;
typedef NodeEditor::Runtime::LinkInstance       RTLinkInstance;
typedef NodeEditor::Runtime::PinInfo            RTPinInfo;
typedef NodeEditor::Runtime::PinDataType        RTPinDataType;
typedef NodeEditor::Runtime::NodeDefinition     RTNodeDef;
typedef NodeEditor::Runtime::PinDefinition      RTPinDef;
typedef NodeEditor::Runtime::NodeCategory       RTNodeCategory;
typedef NodeEditor::Runtime::DefaultNodeRegistry RTNodeRegistry;
typedef NodeEditor::Runtime::NodeHandler        RTNodeHandler;

using namespace ax;
using ax::Widgets::IconType;

// ============================================================================
// 辅助函数
// ============================================================================

static inline ImRect ImGui_GetItemRect()
{
    return ImRect(ImGui::GetItemRectMin(), ImGui::GetItemRectMax());
}

static inline ImRect ImRect_Expanded(const ImRect& rect, float x, float y)
{
    auto result = rect;
    result.Min.x -= x;
    result.Min.y -= y;
    result.Max.x += x;
    result.Max.y += y;
    return result;
}

static bool Splitter(bool split_vertically, float thickness, float* size1, float* size2, float min_size1, float min_size2, float splitter_long_axis_size = -1.0f)
{
    using namespace ImGui;
    ImGuiContext& g = *GImGui;
    ImGuiWindow* window = g.CurrentWindow;
    ImGuiID id = window->GetID("##Splitter");
    ImRect bb;
    bb.Min = window->DC.CursorPos + (split_vertically ? ImVec2(*size1, 0.0f) : ImVec2(0.0f, *size1));
    bb.Max = bb.Min + CalcItemSize(split_vertically ? ImVec2(thickness, splitter_long_axis_size) : ImVec2(splitter_long_axis_size, thickness), 0.0f, 0.0f);
    return SplitterBehavior(bb, id, split_vertically ? ImGuiAxis_X : ImGuiAxis_Y, size1, size2, min_size1, min_size2, 0.0f);
}

// ============================================================================
// 蓝图编辑器主类
// ============================================================================

struct BlueprintEditor : public Application
{
    using Application::Application;

    // ------------------------------------------------------------------
    // ID 管理
    // ------------------------------------------------------------------
    int GetNextId();
    ed::LinkId GetNextLinkId();

    // ------------------------------------------------------------------
    // 触摸追踪（节点高亮动画）
    // ------------------------------------------------------------------
    void  TouchNode(ed::NodeId id);
    float GetTouchProgress(ed::NodeId id);
    void  UpdateTouch();

    // ------------------------------------------------------------------
    // 查找
    // ------------------------------------------------------------------
    Node* FindNode(ed::NodeId id);
    Link* FindLink(ed::LinkId id);
    Pin*  FindPin(ed::PinId id);
    bool  IsPinLinked(ed::PinId id);
    bool  CanCreateLink(Pin* a, Pin* b);

    // ------------------------------------------------------------------
    // 节点构建
    // ------------------------------------------------------------------
    void  BuildNode(Node* node);
    void  BuildNodes();

    // ------------------------------------------------------------------
    // 节点创建（从注册表）
    // ------------------------------------------------------------------
    Node* SpawnNodeByDef(const std::string& defId);
    void  FixupSpecialPinTypes(Node* node, const RTNodeDef* def);

    // ------------------------------------------------------------------
    // 节点定义 & 运行时处理器 注册
    // ------------------------------------------------------------------
    void RegisterBuiltinNodeDefinitions();
    void RegisterBuiltinHandlers();

    // ------------------------------------------------------------------
    // 右键菜单
    // ------------------------------------------------------------------
    Node* ShowCreateNodeMenu();

    // ------------------------------------------------------------------
    // UI 绘制
    // ------------------------------------------------------------------
    ImColor GetIconColor(PinType type);
    void    DrawPinIcon(const Pin& pin, bool connected, int alpha);
    void    ShowStyleEditor(bool* show = nullptr);
    void    ShowLeftPane(float paneWidth);             // 旧版左侧面板（已不使用）
    void    ShowNodeListWindow(bool* p_open);          // 可浮动节点列表窗口
    void    ShowExecutionWindow(bool* p_open);         // 可浮动执行输出窗口

    // ------------------------------------------------------------------
    // 蓝图执行
    // ------------------------------------------------------------------
    RTBlueprintData BuildRuntimeData();
    void            ExecuteBlueprint();
    void            ShowExecutionPanel(float paneWidth);

    // ------------------------------------------------------------------
    // 类型映射
    // ------------------------------------------------------------------
    static RTPinDataType MapPinType(PinType type);
    static PinType       MapRTPinDataType(RTPinDataType dt, bool isExec);

    // ------------------------------------------------------------------
    // 引脚定义辅助
    // ------------------------------------------------------------------
    static RTPinDef MakePin(const char* name, RTPinDataType dt, bool isExec = false);
    static RTPinDef MakeFlowPin(const char* name = "");

    // ------------------------------------------------------------------
    // Application 生命周期
    // ------------------------------------------------------------------
    void OnStart() override;
    void OnStop() override;
    void OnFrame(float deltaTime) override;
    ImGuiWindowFlags GetWindowFlags() const override;

    // ------------------------------------------------------------------
    // 成员变量
    // ------------------------------------------------------------------
    int                  m_NextId = 1;
    const int            m_PinIconSize = 24;
    std::vector<Node>    m_Nodes;
    std::vector<Link>    m_Links;
    ImTextureID          m_HeaderBackground = ImTextureID_Invalid;
    ImTextureID          m_SaveIcon = ImTextureID_Invalid;
    ImTextureID          m_RestoreIcon = ImTextureID_Invalid;
    const float          m_TouchTime = 1.0f;
    std::map<ed::NodeId, float, NodeIdLess> m_NodeTouchTime;
    bool                 m_ShowOrdinals = false;
    bool                 m_ShowNodeListWindow = true;     // 节点列表窗口可见
    bool                 m_ShowExecutionWindow = true;    // 执行输出窗口可见

    // 节点定义注册表 & 处理器注册表
    RTNodeRegistry                                          m_NodeRegistry;
    std::unordered_map<std::string, RTNodeHandler>          m_HandlerRegistry;

    // 运行时执行状态
    std::vector<std::string>    m_ExecutionLog;
    std::string                 m_ExecutionLogText;     // 合并后的日志文本（供选词拷贝）
    bool                        m_ExecutionLogDirty = false; // 日志是否有更新
    bool                        m_ShowExecutionPanel = true;
    bool                        m_IsExecuting = false;
    std::string                 m_LastExecutionStatus;
    std::vector<ed::LinkId>     m_FlowLinks;

    // Default handler
    RTNodeHandler m_DefaultHandler;
};
