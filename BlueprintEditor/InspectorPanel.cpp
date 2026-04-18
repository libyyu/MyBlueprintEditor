// InspectorPanel.cpp -- 检视器面板（节点列表 / 变量 / 函数 / Details）
// 从 EditorUI.cpp 拆分而来
#include "BlueprintEditor.h"
#include "ThemeManager.h"
#include "../Utils/Json/crude_json.h"

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
                int& renamingIdx = doc->funcRenamingIdx;
                char* renameBuf  = doc->funcRenameBuf;
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
                auto* doc_ = ActiveDoc();
                char* payloadBuf             = doc_ ? doc_->evtPayloadBuf : nullptr;
                std::string* pFireEventTarget = doc_ ? &doc_->evtFireTarget : nullptr;
                if (!payloadBuf) break;  // 安全守卫
                std::string& fireEventTarget = *pFireEventTarget;

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
        float dpi = ImGui::GetFontSize() / 13.0f;

        ImGui::TextUnformatted("New Variable");
        ImGui::Separator();

        ImGui::SetNextItemWidth(160.0f * dpi);
        ImGui::InputTextWithHint("##VarName", "Variable name...", newVarName, kVarNameBufSize);

        static const char* containerTypeNames[] = { "Single", "Array", "Map", "Set" };
        static const char* baseTypeNames[]      = { "Boolean", "Integer", "Float", "String", "Object", "Any" };
        static const RTPinDataType baseTypeValues[] = {
            RTPinDataType::Boolean, RTPinDataType::Integer, RTPinDataType::Float,
            RTPinDataType::String, RTPinDataType::Object, RTPinDataType::Any
        };
        static const char* keyTypeNames[]          = { "Boolean", "Integer", "Float", "String" };
        static const RTPinDataType keyTypeValues[]  = {
            RTPinDataType::Boolean, RTPinDataType::Integer, RTPinDataType::Float, RTPinDataType::String
        };

        ImGui::SetNextItemWidth(100.0f * dpi);
        ImGui::Combo("##ContType", &newVarTypeIdx, containerTypeNames, 4);
        ImGui::SameLine();
        static int newItemTypeIdx = 3;   // String
        static int newKeyTypeIdx  = 3;   // String
        ImGui::SetNextItemWidth(90.0f * dpi);
        ImGui::Combo("##ItemType", &newItemTypeIdx, baseTypeNames, 6);
        if (newVarTypeIdx == 2) {  // Map
            ImGui::SameLine();
            ImGui::TextUnformatted("Key:");
            ImGui::SameLine();
            ImGui::SetNextItemWidth(80.0f * dpi);
            ImGui::Combo("##KeyType", &newKeyTypeIdx, keyTypeNames, 4);
        }

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
                var.name          = newVarName;
                var.containerType = static_cast<RTContainerType>(newVarTypeIdx);
                var.itemType      = baseTypeValues[newItemTypeIdx];
                var.mapKeyType    = (newVarTypeIdx == 2) ? keyTypeValues[newKeyTypeIdx] : RTPinDataType::String;
                // dataType 同步
                if      (newVarTypeIdx == 0) var.dataType = baseTypeValues[newItemTypeIdx];
                else if (newVarTypeIdx == 1) var.dataType = RTPinDataType::Array;
                else if (newVarTypeIdx == 2) var.dataType = RTPinDataType::Map;
                else                         var.dataType = RTPinDataType::Set;
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

    // ── 辅助函数：构建完整类型标签 ──────────────────────────────────────
    auto buildTypeLabel = [](RTContainerType ct, RTPinDataType itemType, RTPinDataType mapKeyType) -> std::string {
        auto baseLabel = [](RTPinDataType t) -> const char* {
            switch(t) {
            case RTPinDataType::Boolean: return "Bool";
            case RTPinDataType::Integer: return "Int";
            case RTPinDataType::Float:   return "Float";
            case RTPinDataType::String:  return "String";
            case RTPinDataType::Object:  return "Object";
            case RTPinDataType::Any:     return "Any";
            default:                     return "?";
            }
        };
        switch (ct) {
        case RTContainerType::Array: return std::string("Array<") + baseLabel(itemType) + ">";
        case RTContainerType::Map:   return std::string("Map<") + baseLabel(mapKeyType) + ", " + baseLabel(itemType) + ">";
        case RTContainerType::Set:   return std::string("Set<") + baseLabel(itemType) + ">";
        default:                     return baseLabel(itemType);
        }
    };

    // ── 变量列表 ──────────────────────────────────────────────────────────
    // 类型名映射
    auto typeToStr = [&buildTypeLabel](const RTVariableDefinition& var) -> std::string {
        return buildTypeLabel(var.containerType, var.itemType, var.mapKeyType);
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
        case RTPinDataType::Set:     return ImVec4(0.7f, 0.4f, 0.9f, 1.0f);
        default:                   return ImVec4(0.7f, 0.7f, 0.7f, 1.0f);
        }
    };

    int deleteIdx = -1;  // 待删除的变量下标（延迟删除，避免迭代时修改容器）
    // 当前正在编辑的变量下标（-1 = 无）
    static int s_editingIdx = -1;
    static char s_editBuf[64] = "";
    static int s_selectedVarIdx = -1;  // 当前选中的变量（显示默认值编辑）

    for (int i = 0; i < (int)doc->variables.size(); ++i)
    {
        auto& var = doc->variables[i];
        ImGui::PushID(i);

        float rowHeight  = ImGui::GetFrameHeight();
        float dotRadius  = 5.0f;
        float dotOffsetX = 8.0f;

        // ── 类型色标 ──────────────────────────────────────────────────
        ImVec2 rowMin = ImGui::GetCursorScreenPos();
        ImGui::GetWindowDrawList()->AddCircleFilled(
            rowMin + ImVec2(dotOffsetX, rowHeight * 0.5f),
            dotRadius,
            ImGui::ColorConvertFloat4ToU32(typeColor(var.dataType)));

        // ── 计算各区域宽度 ────────────────────────────────────────────
        std::string typeLabel = "[" + typeToStr(var) + "]";
        float typeLabelW  = ImGui::CalcTextSize(typeLabel.c_str()).x + 8.0f;
        float deleteW     = ImGui::CalcTextSize(ICON_FA_TRASH_CAN).x + ImGui::GetStyle().FramePadding.x * 2 + 4.0f;
        float dotW        = dotOffsetX * 2.0f;                          // 色标占用宽度
        float spacing     = ImGui::GetStyle().ItemSpacing.x;
        float nameW       = paneWidth - dotW - typeLabelW - deleteW - spacing * 3;
        if (nameW < 40.0f) nameW = 40.0f;

        // ── 整行 Selectable（作为拖拽手柄 + 双击检测区域）───────────
        ImGui::SetCursorScreenPos(rowMin);
        // 用透明 Selectable 覆盖整行（不含删除按钮区域）
        float selectableW = paneWidth - deleteW - spacing;
        bool isSelected = (s_selectedVarIdx == i);
        bool rowClicked = ImGui::Selectable("##varrow", isSelected,
            ImGuiSelectableFlags_AllowOverlap | ImGuiSelectableFlags_AllowDoubleClick,
            ImVec2(selectableW, rowHeight));

        // 单击选中，双击编辑名称
        if (rowClicked)
        {
            s_selectedVarIdx = i;
            if (ImGui::IsMouseDoubleClicked(0))
            {
                s_editingIdx = i;
                snprintf(s_editBuf, sizeof(s_editBuf), "%s", var.name.c_str());
                ImGui::SetKeyboardFocusHere(1); // 下帧聚焦输入框
            }
        }

        // 拖拽源：整行都可以拖
        if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID))
        {
            VarDragPayload payload;
            snprintf(payload.varName, sizeof(payload.varName), "%s", var.name.c_str());
            payload.dataType = static_cast<int>(var.dataType);
            ImGui::SetDragDropPayload(VAR_DRAG_DROP_TYPE, &payload, sizeof(payload));
            ImGui::TextColored(typeColor(var.dataType), "● %s  [%s]", var.name.c_str(), typeToStr(var).c_str());
            ImGui::TextDisabled("Drop to canvas → select Get / Set");
            ImGui::EndDragDropSource();
        }

        // ── 名字区域：编辑模式 = InputText，否则 = 文字 ───────────────
        ImGui::SameLine(dotW + spacing, 0);
        ImGui::SetNextItemWidth(nameW);

        if (s_editingIdx == i)
        {
            // 编辑模式：显示输入框
            ImGuiInputTextFlags flags = ImGuiInputTextFlags_EnterReturnsTrue
                                      | ImGuiInputTextFlags_AutoSelectAll;
            bool confirmed = ImGui::InputText("##vedit", s_editBuf, sizeof(s_editBuf), flags);
            bool lostFocus = !ImGui::IsItemActive() && ImGui::IsItemDeactivated();

            if (confirmed || lostFocus)
            {
                if (s_editBuf[0] != '\0' && var.name != s_editBuf)
                {
                    bool dup = false;
                    for (int j = 0; j < (int)doc->variables.size(); ++j)
                        if (j != i && doc->variables[j].name == s_editBuf) { dup = true; break; }
                    if (!dup)
                    {
                        PushUndoState();
                        var.name = s_editBuf;
                        doc->isDirty = true;
                    }
                }
                s_editingIdx = -1;
            }
            // Esc 取消
            if (ImGui::IsKeyPressed(ImGuiKey_Escape))
                s_editingIdx = -1;
        }
        else
        {
            // 正常模式：显示变量名文字（截断过长名称）
            // 用 Dummy 占位保持行高一致，TextUnformatted 叠在上面
            ImVec2 textPos = ImGui::GetCursorScreenPos();
            ImGui::Dummy(ImVec2(nameW, rowHeight));
            // 裁剪渲染
            ImGui::GetWindowDrawList()->PushClipRect(textPos, textPos + ImVec2(nameW, rowHeight), true);
            ImGui::GetWindowDrawList()->AddText(
                textPos + ImVec2(0, (rowHeight - ImGui::GetTextLineHeight()) * 0.5f),
                ImGui::ColorConvertFloat4ToU32(ImGui::GetStyleColorVec4(ImGuiCol_Text)),
                var.name.c_str());
            ImGui::GetWindowDrawList()->PopClipRect();
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("Double-click to rename: %s", var.name.c_str());
        }

        ImGui::SameLine(0, spacing);

        // ── 类型标签（点击切换类型）──────────────────────────────────
        // 先渲染可点击的 InvisibleButton，再叠加文字
        {
            ImVec2 labelSize = ImVec2(typeLabelW, rowHeight);
            ImVec2 labelPos  = ImGui::GetCursorScreenPos();
            // 透明按钮作为点击热区
            ImGui::InvisibleButton("##vartype_btn", labelSize);
            bool typeClicked = ImGui::IsItemClicked();
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("Click to change type");
            // 叠加彩色文字
            ImGui::GetWindowDrawList()->AddText(
                labelPos + ImVec2(0, (rowHeight - ImGui::GetTextLineHeight()) * 0.5f),
                ImGui::ColorConvertFloat4ToU32(typeColor(var.dataType)),
                typeLabel.c_str());
            if (typeClicked)
                ImGui::OpenPopup("##VarType");
        }

        if (ImGui::BeginPopup("##VarType"))
        {
            float dpi = ImGui::GetFontSize() / 13.0f;

            static const char* containerTypeNamesPopup[] = { "Single", "Array", "Map", "Set" };
            static const char* baseTypeNamesPopup[]      = { "Boolean", "Integer", "Float", "String", "Object", "Any" };
            static const RTPinDataType baseTypeValuesPopup[] = {
                RTPinDataType::Boolean, RTPinDataType::Integer, RTPinDataType::Float,
                RTPinDataType::String, RTPinDataType::Object, RTPinDataType::Any
            };
            static const char* keyTypeNamesPopup[]         = { "Boolean", "Integer", "Float", "String" };
            static const RTPinDataType keyTypeValuesPopup[] = {
                RTPinDataType::Boolean, RTPinDataType::Integer, RTPinDataType::Float, RTPinDataType::String
            };

            // 当前容器类型索引
            int curContIdx = static_cast<int>(var.containerType);
            ImGui::TextUnformatted("Container:");
            ImGui::SetNextItemWidth(100.0f * dpi);
            if (ImGui::Combo("##PopContType", &curContIdx, containerTypeNamesPopup, 4))
            {
                PushUndoState();
                var.containerType = static_cast<RTContainerType>(curContIdx);
                if (curContIdx == 0) var.dataType = var.itemType;
                else if (curContIdx == 1) var.dataType = RTPinDataType::Array;
                else if (curContIdx == 2) var.dataType = RTPinDataType::Map;
                else                      var.dataType = RTPinDataType::Set;
                doc->isDirty = true;
            }

            // 元素类型索引
            int curItemIdx = 0;
            for (int ti = 0; ti < 6; ++ti)
                if (baseTypeValuesPopup[ti] == var.itemType) { curItemIdx = ti; break; }
            ImGui::TextUnformatted("Item type:");
            ImGui::SetNextItemWidth(100.0f * dpi);
            if (ImGui::Combo("##PopItemType", &curItemIdx, baseTypeNamesPopup, 6))
            {
                PushUndoState();
                var.itemType = baseTypeValuesPopup[curItemIdx];
                if (var.containerType == RTContainerType::Single)
                    var.dataType = var.itemType;
                doc->isDirty = true;
            }

            // Map 键类型
            if (var.containerType == RTContainerType::Map)
            {
                int curKeyIdx = 3;  // default String
                for (int ki = 0; ki < 4; ++ki)
                    if (keyTypeValuesPopup[ki] == var.mapKeyType) { curKeyIdx = ki; break; }
                ImGui::TextUnformatted("Key type:");
                ImGui::SetNextItemWidth(100.0f * dpi);
                if (ImGui::Combo("##PopKeyType", &curKeyIdx, keyTypeNamesPopup, 4))
                {
                    PushUndoState();
                    var.mapKeyType = keyTypeValuesPopup[curKeyIdx];
                    doc->isDirty = true;
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
            ImGui::Text("Type:     %s", typeToStr(var).c_str());
            ImGui::Text("Exposed:  %s", var.isExposed ? "Yes" : "No");
            if (!var.tooltip.empty()) ImGui::Text("Tip: %s", var.tooltip.c_str());
            ImGui::EndTooltip();
        }

        ImGui::PopID();
    }

    // 延迟删除
    if (deleteIdx >= 0)
    {
        if (s_selectedVarIdx == deleteIdx) s_selectedVarIdx = -1;
        else if (s_selectedVarIdx > deleteIdx) --s_selectedVarIdx;
        PushUndoState();  // 删除变量前保存快照
        doc->variables.erase(doc->variables.begin() + deleteIdx);
        doc->isDirty = true;
    }

    // ── 选中变量的详情（默认值编辑，UE4 风格）─────────────────────────────
    if (s_selectedVarIdx >= 0 && s_selectedVarIdx < (int)doc->variables.size())
    {
        auto& selVar = doc->variables[s_selectedVarIdx];
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        // 标题
        ImGui::TextColored(typeColor(selVar.dataType), "● %s", selVar.name.c_str());
        ImGui::SameLine();
        ImGui::TextDisabled("[%s]", typeToStr(selVar).c_str());

        ImGui::Spacing();

        // 默认值编辑（根据类型不同渲染不同控件）
        ImGui::TextUnformatted("Default Value:");
        ImGui::PushID("##vardefault");

        bool changed = false;
        if (selVar.containerType != RTContainerType::Single)
        {
            // ── 容器类型结构化编辑（UE4 风格）────────────────────────────────
            // 内部状态：缓存解析后的元素列表，避免每帧重新解析 JSON
            // key = (varName + dataType) 作为缓存 invalidate 标记
            struct ContainerEditState {
                std::string         cacheKey;
                // Array/Set: elements[i] = string 表示的值
                // Map: elements[i] = "key\nvalue" 分两行
                std::vector<std::string> keys;    // Map: key；Array/Set: 空
                std::vector<std::string> values;  // 所有类型: 元素值
                bool                dirty = false; // values 被修改，需回写 JSON
            };
            static ContainerEditState s_ces;

            // 构造缓存 key
            std::string ck = selVar.name + "|" +
                             std::to_string(static_cast<int>(selVar.containerType)) + "|" +
                             std::to_string(static_cast<int>(selVar.itemType));

            // 解析 JSON → state（缓存 miss 时）
            if (s_ces.cacheKey != ck)
            {
                s_ces.cacheKey = ck;
                s_ces.keys.clear();
                s_ces.values.clear();
                s_ces.dirty = false;

                std::string jsonStr = selVar.defaultValue.asString();
                if (!jsonStr.empty())
                {
                    using namespace crude_json;
                    value parsed = value::parse(jsonStr);
                    if (selVar.containerType == RTContainerType::Map && parsed.is_object())
                    {
                        for (const auto& kv : parsed.get<object>())
                        {
                            s_ces.keys.push_back(kv.first);
                            s_ces.values.push_back(
                                kv.second.is_string() ? kv.second.get<std::string>()
                                                      : kv.second.dump());
                        }
                    }
                    else if (parsed.is_array())
                    {
                        for (const auto& el : parsed.get<array>())
                            s_ces.values.push_back(
                                el.is_string() ? el.get<std::string>() : el.dump());
                    }
                }
            }

            // 辅助：将 state 回写成 JSON 并存入 defaultValue
            auto flushToVar = [&]() {
                using namespace crude_json;
                if (selVar.containerType == RTContainerType::Map)
                {
                    object obj;
                    for (size_t i = 0; i < s_ces.values.size(); ++i)
                    {
                        std::string key = i < s_ces.keys.size() ? s_ces.keys[i] : "";
                        if (!key.empty())
                        {
                            // 尝试解析为 JSON，失败则视为字符串
                            value v = value::parse(s_ces.values[i]);
                            obj[key] = v.is_null() && !s_ces.values[i].empty()
                                ? value(s_ces.values[i]) : v;
                        }
                    }
                    selVar.defaultValue = RTVariant(value(std::move(obj)).dump());
                }
                else
                {
                    array arr;
                    for (const auto& sv : s_ces.values)
                    {
                        value v = value::parse(sv);
                        arr.push_back(v.is_null() && !sv.empty() ? value(sv) : v);
                    }
                    selVar.defaultValue = RTVariant(value(std::move(arr)).dump());
                }
                doc->isDirty = true;
                s_ces.dirty  = false;
            };

            // 辅助：根据 itemType 渲染单个元素编辑控件，返回 true 表示值改变
            auto renderItemEdit = [&](int idx, std::string& valStr, float itemW) -> bool {
                bool itemChanged = false;
                ImGui::PushID(idx);
                switch (selVar.itemType)
                {
                case RTPinDataType::Boolean: {
                    bool bv = (valStr == "true" || valStr == "1");
                    if (ImGui::Checkbox("##bv", &bv)) {
                        valStr = bv ? "true" : "false";
                        itemChanged = true;
                    }
                    break;
                }
                case RTPinDataType::Integer: {
                    int64_t iv = 0;
                    try { iv = std::stoll(valStr); } catch (...) {}
                    ImS64 imv = static_cast<ImS64>(iv);
                    ImGui::SetNextItemWidth(itemW);
                    if (ImGui::DragScalar("##iv", ImGuiDataType_S64, &imv, 1.0f)) {
                        valStr = std::to_string(static_cast<int64_t>(imv));
                        itemChanged = true;
                    }
                    break;
                }
                case RTPinDataType::Float: {
                    float fv = 0.0f;
                    try { fv = std::stof(valStr); } catch (...) {}
                    ImGui::SetNextItemWidth(itemW);
                    if (ImGui::DragFloat("##fv", &fv, 0.01f)) {
                        valStr = std::to_string(fv);
                        itemChanged = true;
                    }
                    break;
                }
                default: { // String / Object / Any
                    static char s_itembuf[512];
                    snprintf(s_itembuf, sizeof(s_itembuf), "%s", valStr.c_str());
                    ImGui::SetNextItemWidth(itemW);
                    if (ImGui::InputText("##sv", s_itembuf, sizeof(s_itembuf))) {
                        valStr = s_itembuf;
                        itemChanged = true;
                    }
                    if (ImGui::IsItemActivated()) PushUndoState();
                    break;
                }
                }
                ImGui::PopID();
                return itemChanged;
            };

            // ── 渲染元素列表 ──────────────────────────────────────────────────
            int  removeIdx  = -1;
            bool anyChanged = false;
            int  count = static_cast<int>(s_ces.values.size());

            // 表头：元素数 + 添加按钮
            ImGui::TextDisabled("%d element(s)", count);
            ImGui::SameLine(paneWidth - 70.0f);
            if (ImGui::SmallButton(ICON_FA_PLUS " Add"))
            {
                PushUndoState();
                if (selVar.containerType == RTContainerType::Map)
                    s_ces.keys.push_back("key" + std::to_string(count));
                s_ces.values.push_back(
                    selVar.itemType == RTPinDataType::Boolean ? "false" :
                    selVar.itemType == RTPinDataType::Integer ? "0" :
                    selVar.itemType == RTPinDataType::Float   ? "0.0" : "");
                anyChanged = true;
            }

            ImGui::Spacing();

            // 元素列表（最多显示 20 条，超出时滚动）
            float listH = std::min(count, 12) * (ImGui::GetFrameHeightWithSpacing()) + 4.0f;
            if (count > 0)
                ImGui::BeginChild("##cedit", ImVec2(0, listH), false);

            for (int i = 0; i < count; ++i)
            {
                ImGui::PushID(i);

                // ── 序号标签 ──────────────────────────────────────────────
                ImGui::TextDisabled("[%d]", i);
                ImGui::SameLine();

                float delW    = ImGui::CalcTextSize(ICON_FA_TRASH_CAN).x + 8.0f;
                float spacing = ImGui::GetStyle().ItemSpacing.x;

                if (selVar.containerType == RTContainerType::Map)
                {
                    // Map: key 输入 + value 输入
                    float half = (paneWidth - delW - spacing * 3 - 30.0f) * 0.45f;
                    static char s_keybuf[256];
                    std::string& keyStr = s_ces.keys.size() > (size_t)i
                        ? s_ces.keys[i] : *(s_ces.keys.emplace_back(), &s_ces.keys.back());
                    snprintf(s_keybuf, sizeof(s_keybuf), "%s", keyStr.c_str());
                    ImGui::SetNextItemWidth(half);
                    if (ImGui::InputText("##mkey", s_keybuf, sizeof(s_keybuf))) {
                        keyStr = s_keybuf;
                        anyChanged = true;
                    }
                    if (ImGui::IsItemActivated()) PushUndoState();
                    ImGui::SameLine();
                    ImGui::TextDisabled(":");
                    ImGui::SameLine();
                    if (renderItemEdit(i + 1000, s_ces.values[i], half))
                        anyChanged = true;
                }
                else
                {
                    // Array/Set: 只有 value
                    float valW = paneWidth - delW - spacing * 2 - 30.0f;
                    if (renderItemEdit(i, s_ces.values[i], valW))
                        anyChanged = true;
                }

                // 删除按钮
                ImGui::SameLine();
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0,0,0,0));
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.8f,0.2f,0.2f,0.5f));
                if (ImGui::SmallButton(ICON_FA_TRASH_CAN)) removeIdx = i;
                ImGui::PopStyleColor(2);

                ImGui::PopID();
            }
            if (count > 0)
                ImGui::EndChild();

            // 延迟删除
            if (removeIdx >= 0)
            {
                PushUndoState();
                s_ces.values.erase(s_ces.values.begin() + removeIdx);
                if (selVar.containerType == RTContainerType::Map &&
                    (int)s_ces.keys.size() > removeIdx)
                    s_ces.keys.erase(s_ces.keys.begin() + removeIdx);
                anyChanged = true;
            }

            if (anyChanged)
                flushToVar();

            // JSON 原文折叠查看（高级）
            if (ImGui::TreeNode("Raw JSON"))
            {
                static char s_rawbuf[2048] = "";
                std::string curRaw = selVar.defaultValue.asString();
                if (curRaw.size() < sizeof(s_rawbuf) - 1)
                    snprintf(s_rawbuf, sizeof(s_rawbuf), "%s", curRaw.c_str());
                ImGui::SetNextItemWidth(paneWidth - 8.0f);
                if (ImGui::InputTextMultiline("##rawjson", s_rawbuf, sizeof(s_rawbuf),
                    ImVec2(0, 60), ImGuiInputTextFlags_EnterReturnsTrue))
                {
                    PushUndoState();
                    selVar.defaultValue = RTVariant(std::string(s_rawbuf));
                    s_ces.cacheKey = "";  // 强制重新解析
                    doc->isDirty = true;
                }
                ImGui::TreePop();
            }
        }
        else switch (selVar.itemType)
        {
            case RTPinDataType::Boolean:
            {
                bool bval = selVar.defaultValue.asBool();
                bool prev = bval;
                if (ImGui::Checkbox("##defbool", &bval) && bval != prev)
                {
                    PushUndoState();
                    selVar.defaultValue = RTVariant(bval);
                    doc->isDirty = true;
                }
                break;
            }
            case RTPinDataType::Integer:
            {
                int64_t ival = selVar.defaultValue.asInt();
                ImS64 v = static_cast<ImS64>(ival);
                ImGui::SetNextItemWidth(paneWidth - 8.0f);
                if (ImGui::DragScalar("##defint", ImGuiDataType_S64, &v, 1.0f))
                {
                    PushUndoState();
                    selVar.defaultValue = RTVariant(static_cast<int64_t>(v));
                    doc->isDirty = true;
                }
                break;
            }
            case RTPinDataType::Float:
            {
                float fval = static_cast<float>(selVar.defaultValue.asFloat());
                ImGui::SetNextItemWidth(paneWidth - 8.0f);
                if (ImGui::DragFloat("##deffloat", &fval, 0.01f))
                {
                    PushUndoState();
                    selVar.defaultValue = RTVariant(static_cast<double>(fval));
                    doc->isDirty = true;
                }
                break;
            }
            default:  // String / Object / Any
            {
                static char s_defaultStrBuf[512] = "";
                std::string curStr = selVar.defaultValue.asString();
                if (curStr != s_defaultStrBuf)
                    snprintf(s_defaultStrBuf, sizeof(s_defaultStrBuf), "%s", curStr.c_str());
                ImGui::SetNextItemWidth(paneWidth - 8.0f);
                if (ImGui::InputText("##defstr", s_defaultStrBuf, sizeof(s_defaultStrBuf)))
                {
                    selVar.defaultValue = RTVariant(std::string(s_defaultStrBuf));
                    doc->isDirty = true;
                }
                if (ImGui::IsItemActivated()) PushUndoState();
                break;
            }
        }
        ImGui::PopID();

        // 是否暴露（Exposed）
        ImGui::Spacing();
        bool exposed = selVar.isExposed;
        if (ImGui::Checkbox("Exposed (visible externally)", &exposed))
        {
            PushUndoState();
            selVar.isExposed = exposed;
            doc->isDirty = true;
        }
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("When enabled, this variable can be set via\n-v flag in CLI or SetVariable() in code");
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
                    float fval = static_cast<float>(pin.FloatValue);
                    ImGui::SetNextItemWidth(editWidth);
                    if (ImGui::DragFloat("##val", &fval, 0.1f))
                    {
                        PushUndoState();
                        pin.FloatValue = static_cast<double>(fval);
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
    static const char* containerTypeNamesFunc[] = { "Single", "Array", "Map", "Set" };
    static const char* baseTypeNamesFunc[]      = { "Boolean", "Integer", "Float", "String", "Object", "Any" };
    static const RTPinDataType baseTypeValuesFunc[] = {
        RTPinDataType::Boolean, RTPinDataType::Integer, RTPinDataType::Float,
        RTPinDataType::String, RTPinDataType::Object, RTPinDataType::Any
    };
    static const char* keyTypeNamesFunc[]         = { "Boolean", "Integer", "Float", "String" };
    static const RTPinDataType keyTypeValuesFunc[] = {
        RTPinDataType::Boolean, RTPinDataType::Integer, RTPinDataType::Float, RTPinDataType::String
    };

    // Helper: 从 baseTypeValues 找 index
    auto findBaseTypeIdx = [](RTPinDataType dt) -> int {
        static const RTPinDataType bv[] = {
            RTPinDataType::Boolean, RTPinDataType::Integer, RTPinDataType::Float,
            RTPinDataType::String, RTPinDataType::Object, RTPinDataType::Any
        };
        for (int i = 0; i < 6; ++i) if (bv[i] == dt) return i;
        return 3;
    };
    auto findKeyTypeIdx = [](RTPinDataType dt) -> int {
        static const RTPinDataType kv[] = {
            RTPinDataType::Boolean, RTPinDataType::Integer, RTPinDataType::Float, RTPinDataType::String
        };
        for (int i = 0; i < 4; ++i) if (kv[i] == dt) return i;
        return 3;
    };
    // 容器类型 combo 宽度
    float contComboW = 72.0f;
    float itemComboW = 64.0f;
    float keyComboW  = 60.0f;

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
        std::string& lastCatFuncId = doc->inspFuncCatLastId;
        char*        catBuf        = doc->inspFuncCatBuf;
        if (lastCatFuncId != func.id)
        {
            snprintf(catBuf, 128, "%s", func.category.c_str());
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
        std::string& lastFuncId = doc->inspFuncDescLastId;
        char*        descBuf    = doc->inspFuncDescBuf;
        if (lastFuncId != func.id)
        {
            snprintf(descBuf, 256, "%s", func.description.c_str());
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

        // 容器类型下拉
        int contIdx = static_cast<int>(param.containerType);
        ImGui::SetNextItemWidth(contComboW);
        if (ImGui::Combo("##ctype", &contIdx, containerTypeNamesFunc, 4))
        {
            PushUndoState();
            param.containerType = static_cast<RTContainerType>(contIdx);
            if      (contIdx == 0) param.dataType = param.itemType;
            else if (contIdx == 1) param.dataType = RTPinDataType::Array;
            else if (contIdx == 2) param.dataType = RTPinDataType::Map;
            else                   param.dataType = RTPinDataType::Set;
            doc->isDirty = true;
            inputsChanged = true;
        }
        ImGui::SameLine();
        // 元素类型下拉
        int itemIdx = findBaseTypeIdx(param.itemType);
        ImGui::SetNextItemWidth(itemComboW);
        if (ImGui::Combo("##itype", &itemIdx, baseTypeNamesFunc, 6))
        {
            PushUndoState();
            param.itemType = baseTypeValuesFunc[itemIdx];
            if (param.containerType == RTContainerType::Single)
                param.dataType = param.itemType;
            doc->isDirty = true;
            inputsChanged = true;
        }
        // Map key type
        if (param.containerType == RTContainerType::Map)
        {
            ImGui::SameLine();
            int keyIdx = findKeyTypeIdx(param.mapKeyType);
            ImGui::SetNextItemWidth(keyComboW);
            if (ImGui::Combo("##ktype", &keyIdx, keyTypeNamesFunc, 4))
            {
                PushUndoState();
                param.mapKeyType = keyTypeValuesFunc[keyIdx];
                doc->isDirty = true;
                inputsChanged = true;
            }
        }

        // 名称编辑
        ImGui::SameLine();
        char nameBuf[64];
        snprintf(nameBuf, sizeof(nameBuf), "%s", param.name.c_str());
        float dynNameW = nameW - (param.containerType == RTContainerType::Map ? keyComboW + spacingX : 0.0f);
        if (dynNameW < 30.0f) dynNameW = 30.0f;
        ImGui::SetNextItemWidth(dynNameW);
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

        // 容器类型下拉
        int contIdx = static_cast<int>(param.containerType);
        ImGui::SetNextItemWidth(contComboW);
        if (ImGui::Combo("##ctype", &contIdx, containerTypeNamesFunc, 4))
        {
            PushUndoState();
            param.containerType = static_cast<RTContainerType>(contIdx);
            if      (contIdx == 0) param.dataType = param.itemType;
            else if (contIdx == 1) param.dataType = RTPinDataType::Array;
            else if (contIdx == 2) param.dataType = RTPinDataType::Map;
            else                   param.dataType = RTPinDataType::Set;
            doc->isDirty = true;
            outputsChanged = true;
        }
        ImGui::SameLine();
        // 元素类型下拉
        int itemIdx = findBaseTypeIdx(param.itemType);
        ImGui::SetNextItemWidth(itemComboW);
        if (ImGui::Combo("##itype", &itemIdx, baseTypeNamesFunc, 6))
        {
            PushUndoState();
            param.itemType = baseTypeValuesFunc[itemIdx];
            if (param.containerType == RTContainerType::Single)
                param.dataType = param.itemType;
            doc->isDirty = true;
            outputsChanged = true;
        }
        // Map key type
        if (param.containerType == RTContainerType::Map)
        {
            ImGui::SameLine();
            int keyIdx = findKeyTypeIdx(param.mapKeyType);
            ImGui::SetNextItemWidth(keyComboW);
            if (ImGui::Combo("##ktype", &keyIdx, keyTypeNamesFunc, 4))
            {
                PushUndoState();
                param.mapKeyType = keyTypeValuesFunc[keyIdx];
                doc->isDirty = true;
                outputsChanged = true;
            }
        }

        // 名称编辑（独立 buffer）
        ImGui::SameLine();
        char nameBuf[64];
        snprintf(nameBuf, sizeof(nameBuf), "%s", param.name.c_str());
        float dynNameW = nameW - (param.containerType == RTContainerType::Map ? keyComboW + spacingX : 0.0f);
        if (dynNameW < 30.0f) dynNameW = 30.0f;
        ImGui::SetNextItemWidth(dynNameW);
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
