// EditorUI.cpp -- 蓝图编辑器 UI 渲染
#include "BlueprintEditor.h"

// ============================================================================
// 引脚图标颜色
// ============================================================================

ImColor BlueprintEditor::GetIconColor(PinType type)
{
    switch (type)
    {
        default:
        case PinType::Flow:     return ImColor(255, 255, 255);
        case PinType::Bool:     return ImColor(220,  48,  48);
        case PinType::Int:      return ImColor( 68, 201, 156);
        case PinType::Float:    return ImColor(147, 226,  74);
        case PinType::String:   return ImColor(124,  21, 153);
        case PinType::Object:   return ImColor( 51, 150, 215);
        case PinType::Function: return ImColor(218,   0, 183);
        case PinType::Delegate: return ImColor(255,  48,  48);
        case PinType::Array:    return ImColor(255, 165,   0);
        case PinType::Any:      return ImColor(180, 180, 180);
    }
};

// 辅助：获取链接颜色使用的引脚类型（Any 引脚时优先使用对端的实际类型）
PinType BlueprintEditor::GetLinkColor(const Pin* startPin, const Pin* endPin)
{
    if (!startPin || !endPin) return PinType::Flow;
    // 如果起始端是 Any 而终端不是，使用终端类型
    if (startPin->Type == PinType::Any && endPin->Type != PinType::Any)
        return endPin->Type;
    // 如果终端是 Any 而起始端不是，使用起始端类型
    if (endPin->Type == PinType::Any && startPin->Type != PinType::Any)
        return startPin->Type;
    // 两端都是 Any 或都不是 Any，使用起始端类型
    return startPin->Type;
}

// ============================================================================
// 创建链接 + UE4 Flow 自动重连
// ============================================================================
// 当 Flow 输出引脚已有链接时：
//   1. 断开旧链接
//   2. 创建新链接
//   3. 自动把新目标节点的第一个空闲 Flow 输出连到被断开的旧下游节点
void BlueprintEditor::CreateLinkWithFlowReconnect(
    Pin* startPin, ed::PinId startPinId,
    Pin* endPin,   ed::PinId endPinId)
{
    ed::PinId disconnectedPinId = 0;  // 被断开的对端引脚

    // Flow 输出引脚只允许一对一连接：删除旧链接，记录被断开的对端
    if (startPin->Type == PinType::Flow)
    {
        for (auto it = m_Links.begin(); it != m_Links.end();)
        {
            // 检查此链接是否涉及当前 Flow 输出引脚（可能在 Start 或 End 端）
            if (it->StartPinID == startPinId)
            {
                disconnectedPinId = it->EndPinID;
                it = m_Links.erase(it);
            }
            else if (it->EndPinID == startPinId)
            {
                disconnectedPinId = it->StartPinID;
                it = m_Links.erase(it);
            }
            else
                ++it;
        }
    }

    // Flow 输入引脚也只允许一对一连接
    if (endPin->Type == PinType::Flow)
    {
        for (auto it = m_Links.begin(); it != m_Links.end();)
        {
            if (it->StartPinID == endPinId || it->EndPinID == endPinId)
                it = m_Links.erase(it);
            else
                ++it;
        }
    }

    // 创建新链接
    m_Links.emplace_back(Link(GetNextId(), startPinId, endPinId));
    m_Links.back().Color = GetIconColor(GetLinkColor(startPin, endPin));
    m_IsDirty = true;

    // UE4 行为：自动将新目标节点的 Flow 输出连到被断开的旧下游节点
    if (disconnectedPinId && endPin->Node)
    {
        // 确保被断开的引脚不是 endPin 自己（避免循环）
        if (disconnectedPinId != endPinId)
        {
            for (auto& outPin : endPin->Node->Outputs)
            {
                if (outPin.Type == PinType::Flow)
                {
                    // 检查该 Flow 输出引脚是否空闲
                    bool hasExistingLink = false;
                    for (const auto& lnk : m_Links)
                    {
                        if (lnk.StartPinID == outPin.ID || lnk.EndPinID == outPin.ID)
                        {
                            hasExistingLink = true;
                            break;
                        }
                    }
                    if (!hasExistingLink)
                    {
                        auto* disconnectedPin = FindPin(disconnectedPinId);
                        if (disconnectedPin)
                        {
                            m_Links.emplace_back(Link(GetNextId(), outPin.ID, disconnectedPinId));
                            m_Links.back().Color = GetIconColor(PinType::Flow);
                        }
                    }
                    break;  // 只尝试第一个 Flow 输出
                }
            }
        }
    }
}

// ============================================================================
// 引脚图标绘制
// ============================================================================

// 辅助：查找 Any 引脚连线后对端的实际类型（UE4 通配引脚行为）
PinType BlueprintEditor::GetResolvedPinType(const Pin& pin)
{
    if (pin.Type != PinType::Any)
        return pin.Type;

    // 查找与此 Any 引脚相连的对端引脚
    for (const auto& link : m_Links)
    {
        Pin* otherPin = nullptr;
        if (link.StartPinID == pin.ID)
            otherPin = FindPin(link.EndPinID);
        else if (link.EndPinID == pin.ID)
            otherPin = FindPin(link.StartPinID);

        if (otherPin && otherPin->Type != PinType::Any)
            return otherPin->Type;
    }

    return PinType::Any;  // 未连线或对端也是 Any
}

void BlueprintEditor::DrawPinIcon(const Pin& pin, bool connected, int alpha)
{
    // Any 引脚连线后显示为对端的实际类型（颜色 + 形状）
    PinType displayType = GetResolvedPinType(pin);

    IconType iconType;
    ImColor  color = GetIconColor(displayType);
    color.Value.w = alpha / 255.0f;
    switch (displayType)
    {
        case PinType::Flow:     iconType = IconType::Flow;   break;
        case PinType::Bool:     iconType = IconType::Circle; break;
        case PinType::Int:      iconType = IconType::Circle; break;
        case PinType::Float:    iconType = IconType::Circle; break;
        case PinType::String:   iconType = IconType::Circle; break;
        case PinType::Object:   iconType = IconType::Circle; break;
        case PinType::Function: iconType = IconType::Circle; break;
        case PinType::Delegate: iconType = IconType::Square; break;
        case PinType::Array:    iconType = IconType::Grid;   break;
        case PinType::Any:      iconType = IconType::Diamond; break;
        default:
            return;
    }

    ax::Widgets::Icon(ImVec2(static_cast<float>(m_PinIconSize), static_cast<float>(m_PinIconSize)), iconType, connected, color, ImColor(32, 32, 32, alpha));
};

// ============================================================================
// 样式编辑器
// ============================================================================

void BlueprintEditor::ShowStyleEditor(bool* show)
{
    if (!ImGui::Begin("Style", show))
    {
        ImGui::End();
        return;
    }

    auto paneWidth = ImGui::GetContentRegionAvail().x;

    auto& editorStyle = ed::GetStyle();
    ImGui::BeginHorizontal("Style buttons", ImVec2(paneWidth, 0), 1.0f);
    ImGui::TextUnformatted("Values");
    ImGui::Spring();
    if (ImGui::Button("Reset to defaults"))
        editorStyle = ed::Style();
    ImGui::EndHorizontal();
    ImGui::Spacing();
    ImGui::DragFloat4("Node Padding", &editorStyle.NodePadding.x, 0.1f, 0.0f, 40.0f);
    ImGui::DragFloat("Node Rounding", &editorStyle.NodeRounding, 0.1f, 0.0f, 40.0f);
    ImGui::DragFloat("Node Border Width", &editorStyle.NodeBorderWidth, 0.1f, 0.0f, 15.0f);
    ImGui::DragFloat("Hovered Node Border Width", &editorStyle.HoveredNodeBorderWidth, 0.1f, 0.0f, 15.0f);
    ImGui::DragFloat("Hovered Node Border Offset", &editorStyle.HoverNodeBorderOffset, 0.1f, -40.0f, 40.0f);
    ImGui::DragFloat("Selected Node Border Width", &editorStyle.SelectedNodeBorderWidth, 0.1f, 0.0f, 15.0f);
    ImGui::DragFloat("Selected Node Border Offset", &editorStyle.SelectedNodeBorderOffset, 0.1f, -40.0f, 40.0f);
    ImGui::DragFloat("Pin Rounding", &editorStyle.PinRounding, 0.1f, 0.0f, 40.0f);
    ImGui::DragFloat("Pin Border Width", &editorStyle.PinBorderWidth, 0.1f, 0.0f, 15.0f);
    ImGui::DragFloat("Link Strength", &editorStyle.LinkStrength, 1.0f, 0.0f, 500.0f);
    //ImVec2  SourceDirection;
    //ImVec2  TargetDirection;
    ImGui::DragFloat("Scroll Duration", &editorStyle.ScrollDuration, 0.001f, 0.0f, 2.0f);
    ImGui::DragFloat("Flow Marker Distance", &editorStyle.FlowMarkerDistance, 1.0f, 1.0f, 200.0f);
    ImGui::DragFloat("Flow Speed", &editorStyle.FlowSpeed, 1.0f, 1.0f, 2000.0f);
    ImGui::DragFloat("Flow Duration", &editorStyle.FlowDuration, 0.01f, 0.0f, 5.0f);
    //ImVec2  PivotAlignment;
    //ImVec2  PivotSize;
    //ImVec2  PivotScale;
    //float   PinCorners;
    //float   PinRadius;
    //float   PinArrowSize;
    //float   PinArrowWidth;
    ImGui::DragFloat("Group Rounding", &editorStyle.GroupRounding, 0.1f, 0.0f, 40.0f);
    ImGui::DragFloat("Group Border Width", &editorStyle.GroupBorderWidth, 0.1f, 0.0f, 15.0f);

    ImGui::Separator();

    static ImGuiColorEditFlags edit_mode = ImGuiColorEditFlags_DisplayRGB;
    ImGui::BeginHorizontal("Color Mode", ImVec2(paneWidth, 0), 1.0f);
    ImGui::TextUnformatted("Filter Colors");
    ImGui::Spring();
    ImGui::RadioButton("RGB", &edit_mode, ImGuiColorEditFlags_DisplayRGB);
    ImGui::Spring(0);
    ImGui::RadioButton("HSV", &edit_mode, ImGuiColorEditFlags_DisplayHSV);
    ImGui::Spring(0);
    ImGui::RadioButton("HEX", &edit_mode, ImGuiColorEditFlags_DisplayHex);
    ImGui::EndHorizontal();

    static ImGuiTextFilter filter;
    filter.Draw("", paneWidth);

    ImGui::Spacing();

    ImGui::PushItemWidth(-160);
    for (int i = 0; i < ed::StyleColor_Count; ++i)
    {
        auto name = ed::GetStyleColorName((ed::StyleColor)i);
        if (!filter.PassFilter(name))
            continue;
        ImGui::ColorEdit4(name, &editorStyle.Colors[i].x, edit_mode);
    }
    ImGui::PopItemWidth();

    ImGui::End();
}

// ============================================================================
// 左侧面板
// ============================================================================

