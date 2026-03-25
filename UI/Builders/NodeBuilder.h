// UI/Builders/NodeBuilder.h - 节点构建器
// 提供节点构建的辅助工具

#pragma once

#include <imgui.h>
#include <imgui_internal.h>
#include <string>
#include <functional>
#include <vector>

namespace NodeEditor {
namespace UI {

// ============================================================================
// 节点构建器
// ============================================================================

class NodeBuilder
{
public:
    NodeBuilder();
    ~NodeBuilder();
    
    // 开始构建节点
    void Begin(uint64_t nodeId, const ImVec2& position = ImVec2(0, 0));
    
    // 结束构建节点
    void End();
    
    // 开始构建引脚
    void BeginPin(uint64_t pinId, bool isInput, int pinType = 0);
    
    // 结束构建引脚
    void EndPin();
    
    // 设置引脚位置（自动布局）
    void SetPinPosition(const ImVec2& position);
    
    // 设置节点标题
    void SetTitle(const std::string& title);
    
    // 设置节点颜色
    void SetColor(ImU32 color);
    
    // 设置节点背景颜色
    void SetBackgroundColor(ImU32 color);
    
    // 设置节点尺寸
    void SetSize(const ImVec2& size);
    
    // 获取当前节点ID
    uint64_t GetCurrentNodeId() const { return m_CurrentNodeId; }
    
    // 获取当前引脚ID
    uint64_t GetCurrentPinId() const { return m_CurrentPinId; }
    
    // 获取节点矩形区域
    ImRect GetNodeRect() const { return m_NodeRect; }
    
    // 获取引脚矩形区域
    ImRect GetPinRect() const { return m_PinRect; }
    
private:
    uint64_t            m_CurrentNodeId;
    uint64_t            m_CurrentPinId;
    bool                m_PinIsInput;
    std::string         m_Title;
    ImU32               m_NodeColor;
    ImU32               m_BackgroundColor;
    ImVec2              m_NodePos;
    ImVec2              m_NodeSize;
    ImRect              m_NodeRect;
    ImRect              m_PinRect;
    ImVec2              m_PinPos;
    
    ImGuiWindow*        m_Window;
    ImDrawList*         m_DrawList;
    int                 m_CurrentChannel;
    ImDrawListSplitter  m_Splitter;
    
    // 内部状态
    bool                m_IsInNode;
    bool                m_IsInPin;
};

// ============================================================================
// 简化版节点构建器
// ============================================================================

class SimpleNodeBuilder
{
public:
    // 节点布局配置
    struct Config
    {
        float       titleHeight = 24.0f;
        float       pinRadius = 4.0f;
        float       pinSpacing = 8.0f;
        float       paddingX = 8.0f;
        float       paddingY = 4.0f;
        ImU32       titleBgColor = IM_COL32(60, 60, 70, 255);
        ImU32       nodeBgColor = IM_COL32(40, 40, 50, 255);
        ImU32       borderColor = IM_COL32(100, 100, 100, 255);
        ImU32       pinColor = IM_COL32(150, 150, 150, 255);
        ImU32       pinHoverColor = IM_COL32(200, 200, 200, 255);
    };
    
    // Two constructors: GCC rejects a defaulted argument of type Config{} inside
    // the enclosing class body when Config has NSDMI members (CWG 1264 / GCC bug).
    // The no-arg overload delegates to the explicit one with a default-constructed Config.
    SimpleNodeBuilder();
    explicit SimpleNodeBuilder(const Config& config);
    
    // 开始构建节点
    void Begin(uint64_t nodeId, const std::string& title, const ImVec2& position);
    
    // 结束构建节点
    void End();
    
    // 添加输入引脚
    void AddInputPin(uint64_t pinId, const std::string& label, int pinType = 0);
    
    // 添加输出引脚
    void AddOutputPin(uint64_t pinId, const std::string& label, int pinType = 0);
    
    // 添加自定义内容
    void AddContent(const std::function<void()>& contentFunc);
    
    // 设置节点颜色
    void SetColor(ImU32 color);
    
    // 获取节点矩形区域
    ImRect GetNodeRect() const { return m_NodeRect; }
    
private:
    struct PinInfo
    {
        uint64_t    id;
        std::string label;
        bool        isInput;
        int         type;
        ImVec2      position;
    };
    
    Config              m_Config;
    uint64_t            m_CurrentNodeId;
    std::string         m_CurrentTitle;
    ImVec2              m_CurrentPosition;
    std::vector<PinInfo> m_InputPins;
    std::vector<PinInfo> m_OutputPins;
    std::function<void()> m_ContentFunc;
    ImU32               m_NodeColor;
    ImRect              m_NodeRect;
    
    ImDrawList*         m_DrawList;
    
    void                calculateLayout();
    void                drawNode();
};

} // namespace UI
} // namespace NodeEditor
