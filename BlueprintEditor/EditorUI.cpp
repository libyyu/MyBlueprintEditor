// EditorUI.cpp -- 蓝图编辑器 UI 渲染
#include "BlueprintEditor.h"
#include "ThemeManager.h"
#include "FileDialogs.h"
#include <filesystem>

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

    // 使用哈希索引加速查找（避免线性扫描全部链接）
    auto* doc = ActiveDoc();
    if (!doc) return PinType::Any;
    doc->ensureEditorIndices();

    uint64_t pinId = reinterpret_cast<uintptr_t>(pin.ID.AsPointer());
    for (const auto& link : doc->links)
    {
        uint64_t startId = reinterpret_cast<uintptr_t>(link.StartPinID.AsPointer());
        uint64_t endId   = reinterpret_cast<uintptr_t>(link.EndPinID.AsPointer());

        if (startId == pinId)
        {
            auto it = doc->pinIdIndex.find(endId);
            if (it != doc->pinIdIndex.end() && it->second->Type != PinType::Any)
                return it->second->Type;
        }
        else if (endId == pinId)
        {
            auto it = doc->pinIdIndex.find(startId);
            if (it != doc->pinIdIndex.end() && it->second->Type != PinType::Any)
                return it->second->Type;
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
        static int leftTabIndex = 0;  // 0=Project, 1=Nodes
        const float sidebarBtnW = 32.0f;
        float contentW = m_LeftPanelWidth - sidebarBtnW - 2.0f;
        if (contentW < 80.0f) contentW = 80.0f;

        ImGui::BeginChild("##LeftPanel", ImVec2(m_LeftPanelWidth, totalHeight), true, ImGuiWindowFlags_NoScrollbar);

        // 左侧窄按钮列
        ImGui::BeginChild("##Sidebar", ImVec2(sidebarBtnW, totalHeight - 8.0f), false, ImGuiWindowFlags_NoScrollbar);
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
                if (ImGui::Button(icon, ImVec2(sidebarBtnW - 4.0f, sidebarBtnW - 4.0f)))
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

            drawSidebarBtn(0, ICON_FA_DIAGRAM_PROJECT, "Project");
            ImGui::Spacing();
            if (ActiveDoc())  // 无文档时隐藏 Nodes 按钮
                drawSidebarBtn(1, ICON_FA_CUBES, "Nodes");
        }
        ImGui::EndChild();

        ImGui::SameLine(0, 2.0f);

        // 右侧内容区域
        ImGui::BeginChild("##LeftContent", ImVec2(contentW, totalHeight - 8.0f), false);
        {
            if (leftTabIndex == 0)
            {
                DrawProjectPanel();
            }
            else if (leftTabIndex == 1 && ActiveDoc())
            {
                DrawNodeListPanel();
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
            // 约束底部面板高度
            if (m_BottomPanelHeight < 100.0f) m_BottomPanelHeight = 100.0f;
            if (m_BottomPanelHeight > remainingHeight * 0.6f) m_BottomPanelHeight = remainingHeight * 0.6f;

            editorHeight = remainingHeight - m_BottomPanelHeight - splitterThickness;
            if (editorHeight < 200.0f) editorHeight = 200.0f;
            bottomHeight = remainingHeight - editorHeight - splitterThickness;

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
                    if (!isAbsolute && !ActiveDoc()->filePath.empty())
                    {
                        std::string dir = ActiveDoc()->filePath;
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
                        ActiveDoc()->executionLog.push_back("[INFO] Double-clicked Execute Blueprint node, but File pin is empty.");
                        ActiveDoc()->executionLogDirty = true;
                    }
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
        ActiveDoc()->needNavigateToContent = 1;
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
    // 缩放条（右下角浮动可交互 widget）
    // ================================================================
    if (ActiveDoc() && editorMax.x > editorMin.x)
        DrawZoomBar(editorMin, editorMax);

    // ================================================================
    // 调试工具条（编辑器顶部中央浮动 overlay）
    // ================================================================
    if (ActiveDoc() && editorMax.x > editorMin.x)
    {
        auto& runner = ActiveDoc()->persistentRunner;
        bool isRunning = runner.IsRunning();
        bool isPaused  = runner.IsPaused();
        bool isStopped = runner.IsStopped();
        bool isIdle    = runner.IsIdle();
        bool bpIsLibrary = ActiveDoc()->blueprintClass == RTBlueprintClass::FunctionLibrary;

        float toolbarW = 320.0f;
        float toolbarH = 36.0f;
        float centerX = (editorMin.x + editorMax.x) * 0.5f;
        ImVec2 tbPos(centerX - toolbarW * 0.5f, editorMin.y + 6.0f);

        ImGui::SetNextWindowPos(tbPos, ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImVec2(toolbarW, toolbarH));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 8.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8, 4));
        ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.12f, 0.12f, 0.15f, 0.92f));
        ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.0f, 0.48f, 0.80f, 0.40f));
        if (ImGui::Begin("##DebugToolbar", nullptr,
            ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoDocking))
        {
            float btnH = toolbarH - 10.0f;
            float btnW = 52.0f;

            // Execute / Run 按钮（绿色）
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.16f, 0.48f, 0.28f, 1.00f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.22f, 0.60f, 0.35f, 1.00f));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.28f, 0.70f, 0.40f, 1.00f));
            bool canRun = isIdle || isStopped;
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
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.20f, 0.50f, 0.20f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.28f, 0.60f, 0.28f, 1.0f));
                if (ImGui::Button(ICON_FA_PLAY "##resume", ImVec2(btnH, btnH)))
                    runner.Resume();
                if (ImGui::IsItemHovered()) ImGui::SetTooltip("Resume");
                ImGui::PopStyleColor(2);
            }
            else
            {
                if (ImGui::Button(ICON_FA_PAUSE "##pause", ImVec2(btnH, btnH)))
                    runner.Pause();
                if (ImGui::IsItemHovered()) ImGui::SetTooltip("Pause");
            }
            if (!isRunning && !isPaused) ImGui::EndDisabled();

            ImGui::SameLine(0, 4);

            // Step
            if (!isPaused) ImGui::BeginDisabled();
            if (ImGui::Button(ICON_FA_ARROW_RIGHT "##step", ImVec2(btnH, btnH)))
            {
                runner.Resume();
                runner.Tick(0.016f);
                runner.Pause();
                const auto& result = ActiveDoc()->lastExecutionResult;
                if (!result.executedNodeIds.empty())
                {
                    auto lastNodeId = result.executedNodeIds.back();
                    ActiveDoc()->executedNodeHighlight[static_cast<uint64_t>(lastNodeId)] = 3.0f;
                }
            }
            if (ImGui::IsItemHovered()) ImGui::SetTooltip("Step");
            if (!isPaused) ImGui::EndDisabled();

            ImGui::SameLine(0, 4);

            // Stop
            if (isIdle || isStopped) ImGui::BeginDisabled();
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.55f, 0.15f, 0.15f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.70f, 0.20f, 0.20f, 1.0f));
            if (ImGui::Button(ICON_FA_STOP "##stop", ImVec2(btnH, btnH)))
                runner.Stop();
            if (ImGui::IsItemHovered()) ImGui::SetTooltip("Stop");
            ImGui::PopStyleColor(2);
            if (isIdle || isStopped) ImGui::EndDisabled();

            // 状态指示
            ImGui::SameLine(0, 12);
            ImVec4 stateCol;
            const char* stateText;
            if (isRunning)      { stateCol = ImVec4(0.2f, 0.8f, 0.2f, 1.0f);  stateText = ICON_FA_CIRCLE_PLAY; }
            else if (isPaused)  { stateCol = ImVec4(1.0f, 0.7f, 0.1f, 1.0f);  stateText = ICON_FA_PAUSE; }
            else if (isStopped) { stateCol = ImVec4(1.0f, 0.3f, 0.3f, 1.0f);  stateText = ICON_FA_CIRCLE_STOP; }
            else                { stateCol = ImVec4(0.5f, 0.5f, 0.5f, 1.0f);  stateText = ""; }
            if (stateText[0] != '\0')
                ImGui::TextColored(stateCol, "%s", stateText);
        }
        ImGui::End();
        ImGui::PopStyleColor(2);
        ImGui::PopStyleVar(2);
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
        float barH = 22.0f;
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

        if (ImGui::Button(ICON_FA_FLOPPY_DISK " Save", ImVec2(buttonWidth, 0)))
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
        if (ImGui::Button(ICON_FA_TRASH_CAN " Don't Save", ImVec2(buttonWidth, 0)))
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
        if (ImGui::Button(ICON_FA_XMARK " Cancel", ImVec2(buttonWidth, 0)))
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
    // 无文档时不渲染（避免 ActiveDoc() 为 nullptr 崩溃）
    if (!ActiveDoc())
    {
        ImGui::TextDisabled("No blueprint open.");
        return;
    }

    auto& io = ImGui::GetIO();
    float paneWidth = ImGui::GetContentRegionAvail().x;

    // 面板标题（VS 2022 风格：扁平低调）
    {
        auto* drawList = ImGui::GetWindowDrawList();
        ImVec2 cursorPos = ImGui::GetCursorScreenPos();
        float headerH = ImGui::GetTextLineHeight() + 6.0f;

        // VS 风格：扁平色带，与背景微差
        drawList->AddRectFilled(
            cursorPos,
            ImVec2(cursorPos.x + paneWidth, cursorPos.y + headerH),
            IM_COL32(30, 30, 38, 230), 0.0f);
        // 底部 1px 分隔线
        drawList->AddLine(
            ImVec2(cursorPos.x, cursorPos.y + headerH - 1.0f),
            ImVec2(cursorPos.x + paneWidth, cursorPos.y + headerH - 1.0f),
            IM_COL32(0, 122, 204, 100));
        drawList->AddText(
            ImVec2(cursorPos.x + 8.0f, cursorPos.y + 3.0f),
            IM_COL32(140, 170, 210, 240), ICON_FA_SITEMAP);
        drawList->AddText(
            ImVec2(cursorPos.x + 26.0f, cursorPos.y + 3.0f),
            IM_COL32(200, 210, 225, 240), "Inspector");
        ImGui::Dummy(ImVec2(paneWidth, headerH));
    }

    ImGui::Spacing();

    // 工具栏按钮（紧凑行）
    static bool showStyleEditor = false;
    ImGui::BeginHorizontal("Style Editor", ImVec2(paneWidth, 0));
    ImGui::Spring(0.0f, 0.0f);
    if (ImGui::Button(ICON_FA_EXPAND " Zoom"))
        ed::NavigateToContent();
    ImGui::Spring(0.0f);
    if (ImGui::Button(ICON_FA_BOLT " Flow"))
    {
        for (auto& link : ActiveDoc()->links)
            ed::Flow(link.ID);
    }
    ImGui::Spring();
    if (ImGui::Button(ICON_FA_PALETTE " Style"))
        showStyleEditor = true;
    ImGui::EndHorizontal();
    ImGui::Checkbox(ICON_FA_TABLE_CELLS " Ordinals", &m_ShowOrdinals);

    if (showStyleEditor)
        ShowStyleEditor(&showStyleEditor);

    // ── Tab Bar：Nodes / Variables ──────────────────────────────────────────
    if (ImGui::BeginTabBar("##InspectorTabs"))
    {
        // ── Nodes Tab ───────────────────────────────────────────────────────
        if (ImGui::BeginTabItem(ICON_FA_CUBES " Nodes"))
        {
            // 节点过滤器
            char* nodeFilterBuf = ActiveDoc()->nodeFilterBuf;
            ImGui::SetNextItemWidth(paneWidth);
            ImGui::InputTextWithHint("##NodeFilter", ICON_FA_MAGNIFYING_GLASS " Filter nodes...", nodeFilterBuf, 128);

            std::string nodeFilter(nodeFilterBuf);
            std::string lowerFilter = nodeFilter;
            for (auto& c : lowerFilter)
                c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));

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

            // 节点列表 — VS 2022 扁平色带
            {
                auto* drawList = ImGui::GetWindowDrawList();
                ImVec2 cursorPos = ImGui::GetCursorScreenPos();
                float sectionH = ImGui::GetTextLineHeight() + 4.0f;
                drawList->AddRectFilled(
                    cursorPos,
                    ImVec2(cursorPos.x + paneWidth, cursorPos.y + sectionH),
                    IM_COL32(30, 30, 38, 230), 0.0f);
                drawList->AddLine(
                    ImVec2(cursorPos.x, cursorPos.y + sectionH - 1.0f),
                    ImVec2(cursorPos.x + paneWidth, cursorPos.y + sectionH - 1.0f),
                    IM_COL32(0, 122, 204, 100));
                drawList->AddText(
                    ImVec2(cursorPos.x + 8.0f, cursorPos.y + 2.0f),
                    IM_COL32(200, 200, 210, 230),
                    nodeFilter.empty() ? ICON_FA_CUBES " Nodes" : ICON_FA_CUBES " Nodes (filtered)");
                ImGui::Dummy(ImVec2(paneWidth, sectionH));
            }
            // 节点过滤缓存：只在 filter 或节点列表变化时重建 tolower 映射
            if (lowerFilter != m_LastNodeFilter)
            {
                m_LastNodeFilter = lowerFilter;
                m_NodeFilterCache.clear();
            }
            if (!lowerFilter.empty())
            {
                for (const auto& n : ActiveDoc()->nodes)
                {
                    uintptr_t key = reinterpret_cast<uintptr_t>(n.ID.AsPointer());
                    if (m_NodeFilterCache.find(key) == m_NodeFilterCache.end())
                    {
                        NodeFilterCache fc;
                        fc.nameLower = n.Name;
                        for (auto& c : fc.nameLower)
                            c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
                        fc.defIdLower = n.DefinitionId;
                        for (auto& c : fc.defIdLower)
                            c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
                        m_NodeFilterCache[key] = std::move(fc);
                    }
                }
            }

            ImGui::Indent();
            for (auto& node : ActiveDoc()->nodes)
            {
                if (!lowerFilter.empty())
                {
                    uintptr_t key = reinterpret_cast<uintptr_t>(node.ID.AsPointer());
                    auto it = m_NodeFilterCache.find(key);
                    if (it == m_NodeFilterCache.end()) continue;
                    if (it->second.nameLower.find(lowerFilter) == std::string::npos &&
                        it->second.defIdLower.find(lowerFilter) == std::string::npos)
                        continue;
                }
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
#if IMGUI_VERSION_NUM >= 18967
                ImGui::SetNextItemAllowOverlap();
#endif
                if (ImGui::Selectable((node.Name + "##" + std::to_string(reinterpret_cast<uintptr_t>(node.ID.AsPointer()))).c_str(), &isSelected))
                {
                    if (io.KeyCtrl)
                    {
                        if (isSelected) ed::SelectNode(node.ID, true);
                        else            ed::DeselectNode(node.ID);
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
                    paneWidth - ImGui::GetStyle().FramePadding.x - ImGui::GetStyle().IndentSpacing - saveIconWidth - restoreIconWidth - ImGui::GetStyle().ItemInnerSpacing.x,
                    (ImGui::GetTextLineHeight() - saveIconHeight) / 2);
                ImGui::GetWindowDrawList()->AddText(
                    ImVec2(iconPanelPos.x - textSize.x - ImGui::GetStyle().ItemInnerSpacing.x, start.y),
                    IM_COL32(255, 255, 255, 255), id.c_str(), nullptr);

                auto drawList = ImGui::GetWindowDrawList();
                ImGui::SetCursorScreenPos(iconPanelPos);
#if IMGUI_VERSION_NUM < 18967
                ImGui::SetItemAllowOverlap();
#else
                ImGui::SetNextItemAllowOverlap();
#endif
                if (node.SavedState.empty())
                {
                    if (ImGui::InvisibleButton("save", ImVec2((float)saveIconWidth, (float)saveIconHeight)))
                        node.SavedState = node.State;
                    if (ImGui::IsItemActive())
                        drawList->AddImage(m_SaveIcon, ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), ImVec2(0,0), ImVec2(1,1), IM_COL32(255,255,255,96));
                    else if (ImGui::IsItemHovered())
                        drawList->AddImage(m_SaveIcon, ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), ImVec2(0,0), ImVec2(1,1), IM_COL32(255,255,255,255));
                    else
                        drawList->AddImage(m_SaveIcon, ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), ImVec2(0,0), ImVec2(1,1), IM_COL32(255,255,255,160));
                }
                else
                {
                    ImGui::Dummy(ImVec2((float)saveIconWidth, (float)saveIconHeight));
                    drawList->AddImage(m_SaveIcon, ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), ImVec2(0,0), ImVec2(1,1), IM_COL32(255,255,255,32));
                }

                ImGui::SameLine(0, ImGui::GetStyle().ItemInnerSpacing.x);
