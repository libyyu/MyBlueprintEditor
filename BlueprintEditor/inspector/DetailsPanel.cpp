// Auto-generated from InspectorPanel.cpp by tools/split_inspector.ps1
// Implements BlueprintEditor::DrawDetailsPanel (member of the editor class declared in BlueprintEditor.h).
#include "BlueprintEditor.h"
#include "ThemeManager.h"
#include "../Utils/Json/crude_json.h"
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
