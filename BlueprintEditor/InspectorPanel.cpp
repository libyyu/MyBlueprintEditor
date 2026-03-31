// InspectorPanel.cpp -- 检视器面板（节点列表 / 变量 / 函数 / Details）
// 从 EditorUI.cpp 拆分而来
#include "BlueprintEditor.h"
#include "ThemeManager.h"

// ============================================================================
// DrawNodeListPanel — 右侧面板主入口（Tab Bar：Nodes / Variables / Functions / Events / Details）
// ============================================================================

void BlueprintEditor::DrawNodeListPanel(int panelIdx)
{
    // 无文档时不渲染（避免 ActiveDoc() 为 nullptr 崩溃）
    if (!ActiveDoc())
    {
        ImGui::TextDisabled("No blueprint open.");
        return;
    }

    auto& io = ImGui::GetIO();
    float paneWidth = ImGui::GetContentRegionAvail().x;

    ImGui::Spacing();

    // 工具栏按钮（仅 Nodes 面板显示）
    static bool showStyleEditor = false;
    if (panelIdx == 0)
    {
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
        ImGui::Spring(0.0f);
        {
            bool libActive = m_ShowLibraryWindow;
            if (libActive)
            {
                ImGui::PushStyleColor(ImGuiCol_Button,        IM_COL32(0, 122, 204, 80));
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, IM_COL32(0, 122, 204, 120));
            }
            if (ImGui::Button(ICON_FA_LAYER_GROUP " Library"))
                m_ShowLibraryWindow = !m_ShowLibraryWindow;
            if (libActive) ImGui::PopStyleColor(2);
            if (ImGui::IsItemHovered()) ImGui::SetTooltip("Toggle Node Library panel");
        }
        ImGui::Spring();
        if (ImGui::Button(ICON_FA_PALETTE " Style"))
            showStyleEditor = true;
        ImGui::EndHorizontal();
        ImGui::Checkbox(ICON_FA_TABLE_CELLS " Ordinals", &m_ShowOrdinals);
    }

    if (showStyleEditor)
        ShowStyleEditor(&showStyleEditor);

    // ── 根据外层侧边栏传入的 panelIdx 直接渲染对应内容 ─────────────────────
    // 0=Nodes, 1=Variables, 2=Functions, 3=Events, 4=Details
    bool isLibrary = ActiveDoc() && ActiveDoc()->blueprintClass == RTBlueprintClass::FunctionLibrary;

    switch (panelIdx)
    {
    case 0:  // ── Nodes ───────────────────────────────────────────────────────
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

            // 节点列表 — Header bar（显示节点数 + 过滤状态）
            {
                int totalNodes   = static_cast<int>(ActiveDoc()->nodes.size());
                int selCount     = static_cast<int>(selectedNodes.size());
                auto* drawList   = ImGui::GetWindowDrawList();
                ImVec2 cursorPos = ImGui::GetCursorScreenPos();
                float sectionH   = ImGui::GetTextLineHeight() + 4.0f;
                drawList->AddRectFilled(
                    cursorPos,
                    ImVec2(cursorPos.x + paneWidth, cursorPos.y + sectionH),
                    IM_COL32(30, 30, 38, 230), 0.0f);
                drawList->AddLine(
                    ImVec2(cursorPos.x, cursorPos.y + sectionH - 1.0f),
                    ImVec2(cursorPos.x + paneWidth, cursorPos.y + sectionH - 1.0f),
                    IM_COL32(0, 122, 204, 100));
                // 左：图标 + 标题
                drawList->AddText(
                    ImVec2(cursorPos.x + 8.0f, cursorPos.y + 2.0f),
                    IM_COL32(200, 200, 210, 230),
                    nodeFilter.empty() ? ICON_FA_CUBES " Nodes" : ICON_FA_CUBES " Nodes (filtered)");
                // 右：节点数 badge（选中/总数）
                char badge[32];
                if (selCount > 0)
                    std::snprintf(badge, sizeof(badge), "%d / %d", selCount, totalNodes);
                else
                    std::snprintf(badge, sizeof(badge), "%d", totalNodes);
                ImVec2 badgeSize = ImGui::CalcTextSize(badge);
                drawList->AddText(
                    ImVec2(cursorPos.x + paneWidth - badgeSize.x - 6.0f, cursorPos.y + 2.0f),
                    selCount > 0 ? IM_COL32(100, 200, 255, 220) : IM_COL32(130, 140, 155, 160),
                    badge);
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
                bool hasBreakpoint = ActiveDoc()->breakpoints.count(static_cast<uint64_t>(node.ID.Get())) > 0;

                // ── 每行行高 ────────────────────────────────────────────────
                float rowH     = ImGui::GetTextLineHeight() + ImGui::GetStyle().ItemSpacing.y;
                float rowY     = ImGui::GetCursorScreenPos().y;
                float rowX     = ImGui::GetCursorScreenPos().x;
                // rowRight 基于窗口内容区域右边界（不受 Indent 影响）
                float rowRight = ImGui::GetWindowPos().x + ImGui::GetWindowContentRegionMax().x
                                 - ImGui::GetStyle().ScrollbarSize - 2.0f;

                // ── 选中 / hover 背景 ───────────────────────────────────────
                ImVec2 rowMin(rowX - ImGui::GetStyle().IndentSpacing, rowY);
                ImVec2 rowMax(rowRight, rowY + ImGui::GetTextLineHeight() + 2.0f);
                auto* dl = ImGui::GetWindowDrawList();

                if (isSelected)
                    dl->AddRectFilled(rowMin, rowMax, IM_COL32(0, 122, 204, 60), 3.0f);

                // ── 左侧色条（按 DefinitionId 前缀分类着色）────────────────
                ImU32 accentCol = IM_COL32(120, 120, 120, 200);
                const std::string& defId = node.DefinitionId;
                if (defId.rfind("FuncLib.", 0) == 0)           accentCol = IM_COL32(180, 100, 255, 220);
                else if (defId == "ForLoop")                    accentCol = IM_COL32( 80, 160, 255, 220);
                else if (defId == "Branch")                     accentCol = IM_COL32(255, 180,  60, 220);
                else if (defId.rfind("Print", 0) == 0)         accentCol = IM_COL32( 80, 210,  80, 220);
                else if (defId == "Delay")                      accentCol = IM_COL32(255, 120, 120, 220);
                else if (defId == "ExecuteBlueprint")           accentCol = IM_COL32( 60, 200, 200, 220);
                else if (defId.rfind("Math", 0) == 0 ||
                         defId == "Add" || defId == "Sub" ||
                         defId == "Mul" || defId == "Div")      accentCol = IM_COL32(255, 210,  80, 220);
                else if (defId.rfind("Function.", 0) == 0)
                {
                    if (defId == "Function.Entry")        accentCol = IM_COL32( 60, 200,  80, 220);
                    else if (defId == "Function.Return")  accentCol = IM_COL32(255, 140,  40, 220);
                    else                                  accentCol = IM_COL32(220,  80, 180, 220);
                }

                dl->AddRectFilled(
                    ImVec2(rowMin.x, rowMin.y + 2.0f),
                    ImVec2(rowMin.x + 3.0f, rowMax.y - 2.0f),
                    accentCol, 1.5f);

                // ── 断点圆点 ────────────────────────────────────────────────
                if (hasBreakpoint)
                {
                    float cx = rowMin.x + 10.0f;
                    float cy = rowMin.y + rowH * 0.5f - 1.0f;
                    dl->AddCircleFilled(ImVec2(cx, cy), 5.0f, IM_COL32(220, 50, 50, 255));
                    dl->AddCircle(ImVec2(cx, cy), 5.0f, IM_COL32(255, 120, 120, 200), 0, 1.5f);
                }

                // ── 节点图标（按分类）──────────────────────────────────────
                const char* nodeIcon = ICON_FA_CIRCLE_NODES;
                if      (defId.rfind("FuncLib.", 0) == 0)  nodeIcon = ICON_FA_PUZZLE_PIECE;
                else if (defId == "ForLoop")                nodeIcon = ICON_FA_ROTATE;
                else if (defId == "Branch")                 nodeIcon = ICON_FA_CODE_BRANCH;
                else if (defId.rfind("Print", 0) == 0)     nodeIcon = ICON_FA_TERMINAL;
                else if (defId == "Delay")                  nodeIcon = ICON_FA_CLOCK;
                else if (defId == "ExecuteBlueprint")       nodeIcon = ICON_FA_DIAGRAM_PROJECT;
                else if (defId == "Function.Entry")        nodeIcon = ICON_FA_ARROW_RIGHT;
                else if (defId == "Function.Return")       nodeIcon = ICON_FA_ARROW_LEFT;
                else if (defId.rfind("Function.", 0) == 0) nodeIcon = ICON_FA_BOLT;

                // ── Selectable（透明背景，自定义绘制）────────────────────
                ImGui::PushStyleColor(ImGuiCol_Header,        IM_COL32(0, 122, 204, 40));
                ImGui::PushStyleColor(ImGuiCol_HeaderHovered, IM_COL32(0, 122, 204, 30));
                ImGui::PushStyleColor(ImGuiCol_HeaderActive,  IM_COL32(0, 122, 204, 70));
#if IMGUI_VERSION_NUM >= 18967
                ImGui::SetNextItemAllowOverlap();
#endif
                std::string selectableLabel = std::string("##node_") +
                    std::to_string(reinterpret_cast<uintptr_t>(node.ID.AsPointer()));
                if (ImGui::Selectable(selectableLabel.c_str(), isSelected,
                    ImGuiSelectableFlags_SpanAllColumns, ImVec2(0, ImGui::GetTextLineHeight() + 2.0f)))
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
                ImGui::PopStyleColor(3);

                if (ImGui::IsItemHovered())
                {
                    // hover 高亮条
                    dl->AddRectFilled(rowMin, rowMax, IM_COL32(255, 255, 255, 10), 3.0f);
                    if (!node.State.empty())
                        ImGui::SetTooltip("State: %s\nDef: %s", node.State.c_str(), defId.c_str());
                    else
                        ImGui::SetTooltip("Def: %s", defId.c_str());
                }

                // ── 图标 + 节点名（覆盖在 Selectable 上方）────────────────
                ImGui::SameLine(0, 0);
                ImGui::SetCursorScreenPos(ImVec2(rowMin.x + 16.0f, rowY));
                ImGui::PushStyleColor(ImGuiCol_Text, accentCol);
                ImGui::TextUnformatted(nodeIcon);
                ImGui::PopStyleColor();
                ImGui::SameLine(0, 5.0f);
                ImGui::TextUnformatted(node.Name.c_str());

                // ── 右侧节点 ID（淡色）─────────────────────────────────────
                {
                    std::string idStr = "#" + std::to_string(reinterpret_cast<uintptr_t>(node.ID.AsPointer()) & 0xFFFF);
                    ImVec2 idSize = ImGui::CalcTextSize(idStr.c_str());
                    dl->AddText(
                        ImVec2(rowRight - idSize.x - 4.0f, rowY + 1.0f),
                        IM_COL32(120, 130, 145, 140),
                        idStr.c_str());
                }

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

        break;  // case 0: Nodes
    }

    case 1:  // ── Variables ───────────────────────────────────────────────────
        if (!isLibrary)
        {
            DrawVariablePanel();
        }
        else
        {
            ImGui::TextDisabled("Variables are not available for Function Libraries.");
        }
        break;




    case 2:  // ── Functions ───────────────────────────────────────────────────
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

                    // 先渲染右侧小按钮，再渲染 Selectable（避免 Selectable 覆盖按钮）
                    float btnSize = ImGui::GetFrameHeight() * 0.75f;
                    float availW  = ImGui::GetContentRegionAvail().x;
                    float selectW = availW - btnSize * 2 - ImGui::GetStyle().ItemSpacing.x * 2;
                    if (selectW < 20.0f) selectW = 20.0f;

                    if (renamingIdx == i)
                    {
                        ImGui::SetNextItemWidth(selectW);
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
#if IMGUI_VERSION_NUM >= 18967
                        ImGui::SetNextItemAllowOverlap();
#endif
                        if (ImGui::Selectable(func.name.c_str(), isSelected,
                                              ImGuiSelectableFlags_AllowDoubleClick | ImGuiSelectableFlags_AllowOverlap,
                                              ImVec2(selectW, 0)))
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
                                deleteIdx = i;
                            ImGui::EndPopup();
                        }

                        // Delete 键删除
                        if (ImGui::IsItemFocused() && ImGui::IsKeyPressed(ImGuiKey_Delete))
                            deleteIdx = i;
                    }

                    // 删除按钮
                    ImGui::SameLine(0, ImGui::GetStyle().ItemSpacing.x);
                    ImGui::PushStyleColor(ImGuiCol_Button,        ImVec4(0.55f, 0.15f, 0.15f, 0.7f));
                    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.80f, 0.20f, 0.20f, 1.0f));
                    if (ImGui::Button("X##delfunc", ImVec2(btnSize, btnSize)))
                        deleteIdx = i;
                    ImGui::PopStyleColor(2);
                    if (ImGui::IsItemHovered())
                        ImGui::SetTooltip("Delete function");

                    // 跳转按钮
                    ImGui::SameLine(0, 2.0f);
                    if (ImGui::Button("->##gotofunc", ImVec2(btnSize, btnSize)))
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
                        ImGui::SetTooltip("Jump to Entry node");

                    ImGui::PopID();
                }

                // 延迟执行删除（避免在迭代中删除）
                if (deleteIdx >= 0 && deleteIdx < (int)doc->functions.size())
                {
                    PushUndoState();
                    const std::string funcName = doc->functions[deleteIdx].name;
                    for (auto nit = doc->nodes.begin(); nit != doc->nodes.end(); )
                    {
                        if ((nit->DefinitionId == "Function.Entry" || nit->DefinitionId == "Function.Return")
                            && nit->Name == funcName)
                        {
                            // 先断开所有连线
                            doc->links.erase(std::remove_if(doc->links.begin(), doc->links.end(),
                                [&](const Link& lnk) {
                                    for (auto& p : nit->Inputs)
                                        if (lnk.StartPinID == p.ID || lnk.EndPinID == p.ID) return true;
                                    for (auto& p : nit->Outputs)
                                        if (lnk.StartPinID == p.ID || lnk.EndPinID == p.ID) return true;
                                    return false;
                                }), doc->links.end());
                            // 通知节点编辑器删除该节点（同步内部状态）
                            ed::DeleteNode(nit->ID);
                            nit = doc->nodes.erase(nit);
                        }
                        else
                            ++nit;
                    }
                    doc->functions.erase(doc->functions.begin() + deleteIdx);
                    if (doc->selectedFuncIdx >= (int)doc->functions.size())
                        doc->selectedFuncIdx = (int)doc->functions.size() - 1;
                    doc->isDirty = true;
                    // 强制重建创建节点菜单缓存（以防 Function 节点注册在 registry 中）
                    m_CachedDefCount = 0;
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
        break;  // case 2: Functions
    }

    case 3:  // ── Events ──────────────────────────────────────────────────────
        if (!isLibrary)
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
        }
        else
        {
            ImGui::TextDisabled("Events are not available for Function Libraries.");
        }
        break;  // case 3: Events

    case 4:  // ── Details ─────────────────────────────────────────────────────
        DrawDetailsPanel();
        break;

    default:
        break;
    }  // end switch(panelIdx)
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

    constexpr size_t kVarNameBufSize = sizeof(BlueprintDocument::varNewName);  // 64

    if (ImGui::Button(ICON_FA_PLUS " Add Variable"))
    {
        memset(newVarName, 0, kVarNameBufSize);
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
        ImGui::InputTextWithHint("##VarName", "Variable name...", newVarName, kVarNameBufSize);

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

    // 动态计算标签列宽（最宽标签 + 12px 间距）
    float labelCol = std::max({
        ImGui::CalcTextSize("Name:").x,
        ImGui::CalcTextSize("ID:").x,
        ImGui::CalcTextSize("Definition:").x,
        ImGui::CalcTextSize("Category:").x,
        ImGui::CalcTextSize("Type:").x,
        ImGui::CalcTextSize("Pure:").x,
    }) + 12.0f;
    float indentedX = ImGui::GetCursorPosX();  // Indent 后的基准 X
    float valueX    = indentedX + labelCol;    // 值列 X（窗口内坐标）

    // 辅助 lambda：绘制标签-值对
    auto drawRow = [&](const char* label, auto drawValue) {
        ImGui::TextColored(ImVec4(0.55f, 0.65f, 0.80f, 1.0f), "%s", label);
        ImGui::SameLine();
        ImGui::SetCursorPosX(valueX);
        drawValue();
    };

    // 名称
    drawRow("Name:", [&]{ ImGui::TextUnformatted(node->Name.c_str()); });

    // ID
    drawRow("ID:", [&]{
        ImGui::Text("%llu", static_cast<unsigned long long>(
            reinterpret_cast<uintptr_t>(node->ID.AsPointer())));
    });

    // 定义 ID
    drawRow("Definition:", [&]{ ImGui::TextUnformatted(node->DefinitionId.c_str()); });

    // 从注册表查找节点定义
    const RTNodeDef* def = m_NodeRegistry.getNodeDefinition(node->DefinitionId);

    // 类别
    if (def && !def->category.empty())
        drawRow("Category:", [&]{ ImGui::TextUnformatted(def->category.c_str()); });

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
        drawRow("Type:", [&]{ ImGui::TextUnformatted(typeStr); });
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
        drawRow("Pure:", [&]{
            ImGui::TextColored(
                def->isPure ? ImVec4(0.35f, 0.85f, 0.45f, 1.0f) : ImVec4(0.85f, 0.55f, 0.35f, 1.0f),
                "%s", def->isPure ? "Yes" : "No");
        });
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
        DrawSectionHeader(ICON_FA_ARROW_RIGHT " Inputs", paneWidth);

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
                    ImS64 ival = static_cast<ImS64>(pin.IntValue);
                    ImGui::SetNextItemWidth(editWidth);
                    if (ImGui::DragScalar("##val", ImGuiDataType_S64, &ival, 1.0f))
                    {
                        PushUndoState();
                        pin.IntValue = static_cast<int64_t>(ival);
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
        DrawSectionHeader(ICON_FA_ARROW_LEFT " Outputs", paneWidth);

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
        DrawSectionHeader(ICON_FA_GEAR " Properties", paneWidth);

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
    ImGui::Unindent(4.0f);  // 先恢复到全宽，section header 从窗口左边缘开始
    {
        auto* drawList = ImGui::GetWindowDrawList();
        ImVec2 cursorPos = ImGui::GetCursorScreenPos();
        float sectionH = ImGui::GetTextLineHeight() + 4.0f;
        float fullW = ImGui::GetContentRegionAvail().x;
        drawList->AddRectFilled(
            cursorPos,
            ImVec2(cursorPos.x + fullW, cursorPos.y + sectionH),
            IM_COL32(30, 30, 38, 230), 0.0f);
        drawList->AddLine(
            ImVec2(cursorPos.x, cursorPos.y + sectionH - 1.0f),
            ImVec2(cursorPos.x + fullW, cursorPos.y + sectionH - 1.0f),
            IM_COL32(0, 122, 204, 100));
        drawList->AddText(
            ImVec2(cursorPos.x + 8.0f, cursorPos.y + 2.0f),
            IM_COL32(200, 200, 210, 230), ICON_FA_ARROW_RIGHT " Inputs");
        ImGui::Dummy(ImVec2(fullW, sectionH));
    }

    ImGui::Indent(4.0f);
    bool inputsChanged = false;
    int inputDeleteIdx = -1;
    // 动态计算 InputText 宽度：paneWidth - Indent - ComboW - TrashBtnW - spacing
    float comboW  = 70.0f;
    float trashW  = ImGui::CalcTextSize(ICON_FA_TRASH).x + ImGui::GetStyle().FramePadding.x * 2.0f + 2.0f;
    float spacingX = ImGui::GetStyle().ItemSpacing.x;
    float nameW   = paneWidth - 4.0f - comboW - trashW - spacingX * 2.0f - 4.0f;
    if (nameW < 40.0f) nameW = 40.0f;
    for (int i = 0; i < (int)func.inputs.size(); ++i)
    {
        auto& param = func.inputs[i];
        ImGui::PushID(i);

        // 类型下拉
        int typeIdx = dataTypeToIndex(param.dataType);
        ImGui::SetNextItemWidth(comboW);
        if (ImGui::Combo("##type", &typeIdx, typeNames, typeCount))
        {
            PushUndoState();
            param.dataType = typeValues[typeIdx];
            doc->isDirty = true;
            inputsChanged = true;
        }

        // 名称编辑
        ImGui::SameLine();
        char nameBuf[64];
        snprintf(nameBuf, sizeof(nameBuf), "%s", param.name.c_str());
        ImGui::SetNextItemWidth(nameW);
        if (ImGui::InputText("##name", nameBuf, sizeof(nameBuf), ImGuiInputTextFlags_EnterReturnsTrue))
        {
            // 检查重名：若与其他参数同名则加序号
            std::string newName(nameBuf);
            if (newName.empty()) newName = "Input_" + std::to_string(i);
            // 检测重名（与 inputs 和 outputs 所有其他参数）
            auto isNameTaken = [&](const std::string& n, int skipIdx) -> bool {
                for (int k = 0; k < (int)func.inputs.size(); ++k)
                    if (k != skipIdx && func.inputs[k].name == n) return true;
                for (const auto& o : func.outputs)
                    if (o.name == n) return true;
                return false;
            };
            if (isNameTaken(newName, i))
            {
                int suffix = 2;
                std::string candidate;
                do { candidate = newName + "_" + std::to_string(suffix++); }
                while (isNameTaken(candidate, i));
                newName = candidate;
            }
            PushUndoState();
            param.name = newName;
            doc->isDirty = true;
            inputsChanged = true;
        }

        // 删除按钮
        ImGui::SameLine();
        if (ImGui::SmallButton(ICON_FA_TRASH "##del"))
            inputDeleteIdx = i;

        ImGui::PopID();
    }
    // 延迟删除（避免在迭代中修改 vector）
    if (inputDeleteIdx >= 0)
    {
        PushUndoState();
        func.inputs.erase(func.inputs.begin() + inputDeleteIdx);
        doc->isDirty = true;
        inputsChanged = true;
    }

    // 添加输入参数按钮（自动生成唯一名称）
    if (ImGui::SmallButton(ICON_FA_PLUS " Add Input"))
    {
        PushUndoState();
        RTVariableDefinition newParam;
        // 生成唯一名称
        std::string baseName = "NewInput";
        std::string candidate = baseName;
        int suffix = 1;
        auto isNameUsed = [&](const std::string& n) -> bool {
            for (const auto& p : func.inputs)  if (p.name == n) return true;
            for (const auto& p : func.outputs) if (p.name == n) return true;
            return false;
        };
        while (isNameUsed(candidate))
            candidate = baseName + "_" + std::to_string(suffix++);
        newParam.name = candidate;
        newParam.dataType = RTPinDataType::Float;
        func.inputs.push_back(std::move(newParam));
        doc->isDirty = true;
        inputsChanged = true;
    }
    ImGui::Unindent(4.0f);

    ImGui::Spacing();

    // ── Outputs（函数输出参数）───────────────────────────────────────────
    ImGui::Unindent(4.0f);  // 先恢复到全宽
    {
        auto* drawList = ImGui::GetWindowDrawList();
        ImVec2 cursorPos = ImGui::GetCursorScreenPos();
        float sectionH = ImGui::GetTextLineHeight() + 4.0f;
        float fullW = ImGui::GetContentRegionAvail().x;
        drawList->AddRectFilled(
            cursorPos,
            ImVec2(cursorPos.x + fullW, cursorPos.y + sectionH),
            IM_COL32(30, 30, 38, 230), 0.0f);
        drawList->AddLine(
            ImVec2(cursorPos.x, cursorPos.y + sectionH - 1.0f),
            ImVec2(cursorPos.x + fullW, cursorPos.y + sectionH - 1.0f),
            IM_COL32(0, 122, 204, 100));
        drawList->AddText(
            ImVec2(cursorPos.x + 8.0f, cursorPos.y + 2.0f),
            IM_COL32(200, 200, 210, 230), ICON_FA_ARROW_LEFT " Outputs");
        ImGui::Dummy(ImVec2(fullW, sectionH));
    }

    ImGui::Indent(4.0f);
    bool outputsChanged = false;
    int outputDeleteIdx = -1;
    for (int i = 0; i < (int)func.outputs.size(); ++i)
    {
        auto& param = func.outputs[i];
        ImGui::PushID(i + 1000);  // 偏移避免与 inputs PushID 冲突

        // 类型下拉
        int typeIdx = dataTypeToIndex(param.dataType);
        ImGui::SetNextItemWidth(comboW);
        if (ImGui::Combo("##type", &typeIdx, typeNames, typeCount))
        {
            PushUndoState();
            param.dataType = typeValues[typeIdx];
            doc->isDirty = true;
            outputsChanged = true;
        }

        // 名称编辑（独立 buffer）
        ImGui::SameLine();
        char nameBuf[64];
        snprintf(nameBuf, sizeof(nameBuf), "%s", param.name.c_str());
        ImGui::SetNextItemWidth(nameW);
        if (ImGui::InputText("##name", nameBuf, sizeof(nameBuf), ImGuiInputTextFlags_EnterReturnsTrue))
        {
            std::string newName(nameBuf);
            if (newName.empty()) newName = "Output_" + std::to_string(i);
            auto isNameTaken = [&](const std::string& n, int skipIdx) -> bool {
                for (int k = 0; k < (int)func.outputs.size(); ++k)
                    if (k != skipIdx && func.outputs[k].name == n) return true;
                for (const auto& inp : func.inputs)
                    if (inp.name == n) return true;
                return false;
            };
            if (isNameTaken(newName, i))
            {
                int suffix = 2;
                std::string candidate;
                do { candidate = newName + "_" + std::to_string(suffix++); }
                while (isNameTaken(candidate, i));
                newName = candidate;
            }
            PushUndoState();
            param.name = newName;
            doc->isDirty = true;
            outputsChanged = true;
        }

        // 删除按钮
        ImGui::SameLine();
        if (ImGui::SmallButton(ICON_FA_TRASH "##del"))
            outputDeleteIdx = i;

        ImGui::PopID();
    }
    // 延迟删除
    if (outputDeleteIdx >= 0)
    {
        PushUndoState();
        func.outputs.erase(func.outputs.begin() + outputDeleteIdx);
        doc->isDirty = true;
        outputsChanged = true;
    }

    // 添加输出参数按钮（自动生成唯一名称）
    if (ImGui::SmallButton(ICON_FA_PLUS " Add Output"))
    {
        PushUndoState();
        RTVariableDefinition newParam;
        std::string baseName = "NewOutput";
        std::string candidate = baseName;
        int suffix = 1;
        auto isNameUsed = [&](const std::string& n) -> bool {
            for (const auto& p : func.outputs) if (p.name == n) return true;
            for (const auto& p : func.inputs)  if (p.name == n) return true;
            return false;
        };
        while (isNameUsed(candidate))
            candidate = baseName + "_" + std::to_string(suffix++);
        newParam.name = candidate;
        newParam.dataType = RTPinDataType::Float;
        func.outputs.push_back(std::move(newParam));
        doc->isDirty = true;
        outputsChanged = true;
    }
    ImGui::Unindent(4.0f);  // outputs 参数行的 Indent 对应

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
    doc->rebuildEditorIndices();  // 立即重建（防止本帧内旧Pin*被访问导致崩溃）

    // 引脚 vector 已被替换，清除任何可能持有旧 Pin* 的拖拽状态
    // （下一帧 DrawNodes 开头会通过 PinId 重新查找有效指针）
    doc->newLinkPin      = nullptr;
    doc->newLinkPinId    = 0;
    doc->newNodeLinkPin  = nullptr;
    doc->newNodeLinkPinId= 0;
}
