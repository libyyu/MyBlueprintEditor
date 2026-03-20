// UI/Builders/NodeBuilder.cpp - 节点构建器实现

#include "NodeBuilder.h"
#include <imgui.h>
#include <imgui_internal.h>

namespace NodeEditor {
namespace UI {

// ============================================================================
// NodeBuilder 实现
// ============================================================================

NodeBuilder::NodeBuilder()
    : m_CurrentNodeId(0)
    , m_CurrentPinId(0)
    , m_PinIsInput(false)
    , m_NodeColor(IM_COL32(100, 100, 100, 255))
    , m_BackgroundColor(IM_COL32(40, 40, 50, 255))
    , m_Window(nullptr)
    , m_DrawList(nullptr)
    , m_CurrentChannel(0)
    , m_IsInNode(false)
    , m_IsInPin(false)
{
}

NodeBuilder::~NodeBuilder()
{
    if (m_IsInNode)
    {
        End();
    }
}

void NodeBuilder::Begin(uint64_t nodeId, const ImVec2& position)
{
    IM_ASSERT(!m_IsInNode && "Already in node");
    
    m_CurrentNodeId = nodeId;
    m_NodePos = position;
    m_Title.clear();
    m_NodeSize = ImVec2(0, 0);
    m_IsInNode = true;
    
    m_Window = ImGui::GetCurrentWindow();
    m_DrawList = m_Window->DrawList;
    
    // 设置光标位置
    ImGui::SetCursorScreenPos(position);
    
    // 开始分组
    ImGui::BeginGroup();
    
    // 保存初始位置
    m_NodeRect.Min = position;
}

void NodeBuilder::End()
{
    IM_ASSERT(m_IsInNode && "Not in node");
    
    ImGui::EndGroup();
    
    // 计算节点尺寸
    m_NodeRect.Max = ImGui::GetItemRectMax();
    m_NodeSize = m_NodeRect.GetSize();
    
    // 绘制节点背景
    m_DrawList->AddRectFilled(
        m_NodeRect.Min,
        m_NodeRect.Max,
        m_BackgroundColor,
        4.0f
    );
    
    // 绘制节点边框
    m_DrawList->AddRect(
        m_NodeRect.Min,
        m_NodeRect.Max,
        m_NodeColor,
        4.0f,
        0,
        2.0f
    );
    
    m_IsInNode = false;
    m_CurrentNodeId = 0;
}

void NodeBuilder::BeginPin(uint64_t pinId, bool isInput, int pinType)
{
    IM_ASSERT(m_IsInNode && !m_IsInPin && "Not in node or already in pin");
    
    m_CurrentPinId = pinId;
    m_PinIsInput = isInput;
    m_IsInPin = true;
    
    // 保存当前光标位置
    m_PinPos = ImGui::GetCursorScreenPos();
    
    // 创建一个小的不可见按钮用于交互
    ImGui::InvisibleButton("##pin", ImVec2(10, 10));
    
    // 保存引脚区域
    m_PinRect.Min = m_PinPos - ImVec2(5, 5);
    m_PinRect.Max = m_PinPos + ImVec2(5, 5);
}

void NodeBuilder::EndPin()
{
    IM_ASSERT(m_IsInPin && "Not in pin");
    
    // 绘制引脚圆点
    ImU32 pinColor = ImGui::IsItemHovered() ? 
        IM_COL32(200, 200, 200, 255) : 
        IM_COL32(150, 150, 150, 255);
    
    m_DrawList->AddCircleFilled(
        m_PinPos,
        4.0f,
        pinColor
    );
    
    m_IsInPin = false;
    m_CurrentPinId = 0;
}

void NodeBuilder::SetPinPosition(const ImVec2& position)
{
    m_PinPos = position;
    m_PinRect.Min = position - ImVec2(5, 5);
    m_PinRect.Max = position + ImVec2(5, 5);
}

void NodeBuilder::SetTitle(const std::string& title)
{
    m_Title = title;
    
    // 绘制标题
    ImGui::SetCursorScreenPos(m_NodePos + ImVec2(8, 4));
    ImGui::Text("%s", title.c_str());
}

void NodeBuilder::SetColor(ImU32 color)
{
    m_NodeColor = color;
}

void NodeBuilder::SetBackgroundColor(ImU32 color)
{
    m_BackgroundColor = color;
}

void NodeBuilder::SetSize(const ImVec2& size)
{
    m_NodeSize = size;
}

// ============================================================================
// SimpleNodeBuilder 实现
// ============================================================================

SimpleNodeBuilder::SimpleNodeBuilder(const Config& config)
    : m_Config(config)
    , m_CurrentNodeId(0)
    , m_NodeColor(config.titleBgColor)
    , m_DrawList(nullptr)
{
}

void SimpleNodeBuilder::Begin(uint64_t nodeId, const std::string& title, const ImVec2& position)
{
    m_CurrentNodeId = nodeId;
    m_CurrentTitle = title;
    m_CurrentPosition = position;
    m_InputPins.clear();
    m_OutputPins.clear();
    m_ContentFunc = nullptr;
    m_DrawList = ImGui::GetWindowDrawList();
}

void SimpleNodeBuilder::End()
{
    calculateLayout();
    drawNode();
    
    m_CurrentNodeId = 0;
    m_InputPins.clear();
    m_OutputPins.clear();
}

void SimpleNodeBuilder::AddInputPin(uint64_t pinId, const std::string& label, int pinType)
{
    PinInfo pin;
    pin.id = pinId;
    pin.label = label;
    pin.isInput = true;
    pin.type = pinType;
    m_InputPins.push_back(pin);
}

void SimpleNodeBuilder::AddOutputPin(uint64_t pinId, const std::string& label, int pinType)
{
    PinInfo pin;
    pin.id = pinId;
    pin.label = label;
    pin.isInput = false;
    pin.type = pinType;
    m_OutputPins.push_back(pin);
}

void SimpleNodeBuilder::AddContent(const std::function<void()>& contentFunc)
{
    m_ContentFunc = contentFunc;
}

void SimpleNodeBuilder::SetColor(ImU32 color)
{
    m_NodeColor = color;
}

void SimpleNodeBuilder::calculateLayout()
{
    // 计算节点宽度
    float maxWidth = 100.0f;
    
    // 计算标题宽度
    ImVec2 titleSize = ImGui::CalcTextSize(m_CurrentTitle.c_str());
    maxWidth = ImMax(maxWidth, titleSize.x + m_Config.paddingX * 2);
    
    // 计算引脚宽度
    for (const auto& pin : m_InputPins)
    {
        ImVec2 labelSize = ImGui::CalcTextSize(pin.label.c_str());
        maxWidth = ImMax(maxWidth, labelSize.x + m_Config.pinRadius * 4 + m_Config.paddingX * 2);
    }
    
    for (const auto& pin : m_OutputPins)
    {
        ImVec2 labelSize = ImGui::CalcTextSize(pin.label.c_str());
        maxWidth = ImMax(maxWidth, labelSize.x + m_Config.pinRadius * 4 + m_Config.paddingX * 2);
    }
    
    // 计算节点高度
    float height = m_Config.titleHeight;
    height += ImMax(m_InputPins.size(), m_OutputPins.size()) * (m_Config.pinRadius * 2 + m_Config.pinSpacing);
    height += m_Config.paddingY * 2;
    
    m_NodeRect.Min = m_CurrentPosition;
    m_NodeRect.Max = m_CurrentPosition + ImVec2(maxWidth, height);
}

void SimpleNodeBuilder::drawNode()
{
    // 绘制背景
    m_DrawList->AddRectFilled(
        m_NodeRect.Min,
        m_NodeRect.Max,
        m_Config.nodeBgColor,
        4.0f
    );
    
    // 绘制标题栏
    ImVec2 titleMin = m_NodeRect.Min;
    ImVec2 titleMax = ImVec2(m_NodeRect.Max.x, m_NodeRect.Min.y + m_Config.titleHeight);
    m_DrawList->AddRectFilled(titleMin, titleMax, m_NodeColor, 4.0f, ImDrawFlags_RoundCornersTop);
    
    // 绘制标题文本
    ImVec2 titleTextPos = titleMin + ImVec2(m_Config.paddingX, (m_Config.titleHeight - ImGui::GetTextLineHeight()) / 2);
    m_DrawList->AddText(titleTextPos, IM_COL32(255, 255, 255, 255), m_CurrentTitle.c_str());
    
    // 绘制边框
    m_DrawList->AddRect(
        m_NodeRect.Min,
        m_NodeRect.Max,
        m_Config.borderColor,
        4.0f,
        0,
        1.5f
    );
    
    // 绘制输入引脚
    float pinY = m_NodeRect.Min.y + m_Config.titleHeight + m_Config.paddingY;
    for (const auto& pin : m_InputPins)
    {
        ImVec2 pinPos(m_NodeRect.Min.x + m_Config.pinRadius * 2, pinY + m_Config.pinRadius);
        
        // 绘制引脚圆点
        m_DrawList->AddCircleFilled(pinPos, m_Config.pinRadius, m_Config.pinColor);
        
        // 绘制引脚标签
        ImVec2 labelPos = pinPos + ImVec2(m_Config.pinRadius * 2, -m_Config.pinRadius);
        m_DrawList->AddText(labelPos, IM_COL32(200, 200, 200, 255), pin.label.c_str());
        
        pinY += m_Config.pinRadius * 2 + m_Config.pinSpacing;
    }
    
    // 绘制输出引脚
    pinY = m_NodeRect.Min.y + m_Config.titleHeight + m_Config.paddingY;
    for (const auto& pin : m_OutputPins)
    {
        ImVec2 pinPos(m_NodeRect.Max.x - m_Config.pinRadius * 2, pinY + m_Config.pinRadius);
        
        // 绘制引脚圆点
        m_DrawList->AddCircleFilled(pinPos, m_Config.pinRadius, m_Config.pinColor);
        
        // 绘制引脚标签（右对齐）
        ImVec2 labelSize = ImGui::CalcTextSize(pin.label.c_str());
        ImVec2 labelPos = pinPos - ImVec2(m_Config.pinRadius * 2 + labelSize.x, m_Config.pinRadius);
        m_DrawList->AddText(labelPos, IM_COL32(200, 200, 200, 255), pin.label.c_str());
        
        pinY += m_Config.pinRadius * 2 + m_Config.pinSpacing;
    }
}

} // namespace UI
} // namespace NodeEditor
