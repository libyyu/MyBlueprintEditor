// UI/Drawing/DrawHelpers.h - 绘制辅助工具
// 提供常用的绘制辅助函数

#pragma once

#include <imgui.h>
#include <imgui_internal.h>

namespace NodeEditor {
namespace UI {
namespace Drawing {

// 绘制带箭头的贝塞尔曲线
inline void DrawBezierCurveWithArrows(
    ImDrawList* drawList,
    const ImVec2& start,
    const ImVec2& end,
    const ImVec2& control1,
    const ImVec2& control2,
    ImU32 color,
    float thickness,
    bool startArrow = false,
    bool endArrow = true,
    float arrowSize = 10.0f)
{
    // 绘制贝塞尔曲线
    drawList->AddBezierCubic(start, control1, control2, end, color, thickness);
    
    // 绘制箭头
    if (endArrow)
    {
        // 计算箭头方向（曲线终点切线方向）
        ImVec2 dir = end - control2;
        float len = sqrt(dir.x * dir.x + dir.y * dir.y);
        if (len > 0.0001f)
        {
            dir /= len;
            
            // 计算箭头的两个点
            ImVec2 perp(-dir.y, dir.x);
            ImVec2 p1 = end - dir * arrowSize + perp * arrowSize * 0.4f;
            ImVec2 p2 = end - dir * arrowSize - perp * arrowSize * 0.4f;
            
            drawList->AddTriangleFilled(end, p1, p2, color);
        }
    }
}

// 绘制节点背景
inline void DrawNodeBackground(
    ImDrawList* drawList,
    const ImVec2& pos,
    const ImVec2& size,
    ImU32 backgroundColor,
    ImU32 borderColor,
    float borderRadius = 4.0f,
    float borderThickness = 1.0f)
{
    ImRect rect(pos, pos + size);
    drawList->AddRectFilled(rect.Min, rect.Max, backgroundColor, borderRadius);
    drawList->AddRect(rect.Min, rect.Max, borderColor, borderRadius, 0, borderThickness);
}

// 绘制引脚
inline void DrawPin(
    ImDrawList* drawList,
    const ImVec2& center,
    float radius,
    ImU32 fillColor,
    ImU32 borderColor,
    float borderThickness = 1.0f)
{
    drawList->AddCircleFilled(center, radius, fillColor);
    drawList->AddCircle(center, radius, borderColor, 0, borderThickness);
}

// 绘制引脚连接点
inline void DrawPinConnector(
    ImDrawList* drawList,
    const ImVec2& center,
    float radius,
    ImU32 color,
    bool connected = false,
    bool hovered = false)
{
    float r = hovered ? radius * 1.2f : radius;
    drawList->AddCircleFilled(center, r, color);
    
    if (connected)
    {
        drawList->AddCircleFilled(center, r * 0.5f, IM_COL32(255, 255, 255, 255));
    }
}

// 绘制网格背景
inline void DrawGrid(
    ImDrawList* drawList,
    const ImVec2& canvasPos,
    const ImVec2& canvasSize,
    const ImVec2& scroll,
    float gridSize,
    ImU32 gridColor,
    float majorGridMultiple = 5.0f)
{
    ImVec2 offset = ImVec2(scroll.x - (int)scroll.x % (int)gridSize, scroll.y - (int)scroll.y % (int)gridSize);
    
    // 计算可见区域的网格线数量
    int xLines = (int)(canvasSize.x / gridSize) + 2;
    int yLines = (int)(canvasSize.y / gridSize) + 2;
    
    // 绘制垂直线
    for (int i = 0; i < xLines; ++i)
    {
        float x = canvasPos.x + offset.x + i * gridSize;
        if (x < canvasPos.x || x > canvasPos.x + canvasSize.x) continue;
        
        bool isMajor = ((int)(offset.x + i * gridSize) % (int)(gridSize * majorGridMultiple)) < gridSize;
        ImU32 color = isMajor ? gridColor : (gridColor & IM_COL32(255, 255, 255, 50));
        
        drawList->AddLine(
            ImVec2(x, canvasPos.y),
            ImVec2(x, canvasPos.y + canvasSize.y),
            color
        );
    }
    
    // 绘制水平线
    for (int i = 0; i < yLines; ++i)
    {
        float y = canvasPos.y + offset.y + i * gridSize;
        if (y < canvasPos.y || y > canvasPos.y + canvasSize.y) continue;
        
        bool isMajor = ((int)(offset.y + i * gridSize) % (int)(gridSize * majorGridMultiple)) < gridSize;
        ImU32 color = isMajor ? gridColor : (gridColor & IM_COL32(255, 255, 255, 50));
        
        drawList->AddLine(
            ImVec2(canvasPos.x, y),
            ImVec2(canvasPos.x + canvasSize.x, y),
            color
        );
    }
}

// 绘制选中高亮
inline void DrawSelectionHighlight(
    ImDrawList* drawList,
    const ImVec2& pos,
    const ImVec2& size,
    ImU32 highlightColor,
    float borderRadius = 4.0f,
    float thickness = 2.0f)
{
    ImRect rect(pos - ImVec2(thickness, thickness), pos + size + ImVec2(thickness, thickness));
    drawList->AddRect(rect.Min, rect.Max, highlightColor, borderRadius + thickness, 0, thickness);
}

} // namespace Drawing
} // namespace UI
} // namespace NodeEditor