void BlueprintEditor::ShowLeftPane(float paneWidth)
{
    auto& io = ImGui::GetIO();

    ImGui::BeginChild("Selection", ImVec2(paneWidth, 0));

    paneWidth = ImGui::GetContentRegionAvail().x;

    static bool showStyleEditor = false;
    ImGui::BeginHorizontal("Style Editor", ImVec2(paneWidth, 0));
    ImGui::Spring(0.0f, 0.0f);
    if (ImGui::Button("Zoom to Content"))
        ed::NavigateToContent();
    ImGui::Spring(0.0f);
    if (ImGui::Button("Show Flow"))
    {
        for (auto& link : m_Links)
            ed::Flow(link.ID);
    }
    ImGui::Spring();
    if (ImGui::Button("Edit Style"))
        showStyleEditor = true;
    ImGui::EndHorizontal();
    ImGui::Checkbox("Show Ordinals", &m_ShowOrdinals);

    if (showStyleEditor)
        ShowStyleEditor(&showStyleEditor);

    std::vector<ed::NodeId> selectedNodes;
    std::vector<ed::LinkId> selectedLinks;
    selectedNodes.resize(ed::GetSelectedObjectCount());
    selectedLinks.resize(ed::GetSelectedObjectCount());

    int nodeCount = ed::GetSelectedNodes(selectedNodes.data(), static_cast<int>(selectedNodes.size()));
    int linkCount = ed::GetSelectedLinks(selectedLinks.data(), static_cast<int>(selectedLinks.size()));

    selectedNodes.resize(nodeCount);
    selectedLinks.resize(linkCount);

    int saveIconWidth     = GetTextureWidth(m_SaveIcon);
    int saveIconHeight    = GetTextureHeight(m_SaveIcon);
    int restoreIconWidth  = GetTextureWidth(m_RestoreIcon);
    int restoreIconHeight = GetTextureHeight(m_RestoreIcon);

    // 防御性检查：纹理加载失败时使用默认尺寸，避免 InvisibleButton(0,0) 导致崩溃
    if (saveIconWidth <= 0)    saveIconWidth    = 24;
    if (saveIconHeight <= 0)   saveIconHeight   = 24;
    if (restoreIconWidth <= 0) restoreIconWidth = 24;
    if (restoreIconHeight <= 0) restoreIconHeight = 24;

    ImGui::GetWindowDrawList()->AddRectFilled(
        ImGui::GetCursorScreenPos(),
        ImGui::GetCursorScreenPos() + ImVec2(paneWidth, ImGui::GetTextLineHeight()),
        ImColor(ImGui::GetStyle().Colors[ImGuiCol_HeaderActive]), ImGui::GetTextLineHeight() * 0.25f);
    ImGui::Spacing(); ImGui::SameLine();
    ImGui::TextUnformatted("Nodes");
    ImGui::Indent();
    for (auto& node : m_Nodes)
    {
        ImGui::PushID(node.ID.AsPointer());
        auto start = ImGui::GetCursorScreenPos();

        if (const auto progress = GetTouchProgress(node.ID))
        {
            ImGui::GetWindowDrawList()->AddLine(
                start + ImVec2(-8, 0),
                start + ImVec2(-8, ImGui::GetTextLineHeight()),
                IM_COL32(255, 0, 0, 255 - (int)(255 * progress)), 4.0f);
        }

        bool isSelected = std::find(selectedNodes.begin(), selectedNodes.end(), node.ID) != selectedNodes.end();
# if IMGUI_VERSION_NUM >= 18967
        ImGui::SetNextItemAllowOverlap();
# endif
        if (ImGui::Selectable((node.Name + "##" + std::to_string(reinterpret_cast<uintptr_t>(node.ID.AsPointer()))).c_str(), &isSelected))
        {
            if (io.KeyCtrl)
            {
                if (isSelected)
                    ed::SelectNode(node.ID, true);
                else
                    ed::DeselectNode(node.ID);
            }
            else
                ed::SelectNode(node.ID, false);

            ed::NavigateToSelection();
        }
        if (ImGui::IsItemHovered() && !node.State.empty())
            ImGui::SetTooltip("State: %s", node.State.c_str());

        auto id = std::string("(") + std::to_string(reinterpret_cast<uintptr_t>(node.ID.AsPointer())) + ")";
        auto textSize = ImGui::CalcTextSize(id.c_str(), nullptr);
        auto iconPanelPos = start + ImVec2(
            paneWidth - ImGui::GetStyle().FramePadding.x - ImGui::GetStyle().IndentSpacing - saveIconWidth - restoreIconWidth - ImGui::GetStyle().ItemInnerSpacing.x * 1,
            (ImGui::GetTextLineHeight() - saveIconHeight) / 2);
        ImGui::GetWindowDrawList()->AddText(
            ImVec2(iconPanelPos.x - textSize.x - ImGui::GetStyle().ItemInnerSpacing.x, start.y),
            IM_COL32(255, 255, 255, 255), id.c_str(), nullptr);

        auto drawList = ImGui::GetWindowDrawList();
        ImGui::SetCursorScreenPos(iconPanelPos);
# if IMGUI_VERSION_NUM < 18967
        ImGui::SetItemAllowOverlap();
# else
        ImGui::SetNextItemAllowOverlap();
# endif
        if (node.SavedState.empty())
        {
            if (ImGui::InvisibleButton("save", ImVec2((float)saveIconWidth, (float)saveIconHeight)))
                node.SavedState = node.State;

            if (ImGui::IsItemActive())
                drawList->AddImage(m_SaveIcon, ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), ImVec2(0, 0), ImVec2(1, 1), IM_COL32(255, 255, 255, 96));
            else if (ImGui::IsItemHovered())
                drawList->AddImage(m_SaveIcon, ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), ImVec2(0, 0), ImVec2(1, 1), IM_COL32(255, 255, 255, 255));
            else
                drawList->AddImage(m_SaveIcon, ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), ImVec2(0, 0), ImVec2(1, 1), IM_COL32(255, 255, 255, 160));
        }
        else
        {
            ImGui::Dummy(ImVec2((float)saveIconWidth, (float)saveIconHeight));
            drawList->AddImage(m_SaveIcon, ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), ImVec2(0, 0), ImVec2(1, 1), IM_COL32(255, 255, 255, 32));
        }

        ImGui::SameLine(0, ImGui::GetStyle().ItemInnerSpacing.x);
# if IMGUI_VERSION_NUM < 18967
        ImGui::SetItemAllowOverlap();
# else
        ImGui::SetNextItemAllowOverlap();
# endif
        if (!node.SavedState.empty())
        {
            if (ImGui::InvisibleButton("restore", ImVec2((float)restoreIconWidth, (float)restoreIconHeight)))
            {
                node.State = node.SavedState;
                ed::RestoreNodeState(node.ID);
                node.SavedState.clear();
            }

            if (ImGui::IsItemActive())
                drawList->AddImage(m_RestoreIcon, ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), ImVec2(0, 0), ImVec2(1, 1), IM_COL32(255, 255, 255, 96));
            else if (ImGui::IsItemHovered())
                drawList->AddImage(m_RestoreIcon, ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), ImVec2(0, 0), ImVec2(1, 1), IM_COL32(255, 255, 255, 255));
            else
                drawList->AddImage(m_RestoreIcon, ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), ImVec2(0, 0), ImVec2(1, 1), IM_COL32(255, 255, 255, 160));
        }
        else
        {
            ImGui::Dummy(ImVec2((float)restoreIconWidth, (float)restoreIconHeight));
            drawList->AddImage(m_RestoreIcon, ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), ImVec2(0, 0), ImVec2(1, 1), IM_COL32(255, 255, 255, 32));
        }

        ImGui::SameLine(0, 0);
# if IMGUI_VERSION_NUM < 18967
        ImGui::SetItemAllowOverlap();
# endif
        ImGui::Dummy(ImVec2(0, (float)restoreIconHeight));

        ImGui::PopID();
    }
    ImGui::Unindent();

    static int changeCount = 0;

    ImGui::GetWindowDrawList()->AddRectFilled(
        ImGui::GetCursorScreenPos(),
        ImGui::GetCursorScreenPos() + ImVec2(paneWidth, ImGui::GetTextLineHeight()),
        ImColor(ImGui::GetStyle().Colors[ImGuiCol_HeaderActive]), ImGui::GetTextLineHeight() * 0.25f);
    ImGui::Spacing(); ImGui::SameLine();
    ImGui::TextUnformatted("Selection");

    ImGui::BeginHorizontal("Selection Stats", ImVec2(paneWidth, 0));
    ImGui::Text("Changed %d time%s", changeCount, changeCount > 1 ? "s" : "");
    ImGui::Spring();
    if (ImGui::Button("Deselect All"))
        ed::ClearSelection();
    ImGui::EndHorizontal();
    ImGui::Indent();
    for (int i = 0; i < nodeCount; ++i) ImGui::Text("Node (%p)", selectedNodes[i].AsPointer());
    for (int i = 0; i < linkCount; ++i) ImGui::Text("Link (%p)", selectedLinks[i].AsPointer());
    ImGui::Unindent();

    if (ImGui::IsKeyPressed(ImGuiKey_Z))
        for (auto& link : m_Links)
            ed::Flow(link.ID);

    if (ed::HasSelectionChanged())
        ++changeCount;

    ImGui::Separator();
    ShowExecutionPanel(paneWidth);

    ImGui::EndChild();
}

// ============================================================================
// 主渲染帧
// ============================================================================

