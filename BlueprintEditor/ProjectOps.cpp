// BlueprintEditor/ProjectOps.cpp
#include "BlueprintEditor.h"
#include "BpLogger.h"
#include "FileDialogs.h"

#include <filesystem>
#include <algorithm>
namespace fs = std::filesystem;

// ============================================================================
// 工程 —— 新建
// ============================================================================

void BlueprintEditor::NewProject()
{
    // 直接弹出系统保存对话框（不再先弹 ImGui 输入框）
    std::string path = SaveFileDialog(
        "Blueprint Project (*.bp.proj)\0*.bp.proj\0",
        "New Blueprint Project",
        "NewProject.bp.proj"
    );
    if (path.empty()) return;

    if (path.size() < 8 || path.substr(path.size() - 8) != ".bp.proj")
        path += ".bp.proj";

    // 从文件名提取工程名
    std::string name;
    {
        std::string fname = fs::path(path).stem().string();
        // 去掉 .bp 后缀
        if (fname.size() > 3 && fname.substr(fname.size() - 3) == ".bp")
            fname = fname.substr(0, fname.size() - 3);
        name = fname.empty() ? "NewProject" : fname;
    }

    CloseProject();
    m_Project = NewBpProject(name);
    m_Project.filePath   = fs::absolute(path).string();
    m_Project.projectDir = fs::path(m_Project.filePath).parent_path().string();
    SaveBpProject(m_Project, m_Project.filePath);
    AddRecentProject(m_Project.filePath);
    BPLOG("Created new project: " + name);
    SetTitle(("Blueprint Editor - [" + name + "]").c_str());
}

// ============================================================================
// 工程 —— 打开
// ============================================================================

void BlueprintEditor::OpenProject()
{
    std::string path = OpenFileDialog(
        "Blueprint Project (*.bp.proj)\0*.bp.proj\0All Files (*.*)\0*.*\0",
        "Open Blueprint Project"
    );
    if (path.empty()) return;

    BpProject proj;
    if (!LoadBpProject(proj, path))
    {
        BPERROR("Failed to open project: " + path);
        if (ActiveDoc())
            ActiveDoc()->executionLog.push_back("[ERROR] Failed to open project: " + path);
        return;
    }

    CloseProject();   // 先关闭旧工程
    m_Project = std::move(proj);

    // 按工程 libraries 重新注册节点定义
    SyncProjectLibrariesToRegistry();
    AddRecentProject(m_Project.filePath);
    SetTitle(("Blueprint Editor - [" + m_Project.name + "]").c_str());
    BPLOG("Opened project: " + m_Project.name + " @ " + m_Project.filePath);
}

// ============================================================================
// 工程 —— 保存
// ============================================================================

void BlueprintEditor::SaveProject()
{
    if (m_Project.filePath.empty())
    {
        SaveProjectAs();
        return;
    }
    SaveBpProject(m_Project, m_Project.filePath);
}

void BlueprintEditor::SaveProjectAs()
{
    std::string path = SaveFileDialog(
        "Blueprint Project (*.bp.proj)\0*.bp.proj\0",
        "Save Blueprint Project",
        (m_Project.name + ".bp.proj").c_str()
    );
    if (path.empty()) return;

    // 确保扩展名
    if (path.size() < 8 || path.substr(path.size() - 8) != ".bp.proj")
        path += ".bp.proj";

    m_Project.filePath  = fs::absolute(path).string();
    m_Project.projectDir = fs::path(m_Project.filePath).parent_path().string();
    SaveBpProject(m_Project, m_Project.filePath);
    AddRecentProject(m_Project.filePath);
    SetTitle(("Blueprint Editor - [" + m_Project.name + "]").c_str());
}

// ============================================================================
// 工程 —— 关闭
// ============================================================================

void BlueprintEditor::CloseProject()
{
    if (!m_Project.IsOpen()) return;
    BPLOG("Closing project: " + m_Project.name);
    m_Project = BpProject{};
    SetTitle("Blueprint Editor - [No Project]");
}

// ============================================================================
// 将当前文档加入工程
// ============================================================================

