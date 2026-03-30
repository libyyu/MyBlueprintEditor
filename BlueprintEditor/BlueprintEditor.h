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
#include "FunctionLibrary.h"
#include "EventBus.h"
#include "BpProject.h"
#include "BpLogger.h"

#include <string>
#include <vector>
#include <deque>
#include <map>
#include <unordered_map>
#include <unordered_set>
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
typedef NodeEditor::Runtime::ExecutionResult    RTExecutionResult;
typedef NodeEditor::Runtime::BlueprintClass     RTBlueprintClass;
typedef NodeEditor::Runtime::PinDefinition      RTPinDef;
typedef NodeEditor::Runtime::VariableDefinition RTVariableDefinition;
typedef NodeEditor::Runtime::FunctionDefinition RTFunctionDefinition;
typedef NodeEditor::Runtime::EventBus           RTEventBus;
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
// 辅助函数（实现在 EditorUtils.cpp）
// ============================================================================

inline ImRect ImGui_GetItemRect()
{
    return ImRect(ImGui::GetItemRectMin(), ImGui::GetItemRectMax());
}

inline ImRect ImRect_Expanded(const ImRect& rect, float x, float y)
{
    auto result = rect;
    result.Min.x -= x;
    result.Min.y -= y;
    result.Max.x += x;
    result.Max.y += y;
    return result;
}

// 分割条（实现在 EditorUtils.cpp）
bool Splitter(const char* str_id, bool split_vertically, float thickness, float* size1, float* size2, float min_size1, float min_size2, float splitter_long_axis_size = -1.0f);

// 彩色日志行渲染辅助（实现在 EditorUtils.cpp）
void DrawColoredLogLine(const std::string& line);

// 节点颜色映射（统一入口，实现在 EditorUtils.cpp）
ImColor GetNodeColor(const RTNodeDef* def);

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
// Undo/Redo 快照
// ============================================================================

// 一次操作前的完整文档快照（节点/链接/变量/ID计数/节点位置）
struct UndoState
{
    std::deque<Node>                             nodes;
    std::deque<Link>                             links;
    std::vector<RTVariableDefinition>            variables;
    int                                          nextId = 1;
    std::map<ed::NodeId, ImVec2, NodeIdLess>     nodePositions; // 快照捕获时的节点位置
};

static constexpr int kMaxUndoSteps = 50;

// ============================================================================
// 蓝图文档（每个标签页一个实例）
// ============================================================================

class BlueprintDocument
{
public:
    BlueprintDocument() = default;
    ~BlueprintDocument() = default;

    // 不可复制（含 ed::EditorContext* 和 RTBlueprintRunner）
    BlueprintDocument(const BlueprintDocument&) = delete;
    BlueprintDocument& operator=(const BlueprintDocument&) = delete;
    BlueprintDocument(BlueprintDocument&&) = default;
    BlueprintDocument& operator=(BlueprintDocument&&) = default;

    // 编辑器上下文（每个文档独立的节点编辑器画布）
    ed::EditorContext*  editorContext = nullptr;

    // 蓝图数据
    int                  nextId = 1;
    std::deque<Node>     nodes;
    std::deque<Link>     links;

    // 蓝图类型（对应 Runtime::BlueprintClass，决定编辑器 UI 限制与运行行为）
    RTBlueprintClass     blueprintClass = RTBlueprintClass::Actor;

    // 触摸追踪
    std::map<ed::NodeId, float, NodeIdLess> nodeTouchTime;

    // 文件状态
    std::string          filePath;                         // 文件路径（空=未保存的新文件）
    std::string          untitledName;                     // 未保存时显示的名称（Untitled-N）
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

    // 断点集合（nodeId set）
    std::unordered_set<uint64_t> breakpoints;

    // 上次执行结果（用于面板展示）
    RTExecutionResult           lastExecutionResult;
    char                        execLogFilter[128] = {};  // 日志过滤输入框
    std::string                 execLogCachedFilter;      // 上次构建 executionLogText 时的过滤条件

