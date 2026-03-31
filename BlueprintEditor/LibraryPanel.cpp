// LibraryPanel.cpp -- 计时器监控面板 + 节点库面板
// 从 EditorUI.cpp 拆分而来
#include "BlueprintEditor.h"

// ============================================================================
// 计时器监控浮动面板
// ============================================================================

void BlueprintEditor::DrawTimerPanel()
{
    ImGui::SetNextWindowSize(ImVec2(520, 340), ImGuiCond_FirstUseEver);
    if (!ImGui::Begin(ICON_FA_STOPWATCH " Timer Monitor", &m_ShowTimerWindow))
    {
        ImGui::End();
        return;
    }

    auto& timerMgr = ActiveDoc()->GetTimerManager();
    ImGui::Text("Active Timers: %d", timerMgr.GetActiveTimerCount());
    ImGui::SameLine();
    ImGui::Text("  |  Time Scale: ");
    ImGui::SameLine();
    float ts = timerMgr.GetTimeScale();
    ImGui::SetNextItemWidth(100);
    if (ImGui::SliderFloat("##TimeScale", &ts, 0.0f, 5.0f, "%.2f"))
        timerMgr.SetTimeScale(ts);

    ImGui::SameLine(0, 20);
    if (ImGui::Button(ICON_FA_ERASER " Clear All"))
        timerMgr.ClearAllTimers();
    ImGui::SameLine();
    if (ImGui::Button(ICON_FA_PAUSE " Pause All"))
        timerMgr.PauseAll();
    ImGui::SameLine();
    if (ImGui::Button(ICON_FA_PLAY " Resume All"))
        timerMgr.ResumeAll();

    ImGui::Separator();

    static float testInterval = 1.0f;
    static int   testRepeat   = -1;
    static char  testName[64] = "Test";
    ImGui::Text("Quick Timer:");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(80);
    ImGui::InputText("##Name", testName, sizeof(testName));
    ImGui::SameLine();
    ImGui::SetNextItemWidth(60);
    ImGui::InputFloat("##Interval", &testInterval, 0.0f, 0.0f, "%.1fs");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(50);
    ImGui::InputInt("##Repeat", &testRepeat);
    ImGui::SameLine();
    if (ImGui::Button("Add"))
    {
        std::string timerName(testName);
        ActiveDoc()->GetTimerManager().SetTimerByName(timerName, testInterval, testRepeat, [this, timerName]() {
            ActiveDoc()->executionLog.push_back("[Timer:" + timerName + "] fired!");
            ActiveDoc()->executionLogDirty = true;
            return true;
        });
    }

    ImGui::Separator();

    const auto& timers = timerMgr.GetAllTimers();
    if (timers.empty())
    {
        ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "No active timers");
    }
    else
    {
        if (ImGui::BeginTable("##Timers", 7, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable | ImGuiTableFlags_ScrollY))
        {
            ImGui::TableSetupColumn("Handle",   ImGuiTableColumnFlags_WidthFixed, 50.0f);
            ImGui::TableSetupColumn("Name",     ImGuiTableColumnFlags_WidthFixed, 80.0f);
            ImGui::TableSetupColumn("Interval", ImGuiTableColumnFlags_WidthFixed, 60.0f);
            ImGui::TableSetupColumn("Remaining",ImGuiTableColumnFlags_WidthFixed, 70.0f);
            ImGui::TableSetupColumn("Fired",    ImGuiTableColumnFlags_WidthFixed, 45.0f);
            ImGui::TableSetupColumn("State",    ImGuiTableColumnFlags_WidthFixed, 60.0f);
            ImGui::TableSetupColumn("Actions",  ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableHeadersRow();

            for (const auto& t : timers)
            {
                if (t.pendingKill) continue;

                ImGui::TableNextRow();

                ImGui::TableNextColumn();
                ImGui::Text("#%u", t.handle);

                ImGui::TableNextColumn();
                ImGui::TextUnformatted(t.name.empty() ? "-" : t.name.c_str());

                ImGui::TableNextColumn();
                ImGui::Text("%.2fs", t.interval);

                ImGui::TableNextColumn();
                float pct = t.interval > 0.0f ? (t.remaining / t.interval) : 0.0f;
                if (pct < 0.0f) pct = 0.0f;
                if (pct > 1.0f) pct = 1.0f;
                char overlay[32];
                snprintf(overlay, sizeof(overlay), "%.2fs", t.remaining > 0.0f ? t.remaining : 0.0f);
                ImGui::ProgressBar(pct, ImVec2(-1, 0), overlay);

                ImGui::TableNextColumn();
                ImGui::Text("%d", t.fireCount);

                ImGui::TableNextColumn();
                if (t.paused)
                    ImGui::TextColored(ImVec4(1.0f, 0.6f, 0.0f, 1.0f), "Paused");
                else if (t.repeatCount == -1)
                    ImGui::TextColored(ImVec4(0.2f, 0.8f, 0.2f, 1.0f), "Loop");
                else
                    ImGui::TextColored(ImVec4(0.4f, 0.7f, 1.0f, 1.0f), "x%d", t.repeatCount);

                ImGui::TableNextColumn();
                ImGui::PushID(t.handle);
                if (t.paused)
                {
                    if (ImGui::SmallButton(ICON_FA_PLAY " Resume"))
                        timerMgr.ResumeTimer(t.handle);
                }
                else
                {
                    if (ImGui::SmallButton(ICON_FA_PAUSE " Pause"))
                        timerMgr.PauseTimer(t.handle);
                }
                ImGui::SameLine();
                if (ImGui::SmallButton(ICON_FA_XMARK))
                    timerMgr.ClearTimer(t.handle);
                ImGui::PopID();
            }

            ImGui::EndTable();
        }
    }

    ImGui::End();
}