void BlueprintEditor::OnFrame(float deltaTime)
{
    // 驱动所有文档的计时器
    for (auto& doc : m_Documents)
        doc->persistentRunner.Tick(deltaTime);

    // 衰减执行高亮
    for (auto& doc : m_Documents)
    {
        for (auto it = doc->executedNodeHighlight.begin(); it != doc->executedNodeHighlight.end();)
        {
            it->second -= deltaTime;
            if (it->second <= 0.0f)
                it = doc->executedNodeHighlight.erase(it);
            else
                ++it;
        }
    }

    // 当前活跃文档的 UpdateTouch
    if (ActiveDoc())
        UpdateTouch();

    // 动态更新 OS 窗口标题（反映 dirty 状态）
    if (ActiveDoc())
    {
        std::string baseName;
        if (m_CurrentFilePath.empty())
            baseName = "[New]";
        else
        {
            size_t lastSlash = m_CurrentFilePath.find_last_of("/\\");
            baseName = (lastSlash != std::string::npos) ? m_CurrentFilePath.substr(lastSlash + 1) : m_CurrentFilePath;
            size_t lastDot = baseName.find_last_of('.');
            if (lastDot != std::string::npos)
                baseName = baseName.substr(0, lastDot);
        }
        std::string windowTitle = "Blueprint Editor - " + baseName;
        if (m_IsDirty)
            windowTitle += " *";
        SetTitle(windowTitle.c_str());
    }

    auto& io = ImGui::GetIO();

    // ================================================================
    // 菜单栏
    // ================================================================
    if (ImGui::BeginMenuBar())
    {
        if (ImGui::BeginMenu("File"))
        {
            if (ImGui::MenuItem("New", "Ctrl+N"))
                NewFile();
            if (ImGui::MenuItem("Open...", "Ctrl+O"))
                OpenFile();
            if (ImGui::BeginMenu("Recent Files"))
            {
                DrawRecentFilesMenu();
                ImGui::EndMenu();
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Save", "Ctrl+S"))
                SaveFile();
            if (ImGui::MenuItem("Save As...", "Ctrl+Shift+S"))
                SaveFileAs();
            ImGui::Separator();
            if (ImGui::MenuItem("Close Tab", "Ctrl+W"))
            {
                if (!m_Documents.empty())
                {
                    if (ActiveDoc() && ActiveDoc()->isDirty)
                    {
                        m_PendingCloseTabIndex = m_ActiveDocIndex;
                        m_ShowUnsavedDialog = true;
                    }
                    else
                        CloseDocument(m_ActiveDocIndex);
                }
            }
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Edit"))
        {
            if (ImGui::MenuItem("Copy", "Ctrl+C"))
                CopySelectedNodes();
            if (ImGui::MenuItem("Paste", "Ctrl+V"))
            {
                ImVec2 canvasPos = ed::ScreenToCanvas(ImGui::GetMousePos());
                PasteNodes(canvasPos);
            }
            if (ImGui::MenuItem("Cut", "Ctrl+X"))
                CutSelectedNodes();
            if (ImGui::MenuItem("Duplicate", "Ctrl+D"))
                DuplicateSelectedNodes();
            ImGui::Separator();
            if (ImGui::MenuItem("Select All", "Ctrl+A"))
            {
                for (auto& node : m_Nodes)
                    ed::SelectNode(node.ID, true);
            }
            ImGui::Separator();
            if (ImGui::BeginMenu("Align Selected"))
            {
                if (ImGui::MenuItem("Align Left"))    AlignSelectedNodes(AlignMode::Left);
                if (ImGui::MenuItem("Align Right"))   AlignSelectedNodes(AlignMode::Right);
                if (ImGui::MenuItem("Align Top"))     AlignSelectedNodes(AlignMode::Top);
                if (ImGui::MenuItem("Align Bottom"))  AlignSelectedNodes(AlignMode::Bottom);
                ImGui::Separator();
                if (ImGui::MenuItem("Center Horizontally"))  AlignSelectedNodes(AlignMode::CenterH);
                if (ImGui::MenuItem("Center Vertically"))    AlignSelectedNodes(AlignMode::CenterV);
                ImGui::EndMenu();
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Find...", "Ctrl+F"))
                OpenSearchOverlay();
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("View"))
        {
            ImGui::MenuItem("Node List", nullptr, &m_ShowNodeListWindow);
            ImGui::MenuItem("Execution Output", nullptr, &m_ShowExecutionWindow);
            ImGui::MenuItem("Timer Monitor", nullptr, &m_ShowTimerWindow);
            ImGui::MenuItem("Minimap", nullptr, &m_ShowMinimap);
            ImGui::MenuItem("Show Ordinals", nullptr, &m_ShowOrdinals);
            ImGui::Separator();
            if (ImGui::MenuItem("Zoom to Content"))
                ed::NavigateToContent();
            if (ImGui::MenuItem("Style Editor"))
                m_ShowStyleEditorWindow = true;
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Run"))
        {
            if (ImGui::MenuItem("Execute Blueprint", "F5"))
                ExecuteBlueprint();
            ImGui::Separator();
            ImGui::MenuItem("Timer Monitor", nullptr, &m_ShowTimerWindow);
            if (ImGui::MenuItem("Clear Execution Highlight"))
            {
                if (ActiveDoc())
                    ActiveDoc()->executedNodeHighlight.clear();
            }
            ImGui::EndMenu();
        }
        ImGui::Separator();

        // 显示当前文件名
        if (ActiveDoc())
        {
            if (!m_CurrentFilePath.empty())
            {
                std::string displayName = m_CurrentFilePath;
                size_t lastSlash = displayName.find_last_of("/\\");
                if (lastSlash != std::string::npos)
                    displayName = displayName.substr(lastSlash + 1);
                if (m_IsDirty)
                    displayName += " *";
                ImGui::TextColored(ImVec4(0.7f, 0.9f, 1.0f, 1.0f), "%s", displayName.c_str());
                ImGui::Separator();
            }
            else
            {
                ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "[New]");
                ImGui::Separator();
            }
        }

        ImGui::Text("FPS: %.2f (%.2gms)", io.Framerate, io.Framerate ? 1000.0f / io.Framerate : 0.0f);

        // 节点/链接统计
        if (ActiveDoc())
        {
            ImGui::Separator();
            ImGui::TextColored(ImVec4(0.5f, 0.7f, 0.5f, 1.0f), "Nodes:%d  Links:%d",
                               static_cast<int>(m_Nodes.size()), static_cast<int>(m_Links.size()));
        }

        ImGui::EndMenuBar();
    }

    // 键盘快捷键 - 文件操作
    if (io.KeyCtrl && !io.KeyShift && ImGui::IsKeyPressed(ImGuiKey_N))
        NewFile();
    if (io.KeyCtrl && !io.KeyShift && ImGui::IsKeyPressed(ImGuiKey_O))
        OpenFile();
    if (io.KeyCtrl && !io.KeyShift && ImGui::IsKeyPressed(ImGuiKey_S))
        SaveFile();
    if (io.KeyCtrl && io.KeyShift && ImGui::IsKeyPressed(ImGuiKey_S))
        SaveFileAs();
    if (io.KeyCtrl && !io.KeyShift && ImGui::IsKeyPressed(ImGuiKey_W))
    {
        if (!m_Documents.empty())
        {
            if (ActiveDoc() && ActiveDoc()->isDirty)
            {
                m_PendingCloseTabIndex = m_ActiveDocIndex;
                m_ShowUnsavedDialog = true;
            }
            else
                CloseDocument(m_ActiveDocIndex);
        }
    }

    // 键盘快捷键 - 编辑操作
    if (io.KeyCtrl && !io.KeyShift && ImGui::IsKeyPressed(ImGuiKey_C))
        CopySelectedNodes();
    if (io.KeyCtrl && !io.KeyShift && ImGui::IsKeyPressed(ImGuiKey_V))
    {
        ImVec2 canvasPos = ed::ScreenToCanvas(ImGui::GetMousePos());
        PasteNodes(canvasPos);
    }
    if (io.KeyCtrl && !io.KeyShift && ImGui::IsKeyPressed(ImGuiKey_X))
        CutSelectedNodes();
    if (io.KeyCtrl && !io.KeyShift && ImGui::IsKeyPressed(ImGuiKey_D))
        DuplicateSelectedNodes();
    if (io.KeyCtrl && !io.KeyShift && ImGui::IsKeyPressed(ImGuiKey_F))
        OpenSearchOverlay();
    if (io.KeyCtrl && !io.KeyShift && ImGui::IsKeyPressed(ImGuiKey_A))
    {
        if (ActiveDoc())
        {
            for (auto& node : m_Nodes)
                ed::SelectNode(node.ID, true);
        }
    }

    // F5: 执行蓝图
    if (ImGui::IsKeyPressed(ImGuiKey_F5))
        ExecuteBlueprint();

    // ================================================================
    // 标签栏（Tab Bar）
    // ================================================================
    int tabToClose = -1;
    if (ImGui::BeginTabBar("##BlueprintTabs", ImGuiTabBarFlags_Reorderable | ImGuiTabBarFlags_AutoSelectNewTabs | ImGuiTabBarFlags_FittingPolicyScroll))
    {
        for (int i = 0; i < (int)m_Documents.size(); ++i)
        {
            auto& doc = m_Documents[i];
            std::string tabTitle = doc->GetTabTitle();

            bool isOpen = true;
            ImGuiTabItemFlags flags = 0;

            if (ImGui::BeginTabItem((tabTitle + "###tab" + std::to_string(i)).c_str(), &isOpen, flags))
            {
                m_ActiveDocIndex = i;
                ImGui::EndTabItem();
            }

            if (!isOpen)
            {
                // 检查是否有未保存的修改
                if (doc->isDirty)
                {
                    m_PendingCloseTabIndex = i;
                    m_ShowUnsavedDialog = true;
                }
                else
                    tabToClose = i;
            }
        }

        // "+" 按钮：新建标签页
        if (ImGui::TabItemButton("+", ImGuiTabItemFlags_Trailing | ImGuiTabItemFlags_NoTooltip))
        {
            CreateNewDocument();
        }

        ImGui::EndTabBar();
    }

    // 延迟关闭标签
    if (tabToClose >= 0)
        CloseDocument(tabToClose);

    // 确保有活跃文档
    if (!ActiveDoc())
        return;

    // 切换到当前活跃文档的编辑器上下文
    ed::SetCurrentEditor(ActiveDoc()->editorContext);

    // ================================================================
    // VSCode 风格固定面板布局
    // ================================================================
    //
    //  ┌──────────┬──────────────────────────┐
    //  │          │                           │
    //  │ 左侧面板  │    中间 Node Editor       │
    //  │(NodeList)│                           │
    //  │          ├──────────────────────────┤
    //  │          │    底部面板                │
    //  │          │  (Execution Output)       │
    //  └──────────┴──────────────────────────┘
    //

    ImVec2 contentRegion = ImGui::GetContentRegionAvail();
    float totalWidth  = contentRegion.x;
    float totalHeight = contentRegion.y;
    float splitterThickness = 4.0f;

    // --- 左侧面板 + 右侧区域 水平分割 ---
    float rightWidth = totalWidth;
    if (m_ShowNodeListWindow)
    {
        // 约束左侧面板宽度
        if (m_LeftPanelWidth < 150.0f) m_LeftPanelWidth = 150.0f;
        if (m_LeftPanelWidth > totalWidth * 0.4f) m_LeftPanelWidth = totalWidth * 0.4f;

        rightWidth = totalWidth - m_LeftPanelWidth - splitterThickness;
        if (rightWidth < 200.0f) rightWidth = 200.0f;

        Splitter("##HorizontalSplitter", true, splitterThickness, &m_LeftPanelWidth, &rightWidth, 150.0f, 200.0f, totalHeight);

        // 绘制左侧面板
        ImGui::BeginChild("##LeftPanel", ImVec2(m_LeftPanelWidth, totalHeight), true);
        DrawNodeListPanel();
        ImGui::EndChild();

        ImGui::SameLine();
    }

    // --- 右侧区域：上部编辑器 + 下部执行输出 垂直分割 ---
    ImVec2 editorMin(0, 0), editorMax(0, 0);  // 编辑器区域的屏幕坐标（供小地图等使用）
    ImGui::BeginGroup();
    {
        float editorHeight = totalHeight;
        float bottomHeight = 0.0f;

        if (m_ShowExecutionWindow)
        {
            // 约束底部面板高度
            if (m_BottomPanelHeight < 100.0f) m_BottomPanelHeight = 100.0f;
            if (m_BottomPanelHeight > totalHeight * 0.6f) m_BottomPanelHeight = totalHeight * 0.6f;

            editorHeight = totalHeight - m_BottomPanelHeight - splitterThickness;
            if (editorHeight < 200.0f) editorHeight = 200.0f;
            bottomHeight = totalHeight - editorHeight - splitterThickness;

            Splitter("##VerticalSplitter", false, splitterThickness, &editorHeight, &bottomHeight, 200.0f, 100.0f, rightWidth);

            // 将 Splitter 拖动的结果写回成员变量，否则下一帧会被还原
            m_BottomPanelHeight = bottomHeight;
        }

        // --- 中间 Node Editor 区域 ---
        // 直接通过 ed::Begin 的 size 参数限制编辑器区域，不使用额外的 BeginChild
        {
    // 使用当前文档的 UI 状态（而非 static，支持多标签页）
    auto* _doc = ActiveDoc();
    auto& contextNodeId  = _doc->contextNodeId;
    auto& contextLinkId  = _doc->contextLinkId;
    auto& contextPinId   = _doc->contextPinId;
    auto& createNewNode  = _doc->createNewNode;
    auto& newNodeLinkPin = _doc->newNodeLinkPin;
    auto& newLinkPin     = _doc->newLinkPin;

    ed::Begin("Node editor", ImVec2(rightWidth, editorHeight));
    {
        // 加载文件后延迟设置节点位置（必须在 ed::Begin/End 之间）
        if (m_NeedSetNodePositions)
        {
            m_NeedSetNodePositions = false;

            // 从加载数据计算所有节点的包围盒
            ImRect contentBounds(FLT_MAX, FLT_MAX, -FLT_MAX, -FLT_MAX);

            for (const auto& pendNode : m_PendingLoadData.nodes)
            {
                auto it = pendNode.customProperties.find("__newEditorId");
                if (it != pendNode.customProperties.end())
                {
                    int editorId = std::stoi(it->second);
                    ed::NodeId nodeId(editorId);

                    if (pendNode.position.x != 0.0f || pendNode.position.y != 0.0f)
                        ed::SetNodePosition(nodeId, ImVec2(pendNode.position.x, pendNode.position.y));

                    if (pendNode.size.width > 0 && pendNode.size.height > 0)
                    {
                        Node* node = FindNode(nodeId);
                        if (node && node->Type == NodeType::Comment)
                        {
                            ed::SetGroupSize(nodeId, ImVec2(pendNode.size.width, pendNode.size.height));
                            node->Size = ImVec2(pendNode.size.width, pendNode.size.height);
                        }
                    }

                    // 用位置和尺寸计算包围盒（尺寸默认给 200x100）
                    float w = pendNode.size.width > 0 ? pendNode.size.width : 200.0f;
                    float h = pendNode.size.height > 0 ? pendNode.size.height : 100.0f;
                    contentBounds.Add(ImRect(
                        ImVec2(pendNode.position.x, pendNode.position.y),
                        ImVec2(pendNode.position.x + w, pendNode.position.y + h)
                    ));
                }
            }

            m_PendingLoadData.clear();
            m_PendingContentBounds = contentBounds;
            m_NeedNavigateToContent = 1;
        }

        // 延迟居中显示（倒计帧数，到 0 时触发）
        if (m_NeedNavigateToContent > 0)
        {
            m_NeedNavigateToContent--;
            if (m_NeedNavigateToContent == 0)
            {
                // 如果有预计算的 bounds（加载文件时），直接用它导航
                if (m_PendingContentBounds.Min.x < m_PendingContentBounds.Max.x)
                {
                    ed::NavigateToRect(m_PendingContentBounds.Min, m_PendingContentBounds.Max, true, 0);
                    m_PendingContentBounds = ImRect();  // 清除
                }
                else
                    ed::NavigateToContent();
            }
        }

        auto cursorTopLeft = ImGui::GetCursorScreenPos();

        util::BlueprintNodeBuilder builder(m_HeaderBackground, GetTextureWidth(m_HeaderBackground), GetTextureHeight(m_HeaderBackground));

        for (auto& node : m_Nodes)
        {
            if (node.Type != NodeType::Blueprint && node.Type != NodeType::Simple)
                continue;

            // ---- 通用引脚可见性：根据 HiddenWhen 声明式规则动态隐藏/显示引脚 ----
            // 格式: "引脚名==值" — 当同节点指定输入引脚的值等于给定值时隐藏此引脚
            // 被引用的引脚若被连线，则条件不满足（保守显示）
            auto evaluateHiddenWhen = [this](Pin& pin, Node& ownerNode)
            {
                if (pin.HiddenWhen.empty())
                    return;

                // 解析 "PinName==Value"
                auto eqPos = pin.HiddenWhen.find("==");
                if (eqPos == std::string::npos)
                    return;

                std::string refPinName = pin.HiddenWhen.substr(0, eqPos);
                std::string refValue   = pin.HiddenWhen.substr(eqPos + 2);

                // 在同节点的输入引脚中查找引用的引脚
                bool shouldHide = false;
                for (const auto& input : ownerNode.Inputs)
                {
                    if (input.Name == refPinName)
                    {
                        // 被连线时无法确定运行时值，保守显示
                        if (IsPinLinked(input.ID))
                            break;

                        // 根据引脚类型比较值
                        if (input.Type == PinType::Bool)
                            shouldHide = (input.BoolValue ? "true" : "false") == refValue;
                        else if (input.Type == PinType::Int)
                            shouldHide = std::to_string(input.IntValue) == refValue;
                        else if (input.Type == PinType::Float)
                            shouldHide = std::to_string(input.FloatValue) == refValue;
                        else if (input.Type == PinType::String)
                            shouldHide = input.StringValue == refValue;
                        break;
                    }
                }

                // 从"显示"变为"隐藏"时，自动断开该引脚上的所有连线
                if (shouldHide && !pin.IsHidden)
                {
                    ed::PinId pinId = pin.ID;
                    m_Links.erase(std::remove_if(m_Links.begin(), m_Links.end(),
                        [pinId](const Link& l) { return l.StartPinID == pinId || l.EndPinID == pinId; }),
                        m_Links.end());
                }
                pin.IsHidden = shouldHide;
            };

            for (auto& input : node.Inputs)
                evaluateHiddenWhen(input, node);
            for (auto& output : node.Outputs)
                evaluateHiddenWhen(output, node);

            const auto isSimple = node.Type == NodeType::Simple;

            bool hasOutputDelegates = false;
            for (auto& output : node.Outputs)
                if (output.Type == PinType::Delegate)
                    hasOutputDelegates = true;

            builder.Begin(node.ID);
                if (!isSimple)
                {
                    builder.Header(node.Color);
                        ImGui::Spring(0);
                        ImGui::TextUnformatted(node.Name.c_str());
                        ImGui::Spring(1);
                        ImGui::Dummy(ImVec2(0, 28));
                        if (hasOutputDelegates)
                        {
                            ImGui::BeginVertical("delegates", ImVec2(0, 28));
                            ImGui::Spring(1, 0);
                            for (auto& output : node.Outputs)
                            {
                                if (output.Type != PinType::Delegate)
                                    continue;

                                auto alpha = ImGui::GetStyle().Alpha;
                                if (newLinkPin && !CanCreateLink(newLinkPin, &output) && &output != newLinkPin)
                                    alpha = alpha * (48.0f / 255.0f);

                                ed::BeginPin(output.ID, ed::PinKind::Output);
                                ed::PinPivotAlignment(ImVec2(1.0f, 0.5f));
                                ed::PinPivotSize(ImVec2(0, 0));
                                ImGui::BeginHorizontal(output.ID.AsPointer());
                                ImGui::PushStyleVar(ImGuiStyleVar_Alpha, alpha);
                                if (!output.Name.empty())
                                {
                                    ImGui::TextUnformatted(output.Name.c_str());
                                    ImGui::Spring(0);
                                }
                                DrawPinIcon(output, IsPinLinked(output.ID), (int)(alpha * 255));
                                ImGui::Spring(0, ImGui::GetStyle().ItemSpacing.x / 2);
                                ImGui::EndHorizontal();
                                ImGui::PopStyleVar();
                                ed::EndPin();

                                //DrawItemRect(ImColor(255, 0, 0));
                            }
                            ImGui::Spring(1, 0);
                            ImGui::EndVertical();
                            ImGui::Spring(0, ImGui::GetStyle().ItemSpacing.x / 2);
                        }
                        else
                            ImGui::Spring(0);
                    builder.EndHeader();
                }

                for (auto& input : node.Inputs)
                {
                    if (input.IsHidden)
                        continue;

                    auto alpha = ImGui::GetStyle().Alpha;
                    if (newLinkPin && !CanCreateLink(newLinkPin, &input) && &input != newLinkPin)
                        alpha = alpha * (48.0f / 255.0f);
                    // 孤立引脚整体半透明
                    if (input.IsOrphaned)
                        alpha = alpha * 0.5f;

                    builder.Input(input.ID);
                    ImGui::PushStyleVar(ImGuiStyleVar_Alpha, alpha);
                    DrawPinIcon(input, IsPinLinked(input.ID), (int)(alpha * 255));
                    ImGui::Spring(0);
                    if (!input.Name.empty())
                    {
                        if (input.IsOrphaned)
                        {
                            // 孤立引脚：红色 + 删除线
                            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.3f, 0.3f, alpha));
                            auto cursorBefore = ImGui::GetCursorScreenPos();
                            ImGui::TextUnformatted(input.Name.c_str());
                            auto textSize = ImGui::CalcTextSize(input.Name.c_str());
                            float lineY = cursorBefore.y + textSize.y * 0.5f;
                            ImGui::GetWindowDrawList()->AddLine(
                                ImVec2(cursorBefore.x, lineY),
                                ImVec2(cursorBefore.x + textSize.x, lineY),
                                IM_COL32(255, 80, 80, (int)(alpha * 255)), 1.0f);
                            ImGui::PopStyleColor();
                        }
                        else if (input.IsRequired && !IsPinLinked(input.ID))
                        {
                            // 必须连接但未连线：橙色警告
                            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.6f, 0.0f, 1.0f));
                            ImGui::TextUnformatted(input.Name.c_str());
                            ImGui::PopStyleColor();
                        }
                        else
                        {
                            ImGui::TextUnformatted(input.Name.c_str());
                        }
                        ImGui::Spring(0);
                    }

                    // 当输入引脚未连线时，基础数据类型显示内联编辑控件
                    if (!IsPinLinked(input.ID) && input.Type != PinType::Flow && input.Type != PinType::Delegate)
                    {
                        ImGui::PushID(input.ID.AsPointer());

                        static std::unordered_map<uintptr_t, std::array<char, 128>> s_StringBuffers;

                        if (input.Type == PinType::Bool)
                        {
                            if (ImGui::Checkbox("##value", &input.BoolValue))
                                m_IsDirty = true;
                        }
                        else if (input.Type == PinType::Int)
                        {
                            ImGui::SetNextItemWidth(80.0f);
                            ImS64 v = static_cast<ImS64>(input.IntValue);
                            if (ImGui::DragScalar("##value", ImGuiDataType_S64, &v, 1.0f))
                            {
                                input.IntValue = static_cast<int64_t>(v);
                                m_IsDirty = true;
                            }
                        }
                        else if (input.Type == PinType::Float)
                        {
                            ImGui::SetNextItemWidth(80.0f);
                            if (ImGui::DragFloat("##value", &input.FloatValue, 0.01f))
                                m_IsDirty = true;
                        }
                        else if (input.Type == PinType::String)
                        {
                            auto key = reinterpret_cast<uintptr_t>(input.ID.AsPointer());
                            auto& buf = s_StringBuffers[key];
                            if (buf[0] == '\0' && !input.StringValue.empty())
                                snprintf(buf.data(), buf.size(), "%s", input.StringValue.c_str());
                            ImGui::SetNextItemWidth(100.0f);
                            if (ImGui::InputText("##value", buf.data(), buf.size()))
                            {
                                input.StringValue = buf.data();
                                m_IsDirty = true;
                            }
                        }
                        else if (input.Type == PinType::Object)
                        {
                            static std::unordered_map<uintptr_t, std::array<char, 128>> s_ObjectBuffers;
                            auto key = reinterpret_cast<uintptr_t>(input.ID.AsPointer());
                            auto& buf = s_ObjectBuffers[key];
                            if (buf[0] == '\0' && !input.ObjectValue.empty())
                                snprintf(buf.data(), buf.size(), "%s", input.ObjectValue.c_str());
                            ImGui::SetNextItemWidth(100.0f);
                            if (ImGui::InputText("##value", buf.data(), buf.size()))
                            {
                                input.ObjectValue = buf.data();
                                m_IsDirty = true;
                            }
                        }
                        else if (input.Type == PinType::Function)
                        {
                            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.5f, 0.5f, 0.5f, 1.0f));
                            ImGui::TextUnformatted("(none)");
                            ImGui::PopStyleColor();
                        }

                        // 编辑控件处于激活状态时禁用节点编辑器快捷键
                        if (ImGui::IsItemActive())
                            ed::EnableShortcuts(false);
                        else
                            ed::EnableShortcuts(true);

                        ImGui::PopID();
                        ImGui::Spring(0);
                    }

                    // [-] button for dynamic (removable) input pins
                    if (node.HasDynamicInputs)
                    {
                        // Determine the index of this pin in Inputs
                        int pinIdx = static_cast<int>(&input - node.Inputs.data());
                        if (pinIdx >= node.DynamicInputFixedCount)
                        {
                            ImGui::Spring(0);
                            ImGui::PushID(input.ID.AsPointer());
                            ImGui::PushStyleColor(ImGuiCol_Button,        ImVec4(0.6f, 0.1f, 0.1f, 0.6f));
                            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.8f, 0.2f, 0.2f, 0.8f));
                            ImGui::PushStyleColor(ImGuiCol_ButtonActive,  ImVec4(1.0f, 0.3f, 0.3f, 1.0f));
                            if (ImGui::SmallButton("-"))
                            {
                                // Remove all links connected to this pin
                                ed::PinId pinId = input.ID;
                                m_Links.erase(std::remove_if(m_Links.begin(), m_Links.end(),
                                    [pinId](const Link& l) { return l.StartPinID == pinId || l.EndPinID == pinId; }),
                                    m_Links.end());
                                // Mark for removal (can't erase during iteration)
                                // We'll handle it after builder.EndInput()
                                node.Inputs[pinIdx].StringValue = "\x01REMOVE";
                                m_IsDirty = true;
                            }
                            ImGui::PopStyleColor(3);
                            ImGui::PopID();
                        }
                    }

                    ImGui::PopStyleVar();
                    builder.EndInput();
                }

                // Process pending removals for dynamic pins
                if (node.HasDynamicInputs)
                {
                    node.Inputs.erase(std::remove_if(node.Inputs.begin(), node.Inputs.end(),
                        [](const Pin& p) { return p.StringValue == "\x01REMOVE"; }),
                        node.Inputs.end());
                    BuildNode(&node);
                }

                if (isSimple)
                {
                    builder.Middle();

                    ImGui::Spring(1, 0);
                    ImGui::TextUnformatted(node.Name.c_str());
                    ImGui::Spring(1, 0);
                }
                else if (node.HasDynamicInputs)
                {
                    // For Blueprint nodes with dynamic inputs, use Middle() to place the [+] button
                    builder.Middle();

                    ImGui::PushID(node.ID.AsPointer());
                    ImGui::PushStyleColor(ImGuiCol_Button,        ImVec4(0.1f, 0.4f, 0.1f, 0.6f));
                    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.2f, 0.6f, 0.2f, 0.8f));
                    ImGui::PushStyleColor(ImGuiCol_ButtonActive,  ImVec4(0.3f, 0.8f, 0.3f, 1.0f));
                    if (ImGui::SmallButton("+"))
                    {
                        // 根据节点类型生成引脚名称
                        int dynCount = static_cast<int>(node.Inputs.size()) - node.DynamicInputFixedCount;
                        std::string pinName;

                        if (node.DefinitionId == "FormatString")
                        {
                            // FormatString: "Arg 0", "Arg 1", "Arg 2", ...
                            pinName = "Arg " + std::to_string(dynCount);
                        }
                        else if (node.DefinitionId == "MakeArray")
                        {
                            // MakeArray: "Element 0", "Element 1", ...
                            pinName = "Element " + std::to_string(dynCount);
                        }
                        else
                        {
                            // 默认命名: A, B, C, ... Z, AA, AB, ...
                            int idx = static_cast<int>(node.Inputs.size());
                            do {
                                pinName = std::string(1, 'A' + (idx % 26)) + pinName;
                                idx = idx / 26 - 1;
                            } while (idx >= 0);
                        }
                        node.Inputs.emplace_back(GetNextId(), pinName.c_str(), node.DynamicInputPinType);
                        BuildNode(&node);
                        m_IsDirty = true;
                    }
                    ImGui::PopStyleColor(3);
                    ImGui::PopID();
                }

                for (auto& output : node.Outputs)
                {
                    if (!isSimple && output.Type == PinType::Delegate)
                        continue;
                    if (output.IsHidden)
                        continue;

                    auto alpha = ImGui::GetStyle().Alpha;
                    if (newLinkPin && !CanCreateLink(newLinkPin, &output) && &output != newLinkPin)
                        alpha = alpha * (48.0f / 255.0f);
                    if (output.IsOrphaned)
                        alpha = alpha * 0.5f;

                    builder.Output(output.ID);
                    ImGui::PushStyleVar(ImGuiStyleVar_Alpha, alpha);
                    if (!output.Name.empty())
                    {
                        ImGui::Spring(0);
                        if (output.IsOrphaned)
                        {
                            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.3f, 0.3f, alpha));
                            auto cursorBefore = ImGui::GetCursorScreenPos();
                            ImGui::TextUnformatted(output.Name.c_str());
                            auto textSize = ImGui::CalcTextSize(output.Name.c_str());
                            float lineY = cursorBefore.y + textSize.y * 0.5f;
                            ImGui::GetWindowDrawList()->AddLine(
                                ImVec2(cursorBefore.x, lineY),
                                ImVec2(cursorBefore.x + textSize.x, lineY),
                                IM_COL32(255, 80, 80, (int)(alpha * 255)), 1.0f);
                            ImGui::PopStyleColor();
                        }
                        else
                        {
                            ImGui::TextUnformatted(output.Name.c_str());
                        }
                    }
                    ImGui::Spring(0);
                    DrawPinIcon(output, IsPinLinked(output.ID), (int)(alpha * 255));
                    ImGui::PopStyleVar();
                    builder.EndOutput();
                }

            builder.End();

            // ---- 错误节点视觉反馈（UE4 风格）----
            if (node.HasError)
            {
                auto drawList = ed::GetNodeBackgroundDrawList(node.ID);
                auto nodeMin = ed::GetNodePosition(node.ID);
                auto nodeSize = ed::GetNodeSize(node.ID);
                auto nodeMax = ImVec2(nodeMin.x + nodeSize.x, nodeMin.y + nodeSize.y);
                // 将画布坐标转换为屏幕坐标
                auto screenMin = ed::CanvasToScreen(nodeMin);
                auto screenMax = ed::CanvasToScreen(nodeMax);

                // 红色边框
                drawList->AddRect(
                    screenMin - ImVec2(2, 2),
                    screenMax + ImVec2(2, 2),
                    IM_COL32(255, 40, 40, 200), 6.0f, 0, 2.5f);

                // 错误信息：在节点底部渲染红色文字
                if (!node.ErrorMessage.empty())
                {
                    auto textSize = ImGui::CalcTextSize(node.ErrorMessage.c_str());
                    auto textPos = ImVec2(
                        screenMin.x + (screenMax.x - screenMin.x - textSize.x) * 0.5f,
                        screenMax.y + 2.0f);
                    drawList->AddRectFilled(
                        textPos - ImVec2(4, 1),
                        textPos + textSize + ImVec2(4, 1),
                        IM_COL32(80, 0, 0, 200), 3.0f);
                    drawList->AddText(textPos, IM_COL32(255, 100, 100, 255), node.ErrorMessage.c_str());
                }
            }

            // ---- 执行可视化：绿色发光边框高亮已执行节点 ----
            {
                uint64_t nid = static_cast<uint64_t>(reinterpret_cast<uintptr_t>(node.ID.AsPointer()));
                auto hlIt = ActiveDoc()->executedNodeHighlight.find(nid);
                if (hlIt != ActiveDoc()->executedNodeHighlight.end() && hlIt->second > 0.0f)
                {
                    float alpha = hlIt->second / 3.0f;  // 淡出效果
                    if (alpha > 1.0f) alpha = 1.0f;
                    int a = static_cast<int>(alpha * 200);

                    auto drawList = ed::GetNodeBackgroundDrawList(node.ID);
                    auto nodeMin = ed::GetNodePosition(node.ID);
                    auto nodeSize = ed::GetNodeSize(node.ID);
                    auto nodeMax = ImVec2(nodeMin.x + nodeSize.x, nodeMin.y + nodeSize.y);
                    auto screenMin = ed::CanvasToScreen(nodeMin);
                    auto screenMax = ed::CanvasToScreen(nodeMax);

                    // 绿色发光边框
                    drawList->AddRect(
                        screenMin - ImVec2(3, 3),
                        screenMax + ImVec2(3, 3),
                        IM_COL32(50, 255, 100, a), 8.0f, 0, 3.0f);
                    // 外层淡光晕
                    drawList->AddRect(
                        screenMin - ImVec2(6, 6),
                        screenMax + ImVec2(6, 6),
                        IM_COL32(50, 255, 100, a / 3), 10.0f, 0, 2.0f);
                }
            }
        }

        // ================================================================
        // Tree 风格节点
        // ================================================================
        for (auto& node : m_Nodes)
        {
            if (node.Type != NodeType::Tree)
                continue;

            const float rounding = 5.0f;
            const float padding  = 12.0f;

            const auto pinBackground = ed::GetStyle().Colors[ed::StyleColor_NodeBg];

            ed::PushStyleColor(ed::StyleColor_NodeBg,        ImColor(128, 128, 128, 200));
            ed::PushStyleColor(ed::StyleColor_NodeBorder,    ImColor( 32,  32,  32, 200));
            ed::PushStyleColor(ed::StyleColor_PinRect,       ImColor( 60, 180, 255, 150));
            ed::PushStyleColor(ed::StyleColor_PinRectBorder, ImColor( 60, 180, 255, 150));

            ed::PushStyleVar(ed::StyleVar_NodePadding,  ImVec4(0, 0, 0, 0));
            ed::PushStyleVar(ed::StyleVar_NodeRounding, rounding);
            ed::PushStyleVar(ed::StyleVar_SourceDirection, ImVec2(0.0f,  1.0f));
            ed::PushStyleVar(ed::StyleVar_TargetDirection, ImVec2(0.0f, -1.0f));
            ed::PushStyleVar(ed::StyleVar_LinkStrength, 0.0f);
            ed::PushStyleVar(ed::StyleVar_PinBorderWidth, 1.0f);
            ed::PushStyleVar(ed::StyleVar_PinRadius, 5.0f);
            ed::BeginNode(node.ID);

            ImGui::BeginVertical(node.ID.AsPointer());
            ImGui::BeginHorizontal("inputs");
            ImGui::Spring(0, padding * 2);

            ImRect inputsRect;
            int inputAlpha = 200;
            if (!node.Inputs.empty())
            {
                    auto& pin = node.Inputs[0];
                    ImGui::Dummy(ImVec2(0, padding));
                    ImGui::Spring(1, 0);
                    inputsRect = ImGui_GetItemRect();

                    ed::PushStyleVar(ed::StyleVar_PinArrowSize, 10.0f);
                    ed::PushStyleVar(ed::StyleVar_PinArrowWidth, 10.0f);
#if IMGUI_VERSION_NUM > 18101
                    ed::PushStyleVar(ed::StyleVar_PinCorners, ImDrawFlags_RoundCornersBottom);
#else
                    ed::PushStyleVar(ed::StyleVar_PinCorners, 12);
#endif
                    ed::BeginPin(pin.ID, ed::PinKind::Input);
                    ed::PinPivotRect(inputsRect.GetTL(), inputsRect.GetBR());
                    ed::PinRect(inputsRect.GetTL(), inputsRect.GetBR());
                    ed::EndPin();
                    ed::PopStyleVar(3);

                    if (newLinkPin && !CanCreateLink(newLinkPin, &pin) && &pin != newLinkPin)
                        inputAlpha = (int)(255 * ImGui::GetStyle().Alpha * (48.0f / 255.0f));
            }
            else
                ImGui::Dummy(ImVec2(0, padding));

            ImGui::Spring(0, padding * 2);
            ImGui::EndHorizontal();

            ImGui::BeginHorizontal("content_frame");
            ImGui::Spring(1, padding);

            ImGui::BeginVertical("content", ImVec2(0.0f, 0.0f));
            ImGui::Dummy(ImVec2(160, 0));
            ImGui::Spring(1);
            ImGui::TextUnformatted(node.Name.c_str());
            ImGui::Spring(1);
            ImGui::EndVertical();
            auto contentRect = ImGui_GetItemRect();

            ImGui::Spring(1, padding);
            ImGui::EndHorizontal();

            ImGui::BeginHorizontal("outputs");
            ImGui::Spring(0, padding * 2);

            ImRect outputsRect;
            int outputAlpha = 200;
            if (!node.Outputs.empty())
            {
                auto& pin = node.Outputs[0];
                ImGui::Dummy(ImVec2(0, padding));
                ImGui::Spring(1, 0);
                outputsRect = ImGui_GetItemRect();

#if IMGUI_VERSION_NUM > 18101
                ed::PushStyleVar(ed::StyleVar_PinCorners, ImDrawFlags_RoundCornersTop);
#else
                ed::PushStyleVar(ed::StyleVar_PinCorners, 3);
#endif
                ed::BeginPin(pin.ID, ed::PinKind::Output);
                ed::PinPivotRect(outputsRect.GetTL(), outputsRect.GetBR());
                ed::PinRect(outputsRect.GetTL(), outputsRect.GetBR());
                ed::EndPin();
                ed::PopStyleVar();

                if (newLinkPin && !CanCreateLink(newLinkPin, &pin) && &pin != newLinkPin)
                    outputAlpha = (int)(255 * ImGui::GetStyle().Alpha * (48.0f / 255.0f));
            }
            else
                ImGui::Dummy(ImVec2(0, padding));

            ImGui::Spring(0, padding * 2);
            ImGui::EndHorizontal();

            ImGui::EndVertical();

            ed::EndNode();
            ed::PopStyleVar(7);
            ed::PopStyleColor(4);

            auto drawList = ed::GetNodeBackgroundDrawList(node.ID);

#if IMGUI_VERSION_NUM > 18101
            const auto    topRoundCornersFlags = ImDrawFlags_RoundCornersTop;
            const auto bottomRoundCornersFlags = ImDrawFlags_RoundCornersBottom;
#else
            const auto    topRoundCornersFlags = 1 | 2;
            const auto bottomRoundCornersFlags = 4 | 8;
#endif

            drawList->AddRectFilled(inputsRect.GetTL() + ImVec2(0, 1), inputsRect.GetBR(),
                IM_COL32((int)(255 * pinBackground.x), (int)(255 * pinBackground.y), (int)(255 * pinBackground.z), inputAlpha), 4.0f, bottomRoundCornersFlags);
            drawList->AddRect(inputsRect.GetTL() + ImVec2(0, 1), inputsRect.GetBR(),
                IM_COL32((int)(255 * pinBackground.x), (int)(255 * pinBackground.y), (int)(255 * pinBackground.z), inputAlpha), 4.0f, bottomRoundCornersFlags);
            drawList->AddRectFilled(outputsRect.GetTL(), outputsRect.GetBR() - ImVec2(0, 1),
                IM_COL32((int)(255 * pinBackground.x), (int)(255 * pinBackground.y), (int)(255 * pinBackground.z), outputAlpha), 4.0f, topRoundCornersFlags);
            drawList->AddRect(outputsRect.GetTL(), outputsRect.GetBR() - ImVec2(0, 1),
                IM_COL32((int)(255 * pinBackground.x), (int)(255 * pinBackground.y), (int)(255 * pinBackground.z), outputAlpha), 4.0f, topRoundCornersFlags);
            drawList->AddRectFilled(contentRect.GetTL(), contentRect.GetBR(), IM_COL32(24, 64, 128, 200), 0.0f);
            drawList->AddRect(
                contentRect.GetTL(),
                contentRect.GetBR(),
                IM_COL32(48, 128, 255, 100), 0.0f);
        }

        // ================================================================
        // Houdini 风格节点
        // ================================================================
        for (auto& node : m_Nodes)
        {
            if (node.Type != NodeType::Houdini)
                continue;

            const float rounding = 10.0f;
            const float padding  = 12.0f;

            ed::PushStyleColor(ed::StyleColor_NodeBg,        ImColor(229, 229, 229, 200));
            ed::PushStyleColor(ed::StyleColor_NodeBorder,    ImColor(125, 125, 125, 200));
            ed::PushStyleColor(ed::StyleColor_PinRect,       ImColor(229, 229, 229, 60));
            ed::PushStyleColor(ed::StyleColor_PinRectBorder, ImColor(125, 125, 125, 60));

            const auto pinBackground = ed::GetStyle().Colors[ed::StyleColor_NodeBg];

            ed::PushStyleVar(ed::StyleVar_NodePadding,  ImVec4(0, 0, 0, 0));
            ed::PushStyleVar(ed::StyleVar_NodeRounding, rounding);
            ed::PushStyleVar(ed::StyleVar_SourceDirection, ImVec2(0.0f,  1.0f));
            ed::PushStyleVar(ed::StyleVar_TargetDirection, ImVec2(0.0f, -1.0f));
            ed::PushStyleVar(ed::StyleVar_LinkStrength, 0.0f);
            ed::PushStyleVar(ed::StyleVar_PinBorderWidth, 1.0f);
            ed::PushStyleVar(ed::StyleVar_PinRadius, 6.0f);
            ed::BeginNode(node.ID);

            ImGui::BeginVertical(node.ID.AsPointer());
            if (!node.Inputs.empty())
            {
                ImGui::BeginHorizontal("inputs");
                ImGui::Spring(1, 0);

                ImRect inputsRect;
                int inputAlpha = 200;
                for (auto& pin : node.Inputs)
                {
                    ImGui::Dummy(ImVec2(padding, padding));
                    inputsRect = ImGui_GetItemRect();
                    ImGui::Spring(1, 0);
                    inputsRect.Min.y -= padding;
                    inputsRect.Max.y -= padding;

#if IMGUI_VERSION_NUM > 18101
                    const auto allRoundCornersFlags = ImDrawFlags_RoundCornersAll;
#else
                    const auto allRoundCornersFlags = 15;
#endif
                    ed::PushStyleVar(ed::StyleVar_PinCorners, allRoundCornersFlags);

                    ed::BeginPin(pin.ID, ed::PinKind::Input);
                    ed::PinPivotRect(inputsRect.GetCenter(), inputsRect.GetCenter());
                    ed::PinRect(inputsRect.GetTL(), inputsRect.GetBR());
                    ed::EndPin();
                    ed::PopStyleVar(1);

                    auto drawList = ImGui::GetWindowDrawList();
                    drawList->AddRectFilled(inputsRect.GetTL(), inputsRect.GetBR(),
                        IM_COL32((int)(255 * pinBackground.x), (int)(255 * pinBackground.y), (int)(255 * pinBackground.z), inputAlpha), 4.0f, allRoundCornersFlags);
                    drawList->AddRect(inputsRect.GetTL(), inputsRect.GetBR(),
                        IM_COL32((int)(255 * pinBackground.x), (int)(255 * pinBackground.y), (int)(255 * pinBackground.z), inputAlpha), 4.0f, allRoundCornersFlags);

                    if (newLinkPin && !CanCreateLink(newLinkPin, &pin) && &pin != newLinkPin)
                        inputAlpha = (int)(255 * ImGui::GetStyle().Alpha * (48.0f / 255.0f));
                }

                ImGui::EndHorizontal();
            }

            ImGui::BeginHorizontal("content_frame");
            ImGui::Spring(1, padding);

            ImGui::BeginVertical("content", ImVec2(0.0f, 0.0f));
            ImGui::Dummy(ImVec2(160, 0));
            ImGui::Spring(1);
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.0f, 0.0f, 0.0f, 1.0f));
            ImGui::TextUnformatted(node.Name.c_str());
            ImGui::PopStyleColor();
            ImGui::Spring(1);
            ImGui::EndVertical();
            auto contentRect = ImGui_GetItemRect();

            ImGui::Spring(1, padding);
            ImGui::EndHorizontal();

            if (!node.Outputs.empty())
            {
                ImGui::BeginHorizontal("outputs");
                ImGui::Spring(1, 0);

                ImRect outputsRect;
                int outputAlpha = 200;
                for (auto& pin : node.Outputs)
                {
                    ImGui::Dummy(ImVec2(padding, padding));
                    outputsRect = ImGui_GetItemRect();
                    ImGui::Spring(1, 0);
                    outputsRect.Min.y += padding;
                    outputsRect.Max.y += padding;

#if IMGUI_VERSION_NUM > 18101
                    const auto allRoundCornersFlags = ImDrawFlags_RoundCornersAll;
                    const auto topRoundCornersFlags = ImDrawFlags_RoundCornersTop;
#else
                    const auto allRoundCornersFlags = 15;
                    const auto topRoundCornersFlags = 3;
#endif

                    ed::PushStyleVar(ed::StyleVar_PinCorners, topRoundCornersFlags);
                    ed::BeginPin(pin.ID, ed::PinKind::Output);
                    ed::PinPivotRect(outputsRect.GetCenter(), outputsRect.GetCenter());
                    ed::PinRect(outputsRect.GetTL(), outputsRect.GetBR());
                    ed::EndPin();
                    ed::PopStyleVar();

                    auto drawList = ImGui::GetWindowDrawList();
                    drawList->AddRectFilled(outputsRect.GetTL(), outputsRect.GetBR(),
                        IM_COL32((int)(255 * pinBackground.x), (int)(255 * pinBackground.y), (int)(255 * pinBackground.z), outputAlpha), 4.0f, allRoundCornersFlags);
                    drawList->AddRect(outputsRect.GetTL(), outputsRect.GetBR(),
                        IM_COL32((int)(255 * pinBackground.x), (int)(255 * pinBackground.y), (int)(255 * pinBackground.z), outputAlpha), 4.0f, allRoundCornersFlags);

                    if (newLinkPin && !CanCreateLink(newLinkPin, &pin) && &pin != newLinkPin)
                        outputAlpha = (int)(255 * ImGui::GetStyle().Alpha * (48.0f / 255.0f));
                }

                ImGui::EndHorizontal();
            }

            ImGui::EndVertical();

            ed::EndNode();
            ed::PopStyleVar(7);
            ed::PopStyleColor(4);
        }

        // ================================================================
        // Comment 节点
        // ================================================================
        for (auto& node : m_Nodes)
        {
            if (node.Type != NodeType::Comment)
                continue;

            const float commentAlpha = 0.75f;

            ImGui::PushStyleVar(ImGuiStyleVar_Alpha, commentAlpha);
            ed::PushStyleColor(ed::StyleColor_NodeBg, ImColor(255, 255, 255, 64));
            ed::PushStyleColor(ed::StyleColor_NodeBorder, ImColor(255, 255, 255, 64));
            ed::BeginNode(node.ID);
            ImGui::PushID(node.ID.AsPointer());
            ImGui::BeginVertical("content");
            ImGui::BeginHorizontal("horizontal");
            ImGui::Spring(1);
            ImGui::TextUnformatted(node.Name.c_str());
            ImGui::Spring(1);
            ImGui::EndHorizontal();
            ed::Group(node.Size);
            ImGui::EndVertical();
            ImGui::PopID();
            ed::EndNode();
            ed::PopStyleColor(2);
            ImGui::PopStyleVar();

            if (ed::BeginGroupHint(node.ID))
            {
                auto bgAlpha = static_cast<int>(ImGui::GetStyle().Alpha * 255);

                auto min = ed::GetGroupMin();

                ImGui::SetCursorScreenPos(min - ImVec2(-8, ImGui::GetTextLineHeightWithSpacing() + 4));
                ImGui::BeginGroup();
                ImGui::TextUnformatted(node.Name.c_str());
                ImGui::EndGroup();

                auto drawList = ed::GetHintBackgroundDrawList();

                auto hintBounds      = ImGui_GetItemRect();
                auto hintFrameBounds = ImRect_Expanded(hintBounds, 8, 4);

                drawList->AddRectFilled(
                    hintFrameBounds.GetTL(),
                    hintFrameBounds.GetBR(),
                    IM_COL32(255, 255, 255, 64 * bgAlpha / 255), 4.0f);

                drawList->AddRect(
                    hintFrameBounds.GetTL(),
                    hintFrameBounds.GetBR(),
                    IM_COL32(255, 255, 255, 128 * bgAlpha / 255), 4.0f);
            }
            ed::EndGroupHint();
        }

        // ================================================================
        // 链接
        // ================================================================
        for (auto& link : m_Links)
        {
            // 跳过连接到隐藏引脚的 link（双重保险，正常情况下隐藏时已移除 link）
            auto* startPin = FindPin(link.StartPinID);
            auto* endPin   = FindPin(link.EndPinID);
            if ((startPin && startPin->IsHidden) || (endPin && endPin->IsHidden))
                continue;

            // Any 引脚参与的链接：动态计算颜色（使用非 Any 端的类型颜色）
            ImColor linkColor = link.Color;
            if (startPin && endPin &&
                (startPin->Type == PinType::Any || endPin->Type == PinType::Any))
            {
                linkColor = GetIconColor(GetLinkColor(startPin, endPin));
            }
            ed::Link(link.ID, link.StartPinID, link.EndPinID, linkColor, 2.0f);
        }

        // ================================================================
        // 创建 / 删除 交互
        // ================================================================
        if (!createNewNode)
        {
            if (ed::BeginCreate(ImColor(255, 255, 255), 2.0f))
            {
                auto showLabel = [](const char* label, ImColor color)
                {
                    ImGui::SetCursorPosY(ImGui::GetCursorPosY() - ImGui::GetTextLineHeight());
                    auto size = ImGui::CalcTextSize(label);

                    auto padding = ImGui::GetStyle().FramePadding;
                    auto spacing = ImGui::GetStyle().ItemSpacing;

                    ImGui::SetCursorPos(ImGui::GetCursorPos() + ImVec2(spacing.x, -spacing.y));

                    auto rectMin = ImGui::GetCursorScreenPos() - padding;
                    auto rectMax = ImGui::GetCursorScreenPos() + size + padding;

                    auto drawList = ImGui::GetWindowDrawList();
                    drawList->AddRectFilled(rectMin, rectMax, color, size.y * 0.15f);
                    ImGui::TextUnformatted(label);
                };

                ed::PinId startPinId = 0, endPinId = 0;
                if (ed::QueryNewLink(&startPinId, &endPinId))
                {
                    auto startPin = FindPin(startPinId);
                    auto endPin   = FindPin(endPinId);

                    newLinkPin = startPin ? startPin : endPin;

                    if (startPin && startPin->Kind == PinKind::Input)
                    {
                        std::swap(startPin, endPin);
                        std::swap(startPinId, endPinId);
                    }

                    if (startPin && endPin)
                    {
                        if (endPin == startPin)
                        {
                            ed::RejectNewItem(ImColor(255, 0, 0), 2.0f);
                        }
                        else if (endPin->Kind == startPin->Kind)
                        {
                            showLabel("x Incompatible Pin Kind", ImColor(45, 32, 32, 180));
                            ed::RejectNewItem(ImColor(255, 0, 0), 2.0f);
                        }
                        else if (endPin->Type != startPin->Type)
                        {
                            // 检查是否是兼容的隐式转换类型
                            if (CanCreateLink(startPin, endPin))
                            {
                                showLabel("+ Create Link (implicit cast)", ImColor(32, 45, 45, 180));
                                if (ed::AcceptNewItem(ImColor(128, 255, 200), 4.0f))
                                {
                                    CreateLinkWithFlowReconnect(startPin, startPinId, endPin, endPinId);
                                }
                            }
                            else
                            {
                                showLabel("x Incompatible Pin Type", ImColor(45, 32, 32, 180));
                                ed::RejectNewItem(ImColor(255, 128, 128), 1.0f);
                            }
                        }
                        else
                        {
                            showLabel("+ Create Link", ImColor(32, 45, 32, 180));
                            if (ed::AcceptNewItem(ImColor(128, 255, 128), 4.0f))
                            {
                                CreateLinkWithFlowReconnect(startPin, startPinId, endPin, endPinId);
                            }
                        }
                    }
                }

                ed::PinId pinId = 0;
                if (ed::QueryNewNode(&pinId))
                {
                    newLinkPin = FindPin(pinId);
                    if (newLinkPin)
                        showLabel("+ Create Node", ImColor(32, 45, 32, 180));

                    if (ed::AcceptNewItem())
                    {
                        createNewNode  = true;
                        newNodeLinkPin = FindPin(pinId);
                        newLinkPin = nullptr;
                        ed::Suspend();
                        ImGui::OpenPopup("Create New Node");
                        ed::Resume();
                    }
                }
            }
            else
                newLinkPin = nullptr;

            ed::EndCreate();

            if (ed::BeginDelete())
            {
                ed::NodeId nodeId = 0;
                while (ed::QueryDeletedNode(&nodeId))
                {
                    if (ed::AcceptDeletedItem())
                    {
                        auto id = std::find_if(m_Nodes.begin(), m_Nodes.end(), [nodeId](auto& node) { return node.ID == nodeId; });
                        if (id != m_Nodes.end())
                        {
                            m_Nodes.erase(id);
                            m_IsDirty = true;
                        }
                    }
                }

                ed::LinkId linkId = 0;
                while (ed::QueryDeletedLink(&linkId))
                {
                    if (ed::AcceptDeletedItem())
                    {
                        auto id = std::find_if(m_Links.begin(), m_Links.end(), [linkId](auto& link) { return link.ID == linkId; });
                        if (id != m_Links.end())
                        {
                            m_Links.erase(id);
                            m_IsDirty = true;
                        }
                    }
                }
            }
            ed::EndDelete();
        }

        ImGui::SetCursorScreenPos(cursorTopLeft);
    }

    // ================================================================
    // 上下文菜单
    // ================================================================