void BlueprintEditor::AddCurrentDocToProject()
{
    if (!m_Project.IsOpen()) return;
    auto* doc = ActiveDoc();
    if (!doc || doc->filePath.empty()) return;

    BpProjectEntry entry;
    entry.relativePath = m_Project.RelPath(doc->filePath);
    entry.displayName  = doc->GetTabName();

    bool isLib = (doc->blueprintClass == RTBlueprintClass::FunctionLibrary);

    auto& list = isLib ? m_Project.libraries : m_Project.blueprints;
    // 去重
    for (const auto& e : list)
        if (e.relativePath == entry.relativePath) return;

    list.push_back(std::move(entry));
    SaveProject();

    // Library 加入工程后立即同步注册节点定义
    if (isLib)
        SyncProjectLibrariesToRegistry();
}

// ============================================================================
// 按工程 libraries 刷新节点注册表
// ============================================================================

void BlueprintEditor::SyncProjectLibrariesToRegistry()
{
    if (!m_Project.IsOpen()) return;

    // 先清除所有旧的 FuncLib.* 节点定义，防止残留过时的函数显示在菜单里
    {
        std::vector<std::string> toRemove;
        for (const auto* def : m_NodeRegistry.getAllNodeDefinitions())
        {
            if (def->id.rfind("FuncLib.", 0) == 0)
                toRemove.push_back(def->id);
        }
        for (const auto& id : toRemove)
            m_NodeRegistry.unregisterNode(id);
    }

    ::NodeEditor::Runtime::JsonBlueprintExporter exporter;
    int total = 0;
    for (const auto& libEntry : m_Project.libraries)
    {
        std::string absPath = m_Project.AbsPath(libEntry.relativePath);
        if (absPath.empty() || !fs::exists(absPath)) continue;

        auto result = exporter.importRuntimeFromFile(absPath);
        if (!result.success) continue;
        if (result.data.metadata.blueprintClass != RTBlueprintClass::FunctionLibrary) continue;

        int n = ::NodeEditor::Runtime::RegisterLibraryFunctions(m_NodeRegistry, result.data, absPath);
        total += n;
    }
    BPLOG("SyncProjectLibraries: registered " + std::to_string(total) + " functions");

    // 节点定义变更，强制重建缓存
    m_CachedDefCount = 0;
}

// ============================================================================
// 工程面板绘制
// ============================================================================