    // 持久 Runner
    RTBlueprintRunner           persistentRunner;

    // 节点编辑器 UI 状态（原先的 static 变量）
    ed::NodeId contextNodeId      = 0;
    ed::LinkId contextLinkId      = 0;
    ed::PinId  contextPinId       = 0;
    bool       createNewNode      = false;
    ed::PinId  newNodeLinkPinId   = 0;   // 右键创建节点时的起始引脚 PinId
    ed::PinId  newLinkPinId       = 0;   // 当前拖拽连线的起始引脚 PinId
    // 每帧通过 FindPin(id) 刷新，不直接存储 Pin*（会在 SyncFunctionPinsToNodes 后失效）
    Pin*       newNodeLinkPin     = nullptr;  // 由 newNodeLinkPinId 驱动，勿直接赋值
    Pin*       newLinkPin         = nullptr;  // 由 newLinkPinId 驱动，勿直接赋值

    // 变量拖拽到画布的待处理状态
    bool           pendingVarDrop     = false;  // 有待处理的拖拽放置
    VarDragPayload pendingVarPayload  = {};      // 拖拽的变量信息
    ImVec2         pendingVarDropPos  = {};      // 放置时的屏幕坐标

    // 节点拖拽位置 Undo 状态（在位置变化检测处使用）
    bool           nodeDragUndoPushed = false;  // 本次拖拽是否已 push 过快照

    // Comment 节点内联编辑状态
    ed::NodeId     editingCommentId   = 0;         // 当前正在编辑标题的 Comment 节点 ID
    char           commentEditBuf[256] = {};        // 编辑缓冲区

    // 每帧缓存的双击节点 ID（GetDoubleClickedNode 是一次性消费 API，需提前读取共享）
    ed::NodeId     lastDoubleClickedNode = 0;

    // 内联编辑控件字符串缓冲区（按 PinId 索引，文档切换时自然隔离）
    std::unordered_map<uintptr_t, std::array<char, 128>> pinStringBuffers;
    std::unordered_map<uintptr_t, std::array<char, 128>> pinObjectBuffers;

    // 节点过滤器（每个文档独立，切换文档后保留各自的过滤状态）
    char nodeFilterBuf[128] = {};

    // 变量面板新建弹窗状态（每个文档独立，避免多文档 static 污染）
    bool varAddPopupOpen   = false;
    char varNewName[64]    = {};
    int  varNewTypeIdx     = 1;        // 默认 Boolean

    // 变量列表（蓝图级别的变量定义，可在 Get/Set Variable 节点中引用）
    std::vector<RTVariableDefinition> variables;

    // 本蓝图定义的函数
    std::vector<RTFunctionDefinition> functions;

    // Functions 面板状态
    int  selectedFuncIdx = -1;           // 当前选中的函数索引（-1 = 无选中）

    // ---- Undo / Redo ----
    std::deque<UndoState> undoStack;   // 最多 kMaxUndoSteps 步
    std::deque<UndoState> redoStack;

    // 恢复快照后，延迟一帧通过 ed::SetNodePosition 恢复节点位置
    bool                                       pendingRestorePositions = false;
    std::map<ed::NodeId, ImVec2, NodeIdLess>   pendingRestoreNodePos;

    // ---- 编辑器侧哈希索引（O(1) 查找加速） ----
    // 调用 rebuildEditorIndices() 重建；数据变更后调用 invalidateEditorIndices()
    mutable std::unordered_map<uint64_t, size_t>      nodeIdIndex;    // NodeId → nodes[] 下标
    mutable std::unordered_map<uint64_t, size_t>      linkIdIndex;    // LinkId → links[] 下标