# if 1
    auto openPopupPosition = ImGui::GetMousePos();
    ed::Suspend();
    if (ed::ShowNodeContextMenu(&contextNodeId))
        ImGui::OpenPopup("Node Context Menu");
    else if (ed::ShowPinContextMenu(&contextPinId))
        ImGui::OpenPopup("Pin Context Menu");
    else if (ed::ShowLinkContextMenu(&contextLinkId))
        ImGui::OpenPopup("Link Context Menu");
    else if (ed::ShowBackgroundContextMenu())
    {
        ImGui::OpenPopup("Create New Node");
        newNodeLinkPin = nullptr;
    }
    ed::Resume();

    ed::Suspend();
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8, 8));
    if (ImGui::BeginPopup("Node Context Menu"))
    {
        auto node = FindNode(contextNodeId);

        ImGui::TextUnformatted("Node Context Menu");
        ImGui::Separator();
        if (node)
        {
            ImGui::Text("ID: %p", node->ID.AsPointer());
            ImGui::Text("Type: %s", node->Type == NodeType::Blueprint ? "Blueprint" : (node->Type == NodeType::Tree ? "Tree" : "Comment"));
            ImGui::Text("Inputs: %d", (int)node->Inputs.size());
            ImGui::Text("Outputs: %d", (int)node->Outputs.size());
            if (!node->DefinitionId.empty())
            {
                ImGui::Text("Definition: %s", node->DefinitionId.c_str());
                auto* def = m_NodeRegistry.getNodeDefinition(node->DefinitionId);
                if (def && !def->description.empty())
                {
                    ImGui::Separator();
                    ImGui::TextWrapped("%s", def->description.c_str());
                }
                if (def && !def->category.empty())
                    ImGui::TextColored(ImVec4(0.5f, 0.7f, 1.0f, 1.0f), "Category: %s", def->category.c_str());
            }
        }
        else
            ImGui::Text("Unknown node: %p", contextNodeId.AsPointer());
        ImGui::Separator();
        if (ImGui::MenuItem("Copy", "Ctrl+C"))
        {
            ed::SelectNode(contextNodeId, false);
            CopySelectedNodes();
        }
        if (ImGui::MenuItem("Duplicate", "Ctrl+D"))
        {
            ed::SelectNode(contextNodeId, false);
            DuplicateSelectedNodes();
        }
        if (ImGui::MenuItem("Cut", "Ctrl+X"))
        {
            ed::SelectNode(contextNodeId, false);
            CutSelectedNodes();
        }
        ImGui::Separator();
        if (ImGui::MenuItem("Select Connected"))
        {
            // 选中与此节点直接连接的所有节点
            if (node)
            {
                ed::SelectNode(contextNodeId, false);
                for (const auto& link : m_Links)
                {
                    for (const auto& pin : node->Inputs)
                    {
                        if (link.EndPinID == pin.ID)
                        {
                            auto* srcPin = FindPin(link.StartPinID);
                            if (srcPin && srcPin->Node)
                                ed::SelectNode(srcPin->Node->ID, true);
                        }
                    }
                    for (const auto& pin : node->Outputs)
                    {
                        if (link.StartPinID == pin.ID)
                        {
                            auto* dstPin = FindPin(link.EndPinID);
                            if (dstPin && dstPin->Node)
                                ed::SelectNode(dstPin->Node->ID, true);
                        }
                    }
                }
            }
        }
        ImGui::Separator();
        if (ImGui::MenuItem("Delete"))
        {
            // 删除所有选中的节点（而不是仅删除右键点击的节点）
            int selCount = ed::GetSelectedObjectCount();
            if (selCount > 0)
            {
                std::vector<ed::NodeId> selectedNodeIds(selCount);
                int nodeCount = ed::GetSelectedNodes(selectedNodeIds.data(), selCount);
                selectedNodeIds.resize(nodeCount);

                // 如果右键的节点不在选中列表中，只删除右键的节点
                bool contextInSelection = false;
                for (const auto& id : selectedNodeIds)
                {
                    if (id == contextNodeId) { contextInSelection = true; break; }
                }

                if (contextInSelection && nodeCount > 1)
                {
                    // 删除所有选中的节点
                    for (const auto& nodeId : selectedNodeIds)
                        ed::DeleteNode(nodeId);
                }
                else
                {
                    ed::DeleteNode(contextNodeId);
                }
            }
            else
            {
                ed::DeleteNode(contextNodeId);
            }
        }
        ImGui::EndPopup();
    }

    if (ImGui::BeginPopup("Pin Context Menu"))
    {
        auto pin = FindPin(contextPinId);

        ImGui::TextUnformatted("Pin Context Menu");
        ImGui::Separator();
        if (pin)
        {
            ImGui::Text("ID: %p", pin->ID.AsPointer());
            if (pin->Node)
                ImGui::Text("Node: %p", pin->Node->ID.AsPointer());
            else
                ImGui::Text("Node: %s", "<none>");
        }
        else
            ImGui::Text("Unknown pin: %p", contextPinId.AsPointer());
        ImGui::Separator();
        if (pin && IsPinLinked(contextPinId))
        {
            if (ImGui::MenuItem("Break Link(s)"))
                ed::BreakLinks(contextPinId);
        }
        if (pin && pin->Node)
        {
            if (ImGui::MenuItem("Break All Links on Node"))
            {
                // 断开该节点上所有引脚的所有链接
                auto* node = pin->Node;
                for (auto& p : node->Inputs)
                    ed::BreakLinks(p.ID);
                for (auto& p : node->Outputs)
                    ed::BreakLinks(p.ID);
            }
        }
        // 重置引脚值
        if (pin && pin->Kind == PinKind::Input && pin->Type != PinType::Flow)
        {
            if (ImGui::MenuItem("Reset Value"))
            {
                pin->BoolValue = false;
                pin->IntValue = 0;
                pin->FloatValue = 0.0f;
                pin->StringValue.clear();
                pin->ObjectValue.clear();
                m_IsDirty = true;
            }
        }

        ImGui::EndPopup();
    }

    if (ImGui::BeginPopup("Link Context Menu"))
    {
        auto link = FindLink(contextLinkId);

        ImGui::TextUnformatted("Link Context Menu");
        ImGui::Separator();
        if (link)
        {
            ImGui::Text("ID: %p", link->ID.AsPointer());
            ImGui::Text("From: %p", link->StartPinID.AsPointer());
            ImGui::Text("To: %p", link->EndPinID.AsPointer());
        }
        else
            ImGui::Text("Unknown link: %p", contextLinkId.AsPointer());
        ImGui::Separator();
        if (ImGui::MenuItem("Delete"))
            ed::DeleteLink(contextLinkId);
        ImGui::EndPopup();
    }

    if (ImGui::BeginPopup("Create New Node"))
    {
        auto newNodePostion = openPopupPosition;

        // 如果剪贴板中有节点，提供 Paste Here 选项
        if (!m_ClipboardNodes.empty())
        {
            if (ImGui::MenuItem("Paste Here", "Ctrl+V"))
            {
                ImVec2 canvasPos = ed::ScreenToCanvas(newNodePostion);
                PasteNodes(canvasPos);
            }
            ImGui::Separator();
        }

        Node* node = ShowCreateNodeMenu();

        if (node)
        {
            BuildNodes();

            createNewNode = false;
            m_IsDirty = true;

            ed::SetNodePosition(node->ID, newNodePostion);

            if (auto startPin = newNodeLinkPin)
            {
                auto& pins = startPin->Kind == PinKind::Input ? node->Outputs : node->Inputs;

                for (auto& pin : pins)
                {
                    if (CanCreateLink(startPin, &pin))
                    {
                        auto endPin = &pin;
                        if (startPin->Kind == PinKind::Input)
                            std::swap(startPin, endPin);

                        CreateLinkWithFlowReconnect(startPin, startPin->ID, endPin, endPin->ID);

                        break;
                    }
                }
            }
        }

        ImGui::EndPopup();
    }
    else
        createNewNode = false;
    ImGui::PopStyleVar();
    ed::Resume();
