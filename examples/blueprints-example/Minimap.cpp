// Minimap.cpp -- 画布小地图（缩略图导航）
#include "BlueprintEditor.h"

// ============================================================================
// 小地图绘制
// ============================================================================

void BlueprintEditor::DrawMinimap(ImVec2 editorMin, ImVec2 editorMax)
{
    if (!m_ShowMinimap) return;
    if (!ActiveDoc()) return;
    if (m_Nodes.empty()) return;

    auto drawList = ImGui::GetWindowDrawList();

    // 小地图位置：编辑器区域右下角
    float mapSize = m_MinimapSize;
    float padding = 10.0f;
    ImVec2 mapMin(editorMax.x - mapSize - padding, editorMax.y - mapSize - padding);
    ImVec2 mapMax(editorMax.x - padding, editorMax.y - padding);

    // 计算所有节点的边界框（画布坐标）
    float canvasMinX = 1e9f, canvasMinY = 1e9f;
    float canvasMaxX = -1e9f, canvasMaxY = -1e9f;

    for (const auto& node : m_Nodes)
    {
        ImVec2 pos = ed::GetNodePosition(node.ID);
        ImVec2 size = ed::GetNodeSize(node.ID);

        if (size.x <= 0 || size.y <= 0)
            continue;

        if (pos.x < canvasMinX) canvasMinX = pos.x;
        if (pos.y < canvasMinY) canvasMinY = pos.y;
        if (pos.x + size.x > canvasMaxX) canvasMaxX = pos.x + size.x;
        if (pos.y + size.y > canvasMaxY) canvasMaxY = pos.y + size.y;
    }

    // 扩展边界留出一些余量
    float marginCanvas = 100.0f;
    canvasMinX -= marginCanvas;
    canvasMinY -= marginCanvas;
    canvasMaxX += marginCanvas;
    canvasMaxY += marginCanvas;

    float canvasW = canvasMaxX - canvasMinX;
    float canvasH = canvasMaxY - canvasMinY;
    if (canvasW <= 0 || canvasH <= 0) return;

    // 计算缩放比例，保持纵横比
    float mapW = mapMax.x - mapMin.x;
    float mapH = mapMax.y - mapMin.y;
    float scaleX = mapW / canvasW;
    float scaleY = mapH / canvasH;
    float scale = (scaleX < scaleY) ? scaleX : scaleY;

    // 居中偏移
    float drawnW = canvasW * scale;
    float drawnH = canvasH * scale;
    float offsetX = mapMin.x + (mapW - drawnW) * 0.5f;
    float offsetY = mapMin.y + (mapH - drawnH) * 0.5f;

    // 画布坐标 -> 小地图屏幕坐标
    auto canvasToMap = [&](float cx, float cy) -> ImVec2
    {
        return ImVec2(
            offsetX + (cx - canvasMinX) * scale,
            offsetY + (cy - canvasMinY) * scale
        );
    };

    // 绘制背景
    drawList->AddRectFilled(mapMin, mapMax, IM_COL32(20, 20, 25, 200), 4.0f);
    drawList->AddRect(mapMin, mapMax, IM_COL32(80, 80, 90, 200), 4.0f);

    // 收集选中节点
    int selCount = ed::GetSelectedObjectCount();
    std::vector<ed::NodeId> selectedNodeIds;
    if (selCount > 0)
    {
        selectedNodeIds.resize(selCount);
        int nodeCount = ed::GetSelectedNodes(selectedNodeIds.data(), selCount);
        selectedNodeIds.resize(nodeCount);
    }

    // 绘制链接（简化为直线）
    for (const auto& link : m_Links)
    {
        auto* startPin = FindPin(link.StartPinID);
        auto* endPin = FindPin(link.EndPinID);
        if (!startPin || !endPin || !startPin->Node || !endPin->Node)
            continue;

        ImVec2 startPos = ed::GetNodePosition(startPin->Node->ID);
        ImVec2 startSize = ed::GetNodeSize(startPin->Node->ID);
        ImVec2 endPos = ed::GetNodePosition(endPin->Node->ID);
        ImVec2 endSize = ed::GetNodeSize(endPin->Node->ID);

        ImVec2 p1 = canvasToMap(startPos.x + startSize.x * 0.5f, startPos.y + startSize.y * 0.5f);
        ImVec2 p2 = canvasToMap(endPos.x + endSize.x * 0.5f, endPos.y + endSize.y * 0.5f);

        drawList->AddLine(p1, p2, IM_COL32(100, 100, 120, 80), 1.0f);
    }

    // 绘制节点矩形
    for (const auto& node : m_Nodes)
    {
        ImVec2 pos = ed::GetNodePosition(node.ID);
        ImVec2 size = ed::GetNodeSize(node.ID);
        if (size.x <= 0 || size.y <= 0) continue;

        ImVec2 rectMin = canvasToMap(pos.x, pos.y);
        ImVec2 rectMax = canvasToMap(pos.x + size.x, pos.y + size.y);

        // 确保最小可见尺寸
        if (rectMax.x - rectMin.x < 3.0f) rectMax.x = rectMin.x + 3.0f;
        if (rectMax.y - rectMin.y < 2.0f) rectMax.y = rectMin.y + 2.0f;

        // 颜色
        ImU32 fillColor;
        if (node.Type == NodeType::Comment)
        {
            fillColor = IM_COL32(255, 255, 255, 30);
        }
        else
        {
            ImVec4 col = node.Color.Value;
            fillColor = IM_COL32(
                static_cast<int>(col.x * 255),
                static_cast<int>(col.y * 255),
                static_cast<int>(col.z * 255),
                140);
        }

        drawList->AddRectFilled(rectMin, rectMax, fillColor, 1.0f);

        // 选中节点高亮边框
        bool isSelected = false;
        for (const auto& selId : selectedNodeIds)
        {
            if (selId == node.ID) { isSelected = true; break; }
        }
        if (isSelected)
            drawList->AddRect(rectMin, rectMax, IM_COL32(255, 200, 50, 220), 1.0f, 0, 1.5f);
    }

    // 绘制当前视口矩形（表示屏幕上可见的画布区域）
    // 使用 ed::ScreenToCanvas 从编辑器区域的四角反推画布坐标
    ImVec2 viewTL_canvas = ed::ScreenToCanvas(editorMin);
    ImVec2 viewBR_canvas = ed::ScreenToCanvas(editorMax);

    ImVec2 viewTL_map = canvasToMap(viewTL_canvas.x, viewTL_canvas.y);
    ImVec2 viewBR_map = canvasToMap(viewBR_canvas.x, viewBR_canvas.y);

    // 裁剪到小地图范围
    if (viewTL_map.x < mapMin.x) viewTL_map.x = mapMin.x;
    if (viewTL_map.y < mapMin.y) viewTL_map.y = mapMin.y;
    if (viewBR_map.x > mapMax.x) viewBR_map.x = mapMax.x;
    if (viewBR_map.y > mapMax.y) viewBR_map.y = mapMax.y;

    drawList->AddRectFilled(viewTL_map, viewBR_map, IM_COL32(255, 255, 255, 20));
    drawList->AddRect(viewTL_map, viewBR_map, IM_COL32(255, 255, 255, 120), 0.0f, 0, 1.5f);

    // 小地图标题
    drawList->AddText(ImVec2(mapMin.x + 4, mapMin.y + 2),
                      IM_COL32(160, 160, 180, 180), "Minimap");

    // 点击小地图导航到对应画布位置
    ImVec2 mousePos = ImGui::GetMousePos();
    if (mousePos.x >= mapMin.x && mousePos.x <= mapMax.x &&
        mousePos.y >= mapMin.y && mousePos.y <= mapMax.y)
    {
        if (ImGui::IsMouseClicked(ImGuiMouseButton_Left))
        {
            // 屏幕坐标 -> 画布坐标
            float cx = canvasMinX + (mousePos.x - offsetX) / scale;
            float cy = canvasMinY + (mousePos.y - offsetY) / scale;
            ed::NavigateToContent();  // 简单实现：点击时居中到所有内容
        }
    }
}