// ============================================================================
// 节点库面板（Library Tab）— 折叠分类 + 拖拽到画布
// ============================================================================

static ImVec4 GetCategoryDotColor(const std::string& cat)
{
    if (cat.find("Flow")       == 0) return {0.20f, 0.39f, 0.86f, 1.f};
    if (cat.find("Math")       == 0) return {0.24f, 0.71f, 0.31f, 1.f};
    if (cat.find("String")     == 0) return {0.71f, 0.31f, 0.78f, 1.f};
    if (cat.find("Debug")      == 0) return {0.78f, 0.24f, 0.24f, 1.f};
    if (cat.find("Action")     == 0 ||
        cat.find("Event")      == 0) return {0.86f, 0.39f, 0.16f, 1.f};
    if (cat.find("Conversion") == 0) return {0.31f, 0.63f, 0.86f, 1.f};
    if (cat.find("Array")      == 0) return {0.78f, 0.59f, 0.12f, 1.f};
    if (cat.find("Misc/Map")   == 0 ||
        cat.find("Map")        == 0) return {0.16f, 0.71f, 0.78f, 1.f};
    if (cat.find("Custom")     == 0) return {0.24f, 0.71f, 0.51f, 1.f};
    return {0.39f, 0.39f, 0.47f, 1.f};
}