# endif

    // ================================================================
    // 双击节点处理：Execute Blueprint → 延迟打开对应蓝图文件
    // 注意：不能在 ed::Begin/End 块内部调用 DoOpenFile 或切换 EditorContext，
    //       否则会导致 imgui-node-editor 内部状态混乱而崩溃。
    //       这里只记录要打开的路径，在 ed::End() 之后再执行。
    // ================================================================
    {
        auto doubleClickedNodeId = ed::GetDoubleClickedNode();
        if (doubleClickedNodeId)
        {
            auto* node = FindNode(doubleClickedNodeId);
            if (node && node->DefinitionId == "ExecuteBlueprint")
            {
                // 查找 "File" 输入引脚的值
                std::string filePath;
                for (const auto& pin : node->Inputs)
                {
                    if (pin.Name == "File")
                    {
                        filePath = pin.StringValue;
                        break;
                    }
                }

                if (!filePath.empty())
                {
                    // 如果是相对路径，基于当前文档所在目录解析
                    bool isAbsolute = false;
#ifdef _WIN32
                    isAbsolute = (filePath.size() >= 2 && filePath[1] == ':') ||
                                 (filePath.size() >= 2 && filePath[0] == '\\' && filePath[1] == '\\');
#else
                    isAbsolute = (!filePath.empty() && filePath[0] == '/');
#endif
                    if (!isAbsolute && !m_CurrentFilePath.empty())
                    {
                        std::string dir = m_CurrentFilePath;
                        size_t lastSlash = dir.find_last_of("/\\");
                        if (lastSlash != std::string::npos)
                            dir = dir.substr(0, lastSlash + 1);
                        else
                            dir.clear();
                        filePath = dir + filePath;
                    }

                    // 检查文件是否已在某个标签页中打开
                    bool alreadyOpen = false;
                    for (int i = 0; i < (int)m_Documents.size(); ++i)
                    {
                        if (m_Documents[i]->filePath == filePath)
                        {
                            // 延迟切换到已有标签页
                            m_PendingSwitchTabIndex = i;
                            alreadyOpen = true;
                            break;
                        }
                    }

                    if (!alreadyOpen)
                    {
                        // 延迟打开文件（在 ed::End() 之后执行）
                        m_PendingOpenFilePath = filePath;
                    }
                }
                else
                {
                    // File 引脚为空，在执行日志中提示
                    if (ActiveDoc())
                    {
                        m_ExecutionLog.push_back("[INFO] Double-clicked Execute Blueprint node, but File pin is empty.");
                        m_ExecutionLogDirty = true;
                    }
                }
            }
        }
    }

    // 触发执行后的 Flow 动画
    if (!m_FlowLinks.empty())
    {
        for (auto& linkId : m_FlowLinks)
            ed::Flow(linkId);
        m_FlowLinks.clear();
    }

    ed::End();

    // ================================================================
    // 检测节点位置变化（拖拽移动节点 → 标记 dirty）
    // ================================================================
    if (ActiveDoc() && !m_IsDirty)
    {
        auto& lastPositions = ActiveDoc()->lastNodePositions;
        for (const auto& node : m_Nodes)
        {
            ImVec2 curPos = ed::GetNodePosition(node.ID);
            auto it = lastPositions.find(node.ID);
            if (it != lastPositions.end())
            {
                if (it->second.x != curPos.x || it->second.y != curPos.y)
                {
                    m_IsDirty = true;
                    break;
                }
            }
        }
    }
    // 更新上一帧节点位置快照
    if (ActiveDoc())
    {
        auto& lastPositions = ActiveDoc()->lastNodePositions;
        lastPositions.clear();
        for (const auto& node : m_Nodes)
            lastPositions[node.ID] = ed::GetNodePosition(node.ID);
    }

    // ================================================================
    // 延迟处理双击打开文件（必须在 ed::End() 之后执行）
    // ================================================================
    if (m_PendingSwitchTabIndex >= 0)
    {
        m_ActiveDocIndex = m_PendingSwitchTabIndex;
        ed::SetCurrentEditor(ActiveDoc()->editorContext);
        m_NeedNavigateToContent = 1;
        m_PendingSwitchTabIndex = -1;
    }
    else if (!m_PendingOpenFilePath.empty())
    {
        std::string pathToOpen = m_PendingOpenFilePath;
        m_PendingOpenFilePath.clear();
        DoOpenFile(pathToOpen);
    }

    editorMin = ImGui::GetItemRectMin();
    editorMax = ImGui::GetItemRectMax();

    if (m_ShowOrdinals)
    {
        int nodeCount = ed::GetNodeCount();
        std::vector<ed::NodeId> orderedNodeIds;
        orderedNodeIds.resize(static_cast<size_t>(nodeCount));
        ed::GetOrderedNodeIds(orderedNodeIds.data(), nodeCount);

        auto drawList = ImGui::GetWindowDrawList();
        drawList->PushClipRect(editorMin, editorMax);

        int ordinal = 0;
        for (auto& nodeId : orderedNodeIds)
        {
            auto p0 = ed::GetNodePosition(nodeId);
            auto p1 = p0 + ed::GetNodeSize(nodeId);
            p0 = ed::CanvasToScreen(p0);
            p1 = ed::CanvasToScreen(p1);

            ImGuiTextBuffer builder;
            builder.appendf("#%d", ordinal++);

            auto textSize   = ImGui::CalcTextSize(builder.c_str());
            auto padding    = ImVec2(2.0f, 2.0f);
            auto widgetSize = textSize + padding * 2;

            auto widgetPosition = ImVec2(p1.x, p0.y) + ImVec2(0.0f, -widgetSize.y);

            drawList->AddRectFilled(widgetPosition, widgetPosition + widgetSize, IM_COL32(100, 80, 80, 190), 3.0f, ImDrawFlags_RoundCornersAll);
            drawList->AddRect(widgetPosition, widgetPosition + widgetSize, IM_COL32(200, 160, 160, 190), 3.0f, ImDrawFlags_RoundCornersAll);
            drawList->AddText(widgetPosition + padding, IM_COL32(255, 255, 255, 255), builder.c_str());
        }

        drawList->PopClipRect();
    }

        } // end ed::Begin scope

        // --- 底部执行输出面板 ---
        if (m_ShowExecutionWindow)
        {
            ImGui::BeginChild("##BottomPanel", ImVec2(rightWidth, bottomHeight), true);
            DrawExecutionPanel();
            ImGui::EndChild();
        }
    }
    ImGui::EndGroup();

    // ================================================================
    // 计时器监控浮动窗口
    // ================================================================
    if (m_ShowTimerWindow)
        DrawTimerPanel();

    // ================================================================
    // 小地图
    // ================================================================
    if (m_ShowMinimap && ActiveDoc())
        DrawMinimap(editorMin, editorMax);

    // ================================================================
    // 画布节点搜索覆盖层（Ctrl+F）
    // ================================================================
    DrawSearchOverlay();

    // ================================================================
    // 未保存修改确认对话框
    // ================================================================
    ShowUnsavedChangesDialog();

    // ================================================================
    // 样式编辑器浮动窗口
    // ================================================================
    if (m_ShowStyleEditorWindow)
        ShowStyleEditor(&m_ShowStyleEditorWindow);
}

