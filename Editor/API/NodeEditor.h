// Editor/API/NodeEditor.h - 节点编辑器公共 API
// 提供编辑器的主要公共接口

#pragma once

#include <imgui.h>
#include <functional>
#include <string>
#include <vector>
#include "../../Runtime/Types.h"
#include "../../Runtime/BlueprintData.h"

namespace NodeEditor {

// ============================================================================
// 前向声明
// ============================================================================

class EditorContext;
struct Config;
struct Style;

// ============================================================================
// ID 类型（兼容原有 API）
// ============================================================================

using NodeId = Runtime::NodeId;
using PinId = Runtime::PinId;
using LinkId = Runtime::LinkId;

using PinKind = Runtime::PinKind;
using NodeType = Runtime::NodeType;

// ============================================================================
// 配置结构
// ============================================================================

struct Config
{
    using SaveSettingsCallback = std::function<bool(const char* data, size_t size)>;
    using LoadSettingsCallback = std::function<size_t(char* data)>;
    
    const char*             SettingsFile = nullptr;
    SaveSettingsCallback    SaveSettings;
    LoadSettingsCallback    LoadSettings;
    
    int                     DragButtonIndex = 0;          // 拖拽按钮索引
    int                     SelectButtonIndex = 1;        // 选择按钮索引
    int                     ContextMenuButtonIndex = 1;   // 右键菜单按钮索引
    
    // 新增：蓝图数据回调
    std::function<void(const Runtime::BlueprintData&)> OnBlueprintChanged;
    std::function<Runtime::NodeDefinition(const std::string& nodeId)> GetNodeDefinition;
};

// ============================================================================
// 样式结构
// ============================================================================

struct Style
{
    ImVec2  NodePadding = ImVec2(8, 8);
    ImVec2  NodeSpacing = ImVec2(12, 12);
    float   NodeRounding = 4.0f;
    float   NodeBorderWidth = 1.0f;
    
    ImVec2  PinRadius = ImVec2(4, 4);
    float   PinRounding = 4.0f;
    float   PinBorderWidth = 1.0f;
    float   PinConeScale = 0.4f;
    float   PinStrength = 100.0f;
    
    float   LinkStrength = 100.0f;
    float   LinkRounding = 4.0f;
    float   LinkThickness = 2.0f;
    
    float   GroupRounding = 4.0f;
    float   GroupBorderWidth = 1.0f;
    
    ImVec2  NodeBorderHoverPadding = ImVec2(4, 4);
    ImVec2  NodeBorderSelectionPadding = ImVec2(4, 4);
    
    // 颜色
    ImU32   NodeBackgroundColor = IM_COL32(40, 40, 50, 255);
    ImU32   NodeBorderColor = IM_COL32(100, 100, 100, 255);
    ImU32   NodeHoveredBorderColor = IM_COL32(150, 150, 150, 255);
    ImU32   NodeSelectedBorderColor = IM_COL32(200, 200, 200, 255);
    
    ImU32   PinColor = IM_COL32(150, 150, 150, 255);
    ImU32   PinHoveredColor = IM_COL32(200, 200, 200, 255);
    ImU32   PinConnectedColor = IM_COL32(100, 200, 100, 255);
    
    ImU32   LinkColor = IM_COL32(150, 150, 150, 255);
    ImU32   LinkHoveredColor = IM_COL32(200, 200, 200, 255);
    ImU32   LinkSelectedColor = IM_COL32(200, 200, 200, 255);
    
    ImU32   GroupBackgroundColor = IM_COL32(30, 30, 40, 100);
    ImU32   GroupBorderColor = IM_COL32(100, 100, 100, 200);
    
    ImU32   GridColor = IM_COL32(200, 200, 200, 40);
    float   GridSize = 32.0f;
    
