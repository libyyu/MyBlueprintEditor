// LinkRenderer.cpp -- 链接渲染 + BeginCreate/BeginDelete 连线交互
#include "BlueprintEditor.h"

// 辅助宏：通过 PinId 设置 newLinkPin / newNodeLinkPin（通过 ID 确保持久有效性）
#define SET_NEW_LINK_PIN(doc, pin)     do { (doc)->newLinkPinId     = (pin) ? (pin)->ID : ed::PinId(0); (doc)->newLinkPin     = (pin); } while(0)
#define SET_NEW_NODE_PIN(doc, pin)     do { (doc)->newNodeLinkPinId = (pin) ? (pin)->ID : ed::PinId(0); (doc)->newNodeLinkPin = (pin); } while(0)
#define CLEAR_NEW_LINK_PIN(doc)        do { (doc)->newLinkPinId     = 0; (doc)->newLinkPin     = nullptr; } while(0)
#define CLEAR_NEW_NODE_PIN(doc)        do { (doc)->newNodeLinkPinId = 0; (doc)->newNodeLinkPin = nullptr; } while(0)

void BlueprintEditor::DrawLinks()
{
    auto* _doc = ActiveDoc();
    if (!_doc) return;

    auto& createNewNode  = _doc->createNewNode;

        // 链接
        // ================================================================
        for (auto& link : ActiveDoc()->links)
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

                    // 通过宏同时保存 PinId，防止后续帧 Pin* 悬空
                    SET_NEW_LINK_PIN(_doc, startPin ? startPin : endPin);

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
                                // 检查是否有内置转换节点可以自动插入
                                std::string convDef = GetConversionNode(startPin->Type, endPin->Type);
                                if (!convDef.empty())
                                {
                                    showLabel(("+ Auto-convert via " + convDef).c_str(),
                                              ImColor(32, 45, 45, 180));
                                    if (ed::AcceptNewItem(ImColor(100, 200, 255), 4.0f))
                                    {
                                        InsertConversionNode(startPin, startPinId, endPin, endPinId);
                                    }
                                }
                                else
                                {
                                    showLabel("x Incompatible Pin Type", ImColor(45, 32, 32, 180));
                                    ed::RejectNewItem(ImColor(255, 128, 128), 1.0f);
                                }
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
                    Pin* linkPin = FindPin(pinId);
                    SET_NEW_LINK_PIN(_doc, linkPin);
                    if (linkPin)
                        showLabel("+ Create Node", ImColor(32, 45, 32, 180));

                    if (ed::AcceptNewItem())
                    {
                        createNewNode = true;
                        SET_NEW_NODE_PIN(_doc, FindPin(pinId));
                        CLEAR_NEW_LINK_PIN(_doc);
                        ed::Suspend();
                        _doc->createNodePopupPos = ImGui::GetMousePos();  // 记录弹出时的坐标
                        ImGui::OpenPopup("Create New Node");
                        ed::Resume();
                    }
                }
            }
            else
            {
                CLEAR_NEW_LINK_PIN(_doc);
            }

            ed::EndCreate();

            if (ed::BeginDelete())
            {
                bool deletePushed = false;  // 一次 BeginDelete 只 push 一次 undo
                bool deleted = false;

                ed::NodeId nodeId = 0;
                while (ed::QueryDeletedNode(&nodeId))
                {
                    if (ed::AcceptDeletedItem())
                    {
                        if (!deletePushed) { PushUndoState(); deletePushed = true; }
                        auto id = std::find_if(ActiveDoc()->nodes.begin(), ActiveDoc()->nodes.end(), [nodeId](auto& node) { return node.ID == nodeId; });
                        if (id != ActiveDoc()->nodes.end())
                        {
                            ActiveDoc()->nodes.erase(id);
                            ActiveDoc()->isDirty = true;
                            deleted = true;
                        }
                    }
                }

                ed::LinkId linkId = 0;
                while (ed::QueryDeletedLink(&linkId))
                {
                    if (ed::AcceptDeletedItem())
                    {
                        if (!deletePushed) { PushUndoState(); deletePushed = true; }
                        auto id = std::find_if(ActiveDoc()->links.begin(), ActiveDoc()->links.end(), [linkId](auto& link) { return link.ID == linkId; });
                        if (id != ActiveDoc()->links.end())
                        {
                            ActiveDoc()->links.erase(id);
                            ActiveDoc()->isDirty = true;
                            deleted = true;
                        }
                    }
                }

                if (deleted)
                    ActiveDoc()->invalidateEditorIndices();
            }
            ed::EndDelete();
        }
}