#if IMGUI_VERSION_NUM < 18967
                ImGui::SetItemAllowOverlap();
#else
                ImGui::SetNextItemAllowOverlap();
#endif
                if (!node.SavedState.empty())
                {
                    if (ImGui::InvisibleButton("restore", ImVec2((float)restoreIconWidth, (float)restoreIconHeight)))
                    {
                        node.State = node.SavedState;
                        ed::RestoreNodeState(node.ID);
                        node.SavedState.clear();
                    }
                    if (ImGui::IsItemActive())
                        drawList->AddImage(m_RestoreIcon, ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), ImVec2(0,0), ImVec2(1,1), IM_COL32(255,255,255,96));
                    else if (ImGui::IsItemHovered())
                        drawList->AddImage(m_RestoreIcon, ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), ImVec2(0,0), ImVec2(1,1), IM_COL32(255,255,255,255));
                    else
                        drawList->AddImage(m_RestoreIcon, ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), ImVec2(0,0), ImVec2(1,1), IM_COL32(255,255,255,160));
                }
                else
                {
                    ImGui::Dummy(ImVec2((float)restoreIconWidth, (float)restoreIconHeight));
                    drawList->AddImage(m_RestoreIcon, ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), ImVec2(0,0), ImVec2(1,1), IM_COL32(255,255,255,32));
                }

                ImGui::SameLine(0, 0);
#if IMGUI_VERSION_NUM < 18967
                ImGui::SetItemAllowOverlap();