// ============================================================================
// 未保存修改确认对话框
// ============================================================================

void BlueprintEditor::ShowUnsavedChangesDialog()
{
    if (!m_ShowUnsavedDialog) return;

    ImGui::OpenPopup("Unsaved Changes###UnsavedDlg");
    m_ShowUnsavedDialog = false;  // 只触发一次 OpenPopup

    // 保持 popup 持续显示
    ImVec2 center = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

    if (ImGui::BeginPopupModal("Unsaved Changes###UnsavedDlg", nullptr,
        ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings))
    {
        std::string docName = "New";
        if (m_PendingCloseTabIndex >= 0 && m_PendingCloseTabIndex < static_cast<int>(m_Documents.size()))
            docName = m_Documents[m_PendingCloseTabIndex]->GetTabName();

        ImGui::Text("Document \"%s\" has unsaved changes.", docName.c_str());
        ImGui::Text("Do you want to save before closing?");
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        float buttonWidth = 100.0f;
        float totalWidth = buttonWidth * 3 + ImGui::GetStyle().ItemSpacing.x * 2;
        float startX = (ImGui::GetContentRegionAvail().x - totalWidth) * 0.5f;
        if (startX > 0) ImGui::SetCursorPosX(ImGui::GetCursorPosX() + startX);

        if (ImGui::Button("Save", ImVec2(buttonWidth, 0)))
        {
            // 切换到待关闭的文档并保存
            if (m_PendingCloseTabIndex >= 0)
            {
                int oldActiveIdx = m_ActiveDocIndex;
                m_ActiveDocIndex = m_PendingCloseTabIndex;
                ed::SetCurrentEditor(ActiveDoc()->editorContext);
                SaveFile();
                // 保存后关闭
                CloseDocument(m_PendingCloseTabIndex);
                m_PendingCloseTabIndex = -1;
                // 恢复活跃索引
                if (oldActiveIdx >= static_cast<int>(m_Documents.size()))
                    m_ActiveDocIndex = static_cast<int>(m_Documents.size()) - 1;
            }
            ImGui::CloseCurrentPopup();
        }

        ImGui::SameLine();
        if (ImGui::Button("Don't Save", ImVec2(buttonWidth, 0)))
        {
            // 不保存直接关闭
            if (m_PendingCloseTabIndex >= 0)
            {
                CloseDocument(m_PendingCloseTabIndex);
                m_PendingCloseTabIndex = -1;
            }
            ImGui::CloseCurrentPopup();
        }

        ImGui::SameLine();
        if (ImGui::Button("Cancel", ImVec2(buttonWidth, 0)))
        {
            m_PendingCloseTabIndex = -1;
            m_PendingQuitApp = false;
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }
}

// ============================================================================
// 左侧节点列表面板（嵌入式）
// ============================================================================

void BlueprintEditor::DrawNodeListPanel()
{
    auto& io = ImGui::GetIO();
    float paneWidth = ImGui::GetContentRegionAvail().x;

    // 工具栏按钮
    static bool showStyleEditor = false;
    ImGui::BeginHorizontal("Style Editor", ImVec2(paneWidth, 0));
    ImGui::Spring(0.0f, 0.0f);
    if (ImGui::Button("Zoom to Content"))
        ed::NavigateToContent();
    ImGui::Spring(0.0f);
    if (ImGui::Button("Show Flow"))
    {
        for (auto& link : m_Links)
            ed::Flow(link.ID);
    }
    ImGui::Spring();
    if (ImGui::Button("Edit Style"))
        showStyleEditor = true;
    ImGui::EndHorizontal();
    ImGui::Checkbox("Show Ordinals", &m_ShowOrdinals);

    if (showStyleEditor)
        ShowStyleEditor(&showStyleEditor);

    // 选中节点信息
    std::vector<ed::NodeId> selectedNodes;
    std::vector<ed::LinkId> selectedLinks;
    selectedNodes.resize(ed::GetSelectedObjectCount());
    selectedLinks.resize(ed::GetSelectedObjectCount());

    int nodeCount = ed::GetSelectedNodes(selectedNodes.data(), static_cast<int>(selectedNodes.size()));
    int linkCount = ed::GetSelectedLinks(selectedLinks.data(), static_cast<int>(selectedLinks.size()));

    selectedNodes.resize(nodeCount);
    selectedLinks.resize(linkCount);

    int saveIconWidth     = GetTextureWidth(m_SaveIcon);
    int saveIconHeight    = GetTextureHeight(m_SaveIcon);
    int restoreIconWidth  = GetTextureWidth(m_RestoreIcon);
    int restoreIconHeight = GetTextureHeight(m_RestoreIcon);

    if (saveIconWidth <= 0)    saveIconWidth    = 24;
    if (saveIconHeight <= 0)   saveIconHeight   = 24;
    if (restoreIconWidth <= 0) restoreIconWidth = 24;
    if (restoreIconHeight <= 0) restoreIconHeight = 24;

    // 节点列表
    ImGui::GetWindowDrawList()->AddRectFilled(
        ImGui::GetCursorScreenPos(),
        ImGui::GetCursorScreenPos() + ImVec2(paneWidth, ImGui::GetTextLineHeight()),
        ImColor(ImGui::GetStyle().Colors[ImGuiCol_HeaderActive]), ImGui::GetTextLineHeight() * 0.25f);
    ImGui::Spacing(); ImGui::SameLine();
    ImGui::TextUnformatted("Nodes");
    ImGui::Indent();
    for (auto& node : m_Nodes)
    {
        ImGui::PushID(node.ID.AsPointer());
        auto start = ImGui::GetCursorScreenPos();

        if (const auto progress = GetTouchProgress(node.ID))
        {
            ImGui::GetWindowDrawList()->AddLine(
                start + ImVec2(-8, 0),
                start + ImVec2(-8, ImGui::GetTextLineHeight()),
                IM_COL32(255, 0, 0, 255 - (int)(255 * progress)), 4.0f);
        }

        bool isSelected = std::find(selectedNodes.begin(), selectedNodes.end(), node.ID) != selectedNodes.end();
# if IMGUI_VERSION_NUM >= 18967
        ImGui::SetNextItemAllowOverlap();
# endif
        if (ImGui::Selectable((node.Name + "##" + std::to_string(reinterpret_cast<uintptr_t>(node.ID.AsPointer()))).c_str(), &isSelected))
        {
            if (io.KeyCtrl)
            {
                if (isSelected)
                    ed::SelectNode(node.ID, true);
                else
                    ed::DeselectNode(node.ID);
            }
            else
                ed::SelectNode(node.ID, false);

            ed::NavigateToSelection();
        }
        if (ImGui::IsItemHovered() && !node.State.empty())
            ImGui::SetTooltip("State: %s", node.State.c_str());

        auto id = std::string("(") + std::to_string(reinterpret_cast<uintptr_t>(node.ID.AsPointer())) + ")";
        auto textSize = ImGui::CalcTextSize(id.c_str(), nullptr);
        auto iconPanelPos = start + ImVec2(
            paneWidth - ImGui::GetStyle().FramePadding.x - ImGui::GetStyle().IndentSpacing - saveIconWidth - restoreIconWidth - ImGui::GetStyle().ItemInnerSpacing.x * 1,
            (ImGui::GetTextLineHeight() - saveIconHeight) / 2);
        ImGui::GetWindowDrawList()->AddText(
            ImVec2(iconPanelPos.x - textSize.x - ImGui::GetStyle().ItemInnerSpacing.x, start.y),
            IM_COL32(255, 255, 255, 255), id.c_str(), nullptr);

        auto drawList = ImGui::GetWindowDrawList();
        ImGui::SetCursorScreenPos(iconPanelPos);
# if IMGUI_VERSION_NUM < 18967
        ImGui::SetItemAllowOverlap();
# else
        ImGui::SetNextItemAllowOverlap();
# endif
        if (node.SavedState.empty())
        {
            if (ImGui::InvisibleButton("save", ImVec2((float)saveIconWidth, (float)saveIconHeight)))
                node.SavedState = node.State;

            if (ImGui::IsItemActive())
                drawList->AddImage(m_SaveIcon, ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), ImVec2(0, 0), ImVec2(1, 1), IM_COL32(255, 255, 255, 96));
            else if (ImGui::IsItemHovered())
                drawList->AddImage(m_SaveIcon, ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), ImVec2(0, 0), ImVec2(1, 1), IM_COL32(255, 255, 255, 255));
            else
                drawList->AddImage(m_SaveIcon, ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), ImVec2(0, 0), ImVec2(1, 1), IM_COL32(255, 255, 255, 160));
        }
        else
        {
            ImGui::Dummy(ImVec2((float)saveIconWidth, (float)saveIconHeight));
            drawList->AddImage(m_SaveIcon, ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), ImVec2(0, 0), ImVec2(1, 1), IM_COL32(255, 255, 255, 32));
        }

        ImGui::SameLine(0, ImGui::GetStyle().ItemInnerSpacing.x);
