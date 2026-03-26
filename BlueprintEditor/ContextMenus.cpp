// ContextMenus.cpp -- 右键菜单（Node / Pin / Link / Create New Node）
#include "BlueprintEditor.h"

void BlueprintEditor::DrawContextMenus(
    ImVec2 openPopupPosition,
    ed::NodeId& contextNodeId,
    ed::PinId&  contextPinId,
    ed::LinkId& contextLinkId,
    bool&       createNewNode,
    Pin*&       newNodeLinkPin)
{
    // ================================================================
    // 上下文菜单
    // ================================================================
# if 1
    // openPopupPosition 由调用方传入
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
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(10, 10));
    ImGui::PushStyleVar(ImGuiStyleVar_PopupRounding, 6.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8, 6));
    if (ImGui::BeginPopup("Node Context Menu"))
    {
        auto node = FindNode(contextNodeId);

        ImGui::TextColored(ImVec4(0.55f, 0.75f, 1.00f, 1.00f), "Node Context Menu");
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
        if (ImGui::MenuItem(ICON_FA_COPY " Copy", "Ctrl+C"))
        {
            ed::SelectNode(contextNodeId, false);
            CopySelectedNodes();
        }
        if (ImGui::MenuItem(ICON_FA_CLONE " Duplicate", "Ctrl+D"))
        {
            ed::SelectNode(contextNodeId, false);
            DuplicateSelectedNodes();
        }
        if (ImGui::MenuItem(ICON_FA_SCISSORS " Cut", "Ctrl+X"))
        {
            ed::SelectNode(contextNodeId, false);
            CutSelectedNodes();
        }
        ImGui::Separator();
        if (ImGui::MenuItem(ICON_FA_LINK " Select Connected"))
        {
            // 选中与此节点直接连接的所有节点
            if (node)
            {
                ed::SelectNode(contextNodeId, false);
                for (const auto& link : ActiveDoc()->links)
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
        if (ImGui::MenuItem(ICON_FA_TRASH_CAN " Delete"))
        {
            PushUndoState();  // 删除前保存快照
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

        ImGui::TextColored(ImVec4(0.55f, 0.75f, 1.00f, 1.00f), "Pin Context Menu");
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
            if (ImGui::MenuItem(ICON_FA_UNLINK " Break Link(s)"))
                ed::BreakLinks(contextPinId);
        }
        if (pin && pin->Node)
        {
            if (ImGui::MenuItem(ICON_FA_UNLINK " Break All Links on Node"))
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
            if (ImGui::MenuItem(ICON_FA_ARROWS_ROTATE " Reset Value"))
            {
                PushUndoState();
                pin->BoolValue = false;
                pin->IntValue = 0;
                pin->FloatValue = 0.0f;
                pin->StringValue.clear();
                pin->ObjectValue.clear();
                ActiveDoc()->isDirty = true;
            }
        }

        ImGui::EndPopup();
    }

    if (ImGui::BeginPopup("Link Context Menu"))
    {
        auto link = FindLink(contextLinkId);

        ImGui::TextColored(ImVec4(0.55f, 0.75f, 1.00f, 1.00f), "Link Context Menu");
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
        if (ImGui::MenuItem(ICON_FA_TRASH_CAN " Delete"))
        {
            PushUndoState();
            ed::DeleteLink(contextLinkId);
        }
        ImGui::EndPopup();
    }

    if (ImGui::BeginPopup("Create New Node"))
    {
        auto newNodePostion = openPopupPosition;

        // 如果剪贴板中有节点，提供 Paste Here 选项
        if (!m_ClipboardNodes.empty())
        {
            if (ImGui::MenuItem(ICON_FA_PASTE " Paste Here", "Ctrl+V"))
            {
                ImVec2 canvasPos = ed::ScreenToCanvas(newNodePostion);
                PasteNodes(canvasPos);
            }
        }

        // 快捷：添加注释框
        if (ImGui::MenuItem(ICON_FA_PENCIL " Add Comment"))
        {
            PushUndoState();
            Node* cmt = SpawnNodeByDef("Comment");
            if (cmt)
            {
                cmt->Name = "Comment";
                cmt->Size = ImVec2(300, 200);
                BuildNodes();
                ActiveDoc()->isDirty = true;
                ed::SetNodePosition(cmt->ID, newNodePostion);
                // 立即进入编辑状态
                ActiveDoc()->editingCommentId = cmt->ID;
                snprintf(ActiveDoc()->commentEditBuf,
                         sizeof(ActiveDoc()->commentEditBuf), "Comment");
            }
            createNewNode = false;
            ImGui::CloseCurrentPopup();
        }
        if (!m_ClipboardNodes.empty() || true)
            ImGui::Separator();

        Node* node = ShowCreateNodeMenu();   // PushUndoState 在 ShowCreateNodeMenu 内部调用

        if (node)
        {
            BuildNodes();

            createNewNode = false;
            ActiveDoc()->isDirty = true;

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
    ImGui::PopStyleVar(3);
    ed::Resume();
# endif

}