#endif
                ImGui::Dummy(ImVec2(0, (float)restoreIconHeight));
                ImGui::PopID();
            }
            ImGui::Unindent();

            // 选择信息 — VS 2022 扁平色带
            static int changeCount = 0;
            {
                auto* drawList = ImGui::GetWindowDrawList();
                ImVec2 cursorPos = ImGui::GetCursorScreenPos();
                float sectionH = ImGui::GetTextLineHeight() + 4.0f;
                drawList->AddRectFilled(
                    cursorPos,
                    ImVec2(cursorPos.x + paneWidth, cursorPos.y + sectionH),
                    IM_COL32(30, 30, 38, 230), 0.0f);
                drawList->AddLine(
                    ImVec2(cursorPos.x, cursorPos.y + sectionH - 1.0f),
                    ImVec2(cursorPos.x + paneWidth, cursorPos.y + sectionH - 1.0f),
                    IM_COL32(0, 122, 204, 100));
                drawList->AddText(
                    ImVec2(cursorPos.x + 8.0f, cursorPos.y + 2.0f),
                    IM_COL32(200, 200, 210, 230), ICON_FA_HAND_POINTER " Selection");
                ImGui::Dummy(ImVec2(paneWidth, sectionH));
            }
            ImGui::BeginHorizontal("Selection Stats", ImVec2(paneWidth, 0));
            ImGui::Text("Changed %d time%s", changeCount, changeCount > 1 ? "s" : "");
            ImGui::Spring();
            if (ImGui::Button(ICON_FA_XMARK " Deselect"))
                ed::ClearSelection();
            ImGui::EndHorizontal();
            ImGui::Indent();
            for (int i = 0; i < nodeCount; ++i) ImGui::Text("Node (%p)", selectedNodes[i].AsPointer());
            for (int i = 0; i < linkCount; ++i) ImGui::Text("Link (%p)", selectedLinks[i].AsPointer());
            ImGui::Unindent();

            if (ImGui::IsKeyPressed(ImGuiKey_Z))
                for (auto& link : ActiveDoc()->links)
                    ed::Flow(link.ID);

            if (ed::HasSelectionChanged())
                ++changeCount;

            ImGui::EndTabItem();
        }

        // ── Variables Tab（FunctionLibrary 蓝图不显示，函数库无实例状态）──────
        bool isLibrary = ActiveDoc() && ActiveDoc()->blueprintClass == RTBlueprintClass::FunctionLibrary;
        if (!isLibrary && ImGui::BeginTabItem(ICON_FA_LAYER_GROUP " Variables"))
        {
            DrawVariablePanel();
            ImGui::EndTabItem();
        }

        // ── Functions Tab ───────────────────────────────────────────────────
        if (ImGui::BeginTabItem(ICON_FA_CODE_BRANCH " Functions"))
        {
            auto* doc = ActiveDoc();
            if (doc)
            {
                // --- Add Function 按钮 ---
                if (ImGui::Button(ICON_FA_PLUS " Add Function"))
                {
                    PushUndoState();

                    RTFunctionDefinition newFunc;
                    int maxIdx = 0;
                    for (const auto& f : doc->functions)
                    {
                        if (f.id.rfind("func_", 0) == 0)
                        {
                            int n = std::atoi(f.id.c_str() + 5);
                            if (n > maxIdx) maxIdx = n;
                        }
                    }
                    int nextIdx = maxIdx + 1;
                    newFunc.id       = "func_" + std::to_string(nextIdx);
                    newFunc.name     = "NewFunction_" + std::to_string(nextIdx);
                    newFunc.category = "Custom";
                    newFunc.isPublic = true;
                    doc->functions.push_back(std::move(newFunc));
                    int newFuncIdx = static_cast<int>(doc->functions.size()) - 1;
                    doc->selectedFuncIdx = newFuncIdx;

                    // 自动创建 Entry/Return 节点 + Flow 连线
                    {
                        auto& fn = doc->functions[newFuncIdx];
                        float spawnY = 0.0f;
                        for (const auto& n : doc->nodes)
                        {
                            auto pos = ed::GetNodePosition(n.ID);
                            auto sz  = ed::GetNodeSize(n.ID);
                            float bottom = pos.y + sz.y;
                            if (bottom > spawnY) spawnY = bottom;
                        }
                        spawnY += 80.0f;

                        Node* entryNode = SpawnNodeByDef("Function.Entry");
                        Node* returnNode = SpawnNodeByDef("Function.Return");
                        if (entryNode)
                        {
                            entryNode->Name = fn.name;
                            ed::SetNodePosition(entryNode->ID, ImVec2(100.0f, spawnY));
                        }
                        if (returnNode)
                        {
                            returnNode->Name = fn.name;
                            ed::SetNodePosition(returnNode->ID, ImVec2(500.0f, spawnY));
                        }

                        // 连接 Entry Flow → Return Flow
                        if (entryNode && returnNode &&
                            !entryNode->Outputs.empty() && !returnNode->Inputs.empty() &&
                            entryNode->Outputs[0].Type == PinType::Flow &&
                            returnNode->Inputs[0].Type == PinType::Flow)
                        {
                            doc->links.push_back(Link(GetNextLinkId(),
                                entryNode->Outputs[0].ID, returnNode->Inputs[0].ID));
                            doc->links.back().Color = GetIconColor(PinType::Flow);
                        }

                        SyncFunctionPinsToNodes(fn);
                        BuildNodes();

                        // 导航到新创建的 Entry 节点
                        if (entryNode)
                        {
                            ed::ClearSelection();
                            ed::SelectNode(entryNode->ID, false);
                            ed::NavigateToSelection();
                        }
                    }
                    doc->isDirty = true;
                }
                ImGui::Separator();

                // --- 函数列表 ---
                static int renamingIdx = -1;
                static char renameBuf[128] = {};
                int deleteIdx = -1;  // 延迟删除索引
                for (int i = 0; i < (int)doc->functions.size(); ++i)
                {
                    auto& func = doc->functions[i];
                    ImGui::PushID(i);

                    bool isSelected = (doc->selectedFuncIdx == i);

                    if (renamingIdx == i)
                    {
                        if (ImGui::InputText("##rename", renameBuf, sizeof(renameBuf),
                                             ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_AutoSelectAll))
                        {
                            std::string oldName = func.name;
                            func.name = renameBuf;
                            for (auto& node : doc->nodes)
                            {
                                if ((node.DefinitionId == "Function.Entry" || node.DefinitionId == "Function.Return")
                                    && node.Name == oldName)
                                    node.Name = func.name;
                            }
                            doc->isDirty = true;
                            renamingIdx = -1;
                        }
                        if (!ImGui::IsItemActive() && ImGui::IsMouseClicked(0))
                            renamingIdx = -1;
                    }
                    else
                    {
                        if (ImGui::Selectable(func.name.c_str(), isSelected, ImGuiSelectableFlags_AllowDoubleClick))
                        {
                            doc->selectedFuncIdx = i;
                        }
                        if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0))
                        {
                            renamingIdx = i;
                            snprintf(renameBuf, sizeof(renameBuf), "%s", func.name.c_str());
                            ImGui::SetKeyboardFocusHere(-1);
                        }

                        // 右键上下文菜单
                        if (ImGui::BeginPopupContextItem("##funcctx"))
                        {
                            if (ImGui::MenuItem(ICON_FA_PEN " Rename"))
                            {
                                renamingIdx = i;
                                snprintf(renameBuf, sizeof(renameBuf), "%s", func.name.c_str());
                            }
                            if (ImGui::MenuItem(ICON_FA_MAGNIFYING_GLASS " Jump to Entry"))
                            {
                                for (const auto& node : doc->nodes)
                                {
                                    if (node.DefinitionId == "Function.Entry" && node.Name == func.name)
                                    {
                                        ed::ClearSelection();
                                        ed::SelectNode(node.ID, false);
                                        ed::NavigateToSelection();
                                        break;
                                    }
                                }
                            }
                            ImGui::Separator();
                            if (ImGui::MenuItem(ICON_FA_TRASH " Delete"))
                            {
                                deleteIdx = i;
                            }
                            ImGui::EndPopup();
                        }

                        // Delete 键删除
                        if (ImGui::IsItemFocused() && ImGui::IsKeyPressed(ImGuiKey_Delete))
                            deleteIdx = i;
                    }

                    // 删除按钮（🗑）
                    ImGui::SameLine();
                    if (ImGui::SmallButton(ICON_FA_TRASH "##delfunc"))
                        deleteIdx = i;
                    if (ImGui::IsItemHovered())
                        ImGui::SetTooltip("Delete function");

                    // 跳转按钮
                    ImGui::SameLine();
                    if (ImGui::SmallButton(ICON_FA_MAGNIFYING_GLASS "##gotofunc"))
                    {
                        for (const auto& node : doc->nodes)
                        {
                            if (node.DefinitionId == "Function.Entry" && node.Name == func.name)
                            {
                                ed::ClearSelection();
                                ed::SelectNode(node.ID, false);
                                ed::NavigateToSelection();
                                break;
                            }
                        }
                    }
                    if (ImGui::IsItemHovered())
                        ImGui::SetTooltip("Jump to Function.Entry node");

                    ImGui::PopID();
                }

                // 延迟执行删除（避免在迭代中删除）
                if (deleteIdx >= 0 && deleteIdx < (int)doc->functions.size())
                {
                    PushUndoState();
                    const auto& funcName = doc->functions[deleteIdx].name;
                    for (auto nit = doc->nodes.begin(); nit != doc->nodes.end(); )
                    {
                        if ((nit->DefinitionId == "Function.Entry" || nit->DefinitionId == "Function.Return")
                            && nit->Name == funcName)
                        {
                            doc->links.erase(std::remove_if(doc->links.begin(), doc->links.end(),
                                [&](const Link& lnk) {
                                    for (auto& p : nit->Inputs)
                                        if (lnk.StartPinID == p.ID || lnk.EndPinID == p.ID) return true;
                                    for (auto& p : nit->Outputs)
                                        if (lnk.StartPinID == p.ID || lnk.EndPinID == p.ID) return true;
                                    return false;
                                }), doc->links.end());
                            nit = doc->nodes.erase(nit);
                        }
                        else
                            ++nit;
                    }
                    doc->functions.erase(doc->functions.begin() + deleteIdx);
                    if (doc->selectedFuncIdx >= (int)doc->functions.size())
                        doc->selectedFuncIdx = (int)doc->functions.size() - 1;
                    doc->isDirty = true;
                    BuildNodes();
                }

                if (doc->functions.empty())
                {
                    ImGui::TextDisabled("Click '" ICON_FA_PLUS " Add Function' to create one.");
                    doc->selectedFuncIdx = -1;
                }

                // --- 选中函数的参数编辑面板 ---
                if (doc->selectedFuncIdx >= 0 && doc->selectedFuncIdx < (int)doc->functions.size())
                {
                    ImGui::Spacing();
                    ImGui::Separator();
                    ImGui::Spacing();
                    DrawFunctionDetailsPanel(doc->functions[doc->selectedFuncIdx]);
                }
            }
            ImGui::EndTabItem();
        }

        // ── Events Tab（FunctionLibrary 蓝图不显示，函数库无事件驱动）──────────
        if (!isLibrary && ImGui::BeginTabItem(ICON_FA_BOLT " Events"))
        {
            auto events = RTEventBus::Get().GetRegisteredEvents();
            if (events.empty())
            {
                ImGui::TextDisabled("No events registered.");
            }
            else
            {
                static char payloadBuf[256] = {};
                static std::string fireEventTarget;

                for (const auto& evtName : events)
                {
                    ImGui::PushID(evtName.c_str());
                    ImGui::Text("%s", evtName.c_str());
                    ImGui::SameLine();
                    if (ImGui::SmallButton("Fire"))
                    {
                        fireEventTarget = evtName;
                        ImGui::OpenPopup("##fire_payload");
                    }
                    ImGui::SameLine();
                    if (ImGui::SmallButton("Clear"))
                    {
                        RTEventBus::Get().ClearEvent(evtName);
                    }
                    ImGui::PopID();
                }

                // Fire payload popup
                if (ImGui::BeginPopup("##fire_payload"))
                {
                    ImGui::Text("Payload for '%s':", fireEventTarget.c_str());
                    ImGui::InputText("##payload", payloadBuf, sizeof(payloadBuf));
                    if (ImGui::Button("Send"))
                    {
                        RTVariant pl;
                        pl.type = RTPinDataType::String;
                        pl.stringValue = std::string(payloadBuf);
                        RTEventBus::Get().Fire(fireEventTarget, pl);
                        payloadBuf[0] = '\0';
                        ImGui::CloseCurrentPopup();
                    }
                    ImGui::SameLine();
                    if (ImGui::Button("Cancel"))
                        ImGui::CloseCurrentPopup();
                    ImGui::EndPopup();
                }
            }

            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem(ICON_FA_LIST " Library"))
        {
            DrawNodeLibraryPanel();
            ImGui::EndTabItem();
        }

        // ── Details Tab ─────────────────────────────────────────────────────
        if (ImGui::BeginTabItem(ICON_FA_CIRCLE_INFO " Details"))
        {
            DrawDetailsPanel();
            ImGui::EndTabItem();
        }

        ImGui::EndTabBar();
    }
}

