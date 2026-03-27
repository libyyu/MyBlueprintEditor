// Minimap.cpp -- 画布小地图（缩略图导航）
#include "BlueprintEditor.h"

// ============================================================================
// 小地图绘制
// ============================================================================

void BlueprintEditor::DrawMinimap(ImVec2 editorMin, ImVec2 editorMax)
{
    if (!m_ShowMinimap) return;
    if (!ActiveDoc()) return;
    if (ActiveDoc()->nodes.empty()) return;

    auto drawList = ImGui::GetWindowDrawList();

    // 小地图位置：编辑器区域右下角，在缩放条和状态栏上方
    float mapSize = m_MinimapSize;
    float padding = 10.0f;
    // 底部需要避开状态栏(22px) + 缩放条(26px) + 间距(8px+6px)
    float bottomReserve = 62.0f; // 22(状态栏) + 26(缩放条) + 8(间距) + 6(缩放条与状态栏间距)
    ImVec2 mapMin(editorMax.x - mapSize - padding, editorMax.y - mapSize - bottomReserve);
    ImVec2 mapMax(editorMax.x - padding, editorMax.y - bottomReserve);

    // 计算所有节点的边界框（画布坐标）
    float canvasMinX = 1e9f, canvasMinY = 1e9f;
    float canvasMaxX = -1e9f, canvasMaxY = -1e9f;

    for (const auto& node : ActiveDoc()->nodes)
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

    // 绘制背景（柔和的暗色渐变 + 精致边框）
    ImU32 bgTop    = IM_COL32(16, 18, 26, 215);
    ImU32 bgBottom = IM_COL32(12, 14, 20, 230);
    drawList->AddRectFilledMultiColor(mapMin, mapMax, bgTop, bgTop, bgBottom, bgBottom);
    drawList->AddRect(mapMin, mapMax, IM_COL32(55, 70, 100, 140), 6.0f);
    // 内侧微光
    drawList->AddRect(
        ImVec2(mapMin.x + 1, mapMin.y + 1),
        ImVec2(mapMax.x - 1, mapMax.y - 1),
        IM_COL32(80, 100, 140, 30), 5.0f);

    // 收集选中节点（用 unordered_set 加速后续 O(1) 查找）
    int selCount = ed::GetSelectedObjectCount();
    std::vector<ed::NodeId> selNodeBuf;
    std::unordered_set<uintptr_t> selectedNodeSet;
    if (selCount > 0)
    {
        selNodeBuf.resize(selCount);
        int nodeCount = ed::GetSelectedNodes(selNodeBuf.data(), selCount);
        selNodeBuf.resize(nodeCount);
        for (const auto& id : selNodeBuf)
            selectedNodeSet.insert(reinterpret_cast<uintptr_t>(id.AsPointer()));
    }

    // 绘制链接（简化为直线）
    for (const auto& link : ActiveDoc()->links)
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

        drawList->AddLine(p1, p2, IM_COL32(80, 100, 140, 60), 1.0f);
    }

    // 绘制节点矩形
    for (const auto& node : ActiveDoc()->nodes)
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
        bool isSelected = selectedNodeSet.count(
            reinterpret_cast<uintptr_t>(node.ID.AsPointer())) > 0;
        if (isSelected)
            drawList->AddRect(rectMin, rectMax, IM_COL32(100, 170, 255, 220), 1.0f, 0, 1.5f);
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

    drawList->AddRectFilled(viewTL_map, viewBR_map, IM_COL32(75, 140, 190, 22));
    drawList->AddRect(viewTL_map, viewBR_map, IM_COL32(90, 155, 220, 150), 0.0f, 0, 1.5f);

    // 小地图标题（渐变底色条）
    float titleH = 16.0f;
    ImU32 titleColL = IM_COL32(32, 48, 72, 200);
    ImU32 titleColR = IM_COL32(24, 34, 52, 180);
    drawList->AddRectFilledMultiColor(
        mapMin, ImVec2(mapMax.x, mapMin.y + titleH),
        titleColL, titleColR, titleColR, titleColL);
    drawList->AddText(ImVec2(mapMin.x + 6, mapMin.y + 1),
                      IM_COL32(155, 190, 230, 220), "Minimap");

    // 点击/拖拽小地图导航到对应画布位置
    ImVec2 mousePos = ImGui::GetMousePos();
    bool mouseInMinimap = (mousePos.x >= mapMin.x && mousePos.x <= mapMax.x &&
                           mousePos.y >= mapMin.y && mousePos.y <= mapMax.y);
    if (mouseInMinimap)
    {
        // 点击或拖拽时：将画布视口中心移动到鼠标对应的画布位置
        if (ImGui::IsMouseClicked(ImGuiMouseButton_Left) ||
            (ImGui::IsMouseDown(ImGuiMouseButton_Left) && ImGui::IsMouseDragging(ImGuiMouseButton_Left, 1.0f)))
        {
            // 小地图屏幕坐标 -> 画布坐标
            float cx = canvasMinX + (mousePos.x - offsetX) / scale;
            float cy = canvasMinY + (mousePos.y - offsetY) / scale;

            // 计算当前视口在画布中的半尺寸，用于构建以点击位置为中心的视口矩形
            float viewHalfW = (viewBR_canvas.x - viewTL_canvas.x) * 0.5f;
            float viewHalfH = (viewBR_canvas.y - viewTL_canvas.y) * 0.5f;

            ImVec2 navMin(cx - viewHalfW, cy - viewHalfH);
            ImVec2 navMax(cx + viewHalfW, cy + viewHalfH);
            ed::NavigateToRect(navMin, navMax, false, 0.15f);
        }

        // 鼠标悬停时改变光标样式提示可交互
        ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
    }
}
