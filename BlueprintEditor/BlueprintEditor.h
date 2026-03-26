// BlueprintEditor.h -- 蓝图编辑器主类声明
#pragma once

#ifndef IMGUI_DEFINE_MATH_OPERATORS
#define IMGUI_DEFINE_MATH_OPERATORS
#endif
#include <application.h>
#include "utilities/builders.h"
#include "utilities/widgets.h"
#include "EditorTypes.h"
#include "IconsFontAwesome6.h"

#include <imgui_node_editor.h>
#include <imgui_internal.h>

// Runtime module
#include "BlueprintRunner.h"
#include "BlueprintExporter.h"
#include "NodeDefinition.h"

#include <string>
#include <vector>
#include <deque>
#include <map>
#include <unordered_map>
#include <array>
#include <algorithm>
#include <utility>
#include <sstream>
#include <chrono>
#include <functional>

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
typedef NodeEditor::Runtime::VariableDefinition RTVariableDefinition;
typedef NodeEditor::Runtime::NodeCategory       RTNodeCategory;
typedef NodeEditor::Runtime::DefaultNodeRegistry RTNodeRegistry;
typedef NodeEditor::Runtime::NodeHandler        RTNodeHandler;
typedef NodeEditor::Runtime::FrameTimerManager  RTFrameTimerManager;
typedef NodeEditor::Runtime::FrameTimerEntry    RTFrameTimerEntry;
typedef NodeEditor::Runtime::TimerHandle        RTTimerHandle;
typedef NodeEditor::Runtime::TimerCallback      RTTimerCallback;

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

static bool Splitter(const char* str_id, bool split_vertically, float thickness, float* size1, float* size2, float min_size1, float min_size2, float splitter_long_axis_size = -1.0f)
{
    using namespace ImGui;
    ImGuiContext& g = *GImGui;
    ImGuiWindow* window = g.CurrentWindow;
    ImGuiID id = window->GetID(str_id);
    ImRect bb;
    bb.Min = window->DC.CursorPos + (split_vertically ? ImVec2(*size1, 0.0f) : ImVec2(0.0f, *size1));
    bb.Max = bb.Min + CalcItemSize(split_vertically ? ImVec2(thickness, splitter_long_axis_size) : ImVec2(splitter_long_axis_size, thickness), 0.0f, 0.0f);
    bool result = SplitterBehavior(bb, id, split_vertically ? ImGuiAxis_X : ImGuiAxis_Y, size1, size2, min_size1, min_size2, 4.0f);

    // 视觉反馈：悬停/拖动时绘制强调色线条
    bool hovered = g.HoveredId == id;
    bool active  = g.ActiveId  == id;
    if (hovered || active)
    {
        ImU32 lineCol = active  ? IM_COL32(75, 140, 190, 220)
                                : IM_COL32(60, 110, 160, 140);
        auto* dl = GetWindowDrawList();
        if (split_vertically)
        {
            float cx = (bb.Min.x + bb.Max.x) * 0.5f;
            dl->AddLine(ImVec2(cx, bb.Min.y), ImVec2(cx, bb.Max.y), lineCol, 2.0f);
        }
        else
        {
            float cy = (bb.Min.y + bb.Max.y) * 0.5f;
            dl->AddLine(ImVec2(bb.Min.x, cy), ImVec2(bb.Max.x, cy), lineCol, 2.0f);
        }
    }
    return result;
}