// ============================================================================
// 变量面板（在 DrawNodeListPanel 的 Variables Tab 内调用）
// ============================================================================

void BlueprintEditor::DrawVariablePanel()
{
    

    auto* doc = ActiveDoc();
    if (!doc) return;

    float paneWidth = ImGui::GetContentRegionAvail().x;

    // ── 工具栏：添加变量 + 类型选择 ─────────────────────────────────────
    // 新建变量弹窗状态存在 doc 里，多文档切换互不干扰
    bool& showAddPopup = doc->varAddPopupOpen;
    char* newVarName   = doc->varNewName;
    int&  newVarTypeIdx = doc->varNewTypeIdx;

    if (ImGui::Button(ICON_FA_PLUS " Add Variable"))
    {
        memset(newVarName, 0, sizeof(newVarName));
        newVarTypeIdx = 1;
        showAddPopup = true;
        ImGui::OpenPopup("##AddVariable");
    }

    // ── 新建变量弹窗 ──────────────────────────────────────────────────────
    if (ImGui::BeginPopup("##AddVariable"))
    {
        ImGui::TextUnformatted("New Variable");
        ImGui::Separator();

        ImGui::SetNextItemWidth(160.0f);
        ImGui::InputTextWithHint("##VarName", "Variable name...", newVarName, sizeof(newVarName));

        const char* typeNames[] = { "Unknown", "Boolean", "Integer", "Float", "String", "Object", "Array", "Map", "Any" };
        const RTPinDataType typeValues[] = {
            RTPinDataType::Unknown, RTPinDataType::Boolean, RTPinDataType::Integer,
            RTPinDataType::Float, RTPinDataType::String, RTPinDataType::Object,
            RTPinDataType::Array, RTPinDataType::Map, RTPinDataType::Any
        };
        ImGui::SetNextItemWidth(160.0f);
        ImGui::Combo("##VarType", &newVarTypeIdx, typeNames, IM_ARRAYSIZE(typeNames));

        ImGui::Spacing();
        bool canAdd = (newVarName[0] != '\0');
        if (!canAdd) ImGui::BeginDisabled();
        if (ImGui::Button("Add") && canAdd)
        {
            // 检查重名
            bool dup = false;
            for (auto& v : doc->variables)
                if (v.name == newVarName) { dup = true; break; }

            if (!dup)
            {
                PushUndoState();  // 新增变量前保存快照
                RTVariableDefinition var;
                var.name      = newVarName;
                var.dataType  = typeValues[newVarTypeIdx];
                var.isExposed = true;
                doc->variables.push_back(std::move(var));
                doc->isDirty = true;
                ImGui::CloseCurrentPopup();
                showAddPopup = false;
            }
            else
            {
                ImGui::TextColored(ImVec4(1,0.4f,0.4f,1), "Name already exists!");
            }
        }
        if (!canAdd) ImGui::EndDisabled();
        ImGui::SameLine();
        if (ImGui::Button("Cancel"))
        {
            ImGui::CloseCurrentPopup();
            showAddPopup = false;
        }
        ImGui::EndPopup();
    }

    ImGui::Spacing();

    if (doc->variables.empty())
    {
        ImGui::TextDisabled("No variables defined.");
        ImGui::TextDisabled("Click '" ICON_FA_PLUS " Add Variable' to create one.");
        return;
    }

    // ── 变量列表 ──────────────────────────────────────────────────────────
    // 类型名映射
    auto typeToStr = [](RTPinDataType t) -> const char* {
        switch (t) {
        case RTPinDataType::Boolean: return "Bool";
        case RTPinDataType::Integer: return "Int";
        case RTPinDataType::Float:   return "Float";
        case RTPinDataType::String:  return "String";
        case RTPinDataType::Object:  return "Object";
        case RTPinDataType::Array:   return "Array";
        case RTPinDataType::Map:     return "Map";
        case RTPinDataType::Any:     return "Any";
        default:                   return "Unknown";
        }
    };

    // 类型颜色
    auto typeColor = [](RTPinDataType t) -> ImVec4 {
        switch (t) {
        case RTPinDataType::Boolean: return ImVec4(0.9f, 0.4f, 0.4f, 1.0f);
        case RTPinDataType::Integer: return ImVec4(0.4f, 0.8f, 0.4f, 1.0f);
        case RTPinDataType::Float:   return ImVec4(0.4f, 0.7f, 1.0f, 1.0f);
        case RTPinDataType::String:  return ImVec4(1.0f, 0.8f, 0.3f, 1.0f);
        case RTPinDataType::Object:  return ImVec4(0.8f, 0.5f, 1.0f, 1.0f);
        case RTPinDataType::Array:   return ImVec4(0.5f, 1.0f, 0.8f, 1.0f);
        case RTPinDataType::Map:     return ImVec4(1.0f, 0.6f, 0.2f, 1.0f);
        default:                   return ImVec4(0.7f, 0.7f, 0.7f, 1.0f);
        }
    };

    int deleteIdx = -1;  // 待删除的变量下标（延迟删除，避免迭代时修改容器）

    for (int i = 0; i < (int)doc->variables.size(); ++i)
    {
        auto& var = doc->variables[i];
        ImGui::PushID(i);

        // 类型色标（同时作为拖拽手柄）
        ImVec2 dotPos = ImGui::GetCursorScreenPos() + ImVec2(4.0f, ImGui::GetTextLineHeight() * 0.5f - 4.0f);
        ImGui::GetWindowDrawList()->AddCircleFilled(dotPos + ImVec2(4,4), 5.0f, ImGui::ColorConvertFloat4ToU32(typeColor(var.dataType)));
        ImGui::Dummy(ImVec2(14.0f, ImGui::GetTextLineHeight()));

        // 拖拽源：从色标或变量名开始拖拽
        if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID))
        {
            VarDragPayload payload;
            snprintf(payload.varName, sizeof(payload.varName), "%s", var.name.c_str());
            payload.dataType = static_cast<int>(var.dataType);
            ImGui::SetDragDropPayload(VAR_DRAG_DROP_TYPE, &payload, sizeof(payload));
            // 拖拽预览提示
            ImGui::TextColored(typeColor(var.dataType), "● %s  [%s]", var.name.c_str(), typeToStr(var.dataType));
            ImGui::TextDisabled("Drop → Get node   Shift+Drop → Set node");
            ImGui::EndDragDropSource();
        }

        ImGui::SameLine(0, 2.0f);

        // 变量名（可内联重命名）
        char nameBuf[64];
        snprintf(nameBuf, sizeof(nameBuf), "%s", var.name.c_str());
        ImGui::SetNextItemWidth(paneWidth - 80.0f);
        if (ImGui::InputText("##vname", nameBuf, sizeof(nameBuf), ImGuiInputTextFlags_EnterReturnsTrue))
        {
            if (nameBuf[0] != '\0' && var.name != nameBuf)
            {
                // 检查重名
                bool dup = false;
                for (int j = 0; j < (int)doc->variables.size(); ++j)
                    if (j != i && doc->variables[j].name == nameBuf) { dup = true; break; }
                if (!dup)
                {
                    PushUndoState();  // 重命名前保存快照
                    var.name = nameBuf;
                    doc->isDirty = true;
                }
            }
        }
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("Press Enter to rename");

        ImGui::SameLine();

        // 类型标签（点击切换类型）
        ImGui::TextColored(typeColor(var.dataType), "[%s]", typeToStr(var.dataType));
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("Click to change type");
        if (ImGui::IsItemClicked())
            ImGui::OpenPopup("##VarType");

        if (ImGui::BeginPopup("##VarType"))
        {
            const char* typeNames[] = { "Boolean", "Integer", "Float", "String", "Object", "Array", "Map", "Any" };
            const RTPinDataType typeValues[] = {
                RTPinDataType::Boolean, RTPinDataType::Integer, RTPinDataType::Float,
                RTPinDataType::String, RTPinDataType::Object, RTPinDataType::Array,
                RTPinDataType::Map, RTPinDataType::Any
            };
            for (int t = 0; t < IM_ARRAYSIZE(typeNames); ++t)
            {
                ImGui::TextColored(typeColor(typeValues[t]), "●");
                ImGui::SameLine();
                if (ImGui::Selectable(typeNames[t], var.dataType == typeValues[t]))
                {
                    PushUndoState();  // 类型修改前保存快照
                    var.dataType = typeValues[t];
                    doc->isDirty = true;
                    ImGui::CloseCurrentPopup();
                }
            }
            ImGui::EndPopup();
        }

        ImGui::SameLine();

        // 删除按钮
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0,0,0,0));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.8f,0.2f,0.2f,0.6f));
        if (ImGui::SmallButton(ICON_FA_TRASH_CAN))
            deleteIdx = i;
        ImGui::PopStyleColor(2);
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("Delete variable");

        // Tooltip：展示变量详情
        if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort))
        {
            ImGui::BeginTooltip();
            ImGui::Text("Name:     %s", var.name.c_str());
            ImGui::Text("Type:     %s", typeToStr(var.dataType));
            ImGui::Text("Exposed:  %s", var.isExposed ? "Yes" : "No");
            if (!var.tooltip.empty()) ImGui::Text("Tip: %s", var.tooltip.c_str());
            ImGui::EndTooltip();
        }

        ImGui::PopID();
    }

    // 延迟删除
    if (deleteIdx >= 0)
    {
        PushUndoState();  // 删除变量前保存快照
        doc->variables.erase(doc->variables.begin() + deleteIdx);
        doc->isDirty = true;
    }
}

// ============================================================================
// Details 面板（选中节点的属性检查器）
// ============================================================================

