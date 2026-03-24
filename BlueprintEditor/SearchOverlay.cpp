// SearchOverlay.cpp -- 画布上的节点搜索覆盖层（Ctrl+F）
#include "BlueprintEditor.h"
#include <algorithm>
#include <cctype>

// ============================================================================
// 打开搜索覆盖层
// ============================================================================

void BlueprintEditor::OpenSearchOverlay()
{
    m_ShowSearchOverlay = true;
    m_SearchResultIndex = -1;
    // 不清空搜索内容，方便连续搜索
}

// ============================================================================
// 更新搜索结果
// ============================================================================

void BlueprintEditor::UpdateSearchResults()
{
    m_SearchResults.clear();

    if (!ActiveDoc()) return;

    std::string filter(m_SearchBuffer);
    if (filter.empty()) return;

    // 转小写进行大小写不敏感匹配
    std::string lowerFilter = filter;
    for (auto& c : lowerFilter) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));

    for (const auto& node : m_Nodes)
    {
        // 匹配节点名
        std::string lowerName = node.Name;
        for (auto& c : lowerName) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));

        bool match = false;
        if (lowerName.find(lowerFilter) != std::string::npos)
            match = true;

        // 匹配定义 ID
        if (!match)
        {
            std::string lowerDefId = node.DefinitionId;
            for (auto& c : lowerDefId) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
            if (lowerDefId.find(lowerFilter) != std::string::npos)
                match = true;
        }

        // 匹配引脚名
        if (!match)
        {
            for (const auto& pin : node.Inputs)
            {
                std::string lp = pin.Name;
                for (auto& c : lp) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
                if (lp.find(lowerFilter) != std::string::npos) { match = true; break; }

                // 匹配引脚字符串值
                if (pin.Type == PinType::String)
                {
                    std::string lv = pin.StringValue;
                    for (auto& c : lv) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
                    if (lv.find(lowerFilter) != std::string::npos) { match = true; break; }
                }
            }
        }
        if (!match)
        {
            for (const auto& pin : node.Outputs)
            {
                std::string lp = pin.Name;
                for (auto& c : lp) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
                if (lp.find(lowerFilter) != std::string::npos) { match = true; break; }
            }
        }

        if (match)
            m_SearchResults.push_back(node.ID);
    }
}

// ============================================================================
// 导航到搜索结果
// ============================================================================

void BlueprintEditor::NavigateToSearchResult(int index)
{
    if (index < 0 || index >= static_cast<int>(m_SearchResults.size()))
        return;

    m_SearchResultIndex = index;
    ed::NodeId nodeId = m_SearchResults[index];

    // 选中并导航到该节点
    ed::ClearSelection();
    ed::SelectNode(nodeId, false);
    ed::NavigateToSelection();
}

// ============================================================================
// 绘制搜索覆盖层
// ============================================================================

