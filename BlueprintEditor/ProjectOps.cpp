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
    m_ShowNewProjectDialog = true;
    memset(m_NewProjNameBuf, 0, sizeof(m_NewProjNameBuf));
    snprintf(m_NewProjNameBuf, sizeof(m_NewProjNameBuf), "%s", "NewProject");
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

    // 清理已有 FunctionLibrary 前缀的节点定义（避免重复注册）
    // 注：DefaultNodeRegistry 目前没有 unregister，这里只做 re-register（重复 id 会覆盖）
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
    m_CachedDefCount = SIZE_MAX;
}

// ============================================================================
// 工程面板绘制
// ============================================================================

void BlueprintEditor::DrawProjectPanel()
{
    bool hasProj = m_Project.IsOpen();

    if (!hasProj)
    {
        ImGui::TextDisabled("No project open.");
        ImGui::Spacing();
        if (ImGui::Button(ICON_FA_DIAGRAM_PROJECT " New Project", ImVec2(-1, 0)))
            NewProject();
        if (ImGui::Button(ICON_FA_FOLDER_OPEN " Open Project", ImVec2(-1, 0)))
            OpenProject();
        return;
    }

    // ── 工程标题 ─────────────────────────────────────────────────────────
    ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f),
                       ICON_FA_DIAGRAM_PROJECT " %s", m_Project.name.c_str());
    ImGui::TextDisabled("%s", m_Project.filePath.c_str());
    ImGui::Separator();

    auto drawEntryList = [&](std::vector<BpProjectEntry>& entries,
                              const char* sectionLabel,
                              RTBlueprintClass bpClass)
    {
        if (ImGui::TreeNodeEx(sectionLabel, ImGuiTreeNodeFlags_DefaultOpen))
        {
            int removeIdx = -1;
            for (int i = 0; i < (int)entries.size(); ++i)
            {
                auto& e = entries[i];
                ImGui::PushID(i);

                const char* icon = (bpClass == RTBlueprintClass::FunctionLibrary)
                                   ? ICON_FA_CUBE : ICON_FA_FILE;
                std::string label = e.displayName.empty()
                    ? fs::path(e.relativePath).stem().string()
                    : e.displayName;

                bool isActive = ActiveDoc() && !ActiveDoc()->filePath.empty() &&
                                m_Project.RelPath(ActiveDoc()->filePath) == e.relativePath;

                if (isActive)
                    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.3f, 1.0f, 0.5f, 1.0f));

                if (ImGui::Selectable((std::string(icon) + " " + label).c_str(), isActive))
                {
                    // 点击打开对应文件
                    std::string absPath = m_Project.AbsPath(e.relativePath);
                    if (fs::exists(absPath))
                        DoOpenFile(absPath);
                }
                if (ImGui::IsItemHovered())
                    ImGui::SetTooltip("%s", e.relativePath.c_str());

                if (isActive)
                    ImGui::PopStyleColor();

                // 右键：从工程移除
                if (ImGui::BeginPopupContextItem("##projEntryCtx"))
                {
                    if (ImGui::MenuItem(ICON_FA_XMARK " Remove from Project"))
                        removeIdx = i;
                    ImGui::EndPopup();
                }
                ImGui::PopID();
            }
            if (removeIdx >= 0)
            {
                entries.erase(entries.begin() + removeIdx);
                SaveProject();
            }

            // 加号按钮：新建对应类型
            ImGui::Spacing();
            if (bpClass == RTBlueprintClass::Actor)
            {
                if (ImGui::SmallButton(ICON_FA_PLUS " New Blueprint"))
                    NewFile(RTBlueprintClass::Actor);
            }
            else
            {
                if (ImGui::SmallButton(ICON_FA_PLUS " New Library"))
                    NewFile(RTBlueprintClass::FunctionLibrary);
            }

            ImGui::TreePop();
        }
    };

    drawEntryList(m_Project.blueprints, ICON_FA_FILE "  Blueprints", RTBlueprintClass::Actor);
    ImGui::Spacing();
    drawEntryList(m_Project.libraries, ICON_FA_CUBE "  Libraries", RTBlueprintClass::FunctionLibrary);

    ImGui::Separator();
    ImGui::Spacing();

    // 当前文档加入工程
    auto* doc = ActiveDoc();
    bool canAdd = doc && !doc->filePath.empty() && m_Project.IsOpen();
    if (!canAdd) ImGui::BeginDisabled();
    if (ImGui::Button(ICON_FA_PLUS " Add Current to Project", ImVec2(-1, 0)))
        AddCurrentDocToProject();
    if (!canAdd) ImGui::EndDisabled();
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled) && !canAdd)
        ImGui::SetTooltip("Save the current document first.");
}
