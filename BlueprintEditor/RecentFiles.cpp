// RecentFiles.cpp -- Recent Files & Recent Projects history (file menu helpers).
// Extracted from FileOperations.cpp by tools/split_recentfiles.ps1.
//
// Methods (all BlueprintEditor:: members):
//   - AddRecentFile / SaveRecentFiles / LoadRecentFiles / DrawRecentFilesMenu
//   - AddRecentProject / SaveRecentProjects / LoadRecentProjects / DrawRecentProjectsMenu
//
// Local helper:
//   - GetCanonicalPath (static) -- path normalization for duplicate-detection.

#include "BlueprintEditor.h"
#include "PathUtils.h"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <sstream>

namespace fs = std::filesystem;

// Path normalization helper (unify to forward slashes). Used by AddRecent* /
// LoadRecent* / GetCanonicalPath for duplicate detection.
static std::string NormalizePath(const std::string& path)
{
    std::string result = path;
    for (auto& c : result)
    {
        if (c == '\\') c = '/';
    }
    return result;
}

// 辅助函数：获取路径的规范形式用于去重比较
static std::string GetCanonicalPath(const std::string& normalizedPath)
{
    return normalizedPath;
}

void BlueprintEditor::AddRecentFile(const std::string& path)
{
    // 规范化路径
    std::string normalized = NormalizePath(path);
    std::string canonical = GetCanonicalPath(normalized);

    // 移除同一路径的旧记录
    m_RecentFiles.erase(
        std::remove_if(m_RecentFiles.begin(), m_RecentFiles.end(),
            [&canonical](const std::string& existing) {
                return GetCanonicalPath(NormalizePath(existing)) == canonical;
            }),
        m_RecentFiles.end());

    // 插入到最前面（使用规范化后的路径）
    m_RecentFiles.insert(m_RecentFiles.begin(), normalized);

    // 保持最大数量
    while (static_cast<int>(m_RecentFiles.size()) > MaxRecentFiles)
        m_RecentFiles.pop_back();

    // 持久化到磁盘
    SaveRecentFiles();
}

// ============================================================================
// 最近文件列表持久化
// ============================================================================

static const char* kRecentFilesName = "Blueprint Editor.recent.txt";

void BlueprintEditor::SaveRecentFiles()
{
    std::string content;
    for (const auto& path : m_RecentFiles)
        content += path + "\n";
    
    auto fs = ::NodeEditor::Runtime::GetDefaultFileSystem();
    std::string errorMsg;
    fs->WriteFile(kRecentFilesName, content, errorMsg);
}

void BlueprintEditor::LoadRecentFiles()
{
    auto fs = ::NodeEditor::Runtime::GetDefaultFileSystem();
    std::string fileContent;
    std::string errorMsg;
    if (!fs->ReadFile(kRecentFilesName, fileContent, errorMsg))
        return;

    m_RecentFiles.clear();
    std::istringstream iss(fileContent);
    std::string line;
    while (std::getline(iss, line))
    {
        // 去除尾部的 \r（跨平台兼容）
        if (!line.empty() && line.back() == '\r')
            line.pop_back();
        if (line.empty())
            continue;

        // 规范化路径
        std::string normalized = NormalizePath(line);
        std::string canonical = GetCanonicalPath(normalized);

        // 去重检查
        bool duplicate = false;
        for (const auto& existing : m_RecentFiles)
        {
            if (GetCanonicalPath(NormalizePath(existing)) == canonical) { duplicate = true; break; }
        }
        if (!duplicate)
            m_RecentFiles.push_back(normalized);
    }

    // 确保不超过最大数量
    while (static_cast<int>(m_RecentFiles.size()) > MaxRecentFiles)
        m_RecentFiles.pop_back();
}