# if IMGUI_VERSION_NUM < 18967
        ImGui::SetItemAllowOverlap();
# else
        ImGui::SetNextItemAllowOverlap();
# endif
        if (!node.SavedState.empty())
        {
            if (ImGui::InvisibleButton("restore", ImVec2((float)restoreIconWidth, (float)restoreIconHeight)))
            {
                node.State = node.SavedState;
                ed::RestoreNodeState(node.ID);
                node.SavedState.clear();
            }

            if (ImGui::IsItemActive())
                drawList->AddImage(m_RestoreIcon, ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), ImVec2(0, 0), ImVec2(1, 1), IM_COL32(255, 255, 255, 96));
            else if (ImGui::IsItemHovered())
                drawList->AddImage(m_RestoreIcon, ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), ImVec2(0, 0), ImVec2(1, 1), IM_COL32(255, 255, 255, 255));
            else
                drawList->AddImage(m_RestoreIcon, ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), ImVec2(0, 0), ImVec2(1, 1), IM_COL32(255, 255, 255, 160));
        }
        else
        {
            ImGui::Dummy(ImVec2((float)restoreIconWidth, (float)restoreIconHeight));
            drawList->AddImage(m_RestoreIcon, ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), ImVec2(0, 0), ImVec2(1, 1), IM_COL32(255, 255, 255, 32));
        }

        ImGui::SameLine(0, 0);
