// BlueprintEditor/ProjectOps.cpp
#include "BlueprintEditor.h"
#include "BpLogger.h"
#include "FileDialogs.h"

#include <filesystem>
#include <algorithm>
#include <functional>
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

    // 固定样式：只在这个函数内部压入，确保不泄漏到外部
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding,  ImVec2(2.0f, 2.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing,   ImVec2(4.0f, 2.0f));

    float panelW = ImGui::GetContentRegionAvail().x;
    float lineH  = ImGui::GetTextLineHeight();
    auto* dl     = ImGui::GetWindowDrawList();

    // ────────────────────────────────────────────────────────────────────
    // 工程 Header：深色背景条，左侧项目名，右侧 4 个小图标按钮
    // ────────────────────────────────────────────────────────────────────
    {
        float hdrH   = lineH + 8.0f;
        ImVec2 hdrMin = ImGui::GetCursorScreenPos();
        dl->AddRectFilled(hdrMin, ImVec2(hdrMin.x + panelW, hdrMin.y + hdrH),
                          IM_COL32(35, 37, 43, 255));

        // 左侧项目名（垂直居中）
        ImVec2 textPos(hdrMin.x + 8.0f, hdrMin.y + (hdrH - lineH) * 0.5f);
        dl->AddText(textPos, IM_COL32(200, 210, 220, 255),
                    (m_Project.name.empty() ? "(untitled)" : m_Project.name).c_str());

        // 右侧 4 个按钮（New BP, New Lib, Refresh, Add Current）
        const float btnSz  = hdrH - 6.0f;
        const float btnGap = 2.0f;
        float btnX = hdrMin.x + panelW - (btnSz + btnGap) * 4.0f - 4.0f;
        float btnY = hdrMin.y + (hdrH - btnSz) * 0.5f;
        ImGui::SetCursorScreenPos(ImVec2(btnX, btnY));

        ImGui::PushStyleColor(ImGuiCol_Button,        IM_COL32(0,0,0,0));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, IM_COL32(0, 122, 204, 70));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive,  IM_COL32(0, 122, 204, 130));

        if (ImGui::Button(ICON_FA_FILE "##newBP", ImVec2(btnSz, btnSz)))
            NewFile(RTBlueprintClass::Actor);
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("New Blueprint");

        ImGui::SameLine(0, btnGap);
        if (ImGui::Button(ICON_FA_CUBE "##newLib", ImVec2(btnSz, btnSz)))
            NewFile(RTBlueprintClass::FunctionLibrary);
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("New Library");

        ImGui::SameLine(0, btnGap);
        if (ImGui::Button(ICON_FA_ARROWS_ROTATE "##refresh", ImVec2(btnSz, btnSz)))
        {
            if (!m_Project.projectDir.empty())
            {
                for (auto it = m_Project.blueprints.begin(); it != m_Project.blueprints.end();)
                    if (!fs::exists(m_Project.AbsPath(it->relativePath))) it = m_Project.blueprints.erase(it);
                    else ++it;
                for (auto it = m_Project.libraries.begin(); it != m_Project.libraries.end();)
                    if (!fs::exists(m_Project.AbsPath(it->relativePath))) it = m_Project.libraries.erase(it);
                    else ++it;
                SaveProject();
            }
        }
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Refresh (remove missing files)");

        ImGui::SameLine(0, btnGap);
        bool canAdd = ActiveDoc() && !ActiveDoc()->filePath.empty();
        if (!canAdd) ImGui::BeginDisabled();
        if (ImGui::Button(ICON_FA_PLUS "##addCurrent", ImVec2(btnSz, btnSz)))
            AddCurrentDocToProject();
        if (!canAdd) ImGui::EndDisabled();
        if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
            ImGui::SetTooltip(canAdd ? "Add Current File to Project" : "Save the file first");

        ImGui::PopStyleColor(3);

        // 推进光标到 header 下方
        ImGui::SetCursorScreenPos(ImVec2(hdrMin.x, hdrMin.y + hdrH + 2.0f));
    }

    // ────────────────────────────────────────────────────────────────────
    // Section 绘制 lambda（手绘 header，无 TreeNodeFlags_Framed）
    // ────────────────────────────────────────────────────────────────────
    auto drawSection = [&](std::vector<BpProjectEntry>& entries,
                           const char* openStateKey,
                           const char* sectionLabel,
                           const char* entryIcon,
                           RTBlueprintClass bpClass)
    {
        // 用 openStateKey 作为本 section 的 ID 隔离作用域，避免两个 section 里相同 i 产生相同 PushID 散列
        ImGui::PushID(openStateKey);
        // 用 ImGui Storage 维护折叠状态（比 static 更安全，跨帧稳定）
        ImGuiID stateId = ImGui::GetID(openStateKey);
        bool* pOpen = ImGui::GetStateStorage()->GetBoolRef(stateId, true);

        float secH   = lineH + 6.0f;
        ImVec2 secMin = ImGui::GetCursorScreenPos();

        // Section header 背景
        dl->AddRectFilled(secMin, ImVec2(secMin.x + panelW, secMin.y + secH),
                          IM_COL32(28, 30, 36, 220));
        dl->AddLine(ImVec2(secMin.x, secMin.y + secH - 1),
                    ImVec2(secMin.x + panelW, secMin.y + secH - 1),
                    IM_COL32(55, 60, 70, 200));

        // 折叠箭头 + 标签（手绘，完全不影响 ItemSpacing/FramePadding）
        const char* arrow = *pOpen ? ICON_FA_CARET_DOWN : ICON_FA_CARET_RIGHT;
        char headerText[64];
        std::snprintf(headerText, sizeof(headerText), "%s  %s  (%d)",
                      arrow, sectionLabel, (int)entries.size());
        ImVec2 textPos(secMin.x + 6.0f, secMin.y + (secH - lineH) * 0.5f);
        dl->AddText(textPos, IM_COL32(190, 195, 205, 230), headerText);

        // [+] 新建按钮（右侧，手绘）
        float plusW = ImGui::CalcTextSize(ICON_FA_PLUS).x + 8.0f;
        float plusX = secMin.x + panelW - plusW - 4.0f;
        float plusY = secMin.y + (secH - lineH) * 0.5f;
        ImVec2 plusMin(plusX - 2.0f, secMin.y + 1.0f);
        ImVec2 plusMax(plusX + plusW, secMin.y + secH - 1.0f);
        bool plusHovered = ImGui::IsMouseHoveringRect(plusMin, plusMax);
        bool plusClicked = plusHovered && ImGui::IsMouseClicked(ImGuiMouseButton_Left);
        if (plusHovered)
            dl->AddRectFilled(plusMin, plusMax, IM_COL32(0, 122, 204, 80), 3.0f);
        dl->AddText(ImVec2(plusX + 2.0f, plusY), IM_COL32(160, 180, 220, 220), ICON_FA_PLUS);
        if (plusClicked)
            NewFile(bpClass);
        if (plusHovered && ImGui::BeginTooltip())
        {
            ImGui::TextUnformatted(bpClass == RTBlueprintClass::Actor ? "New Blueprint" : "New Library");
            ImGui::EndTooltip();
        }

        // Header 点击切换折叠（排除按钮区域）
        ImRect headerRect(secMin, ImVec2(plusMin.x - 2.0f, secMin.y + secH));
        if (ImGui::IsMouseHoveringRect(headerRect.Min, headerRect.Max))
        {
            dl->AddRectFilled(secMin, ImVec2(plusMin.x - 2.0f, secMin.y + secH),
                              IM_COL32(255, 255, 255, 8));
            if (ImGui::IsMouseClicked(ImGuiMouseButton_Left))
                *pOpen = !*pOpen;
        }

        // 推进光标
        ImGui::SetCursorScreenPos(ImVec2(secMin.x, secMin.y + secH));
        ImGui::Dummy(ImVec2(panelW, 0.0f));

        if (*pOpen)
        {
            // ── 目录树渲染 ──────────────────────────────────────────────
            // 将 entries 按目录前缀分组，构建虚拟目录树
            // 使用 ordered map 保证目录名有序
            struct DirNode {
                std::vector<int> fileIndices;  // 直接在本目录下的文件
                std::map<std::string, DirNode> subdirs;  // 子目录
            };
            DirNode root;
            for (int i = 0; i < (int)entries.size(); ++i)
            {
                // relativePath 格式: "sub/dir/file.json" 或 "file.json"
                std::string rp = entries[i].relativePath;
                // 统一分隔符
                for (char& c : rp) if (c == '\\') c = '/';
                // 去掉 assets/ 前缀（如果有）
                if (rp.size() > 7 && rp.substr(0, 7) == "assets/") rp = rp.substr(7);

                // 按 '/' 分割路径
                DirNode* cur = &root;
                size_t pos = 0;
                while (true)
                {
                    size_t slash = rp.find('/', pos);
                    if (slash == std::string::npos)
                    {
                        cur->fileIndices.push_back(i);
                        break;
                    }
                    std::string dirName = rp.substr(pos, slash - pos);
                    cur = &cur->subdirs[dirName];
                    pos = slash + 1;
                }
            }

            // 改名/移动弹窗状态（per-section static 避免跨 section 干扰）
            // 用 ImGui Storage 存储弹窗状态
            struct RenameState {
                int  targetIdx   = -1;
                bool isRename    = true;   // true=改名, false=移动
                char inputBuf[256] = {};
                bool openNextFrame = false;
            };
            static RenameState s_renameState;

            // 弹出改名/移动对话框
            if (s_renameState.openNextFrame)
            {
                s_renameState.openNextFrame = false;
                ImGui::OpenPopup("##renameDialog");
            }
            if (ImGui::BeginPopupModal("##renameDialog", nullptr,
                ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings))
            {
                int idx = s_renameState.targetIdx;
                bool valid = (idx >= 0 && idx < (int)entries.size());
                const char* dlgTitle = s_renameState.isRename ? "Rename File" : "Move File";
                ImGui::TextColored(ImVec4(0.7f, 0.85f, 1.0f, 1.0f), "%s", dlgTitle);
                ImGui::Separator();
                if (valid)
                {
                    ImGui::TextDisabled("Current: %s", entries[idx].relativePath.c_str());
                    ImGui::Spacing();
                    const char* hint = s_renameState.isRename
                        ? "New name (no extension, same dir)"
                        : "New relative path (e.g. sub/dir/Name)";
                    ImGui::TextUnformatted(hint);
                    ImGui::SetNextItemWidth(360.0f);
                    bool enter = ImGui::InputText("##renameInput", s_renameState.inputBuf,
                        sizeof(s_renameState.inputBuf),
                        ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_AutoSelectAll);
                    ImGui::Spacing();
                    ImGui::Separator();
                    ImGui::Spacing();

                    bool confirmed = false;
                    ImGui::PushStyleColor(ImGuiCol_Button,        IM_COL32(30, 100, 50, 255));
                    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, IM_COL32(45, 135, 70, 255));
                    if (ImGui::Button(ICON_FA_CHECK " OK", ImVec2(100, 0)) || enter)
                        confirmed = true;
                    ImGui::PopStyleColor(2);
                    ImGui::SameLine(0, 8);
                    if (ImGui::Button(ICON_FA_XMARK " Cancel", ImVec2(100, 0)))
                        ImGui::CloseCurrentPopup();

                    if (confirmed)
                    {
                        std::string newInput(s_renameState.inputBuf);
                        // 去首尾空白
                        while (!newInput.empty() && (newInput.front() == ' ' || newInput.front() == '\t')) newInput.erase(newInput.begin());
                        while (!newInput.empty() && (newInput.back()  == ' ' || newInput.back()  == '\t')) newInput.pop_back();
                        for (char& c : newInput) if (c == '\\') c = '/';

                        if (!newInput.empty())
                        {
                            auto& e = entries[idx];
                            std::string oldAbs = m_Project.AbsPath(e.relativePath);
                            std::string oldRelDir;
                            {
                                std::string rp = e.relativePath;
                                for (char& c : rp) if (c == '\\') c = '/';
                                if (rp.size() > 7 && rp.substr(0, 7) == "assets/") rp = rp.substr(7);
                                auto slashPos = rp.rfind('/');
                                oldRelDir = (slashPos != std::string::npos) ? rp.substr(0, slashPos) : "";
                            }

                            // 计算新的 relative path（相对 assets/）
                            std::string newRelNoExt;
                            if (s_renameState.isRename)
                            {
                                // 改名：只改文件名，目录不变
                                newRelNoExt = oldRelDir.empty() ? newInput : (oldRelDir + "/" + newInput);
                            }
                            else
                            {
                                // 移动：newInput 是完整相对路径（不含扩展名）
                                newRelNoExt = newInput;
                            }

                            // 确定扩展名（从旧文件保留）
                            std::string ext = fs::path(oldAbs).extension().string();
                            std::string newRelPath = "assets/" + newRelNoExt + ext;
                            std::string newAbs     = m_Project.AbsPath(newRelPath);

                            // 执行文件系统操作
                            std::error_code ec;
                            fs::create_directories(fs::path(newAbs).parent_path(), ec);
                            fs::rename(oldAbs, newAbs, ec);
                            if (!ec)
                            {
                                // 同步 editor 文件（.editor.json）
                                std::string oldEditor = oldAbs.substr(0, oldAbs.rfind('.')) + ".editor.json";
                                std::string newEditor = newAbs.substr(0, newAbs.rfind('.')) + ".editor.json";
                                if (fs::exists(oldEditor))
                                    fs::rename(oldEditor, newEditor, ec);

                                // 更新 project 数据
                                e.relativePath = newRelPath;
                                e.displayName  = fs::path(newRelPath).stem().string();
                                SaveProject();

                                // 更新已打开文档的 filePath
                                for (auto& doc : m_Documents)
                                {
                                    std::string normOld = fs::path(oldAbs).lexically_normal().string();
                                    std::string normDoc = fs::path(doc->filePath).lexically_normal().string();
                                    if (normDoc == normOld)
                                    {
                                        doc->filePath = newAbs;
                                        std::string title = "Blueprint Editor - " + fs::path(newAbs).stem().string();
                                        SetTitle(title.c_str());
                                    }
                                }
                            }
                        }
                        ImGui::CloseCurrentPopup();
                    }
                }
                else
                {
                    ImGui::TextColored(ImVec4(1,0.3f,0.3f,1), "Invalid target");
                    if (ImGui::Button("Close")) ImGui::CloseCurrentPopup();
                }
                ImGui::EndPopup();
            }

            int removeIdx = -1;

            // ── 递归渲染目录树 ──────────────────────────────────────────
            // 用 std::function 实现递归 lambda
            std::function<void(DirNode&, const std::string&, int)> renderDir;
            renderDir = [&](DirNode& node, const std::string& dirPath, int depth)
            {
                float indent = depth * 12.0f;

                // 先渲染子目录
                for (auto& [name, child] : node.subdirs)
                {
                    std::string fullDirPath = dirPath.empty() ? name : (dirPath + "/" + name);
                    std::string dirStateKey = openStateKey + std::string("/") + fullDirPath;
                    ImGuiID dirStateId = ImGui::GetID(dirStateKey.c_str());
                    bool* pDirOpen = ImGui::GetStateStorage()->GetBoolRef(dirStateId, true);

                    float rowH = lineH + 4.0f;
                    ImVec2 rowMin = ImGui::GetCursorScreenPos();

                    bool rowHov = ImGui::IsMouseHoveringRect(rowMin, ImVec2(rowMin.x + panelW, rowMin.y + rowH));
                    if (rowHov) dl->AddRectFilled(rowMin, ImVec2(rowMin.x + panelW, rowMin.y + rowH), IM_COL32(255,255,255,12));

                    // 折叠箭头 + 目录图标 + 名称
                    const char* arr = *pDirOpen ? ICON_FA_CARET_DOWN : ICON_FA_CARET_RIGHT;
                    std::string dirLabel = std::string("  ") + arr + "  " + ICON_FA_FOLDER_OPEN + "  " + name;
                    dl->AddText(ImVec2(rowMin.x + 4.0f + indent, rowMin.y + 2.0f),
                                IM_COL32(180, 195, 215, 220), dirLabel.c_str());

                    // 点击切换折叠
                    if (rowHov && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
                        *pDirOpen = !*pDirOpen;

                    // 目录右键菜单：在此目录下新建文件
                    ImGui::PushID(dirStateKey.c_str());
                    ImGui::SetCursorScreenPos(rowMin);
                    ImGui::InvisibleButton("##dirRow", ImVec2(panelW, rowH));
                    std::string dirCtxId = "##dirCtx_" + dirStateKey;
                    if (ImGui::BeginPopupContextItem(dirCtxId.c_str()))
                    {
                        if (ImGui::MenuItem(ICON_FA_FILE " New Blueprint Here"))
                        {
                            // 打开命名对话框，预填目录前缀
                            OpenSaveNameDialog(/*isNew=*/true, bpClass);
                            // 预填路径
                            snprintf(m_SaveNameDialog.inputBuf, sizeof(m_SaveNameDialog.inputBuf),
                                     "%s/", fullDirPath.c_str());
                        }
                        ImGui::EndPopup();
                    }
                    ImGui::PopID();

                    ImGui::SetCursorScreenPos(ImVec2(rowMin.x, rowMin.y + rowH));

                    if (*pDirOpen)
                        renderDir(child, fullDirPath, depth + 1);
                }

                // 再渲染本目录下的文件
                for (int i : node.fileIndices)
                {
                    auto& e = entries[i];
                    ImGui::PushID(i);

                    std::string label = e.displayName.empty()
                        ? fs::path(e.relativePath).stem().string()
                        : e.displayName;
                    bool isActive = ActiveDoc() && !ActiveDoc()->filePath.empty() &&
                                    m_Project.RelPath(ActiveDoc()->filePath) == e.relativePath;

                    float rowH = lineH + 4.0f;
                    ImVec2 rowMin = ImGui::GetCursorScreenPos();

                    bool rowHov = ImGui::IsMouseHoveringRect(rowMin, ImVec2(rowMin.x + panelW, rowMin.y + rowH));
                    if (isActive)
                        dl->AddRectFilled(rowMin, ImVec2(rowMin.x + panelW, rowMin.y + rowH), IM_COL32(0, 122, 204, 40));
                    else if (rowHov)
                        dl->AddRectFilled(rowMin, ImVec2(rowMin.x + panelW, rowMin.y + rowH), IM_COL32(255, 255, 255, 12));

                    ImU32 textColor = isActive ? IM_COL32(80, 220, 120, 255) : IM_COL32(200, 205, 215, 230);
                    std::string rowText = std::string("  ") + entryIcon + "  " + label;
                    dl->AddText(ImVec2(rowMin.x + 4.0f + indent, rowMin.y + 2.0f), textColor, rowText.c_str());

                    // 点击打开
                    if (rowHov && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
                    {
                        std::string absPath = m_Project.AbsPath(e.relativePath);
                        std::string normAbs = fs::path(absPath).lexically_normal().string();
                        bool found = false;
                        for (int j = 0; j < (int)m_Documents.size(); ++j)
                        {
                            if (fs::path(m_Documents[j]->filePath).lexically_normal().string() == normAbs)
                            { m_PendingSwitchTabIndex = j; found = true; break; }
                        }
                        if (!found && fs::exists(absPath))
                            DoOpenFile(absPath);
                    }

                    // Tooltip
                    bool ctxOpen = ImGui::IsPopupOpen("##projEntryCtx");
                    if (rowHov && !ctxOpen && ImGui::BeginTooltip())
                    {
                        ImGui::TextUnformatted(e.relativePath.c_str());
                        ImGui::EndTooltip();
                    }

                    // 右键菜单
                    ImGui::SetCursorScreenPos(rowMin);
                    ImGui::InvisibleButton(("##row" + std::to_string(i)).c_str(), ImVec2(panelW, rowH));
                    if (ImGui::BeginPopupContextItem("##projEntryCtx"))
                    {
                        if (ImGui::MenuItem(ICON_FA_PEN " Rename"))
                        {
                            s_renameState.targetIdx = i;
                            s_renameState.isRename  = true;
                            // 预填当前文件名（不含目录和扩展名）
                            std::string stem = fs::path(e.relativePath).stem().string();
                            snprintf(s_renameState.inputBuf, sizeof(s_renameState.inputBuf), "%s", stem.c_str());
                            s_renameState.openNextFrame = true;
                        }
                        if (ImGui::MenuItem(ICON_FA_ARROW_RIGHT " Move"))
                        {
                            s_renameState.targetIdx = i;
                            s_renameState.isRename  = false;
                            // 预填当前相对路径（不含 assets/ 前缀和扩展名）
                            std::string rp = e.relativePath;
                            for (char& c : rp) if (c == '\\') c = '/';
                            if (rp.size() > 7 && rp.substr(0, 7) == "assets/") rp = rp.substr(7);
                            // 去掉扩展名
                            auto dotPos = rp.rfind('.');
                            if (dotPos != std::string::npos) rp = rp.substr(0, dotPos);
                            snprintf(s_renameState.inputBuf, sizeof(s_renameState.inputBuf), "%s", rp.c_str());
                            s_renameState.openNextFrame = true;
                        }
                        ImGui::Separator();
                        if (ImGui::MenuItem(ICON_FA_XMARK " Remove from Project"))
                            removeIdx = i;
                        ImGui::EndPopup();
                    }

                    ImGui::SetCursorScreenPos(ImVec2(rowMin.x, rowMin.y + rowH));
                    ImGui::PopID();
                }
            };

            renderDir(root, "", 0);

            if (entries.empty())
            {
                ImVec2 emptyPos = ImGui::GetCursorScreenPos();
                dl->AddText(ImVec2(emptyPos.x + 12.0f, emptyPos.y + 2.0f),
                            IM_COL32(100, 105, 115, 160), "(empty)");
                ImGui::Dummy(ImVec2(panelW, lineH + 4.0f));
            }

            if (removeIdx >= 0)
            {
                entries.erase(entries.begin() + removeIdx);
                SaveProject();
            }
        }

        ImGui::Spacing();
        ImGui::PopID(); // 对应 drawSection 开头的 PushID(openStateKey)
    };

    drawSection(m_Project.blueprints, "##sec_bp",  "BLUEPRINTS", ICON_FA_FILE, RTBlueprintClass::Actor);
    drawSection(m_Project.libraries,  "##sec_lib", "LIBRARIES",  ICON_FA_CUBE, RTBlueprintClass::FunctionLibrary);

    ImGui::PopStyleVar(2);  // FramePadding + ItemSpacing
}