void BlueprintEditor::DrawDetailsPanel()
{
    auto* doc = ActiveDoc();
    if (!doc) return;

    float paneWidth = ImGui::GetContentRegionAvail().x;

    // 获取选中的节点
    std::vector<ed::NodeId> selectedNodes;
    selectedNodes.resize(ed::GetSelectedObjectCount());
    int nodeCount = ed::GetSelectedNodes(selectedNodes.data(), static_cast<int>(selectedNodes.size()));
    selectedNodes.resize(nodeCount);

    if (selectedNodes.empty())
    {
        ImGui::Spacing();
        ImGui::TextDisabled("  No node selected.");
        ImGui::TextDisabled("  Select a node on the canvas");
        ImGui::TextDisabled("  to view its details.");
        return;
    }

    if (selectedNodes.size() > 1)
    {
        ImGui::Spacing();
        ImGui::TextDisabled("  %d nodes selected.", static_cast<int>(selectedNodes.size()));
        ImGui::TextDisabled("  Select a single node to edit.");
        return;
    }

    // 单节点选中 —— 显示详细属性
    Node* node = FindNode(selectedNodes[0]);
    if (!node) return;

    // 类型→名称映射
    auto pinTypeStr = [](PinType t) -> const char* {
        switch (t) {
        case PinType::Flow:     return "Flow";
        case PinType::Bool:     return "Bool";
        case PinType::Int:      return "Int";
        case PinType::Float:    return "Float";
        case PinType::String:   return "String";
        case PinType::Object:   return "Object";
        case PinType::Function: return "Function";
        case PinType::Delegate: return "Delegate";
        case PinType::Array:    return "Array";
        case PinType::Map:      return "Map";
        case PinType::Any:      return "Any";
        default:                return "Unknown";
        }
    };

    // ── 节点基本信息 — VS 2022 扁平色带 ────────────────────────────────
    {
        auto* drawList = ImGui::GetWindowDrawList();
        ImVec2 cursorPos = ImGui::GetCursorScreenPos();
        float sectionH = ImGui::GetTextLineHeight() + 4.0f;
        drawList->AddRectFilled(
            cursorPos,
            ImVec2(cursorPos.x + paneWidth, cursorPos.y + sectionH),
            IM_COL32(30, 30, 38, 230), 0.0f);
        drawList->AddLine(
            ImVec2(cursorPos.x, cursorPos.y + sectionH - 1.0f),
            ImVec2(cursorPos.x + paneWidth, cursorPos.y + sectionH - 1.0f),
            IM_COL32(0, 122, 204, 100));
        drawList->AddText(
            ImVec2(cursorPos.x + 8.0f, cursorPos.y + 2.0f),
            IM_COL32(200, 200, 210, 230), ICON_FA_CUBE " Node Info");
        ImGui::Dummy(ImVec2(paneWidth, sectionH));
    }

    ImGui::Indent(8.0f);

    // 名称
    ImGui::TextColored(ImVec4(0.55f, 0.65f, 0.80f, 1.0f), "Name:");
    ImGui::SameLine(90.0f);
    ImGui::TextUnformatted(node->Name.c_str());

    // ID
    ImGui::TextColored(ImVec4(0.55f, 0.65f, 0.80f, 1.0f), "ID:");
    ImGui::SameLine(90.0f);
    ImGui::Text("%llu", static_cast<unsigned long long>(reinterpret_cast<uintptr_t>(node->ID.AsPointer())));

    // 定义 ID
    ImGui::TextColored(ImVec4(0.55f, 0.65f, 0.80f, 1.0f), "Definition:");
    ImGui::SameLine(90.0f);
    ImGui::TextUnformatted(node->DefinitionId.c_str());

    // 从注册表查找节点定义
    const RTNodeDef* def = m_NodeRegistry.getNodeDefinition(node->DefinitionId);

    // 类别
    if (def && !def->category.empty())
    {
        ImGui::TextColored(ImVec4(0.55f, 0.65f, 0.80f, 1.0f), "Category:");
        ImGui::SameLine(90.0f);
        ImGui::TextUnformatted(def->category.c_str());
    }

    // 节点类型
    {
        const char* typeStr = "Blueprint";
        switch (node->Type)
        {
        case NodeType::Simple:    typeStr = "Simple";    break;
        case NodeType::Tree:      typeStr = "Tree";      break;
        case NodeType::Houdini:   typeStr = "Houdini";   break;
        case NodeType::Comment:   typeStr = "Comment";   break;
        default: break;
        }
        ImGui::TextColored(ImVec4(0.55f, 0.65f, 0.80f, 1.0f), "Type:");
        ImGui::SameLine(90.0f);
        ImGui::TextUnformatted(typeStr);
    }

    // 描述
    if (def && !def->description.empty())
    {
        ImGui::Spacing();
        ImGui::TextColored(ImVec4(0.55f, 0.65f, 0.80f, 1.0f), "Description:");
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.70f, 0.72f, 0.78f, 1.0f));
        ImGui::TextWrapped("%s", def->description.c_str());
        ImGui::PopStyleColor();
    }

    // 纯函数标记
    if (def)
    {
        ImGui::TextColored(ImVec4(0.55f, 0.65f, 0.80f, 1.0f), "Pure:");
        ImGui::SameLine(90.0f);
        ImGui::TextColored(def->isPure ? ImVec4(0.35f, 0.85f, 0.45f, 1.0f) : ImVec4(0.85f, 0.55f, 0.35f, 1.0f),
            "%s", def->isPure ? "Yes" : "No");
    }

    // 错误状态
    if (node->HasError)
    {
        ImGui::Spacing();
        ImGui::TextColored(ImVec4(0.95f, 0.35f, 0.35f, 1.0f), ICON_FA_TRIANGLE_EXCLAMATION " Error: %s", node->ErrorMessage.c_str());
    }

    ImGui::Unindent(8.0f);

    // ── 输入引脚 ─────────────────────────────────────────────────────────
    if (!node->Inputs.empty())
    {
        ImGui::Spacing();
        {
            auto* drawList = ImGui::GetWindowDrawList();
            ImVec2 cursorPos = ImGui::GetCursorScreenPos();
            float sectionH = ImGui::GetTextLineHeight() + 4.0f;
            drawList->AddRectFilled(
                cursorPos,
                ImVec2(cursorPos.x + paneWidth, cursorPos.y + sectionH),
                IM_COL32(30, 30, 38, 230), 0.0f);
            drawList->AddLine(
                ImVec2(cursorPos.x, cursorPos.y + sectionH - 1.0f),
                ImVec2(cursorPos.x + paneWidth, cursorPos.y + sectionH - 1.0f),
                IM_COL32(0, 122, 204, 100));
            drawList->AddText(
                ImVec2(cursorPos.x + 8.0f, cursorPos.y + 2.0f),
                IM_COL32(200, 200, 210, 230), ICON_FA_ARROW_RIGHT " Inputs");
            ImGui::Dummy(ImVec2(paneWidth, sectionH));
        }

        ImGui::Indent(8.0f);
        for (auto& pin : node->Inputs)
        {
            if (pin.IsOrphaned || pin.IsHidden) continue;
            if (pin.Type == PinType::Flow) continue;  // Flow 引脚无值可编辑

            ImGui::PushID(pin.ID.AsPointer());

            // 引脚类型色标
            ImColor pinColor = GetIconColor(pin.Type);
            auto* dl = ImGui::GetWindowDrawList();
            ImVec2 dotPos = ImGui::GetCursorScreenPos() + ImVec2(2.0f, ImGui::GetTextLineHeight() * 0.5f - 3.0f);
            dl->AddCircleFilled(dotPos + ImVec2(3, 3), 4.0f, pinColor);
            ImGui::Dummy(ImVec2(10.0f, ImGui::GetTextLineHeight()));
            ImGui::SameLine(0, 2.0f);

            // 引脚名和类型
            ImGui::TextColored(ImVec4(0.75f, 0.80f, 0.90f, 1.0f), "%s", pin.Name.c_str());
            ImGui::SameLine();
            ImGui::TextColored(ImVec4(0.50f, 0.55f, 0.60f, 0.80f), "[%s]", pinTypeStr(pin.Type));

            // 连线状态
            bool linked = IsPinLinked(pin.ID);
            if (linked)
            {
                ImGui::SameLine();
                ImGui::TextColored(ImVec4(0.35f, 0.70f, 0.95f, 0.80f), ICON_FA_LINK);
            }

            // 值编辑控件（仅在未连线时可编辑）
            if (!linked)
            {
                float editWidth = paneWidth - 24.0f;
                switch (pin.Type)
                {
                case PinType::Bool:
                {
                    bool val = pin.BoolValue;
                    if (ImGui::Checkbox("##val", &val))
                    {
                        PushUndoState();
                        pin.BoolValue = val;
                        doc->isDirty = true;
                    }
                    break;
                }
                case PinType::Int:
                {
                    int ival = static_cast<int>(pin.IntValue);
                    ImGui::SetNextItemWidth(editWidth);
                    if (ImGui::DragInt("##val", &ival, 1.0f))
                    {
                        PushUndoState();
                        pin.IntValue = ival;
                        doc->isDirty = true;
                    }
                    break;
                }
                case PinType::Float:
                {
                    float fval = pin.FloatValue;
                    ImGui::SetNextItemWidth(editWidth);
                    if (ImGui::DragFloat("##val", &fval, 0.1f))
                    {
                        PushUndoState();
                        pin.FloatValue = fval;
                        doc->isDirty = true;
                    }
                    break;
                }
                case PinType::String:
                {
                    uintptr_t pinKey = reinterpret_cast<uintptr_t>(pin.ID.AsPointer());
                    auto& buf = doc->pinStringBuffers[pinKey];
                    // 初始化缓冲区
                    if (buf[0] == '\0' && !pin.StringValue.empty())
                        snprintf(buf.data(), buf.size(), "%s", pin.StringValue.c_str());
                    ImGui::SetNextItemWidth(editWidth);
                    if (ImGui::InputText("##val", buf.data(), buf.size(), ImGuiInputTextFlags_EnterReturnsTrue))
                    {
                        PushUndoState();
                        pin.StringValue = buf.data();
                        doc->isDirty = true;
                    }
                    break;
                }
                case PinType::Object:
                {
                    uintptr_t pinKey = reinterpret_cast<uintptr_t>(pin.ID.AsPointer());
                    auto& buf = doc->pinObjectBuffers[pinKey];
                    if (buf[0] == '\0' && !pin.ObjectValue.empty())
                        snprintf(buf.data(), buf.size(), "%s", pin.ObjectValue.c_str());
                    ImGui::SetNextItemWidth(editWidth);
                    if (ImGui::InputText("##val", buf.data(), buf.size(), ImGuiInputTextFlags_EnterReturnsTrue))
                    {
                        PushUndoState();
                        pin.ObjectValue = buf.data();
                        doc->isDirty = true;
                    }
                    break;
                }
                default:
                    ImGui::TextDisabled("  (no editor)");
                    break;
                }
            }
            else
            {
                ImGui::TextDisabled("  (linked)");
            }

            ImGui::PopID();
        }
        ImGui::Unindent(8.0f);
    }

    // ── 输出引脚 ─────────────────────────────────────────────────────────
    if (!node->Outputs.empty())
    {
        ImGui::Spacing();
        {
            auto* drawList = ImGui::GetWindowDrawList();
            ImVec2 cursorPos = ImGui::GetCursorScreenPos();
            float sectionH = ImGui::GetTextLineHeight() + 4.0f;
            drawList->AddRectFilled(
                cursorPos,
                ImVec2(cursorPos.x + paneWidth, cursorPos.y + sectionH),
                IM_COL32(30, 30, 38, 230), 0.0f);
            drawList->AddLine(
                ImVec2(cursorPos.x, cursorPos.y + sectionH - 1.0f),
                ImVec2(cursorPos.x + paneWidth, cursorPos.y + sectionH - 1.0f),
                IM_COL32(0, 122, 204, 100));
            drawList->AddText(
                ImVec2(cursorPos.x + 8.0f, cursorPos.y + 2.0f),
                IM_COL32(200, 200, 210, 230), ICON_FA_ARROW_LEFT " Outputs");
            ImGui::Dummy(ImVec2(paneWidth, sectionH));
        }

        ImGui::Indent(8.0f);
        for (const auto& pin : node->Outputs)
        {
            if (pin.IsOrphaned || pin.IsHidden) continue;
            if (pin.Type == PinType::Flow) continue;

            ImGui::PushID(pin.ID.AsPointer());

            // 色标
            ImColor pinColor = GetIconColor(pin.Type);
            auto* dl = ImGui::GetWindowDrawList();
            ImVec2 dotPos = ImGui::GetCursorScreenPos() + ImVec2(2.0f, ImGui::GetTextLineHeight() * 0.5f - 3.0f);
            dl->AddCircleFilled(dotPos + ImVec2(3, 3), 4.0f, pinColor);
            ImGui::Dummy(ImVec2(10.0f, ImGui::GetTextLineHeight()));
            ImGui::SameLine(0, 2.0f);

            ImGui::TextColored(ImVec4(0.75f, 0.80f, 0.90f, 1.0f), "%s", pin.Name.c_str());
            ImGui::SameLine();
            ImGui::TextColored(ImVec4(0.50f, 0.55f, 0.60f, 0.80f), "[%s]", pinTypeStr(pin.Type));

            // 上次执行的输出值（只读）
            uint64_t pinIdVal = reinterpret_cast<uintptr_t>(pin.ID.AsPointer());
            auto it = doc->lastExecutionResult.outputValues.find(pinIdVal);
            if (it != doc->lastExecutionResult.outputValues.end())
            {
                std::string valStr = it->second.asString();
                ImGui::TextColored(ImVec4(0.55f, 0.85f, 0.65f, 1.0f), "  = %s", valStr.c_str());
            }
            else
            {
                // 也从 runner 的 pinValues 尝试获取
                auto val = doc->persistentRunner.GetPinValue(pinIdVal);
                if (val.type != RTPinDataType::Unknown)
                {
                    std::string valStr = val.asString();
                    ImGui::TextColored(ImVec4(0.55f, 0.85f, 0.65f, 0.80f), "  = %s", valStr.c_str());
                }
                else
                {
                    ImGui::TextDisabled("  (no value)");
                }
            }

            ImGui::PopID();
        }
        ImGui::Unindent(8.0f);
    }

    // ── 自定义属性（来自 NodeDefinition） ────────────────────────────────
    if (def && !def->customProperties.empty())
    {
        ImGui::Spacing();
        {
            auto* drawList = ImGui::GetWindowDrawList();
            ImVec2 cursorPos = ImGui::GetCursorScreenPos();
            float sectionH = ImGui::GetTextLineHeight() + 4.0f;
            drawList->AddRectFilled(
                cursorPos,
                ImVec2(cursorPos.x + paneWidth, cursorPos.y + sectionH),
                IM_COL32(30, 30, 38, 230), 0.0f);
            drawList->AddLine(
                ImVec2(cursorPos.x, cursorPos.y + sectionH - 1.0f),
                ImVec2(cursorPos.x + paneWidth, cursorPos.y + sectionH - 1.0f),
                IM_COL32(0, 122, 204, 100));
            drawList->AddText(
                ImVec2(cursorPos.x + 8.0f, cursorPos.y + 2.0f),
                IM_COL32(200, 200, 210, 230), ICON_FA_GEAR " Properties");
            ImGui::Dummy(ImVec2(paneWidth, sectionH));
        }

        ImGui::Indent(8.0f);
        for (const auto& kv : def->customProperties)
        {
            ImGui::TextColored(ImVec4(0.60f, 0.65f, 0.80f, 1.0f), "%s:", kv.first.c_str());
            ImGui::SameLine();
            ImGui::TextUnformatted(kv.second.c_str());
        }
        ImGui::Unindent(8.0f);
    }
}