void BlueprintEditor::DrawNodeLibraryPanel()
{
    float paneWidth = ImGui::GetContentRegionAvail().x;

    static char libSearchBuf[128] = "";
    ImGui::SetNextItemWidth(paneWidth);
    ImGui::InputTextWithHint("##LibSearch", ICON_FA_MAGNIFYING_GLASS " Search...", libSearchBuf, sizeof(libSearchBuf));

    std::string searchStr(libSearchBuf);
    std::string searchLower = searchStr;
    for (auto& c : searchLower)
        c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));

    ImGui::Spacing();

    const auto& allDefs = m_NodeRegistry.getAllNodeDefinitions();

    static size_t s_libCachedDefCount = 0;
    static std::string s_libCachedSearch;
    static std::map<std::string, std::vector<const RTNodeDef*>> s_libCatMap;

    bool needRebuild = (allDefs.size() != s_libCachedDefCount) || (searchLower != s_libCachedSearch);
    if (needRebuild)
    {
        s_libCachedDefCount = allDefs.size();
        s_libCachedSearch = searchLower;
        s_libCatMap.clear();

        for (const auto* d : allDefs)
        {
            if (d->isAbstract) continue;
            if (!searchLower.empty())
            {
                std::string nameLower = d->name;
                for (auto& c : nameLower) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
                std::string idLower = d->id;
                for (auto& c : idLower) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
                if (nameLower.find(searchLower) == std::string::npos &&
                    idLower.find(searchLower) == std::string::npos &&
                    d->category.find(searchStr) == std::string::npos)
                    continue;
            }
            s_libCatMap[d->category.empty() ? "Misc" : d->category].push_back(d);
        }
    }

    const auto& catMap = s_libCatMap;

    static std::unordered_map<std::string, bool> catOpenState;

    for (const auto& [cat, nodes] : catMap)
    {
        if (catOpenState.find(cat) == catOpenState.end())
            catOpenState[cat] = (cat == "Flow" || cat == "Math" || cat == "String");

        ImVec4 dotCol = GetCategoryDotColor(cat);
        ImGui::PushStyleColor(ImGuiCol_Header,        ImVec4(dotCol.x*0.35f, dotCol.y*0.35f, dotCol.z*0.35f, 0.80f));
        ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(dotCol.x*0.50f, dotCol.y*0.50f, dotCol.z*0.50f, 0.90f));
        ImGui::PushStyleColor(ImGuiCol_HeaderActive,  ImVec4(dotCol.x*0.65f, dotCol.y*0.65f, dotCol.z*0.65f, 1.00f));

        ImGui::SetNextItemOpen(catOpenState[cat], ImGuiCond_Once);
        bool open = ImGui::CollapsingHeader(
            (cat + " (" + std::to_string(nodes.size()) + ")").c_str(),
            ImGuiTreeNodeFlags_None);
        catOpenState[cat] = open;

        ImGui::PopStyleColor(3);

        if (!open) continue;

        ImGui::Indent(8.0f);
        for (const auto* d : nodes)
        {
            ImGui::PushID(d->id.c_str());

            ImVec2 squareMin = ImGui::GetCursorScreenPos();
            squareMin.y += (ImGui::GetTextLineHeight() - 8.0f) * 0.5f;
            ImGui::GetWindowDrawList()->AddRectFilled(
                squareMin,
                ImVec2(squareMin.x + 8.0f, squareMin.y + 8.0f),
                ImGui::ColorConvertFloat4ToU32(dotCol), 2.0f);
            ImGui::Dummy(ImVec2(10.0f, ImGui::GetTextLineHeight()));
            ImGui::SameLine(0, 2.0f);

            if (!searchLower.empty())
            {
                std::string nameLower = d->name;
                for (auto& c : nameLower)
                    c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
                size_t pos = nameLower.find(searchLower);
                if (pos != std::string::npos)
                {
                    ImGui::TextUnformatted(d->name.substr(0, pos).c_str());
                    ImGui::SameLine(0, 0);
                    ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.2f, 1.0f),
                        "%s", d->name.substr(pos, searchLower.size()).c_str());
                    ImGui::SameLine(0, 0);
                    ImGui::TextUnformatted(d->name.substr(pos + searchLower.size()).c_str());
                }
                else
                {
                    ImGui::TextUnformatted(d->name.c_str());
                }
            }
            else
            {
                ImGui::TextUnformatted(d->name.c_str());
            }

            if (ImGui::IsItemHovered())
            {
                ImGui::SetTooltip("ID: %s\nCategory: %s%s",
                    d->id.c_str(), d->category.c_str(),
                    d->description.empty() ? "" : ("\n" + d->description).c_str());
            }

            if (ImGui::IsItemActive() || ImGui::IsItemHovered())
            {
                if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID))
                {
                    ImGui::SetDragDropPayload("BP_NODE_DEF",
                        d->id.c_str(),
                        d->id.size() + 1);
                    ImGui::TextColored(dotCol, "%s", d->name.c_str());
                    ImGui::TextDisabled("Drop on canvas to create");
                    ImGui::EndDragDropSource();
                }
            }

            ImGui::PopID();
        }
        ImGui::Unindent(8.0f);
        ImGui::Spacing();
    }
}
