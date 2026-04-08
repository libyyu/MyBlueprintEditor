// Dialogs.cpp -- 编辑器对话框（未保存确认 / 命名保存）
// 从 EditorUI.cpp 拆分而来
#include "BlueprintEditor.h"
#include "FileDialogs.h"
#include <filesystem>

// ============================================================================
// 未保存修改确认对话框
// ============================================================================

void BlueprintEditor::ShowUnsavedChangesDialog()
{
    // OpenPopup 只在触发帧调用一次；BeginPopupModal 每帧都必须调用（ImGui 内部状态机要求）
    if (m_ShowUnsavedDialog)
    {
        ImGui::OpenPopup("Unsaved Changes###UnsavedDlg");
        m_ShowUnsavedDialog = false;
    }

    ImVec2 center = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

    if (ImGui::BeginPopupModal("Unsaved Changes###UnsavedDlg", nullptr,
        ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings))
    {
        std::string docName = "New";
        if (m_PendingCloseTabIndex >= 0 && m_PendingCloseTabIndex < static_cast<int>(m_Documents.size()))
            docName = m_Documents[m_PendingCloseTabIndex]->GetTabName();

        ImGui::Text("Document \"%s\" has unsaved changes.", docName.c_str());
        ImGui::Text("Do you want to save before closing?");
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        float buttonWidth = 100.0f;
        float totalWidth = buttonWidth * 3 + ImGui::GetStyle().ItemSpacing.x * 2;
        float startX = (ImGui::GetContentRegionAvail().x - totalWidth) * 0.5f;
        if (startX > 0) ImGui::SetCursorPosX(ImGui::GetCursorPosX() + startX);

        if (ImGui::Button(ICON_FA_FLOPPY_DISK " Save", ImVec2(buttonWidth, 0)))
        {
            if (m_PendingCloseTabIndex >= 0)
            {
                int oldActiveIdx = m_ActiveDocIndex;
                m_ActiveDocIndex = m_PendingCloseTabIndex;
                ed::SetCurrentEditor(ActiveDoc()->editorContext);
                SaveFile();
                CloseDocument(m_PendingCloseTabIndex);
                m_PendingCloseTabIndex = -1;
                if (oldActiveIdx >= static_cast<int>(m_Documents.size()))
                    m_ActiveDocIndex = static_cast<int>(m_Documents.size()) - 1;
            }
            ImGui::CloseCurrentPopup();
        }

        ImGui::SameLine();
        if (ImGui::Button(ICON_FA_TRASH_CAN " Don't Save", ImVec2(buttonWidth, 0)))
        {
            if (m_PendingCloseTabIndex >= 0)
            {
                CloseDocument(m_PendingCloseTabIndex);
                m_PendingCloseTabIndex = -1;
            }
            ImGui::CloseCurrentPopup();
        }

        ImGui::SameLine();
        if (ImGui::Button(ICON_FA_XMARK " Cancel", ImVec2(buttonWidth, 0)))
        {
            m_PendingCloseTabIndex = -1;
            m_PendingQuitApp = false;
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }
}

// ============================================================================
// 工程内命名对话框 — DrawSaveNameDialog
// 在有工程时替代系统文件对话框，让用户输入相对于 assets/ 的文件路径（含子目录）
// ============================================================================

void BlueprintEditor::DrawSaveNameDialog()
{
    auto& d = m_SaveNameDialog;

    if (d.open)
    {
        d.open = false;
        ImGui::OpenPopup("###SaveNameDlg");
        ImVec2 center = ImGui::GetMainViewport()->GetCenter();
        ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    }

    const char* title = d.isNew
        ? (ICON_FA_FILE " New Blueprint###SaveNameDlg")
        : (ICON_FA_FLOPPY_DISK " Save Blueprint As###SaveNameDlg");

    float dlgW = ImGui::CalcTextSize("File path (relative to assets/, subdirs OK, no extension):").x
                 + ImGui::GetStyle().WindowPadding.x * 2.0f + 16.0f;
    if (dlgW < 420.0f) dlgW = 420.0f;
    ImGui::SetNextWindowSize(ImVec2(dlgW, 0), ImGuiCond_Always);
    if (!ImGui::BeginPopupModal(title, nullptr,
        ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings))
        return;

    // ── 头部信息 ────────────────────────────────────────────────────────────
    ImGui::TextDisabled("Project: %s", m_Project.name.c_str());
    {
        namespace fs = std::filesystem;
        std::string assetsDir = (fs::path(m_Project.projectDir) / "assets").string();
        ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled));
        ImGui::TextWrapped("Root: %s", assetsDir.c_str());
        ImGui::PopStyleColor();
    }
    ImGui::Spacing();

    const char* typeLabel = (d.bpClass == RTBlueprintClass::FunctionLibrary)
        ? (ICON_FA_CUBE " Function Library")
        : (ICON_FA_DIAGRAM_PROJECT " Blueprint");
    ImGui::TextColored(ImVec4(0.5f, 0.85f, 0.5f, 1.0f), "%s", typeLabel);
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    // ── 路径输入 ─────────────────────────────────────────────────────────────
    ImGui::TextUnformatted("File path (relative to assets/, subdirs OK, no extension):");
    ImGui::SetNextItemWidth(-1.0f);
    bool enterPressed = ImGui::InputText("##saveNameInput", d.inputBuf, sizeof(d.inputBuf),
        ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_AutoSelectAll);

    std::string resolved = ResolveSaveDialogPath();
    if (!resolved.empty())
    {
        ImGui::Spacing();
        ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled));
        ImGui::TextWrapped("Full path: %s", resolved.c_str());
        ImGui::PopStyleColor();
        if (std::filesystem::exists(resolved))
            ImGui::TextColored(ImVec4(1.0f, 0.75f, 0.2f, 1.0f),
                ICON_FA_TRIANGLE_EXCLAMATION " File exists — will be overwritten.");
    }

    if (!d.errorMsg.empty())
    {
        ImGui::Spacing();
        ImGui::TextColored(ImVec4(1.0f, 0.35f, 0.35f, 1.0f),
            ICON_FA_CIRCLE_EXCLAMATION " %s", d.errorMsg.c_str());
    }

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    // ── 按钮行 ───────────────────────────────────────────────────────────────
    float btnW = std::max(
        ImGui::CalcTextSize(ICON_FA_CHECK " Confirm").x,
        ImGui::CalcTextSize(ICON_FA_XMARK  " Cancel").x)
        + ImGui::GetStyle().FramePadding.x * 2.0f + 16.0f;
    float totalBtnW = btnW * 2 + ImGui::GetStyle().ItemSpacing.x;
    float indentX   = (ImGui::GetContentRegionAvail().x - totalBtnW) * 0.5f;
    if (indentX > 0) ImGui::SetCursorPosX(ImGui::GetCursorPosX() + indentX);

    bool confirmed = false;
    ImGui::PushStyleColor(ImGuiCol_Button,        IM_COL32(30, 100, 50, 255));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, IM_COL32(45, 135, 70, 255));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive,  IM_COL32(55, 160, 85, 255));
    if (ImGui::Button(ICON_FA_CHECK " Confirm", ImVec2(btnW, 0)) || enterPressed)
        confirmed = true;
    ImGui::PopStyleColor(3);

    ImGui::SameLine();
    if (ImGui::Button(ICON_FA_XMARK " Cancel", ImVec2(btnW, 0)))
    {
        if (d.isNew && ActiveDoc() && ActiveDoc()->filePath.empty() && !ActiveDoc()->isDirty)
            CloseDocument(m_ActiveDocIndex);
        ImGui::CloseCurrentPopup();
        ImGui::EndPopup();
        return;
    }

    // ── 确认逻辑 ─────────────────────────────────────────────────────────────
    if (confirmed)
    {
        d.errorMsg.clear();

        std::string raw = d.inputBuf;
        while (!raw.empty() && (raw.front() == ' ' || raw.front() == '\t')) raw.erase(raw.begin());
        while (!raw.empty() && (raw.back()  == ' ' || raw.back()  == '\t')) raw.pop_back();
        for (char& c : raw) if (c == '\\') c = '/';

        if (raw.empty())
            d.errorMsg = "File name cannot be empty.";
        else if (raw.front() == '/')
            d.errorMsg = "Path must be relative (do not start with /).";
        else if (raw.find("..") != std::string::npos)
            d.errorMsg = "Path must not contain '..'.";
        else if (raw.back() == '/')
            d.errorMsg = "Path must end with a file name, not a directory.";
        else
        {
            for (char c : raw)
            {
                if (c == '*' || c == '?' || c == '"' || c == '<' || c == '>' || c == '|' || c == ':')
                {
                    d.errorMsg = std::string("Invalid character '") + c + "'.";
                    break;
                }
            }
        }

        if (d.errorMsg.empty())
        {
            std::string fullPath = ResolveSaveDialogPath();
            if (fullPath.empty())
            {
                d.errorMsg = "Could not resolve path. Please check the input.";
            }
            else
            {
                try {
                    std::filesystem::create_directories(
                        std::filesystem::path(fullPath).parent_path());
                } catch (const std::exception& e) {
                    d.errorMsg = std::string("Failed to create directory: ") + e.what();
                }

                if (d.errorMsg.empty() && ActiveDoc())
                {
                    ActiveDoc()->filePath = fullPath;
                    DoSaveFile(fullPath);
                    ImGui::CloseCurrentPopup();
                }
            }
        }
    }

    ImGui::EndPopup();
}