// ============================================================================
// 函数参数编辑面板（在 Functions Tab 中选中函数时显示）
// ============================================================================

void BlueprintEditor::DrawFunctionDetailsPanel(RTFunctionDefinition& func)
{
    auto* doc = ActiveDoc();
    if (!doc) return;

    float paneWidth = ImGui::GetContentRegionAvail().x;

    // 类型选项（与变量面板保持一致）
    static const char* typeNames[] = { "Boolean", "Integer", "Float", "String", "Object" };
    static const RTPinDataType typeValues[] = {
        RTPinDataType::Boolean, RTPinDataType::Integer, RTPinDataType::Float,
        RTPinDataType::String, RTPinDataType::Object
    };
    static const int typeCount = 5;

    auto dataTypeToIndex = [](RTPinDataType dt) -> int {
        switch (dt) {
        case RTPinDataType::Boolean: return 0;
        case RTPinDataType::Integer: return 1;
        case RTPinDataType::Float:   return 2;
        case RTPinDataType::String:  return 3;
        case RTPinDataType::Object:  return 4;
        default:                     return 3; // 默认 String
        }
    };

    // ── 函数信息标题 ──────────────────────────────────────────────────
    {
        auto* drawList = ImGui::GetWindowDrawList();
        ImVec2 cursorPos = ImGui::GetCursorScreenPos();
        float sectionH = ImGui::GetTextLineHeight() + 4.0f;
        drawList->AddRectFilled(
            cursorPos,
            ImVec2(cursorPos.x + paneWidth, cursorPos.y + sectionH),
            IM_COL32(30, 50, 30, 230), 0.0f);
        drawList->AddLine(
            ImVec2(cursorPos.x, cursorPos.y + sectionH - 1.0f),
            ImVec2(cursorPos.x + paneWidth, cursorPos.y + sectionH - 1.0f),
            IM_COL32(96, 192, 96, 100));
        drawList->AddText(
            ImVec2(cursorPos.x + 8.0f, cursorPos.y + 2.0f),
            IM_COL32(200, 210, 200, 230), (ICON_FA_CODE_BRANCH " " + func.name).c_str());
        ImGui::Dummy(ImVec2(paneWidth, sectionH));
    }

    ImGui::Indent(4.0f);

    // Public 标记
    {
        bool pub = func.isPublic;
        if (ImGui::Checkbox("Public", &pub))
        {
            PushUndoState();
            func.isPublic = pub;
            doc->isDirty = true;
        }
    }

    // Category
    {
        static char catBuf[128] = {};
        static std::string lastCatFuncId;
        if (lastCatFuncId != func.id)
        {
            snprintf(catBuf, sizeof(catBuf), "%s", func.category.c_str());
            lastCatFuncId = func.id;
        }
        ImGui::TextColored(ImVec4(0.55f, 0.65f, 0.80f, 1.0f), "Category:");
        ImGui::SetNextItemWidth(paneWidth - 16.0f);
        if (ImGui::InputText("##funccat", catBuf, sizeof(catBuf), ImGuiInputTextFlags_EnterReturnsTrue))
        {
            PushUndoState();
            func.category = catBuf;
            doc->isDirty = true;
        }
    }

    // Description
    {
        static char descBuf[256] = {};
        static std::string lastFuncId;
        if (lastFuncId != func.id)
        {
            snprintf(descBuf, sizeof(descBuf), "%s", func.description.c_str());
            lastFuncId = func.id;
        }
        ImGui::TextColored(ImVec4(0.55f, 0.65f, 0.80f, 1.0f), "Description:");
        ImGui::SetNextItemWidth(paneWidth - 16.0f);
        if (ImGui::InputText("##funcdesc", descBuf, sizeof(descBuf), ImGuiInputTextFlags_EnterReturnsTrue))
        {
            PushUndoState();
            func.description = descBuf;
            doc->isDirty = true;
        }
    }

    ImGui::Spacing();

    // ── Inputs（函数输入参数）────────────────────────────────────────────
    {
        auto* drawList = ImGui::GetWindowDrawList();
        ImVec2 cursorPos = ImGui::GetCursorScreenPos();
        float sectionH = ImGui::GetTextLineHeight() + 4.0f;
        drawList->AddRectFilled(
            cursorPos,
            ImVec2(cursorPos.x + paneWidth, cursorPos.y + sectionH),
            IM_COL32(30, 30, 38, 230), 0.0f);
        drawList->AddLine(
            ImVec2(cursorPos.x, cursorPos.y + sectionH - 1.0f),
            ImVec2(cursorPos.x + paneWidth, cursorPos.y + sectionH - 1.0f),
            IM_COL32(0, 122, 204, 100));
        drawList->AddText(
            ImVec2(cursorPos.x + 8.0f, cursorPos.y + 2.0f),
            IM_COL32(200, 200, 210, 230), ICON_FA_ARROW_RIGHT " Inputs");
        ImGui::Dummy(ImVec2(paneWidth, sectionH));
    }

    ImGui::Indent(4.0f);
    bool inputsChanged = false;
    for (int i = 0; i < (int)func.inputs.size(); ++i)
    {
        auto& param = func.inputs[i];
        ImGui::PushID(("fi" + std::to_string(i)).c_str());

        // 类型下拉
        int typeIdx = dataTypeToIndex(param.dataType);
        ImGui::SetNextItemWidth(70.0f);
        if (ImGui::Combo("##type", &typeIdx, typeNames, typeCount))
        {
            PushUndoState();
            param.dataType = typeValues[typeIdx];
            doc->isDirty = true;
            inputsChanged = true;
        }

        // 名称编辑
        ImGui::SameLine();
        static char nameBuf[64] = {};
        snprintf(nameBuf, sizeof(nameBuf), "%s", param.name.c_str());
        ImGui::SetNextItemWidth(paneWidth - 130.0f);
        if (ImGui::InputText("##name", nameBuf, sizeof(nameBuf), ImGuiInputTextFlags_EnterReturnsTrue))
        {
            PushUndoState();
            param.name = nameBuf;
            doc->isDirty = true;
            inputsChanged = true;
        }

        // 删除按钮
        ImGui::SameLine();
        if (ImGui::SmallButton(ICON_FA_TRASH "##del"))
        {
            PushUndoState();
            func.inputs.erase(func.inputs.begin() + i);
            doc->isDirty = true;
            inputsChanged = true;
            ImGui::PopID();
            --i;
            continue;
        }

        ImGui::PopID();
    }

    // 添加输入参数按钮
    if (ImGui::SmallButton(ICON_FA_PLUS " Add Input"))
    {
        PushUndoState();
        RTVariableDefinition newParam;
        newParam.name = "NewInput";
        newParam.dataType = RTPinDataType::Float;
        func.inputs.push_back(std::move(newParam));
        doc->isDirty = true;
        inputsChanged = true;
    }
    ImGui::Unindent(4.0f);

    ImGui::Spacing();

    // ── Outputs（函数输出参数）───────────────────────────────────────────
    {
        auto* drawList = ImGui::GetWindowDrawList();
        ImVec2 cursorPos = ImGui::GetCursorScreenPos();
        float sectionH = ImGui::GetTextLineHeight() + 4.0f;
        drawList->AddRectFilled(
            cursorPos,
            ImVec2(cursorPos.x + paneWidth, cursorPos.y + sectionH),
            IM_COL32(30, 30, 38, 230), 0.0f);
        drawList->AddLine(
            ImVec2(cursorPos.x, cursorPos.y + sectionH - 1.0f),
            ImVec2(cursorPos.x + paneWidth, cursorPos.y + sectionH - 1.0f),
            IM_COL32(0, 122, 204, 100));
        drawList->AddText(
            ImVec2(cursorPos.x + 8.0f, cursorPos.y + 2.0f),
            IM_COL32(200, 200, 210, 230), ICON_FA_ARROW_LEFT " Outputs");
        ImGui::Dummy(ImVec2(paneWidth, sectionH));
    }

    ImGui::Indent(4.0f);
    bool outputsChanged = false;
    for (int i = 0; i < (int)func.outputs.size(); ++i)
    {
        auto& param = func.outputs[i];
        ImGui::PushID(("fo" + std::to_string(i)).c_str());

        // 类型下拉
        int typeIdx = dataTypeToIndex(param.dataType);
        ImGui::SetNextItemWidth(70.0f);
        if (ImGui::Combo("##type", &typeIdx, typeNames, typeCount))
        {
            PushUndoState();
            param.dataType = typeValues[typeIdx];
            doc->isDirty = true;
            outputsChanged = true;
        }

        // 名称编辑
        ImGui::SameLine();
        static char nameBuf[64] = {};
        snprintf(nameBuf, sizeof(nameBuf), "%s", param.name.c_str());
        ImGui::SetNextItemWidth(paneWidth - 130.0f);
        if (ImGui::InputText("##name", nameBuf, sizeof(nameBuf), ImGuiInputTextFlags_EnterReturnsTrue))
        {
            PushUndoState();
            param.name = nameBuf;
            doc->isDirty = true;
            outputsChanged = true;
        }

        // 删除按钮
        ImGui::SameLine();
        if (ImGui::SmallButton(ICON_FA_TRASH "##del"))
        {
            PushUndoState();
            func.outputs.erase(func.outputs.begin() + i);
            doc->isDirty = true;
            outputsChanged = true;
            ImGui::PopID();
            --i;
            continue;
        }

        ImGui::PopID();
    }

    // 添加输出参数按钮
    if (ImGui::SmallButton(ICON_FA_PLUS " Add Output"))
    {
        PushUndoState();
        RTVariableDefinition newParam;
        newParam.name = "NewOutput";
        newParam.dataType = RTPinDataType::Float;
        func.outputs.push_back(std::move(newParam));
        doc->isDirty = true;
        outputsChanged = true;
    }
    ImGui::Unindent(4.0f);

    ImGui::Unindent(4.0f);

    // 如果参数发生变化，同步到画布上的 Function.Entry/Return 节点
    if (inputsChanged || outputsChanged)
    {
        SyncFunctionPinsToNodes(func);
    }

    ImGui::Spacing();
}