    // PinId → {nodeIdx, pinIdx, isOutput}，间接寻址避免 deque realloc 后裸指针失效
    struct PinLocation { size_t nodeIdx; size_t pinIdx; bool isOutput; };
    mutable std::unordered_map<uint64_t, PinLocation>  pinIdIndex;     // PinId → PinLocation
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
            for (size_t j = 0; j < nodes[i].Inputs.size(); ++j)
            {
                uint64_t pid = reinterpret_cast<uintptr_t>(nodes[i].Inputs[j].ID.AsPointer());
                pinIdIndex[pid] = { i, j, false };
            }
            for (size_t j = 0; j < nodes[i].Outputs.size(); ++j)
            {
                uint64_t pid = reinterpret_cast<uintptr_t>(nodes[i].Outputs[j].ID.AsPointer());
                pinIdIndex[pid] = { i, j, true };
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
            return untitledName.empty() ? "New" : untitledName;
        size_t lastSlash = filePath.find_last_of("/\\");
        std::string name = (lastSlash != std::string::npos) ? filePath.substr(lastSlash + 1) : filePath;
        size_t lastDot = name.find_last_of('.');
        if (lastDot != std::string::npos)
            name = name.substr(0, lastDot);
        return name;
    }

    // 标签页标题（含修改标记 + 蓝图类型徽章）
    std::string GetTabTitle() const
    {
        std::string title = GetTabName();
        if (blueprintClass == RTBlueprintClass::FunctionLibrary)
            title = "[Lib] " + title;
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

    // Create Node 菜单缓存（性能优化）
    // ------------------------------------------------------------------
    struct CategoryMenuNode {
        std::map<std::string, CategoryMenuNode>  children;
        std::vector<const RTNodeDef*>            directNodes;
    };
    // 缓存的分类树
    std::map<std::string, CategoryMenuNode>   m_CachedRootChildren;
    std::vector<std::string>                  m_CachedRootOrder;
    std::unordered_map<std::string, std::string> m_CachedCatIdToName;
    size_t                                    m_CachedDefCount = 0;  // 用于检测 registry 变化
    // 缓存的搜索结果
    std::string                               m_CachedSearchFilter;
    std::vector<const RTNodeDef*>             m_CachedSearchResults;
    // 左侧 Nodes 面板的 tolower 名字缓存
    // key = node.ID pointer, value = { name_lower, defId_lower }
    struct NodeFilterCache {
        std::string nameLower;
        std::string defIdLower;
    };
    std::unordered_map<uintptr_t, NodeFilterCache> m_NodeFilterCache;
    std::string                               m_LastNodeFilter;

    // ------------------------------------------------------------------
    // 缩放条（ZoomBar）
    // ------------------------------------------------------------------
    void  DrawZoomBar(ImVec2 editorMin, ImVec2 editorMax);
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
    // 工程内保存名称对话框（有工程时替代系统 Dialog）
    // 用户在编辑器窗口内输入相对于 assets/ 的路径（支持子目录），
    // 确认后自动创建目录并保存蓝图文件。
    // ------------------------------------------------------------------
    struct SaveNameDialogState
    {
        bool        open       = false;   // 是否显示
        bool        isNew      = false;   // true=新建, false=另存为
        RTBlueprintClass bpClass = RTBlueprintClass::Actor;
        char        inputBuf[256] = {};   // 用户输入的相对路径（不含扩展名）
        std::string errorMsg;             // 路径校验错误提示（空=无错误）
    };
    SaveNameDialogState m_SaveNameDialog;
    void  OpenSaveNameDialog(bool isNew, RTBlueprintClass bpClass = RTBlueprintClass::Actor);
    void  DrawSaveNameDialog();           // 每帧在主 UI 中调用
    // 从 inputBuf 解析出完整绝对路径（含扩展名），空串表示校验失败
    std::string ResolveSaveDialogPath() const;

    // ------------------------------------------------------------------
    // 最近打开文件（持久化到磁盘）
    // ------------------------------------------------------------------
    static const int MaxRecentFiles = 10;
    std::vector<std::string> m_RecentFiles;
    void  AddRecentFile(const std::string& path);
    void  DrawRecentFilesMenu();
    void  SaveRecentFiles();
    void  LoadRecentFiles();

    // 最近打开的工程
    static const int MaxRecentProjects = 8;
    std::vector<std::string> m_RecentProjects;
    void  AddRecentProject(const std::string& path);
    void  DrawRecentProjectsMenu();
    void  SaveRecentProjects();
    void  LoadRecentProjects();

    // ------------------------------------------------------------------
    // 节点对齐（Align Selected Nodes）
    // ------------------------------------------------------------------
    enum class AlignMode { Left, Right, Top, Bottom, CenterH, CenterV };
    void  AlignSelectedNodes(AlignMode mode);

    // ------------------------------------------------------------------
    // 工程系统（*.bp.proj）
    // ------------------------------------------------------------------
    BpProject   m_Project;                          // 当前工程（filePath 为空 = 无工程）
    void        NewProject();                        // 新建工程（弹框输入名称）
    void        OpenProject();                       // 打开工程（文件对话框）
    void        SaveProject();                       // 保存工程
    void        SaveProjectAs();                     // 另存为
    void        CloseProject();                      // 关闭工程
    void        AddCurrentDocToProject();            // 将当前文档加入工程列表
    void        SyncProjectLibrariesToRegistry();    // 按工程 libraries 刷新节点定义注册表
    void        DrawProjectPanel();                  // 工程面板（左侧栏）

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

    // Undo / Redo
    void  PushUndoState();           // 在修改操作之前调用，保存当前快照
    void  Undo();
    void  Redo();
    bool  CanUndo();
    bool  CanRedo();

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
    void    DrawNodeLibraryPanel();                    // 节点库面板（可折叠分类 + 拖拽）
    void    DrawDetailsPanel();                        // Details 面板（选中节点的属性检查器）
    void    DrawFunctionDetailsPanel(RTFunctionDefinition& func);  // 函数参数编辑面板
    void    SyncFunctionPinsToNodes(const RTFunctionDefinition& func);  // 同步函数参数到画布节点引脚
    void    DrawExecutionPanel();                      // 底部执行输出面板（嵌入式）
    void    DrawWatchPanel(float paneWidth);           // Watch 面板（运行时变量/引脚值监控）
    void    DrawTimerPanel();                          // 计时器监控浮动面板

    // ------------------------------------------------------------------
    // 拆分自 EditorUI.cpp 的子渲染函数
    // NodeRenderer.cpp / LinkRenderer.cpp / ContextMenus.cpp
    // ------------------------------------------------------------------
    void    DrawNodes(util::BlueprintNodeBuilder& builder);   // 所有节点渲染（Blueprint/Simple/Tree/Houdini/Comment）
    void    DrawLinks();                                       // 链接渲染 + BeginCreate/BeginDelete
    void    DrawContextMenus(ImVec2 openPopupPosition,         // 四种右键菜单
                             ed::NodeId& contextNodeId,
                             ed::PinId&  contextPinId,
                             ed::LinkId& contextLinkId,
                             bool&       createNewNode,
                             Pin*&       newNodeLinkPin);

    // ------------------------------------------------------------------
    // 文件操作
    // ------------------------------------------------------------------
    void    NewFile(RTBlueprintClass bpClass = RTBlueprintClass::Actor); // 新建蓝图（新标签页）
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
    bool                 m_ShowLibraryWindow = false;     // 节点库浮动面板可见

    // VSCode 风格布局尺寸（可拖拽调整）
    float                m_LeftPanelWidth  = 320.0f;
    float                m_BottomPanelHeight = 200.0f;

    // 节点定义注册表 & 处理器注册表（全局共享）
    RTNodeRegistry                                          m_NodeRegistry;
    std::unordered_map<std::string, RTNodeHandler>          m_HandlerRegistry;

    // Default handler
    RTNodeHandler m_DefaultHandler;

};