// 彩色日志行渲染辅助（共享于 DrawExecutionPanel 和 ShowExecutionPanel）
static inline void DrawColoredLogLine(const std::string& line)
{
    static const ImVec4 colError   (0.95f, 0.32f, 0.32f, 1.00f);
    static const ImVec4 colWarn    (0.95f, 0.72f, 0.28f, 1.00f);
    static const ImVec4 colInfo    (0.38f, 0.68f, 0.95f, 1.00f);
    static const ImVec4 colSuccess (0.35f, 0.88f, 0.42f, 1.00f);
    static const ImVec4 colSeparator(0.45f, 0.48f, 0.56f, 0.70f);
    static const ImVec4 colDim     (0.50f, 0.52f, 0.58f, 0.90f);
    static const ImVec4 colDefault (0.82f, 0.84f, 0.90f, 1.00f);

    const ImVec4* color = &colDefault;

    if (line.find("[ERROR]") != std::string::npos)
        color = &colError;
    else if (line.find("[WARN]") != std::string::npos)
        color = &colWarn;
    else if (line.find("[INFO]") != std::string::npos || line.find("[Timer:") != std::string::npos)
        color = &colInfo;
    else if (line.find("Completed Successfully") != std::string::npos)
        color = &colSuccess;
    else if (line.find("FAILED") != std::string::npos || line.find("ABORTED") != std::string::npos)
        color = &colError;
    else if (line.find("========") != std::string::npos)
        color = &colSeparator;
    else if (line.find("Nodes:") == 0 || line.find("Links:") == 0 ||
             line.find("Validation:") == 0 || line.find("Elapsed:") != std::string::npos)
        color = &colDim;

    ImGui::PushStyleColor(ImGuiCol_Text, *color);
    ImGui::TextUnformatted(line.c_str());
    ImGui::PopStyleColor();
}

// ============================================================================
// 变量拖拽 Payload（变量面板 → 画布）
// ============================================================================

struct VarDragPayload
{
    char varName[64]  = {};
    int  dataType     = 0;   // RTPinDataType as int
};
static constexpr const char* VAR_DRAG_DROP_TYPE = "BP_VARIABLE";

// ============================================================================
// 蓝图文档（每个标签页一个实例）
// ============================================================================

struct BlueprintDocument
{
    // 编辑器上下文（每个文档独立的节点编辑器画布）
    ed::EditorContext*  editorContext = nullptr;

    // 蓝图数据
    int                  nextId = 1;
    std::deque<Node>     nodes;
    std::deque<Link>     links;

    // 触摸追踪
    std::map<ed::NodeId, float, NodeIdLess> nodeTouchTime;

    // 文件状态
    std::string          filePath;                         // 文件路径（空=未保存的新文件）
    bool                 isDirty = false;                  // 是否有未保存的修改
    bool                 needSetNodePositions = false;     // 加载后需要设置节点位置
    int                  needNavigateToContent = 0;       // >0 时倒计帧数，到 0 时触发居中
    ImRect               pendingContentBounds;             // 从加载数据计算的节点包围盒
    RTBlueprintData      pendingLoadData;                  // 待设置位置的加载数据

    // 运行时执行状态
    std::vector<std::string>    executionLog;
    std::string                 executionLogText;          // 合并后的日志文本
    bool                        executionLogDirty = false;
    bool                        isExecuting = false;
    std::string                 lastExecutionStatus;
    std::vector<ed::LinkId>     flowLinks;

    // 节点位置追踪（用于检测拖拽移动，标记 dirty）
    std::map<ed::NodeId, ImVec2, NodeIdLess> lastNodePositions;

    // 执行可视化（高亮已执行的节点）
    std::unordered_map<uint64_t, float> executedNodeHighlight;  // nodeId -> 剩余高亮时间(秒)

    // 持久 Runner
    RTBlueprintRunner           persistentRunner;

    // 节点编辑器 UI 状态（原先的 static 变量）
    ed::NodeId contextNodeId      = 0;
    ed::LinkId contextLinkId      = 0;
    ed::PinId  contextPinId       = 0;
    bool       createNewNode      = false;
    Pin*       newNodeLinkPin     = nullptr;
    Pin*       newLinkPin         = nullptr;

    // 变量拖拽到画布的待处理状态
    bool           pendingVarDrop     = false;  // 有待处理的拖拽放置
    VarDragPayload pendingVarPayload  = {};      // 拖拽的变量信息
    ImVec2         pendingVarDropPos  = {};      // 放置时的屏幕坐标