void BlueprintEditor::DrawSearchOverlay()
{
    if (!m_ShowSearchOverlay) return;

    auto& io = ImGui::GetIO();

    // ESC 关闭搜索
    if (ImGui::IsKeyPressed(ImGuiKey_Escape))
    {
        m_ShowSearchOverlay = false;
        return;
    }

    // 固定在编辑器区域右上角
    ImVec2 overlayPos = ImGui::GetWindowPos();
    ImVec2 windowSize = ImGui::GetWindowSize();
    float overlayWidth = 320.0f;
    float overlayX = overlayPos.x + windowSize.x - overlayWidth - 20.0f;
    float overlayY = overlayPos.y + 60.0f;  // 菜单栏和标签栏下方

    ImGui::SetNextWindowPos(ImVec2(overlayX, overlayY), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(overlayWidth, 0), ImGuiCond_Always);
    ImGui::SetNextWindowBgAlpha(0.94f);

    ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar |
                             ImGuiWindowFlags_NoResize |
                             ImGuiWindowFlags_NoMove |
                             ImGuiWindowFlags_NoScrollbar |
                             ImGuiWindowFlags_AlwaysAutoResize |
                             ImGuiWindowFlags_NoSavedSettings;

    static bool s_NeedFocus = false;
    static bool s_WasShowing = false;

    // 检测刚打开
    if (m_ShowSearchOverlay && !s_WasShowing)
        s_NeedFocus = true;
    s_WasShowing = m_ShowSearchOverlay;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 8.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(10, 10));
    ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.22f, 0.32f, 0.48f, 0.50f));

    if (ImGui::Begin("##SearchOverlay", &m_ShowSearchOverlay, flags))
    {
        // 标题行：搜索图标 + 标题 + 关闭按钮
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.45f, 0.70f, 0.95f, 1.00f));
        ImGui::TextUnformatted(ICON_FA_MAGNIFYING_GLASS);
        ImGui::PopStyleColor();
        ImGui::SameLine();
        ImGui::TextColored(ImVec4(0.80f, 0.85f, 0.95f, 1.00f), "Search Nodes");
        ImGui::SameLine();
        ImGui::TextColored(ImVec4(0.40f, 0.42f, 0.48f, 1.00f), "(Ctrl+F)");
        ImGui::SameLine(overlayWidth - 30.0f);
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.5f, 0.2f, 0.2f, 0.5f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.7f, 0.2f, 0.2f, 0.7f));
        if (ImGui::SmallButton(ICON_FA_XMARK))
            m_ShowSearchOverlay = false;
        ImGui::PopStyleColor(3);

        ImGui::Spacing();
        ImGui::SetNextItemWidth(overlayWidth - 16.0f);

        // 首次打开时聚焦输入框
        if (s_NeedFocus)
        {
            ImGui::SetKeyboardFocusHere();
            s_NeedFocus = false;
        }

        bool changed = ImGui::InputText("##SearchInput", m_SearchBuffer, sizeof(m_SearchBuffer),
                                         ImGuiInputTextFlags_AutoSelectAll);

        if (changed)
        {
            UpdateSearchResults();
            if (!m_SearchResults.empty())
                NavigateToSearchResult(0);
            else
                m_SearchResultIndex = -1;
        }

        // Enter 跳转到下一个结果, Shift+Enter 跳转到上一个
        if (ImGui::IsKeyPressed(ImGuiKey_Enter) || ImGui::IsKeyPressed(ImGuiKey_KeypadEnter))
        {
            if (!m_SearchResults.empty())
            {
                int count = static_cast<int>(m_SearchResults.size());
                int nextIdx;
                if (io.KeyShift)
                    nextIdx = (m_SearchResultIndex - 1 + count) % count;
                else
                    nextIdx = (m_SearchResultIndex + 1) % count;
                NavigateToSearchResult(nextIdx);
            }
        }

        // 显示结果统计
        if (m_SearchBuffer[0] != '\0')
        {
            ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f),
                               "%d result(s)", static_cast<int>(m_SearchResults.size()));

            if (m_SearchResultIndex >= 0)
            {
                ImGui::SameLine();
                ImGui::TextColored(ImVec4(0.9f, 0.9f, 0.4f, 1.0f),
                                   " [%d/%d]", m_SearchResultIndex + 1, static_cast<int>(m_SearchResults.size()));
            }
        }

        // 结果列表（最多显示 8 个）
        int maxVisible = 8;
        int count = static_cast<int>(m_SearchResults.size());
        if (count > 0)
        {
            ImGui::Separator();
            for (int i = 0; i < count && i < maxVisible; ++i)
            {
                auto* node = FindNode(m_SearchResults[i]);
                if (!node) continue;

                bool isSelected = (i == m_SearchResultIndex);

                // 节点类型彩色小标签
                ImVec4 typeColor;
                const char* typeLabel;
                switch (node->Type)
                {
                    case NodeType::Blueprint: typeColor = ImVec4(0.26f, 0.46f, 0.72f, 1.0f); typeLabel = "BP";   break;
                    case NodeType::Simple:    typeColor = ImVec4(0.35f, 0.65f, 0.35f, 1.0f); typeLabel = "S";    break;
                    case NodeType::Tree:      typeColor = ImVec4(0.65f, 0.50f, 0.25f, 1.0f); typeLabel = "T";    break;
                    case NodeType::Houdini:   typeColor = ImVec4(0.75f, 0.30f, 0.30f, 1.0f); typeLabel = "H";    break;
                    case NodeType::Comment:   typeColor = ImVec4(0.50f, 0.50f, 0.50f, 1.0f); typeLabel = "C";    break;
                    default:                  typeColor = ImVec4(0.50f, 0.50f, 0.60f, 1.0f); typeLabel = "?";    break;
                }

                // 绘制类型标签背景
                ImVec2 labelSize = ImGui::CalcTextSize(typeLabel);
                ImVec2 curPos = ImGui::GetCursorScreenPos();
                float badgePadX = 4.0f;
                float badgePadY = 1.0f;
                float badgeW = labelSize.x + badgePadX * 2;
                float badgeH = labelSize.y + badgePadY * 2;

                auto* dl = ImGui::GetWindowDrawList();
                ImVec2 badgeMin = ImVec2(curPos.x, curPos.y + 1.0f);
                ImVec2 badgeMax = ImVec2(curPos.x + badgeW, curPos.y + badgeH + 1.0f);
                dl->AddRectFilled(badgeMin, badgeMax,
                    ImGui::ColorConvertFloat4ToU32(ImVec4(typeColor.x, typeColor.y, typeColor.z, 0.75f)),
                    3.0f);
                dl->AddText(ImVec2(curPos.x + badgePadX, curPos.y + badgePadY + 1.0f),
                    IM_COL32(255, 255, 255, 220), typeLabel);

                ImGui::Dummy(ImVec2(badgeW + 4.0f, badgeH));
                ImGui::SameLine();

                // 节点名
                if (isSelected)
                    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 0.3f, 1.0f));

                std::string label = node->Name + "##sr" + std::to_string(i);
                if (ImGui::Selectable(label.c_str(), isSelected))
                    NavigateToSearchResult(i);

                if (isSelected)
                    ImGui::PopStyleColor();
            }

            if (count > maxVisible)
            {
                ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f),
                                   "... and %d more", count - maxVisible);
            }
        }
    }
    ImGui::End();
    ImGui::PopStyleColor();
    ImGui::PopStyleVar(2);
}