void BlueprintEditor::DrawRecentFilesMenu()
{
    if (m_RecentFiles.empty())
    {
        ImGui::MenuItem("(No Recent Files)", nullptr, false, false);
        return;
    }

    for (int i = 0; i < static_cast<int>(m_RecentFiles.size()); ++i)
    {
        const auto& path = m_RecentFiles[i];
        // 显示文件名 + 完整路径作为 tooltip
        std::string displayName = BpPath::BaseName(path) + BpPath::Extension(path);
        std::string label = std::to_string(i + 1) + ". " + displayName;

        if (ImGui::MenuItem(label.c_str()))
        {
            // 检查是否已在某个标签页中打开
            bool alreadyOpen = false;
            for (int j = 0; j < static_cast<int>(m_Documents.size()); ++j)
            {
                if (m_Documents[j]->filePath == path)
                {
                    m_ActiveDocIndex = j;
                    ed::SetCurrentEditor(ActiveDoc()->editorContext);
                    ActiveDoc()->needNavigateToContent = 1;
                    alreadyOpen = true;
                    break;
                }
            }
            if (!alreadyOpen)
                DoOpenFile(path);
        }
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("%s", path.c_str());
    }

    ImGui::Separator();
    if (ImGui::MenuItem("Clear Recent Files"))
    {
        m_RecentFiles.clear();
        SaveRecentFiles();  // 同步清空磁盘文件
    }
}

// ============================================================================
// 最近工程列表
// ============================================================================

static const char* kRecentProjectsName = "Blueprint Editor.recent_projects.txt";

void BlueprintEditor::AddRecentProject(const std::string& path)
{
    std::string normalized = NormalizePath(path);
    m_RecentProjects.erase(
        std::remove_if(m_RecentProjects.begin(), m_RecentProjects.end(),
            [&normalized](const std::string& e) {
                return NormalizePath(e) == normalized;
            }),
        m_RecentProjects.end());
    m_RecentProjects.insert(m_RecentProjects.begin(), normalized);
    while (static_cast<int>(m_RecentProjects.size()) > MaxRecentProjects)
        m_RecentProjects.pop_back();
    SaveRecentProjects();
}

void BlueprintEditor::SaveRecentProjects()
{
    std::string content;
    for (const auto& p : m_RecentProjects)
        content += p + "\n";
    auto fs = ::NodeEditor::Runtime::GetDefaultFileSystem();
    std::string err;
    fs->WriteFile(kRecentProjectsName, content, err);
}

void BlueprintEditor::LoadRecentProjects()
{
    auto fs = ::NodeEditor::Runtime::GetDefaultFileSystem();
    std::string fileContent, err;
    if (!fs->ReadFile(kRecentProjectsName, fileContent, err)) return;

    m_RecentProjects.clear();
    std::istringstream iss(fileContent);
    std::string line;
    while (std::getline(iss, line))
    {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        if (line.empty()) continue;
        std::string normalized = NormalizePath(line);
        bool dup = false;
        for (const auto& e : m_RecentProjects)
            if (NormalizePath(e) == normalized) { dup = true; break; }
        if (!dup)
            m_RecentProjects.push_back(normalized);
    }
    while (static_cast<int>(m_RecentProjects.size()) > MaxRecentProjects)
        m_RecentProjects.pop_back();
}

void BlueprintEditor::DrawRecentProjectsMenu()
{
    // 过滤掉当前已打开的工程
    std::string currentProjPath = NormalizePath(m_Project.filePath);

    // 收集显示列表（排除当前工程）
    std::vector<std::string> display;
    for (const auto& p : m_RecentProjects)
        if (NormalizePath(p) != currentProjPath)
            display.push_back(p);

    if (display.empty())
    {
        ImGui::MenuItem("(No Recent Projects)", nullptr, false, false);
        return;
    }

    for (int i = 0; i < static_cast<int>(display.size()); ++i)
    {
        const auto& path = display[i];
        std::string name = BpPath::Stem(path);  // 去掉目录和 .bproj 扩展名

        std::string label = std::to_string(i + 1) + ". " + name;
        if (ImGui::MenuItem(label.c_str()))
        {
            BpProject proj;
            if (LoadBpProject(proj, path))
            {
                CloseProject();
                m_Project = std::move(proj);
                SyncProjectLibrariesToRegistry();
                SetTitle(("Blueprint Editor - [" + m_Project.name + "]").c_str());
                AddRecentProject(m_Project.filePath);
            }
        }
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("%s", path.c_str());
    }

    ImGui::Separator();
    if (ImGui::MenuItem("Clear Recent Projects"))
    {
        m_RecentProjects.clear();
        SaveRecentProjects();
    }
}