    // 内联编辑控件字符串缓冲区（按 PinId 索引，文档切换时自然隔离）
    std::unordered_map<uintptr_t, std::array<char, 128>> pinStringBuffers;
    std::unordered_map<uintptr_t, std::array<char, 128>> pinObjectBuffers;

    // 节点过滤器（每个文档独立，切换文档后保留各自的过滤状态）
    char nodeFilterBuf[128] = {};

    // 变量列表（蓝图级别的变量定义，可在 Get/Set Variable 节点中引用）
    std::vector<RTVariableDefinition> variables;

    // ---- 编辑器侧哈希索引（O(1) 查找加速） ----
    // 调用 rebuildEditorIndices() 重建；数据变更后调用 invalidateEditorIndices()
    mutable std::unordered_map<uint64_t, size_t>      nodeIdIndex;    // NodeId → nodes[] 下标
    mutable std::unordered_map<uint64_t, size_t>      linkIdIndex;    // LinkId → links[] 下标
    mutable std::unordered_map<uint64_t, const Pin*>  pinIdIndex;     // PinId → Pin* (const, no cast needed)
    mutable std::unordered_map<uint64_t, bool>         pinLinkedCache; // PinId → 是否有链接
    mutable bool editorIndexDirty = true;

    void invalidateEditorIndices() const { editorIndexDirty = true; }

    void rebuildEditorIndices() const
    {
        nodeIdIndex.clear();
        linkIdIndex.clear();
        pinIdIndex.clear();
        pinLinkedCache.clear();

        for (size_t i = 0; i < nodes.size(); ++i)
        {
            uint64_t nid = reinterpret_cast<uintptr_t>(nodes[i].ID.AsPointer());
            nodeIdIndex[nid] = i;
            for (const auto& pin : nodes[i].Inputs)
            {
                uint64_t pid = reinterpret_cast<uintptr_t>(pin.ID.AsPointer());
                pinIdIndex[pid] = &pin;
            }
            for (const auto& pin : nodes[i].Outputs)
            {
                uint64_t pid = reinterpret_cast<uintptr_t>(pin.ID.AsPointer());
                pinIdIndex[pid] = &pin;
            }
        }

        for (size_t i = 0; i < links.size(); ++i)
        {
            uint64_t lid = reinterpret_cast<uintptr_t>(links[i].ID.AsPointer());
            linkIdIndex[lid] = i;

            uint64_t sid = reinterpret_cast<uintptr_t>(links[i].StartPinID.AsPointer());
            uint64_t eid = reinterpret_cast<uintptr_t>(links[i].EndPinID.AsPointer());
            pinLinkedCache[sid] = true;
            pinLinkedCache[eid] = true;
        }

        editorIndexDirty = false;
    }

    void ensureEditorIndices() const
    {
        if (editorIndexDirty) rebuildEditorIndices();
    }

    // 标签页显示名
    std::string GetTabName() const
    {
        if (filePath.empty())
            return "New";
        size_t lastSlash = filePath.find_last_of("/\\");
        std::string name = (lastSlash != std::string::npos) ? filePath.substr(lastSlash + 1) : filePath;
        size_t lastDot = name.find_last_of('.');
        if (lastDot != std::string::npos)
            name = name.substr(0, lastDot);
        return name;
    }

    // 标签页标题（含修改标记）
    std::string GetTabTitle() const
    {
        std::string title = GetTabName();
        if (isDirty)
            title += " *";
        return title;
    }

    // 便捷访问计时器管理器
    RTFrameTimerManager& GetTimerManager() { return persistentRunner.GetTimerManager(); }
    const RTFrameTimerManager& GetTimerManager() const { return persistentRunner.GetTimerManager(); }
};

// ============================================================================
// 蓝图编辑器主类
// ============================================================================

struct BlueprintEditor : public Application
{
    using Application::Application;

    // ------------------------------------------------------------------
    // 多文档管理
    // ------------------------------------------------------------------
    std::vector<std::unique_ptr<BlueprintDocument>> m_Documents;
    int  m_ActiveDocIndex = 0;