void BlueprintEditor::DrawProjectPanel()
{
    bool hasProj = m_Project.IsOpen();

    if (!hasProj)
    {
        ImGui::Spacing();
        ImGui::TextDisabled("  No project open.");
        ImGui::Spacing();
        if (ImGui::Button(ICON_FA_DIAGRAM_PROJECT " New Project", ImVec2(-1, 0)))
            NewProject();
        if (ImGui::Button(ICON_FA_FOLDER_OPEN " Open Project", ImVec2(-1, 0)))
            OpenProject();
        return;
    }

    float panelW = ImGui::GetContentRegionAvail().x;

    // ── 工程 Header（VSCode Explorer 风格）────────────────────────────────
    //   [ ▸ PROJECT NAME ]          [+BP] [+Lib] [↻] [⋯]
    {
        auto* dl = ImGui::GetWindowDrawList();
        ImVec2 hdrMin = ImGui::GetCursorScreenPos();
        float hdrH = ImGui::GetTextLineHeight() + 6.0f;

        // 背景条
        dl->AddRectFilled(hdrMin, ImVec2(hdrMin.x + panelW, hdrMin.y + hdrH),
                          IM_COL32(35, 37, 43, 255));

        // 工程名（左侧）
        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 3.0f);
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 6.0f);
        ImGui::TextColored(ImVec4(0.85f, 0.87f, 0.92f, 1.0f),
                           ICON_FA_DIAGRAM_PROJECT "  %s",
                           m_Project.name.empty() ? "(untitled)" : m_Project.name.c_str());

        // 右侧操作按钮（小图标按钮，对齐右边）
        const float btnSz  = hdrH - 2.0f;
        const float btnPad = 2.0f;
        // 4个按钮：New BP, New Lib, Refresh, Add Current
        float btnAreaW = (btnSz + btnPad) * 4.0f;
        ImGui::SameLine(panelW - btnAreaW - 4.0f);
        ImGui::SetCursorPosY(ImGui::GetCursorPosY() - 3.0f);  // 对齐垂直

        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2.0f, 2.0f));
        ImGui::PushStyleColor(ImGuiCol_Button,        IM_COL32(0,0,0,0));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, IM_COL32(0, 122, 204, 60));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive,  IM_COL32(0, 122, 204, 120));

        if (ImGui::Button(ICON_FA_FILE "##newBP", ImVec2(btnSz, btnSz)))
            NewFile(RTBlueprintClass::Actor);
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("New Blueprint");

        ImGui::SameLine(0, btnPad);
        if (ImGui::Button(ICON_FA_CUBE "##newLib", ImVec2(btnSz, btnSz)))
            NewFile(RTBlueprintClass::FunctionLibrary);
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("New Library");

        ImGui::SameLine(0, btnPad);
        if (ImGui::Button(ICON_FA_ARROWS_ROTATE "##refresh", ImVec2(btnSz, btnSz)))
        {
            // 刷新：重新扫描工程目录并同步缺失文件
            if (!m_Project.projectDir.empty())
            {
                for (auto it = m_Project.blueprints.begin(); it != m_Project.blueprints.end();)
                {
                    if (!fs::exists(m_Project.AbsPath(it->relativePath))) it = m_Project.blueprints.erase(it);
                    else ++it;
                }
                for (auto it = m_Project.libraries.begin(); it != m_Project.libraries.end();)
                {
                    if (!fs::exists(m_Project.AbsPath(it->relativePath))) it = m_Project.libraries.erase(it);
                    else ++it;
                }
                SaveProject();
            }
        }
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Refresh (remove missing files)");

        ImGui::SameLine(0, btnPad);
        bool canAdd = ActiveDoc() && !ActiveDoc()->filePath.empty();
        if (!canAdd) ImGui::BeginDisabled();
        if (ImGui::Button(ICON_FA_PLUS "##addCurrent", ImVec2(btnSz, btnSz)))
            AddCurrentDocToProject();
        if (!canAdd) ImGui::EndDisabled();
        if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
            ImGui::SetTooltip(canAdd ? "Add Current File to Project" : "Save the file first");

        ImGui::PopStyleColor(3);
        ImGui::PopStyleVar();

        // 占位：让 Dummy 撑满这一行（防止 TreeNode 抢走光标位置）
        ImGui::Dummy(ImVec2(panelW, 0.0f));
        ImGui::Spacing();
    }

    // ── Section：Blueprints ───────────────────────────────────────────────
    auto drawSection = [&](std::vector<BpProjectEntry>& entries,
                           const char* sectionId,
                           const char* sectionLabel,
                           const char* entryIcon,
                           RTBlueprintClass bpClass)
    {
        // Section header 行：[▶/▼ BLUEPRINTS]  [+ new]
        float sectionH = ImGui::GetTextLineHeight() + 4.0f;
        auto* dl       = ImGui::GetWindowDrawList();
        ImVec2 secMin  = ImGui::GetCursorScreenPos();
        dl->AddRectFilled(secMin, ImVec2(secMin.x + panelW, secMin.y + sectionH),
                          IM_COL32(28, 30, 36, 200));
        dl->AddLine(ImVec2(secMin.x, secMin.y + sectionH - 1),
                    ImVec2(secMin.x + panelW, secMin.y + sectionH - 1),
                    IM_COL32(60, 64, 72, 180));

        // 折叠状态用 TreeNode（渲染成单行 header）
        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 2.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(3.0f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_Header,        IM_COL32(0,0,0,0));
        ImGui::PushStyleColor(ImGuiCol_HeaderHovered, IM_COL32(0,0,0,0));
        ImGui::PushStyleColor(ImGuiCol_HeaderActive,  IM_COL32(0,0,0,0));

        bool open = ImGui::TreeNodeEx(sectionId,
            ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_SpanFullWidth |
            ImGuiTreeNodeFlags_Framed,
            "%s  %s  (%d)", sectionLabel, entryIcon, (int)entries.size());

        // 右侧 new 按钮（浮在 header 右侧）
        float newBtnW = ImGui::CalcTextSize(ICON_FA_PLUS).x + 10.0f;
        ImGui::SameLine(panelW - newBtnW - 4.0f);
        ImGui::PushStyleColor(ImGuiCol_Button,        IM_COL32(0,0,0,0));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, IM_COL32(0, 122, 204, 80));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive,  IM_COL32(0, 122, 204, 150));
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2.0f, 1.0f));
        std::string newBtnId = std::string("##new_") + sectionId;
        if (ImGui::Button((ICON_FA_PLUS + newBtnId).c_str()))
            NewFile(bpClass);
        ImGui::PopStyleVar();
        ImGui::PopStyleColor(3);
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip(bpClass == RTBlueprintClass::Actor ? "New Blueprint" : "New Library");

        ImGui::PopStyleColor(3);
        ImGui::PopStyleVar();

        if (open)
        {
            int removeIdx = -1;
            for (int i = 0; i < (int)entries.size(); ++i)
            {
                auto& e = entries[i];
                ImGui::PushID(i);

                std::string label = e.displayName.empty()
                    ? fs::path(e.relativePath).stem().string()
                    : e.displayName;

                bool isActive = ActiveDoc() && !ActiveDoc()->filePath.empty() &&
                                m_Project.RelPath(ActiveDoc()->filePath) == e.relativePath;

                // 行背景高亮（当前活跃 / hover）
                ImVec2 rowMin = ImGui::GetCursorScreenPos();
                float  rowH   = ImGui::GetTextLineHeightWithSpacing();

                if (isActive)
                    ImGui::GetWindowDrawList()->AddRectFilled(
                        rowMin, ImVec2(rowMin.x + panelW, rowMin.y + rowH),
                        IM_COL32(0, 122, 204, 35));

                if (isActive)
                    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.3f, 1.0f, 0.5f, 1.0f));

                std::string selLabel = std::string("  ") + entryIcon + "  " + label;
                if (ImGui::Selectable(selLabel.c_str(), isActive,
                                      ImGuiSelectableFlags_SpanAllColumns, ImVec2(0, 0)))
                {
                    std::string absPath = m_Project.AbsPath(e.relativePath);
                    std::string normAbs = fs::path(absPath).lexically_normal().string();
                    bool alreadyOpen = false;
                    for (int j = 0; j < (int)m_Documents.size(); ++j)
                    {
                        std::string normDoc = fs::path(m_Documents[j]->filePath).lexically_normal().string();
                        if (normDoc == normAbs)
                        {
                            m_PendingSwitchTabIndex = j;
                            alreadyOpen = true;
                            break;
                        }
                    }
                    if (!alreadyOpen && fs::exists(absPath))
                        DoOpenFile(absPath);
                }
                if (ImGui::IsItemHovered())
                    ImGui::SetTooltip("%s", e.relativePath.c_str());

                if (isActive)
                    ImGui::PopStyleColor();

                // 右键菜单
                if (ImGui::BeginPopupContextItem("##projEntryCtx"))
                {
                    if (ImGui::MenuItem(ICON_FA_XMARK " Remove from Project"))
                        removeIdx = i;
                    ImGui::EndPopup();
                }
                ImGui::PopID();
            }

            if (entries.empty())
            {
                ImGui::TextDisabled("   (empty)");
            }

            if (removeIdx >= 0)
            {
                entries.erase(entries.begin() + removeIdx);
                SaveProject();
            }

            ImGui::TreePop();
        }
        ImGui::Spacing();
    };

    drawSection(m_Project.blueprints,
                "##sec_bp",   "BLUEPRINTS", ICON_FA_FILE,
                RTBlueprintClass::Actor);

    drawSection(m_Project.libraries,
                "##sec_lib",  "LIBRARIES",  ICON_FA_CUBE,
                RTBlueprintClass::FunctionLibrary);
}
