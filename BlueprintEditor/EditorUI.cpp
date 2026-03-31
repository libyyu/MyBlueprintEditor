// EditorUI.cpp -- 蓝图编辑器 UI 渲染
#include "BlueprintEditor.h"
#include "ThemeManager.h"
#include "FileDialogs.h"
#include <filesystem>

namespace fs = std::filesystem;

// ============================================================================
// 引脚图标颜色
// ============================================================================

ImColor BlueprintEditor::GetIconColor(PinType type)
{
    switch (type)
    {
        default:
        case PinType::Flow:     return ImColor(240, 240, 240);
        case PinType::Bool:     return ImColor(200,  50,  50);
        case PinType::Int:      return ImColor( 55, 195, 150);
        case PinType::Float:    return ImColor(130, 210,  80);
        case PinType::String:   return ImColor(160,  60, 200);
        case PinType::Object:   return ImColor( 60, 150, 220);
        case PinType::Function: return ImColor(220,  30, 190);
        case PinType::Delegate: return ImColor(235,  55,  55);
        case PinType::Array:    return ImColor(245, 170,  30);
        case PinType::Map:      return ImColor( 80, 190, 220);
        case PinType::Any:      return ImColor(170, 170, 180);
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
    PushUndoState();   // 连线前保存快照

    ed::PinId disconnectedPinId = 0;  // 被断开的对端引脚

    // Flow 输出引脚只允许一对一连接：删除旧链接，记录被断开的对端
    if (startPin->Type == PinType::Flow)
    {
        for (auto it = ActiveDoc()->links.begin(); it != ActiveDoc()->links.end();)
        {
            // 检查此链接是否涉及当前 Flow 输出引脚（可能在 Start 或 End 端）
            if (it->StartPinID == startPinId)
            {
                disconnectedPinId = it->EndPinID;
                it = ActiveDoc()->links.erase(it);
            }
            else if (it->EndPinID == startPinId)
            {
                disconnectedPinId = it->StartPinID;
                it = ActiveDoc()->links.erase(it);
            }
            else
                ++it;
        }
    }

    // Flow 输入引脚也只允许一对一连接
    if (endPin->Type == PinType::Flow)
    {
        for (auto it = ActiveDoc()->links.begin(); it != ActiveDoc()->links.end();)
        {
            if (it->StartPinID == endPinId || it->EndPinID == endPinId)
                it = ActiveDoc()->links.erase(it);
            else
                ++it;
        }
    }

    // 创建新链接（快照已在函数入口处保存，无需重复 PushUndoState）
    ActiveDoc()->links.emplace_back(Link(GetNextId(), startPinId, endPinId));
    ActiveDoc()->links.back().Color = GetIconColor(GetLinkColor(startPin, endPin));
    ActiveDoc()->isDirty = true;

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
                    for (const auto& lnk : ActiveDoc()->links)
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
                            ActiveDoc()->links.emplace_back(Link(GetNextId(), outPin.ID, disconnectedPinId));
                            ActiveDoc()->links.back().Color = GetIconColor(PinType::Flow);
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

    auto* doc = ActiveDoc();
    if (!doc) return PinType::Any;
    doc->ensureEditorIndices();

    // 辅助：通过 PinLocation 安全解引用 Pin 类型
    auto resolvePinType = [&](uint64_t pid) -> PinType {
        auto it = doc->pinIdIndex.find(pid);
        if (it == doc->pinIdIndex.end()) return PinType::Any;
        const auto& loc = it->second;
        if (loc.nodeIdx >= doc->nodes.size()) return PinType::Any;
        const auto& node = doc->nodes[loc.nodeIdx];
        const Pin* p = loc.isOutput
            ? (loc.pinIdx < node.Outputs.size() ? &node.Outputs[loc.pinIdx] : nullptr)
            : (loc.pinIdx < node.Inputs.size()  ? &node.Inputs[loc.pinIdx]  : nullptr);
        return p ? p->Type : PinType::Any;
    };

    uint64_t pinId = reinterpret_cast<uintptr_t>(pin.ID.AsPointer());
    for (const auto& link : doc->links)
    {
        uint64_t startId = reinterpret_cast<uintptr_t>(link.StartPinID.AsPointer());
        uint64_t endId   = reinterpret_cast<uintptr_t>(link.EndPinID.AsPointer());

        if (startId == pinId)
        {
            PinType t = resolvePinType(endId);
            if (t != PinType::Any) return t;
        }
        else if (endId == pinId)
        {
            PinType t = resolvePinType(startId);
            if (t != PinType::Any) return t;
        }
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
        case PinType::Map:      iconType = IconType::Grid;   break;
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

    // ── 主题切换 ────────────────────────────────────────────────────────
    ImGui::SeparatorText(ICON_FA_PALETTE " Theme");
    {
        auto& tm = ThemeManager::Get();
        const auto& themes = tm.GetThemeNames();
        const std::string& cur = tm.GetCurrentTheme();
        ImGui::BeginHorizontal("##ThemeRow", ImVec2(paneWidth, 0));
        ImGui::Text("Theme:");
        ImGui::Spring(0, 8.0f);
        ImGui::SetNextItemWidth(120.0f);
        if (ImGui::BeginCombo("##ThemeCombo", cur.c_str()))
        {
            for (const auto& name : themes)
            {
                bool selected = (name == cur);
                if (ImGui::Selectable(name.c_str(), selected))
                    tm.Apply(name);
                if (selected)
                    ImGui::SetItemDefaultFocus();
            }
            ImGui::EndCombo();
        }
        ImGui::Spring();
        if (ImGui::Button(ICON_FA_FLOPPY_DISK " Save"))
            tm.SaveToFile("data/theme.json");
        ImGui::EndHorizontal();
    }
    ImGui::Spacing();
    ImGui::SeparatorText(ICON_FA_SLIDERS " Node Editor Style");

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
// ShowLeftPane — 已废弃，由 DrawNodeListPanel() 替代
// 保留空实现以防外部还有调用点（编译期可发现后删除声明）
// ============================================================================

void BlueprintEditor::ShowLeftPane(float /*paneWidth*/)
{
    // intentionally empty – replaced by DrawNodeListPanel()
}

// ============================================================================
// 主渲染帧
// ============================================================================

void BlueprintEditor::OnFrame(float deltaTime)
{
    // 使当前活跃文档的编辑器侧索引标记为 dirty（懒重建：首次查找时自动重建）
    // 目前所有 mutation 发生在同一帧的 ed::Begin/End 区间内，此处每帧标记 dirty 保证安全
    // TODO: 后续可在每个 mutation 点单独调用 invalidateEditorIndices() 来完全消除此处调用
    if (ActiveDoc())
        ActiveDoc()->invalidateEditorIndices();

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

    // 动态更新 OS 窗口标题（仅在 dirty 状态或文件变化时重建，避免每帧调用 OS API）
    if (ActiveDoc())
    {
        std::string baseName;
        if (ActiveDoc()->filePath.empty())
            baseName = "[New]";
        else
        {
            size_t lastSlash = ActiveDoc()->filePath.find_last_of("/\\");
            baseName = (lastSlash != std::string::npos) ? ActiveDoc()->filePath.substr(lastSlash + 1) : ActiveDoc()->filePath;
            size_t lastDot = baseName.find_last_of('.');
            if (lastDot != std::string::npos)
                baseName = baseName.substr(0, lastDot);
        }
        std::string windowTitle = "Blueprint Editor - " + baseName;
        if (ActiveDoc()->isDirty)
            windowTitle += " *";

        // 只在标题变化时调用 SetTitle（避免每帧 OS 调用）
        static std::string s_lastWindowTitle;
        if (windowTitle != s_lastWindowTitle)
        {
            SetTitle(windowTitle.c_str());
            s_lastWindowTitle = windowTitle;
        }
    }

    auto& io = ImGui::GetIO();

    // ================================================================
    // 菜单栏
    // ================================================================
    if (ImGui::BeginMenuBar())
    {
        bool hasProj = m_Project.IsOpen();
        bool hasDoc  = (ActiveDoc() != nullptr);

        if (ImGui::BeginMenu(ICON_FA_FILE " File"))
        {
            // ── 工程 ──────────────────────────────────────────────────────
            if (ImGui::MenuItem(ICON_FA_DIAGRAM_PROJECT " New Project"))
                NewProject();
            if (ImGui::MenuItem(ICON_FA_FOLDER_OPEN " Open Project..."))
                OpenProject();
            // 最近工程（始终显示，排除当前已打开的工程）
            if (ImGui::BeginMenu(ICON_FA_CLOCK_ROTATE_LEFT " Recent Projects"))
            {
                DrawRecentProjectsMenu();
                ImGui::EndMenu();
            }
            if (ImGui::MenuItem(ICON_FA_FLOPPY_DISK " Save Project", nullptr, false, hasProj))
                SaveProject();
            if (ImGui::MenuItem(ICON_FA_FILE_EXPORT " Save Project As...", nullptr, false, hasProj))
                SaveProjectAs();
            // ── 蓝图（仅有工程时显示）─────────────────────────────────────
            if (hasProj)
            {
                ImGui::Separator();
                if (ImGui::MenuItem(ICON_FA_FILE " New Actor Blueprint", "Ctrl+N"))
                    NewFile(RTBlueprintClass::Actor);
                if (ImGui::MenuItem(ICON_FA_CUBE " New Function Library"))
                    NewFile(RTBlueprintClass::FunctionLibrary);
                if (ImGui::MenuItem(ICON_FA_FOLDER_OPEN " Open Blueprint...", "Ctrl+O"))
                    OpenFile();
                if (ImGui::BeginMenu(ICON_FA_CLOCK_ROTATE_LEFT " Recent Files"))
                {
                    DrawRecentFilesMenu();
                    ImGui::EndMenu();
                }
                ImGui::Separator();
                if (ImGui::MenuItem(ICON_FA_FLOPPY_DISK " Save", "Ctrl+S", false, hasDoc))
                    SaveFile();
                if (ImGui::MenuItem(ICON_FA_FILE_EXPORT " Save As...", "Ctrl+Shift+S", false, hasDoc))
                    SaveFileAs();
                ImGui::Separator();
                if (ImGui::MenuItem(ICON_FA_XMARK " Close Tab", "Ctrl+W", false, hasDoc))
                {
                    if (ActiveDoc()->isDirty)
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

        // Edit 菜单：完全依赖文档，无文档时整体禁用
        if (ImGui::BeginMenu(ICON_FA_PEN " Edit", hasDoc))
        {
            if (ImGui::MenuItem(ICON_FA_ARROW_ROTATE_LEFT " Undo", "Ctrl+Z", false, CanUndo()))
                Undo();
            if (ImGui::MenuItem(ICON_FA_ARROWS_ROTATE " Redo", "Ctrl+Y", false, CanRedo()))
                Redo();
            ImGui::Separator();
            if (ImGui::MenuItem(ICON_FA_COPY " Copy", "Ctrl+C"))
                CopySelectedNodes();
            if (ImGui::MenuItem(ICON_FA_PASTE " Paste", "Ctrl+V"))
            {
                ImVec2 canvasPos = ed::ScreenToCanvas(ImGui::GetMousePos());
                PasteNodes(canvasPos);
            }
            if (ImGui::MenuItem(ICON_FA_SCISSORS " Cut", "Ctrl+X"))
                CutSelectedNodes();
            if (ImGui::MenuItem(ICON_FA_CLONE " Duplicate", "Ctrl+D"))
                DuplicateSelectedNodes();
            ImGui::Separator();
            if (ImGui::MenuItem(ICON_FA_OBJECT_GROUP " Select All", "Ctrl+A"))
            {
                for (auto& node : ActiveDoc()->nodes)
                    ed::SelectNode(node.ID, true);
            }
            ImGui::Separator();
            if (ImGui::BeginMenu(ICON_FA_ALIGN_LEFT " Align Selected"))
            {
                if (ImGui::MenuItem(ICON_FA_ALIGN_LEFT " Align Left"))    AlignSelectedNodes(AlignMode::Left);
                if (ImGui::MenuItem(ICON_FA_ALIGN_RIGHT " Align Right"))   AlignSelectedNodes(AlignMode::Right);
                if (ImGui::MenuItem(ICON_FA_ARROW_UP " Align Top"))     AlignSelectedNodes(AlignMode::Top);
                if (ImGui::MenuItem(ICON_FA_ARROW_DOWN " Align Bottom"))  AlignSelectedNodes(AlignMode::Bottom);
                ImGui::Separator();
                if (ImGui::MenuItem(ICON_FA_ALIGN_CENTER " Center Horizontally"))  AlignSelectedNodes(AlignMode::CenterH);
                if (ImGui::MenuItem(ICON_FA_ALIGN_CENTER " Center Vertically"))    AlignSelectedNodes(AlignMode::CenterV);
                ImGui::EndMenu();
            }
            ImGui::Separator();
            if (ImGui::MenuItem(ICON_FA_MAGNIFYING_GLASS " Find...", "Ctrl+F"))
                OpenSearchOverlay();
            ImGui::EndMenu();
        }

        // View 菜单：部分条目依赖文档
        if (ImGui::BeginMenu(ICON_FA_EYE " View"))
        {
            if (hasDoc)
            {
                ImGui::MenuItem(ICON_FA_LIST " Node List", nullptr, &m_ShowNodeListWindow);
                ImGui::MenuItem(ICON_FA_TERMINAL " Execution Output", nullptr, &m_ShowExecutionWindow);
                ImGui::MenuItem(ICON_FA_STOPWATCH " Timer Monitor", nullptr, &m_ShowTimerWindow);
                ImGui::MenuItem(ICON_FA_MAP " Minimap", nullptr, &m_ShowMinimap);
                ImGui::MenuItem(ICON_FA_LAYER_GROUP " Node Library", nullptr, &m_ShowLibraryWindow);
                ImGui::MenuItem(ICON_FA_TABLE_CELLS " Show Ordinals", nullptr, &m_ShowOrdinals);
                ImGui::Separator();
                if (ImGui::MenuItem(ICON_FA_EXPAND " Zoom to Content"))
                    ed::NavigateToContent();
                ImGui::Separator();
            }
            if (ImGui::MenuItem(ICON_FA_PALETTE " Style Editor"))
                m_ShowStyleEditorWindow = true;
            ImGui::EndMenu();
        }

        // Run 菜单：整体依赖文档
        if (ImGui::BeginMenu(ICON_FA_BOLT " Run", hasDoc))
        {
            if (ImGui::MenuItem(ICON_FA_PLAY " Execute Blueprint", "F5"))
                ExecuteBlueprint();
            ImGui::Separator();
            ImGui::MenuItem(ICON_FA_STOPWATCH " Timer Monitor", nullptr, &m_ShowTimerWindow);
            if (ImGui::MenuItem(ICON_FA_ERASER " Clear Execution Highlight"))
                ActiveDoc()->executedNodeHighlight.clear();
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu(ICON_FA_CIRCLE_QUESTION " Help"))
        {
            ImGui::TextColored(ImVec4(0.45f, 0.70f, 0.95f, 1.00f), ICON_FA_KEYBOARD " Keyboard Shortcuts");
            ImGui::Separator();
            ImGui::TextDisabled("File");
            ImGui::BulletText("Ctrl+N         New File");
            ImGui::BulletText("Ctrl+O         Open File");
            ImGui::BulletText("Ctrl+S         Save");
            ImGui::BulletText("Ctrl+Shift+S   Save As");
            ImGui::BulletText("Ctrl+W         Close Tab");
            ImGui::Spacing();
            ImGui::TextDisabled("Edit");
            ImGui::BulletText("Ctrl+C         Copy");
            ImGui::BulletText("Ctrl+V         Paste");
            ImGui::BulletText("Ctrl+X         Cut");
            ImGui::BulletText("Ctrl+D         Duplicate");
            ImGui::BulletText("Ctrl+A         Select All");
            ImGui::BulletText("Ctrl+F         Find Nodes");
            ImGui::BulletText("Delete         Delete Selected");
            ImGui::Spacing();
            ImGui::TextDisabled("View");
            ImGui::BulletText("F              Zoom to Content");
            ImGui::BulletText("F5             Execute Blueprint");
            ImGui::BulletText("Right Click    Context Menu");
            ImGui::BulletText("Double Click   Open Sub-Blueprint");
            ImGui::EndMenu();
        }
        ImGui::Separator();

        // 显示当前文件名
        if (hasDoc)
        {
            if (!ActiveDoc()->filePath.empty())
            {
                std::string displayName = ActiveDoc()->filePath;
                size_t lastSlash = displayName.find_last_of("/\\");
                if (lastSlash != std::string::npos)
                    displayName = displayName.substr(lastSlash + 1);
                if (ActiveDoc()->isDirty)
                    displayName += " \xe2\x80\xa2";  // bullet
                ImGui::TextColored(ImVec4(0.55f, 0.75f, 1.0f, 0.90f), "%s", displayName.c_str());
                ImGui::Separator();
            }
            else
            {
                ImGui::TextColored(ImVec4(0.55f, 0.55f, 0.60f, 0.90f), "[New]");
                ImGui::Separator();
            }
        }

        // FPS（低调灰色）
        ImGui::TextColored(ImVec4(0.50f, 0.52f, 0.58f, 0.90f),
            "%.0f fps", io.Framerate);

        // 节点/链接统计
        if (ActiveDoc())
        {
            ImGui::Separator();
            ImGui::TextColored(ImVec4(0.42f, 0.60f, 0.42f, 0.85f), "N:%d  L:%d",
                               static_cast<int>(ActiveDoc()->nodes.size()), static_cast<int>(ActiveDoc()->links.size()));
        }

        // 菜单栏最右侧：侧边栏切换按钮（点击显示/隐藏左侧面板）
        {
            float btnW = ImGui::CalcTextSize(ICON_FA_LIST).x + ImGui::GetStyle().FramePadding.x * 2.0f + 4.0f;
            float avail = ImGui::GetContentRegionAvail().x;
            if (avail > btnW + 4.0f)
                ImGui::SetCursorPosX(ImGui::GetCursorPosX() + avail - btnW - 4.0f);
            ImGui::PushStyleColor(ImGuiCol_Button,        m_ShowNodeListWindow ? IM_COL32(0, 122, 204, 80) : IM_COL32(0,0,0,0));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, IM_COL32(0, 122, 204, 50));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive,  IM_COL32(0, 122, 204, 120));
            ImGui::PushStyleColor(ImGuiCol_Text,          m_ShowNodeListWindow ? IM_COL32(100, 190, 255, 255) : IM_COL32(160, 165, 175, 220));
            if (ImGui::Button(ICON_FA_LIST "##toggleSidebar"))
                m_ShowNodeListWindow = !m_ShowNodeListWindow;
            ImGui::PopStyleColor(4);
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip(m_ShowNodeListWindow ? "Hide Side Panel" : "Show Side Panel");
        }

        ImGui::EndMenuBar();
    }

    // 键盘快捷键 - 文件操作
    // 当用户正在输入文本（搜索框/重命名框等）时，不触发快捷键
    const bool canDoShortcut = !io.WantTextInput;

    if (canDoShortcut && io.KeyCtrl && !io.KeyShift && ImGui::IsKeyPressed(ImGuiKey_N))
        NewFile(RTBlueprintClass::Actor); // Ctrl+N 默认新建 Actor 蓝图
    if (canDoShortcut && io.KeyCtrl && !io.KeyShift && ImGui::IsKeyPressed(ImGuiKey_O))
        OpenFile();
    if (canDoShortcut && io.KeyCtrl && !io.KeyShift && ImGui::IsKeyPressed(ImGuiKey_S))
        SaveFile();
    if (canDoShortcut && io.KeyCtrl && io.KeyShift && ImGui::IsKeyPressed(ImGuiKey_S))
        SaveFileAs();
    if (canDoShortcut && io.KeyCtrl && !io.KeyShift && ImGui::IsKeyPressed(ImGuiKey_W))
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
    if (canDoShortcut && io.KeyCtrl && !io.KeyShift && ImGui::IsKeyPressed(ImGuiKey_C))
        CopySelectedNodes();
    if (canDoShortcut && io.KeyCtrl && !io.KeyShift && ImGui::IsKeyPressed(ImGuiKey_V))
    {
        ImVec2 canvasPos = ed::ScreenToCanvas(ImGui::GetMousePos());
        PasteNodes(canvasPos);
    }
    if (canDoShortcut && io.KeyCtrl && !io.KeyShift && ImGui::IsKeyPressed(ImGuiKey_X))
        CutSelectedNodes();
    if (canDoShortcut && io.KeyCtrl && !io.KeyShift && ImGui::IsKeyPressed(ImGuiKey_D))
        DuplicateSelectedNodes();
    if (canDoShortcut && io.KeyCtrl && !io.KeyShift && ImGui::IsKeyPressed(ImGuiKey_Z))
        Undo();
    if (canDoShortcut && io.KeyCtrl && !io.KeyShift && ImGui::IsKeyPressed(ImGuiKey_Y))
        Redo();
    if (canDoShortcut && io.KeyCtrl && !io.KeyShift && ImGui::IsKeyPressed(ImGuiKey_F))
        OpenSearchOverlay();
    if (canDoShortcut && io.KeyCtrl && !io.KeyShift && ImGui::IsKeyPressed(ImGuiKey_A))
    {
        if (ActiveDoc())
        {
            for (auto& node : ActiveDoc()->nodes)
                ed::SelectNode(node.ID, true);
        }
    }

    // F5: 执行蓝图
    if (canDoShortcut && ImGui::IsKeyPressed(ImGuiKey_F5))
        ExecuteBlueprint();

    // 确保有活跃文档
    if (!m_Documents.empty() && !ActiveDoc())
        return;

    // 提前设置编辑器上下文（左侧面板 DrawNodeListPanel 需要 ed:: 函数）
    if (ActiveDoc())
    {
        ed::SetCurrentEditor(ActiveDoc()->editorContext);
        // 延迟应用 NodeEditor style（OnStart 时 ed context 尚未激活）
        ThemeManager::Get().ApplyPendingNodeEditorStyle();
    }

    // ================================================================
    // VSCode 风格固定面板布局
    // ================================================================
    //
    //  ┌──────────┬──────────────────────────┐
    //  │          │  [Tab1] [Tab2] [+]        │
    //  │ 左侧面板  ├──────────────────────────┤
    //  │(NodeList)│    中间 Node Editor       │
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

        // 绘制左侧面板（VS-style 垂直侧边栏按钮 + 内容区域）
        static int leftTabIndex = 0;  // 0=Project, 1=Nodes, 2=Vars, 3=Funcs, 4=Events, 5=Details
        // DPI 自适应：侧边栏宽度 = 图标宽度 + padding，避免高 DPI 下图标被截断
        const float sidebarBtnW = ImGui::GetTextLineHeight() + ImGui::GetStyle().FramePadding.x * 2.0f + 8.0f;
        float contentW = m_LeftPanelWidth - sidebarBtnW - 2.0f;
        if (contentW < 80.0f) contentW = 80.0f;

        ImGui::BeginChild("##LeftPanel", ImVec2(m_LeftPanelWidth, totalHeight), true, ImGuiWindowFlags_NoScrollbar);

        // 左侧窄按钮列
        ImGui::BeginChild("##Sidebar", ImVec2(sidebarBtnW, totalHeight - 4.0f), false, ImGuiWindowFlags_NoScrollbar);
        {
            auto drawSidebarBtn = [&](int idx, const char* icon, const char* tooltip) {
                ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4.0f);
                bool active = (leftTabIndex == idx);
                if (active)
                {
                    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.20f, 0.40f, 0.68f, 1.0f));
                    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.25f, 0.48f, 0.78f, 1.0f));
                }
                else
                {
                    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.15f, 0.15f, 0.18f, 0.0f));
                    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.25f, 0.25f, 0.30f, 1.0f));
                }
                if (ImGui::Button(icon, ImVec2(sidebarBtnW - 2.0f, sidebarBtnW - 2.0f)))
                    leftTabIndex = idx;
                ImGui::PopStyleColor(2);
                ImGui::PopStyleVar();
                if (ImGui::IsItemHovered())
                    ImGui::SetTooltip("%s", tooltip);
                // VS-style active indicator bar
                if (active)
                {
                    auto* dl = ImGui::GetWindowDrawList();
                    ImVec2 rmin = ImGui::GetItemRectMin();
                    ImVec2 rmax = ImGui::GetItemRectMax();
                    dl->AddRectFilled(ImVec2(rmin.x, rmin.y), ImVec2(rmin.x + 3.0f, rmax.y),
                                      IM_COL32(0, 122, 204, 255), 2.0f);
                }
            };

            // ── 组一：Project ──────────────────────────────────────────────
            drawSidebarBtn(0, ICON_FA_DIAGRAM_PROJECT, "Project");

            // ── 分隔线 ─────────────────────────────────────────────────────
            if (ActiveDoc())
            {
                ImGui::Spacing();
                {
                    auto* dl = ImGui::GetWindowDrawList();
                    ImVec2 p  = ImGui::GetCursorScreenPos();
                    float cx  = p.x + sidebarBtnW * 0.5f - 1.0f;
                    dl->AddLine(ImVec2(cx - sidebarBtnW * 0.3f, p.y),
                                ImVec2(cx + sidebarBtnW * 0.3f, p.y),
                                IM_COL32(80, 80, 90, 160), 1.0f);
                }
                ImGui::Dummy(ImVec2(sidebarBtnW - 2.0f, 3.0f));
                ImGui::Spacing();

                // ── 组二：Blueprint 内容面板 ──────────────────────────────
                drawSidebarBtn(1, ICON_FA_CUBES,        "Nodes");
                drawSidebarBtn(2, ICON_FA_LAYER_GROUP,  "Variables");
                drawSidebarBtn(3, ICON_FA_CODE_BRANCH,  "Functions");
                drawSidebarBtn(4, ICON_FA_BOLT,         "Events");
                drawSidebarBtn(5, ICON_FA_CIRCLE_INFO,  "Details");
            }
        }
        ImGui::EndChild();

        ImGui::SameLine(0, 2.0f);

        // 右侧内容区域
        ImGui::BeginChild("##LeftContent", ImVec2(contentW, totalHeight - 4.0f), false);
        {
            if (leftTabIndex == 0)
            {
                DrawProjectPanel();
            }
            else if (ActiveDoc())
            {
                // panelIdx: leftTabIndex 1→0(Nodes), 2→1(Vars), 3→2(Funcs), 4→3(Events), 5→4(Details)
                DrawNodeListPanel(leftTabIndex - 1);
            }
        }
        ImGui::EndChild();

        ImGui::EndChild();

        ImGui::SameLine();
    }

    // --- 右侧区域：标签栏 + 编辑器 + 执行输出 垂直布局 ---
    ImVec2 editorMin(0, 0), editorMax(0, 0);
    ImGui::BeginGroup();
    {
        // ── 无文档时：优雅的欢迎提示页 ──────────────────────────────
        if (m_Documents.empty())
        {
            ImVec2 avail = ImGui::GetContentRegionAvail();
            ImVec2 regionStart = ImGui::GetCursorScreenPos();
            float baseX = ImGui::GetCursorPosX();

            // 居中辅助
            auto centerTextScaled = [&](const char* text, float scale = 1.0f) {
                float oldScale = ImGui::GetFont()->Scale;
                ImGui::GetFont()->Scale = scale;
                ImGui::PushFont(ImGui::GetFont());
                float w = ImGui::CalcTextSize(text).x;
                ImGui::SetCursorPosX(baseX + (avail.x - w) * 0.5f);
                ImGui::Text("%s", text);
                ImGui::GetFont()->Scale = oldScale;
                ImGui::PopFont();
            };

            auto centerStringScaled = [&](const std::string& text, float scale = 1.0f) {
                float oldScale = ImGui::GetFont()->Scale;
                ImGui::GetFont()->Scale = scale;
                ImGui::PushFont(ImGui::GetFont());
                float w = ImGui::CalcTextSize(text.c_str()).x;
                ImGui::SetCursorPosX(baseX + (avail.x - w) * 0.5f);
                ImGui::Text("%s", text.c_str());
                ImGui::GetFont()->Scale = oldScale;
                ImGui::PopFont();
            };

            // 绘制快捷键行（左：快捷键，右：描述），整体居中
            auto drawShortcutTable = [&](const std::vector<std::pair<std::string, std::string>>& items)
            {
                // 自动计算列宽
                float keyColW  = 0.0f;
                float descColW = 0.0f;
                float gap      = 24.0f;
                for (const auto& [key, desc] : items)
                {
                    float kw = ImGui::CalcTextSize(key.c_str()).x;
                    float dw = ImGui::CalcTextSize(desc.c_str()).x;
                    if (kw > keyColW)  keyColW  = kw;
                    if (dw > descColW) descColW = dw;
                }
                float tableW = keyColW + gap + descColW;
                float tableStartX = baseX + (avail.x - tableW) * 0.5f;
                float descStartX  = tableStartX + keyColW + gap;

                for (auto& [key, desc] : items)
                {
                    float curY = ImGui::GetCursorPosY();

                    ImGui::SetCursorPosX(tableStartX);
                    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.50f, 0.55f, 0.70f, 0.9f));
                    ImGui::Text("%s", key.c_str());
                    ImGui::PopStyleColor();

                    ImGui::SetCursorPosY(curY);
                    ImGui::SetCursorPosX(descStartX);
                    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.40f, 0.44f, 0.55f, 0.7f));
                    ImGui::Text("%s", desc.c_str());
                    ImGui::PopStyleColor();

                    ImGui::Spacing();
                }
            };

            if (m_Project.IsOpen())
            {
                // ── 工程已打开，但尚无蓝图文档 ──
                float blockH = 200.0f;
                float offsetY = (avail.y - blockH) * 0.40f;
                if (offsetY > 0) ImGui::SetCursorPosY(ImGui::GetCursorPosY() + offsetY);

                // 工程图标（大）
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.30f, 0.60f, 0.85f, 0.4f));
                centerTextScaled(ICON_FA_DIAGRAM_PROJECT, 2.5f);
                ImGui::PopStyleColor();

                ImGui::Spacing();

                // 工程名称
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.40f, 0.78f, 0.95f, 1.0f));
                centerStringScaled(m_Project.name, 1.6f);
                ImGui::PopStyleColor();

                ImGui::Spacing();

                // 副标题
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.45f, 0.50f, 0.60f, 0.8f));
                centerTextScaled("No blueprints open yet", 1.1f);
                ImGui::PopStyleColor();

                ImGui::Spacing(); ImGui::Spacing(); ImGui::Spacing();
                ImGui::Spacing(); ImGui::Spacing();

                // 分隔线（短居中线段）
                {
                    float lineW = 160.0f;
                    float lineX = regionStart.x + (avail.x - lineW) * 0.5f;
                    float lineY = ImGui::GetCursorScreenPos().y;
                    ImGui::GetWindowDrawList()->AddLine(
                        ImVec2(lineX, lineY), ImVec2(lineX + lineW, lineY),
                        IM_COL32(80, 100, 140, 60), 1.0f);
                    ImGui::Spacing(); ImGui::Spacing();
                }

                // 快捷操作提示
                drawShortcutTable({
                    { ICON_FA_KEYBOARD "  Ctrl+N",          "New Blueprint" },
                    { ICON_FA_KEYBOARD "  Ctrl+O",          "Open Blueprint" },
                    { ICON_FA_CIRCLE_INFO "  Project Panel",  "Manage files on the left" },
                });
            }
            else
            {
                // ── 无工程打开 ──
                float blockH = 240.0f;
                float offsetY = (avail.y - blockH) * 0.38f;
                if (offsetY > 0) ImGui::SetCursorPosY(ImGui::GetCursorPosY() + offsetY);

                // 大图标
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.30f, 0.40f, 0.55f, 0.35f));
                centerTextScaled(ICON_FA_DIAGRAM_PROJECT, 3.0f);
                ImGui::PopStyleColor();

                ImGui::Spacing(); ImGui::Spacing();

                // 标题（大号）
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.55f, 0.60f, 0.72f, 1.0f));
                centerTextScaled("Blueprint Editor", 1.8f);
                ImGui::PopStyleColor();

                ImGui::Spacing();

                // 副标题
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.42f, 0.46f, 0.56f, 0.75f));
                centerTextScaled("Create or open a project to get started", 1.1f);
                ImGui::PopStyleColor();

                ImGui::Spacing(); ImGui::Spacing(); ImGui::Spacing();
                ImGui::Spacing(); ImGui::Spacing();

                // 分隔线
                {
                    float lineW = 200.0f;
                    float lineX = regionStart.x + (avail.x - lineW) * 0.5f;
                    float lineY = ImGui::GetCursorScreenPos().y;
                    ImGui::GetWindowDrawList()->AddLine(
                        ImVec2(lineX, lineY), ImVec2(lineX + lineW, lineY),
                        IM_COL32(80, 100, 140, 50), 1.0f);
                    ImGui::Spacing(); ImGui::Spacing();
                }

                // 快捷键表格
                drawShortcutTable({
                    { ICON_FA_FILE "  File > New Project",   "Create a new project" },
                    { ICON_FA_FOLDER_OPEN "  File > Open Project",  "Open an existing project" },
                    { ICON_FA_KEYBOARD "  Ctrl+N",            "Quick new blueprint" },
                });
            }

            ImGui::EndGroup();
            // 弹框仍需每帧处理
            ShowUnsavedChangesDialog();
            return;
        }

        // ================================================================
        // 标签栏（Tab Bar）—— 与中间编辑器视图对齐
        // ================================================================
        int tabToClose = -1;
        float tabBarHeight = 0.0f;
        {
            ImVec2 cursorBefore = ImGui::GetCursorPos();
            ImGui::PushItemWidth(rightWidth);

            // 标签栏 VS 2022 风格颜色（非活跃略暗，活跃与编辑器背景融合）
            ImGui::PushStyleColor(ImGuiCol_Tab,        ImVec4(0.120f, 0.120f, 0.148f, 1.00f));
            ImGui::PushStyleColor(ImGuiCol_TabSelected, ImVec4(0.150f, 0.155f, 0.195f, 1.00f));
            ImGui::PushStyleColor(ImGuiCol_TabHovered,  ImVec4(0.180f, 0.200f, 0.280f, 0.85f));

            if (ImGui::BeginTabBar("##BlueprintTabs", ImGuiTabBarFlags_Reorderable | ImGuiTabBarFlags_AutoSelectNewTabs | ImGuiTabBarFlags_FittingPolicyScroll))
            {
                for (int i = 0; i < (int)m_Documents.size(); ++i)
                {
                    auto& doc = m_Documents[i];
                    std::string tabTitle = doc->GetTabTitle();

                    bool isOpen = true;
                    ImGuiTabItemFlags flags = 0;
                    if (m_PendingSwitchTabIndex == i)
                        flags |= ImGuiTabItemFlags_SetSelected;

                    if (ImGui::BeginTabItem((tabTitle + "###tab" + std::to_string(i)).c_str(), &isOpen, flags))
                    {
                        m_ActiveDocIndex = i;

                        // VS 2022 风格：活跃标签顶部蓝色指示线
                        {
                            ImVec2 tabMin = ImGui::GetItemRectMin();
                            ImVec2 tabMax = ImGui::GetItemRectMax();
                            auto* dl = ImGui::GetWindowDrawList();
                            dl->AddRectFilled(
                                ImVec2(tabMin.x + 1.0f, tabMin.y),
                                ImVec2(tabMax.x - 1.0f, tabMin.y + 2.0f),
                                IM_COL32(0, 122, 204, 255), 0.0f);  // VS 蓝 #007ACC
                        }

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
            ImGui::PopStyleColor(3);
            ImGui::PopItemWidth();
            tabBarHeight = ImGui::GetCursorPos().y - cursorBefore.y;

            // Tab Bar 底部分隔线（VS 风格：细而低调）
            {
                ImVec2 lineStart = ImGui::GetCursorScreenPos();
                lineStart.y -= 1.0f;
                auto* dl = ImGui::GetWindowDrawList();
                dl->AddLine(lineStart, ImVec2(lineStart.x + rightWidth, lineStart.y),
                            IM_COL32(48, 50, 62, 200), 1.0f);
            }
        }

        // 延迟关闭标签
        if (tabToClose >= 0)
            CloseDocument(tabToClose);

        // 确保有活跃文档
        if (!ActiveDoc())
        {
            ImGui::EndGroup();
            return;
        }

        // 切换到当前活跃文档的编辑器上下文（标签切换后需要更新）
        ed::SetCurrentEditor(ActiveDoc()->editorContext);

        // 剩余高度（减去 Tab Bar 占用）
        float remainingHeight = totalHeight - tabBarHeight;
        float editorHeight = remainingHeight;
        float bottomHeight = 0.0f;

        if (m_ShowExecutionWindow)
        {
            // 约束底部面板高度（DPI 自适应：最小高度至少能容纳 TabBar + Filter + 2行日志）
            float minPanelH = ImGui::GetTextLineHeightWithSpacing() * 5.0f + ImGui::GetStyle().FramePadding.y * 4.0f + 8.0f;
            if (minPanelH < 100.0f) minPanelH = 100.0f;
            if (m_BottomPanelHeight < minPanelH) m_BottomPanelHeight = minPanelH;
            if (m_BottomPanelHeight > remainingHeight * 0.6f) m_BottomPanelHeight = remainingHeight * 0.6f;

            editorHeight = remainingHeight - m_BottomPanelHeight - splitterThickness;
            if (editorHeight < 200.0f) editorHeight = 200.0f;
            bottomHeight = remainingHeight - editorHeight - splitterThickness;

            Splitter("##VerticalSplitter", false, splitterThickness, &editorHeight, &bottomHeight, 200.0f, minPanelH, rightWidth);

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
        if (ActiveDoc()->needSetNodePositions)
        {
            ActiveDoc()->needSetNodePositions = false;

            // 从加载数据计算所有节点的包围盒
            ImRect contentBounds(FLT_MAX, FLT_MAX, -FLT_MAX, -FLT_MAX);

            for (const auto& pendNode : ActiveDoc()->pendingLoadData.nodes)
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

            ActiveDoc()->pendingLoadData.clear();
            ActiveDoc()->pendingContentBounds = contentBounds;
            ActiveDoc()->needNavigateToContent = 1;
        }

        // Undo/Redo 恢复节点位置
        if (ActiveDoc()->pendingRestorePositions)
        {
            ActiveDoc()->pendingRestorePositions = false;
            for (const auto& kv : ActiveDoc()->pendingRestoreNodePos)
                ed::SetNodePosition(kv.first, kv.second);
            ActiveDoc()->pendingRestoreNodePos.clear();
        }

        // 延迟居中显示（倒计帧数，到 0 时触发）
        if (ActiveDoc()->needNavigateToContent > 0)
        {
            ActiveDoc()->needNavigateToContent--;
            if (ActiveDoc()->needNavigateToContent == 0)
            {
                // 如果有预计算的 bounds（加载文件时），直接用它导航
                if (ActiveDoc()->pendingContentBounds.Min.x < ActiveDoc()->pendingContentBounds.Max.x)
                {
                    ed::NavigateToRect(ActiveDoc()->pendingContentBounds.Min, ActiveDoc()->pendingContentBounds.Max, true, 0);
                    ActiveDoc()->pendingContentBounds = ImRect();  // 清除
                }
                else
                    ed::NavigateToContent();
            }
        }

        auto cursorTopLeft = ImGui::GetCursorScreenPos();

        // 读取双击节点事件（一次性消费 API），保存到文档供后续使用
        // 必须在 DrawNodes 之前读取，否则会被 Comment 编辑逻辑消费
        ActiveDoc()->lastDoubleClickedNode = ed::GetDoubleClickedNode();

        util::BlueprintNodeBuilder builder(m_HeaderBackground,
            GetTextureWidth(m_HeaderBackground), GetTextureHeight(m_HeaderBackground));
        DrawNodes(builder);  // → NodeRenderer.cpp

        DrawLinks();  // → LinkRenderer.cpp

        ImGui::SetCursorScreenPos(cursorTopLeft);
    }

    // 上下文菜单（→ ContextMenus.cpp）
    auto openPopupPosition = ImGui::GetMousePos();
    DrawContextMenus(openPopupPosition, contextNodeId, contextPinId,
                     contextLinkId, createNewNode, newNodeLinkPin);

    // ================================================================
    // 双击节点处理：Execute Blueprint → 延迟打开对应蓝图文件
    // 注意：不能在 ed::Begin/End 块内部调用 DoOpenFile 或切换 EditorContext，
    //       否则会导致 imgui-node-editor 内部状态混乱而崩溃。
    //       这里只记录要打开的路径，在 ed::End() 之后再执行。
    // ================================================================
    {
        auto doubleClickedNodeId = ActiveDoc()->lastDoubleClickedNode;
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

                // 补全扩展名：没有 .json 后缀时自动补 .json
                if (!filePath.empty())
                {
                    // 去掉末尾空格
                    while (!filePath.empty() && filePath.back() == ' ') filePath.pop_back();

                    // 不区分大小写检查是否已有 .json 后缀
                    auto hasSuffix = [](const std::string& s, const std::string& suf) {
                        if (s.size() < suf.size()) return false;
                        std::string tail = s.substr(s.size() - suf.size());
                        for (auto& c : tail) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
                        return tail == suf;
                    };
                    if (!hasSuffix(filePath, ".json"))
                        filePath += ".json";
                }

                if (!filePath.empty())
                {
                    // 优先通过工程路径解析
                    std::string resolvedPath;
                    if (m_Project.IsOpen())
                    {
                        // 在工程 blueprints 和 libraries 里按文件名匹配
                        auto stem = fs::path(filePath).stem().string();
                        // stem 可能是 "Foo.bp"，再去一层
                        if (stem.size() > 3 && stem.substr(stem.size()-3) == ".bp")
                            stem = stem.substr(0, stem.size()-3);

                        for (const auto& e : m_Project.blueprints)
                        {
                            std::string abs = m_Project.AbsPath(e.relativePath);
                            std::string es  = fs::path(e.relativePath).stem().string();
                            if (es.size() > 3 && es.substr(es.size()-3) == ".bp")
                                es = es.substr(0, es.size()-3);
                            if (es == stem && fs::exists(abs))
                            { resolvedPath = abs; break; }
                        }
                        if (resolvedPath.empty())
                        {
                            for (const auto& e : m_Project.libraries)
                            {
                                std::string abs = m_Project.AbsPath(e.relativePath);
                                std::string es  = fs::path(e.relativePath).stem().string();
                                if (es.size() > 3 && es.substr(es.size()-3) == ".bp")
                                    es = es.substr(0, es.size()-3);
                                if (es == stem && fs::exists(abs))
                                { resolvedPath = abs; break; }
                            }
                        }
                    }

                    // 回退：基于当前文档目录的相对路径解析
                    if (resolvedPath.empty())
                    {
                        bool isAbsolute = false;
#ifdef _WIN32
                        isAbsolute = (filePath.size() >= 2 && filePath[1] == ':') ||
                                     (filePath.size() >= 2 && filePath[0] == '\\' && filePath[1] == '\\');
#else
                        isAbsolute = (!filePath.empty() && filePath[0] == '/');
#endif
                        if (!isAbsolute && !ActiveDoc()->filePath.empty())
                        {
                            std::string dir = ActiveDoc()->filePath;
                            size_t lastSlash = dir.find_last_of("/\\");
                            if (lastSlash != std::string::npos)
                                dir = dir.substr(0, lastSlash + 1);
                            else
                                dir.clear();
                            resolvedPath = dir + filePath;
                        }
                        else
                        {
                            resolvedPath = filePath;
                        }
                    }

                    if (!resolvedPath.empty() && fs::exists(resolvedPath))
                    {
                        std::string normResolved = fs::path(resolvedPath).lexically_normal().string();
                        bool alreadyOpen = false;
                        for (int i = 0; i < (int)m_Documents.size(); ++i)
                        {
                            if (fs::path(m_Documents[i]->filePath).lexically_normal().string() == normResolved)
                            {
                                m_PendingSwitchTabIndex = i;
                                alreadyOpen = true;
                                break;
                            }
                        }
                        if (!alreadyOpen)
                            m_PendingOpenFilePath = resolvedPath;
                    }
                    else
                    {
                        ActiveDoc()->executionLog.push_back("[INFO] ExecuteBlueprint: cannot find file: " + filePath);
                        ActiveDoc()->executionLogDirty = true;
                    }
                }
                else
                {
                    ActiveDoc()->executionLog.push_back("[INFO] Double-clicked Execute Blueprint node, but File pin is empty.");
                    ActiveDoc()->executionLogDirty = true;
                }
            }
            else if (node && node->DefinitionId.rfind("FuncLib.", 0) == 0)
            {
                // FuncLib.<libStem>.<funcId> → 在工程 libraries 里找对应的库文件并打开定位
                // 提取 libStem：FuncLib.<libStem>.<funcId>，libStem 可能含多个 '.'
                const std::string& defId = node->DefinitionId;
                // 去掉 "FuncLib." 前缀，再从右侧剥离最后一段（funcId）
                std::string withoutPrefix = defId.substr(8);  // 去掉 "FuncLib."
                size_t lastDot = withoutPrefix.rfind('.');
                std::string libStem = (lastDot != std::string::npos)
                                      ? withoutPrefix.substr(0, lastDot)
                                      : withoutPrefix;
                std::string funcId  = (lastDot != std::string::npos)
                                      ? withoutPrefix.substr(lastDot + 1)
                                      : "";

                // 在工程 libraries 中找文件名 stem 匹配的库
                std::string libAbsPath;
                if (m_Project.IsOpen())
                {
                    for (const auto& e : m_Project.libraries)
                    {
                        std::string abs = m_Project.AbsPath(e.relativePath);
                        std::string es  = fs::path(e.relativePath).stem().string();
                        // 去掉 .bp 后缀（文件名 Foo.bp.json → stem = Foo.bp → 再去 = Foo）
                        if (es.size() > 3 && es.substr(es.size()-3) == ".bp")
                            es = es.substr(0, es.size()-3);
                        if (es == libStem && fs::exists(abs))
                        { libAbsPath = abs; break; }
                    }
                }

                if (!libAbsPath.empty())
                {
                    std::string normLib = fs::path(libAbsPath).lexically_normal().string();
                    bool alreadyOpen = false;
                    for (int i = 0; i < (int)m_Documents.size(); ++i)
                    {
                        if (fs::path(m_Documents[i]->filePath).lexically_normal().string() == normLib)
                        {
                            m_PendingSwitchTabIndex = i;
                            m_PendingNavigateToFunc = funcId;
                            alreadyOpen = true;
                            break;
                        }
                    }
                    if (!alreadyOpen)
                    {
                        m_PendingOpenFilePath   = libAbsPath;
                        m_PendingNavigateToFunc = funcId;
                    }
                }
                else
                {
                    ActiveDoc()->executionLog.push_back("[INFO] FuncLib node: cannot find library for: " + defId);
                    ActiveDoc()->executionLogDirty = true;
                }
            }
        }
    }

    // 触发执行后的 Flow 动画
    if (!ActiveDoc()->flowLinks.empty())
    {
        for (auto& linkId : ActiveDoc()->flowLinks)
            ed::Flow(linkId);
        ActiveDoc()->flowLinks.clear();
    }

    // ================================================================
    // 变量拖拽放置（在 ed::End 之前，画布仍在 Begin/End 块内）
    // ================================================================
    // 仅当有 DragDrop payload 飞行时才创建 InvisibleButton 接收投放，
    // 平时不创建，以免截获节点编辑器的拖拽/连线鼠标事件
    if (ImGui::GetDragDropPayload() != nullptr)
    {
        ImVec2 editorMin = ImGui::GetItemRectMin();
        ImVec2 editorMax = ImGui::GetItemRectMax();
        ImGui::SetCursorScreenPos(editorMin);
        ImGui::InvisibleButton("##canvas_drop_target", editorMax - editorMin,
                               ImGuiButtonFlags_AllowOverlap);
        if (ImGui::BeginDragDropTarget())
        {
            if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(VAR_DRAG_DROP_TYPE))
            {
                auto* p = static_cast<const VarDragPayload*>(payload->Data);
                auto* doc = ActiveDoc();
                if (doc)
                {
                    doc->pendingVarDrop    = true;
                    doc->pendingVarPayload = *p;
                    doc->pendingVarDropPos = ImGui::GetMousePos();
                }
            }
            // 节点库拖拽：从 Library 面板拖节点定义到画布
            if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("BP_NODE_DEF"))
            {
                const char* defId = static_cast<const char*>(payload->Data);
                if (defId && defId[0] != '\0')
                {
                    PushUndoState();
                    Node* node = SpawnNodeByDef(std::string(defId));
                    if (node)
                    {
                        FixupSpecialPinTypes(node, m_NodeRegistry.getNodeDefinition(defId));
                        BuildNodes();
                        // 在鼠标释放位置生成节点（转换到画布坐标）
                        ImVec2 canvasPos = ed::ScreenToCanvas(ImGui::GetMousePos());
                        ed::SetNodePosition(node->ID, canvasPos);
                        ActiveDoc()->isDirty = true;
                    }
                }
            }
            ImGui::EndDragDropTarget();
        }
    }

    ed::End();

    // ================================================================
    // 检测节点位置变化（拖拽移动节点 → 标记 dirty + Undo 快照）
    //
    // 策略：
    //   • 检测到位置变化 + 鼠标左键按下 → 视为拖拽进行中
    //     若本次拖拽尚未 push，先 push 快照再标 dirty（只 push 一次）
    //   • 鼠标左键释放 → 重置 nodeDragUndoPushed，下次拖拽重新计
    // ================================================================
    if (ActiveDoc())
    {
        auto& lastPositions = ActiveDoc()->lastNodePositions;
        bool posChanged = false;
        for (const auto& node : ActiveDoc()->nodes)
        {
            ImVec2 curPos = ed::GetNodePosition(node.ID);
            auto it = lastPositions.find(node.ID);
            if (it != lastPositions.end() &&
                (it->second.x != curPos.x || it->second.y != curPos.y))
            {
                posChanged = true;
                break;
            }
        }

        if (posChanged)
        {
            // 鼠标按下 → 拖拽进行中，push 一次快照
            if (io.MouseDown[0] && !ActiveDoc()->nodeDragUndoPushed)
            {
                PushUndoState();
                ActiveDoc()->nodeDragUndoPushed = true;
            }
            ActiveDoc()->isDirty = true;
        }

        // 鼠标左键释放 → 本次拖拽结束，重置标志
        if (ImGui::IsMouseReleased(0))
            ActiveDoc()->nodeDragUndoPushed = false;
    }
    // 更新上一帧节点位置快照
    if (ActiveDoc())
    {
        auto& lastPositions = ActiveDoc()->lastNodePositions;
        lastPositions.clear();
        for (const auto& node : ActiveDoc()->nodes)
            lastPositions[node.ID] = ed::GetNodePosition(node.ID);
    }

    // ================================================================
    // 延迟处理变量拖拽放置（必须在 ed::End() 之后执行）
    // ================================================================
    if (ActiveDoc() && ActiveDoc()->pendingVarDrop)
    {
        ActiveDoc()->pendingVarDrop = false;

        const VarDragPayload& vp  = ActiveDoc()->pendingVarPayload;
        ImVec2                dropPos   = ActiveDoc()->pendingVarDropPos;
        bool                  shiftHeld = ImGui::GetIO().KeyShift;

        auto spawnVarNode = [&](const char* defId)
        {
            ed::SetCurrentEditor(ActiveDoc()->editorContext);
            PushUndoState();  // 变量拖拽生成节点前保存快照
            Node* node = SpawnNodeByDef(defId);
            if (node)
            {
                BuildNodes();
                ActiveDoc()->isDirty = true;
                for (auto& pin : node->Inputs)
                    if (pin.Name == "Name") { pin.StringValue = vp.varName; break; }
                ed::SetNodePosition(node->ID, ed::ScreenToCanvas(dropPos));
            }
        };

        if (shiftHeld)
        {
            // Shift+拖拽：直接生成 SetVariable
            spawnVarNode("SetVariable");
        }
        else
        {
            // 无修饰键：弹出 Get/Set 选择菜单
            ImGui::SetNextWindowPos(dropPos, ImGuiCond_Always);
            ImGui::OpenPopup("##VarDropMenu");
        }
    }

    // 变量 Get/Set 选择弹窗
    if (ActiveDoc() && ImGui::BeginPopup("##VarDropMenu"))
    {
        const VarDragPayload& vp = ActiveDoc()->pendingVarPayload;
        ImGui::TextDisabled("Variable: %s", vp.varName);
        ImGui::Separator();

        auto spawnAndClose = [&](const char* defId)
        {
            ed::SetCurrentEditor(ActiveDoc()->editorContext);
            PushUndoState();  // 变量拖拽生成节点前保存快照
            Node* node = SpawnNodeByDef(defId);
            if (node)
            {
                BuildNodes();
                ActiveDoc()->isDirty = true;
                for (auto& pin : node->Inputs)
                    if (pin.Name == "Name") { pin.StringValue = vp.varName; break; }
                ed::SetNodePosition(node->ID, ed::ScreenToCanvas(ActiveDoc()->pendingVarDropPos));
            }
            ImGui::CloseCurrentPopup();
        };

        if (ImGui::MenuItem(ICON_FA_EYE " Get Variable"))
            spawnAndClose("GetVariable");
        if (ImGui::MenuItem(ICON_FA_PEN " Set Variable"))
            spawnAndClose("SetVariable");

        ImGui::EndPopup();
    }

    // ================================================================
    // 延迟处理双击打开文件（必须在 ed::End() 之后执行）
    // ================================================================
    if (m_PendingSwitchTabIndex >= 0)
    {
        m_ActiveDocIndex = m_PendingSwitchTabIndex;
        ed::SetCurrentEditor(ActiveDoc()->editorContext);
        m_PendingSwitchTabIndex = -1;
        if (!m_PendingNavigateToFunc.empty())
        {
            // 在 Functions 面板中定位并展开对应函数
            auto* doc = ActiveDoc();
            for (int fi = 0; fi < (int)doc->functions.size(); ++fi)
            {
                if (doc->functions[fi].id == m_PendingNavigateToFunc)
                {
                    doc->selectedFuncIdx = fi;
                    doc->needNavigateToContent = 2;
                    break;
                }
            }
            m_PendingNavigateToFunc.clear();
        }
        else
        {
            ActiveDoc()->needNavigateToContent = 1;
        }
    }
    else if (!m_PendingOpenFilePath.empty())
    {
        std::string pathToOpen = m_PendingOpenFilePath;
        std::string funcToNav  = m_PendingNavigateToFunc;
        m_PendingOpenFilePath.clear();
        m_PendingNavigateToFunc.clear();
        DoOpenFile(pathToOpen);
        // 文件打开后定位函数
        if (!funcToNav.empty() && ActiveDoc())
        {
            auto* doc = ActiveDoc();
            for (int fi = 0; fi < (int)doc->functions.size(); ++fi)
            {
                if (doc->functions[fi].id == funcToNav)
                {
                    doc->selectedFuncIdx = fi;
                    doc->needNavigateToContent = 2;
                    break;
                }
            }
        }
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
            ShowExecutionPanel(rightWidth);
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
    // 缩放条（右下角浮动可交互 widget）
    // ================================================================
    if (ActiveDoc() && editorMax.x > editorMin.x)
        DrawZoomBar(editorMin, editorMax);


    // ================================================================
    // 调试工具条（浮动，画布顶部中央）
    // 包含完整操作：Run / Pause / Resume / Step / Stop / Copy / Clear + 状态
    // ================================================================
    if (ActiveDoc() && editorMax.x > editorMin.x)
    {
        auto& runner   = ActiveDoc()->persistentRunner;
        bool isRunning = runner.IsRunning();
        bool isPaused  = runner.IsPaused();
        bool isStopped = runner.IsStopped();
        bool isIdle    = runner.IsIdle();
        bool bpIsLibrary = ActiveDoc()->blueprintClass == RTBlueprintClass::FunctionLibrary;

        // DPI 自适应：所有尺寸基于字体行高
        float lineH = ImGui::GetTextLineHeight();
        float tbH   = lineH + ImGui::GetStyle().FramePadding.y * 2.0f + 10.0f;
        float btnH  = lineH + ImGui::GetStyle().FramePadding.y * 2.0f + 2.0f;
        // Run 按钮宽度动态计算，避免字体缩放时截断
        float btnW  = ImGui::CalcTextSize(ICON_FA_PLAY " Run").x + ImGui::GetStyle().FramePadding.x * 2.0f + 4.0f;
        float iconW = ImGui::CalcTextSize(ICON_FA_PAUSE).x + ImGui::GetStyle().FramePadding.x * 2.0f + 6.0f;
        // 工具条宽度自适应（AlwaysAutoResize），只用 totalW 估算初始水平居中位置
        float totalW = btnW + 4 + iconW + 4 + iconW + 4 + iconW + 14 + iconW + 4 + iconW + 24;
        float centerX = (editorMin.x + editorMax.x) * 0.5f;
        ImVec2 tbPos(centerX - totalW * 0.5f, editorMin.y + 6.0f);

        ImGui::SetNextWindowPos(tbPos, ImGuiCond_Always);
        // 不手动设置 Size，改用 AlwaysAutoResize，让窗口自适应内容宽高
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 8.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding,  ImVec2(8.0f, 5.0f));
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing,    ImVec2(4.0f, 4.0f));
        ImGui::PushStyleColor(ImGuiCol_WindowBg, IM_COL32(18, 20, 26, 235));
        ImGui::PushStyleColor(ImGuiCol_Border,
            isPaused  ? IM_COL32(180, 130, 20, 160) :
            isRunning ? IM_COL32(30, 160, 60, 160)  :
                        IM_COL32(0, 100, 180, 100));
        bool tbOpen = ImGui::Begin("##DebugToolbar", nullptr,
            ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoMove     | ImGuiWindowFlags_NoScrollbar |
            ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoDocking |
            ImGuiWindowFlags_AlwaysAutoResize);
        ImGui::PopStyleColor(2);
        ImGui::PopStyleVar(3);

        if (tbOpen)
        {
            // Run
            bool canRun = isIdle || isStopped;
            ImGui::PushStyleColor(ImGuiCol_Button,        IM_COL32(30, 100, 50, 255));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, IM_COL32(45, 135, 70, 255));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive,  IM_COL32(55, 160, 85, 255));
            if (!canRun || bpIsLibrary) ImGui::BeginDisabled();
            if (ImGui::Button(ICON_FA_PLAY " Run", ImVec2(btnW, btnH)))
            {
                if (isStopped) runner.ResetState();
                ExecuteBlueprint();
            }
            if (!canRun || bpIsLibrary) ImGui::EndDisabled();
            if (bpIsLibrary && ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
                ImGui::SetTooltip("FunctionLibrary cannot execute directly.");
            ImGui::PopStyleColor(3);

            ImGui::SameLine(0, 4);

            // Pause / Resume
            if (!isRunning && !isPaused) ImGui::BeginDisabled();
            if (isPaused)
            {
                ImGui::PushStyleColor(ImGuiCol_Button,        IM_COL32(30, 100, 50, 255));
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, IM_COL32(45, 135, 70, 255));
                if (ImGui::Button(ICON_FA_PLAY "##resume", ImVec2(iconW, btnH)))
                    runner.Resume();
                if (ImGui::IsItemHovered()) ImGui::SetTooltip("Resume");
                ImGui::PopStyleColor(2);
            }
            else
            {
                if (ImGui::Button(ICON_FA_PAUSE "##pause", ImVec2(iconW, btnH)))
                    runner.Pause();
                if (ImGui::IsItemHovered()) ImGui::SetTooltip("Pause");
            }
            if (!isRunning && !isPaused) ImGui::EndDisabled();

            ImGui::SameLine(0, 4);

            // Step
            if (!isPaused) ImGui::BeginDisabled();
            if (ImGui::Button(ICON_FA_ARROW_RIGHT "##step", ImVec2(iconW, btnH)))
            {
                bool hasMore = runner.StepNextNode();
                if (hasMore)
                {
                    const auto& topo = runner.GetTopoCache();
                    size_t stepIdx   = runner.GetStepTopoIndex();
                    if (stepIdx > 0 && stepIdx - 1 < topo.size())
                    {
                        uint64_t nid = static_cast<uint64_t>(topo[stepIdx - 1]);
                        ActiveDoc()->executedNodeHighlight[nid] = 3.0f;
                    }
                }
            }
            if (ImGui::IsItemHovered()) ImGui::SetTooltip("Step (next node)");
            if (!isPaused) ImGui::EndDisabled();

            ImGui::SameLine(0, 4);

            // Stop
            ImGui::PushStyleColor(ImGuiCol_Button,        IM_COL32(120, 30, 30, 255));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, IM_COL32(160, 40, 40, 255));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive,  IM_COL32(190, 50, 50, 255));
            if (isIdle || isStopped) ImGui::BeginDisabled();
            if (ImGui::Button(ICON_FA_STOP "##stop", ImVec2(iconW, btnH)))
                runner.Stop();
            if (isIdle || isStopped) ImGui::EndDisabled();
            if (ImGui::IsItemHovered()) ImGui::SetTooltip("Stop");
            ImGui::PopStyleColor(3);

            // 状态指示
            ImGui::SameLine(0, 10);
            if (isRunning)
                ImGui::TextColored(ImVec4(0.25f, 0.95f, 0.35f, 1.0f), ICON_FA_CIRCLE_PLAY);
            else if (isPaused)
                ImGui::TextColored(ImVec4(1.0f, 0.75f, 0.1f, 1.0f),   ICON_FA_PAUSE);
            else if (isStopped)
                ImGui::TextColored(ImVec4(1.0f, 0.35f, 0.30f, 1.0f),  ICON_FA_CIRCLE_STOP);

            // 用本帧实际窗口宽度修正居中位置（AlwaysAutoResize 场景下确保居中）
            float actualW = ImGui::GetWindowSize().x;
            float correctedX = centerX - actualW * 0.5f;
            // 限制不超出画布左右边界（防止 DPI 缩放下工具条被裁剪）
            float margin = 4.0f;
            if (correctedX < editorMin.x + margin)
                correctedX = editorMin.x + margin;
            if (correctedX + actualW > editorMax.x - margin)
                correctedX = editorMax.x - margin - actualW;
            ImGui::SetWindowPos(ImVec2(correctedX, editorMin.y + 6.0f));
        }
        ImGui::End();
    }

    // ================================================================
    // 画布节点搜索覆盖层（Ctrl+F）
    // ================================================================
    DrawSearchOverlay();

    // ================================================================
    // 编辑器区域底部状态栏（覆盖在编辑器之上）
    // ================================================================
    if (ActiveDoc() && editorMax.x > editorMin.x)
    {
        auto* dl = ImGui::GetWindowDrawList();
        // DPI 自适应高度：根据字体行高动态计算，避免高 DPI 下文字被截断
        float barH = ImGui::GetTextLineHeight() + 8.0f;
        ImVec2 barMin(editorMin.x, editorMax.y - barH);
        ImVec2 barMax(editorMax.x, editorMax.y);

        // VS 2022 扁平状态栏
        dl->AddRectFilled(barMin, barMax, IM_COL32(30, 30, 38, 230), 0.0f);
        // 顶部 1px 分隔线
        dl->AddLine(barMin, ImVec2(barMax.x, barMin.y), IM_COL32(0, 122, 204, 80));

        float textY = barMin.y + 4.0f;
        float x = barMin.x + 12.0f;

        // 节点数
        char buf[256];
        snprintf(buf, sizeof(buf), "Nodes: %d", static_cast<int>(ActiveDoc()->nodes.size()));
        dl->AddText(ImVec2(x, textY), IM_COL32(135, 160, 200, 210), buf);
        x += ImGui::CalcTextSize(buf).x + 8.0f;

        // 竖线分隔符
        dl->AddLine(ImVec2(x, barMin.y + 4.0f), ImVec2(x, barMax.y - 4.0f), IM_COL32(60, 70, 90, 120));
        x += 8.0f;

        // 链接数
        snprintf(buf, sizeof(buf), "Links: %d", static_cast<int>(ActiveDoc()->links.size()));
        dl->AddText(ImVec2(x, textY), IM_COL32(135, 160, 200, 210), buf);
        x += ImGui::CalcTextSize(buf).x + 8.0f;

        // 选中数
        int selCount = ed::GetSelectedObjectCount();
        if (selCount > 0)
        {
            dl->AddLine(ImVec2(x, barMin.y + 4.0f), ImVec2(x, barMax.y - 4.0f), IM_COL32(60, 70, 90, 120));
            x += 8.0f;
            snprintf(buf, sizeof(buf), "Selected: %d", selCount);
            dl->AddText(ImVec2(x, textY), IM_COL32(200, 210, 130, 230), buf);
            x += ImGui::CalcTextSize(buf).x + 8.0f;
        }

        // 缩放比例
        {
            dl->AddLine(ImVec2(x, barMin.y + 4.0f), ImVec2(x, barMax.y - 4.0f), IM_COL32(60, 70, 90, 120));
            x += 8.0f;
            float zoom = ed::GetCurrentZoom();
            snprintf(buf, sizeof(buf), ICON_FA_MAGNIFYING_GLASS " %.0f%%", zoom * 100.0f);
            ImU32 zoomCol = IM_COL32(135, 160, 200, 210);
            if (zoom < 0.3f)       zoomCol = IM_COL32(220, 140,  80, 230);  // 过小：橙色警示
            else if (zoom > 2.0f)  zoomCol = IM_COL32(140, 220, 140, 230);  // 过大：绿色
            dl->AddText(ImVec2(x, textY), zoomCol, buf);
            x += ImGui::CalcTextSize(buf).x + 8.0f;
        }

        // 右侧：上次执行状态 + 文件名
        float rightX = barMax.x - 12.0f;

        // 文件名（最右）
        if (!ActiveDoc()->filePath.empty())
        {
            size_t lastSlash = ActiveDoc()->filePath.find_last_of("/\\");
            std::string fileName = (lastSlash != std::string::npos) ? ActiveDoc()->filePath.substr(lastSlash + 1) : ActiveDoc()->filePath;
            if (ActiveDoc()->isDirty) fileName += " \xe2\x80\xa2";  // bullet
            float textW = ImGui::CalcTextSize(fileName.c_str()).x;
            rightX -= textW;
            dl->AddText(ImVec2(rightX, textY), IM_COL32(120, 145, 180, 190), fileName.c_str());
            rightX -= 16.0f;
            dl->AddLine(ImVec2(rightX, barMin.y + 4.0f), ImVec2(rightX, barMax.y - 4.0f), IM_COL32(60, 70, 90, 120));
            rightX -= 8.0f;
        }

        // 上次执行状态（文件名左侧）
        if (!ActiveDoc()->lastExecutionStatus.empty())
        {
            const auto& status = ActiveDoc()->lastExecutionStatus;
            ImU32 statusCol;
            if (status.rfind("OK", 0) == 0)
                statusCol = IM_COL32(80, 220, 100, 230);
            else if (status.rfind("Paused", 0) == 0)
                statusCol = IM_COL32(240, 180, 40, 230);
            else if (status.rfind("FAILED", 0) == 0 || status.rfind("Load Failed", 0) == 0)
                statusCol = IM_COL32(230, 80, 80, 230);
            else
                statusCol = IM_COL32(200, 190, 120, 220);
            std::string statusLabel = ICON_FA_PLAY " " + status;
            float textW = ImGui::CalcTextSize(statusLabel.c_str()).x;
            rightX -= textW;
            dl->AddText(ImVec2(rightX, textY), statusCol, statusLabel.c_str());
        }
    }

    // ================================================================
    // 未保存修改确认对话框
    // ================================================================
    ShowUnsavedChangesDialog();

    // ================================================================
    // 工程内命名对话框（有工程时替代系统 Dialog）
    // ================================================================
    DrawSaveNameDialog();

    // ================================================================
    // 样式编辑器浮动窗口
    // ================================================================
    if (m_ShowStyleEditorWindow)
        ShowStyleEditor(&m_ShowStyleEditorWindow);

    // ================================================================
    // 节点库浮动面板（右侧浮动，可通过 Inspector 工具栏 Library 按钮切换）
    // ================================================================
    if (m_ShowLibraryWindow)
    {
        ImVec2 viewport = ImGui::GetMainViewport()->Size;
        ImGui::SetNextWindowSize(ImVec2(300, viewport.y * 0.65f), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowPos(ImVec2(viewport.x - 320, 80), ImGuiCond_FirstUseEver);
        if (ImGui::Begin(ICON_FA_LAYER_GROUP " Node Library", &m_ShowLibraryWindow,
            ImGuiWindowFlags_NoSavedSettings))
        {
            DrawNodeLibraryPanel();
        }
        ImGui::End();
    }
}


// ShowUnsavedChangesDialog, DrawSaveNameDialog → 已移至 Dialogs.cpp
// DrawTimerPanel, DrawNodeLibraryPanel → 已移至 LibraryPanel.cpp