    // 延迟打开文件（双击节点时使用，不能在 ed::Begin/End 内部调用 DoOpenFile）
    std::string m_PendingOpenFilePath;
    int         m_PendingSwitchTabIndex = -1;

    // ------------------------------------------------------------------
    // 剪贴板（Copy/Paste/Duplicate）
    // ------------------------------------------------------------------
    struct ClipboardNode
    {
        std::string definitionId;
        std::string name;
        ImVec2      position;              // 原始位置（用于计算粘贴偏移）
        ImColor     color;
        NodeType    type;
        ImVec2      size;                  // Comment 节点尺寸
        bool        hasDynamicInputs = false;
        PinType     dynamicInputPinType = PinType::Flow;
        int         dynamicInputFixedCount = 0;

        struct ClipboardPin
        {
            std::string name;
            PinType     type;
            PinKind     kind;
            bool        boolValue   = false;
            int64_t     intValue    = 0;
            float       floatValue  = 0.0f;
            std::string stringValue;
            std::string objectValue;
            std::string hiddenWhen;
            int         originalLocalIndex = 0;  // 在原节点引脚列表中的索引
        };
        std::vector<ClipboardPin> inputs;
        std::vector<ClipboardPin> outputs;
    };

    struct ClipboardLink
    {
        int srcNodeIdx;     // 在 clipboardNodes 中的索引
        int srcPinIdx;      // 在该节点 outputs 中的索引
        int dstNodeIdx;     // 在 clipboardNodes 中的索引
        int dstPinIdx;      // 在该节点 inputs 中的索引
    };

    std::vector<ClipboardNode> m_ClipboardNodes;
    std::vector<ClipboardLink> m_ClipboardLinks;
    ImVec2                     m_ClipboardCenter;  // 剪贴板中所有节点的质心

    void CopySelectedNodes();
    void PasteNodes(ImVec2 pastePosition);
    void DuplicateSelectedNodes();
    void CutSelectedNodes();

    // ------------------------------------------------------------------
    // 画布节点搜索（Ctrl+F）
    // ------------------------------------------------------------------
    bool  m_ShowSearchOverlay  = false;
    char  m_SearchBuffer[256]  = {};
    int   m_SearchResultIndex  = -1;
    std::vector<ed::NodeId> m_SearchResults;

    void  OpenSearchOverlay();
    void  UpdateSearchResults();
    void  NavigateToSearchResult(int index);
    void  DrawSearchOverlay();

    // ------------------------------------------------------------------
    // 小地图（Minimap）
    // ------------------------------------------------------------------
    bool  m_ShowMinimap = true;
    float m_MinimapSize = 180.0f;   // 小地图边长（像素）

    void  DrawMinimap(ImVec2 editorMin, ImVec2 editorMax);

    // ------------------------------------------------------------------
    // 未保存确认对话框
    // ------------------------------------------------------------------
    bool  m_ShowUnsavedDialog = false;
    int   m_PendingCloseTabIndex = -1;    // 待关闭的标签索引
    bool  m_PendingQuitApp = false;       // 待退出应用

    void  ShowUnsavedChangesDialog();

    // ------------------------------------------------------------------
    // 最近打开文件（持久化到磁盘）
    // ------------------------------------------------------------------
    static const int MaxRecentFiles = 10;
    std::vector<std::string> m_RecentFiles;
    void  AddRecentFile(const std::string& path);
    void  DrawRecentFilesMenu();
    void  SaveRecentFiles();
    void  LoadRecentFiles();

    // ------------------------------------------------------------------
    // 节点对齐（Align Selected Nodes）
    // ------------------------------------------------------------------
    enum class AlignMode { Left, Right, Top, Bottom, CenterH, CenterV };
    void  AlignSelectedNodes(AlignMode mode);

    BlueprintDocument* ActiveDoc()
    {
        if (m_Documents.empty()) return nullptr;
        if (m_ActiveDocIndex < 0 || m_ActiveDocIndex >= (int)m_Documents.size())
            m_ActiveDocIndex = 0;
        return m_Documents[m_ActiveDocIndex].get();
    }

