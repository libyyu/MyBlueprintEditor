// EditorUI.cpp -- 蓝图编辑器 UI 渲染
#include "BlueprintEditor.h"

extern ed::EditorContext* s_Editor; // defined in BlueprintEditor.cpp

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
    }
};

// ============================================================================
// 引脚图标绘制
// ============================================================================

void BlueprintEditor::DrawPinIcon(const Pin& pin, bool connected, int alpha)
{
    IconType iconType;
    ImColor  color = GetIconColor(pin.Type);
    color.Value.w = alpha / 255.0f;
    switch (pin.Type)
    {
        case PinType::Flow:     iconType = IconType::Flow;   break;
        case PinType::Bool:     iconType = IconType::Circle; break;
        case PinType::Int:      iconType = IconType::Circle; break;
        case PinType::Float:    iconType = IconType::Circle; break;
        case PinType::String:   iconType = IconType::Circle; break;
        case PinType::Object:   iconType = IconType::Circle; break;
        case PinType::Function: iconType = IconType::Circle; break;
        case PinType::Delegate: iconType = IconType::Square; break;
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
    UpdateTouch();

    auto& io = ImGui::GetIO();

    // 菜单栏
    if (ImGui::BeginMenuBar())
    {
        if (ImGui::BeginMenu("File"))
        {
            if (ImGui::MenuItem("New", "Ctrl+N"))
                NewFile();
            if (ImGui::MenuItem("Open...", "Ctrl+O"))
                OpenFile();
            ImGui::Separator();
            if (ImGui::MenuItem("Save", "Ctrl+S"))
                SaveFile();
            if (ImGui::MenuItem("Save As...", "Ctrl+Shift+S"))
                SaveFileAs();
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("View"))
        {
            ImGui::MenuItem("Node List", nullptr, &m_ShowNodeListWindow);
            ImGui::MenuItem("Execution Output", nullptr, &m_ShowExecutionWindow);
            ImGui::MenuItem("Show Ordinals", nullptr, &m_ShowOrdinals);
            ImGui::EndMenu();
        }
        ImGui::Separator();

        // 显示当前文件名
        if (!m_CurrentFilePath.empty())
        {
            // 提取文件名
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

        ImGui::Text("FPS: %.2f (%.2gms)", io.Framerate, io.Framerate ? 1000.0f / io.Framerate : 0.0f);
        ImGui::EndMenuBar();
    }

    // 键盘快捷键
    if (io.KeyCtrl && !io.KeyShift && ImGui::IsKeyPressed(ImGuiKey_N))
        NewFile();
    if (io.KeyCtrl && !io.KeyShift && ImGui::IsKeyPressed(ImGuiKey_O))
        OpenFile();
    if (io.KeyCtrl && !io.KeyShift && ImGui::IsKeyPressed(ImGuiKey_S))
        SaveFile();
    if (io.KeyCtrl && io.KeyShift && ImGui::IsKeyPressed(ImGuiKey_S))
        SaveFileAs();

    ed::SetCurrentEditor(s_Editor);

    // 加载文件后延迟设置节点位置（需要在 ed::Begin/End 之间）
    if (m_NeedSetNodePositions)
    {
        m_NeedSetNodePositions = false;

        for (const auto& pendNode : m_PendingLoadData.nodes)
        {
            auto it = pendNode.customProperties.find("__newEditorId");
            if (it != pendNode.customProperties.end())
            {
                int editorId = std::stoi(it->second);
                ed::NodeId nodeId(editorId);

                if (pendNode.position.x != 0.0f || pendNode.position.y != 0.0f)
                    ed::SetNodePosition(nodeId, ImVec2(pendNode.position.x, pendNode.position.y));

                // Comment 节点需要设置 Group 尺寸
                if (pendNode.size.width > 0 && pendNode.size.height > 0)
                {
                    Node* node = FindNode(nodeId);
                    if (node && node->Type == NodeType::Comment)
                    {
                        ed::SetGroupSize(nodeId, ImVec2(pendNode.size.width, pendNode.size.height));
                        node->Size = ImVec2(pendNode.size.width, pendNode.size.height);
                    }
                }
            }
        }

        m_PendingLoadData.clear();
        ed::NavigateToContent();
    }

    // 浮动面板窗口
    if (m_ShowNodeListWindow)
        ShowNodeListWindow(&m_ShowNodeListWindow);
    if (m_ShowExecutionWindow)
        ShowExecutionWindow(&m_ShowExecutionWindow);

    static ed::NodeId contextNodeId      = 0;
    static ed::LinkId contextLinkId      = 0;
    static ed::PinId  contextPinId       = 0;
    static bool createNewNode  = false;
    static Pin* newNodeLinkPin = nullptr;
    static Pin* newLinkPin     = nullptr;

    ed::Begin("Node editor");
    {
        auto cursorTopLeft = ImGui::GetCursorScreenPos();

        util::BlueprintNodeBuilder builder(m_HeaderBackground, GetTextureWidth(m_HeaderBackground), GetTextureHeight(m_HeaderBackground));

        for (auto& node : m_Nodes)
        {
            if (node.Type != NodeType::Blueprint && node.Type != NodeType::Simple)
                continue;

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
                    auto alpha = ImGui::GetStyle().Alpha;
                    if (newLinkPin && !CanCreateLink(newLinkPin, &input) && &input != newLinkPin)
                        alpha = alpha * (48.0f / 255.0f);

                    builder.Input(input.ID);
                    ImGui::PushStyleVar(ImGuiStyleVar_Alpha, alpha);
                    DrawPinIcon(input, IsPinLinked(input.ID), (int)(alpha * 255));
                    ImGui::Spring(0);
                    if (!input.Name.empty())
                    {
                        ImGui::TextUnformatted(input.Name.c_str());
                        ImGui::Spring(0);
                    }

                    // 当输入引脚未连线时，基础数据类型显示内联编辑控件
                    if (!IsPinLinked(input.ID) && input.Type != PinType::Flow && input.Type != PinType::Delegate)
                    {
                        ImGui::PushID(input.ID.AsPointer());

                        static std::unordered_map<uintptr_t, std::array<char, 128>> s_StringBuffers;

                        if (input.Type == PinType::Bool)
                        {
                            ImGui::Checkbox("##value", &input.BoolValue);
                        }
                        else if (input.Type == PinType::Int)
                        {
                            ImGui::SetNextItemWidth(80.0f);
                            ImGui::DragInt("##value", &input.IntValue, 1.0f);
                        }
                        else if (input.Type == PinType::Float)
                        {
                            ImGui::SetNextItemWidth(80.0f);
                            ImGui::DragFloat("##value", &input.FloatValue, 0.01f);
                        }
                        else if (input.Type == PinType::String)
                        {
                            auto key = reinterpret_cast<uintptr_t>(input.ID.AsPointer());
                            auto& buf = s_StringBuffers[key];
                            if (buf[0] == '\0' && !input.StringValue.empty())
                                snprintf(buf.data(), buf.size(), "%s", input.StringValue.c_str());
                            ImGui::SetNextItemWidth(100.0f);
                            if (ImGui::InputText("##value", buf.data(), buf.size()))
                                input.StringValue = buf.data();
                        }
                        else if (input.Type == PinType::Object || input.Type == PinType::Function)
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

                    ImGui::PopStyleVar();
                    builder.EndInput();
                }

                if (isSimple)
                {
                    builder.Middle();

                    ImGui::Spring(1, 0);
                    ImGui::TextUnformatted(node.Name.c_str());
                    ImGui::Spring(1, 0);
                }

                for (auto& output : node.Outputs)
                {
                    if (!isSimple && output.Type == PinType::Delegate)
                        continue;

                    auto alpha = ImGui::GetStyle().Alpha;
                    if (newLinkPin && !CanCreateLink(newLinkPin, &output) && &output != newLinkPin)
                        alpha = alpha * (48.0f / 255.0f);

                    builder.Output(output.ID);
                    ImGui::PushStyleVar(ImGuiStyleVar_Alpha, alpha);
                    if (!output.Name.empty())
                    {
                        ImGui::Spring(0);
                        ImGui::TextUnformatted(output.Name.c_str());
                    }
                    ImGui::Spring(0);
                    DrawPinIcon(output, IsPinLinked(output.ID), (int)(alpha * 255));
                    ImGui::PopStyleVar();
                    builder.EndOutput();
                }

            builder.End();
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
            ed::Link(link.ID, link.StartPinID, link.EndPinID, link.Color, 2.0f);

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

                    if (startPin->Kind == PinKind::Input)
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
                            showLabel("x Incompatible Pin Type", ImColor(45, 32, 32, 180));
                            ed::RejectNewItem(ImColor(255, 128, 128), 1.0f);
                        }
                        else
                        {
                            showLabel("+ Create Link", ImColor(32, 45, 32, 180));
                            if (ed::AcceptNewItem(ImColor(128, 255, 128), 4.0f))
                            {
                                // Flow 类型输出引脚只允许一对一连接
                                if (startPin->Type == PinType::Flow)
                                {
                                    auto sid = startPinId;
                                    for (auto it = m_Links.begin(); it != m_Links.end();)
                                    {
                                        if (it->StartPinID == sid)
                                            it = m_Links.erase(it);
                                        else
                                            ++it;
                                    }
                                }

                                m_Links.emplace_back(Link(GetNextId(), startPinId, endPinId));
                                m_Links.back().Color = GetIconColor(startPin->Type);
                                m_IsDirty = true;
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
        }
        else
            ImGui::Text("Unknown node: %p", contextNodeId.AsPointer());
        ImGui::Separator();
        if (ImGui::MenuItem("Delete"))
            ed::DeleteNode(contextNodeId);
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

                        m_Links.emplace_back(Link(GetNextId(), startPin->ID, endPin->ID));
                        m_Links.back().Color = GetIconColor(startPin->Type);

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

    // 触发执行后的 Flow 动画
    if (!m_FlowLinks.empty())
    {
        for (auto& linkId : m_FlowLinks)
            ed::Flow(linkId);
        m_FlowLinks.clear();
    }

    ed::End();

    auto editorMin = ImGui::GetItemRectMin();
    auto editorMax = ImGui::GetItemRectMax();

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
}

// ============================================================================
// 可浮动节点列表窗口
// ============================================================================

void BlueprintEditor::ShowNodeListWindow(bool* p_open)
{
    ImGui::SetNextWindowSize(ImVec2(350, 500), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowPos(ImVec2(20, 50), ImGuiCond_FirstUseEver);

    if (!ImGui::Begin("Node List", p_open))
    {
        ImGui::End();
        return;
    }

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

    ImGui::End();
}

// ============================================================================
// 可浮动执行输出窗口
// ============================================================================

void BlueprintEditor::ShowExecutionWindow(bool* p_open)
{
    ImGui::SetNextWindowSize(ImVec2(400, 300), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowPos(ImVec2(20, 560), ImGuiCond_FirstUseEver);

    if (!ImGui::Begin("Execution Output", p_open))
    {
        ImGui::End();
        return;
    }

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

    ImGui::End();
}
