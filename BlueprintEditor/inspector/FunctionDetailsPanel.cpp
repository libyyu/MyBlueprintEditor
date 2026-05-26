// Auto-generated from InspectorPanel.cpp by tools/split_inspector.ps1
// Implements BlueprintEditor::DrawFunctionDetailsPanel + SyncFunctionPinsToNodes (member of the editor class declared in BlueprintEditor.h).
#include "BlueprintEditor.h"
#include "ThemeManager.h"
#include "../Utils/Json/crude_json.h"
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