    BlueprintDocument* CreateNewDocument();               // 创建新的空白文档
    void CloseDocument(int index);                        // 关闭指定文档

    // ------------------------------------------------------------------
    // ID 管理（操作当前活跃文档）
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

    // 自动类型转换节点辅助
    // 返回能将 from 类型转为 to 类型的内置节点 definitionId，无则返回空串
    static std::string GetConversionNode(PinType from, PinType to);
    // 在 startPin → endPin 之间插入一个转换节点，自动连好两端
    // 返回生成的转换节点指针（失败返回 nullptr）
    Node* InsertConversionNode(Pin* startPin, ed::PinId startPinId,
                               Pin* endPin,   ed::PinId endPinId);

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
    PinType GetLinkColor(const Pin* startPin, const Pin* endPin);  // 链接颜色：Any 引脚使用对端类型
    PinType GetResolvedPinType(const Pin& pin);  // Any 引脚连线后解析为对端实际类型
    void    DrawPinIcon(const Pin& pin, bool connected, int alpha);
    void    CreateLinkWithFlowReconnect(Pin* startPin, ed::PinId startPinId,
                                        Pin* endPin, ed::PinId endPinId);  // 创建链接 + UE4 Flow 自动重连
    void    ShowStyleEditor(bool* show = nullptr);
    void    ShowLeftPane(float paneWidth);             // 旧版左侧面板（已不使用）
    void    DrawNodeListPanel();                       // 右侧节点列表/变量面板（嵌入式）
    void    DrawVariablePanel();                       // 变量面板（在 DrawNodeListPanel TabBar 内调用）
    void    DrawExecutionPanel();                      // 底部执行输出面板（嵌入式）
    void    DrawTimerPanel();                          // 计时器监控浮动面板

    // ------------------------------------------------------------------
    // 文件操作
    // ------------------------------------------------------------------
    void    NewFile();                                  // 新建蓝图（新标签页）
    void    OpenFile();                                 // 打开蓝图文件（新标签页）
    void    SaveFile();                                 // 保存当前标签页
    void    SaveFileAs();                               // 另存为
    void    DoSaveFile(const std::string& path);        // 执行保存
    void    DoOpenFile(const std::string& path);        // 执行打开
    void    ClearEditor();                              // 清空当前文档

    // 编辑器数据序列化
    RTBlueprintData BuildFullEditorData();
    void            LoadEditorData(const RTBlueprintData& data);

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
    // Application 生命周期
    // ------------------------------------------------------------------
    void OnStart() override;
    void OnStop() override;
    void OnFrame(float deltaTime) override;
    ImGuiWindowFlags GetWindowFlags() const override;

    // ------------------------------------------------------------------
    // 成员变量（全局共享，不随文档变化）
    // ------------------------------------------------------------------
    const int            m_PinIconSize = 24;
    ImTextureID          m_HeaderBackground = ImTextureID_Invalid;
    ImTextureID          m_SaveIcon = ImTextureID_Invalid;
    ImTextureID          m_RestoreIcon = ImTextureID_Invalid;
    const float          m_TouchTime = 1.0f;
    bool                 m_ShowOrdinals = false;
    bool                 m_ShowNodeListWindow = true;     // 左侧面板可见
    bool                 m_ShowExecutionWindow = true;    // 底部面板可见
    bool                 m_ShowTimerWindow = false;       // 计时器监控面板可见
    bool                 m_ShowStyleEditorWindow = false; // 样式编辑器窗口可见

    // VSCode 风格布局尺寸（可拖拽调整）
    float                m_LeftPanelWidth  = 250.0f;
    float                m_BottomPanelHeight = 200.0f;

    // 节点定义注册表 & 处理器注册表（全局共享）
    RTNodeRegistry                                          m_NodeRegistry;
    std::unordered_map<std::string, RTNodeHandler>          m_HandlerRegistry;

    // Default handler
    RTNodeHandler m_DefaultHandler;

};
