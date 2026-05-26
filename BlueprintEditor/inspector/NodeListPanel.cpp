// Auto-generated from InspectorPanel.cpp by tools/split_inspector.ps1
// Implements BlueprintEditor::DrawNodeListPanel (member of the editor class declared in BlueprintEditor.h).
#include "BlueprintEditor.h"
#include "ThemeManager.h"
#include "../Utils/Json/crude_json.h"
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