// ============================================================================
// 同步函数参数到画布上的 Function.Entry / Function.Return 节点引脚
// ============================================================================

void BlueprintEditor::SyncFunctionPinsToNodes(const RTFunctionDefinition& func)
{
    auto* doc = ActiveDoc();
    if (!doc) return;

    // Function.Entry 节点：
    //   Outputs = [Flow(exec)] + [函数输入参数作为数据输出引脚]
    //   （调用者的输入 → 进入函数体 → 从 Entry 输出端流出）
    for (auto& node : doc->nodes)
    {
        if (node.DefinitionId == "Function.Entry" && node.Name == func.name)
        {
            // 保留第一个 Flow 输出引脚，重建后面的数据引脚
            std::vector<Pin> newOutputs;
            if (!node.Outputs.empty() && node.Outputs[0].Type == PinType::Flow)
                newOutputs.push_back(node.Outputs[0]);
            else
            {
                newOutputs.emplace_back(GetNextId(), "", PinType::Flow);
                newOutputs.back().Kind = PinKind::Output;
                newOutputs.back().Node = &node;
            }

            // 为每个函数输入参数创建一个输出引脚
            for (const auto& param : func.inputs)
            {
                PinType pt = MapRTPinDataType(param.dataType, false);
                // 尝试复用同名同类型的现有引脚（保持连线）
                bool reused = false;
                for (size_t j = 1; j < node.Outputs.size(); ++j)
                {
                    if (node.Outputs[j].Name == param.name && node.Outputs[j].Type == pt)
                    {
                        newOutputs.push_back(node.Outputs[j]);
                        reused = true;
                        break;
                    }
                }
                if (!reused)
                {
                    newOutputs.emplace_back(GetNextId(), param.name.c_str(), pt);
                    newOutputs.back().Kind = PinKind::Output;
                    newOutputs.back().Node = &node;
                }
            }

            // 删除旧引脚不再存在的链接
            for (size_t j = 1; j < node.Outputs.size(); ++j)
            {
                bool stillExists = false;
                for (const auto& np : newOutputs)
                {
                    if (np.ID == node.Outputs[j].ID) { stillExists = true; break; }
                }
                if (!stillExists)
                {
                    auto pinId = node.Outputs[j].ID;
                    doc->links.erase(std::remove_if(doc->links.begin(), doc->links.end(),
                        [pinId](const Link& l) { return l.StartPinID == pinId || l.EndPinID == pinId; }),
                        doc->links.end());
                }
            }

            node.Outputs = std::move(newOutputs);
            // 确保 Node 指针正确
            for (auto& p : node.Outputs) p.Node = &node;
            BuildNode(&node);
        }

        // Function.Return 节点：
        //   Inputs = [Flow(exec)] + [函数输出参数作为数据输入引脚]
        if (node.DefinitionId == "Function.Return" && node.Name == func.name)
        {
            std::vector<Pin> newInputs;
            if (!node.Inputs.empty() && node.Inputs[0].Type == PinType::Flow)
                newInputs.push_back(node.Inputs[0]);
            else
            {
                newInputs.emplace_back(GetNextId(), "", PinType::Flow);
                newInputs.back().Kind = PinKind::Input;
                newInputs.back().Node = &node;
            }

            for (const auto& param : func.outputs)
            {
                PinType pt = MapRTPinDataType(param.dataType, false);
                bool reused = false;
                for (size_t j = 1; j < node.Inputs.size(); ++j)
                {
                    if (node.Inputs[j].Name == param.name && node.Inputs[j].Type == pt)
                    {
                        newInputs.push_back(node.Inputs[j]);
                        reused = true;
                        break;
                    }
                }
                if (!reused)
                {
                    newInputs.emplace_back(GetNextId(), param.name.c_str(), pt);
                    newInputs.back().Kind = PinKind::Input;
                    newInputs.back().Node = &node;
                }
            }

            // 删除旧引脚不再存在的链接
            for (size_t j = 1; j < node.Inputs.size(); ++j)
            {
                bool stillExists = false;
                for (const auto& np : newInputs)
                {
                    if (np.ID == node.Inputs[j].ID) { stillExists = true; break; }
                }
                if (!stillExists)
                {
                    auto pinId = node.Inputs[j].ID;
                    doc->links.erase(std::remove_if(doc->links.begin(), doc->links.end(),
                        [pinId](const Link& l) { return l.StartPinID == pinId || l.EndPinID == pinId; }),
                        doc->links.end());
                }
            }

            node.Inputs = std::move(newInputs);
            for (auto& p : node.Inputs) p.Node = &node;
            BuildNode(&node);
        }
    }

    doc->invalidateEditorIndices();
}

// ============================================================================
// 底部执行输出面板（嵌入式）
// ============================================================================

