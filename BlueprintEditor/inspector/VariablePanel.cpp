// Auto-generated from InspectorPanel.cpp by tools/split_inspector.ps1
// Implements BlueprintEditor::DrawVariablePanel (member of the editor class declared in BlueprintEditor.h).
#include "BlueprintEditor.h"
#include "ThemeManager.h"
#include "../Utils/Json/crude_json.h"
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
