// NodeRenderer.cpp -- 所有节点类型渲染（Blueprint/Simple/Tree/Houdini/Comment）
// 由 OnFrame 通过 DrawNodes(builder) 调用（builder 在调用方创建后传入）
#include "BlueprintEditor.h"

void BlueprintEditor::DrawNodes(util::BlueprintNodeBuilder& builder)
{
    auto* _doc = ActiveDoc();
    if (!_doc) return;

    auto& newLinkPin = _doc->newLinkPin;

        auto cursorTopLeft = ImGui::GetCursorScreenPos();


        for (auto& node : ActiveDoc()->nodes)
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
                    ActiveDoc()->links.erase(std::remove_if(ActiveDoc()->links.begin(), ActiveDoc()->links.end(),
                        [pinId](const Link& l) { return l.StartPinID == pinId || l.EndPinID == pinId; }),
                        ActiveDoc()->links.end());
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
                        // 折叠/展开小按钮（紧贴节点名左侧）
                        {
                            ImGui::PushStyleColor(ImGuiCol_Button,        ImVec4(0,0,0,0));
                            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(1,1,1,0.15f));
                            ImGui::PushStyleColor(ImGuiCol_ButtonActive,  ImVec4(1,1,1,0.25f));
                            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2,2));
                            const char* collapseIcon = node.isCollapsed ? ICON_FA_CARET_RIGHT : ICON_FA_CARET_DOWN;
                            if (ImGui::SmallButton(collapseIcon))
                            {
                                node.isCollapsed = !node.isCollapsed;
                                ActiveDoc()->isDirty = true;
                            }
                            ImGui::PopStyleVar();
                            ImGui::PopStyleColor(3);
                        }
                        ImGui::Spring(0, 4.0f);
                        ImGui::TextUnformatted(node.Name.c_str());
                        ImGui::Spring(1);
                        ImGui::Dummy(ImVec2(0, 28));
                        if (hasOutputDelegates && !node.isCollapsed)
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
                            }
                            ImGui::Spring(1, 0);
                            ImGui::EndVertical();
                            ImGui::Spring(0, ImGui::GetStyle().ItemSpacing.x / 2);
                        }
                        else
                            ImGui::Spring(0);
                    builder.EndHeader();
                }

                if (node.isCollapsed)
                {
                    // 折叠时：静默注册所有引脚（保持连线锚点有效），但不渲染任何内容
                    for (auto& input : node.Inputs)
                    {
                        if (input.IsHidden) continue;
                        builder.Input(input.ID);
                        ImGui::Dummy(ImVec2(0, 0));
                        builder.EndInput();
                    }
                    for (auto& output : node.Outputs)
                    {
                        if (output.IsHidden || output.Type == PinType::Delegate) continue;
                        builder.Output(output.ID);
                        ImGui::Dummy(ImVec2(0, 0));
                        builder.EndOutput();
                    }
                }
                else
                {
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

                        auto& s_StringBuffers = ActiveDoc()->pinStringBuffers;

                        if (input.Type == PinType::Bool)
                        {
                            // 点击前暂存旧值，点击后 push（快照包含旧值）再写新值
                            bool prevVal = input.BoolValue;
                            if (ImGui::Checkbox("##value", &input.BoolValue))
                            {
                                bool newVal = input.BoolValue;
                                input.BoolValue = prevVal;   // 先恢复
                                PushUndoState();             // 快照（记录旧值）
                                input.BoolValue = newVal;    // 再应用新值
                                ActiveDoc()->isDirty = true;
                            }
                        }
                        else if (input.Type == PinType::Int)
                        {
                            ImGui::SetNextItemWidth(80.0f);
                            ImS64 v = static_cast<ImS64>(input.IntValue);
                            if (ImGui::DragScalar("##value", ImGuiDataType_S64, &v, 1.0f))
                            {
                                input.IntValue = static_cast<int64_t>(v);
                                ActiveDoc()->isDirty = true;
                            }
                            if (ImGui::IsItemActivated()) PushUndoState();  // 拖拽开始帧保存
                        }
                        else if (input.Type == PinType::Float)
                        {
                            ImGui::SetNextItemWidth(80.0f);
                            if (ImGui::DragFloat("##value", &input.FloatValue, 0.01f))
                                ActiveDoc()->isDirty = true;
                            if (ImGui::IsItemActivated()) PushUndoState();  // 拖拽开始帧保存
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
                                ActiveDoc()->isDirty = true;
                            }
                            if (ImGui::IsItemActivated()) PushUndoState();  // 输入框获焦时保存
                        }
                        else if (input.Type == PinType::Object)
                        {
                            auto& s_ObjectBuffers = ActiveDoc()->pinObjectBuffers;
                            auto key = reinterpret_cast<uintptr_t>(input.ID.AsPointer());
                            auto& buf = s_ObjectBuffers[key];
                            if (buf[0] == '\0' && !input.ObjectValue.empty())
                                snprintf(buf.data(), buf.size(), "%s", input.ObjectValue.c_str());
                            ImGui::SetNextItemWidth(100.0f);
                            if (ImGui::InputText("##value", buf.data(), buf.size()))
                            {
                                input.ObjectValue = buf.data();
                                ActiveDoc()->isDirty = true;
                            }
                            if (ImGui::IsItemActivated()) PushUndoState();  // 输入框获焦时保存
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
                            // MakeMap: 成对删除，只在 Key 引脚上显示 [-] 按钮
                            bool isMakeMap = (node.DefinitionId == "MakeMap");
                            bool showRemoveBtn = true;
                            if (isMakeMap)
                            {
                                int dynIdx = pinIdx - node.DynamicInputFixedCount;
                                // 动态引脚中偶数索引是 Key，奇数是 Value
                                if (dynIdx % 2 != 0)
                                    showRemoveBtn = false;
                            }

                            if (showRemoveBtn)
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
                                    ActiveDoc()->links.erase(std::remove_if(ActiveDoc()->links.begin(), ActiveDoc()->links.end(),
                                        [pinId](const Link& l) { return l.StartPinID == pinId || l.EndPinID == pinId; }),
                                        ActiveDoc()->links.end());
                                    node.Inputs[pinIdx].StringValue = "\x01REMOVE";

                                    // MakeMap: 同时标记删除配对的 Value 引脚
                                    if (isMakeMap && pinIdx + 1 < static_cast<int>(node.Inputs.size()))
                                    {
                                        ed::PinId valuePinId = node.Inputs[pinIdx + 1].ID;
                                        ActiveDoc()->links.erase(std::remove_if(ActiveDoc()->links.begin(), ActiveDoc()->links.end(),
                                            [valuePinId](const Link& l) { return l.StartPinID == valuePinId || l.EndPinID == valuePinId; }),
                                            ActiveDoc()->links.end());
                                        node.Inputs[pinIdx + 1].StringValue = "\x01REMOVE";
                                    }

                                    ActiveDoc()->isDirty = true;
                                }
                                ImGui::PopStyleColor(3);
                                ImGui::PopID();
                            }
                        }
                    }

                    ImGui::PopStyleVar();
                    builder.EndInput();
                }

                // Process pending removals for dynamic pins
                if (node.HasDynamicInputs)
                {
                    auto oldSize = node.Inputs.size();
                    node.Inputs.erase(std::remove_if(node.Inputs.begin(), node.Inputs.end(),
                        [](const Pin& p) { return p.StringValue == "\x01REMOVE"; }),
                        node.Inputs.end());
                    if (node.Inputs.size() != oldSize)
                        ActiveDoc()->invalidateEditorIndices();  // 引脚被删除，需要重建索引
                    BuildNode(&node);
                }

                if (isSimple)
                {
                    builder.Middle();

                    ImGui::Spring(1, 0);
                    ImGui::TextUnformatted(node.Name.c_str());
                    // Simple + Dynamic: 在节点名称右侧显示 [+] 按钮
                    if (node.HasDynamicInputs)
                    {
                        ImGui::Spring(0);
                        ImGui::PushID(node.ID.AsPointer());
                        ImGui::PushStyleColor(ImGuiCol_Button,        ImVec4(0.1f, 0.4f, 0.1f, 0.6f));
                        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.2f, 0.6f, 0.2f, 0.8f));
                        ImGui::PushStyleColor(ImGuiCol_ButtonActive,  ImVec4(0.3f, 0.8f, 0.3f, 1.0f));
                        if (ImGui::SmallButton("+"))
                        {
                            int dynCount = static_cast<int>(node.Inputs.size()) - node.DynamicInputFixedCount;

                            if (node.DefinitionId == "MakeMap")
                            {
                                // MakeMap: 成对添加 Key + Value
                                int pairIdx = dynCount / 2;
                                int totalPairs = static_cast<int>(node.Inputs.size()) / 2;
                                std::string keyName = "Key " + std::to_string(totalPairs);
                                std::string valName = "Value " + std::to_string(totalPairs);
                                node.Inputs.emplace_back(GetNextId(), keyName.c_str(), node.DynamicInputPinType);
                                node.Inputs.emplace_back(GetNextId(), valName.c_str(), node.DynamicInputPinType);
                            }
                            else if (node.DefinitionId == "FormatString")
                            {
                                std::string pinName = "Arg " + std::to_string(dynCount);
                                node.Inputs.emplace_back(GetNextId(), pinName.c_str(), node.DynamicInputPinType);
                            }
                            else if (node.DefinitionId == "MakeArray")
                            {
                                std::string pinName = "Element " + std::to_string(dynCount);
                                node.Inputs.emplace_back(GetNextId(), pinName.c_str(), node.DynamicInputPinType);
                            }
                            else
                            {
                                int idx = static_cast<int>(node.Inputs.size());
                                std::string pinName;
                                do {
                                    pinName = std::string(1, 'A' + (idx % 26)) + pinName;
                                    idx = idx / 26 - 1;
                                } while (idx >= 0);
                                node.Inputs.emplace_back(GetNextId(), pinName.c_str(), node.DynamicInputPinType);
                            }
                            BuildNode(&node);
                            ActiveDoc()->invalidateEditorIndices();  // 引脚指针可能失效，需要重建索引
                            ActiveDoc()->isDirty = true;
                        }
                        ImGui::PopStyleColor(3);
                        ImGui::PopID();
                    }
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

                        if (node.DefinitionId == "MakeMap")
                        {
                            // MakeMap: 成对添加 Key + Value
                            int totalPairs = static_cast<int>(node.Inputs.size()) / 2;
                            std::string keyName = "Key " + std::to_string(totalPairs);
                            std::string valName = "Value " + std::to_string(totalPairs);
                            node.Inputs.emplace_back(GetNextId(), keyName.c_str(), node.DynamicInputPinType);
                            node.Inputs.emplace_back(GetNextId(), valName.c_str(), node.DynamicInputPinType);
                        }
                        else if (node.DefinitionId == "FormatString")
                        {
                            pinName = "Arg " + std::to_string(dynCount);
                            node.Inputs.emplace_back(GetNextId(), pinName.c_str(), node.DynamicInputPinType);
                        }
                        else if (node.DefinitionId == "MakeArray")
                        {
                            pinName = "Element " + std::to_string(dynCount);
                            node.Inputs.emplace_back(GetNextId(), pinName.c_str(), node.DynamicInputPinType);
                        }
                        else
                        {
                            // 默认命名: A, B, C, ... Z, AA, AB, ...
                            int idx = static_cast<int>(node.Inputs.size());
                            do {
                                pinName = std::string(1, 'A' + (idx % 26)) + pinName;
                                idx = idx / 26 - 1;
                            } while (idx >= 0);
                            node.Inputs.emplace_back(GetNextId(), pinName.c_str(), node.DynamicInputPinType);
                        }
                        BuildNode(&node);
                        ActiveDoc()->invalidateEditorIndices();  // 引脚指针可能失效，需要重建索引
                        ActiveDoc()->isDirty = true;
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

                } // if (!node.isCollapsed) else

            builder.End();

            // ---- 错误节点视觉反馈（UE4 风格）----
            if (node.HasError)
            {
                // GetNodeBackgroundDrawList 使用画布坐标，不需要转换到屏幕坐标
                auto drawList = ed::GetNodeBackgroundDrawList(node.ID);
                auto nodePos = ed::GetNodePosition(node.ID);
                auto nodeSize = ed::GetNodeSize(node.ID);
                ImVec2 rectMin = nodePos;
                ImVec2 rectMax = ImVec2(nodePos.x + nodeSize.x, nodePos.y + nodeSize.y);

                // 红色边框
                drawList->AddRect(
                    rectMin - ImVec2(2, 2),
                    rectMax + ImVec2(2, 2),
                    IM_COL32(255, 40, 40, 200), 6.0f, 0, 2.5f);

                // 错误信息：在节点底部渲染红色文字
                if (!node.ErrorMessage.empty())
                {
                    auto textSize = ImGui::CalcTextSize(node.ErrorMessage.c_str());
                    auto textPos = ImVec2(
                        rectMin.x + (rectMax.x - rectMin.x - textSize.x) * 0.5f,
                        rectMax.y + 2.0f);
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

                    // GetNodeBackgroundDrawList 使用画布坐标，不需要转换到屏幕坐标
                    auto drawList = ed::GetNodeBackgroundDrawList(node.ID);
                    auto nodePos = ed::GetNodePosition(node.ID);
                    auto nodeSize = ed::GetNodeSize(node.ID);
                    ImVec2 rectMin = nodePos;
                    ImVec2 rectMax = ImVec2(nodePos.x + nodeSize.x, nodePos.y + nodeSize.y);

                    // 绿色发光边框
                    drawList->AddRect(
                        rectMin - ImVec2(3, 3),
                        rectMax + ImVec2(3, 3),
                        IM_COL32(50, 255, 100, a), 8.0f, 0, 3.0f);
                    // 外层淡光晕
                    drawList->AddRect(
                        rectMin - ImVec2(6, 6),
                        rectMax + ImVec2(6, 6),
                        IM_COL32(50, 255, 100, a / 3), 10.0f, 0, 2.0f);
                }
            }
        }

        // ================================================================
        // Tree 风格节点
        // ================================================================
        for (auto& node : ActiveDoc()->nodes)
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
        for (auto& node : ActiveDoc()->nodes)
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
        // 支持：双击标题进入编辑、Enter/Esc 确认/取消、颜色存储于 node.Color
        // ================================================================
        {
            // 处理双击进入编辑状态
            ed::NodeId dblNode = ed::GetDoubleClickedNode();
            if (dblNode)
            {
                Node* n = FindNode(dblNode);
                if (n && n->Type == NodeType::Comment)
                {
                    ActiveDoc()->editingCommentId = dblNode;
                    snprintf(ActiveDoc()->commentEditBuf,
                             sizeof(ActiveDoc()->commentEditBuf),
                             "%s", n->Name.c_str());
                }
            }
        }

        for (auto& node : ActiveDoc()->nodes)
        {
            if (node.Type != NodeType::Comment)
                continue;

            // 节点背景色：使用 node.Color（默认 #FFFFFF40）
            ImColor bgColor   = ImColor(node.Color.Value.x, node.Color.Value.y,
                                        node.Color.Value.z, 0.25f);
            ImColor bordColor = ImColor(node.Color.Value.x, node.Color.Value.y,
                                        node.Color.Value.z, 0.5f);

            const float commentAlpha = 0.85f;
            ImGui::PushStyleVar(ImGuiStyleVar_Alpha, commentAlpha);
            ed::PushStyleColor(ed::StyleColor_NodeBg,     bgColor);
            ed::PushStyleColor(ed::StyleColor_NodeBorder, bordColor);
            ed::BeginNode(node.ID);
            ImGui::PushID(node.ID.AsPointer());
            ImGui::BeginVertical("content");
            ImGui::BeginHorizontal("horizontal");
            ImGui::Spring(1);

            bool isEditing = (ActiveDoc()->editingCommentId == node.ID);
            if (isEditing)
            {
                // 内联文本编辑框
                ImGui::SetNextItemWidth(std::max(120.0f, node.Size.x - 32.0f));
                ImGui::SetKeyboardFocusHere();
                if (ImGui::InputText("##commentedit",
                                     ActiveDoc()->commentEditBuf,
                                     sizeof(ActiveDoc()->commentEditBuf),
                                     ImGuiInputTextFlags_EnterReturnsTrue |
                                     ImGuiInputTextFlags_AutoSelectAll))
                {
                    // Enter 确认
                    PushUndoState();
                    node.Name = ActiveDoc()->commentEditBuf;
                    ActiveDoc()->isDirty = true;
                    ActiveDoc()->editingCommentId = 0;
                }
                if (ImGui::IsKeyPressed(ImGuiKey_Escape))
                {
                    // Esc 取消
                    ActiveDoc()->editingCommentId = 0;
                }
                // 点击其他地方也确认
                if (!ImGui::IsItemActive() && !ImGui::IsItemFocused() &&
                    ImGui::IsMouseClicked(0))
                {
                    PushUndoState();
                    node.Name = ActiveDoc()->commentEditBuf;
                    ActiveDoc()->isDirty = true;
                    ActiveDoc()->editingCommentId = 0;
                }
                ed::EnableShortcuts(false);
            }
            else
            {
                ImGui::TextUnformatted(node.Name.c_str());
            }

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
                auto min     = ed::GetGroupMin();

                ImGui::SetCursorScreenPos(min - ImVec2(-8, ImGui::GetTextLineHeightWithSpacing() + 4));
                ImGui::BeginGroup();
                ImGui::TextUnformatted(node.Name.c_str());
                ImGui::EndGroup();

                auto drawList        = ed::GetHintBackgroundDrawList();
                auto hintBounds      = ImGui_GetItemRect();
                auto hintFrameBounds = ImRect_Expanded(hintBounds, 8, 4);

                drawList->AddRectFilled(
                    hintFrameBounds.GetTL(), hintFrameBounds.GetBR(),
                    IM_COL32(255, 255, 255, 64 * bgAlpha / 255), 4.0f);
                drawList->AddRect(
                    hintFrameBounds.GetTL(), hintFrameBounds.GetBR(),
                    IM_COL32(255, 255, 255, 128 * bgAlpha / 255), 4.0f);
            }
            ed::EndGroupHint();
        }

        // ================================================================
}