void BlueprintEditor::DrawExecutionPanel()
{
    if (!ActiveDoc()) return;   // 无文档时跳过

    float paneWidth = ImGui::GetContentRegionAvail().x;

    // 面板标题
    {
        auto* drawList = ImGui::GetWindowDrawList();
        ImVec2 cursorPos = ImGui::GetCursorScreenPos();
        float headerH = ImGui::GetTextLineHeight() + 8.0f;

        // VS 2022 扁平标题栏
        drawList->AddRectFilled(
            cursorPos,
            ImVec2(cursorPos.x + paneWidth, cursorPos.y + headerH),
            IM_COL32(30, 30, 38, 230), 0.0f);
        // 底部 1px VS blue accent 分隔线
        drawList->AddLine(
            ImVec2(cursorPos.x, cursorPos.y + headerH - 1.0f),
            ImVec2(cursorPos.x + paneWidth, cursorPos.y + headerH - 1.0f),
            IM_COL32(0, 122, 204, 100));
        drawList->AddText(
            ImVec2(cursorPos.x + 10.0f, cursorPos.y + 4.0f),
            IM_COL32(160, 195, 240, 240), ICON_FA_TERMINAL);
        drawList->AddText(
            ImVec2(cursorPos.x + 28.0f, cursorPos.y + 4.0f),
            IM_COL32(175, 210, 250, 245), "Output");
        ImGui::Dummy(ImVec2(paneWidth, headerH));
    }

    ImGui::Spacing();

    // 日志工具栏（Execute 按钮已移至顶部调试工具条）
    ImGui::BeginHorizontal("ExecButtons", ImVec2(paneWidth, 0));

    if (ImGui::Button(ICON_FA_COPY " Copy Log", ImVec2(100, 0)))
    {
        if (!ActiveDoc()->executionLog.empty())
        {
            std::string allText;
            for (const auto& line : ActiveDoc()->executionLog)
            {
                allText += line;
                allText += '\n';
            }
            ImGui::SetClipboardText(allText.c_str());
        }
    }
    ImGui::Spring(0.0f);
    if (ImGui::Button(ICON_FA_ERASER " Clear", ImVec2(80, 0)))
    {
        ActiveDoc()->executionLog.clear();
        ActiveDoc()->executionLogText.clear();
        ActiveDoc()->executionLogDirty = false;
        ActiveDoc()->lastExecutionStatus.clear();
    }
    ImGui::Spring();
    ImGui::EndHorizontal();

    // 状态信息
    if (!ActiveDoc()->lastExecutionStatus.empty())
    {
        bool isOk = ActiveDoc()->lastExecutionStatus.find("OK") == 0;
        ImGui::TextColored(isOk ? ImVec4(0.4f, 1.0f, 0.4f, 1.0f) : ImVec4(1.0f, 0.4f, 0.4f, 1.0f),
            "Status: %s", ActiveDoc()->lastExecutionStatus.c_str());
    }

    // 彩色日志输出区域
    float logHeight = ImGui::GetContentRegionAvail().y;
    if (logHeight < 60.0f) logHeight = 60.0f;

    ImGui::BeginChild("##ExecutionLog", ImVec2(paneWidth, logHeight), true,
        ImGuiWindowFlags_HorizontalScrollbar);

    for (const auto& line : ActiveDoc()->executionLog)
    {
        DrawColoredLogLine(line);
    }

    // 自动滚动到底部（新日志时）
    if (ActiveDoc()->executionLogDirty)
    {
        ImGui::SetScrollHereY(1.0f);
        ActiveDoc()->executionLogDirty = false;
    }

    ImGui::EndChild();
}

// ============================================================================
// 计时器监控面板（浮动窗口）
// ============================================================================

void BlueprintEditor::DrawTimerPanel()
{
    ImGui::SetNextWindowSize(ImVec2(520, 340), ImGuiCond_FirstUseEver);
    if (!ImGui::Begin(ICON_FA_STOPWATCH " Timer Monitor", &m_ShowTimerWindow))
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
    if (ImGui::Button(ICON_FA_ERASER " Clear All"))
        timerMgr.ClearAllTimers();
    ImGui::SameLine();
    if (ImGui::Button(ICON_FA_PAUSE " Pause All"))
        timerMgr.PauseAll();
    ImGui::SameLine();
    if (ImGui::Button(ICON_FA_PLAY " Resume All"))
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
            ActiveDoc()->executionLog.push_back("[Timer:" + timerName + "] fired!");
            ActiveDoc()->executionLogDirty = true;
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
                    if (ImGui::SmallButton(ICON_FA_PLAY " Resume"))
                        timerMgr.ResumeTimer(t.handle);
                }
                else
                {
                    if (ImGui::SmallButton(ICON_FA_PAUSE " Pause"))
                        timerMgr.PauseTimer(t.handle);
                }
                ImGui::SameLine();
                if (ImGui::SmallButton(ICON_FA_XMARK))
                    timerMgr.ClearTimer(t.handle);
                ImGui::PopID();
            }

            ImGui::EndTable();
        }
    }

    ImGui::End();
}

// ============================================================================
// 节点库面板（Library Tab）— 折叠分类 + 拖拽到画布
// ============================================================================

// 辅助：从分类名获取小彩色方块颜色
static ImVec4 GetCategoryDotColor(const std::string& cat)
{
    if (cat.find("Flow")       == 0) return {0.20f, 0.39f, 0.86f, 1.f};
    if (cat.find("Math")       == 0) return {0.24f, 0.71f, 0.31f, 1.f};
    if (cat.find("String")     == 0) return {0.71f, 0.31f, 0.78f, 1.f};
    if (cat.find("Debug")      == 0) return {0.78f, 0.24f, 0.24f, 1.f};
    if (cat.find("Action")     == 0 ||
        cat.find("Event")      == 0) return {0.86f, 0.39f, 0.16f, 1.f};
    if (cat.find("Conversion") == 0) return {0.31f, 0.63f, 0.86f, 1.f};
    if (cat.find("Array")      == 0) return {0.78f, 0.59f, 0.12f, 1.f};
    if (cat.find("Misc/Map")   == 0 ||
        cat.find("Map")        == 0) return {0.16f, 0.71f, 0.78f, 1.f};
    if (cat.find("Custom")     == 0) return {0.24f, 0.71f, 0.51f, 1.f};
    return {0.39f, 0.39f, 0.47f, 1.f};
}

void BlueprintEditor::DrawNodeLibraryPanel()
{
    float paneWidth = ImGui::GetContentRegionAvail().x;

    // 搜索框
    static char libSearchBuf[128] = "";
    ImGui::SetNextItemWidth(paneWidth);
    ImGui::InputTextWithHint("##LibSearch", ICON_FA_MAGNIFYING_GLASS " Search...", libSearchBuf, sizeof(libSearchBuf));

    std::string searchStr(libSearchBuf);
    std::string searchLower = searchStr;
    for (auto& c : searchLower)
        c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));

    ImGui::Spacing();

    const auto& allDefs = m_NodeRegistry.getAllNodeDefinitions();

    // 缓存分类 → 节点列表映射（仅在 registry 大小或搜索词变化时重建）
    static size_t s_libCachedDefCount = 0;
    static std::string s_libCachedSearch;
    static std::map<std::string, std::vector<const RTNodeDef*>> s_libCatMap;

    bool needRebuild = (allDefs.size() != s_libCachedDefCount) || (searchLower != s_libCachedSearch);
    if (needRebuild)
    {
        s_libCachedDefCount = allDefs.size();
        s_libCachedSearch = searchLower;
        s_libCatMap.clear();

        for (const auto* d : allDefs)
        {
            if (d->isAbstract) continue;
            // 搜索过滤
            if (!searchLower.empty())
            {
                std::string nameLower = d->name;
                for (auto& c : nameLower) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
                std::string idLower = d->id;
                for (auto& c : idLower) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
                if (nameLower.find(searchLower) == std::string::npos &&
                    idLower.find(searchLower) == std::string::npos &&
                    d->category.find(searchStr) == std::string::npos)
                    continue;
            }
            s_libCatMap[d->category.empty() ? "Misc" : d->category].push_back(d);
        }
    }

    const auto& catMap = s_libCatMap;

    // 默认展开的分类（Flow/Math/String 默认展开，其余折叠）
    static std::unordered_map<std::string, bool> catOpenState;

    for (const auto& [cat, nodes] : catMap)
    {
        // 首次出现时设定默认展开状态
        if (catOpenState.find(cat) == catOpenState.end())
        {
            catOpenState[cat] = (cat == "Flow" || cat == "Math" || cat == "String");
        }

        // 分类头部行：彩色方块 + CollapsingHeader
        ImVec4 dotCol = GetCategoryDotColor(cat);
        ImGui::PushStyleColor(ImGuiCol_Header,        ImVec4(dotCol.x*0.35f, dotCol.y*0.35f, dotCol.z*0.35f, 0.80f));
        ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(dotCol.x*0.50f, dotCol.y*0.50f, dotCol.z*0.50f, 0.90f));
        ImGui::PushStyleColor(ImGuiCol_HeaderActive,  ImVec4(dotCol.x*0.65f, dotCol.y*0.65f, dotCol.z*0.65f, 1.00f));

        ImGui::SetNextItemOpen(catOpenState[cat], ImGuiCond_Once);
        bool open = ImGui::CollapsingHeader(
            (cat + " (" + std::to_string(nodes.size()) + ")").c_str(),
            ImGuiTreeNodeFlags_None);
        catOpenState[cat] = open;

        ImGui::PopStyleColor(3);

        if (!open) continue;

        ImGui::Indent(8.0f);
        for (const auto* d : nodes)
        {
            ImGui::PushID(d->id.c_str());

            // 小彩色方块
            ImVec2 squareMin = ImGui::GetCursorScreenPos();
            squareMin.y += (ImGui::GetTextLineHeight() - 8.0f) * 0.5f;
            ImGui::GetWindowDrawList()->AddRectFilled(
                squareMin,
                ImVec2(squareMin.x + 8.0f, squareMin.y + 8.0f),
                ImGui::ColorConvertFloat4ToU32(dotCol), 2.0f);
            ImGui::Dummy(ImVec2(10.0f, ImGui::GetTextLineHeight()));
            ImGui::SameLine(0, 2.0f);

            // 高亮搜索匹配文字
            if (!searchLower.empty())
            {
                std::string nameLower = d->name;
                for (auto& c : nameLower)
                    c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
                size_t pos = nameLower.find(searchLower);
                if (pos != std::string::npos)
                {
                    // 拆成三段：前、匹配、后
                    ImGui::TextUnformatted(d->name.substr(0, pos).c_str());
                    ImGui::SameLine(0, 0);
                    ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.2f, 1.0f),
                        "%s", d->name.substr(pos, searchLower.size()).c_str());
                    ImGui::SameLine(0, 0);
                    ImGui::TextUnformatted(d->name.substr(pos + searchLower.size()).c_str());
                }
                else
                {
                    ImGui::TextUnformatted(d->name.c_str());
                }
            }
            else
            {
                ImGui::TextUnformatted(d->name.c_str());
            }

            // Tooltip
            if (ImGui::IsItemHovered())
            {
                ImGui::SetTooltip("ID: %s\nCategory: %s%s",
                    d->id.c_str(), d->category.c_str(),
                    d->description.empty() ? "" : ("\n" + d->description).c_str());
            }

            // DragDrop 拖拽源：拖拽到画布生成节点
            if (ImGui::IsItemActive() || ImGui::IsItemHovered())
            {
                if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID))
                {
                    // payload = defId 字符串
                    ImGui::SetDragDropPayload("BP_NODE_DEF",
                        d->id.c_str(),
                        d->id.size() + 1);
                    // 拖拽预览
                    ImGui::TextColored(dotCol, "%s", d->name.c_str());
                    ImGui::TextDisabled("Drop on canvas to create");
                    ImGui::EndDragDropSource();
                }
            }

            ImGui::PopID();
        }
        ImGui::Unindent(8.0f);
        ImGui::Spacing();
    }
}