    ImU32   SelectionRectColor = IM_COL32(100, 150, 200, 100);
};

// ============================================================================
// 编辑器生命周期
// ============================================================================

// 创建编辑器
EditorContext* CreateEditor(const Config* config = nullptr);

// 销毁编辑器
void DestroyEditor(EditorContext* editor);

// 设置当前编辑器
void SetCurrentEditor(EditorContext* editor);

// 获取当前编辑器
EditorContext* GetCurrentEditor();

// 获取配置
const Config& GetConfig(EditorContext* editor = nullptr);

// ============================================================================
// 主循环
// ============================================================================

// 开始编辑器帧
void Begin(const char* id, const ImVec2& size = ImVec2(0, 0));

// 结束编辑器帧
void End();

// ============================================================================
// 节点操作
// ============================================================================

// 开始构建节点
void BeginNode(NodeId id);

// 结束构建节点
void EndNode();

// 开始构建引脚
void BeginPin(PinId id, PinKind kind);

// 结束构建引脚
void EndPin();

// 设置引脚位置（用于精确控制）
void PinRect(const ImVec2& min, const ImVec2& max);

// 组节点
void Group(const ImVec2& size);

// ============================================================================
// 链接操作
// ============================================================================

// 创建链接
void Link(LinkId id, PinId startPinId, PinId endPinId);

// 流动画（用于动画效果）
void Flow(LinkId linkId, float speed = 1.0f);

// ============================================================================
// 交互查询
// ============================================================================

// 开始创建链接/节点
bool BeginCreate();

// 结束创建
void EndCreate();

// 查询新链接
bool QueryNewLink(PinId* startPinId, PinId* endPinId);

// 查询新节点
bool QueryNewNode(PinId* pinId);

// 接受创建
void AcceptNewItem();

// 拒绝创建
void RejectNewItem();

// 开始删除
bool BeginDelete();

// 结束删除
void EndDelete();

// 查询删除的链接
bool QueryDeletedLink(LinkId* linkId, PinId* startPinId = nullptr, PinId* endPinId = nullptr);

// 查询删除的节点
bool QueryDeletedNode(NodeId* nodeId);

// 接受删除
void AcceptDeletedItem();

// 拒绝删除
void RejectDeletedItem();

// ============================================================================
// 选择操作
// ============================================================================

// 获取选中的节点
void GetSelectedNodes(std::vector<NodeId>& nodes);

// 获取选中的链接
void GetSelectedLinks(std::vector<LinkId>& links);

// 选择节点
void SelectNode(NodeId nodeId, bool append = false);

// 取消选择节点
void DeselectNode(NodeId nodeId);

// 清除选择
void ClearSelection();

// 全选
void SelectAll();

// ============================================================================
// 导航操作
// ============================================================================

// 导航到内容
void NavigateToContent(float duration = -1);

// 导航到选择
void NavigateToSelection(float duration = -1);

// 导航到节点
void NavigateToNode(NodeId nodeId, float duration = -1);

// 屏幕坐标转画布坐标
ImVec2 ScreenToCanvas(const ImVec2& screenPos);

// 画布坐标转屏幕坐标
ImVec2 CanvasToScreen(const ImVec2& canvasPos);

// 获取画布位置
ImVec2 GetCanvasPosition();

// 获取画布缩放
float GetCanvasScale();

// 设置画布位置
void SetCanvasPosition(const ImVec2& position);

// 设置画布缩放
void SetCanvasScale(float scale);

// ============================================================================
// 布局辅助
// ============================================================================

// 自动布局节点
void LayoutNodes(const ImVec2& direction = ImVec2(1, 0), float spacing = 100.0f);

// 获取节点位置
ImVec2 GetNodePosition(NodeId nodeId);

// 设置节点位置
void SetNodePosition(NodeId nodeId, const ImVec2& position);

// 获取节点尺寸
ImVec2 GetNodeSize(NodeId nodeId);

// ============================================================================
// 蓝图数据接口（新增）
// ============================================================================

// 导出蓝图数据
Runtime::BlueprintData ExportBlueprint();

// 导入蓝图数据
void ImportBlueprint(const Runtime::BlueprintData& data);

// 清空蓝图
void ClearBlueprint();

// 获取节点数量
int GetNodeCount();

// 获取链接数量
int GetLinkCount();

// 检查链接是否存在
bool LinkExists(PinId startPinId, PinId endPinId);

// 获取连接到引脚的链接
std::vector<LinkId> GetLinksFromPin(PinId pinId);

// ============================================================================
// 样式操作
// ============================================================================

// 获取样式
Style& GetStyle();

// 推送样式颜色
void PushStyleColor(int styleColorIndex, ImU32 color);

// 弹出样式颜色
void PopStyleColor(int count = 1);

// 推送样式变量
void PushStyleVar(int styleVarIndex, float value);

// 弹出样式变量
void PopStyleVar(int count = 1);

} // namespace NodeEditor