# if IMGUI_VERSION_NUM < 18967
        ImGui::SetItemAllowOverlap();
# endif
        ImGui::Dummy(ImVec2(0, (float)restoreIconHeight));

        ImGui::PopID();
    }
    ImGui::Unindent();

    // 选择信息
    static int changeCount = 0;

    ImGui::GetWindowDrawList()->AddRectFilled(
        ImGui::GetCursorScreenPos(),
        ImGui::GetCursorScreenPos() + ImVec2(paneWidth, ImGui::GetTextLineHeight()),
        ImColor(ImGui::GetStyle().Colors[ImGuiCol_HeaderActive]), ImGui::GetTextLineHeight() * 0.25f);
    ImGui::Spacing(); ImGui::SameLine();
    ImGui::TextUnformatted("Selection");

    ImGui::BeginHorizontal("Selection Stats", ImVec2(paneWidth, 0));
    ImGui::Text("Changed %d time%s", changeCount, changeCount > 1 ? "s" : "");
    ImGui::Spring();
    if (ImGui::Button("Deselect All"))
        ed::ClearSelection();
    ImGui::EndHorizontal();
    ImGui::Indent();
    for (int i = 0; i < nodeCount; ++i) ImGui::Text("Node (%p)", selectedNodes[i].AsPointer());
    for (int i = 0; i < linkCount; ++i) ImGui::Text("Link (%p)", selectedLinks[i].AsPointer());
    ImGui::Unindent();

    if (ImGui::IsKeyPressed(ImGuiKey_Z))
        for (auto& link : m_Links)
            ed::Flow(link.ID);

    if (ed::HasSelectionChanged())
        ++changeCount;
}

// ============================================================================
// 底部执行输出面板（嵌入式）
// ============================================================================

void BlueprintEditor::DrawExecutionPanel()
{
    float paneWidth = ImGui::GetContentRegionAvail().x;

    // 执行按钮栏
    ImGui::BeginHorizontal("ExecButtons", ImVec2(paneWidth, 0));
    if (ImGui::Button("Execute", ImVec2(80, 0)))
    {
        ExecuteBlueprint();
    }
    ImGui::Spring(0.0f);
    if (ImGui::Button("Copy Log", ImVec2(80, 0)))
    {
        if (!m_ExecutionLog.empty())
        {
            std::string allText;
            for (const auto& line : m_ExecutionLog)
            {
                allText += line;
                allText += '\n';
            }
            ImGui::SetClipboardText(allText.c_str());
        }
    }
    ImGui::Spring(0.0f);
    if (ImGui::Button("Clear Log", ImVec2(80, 0)))
    {
        m_ExecutionLog.clear();
        m_ExecutionLogText.clear();
        m_ExecutionLogDirty = false;
        m_LastExecutionStatus.clear();
    }
    ImGui::Spring();
    ImGui::EndHorizontal();

    // 状态信息
    if (!m_LastExecutionStatus.empty())
    {
        bool isOk = m_LastExecutionStatus.find("OK") == 0;
        ImGui::TextColored(isOk ? ImVec4(0.4f, 1.0f, 0.4f, 1.0f) : ImVec4(1.0f, 0.4f, 0.4f, 1.0f),
            "Status: %s", m_LastExecutionStatus.c_str());
    }

    // 日志输出区域
    float logHeight = ImGui::GetContentRegionAvail().y;
    if (logHeight < 60.0f) logHeight = 60.0f;

    if (m_ExecutionLogDirty)
    {
        m_ExecutionLogText.clear();
        for (const auto& line : m_ExecutionLog)
        {
            m_ExecutionLogText += line;
            m_ExecutionLogText += '\n';
        }
        m_ExecutionLogDirty = false;
    }

    ImGui::InputTextMultiline("##ExecutionLog",
        const_cast<char*>(m_ExecutionLogText.c_str()),
        m_ExecutionLogText.size() + 1,
        ImVec2(paneWidth, logHeight),
        ImGuiInputTextFlags_ReadOnly);
}

// ============================================================================
// 计时器监控面板（浮动窗口）
// ============================================================================

void BlueprintEditor::DrawTimerPanel()
{
    ImGui::SetNextWindowSize(ImVec2(520, 340), ImGuiCond_FirstUseEver);
    if (!ImGui::Begin("Timer Monitor", &m_ShowTimerWindow))
    {
        ImGui::End();
        return;
    }

    // 工具栏
    auto& timerMgr = ActiveDoc()->GetTimerManager();
    ImGui::Text("Active Timers: %d", timerMgr.GetActiveTimerCount());
    ImGui::SameLine();
    ImGui::Text("  |  Time Scale: ");
    ImGui::SameLine();
    float ts = timerMgr.GetTimeScale();
    ImGui::SetNextItemWidth(100);
    if (ImGui::SliderFloat("##TimeScale", &ts, 0.0f, 5.0f, "%.2f"))
        timerMgr.SetTimeScale(ts);

    ImGui::SameLine(0, 20);
    if (ImGui::Button("Clear All"))
        timerMgr.ClearAllTimers();
    ImGui::SameLine();
    if (ImGui::Button("Pause All"))
        timerMgr.PauseAll();
    ImGui::SameLine();
    if (ImGui::Button("Resume All"))
        timerMgr.ResumeAll();

    ImGui::Separator();

    // 快速创建测试计时器
    static float testInterval = 1.0f;
    static int   testRepeat   = -1;
    static char  testName[64] = "Test";
    ImGui::Text("Quick Timer:");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(80);
    ImGui::InputText("##Name", testName, sizeof(testName));
    ImGui::SameLine();
    ImGui::SetNextItemWidth(60);
    ImGui::InputFloat("##Interval", &testInterval, 0.0f, 0.0f, "%.1fs");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(50);
    ImGui::InputInt("##Repeat", &testRepeat);
    ImGui::SameLine();
    if (ImGui::Button("Add"))
    {
        std::string timerName(testName);
        ActiveDoc()->GetTimerManager().SetTimerByName(timerName, testInterval, testRepeat, [this, timerName]() {
            m_ExecutionLog.push_back("[Timer:" + timerName + "] fired!");
            m_ExecutionLogDirty = true;
            return true;
        });
    }

    ImGui::Separator();

    // 计时器列表表格
    const auto& timers = timerMgr.GetAllTimers();
    if (timers.empty())
    {
        ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "No active timers");
    }
    else
    {
        if (ImGui::BeginTable("##Timers", 7, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable | ImGuiTableFlags_ScrollY))
        {
            ImGui::TableSetupColumn("Handle", ImGuiTableColumnFlags_WidthFixed, 50.0f);
            ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthFixed, 80.0f);
            ImGui::TableSetupColumn("Interval", ImGuiTableColumnFlags_WidthFixed, 60.0f);
            ImGui::TableSetupColumn("Remaining", ImGuiTableColumnFlags_WidthFixed, 70.0f);
            ImGui::TableSetupColumn("Fired", ImGuiTableColumnFlags_WidthFixed, 45.0f);
            ImGui::TableSetupColumn("State", ImGuiTableColumnFlags_WidthFixed, 60.0f);
            ImGui::TableSetupColumn("Actions", ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableHeadersRow();

            for (const auto& t : timers)
            {
                if (t.pendingKill) continue;

                ImGui::TableNextRow();

                // Handle
                ImGui::TableNextColumn();
                ImGui::Text("#%u", t.handle);

                // Name
                ImGui::TableNextColumn();
                ImGui::TextUnformatted(t.name.empty() ? "-" : t.name.c_str());

                // Interval
                ImGui::TableNextColumn();
                ImGui::Text("%.2fs", t.interval);

                // Remaining
                ImGui::TableNextColumn();
                float pct = t.interval > 0.0f ? (1.0f - t.remaining / t.interval) : 1.0f;
                if (pct < 0.0f) pct = 0.0f;
                if (pct > 1.0f) pct = 1.0f;
                char overlay[32];
                snprintf(overlay, sizeof(overlay), "%.2fs", t.remaining > 0.0f ? t.remaining : 0.0f);
                ImGui::ProgressBar(pct, ImVec2(-1, 0), overlay);

                // Fire count
                ImGui::TableNextColumn();
                ImGui::Text("%d", t.fireCount);

                // State
                ImGui::TableNextColumn();
                if (t.paused)
                    ImGui::TextColored(ImVec4(1.0f, 0.6f, 0.0f, 1.0f), "Paused");
                else if (t.repeatCount == -1)
                    ImGui::TextColored(ImVec4(0.2f, 0.8f, 0.2f, 1.0f), "Loop");
                else
                    ImGui::TextColored(ImVec4(0.4f, 0.7f, 1.0f, 1.0f), "x%d", t.repeatCount);

                // Actions
                ImGui::TableNextColumn();
                ImGui::PushID(t.handle);
                if (t.paused)
                {
                    if (ImGui::SmallButton("Resume"))
                        timerMgr.ResumeTimer(t.handle);
                }
                else
                {
                    if (ImGui::SmallButton("Pause"))
                        timerMgr.PauseTimer(t.handle);
                }
                ImGui::SameLine();
                if (ImGui::SmallButton("X"))
                    timerMgr.ClearTimer(t.handle);
                ImGui::PopID();
            }

            ImGui::EndTable();
        }
    }

    ImGui::End();
}
