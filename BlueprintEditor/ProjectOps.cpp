// BlueprintEditor/ProjectOps.cpp
#include "BlueprintEditor.h"
#include "BpLogger.h"
#include "FileDialogs.h"

#include <filesystem>
#include <algorithm>
#include <functional>
namespace fs = std::filesystem;

// ============================================================================
// 路径工具函数（内部使用）
// ============================================================================

// 统一路径分隔符为 '/'
static std::string NormSlash(std::string s)
{
    for (char& c : s) if (c == '\\') c = '/';
    return s;
}

// 去掉 "assets/" 前缀（如果存在）
static std::string StripAssetsPrefix(std::string rp)
{
    rp = NormSlash(std::move(rp));
    if (rp.size() > 7 && rp.substr(0, 7) == "assets/")
        rp = rp.substr(7);
    return rp;
}

// 取 stem（不含扩展名、不含目录）
static std::string GetStem(const std::string& path)
{
    return fs::path(path).stem().string();
}

// 取扩展名（含点，如 ".bjson"）
static std::string GetExtension(const std::string& path)
{
    return fs::path(path).extension().string();
}

// 取目录部分（相对路径，去掉 assets/ 后的目录前缀）
// e.g. "assets/sub/dir/File.bjson" → "sub/dir"
static std::string GetRelDir(const std::string& relPath)
{
    std::string rp = StripAssetsPrefix(relPath);
    auto slash = rp.rfind('/');
    return (slash != std::string::npos) ? rp.substr(0, slash) : "";
}

// 构建工程相对路径（含 assets/ 前缀）
// e.g. relNoExt="sub/dir/Foo", ext=".bjson" → "assets/sub/dir/Foo.bjson"
static std::string BuildRelPath(const std::string& relNoExt, const std::string& ext)
{
    return "assets/" + relNoExt + ext;
}

// 两路径是否指向同一文件（lexically_normal 比较）
static bool SamePath(const std::string& a, const std::string& b)
{
    return fs::path(a).lexically_normal().string() ==
           fs::path(b).lexically_normal().string();
}

// ============================================================================
// 工程 —— 新建
// ============================================================================

void BlueprintEditor::NewProject()
{
    // 直接弹出系统保存对话框（不再先弹 ImGui 输入框）
    std::string path = SaveFileDialog(
        "Blueprint Project (*.bproj)\0*.bproj\0",
        "New Blueprint Project",
        "NewProject.bproj"
    );
    if (path.empty()) return;

    if (path.size() < 6 || path.substr(path.size() - 6) != ".bproj")
        path += ".bproj";

    // 从文件名提取工程名
    std::string name;
    {
        std::string fname = fs::path(path).stem().string();
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

    // ── 新建工程后：watch 工程目录下的 BlueprintEntry.lua（静默）──────────
    m_LuaNodeRegistrar.AddLuaPath(m_Project.projectDir);
    m_LuaNodeRegistrar.WatchEntryScript(
        m_Project.projectDir + "/BlueprintEntry.lua",
        m_Project.name + ":BlueprintEntry"
    );
}

// ============================================================================
// 工程 —— 打开
// ============================================================================

void BlueprintEditor::OpenProject()
{
    std::string path = OpenFileDialog(
        "Blueprint Project (*.bproj)\0*.bproj\0All Files (*.*)\0*.*\0",
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

    // ── 打开工程后：watch 工程目录下的 BlueprintEntry.lua（静默）──────────
    m_LuaNodeRegistrar.AddLuaPath(m_Project.projectDir);
    m_LuaNodeRegistrar.WatchEntryScript(
        m_Project.projectDir + "/BlueprintEntry.lua",
        m_Project.name + ":BlueprintEntry"
    );
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
        "Blueprint Project (*.bproj)\0*.bproj\0",
        "Save Blueprint Project",
        (m_Project.name + ".bproj").c_str()
    );
    if (path.empty()) return;

    // 确保扩展名
    if (path.size() < 6 || path.substr(path.size() - 6) != ".bproj")
        path += ".bproj";

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

    // 清除 Lua 注册的节点定义
#ifdef BLUEPRINT_HAS_LUA
    m_LuaNodeRegistrar.UnregisterAll();
#endif

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

// ============================================================================
// 重命名/移动后更新工程内所有蓝图文件的引用
// ============================================================================

void BlueprintEditor::UpdateBlueprintReferences(const std::string& oldAbsPath,
                                                 const std::string& newAbsPath)
{
    if (!m_Project.IsOpen()) return;

    // 计算旧/新的 stem（不含扩展名的纯文件名）
    // 用于替换 FuncLib.<stem>.funcId 这类 definitionId
    auto extractStem = [](const std::string& absPath) -> std::string {
        std::string s = fs::path(absPath).filename().string();
        if (s.size() > 6 && s.substr(s.size()-6) == ".bjson")
            s = s.substr(0, s.size()-6);
        return s;
    };
    std::string oldStem = extractStem(oldAbsPath);
    std::string newStem = extractStem(newAbsPath);

    // 旧/新相对路径（相对于工程目录，用于 dependencies 数组替换）
    std::string oldRel = m_Project.RelPath(oldAbsPath);
    std::string newRel = m_Project.RelPath(newAbsPath);
    // 统一路径分隔符
    for (char& c : oldRel) if (c == '\\') c = '/';
    for (char& c : newRel) if (c == '\\') c = '/';

    // 收集工程内所有蓝图 JSON 文件（排除被重命名的那个）
    std::vector<std::string> allFiles;
    for (const auto& e : m_Project.blueprints)
    {
        std::string abs = m_Project.AbsPath(e.relativePath);
        if (!abs.empty() && fs::exists(abs))
            allFiles.push_back(abs);
    }
    for (const auto& e : m_Project.libraries)
    {
        std::string abs = m_Project.AbsPath(e.relativePath);
        if (!abs.empty() && fs::exists(abs))
            allFiles.push_back(abs);
    }

    int updatedFiles = 0;
    ::NodeEditor::Runtime::JsonBlueprintExporter exporter;

    for (const auto& filePath : allFiles)
    {
        // 跳过刚改名的文件本身
        if (fs::path(filePath).lexically_normal() ==
            fs::path(newAbsPath).lexically_normal()) continue;

        auto result = exporter.importRuntimeFromFile(filePath);
        if (!result.success) continue;

        bool modified = false;
        auto& data = result.data;

        // 1. 更新 metadata.dependencies
        for (auto& dep : data.metadata.dependencies)
        {
            // 规范化比较（去掉 ./ 等）
            std::string normDep = dep;
            for (char& c : normDep) if (c == '\\') c = '/';
            if (normDep == oldRel ||
                fs::path(m_Project.AbsPath(dep)).lexically_normal() ==
                fs::path(oldAbsPath).lexically_normal())
            {
                dep = newRel;
                modified = true;
            }
        }

        // 2. 更新 ExecuteBlueprint 节点的 File 引脚值
        for (auto& node : data.nodes)
        {
            if (node.definitionId != "ExecuteBlueprint") continue;
            for (auto& pin : node.pins)
            {
                if (pin.name != "File") continue;
                std::string val = pin.defaultValue.asString();
                if (val.empty()) continue;
                // 比较文件名 stem（不含扩展名）
                std::string valStem = fs::path(val).stem().string();
                if (valStem == oldStem)
                {
                    // 保留目录前缀，只替换文件名 stem，保留 .bjson 扩展名
                    std::string dir = fs::path(val).parent_path().string();
                    std::string newVal = newStem + ".bjson";
                    if (!dir.empty() && dir != ".")
                        newVal = dir + "/" + newVal;
                    pin.defaultValue = ::NodeEditor::Runtime::Variant(newVal);
                    modified = true;
                }
            }
        }

        // 3. 更新 FuncLib.<oldStem>.<funcId> → FuncLib.<newStem>.<funcId>
        std::string oldPrefix = "FuncLib." + oldStem + ".";
        std::string newPrefix = "FuncLib." + newStem + ".";
        for (auto& node : data.nodes)
        {
            if (node.definitionId.rfind(oldPrefix, 0) == 0)
            {
                node.definitionId = newPrefix + node.definitionId.substr(oldPrefix.size());
                modified = true;
            }
        }

        if (modified)
        {
            // 单文件模式：重新加载完整数据（runtime+editor），再保存
            // importRuntimeFromFile 自动检测新格式，同时加载 editor 数据
            auto fullResult = exporter.importRuntimeFromFile(filePath);
            if (fullResult.success)
            {
                fullResult.data.metadata.dependencies = data.metadata.dependencies;
                for (size_t i = 0; i < data.nodes.size() && i < fullResult.data.nodes.size(); ++i)
                {
                    fullResult.data.nodes[i].definitionId = data.nodes[i].definitionId;
                    fullResult.data.nodes[i].pins         = data.nodes[i].pins;
                }
                exporter.exportEditorFiles(fullResult.data, filePath);
            }

            // 同步内存中已打开的文档
            for (auto& doc : m_Documents)
            {
                if (fs::path(doc->filePath).lexically_normal() ==
                    fs::path(filePath).lexically_normal())
                {
                    // 更新内存中的节点 definitionId
                    for (auto& node : doc->nodes)
                    {
                        if (node.DefinitionId.rfind(oldPrefix, 0) == 0)
                            node.DefinitionId = newPrefix + node.DefinitionId.substr(oldPrefix.size());
                    }
                    // 更新 dependencies
                    doc->dependencies.clear();
                    for (const auto& d : data.metadata.dependencies)
                        doc->dependencies.push_back(d);
                    doc->isDirty = false;  // 我们已经保存了
                }
            }

            ++updatedFiles;
        }
    }

    if (updatedFiles > 0)
    {
        BPLOG("UpdateBlueprintReferences: updated " + std::to_string(updatedFiles) + " file(s): "
              + oldStem + " -> " + newStem);
        // 重新注册库函数（定义ID已改变）
        SyncProjectLibrariesToRegistry();
        // 清除节点缓存
        m_CachedDefCount = 0;
    }
}

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

    // 确保 FunctionLibrary 根分类存在（否则右键菜单分类树不会显示库函数）
    if (total > 0)
    {
        RTNodeCategory cat;
        cat.id   = "FunctionLibrary";
        cat.name = "FunctionLibrary";
        m_NodeRegistry.registerCategory(cat);
    }

    // Phase 3：加载 Lua 扩展脚本（先清除旧定义再重新加载）
#ifdef BLUEPRINT_HAS_LUA
    m_LuaNodeRegistrar.UnregisterAll();
    m_LuaNodeRegistrar.Initialize(&m_NodeRegistry, &m_HandlerRegistry);
    int luaTotal = 0;
    for (const auto& relPath : m_Project.luaExtensions)
    {
        std::string absPath = m_Project.AbsPath(relPath);
        if (absPath.empty() || !fs::exists(absPath)) continue;
        int n = m_LuaNodeRegistrar.LoadScript(absPath);
        if (n >= 0)
            luaTotal += n;
        else
            BPLOG("[Lua] Error loading " + relPath + ": " + m_LuaNodeRegistrar.GetLastError());
    }
    if (luaTotal > 0)
        BPLOG("SyncProjectLibraries: registered " + std::to_string(luaTotal) + " Lua nodes");
#endif

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
        bool* pOpen = ImGui::GetStateStorage()->GetBoolRef(stateId, false);

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
            // 右键打开 Section 菜单
            if (ImGui::IsMouseClicked(ImGuiMouseButton_Right))
            {
                std::string secCtxId = std::string("##secCtx_") + openStateKey;
                ImGui::OpenPopup(secCtxId.c_str());
            }
        }
        {
            std::string secCtxId = std::string("##secCtx_") + openStateKey;
            if (ImGui::BeginPopup(secCtxId.c_str()))
            {
                const char* newLabel = (bpClass == RTBlueprintClass::FunctionLibrary)
                    ? ICON_FA_CUBE " New Library"
                    : ICON_FA_FILE " New Blueprint";
                if (ImGui::MenuItem(newLabel))
                    NewFile(bpClass);
                ImGui::EndPopup();
            }
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
                std::string rp = StripAssetsPrefix(entries[i].relativePath);

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

            // ── 新建/保存后自动展开到对应目录 ──────────────────────────
            if (!m_PendingExpandToPath.empty())
            {
                std::string target = StripAssetsPrefix(m_PendingExpandToPath);
                // 展开该路径中每一级子目录
                std::string accum;
                size_t pos2 = 0;
                while (true)
                {
                    size_t slash2 = target.find('/', pos2);
                    if (slash2 == std::string::npos) break;  // 最后一段是文件名
                    std::string seg = target.substr(pos2, slash2 - pos2);
                    accum = accum.empty() ? seg : (accum + "/" + seg);
                    std::string key2 = std::string(openStateKey) + "/" + accum;
                    ImGuiID id2 = ImGui::GetID(key2.c_str());
                    ImGui::GetStateStorage()->SetBool(id2, true);  // 强制展开
                    pos2 = slash2 + 1;
                }
                // 同时展开本 section
                ImGuiID secId2 = ImGui::GetID(openStateKey);
                ImGui::GetStateStorage()->SetBool(secId2, true);
                m_PendingExpandToPath.clear();
            }

            // 删除磁盘文件二次确认状态
            struct DeleteConfirmState {
                int  targetIdx    = -1;
                bool openNextFrame = false;
            };
            static DeleteConfirmState s_deleteState;

            // 弹出改名/移动对话框
            if (s_renameState.openNextFrame)
            {
                s_renameState.openNextFrame = false;
                ImGui::OpenPopup("##renameDialog");
            }
            // 弹出删除确认对话框
            if (s_deleteState.openNextFrame)
            {
                s_deleteState.openNextFrame = false;
                ImGui::OpenPopup("##deleteConfirmDialog");
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
                        newInput = NormSlash(std::move(newInput));

                        if (!newInput.empty())
                        {
                            auto& e = entries[idx];
                            std::string oldAbs     = m_Project.AbsPath(e.relativePath);
                            std::string oldRelDir  = GetRelDir(e.relativePath);
                            std::string ext        = GetExtension(oldAbs);

                            // 计算新相对路径（不含 assets/ 前缀和扩展名）
                            std::string newRelNoExt = s_renameState.isRename
                                ? (oldRelDir.empty() ? newInput : (oldRelDir + "/" + newInput))
                                : newInput;

                            std::string newRelPath = BuildRelPath(newRelNoExt, ext);
                            std::string newAbs     = m_Project.AbsPath(newRelPath);

                            // 执行文件系统操作
                            std::error_code ec;
                            fs::create_directories(fs::path(newAbs).parent_path(), ec);
                            fs::rename(oldAbs, newAbs, ec);
                            if (!ec)
                            {
                                e.relativePath = newRelPath;
                                e.displayName  = GetStem(newRelPath);
                                SaveProject();

                                // 更新已打开文档的 filePath
                                for (auto& doc : m_Documents)
                                    if (SamePath(doc->filePath, oldAbs))
                                    {
                                        doc->filePath = newAbs;
                                        SetTitle(("Blueprint Editor - " + GetStem(newAbs)).c_str());
                                    }

                                UpdateBlueprintReferences(oldAbs, newAbs);
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

            // ── 删除磁盘文件确认框 ────────────────────────────────────────
            if (ImGui::BeginPopupModal("##deleteConfirmDialog", nullptr,
                ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings))
            {
                int idx = s_deleteState.targetIdx;
                bool valid = (idx >= 0 && idx < (int)entries.size());
                ImGui::TextColored(ImVec4(1.0f, 0.35f, 0.3f, 1.0f),
                    ICON_FA_TRIANGLE_EXCLAMATION " Delete File from Disk");
                ImGui::Separator();
                if (valid)
                {
                    ImGui::TextWrapped("This will permanently delete the file from disk:\n  %s",
                        entries[idx].relativePath.c_str());
                    ImGui::Spacing();
                    ImGui::TextColored(ImVec4(1.0f, 0.6f, 0.3f, 1.0f), "This action cannot be undone!");
                    ImGui::Spacing();
                    ImGui::Separator();
                    ImGui::Spacing();

                    ImGui::PushStyleColor(ImGuiCol_Button,        IM_COL32(160, 30, 30, 255));
                    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, IM_COL32(200, 45, 45, 255));
                    bool doDelete = ImGui::Button(ICON_FA_TRASH " Delete", ImVec2(110, 0));
                    ImGui::PopStyleColor(2);
                    ImGui::SameLine(0, 8);
                    if (ImGui::Button(ICON_FA_XMARK " Cancel", ImVec2(110, 0)))
                        ImGui::CloseCurrentPopup();

                    if (doDelete)
                    {
                        std::string absPath = m_Project.AbsPath(entries[idx].relativePath);
                        // 关闭已打开的对应文档（避免悬空文件引用）
                        for (int d = (int)m_Documents.size() - 1; d >= 0; --d)
                            if (SamePath(m_Documents[d]->filePath, absPath))
                                m_Documents.erase(m_Documents.begin() + d);
                        if (m_ActiveDocIndex >= (int)m_Documents.size())
                            m_ActiveDocIndex = (int)m_Documents.size() - 1;

                        std::error_code ec;
                        fs::remove(absPath, ec);
                        entries.erase(entries.begin() + idx);
                        SaveProject();
                        ImGui::CloseCurrentPopup();
                    }
                }
                else
                {
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
                    bool* pDirOpen = ImGui::GetStateStorage()->GetBoolRef(dirStateId, false);

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

                    // 目录右键菜单：在此目录下新建文件 + 批量操作
                    ImGui::PushID(dirStateKey.c_str());
                    ImGui::SetCursorScreenPos(rowMin);
                    ImGui::InvisibleButton("##dirRow", ImVec2(panelW, rowH));
                    std::string dirCtxId = "##dirCtx_" + dirStateKey;
                    if (ImGui::BeginPopupContextItem(dirCtxId.c_str()))
                    {
                        const char* newLabel = (bpClass == RTBlueprintClass::FunctionLibrary)
                            ? ICON_FA_CUBE " New Library Here"
                            : ICON_FA_FILE " New Blueprint Here";
                        if (ImGui::MenuItem(newLabel))
                        {
                            // 先创建空白文档，再打开命名对话框（与 NewFile() 流程一致）
                            CreateNewDocument();
                            ActiveDoc()->blueprintClass = bpClass;
                            ed::SetCurrentEditor(ActiveDoc()->editorContext);
                            OpenSaveNameDialog(/*isNew=*/true, bpClass);
                            snprintf(m_SaveNameDialog.inputBuf, sizeof(m_SaveNameDialog.inputBuf),
                                     "%s/", fullDirPath.c_str());
                        }
                        ImGui::Separator();
                        if (ImGui::MenuItem(ICON_FA_XMARK " Remove All from Project"))
                        {
                            // 从 entries 中移除所有属于此目录的文件（不删磁盘）
                            std::string prefix = "assets/" + fullDirPath + "/";
                            entries.erase(std::remove_if(entries.begin(), entries.end(),
                                [&](const BpProjectEntry& e) {
                                    std::string rp = NormSlash(e.relativePath);
                                    return rp.rfind(prefix, 0) == 0;
                                }), entries.end());
                            SaveProject();
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
                        ? GetStem(e.relativePath)
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
                            snprintf(s_renameState.inputBuf, sizeof(s_renameState.inputBuf),
                                     "%s", GetStem(e.relativePath).c_str());
                            s_renameState.openNextFrame = true;
                        }
                        if (ImGui::MenuItem(ICON_FA_ARROW_RIGHT " Move"))
                        {
                            s_renameState.targetIdx = i;
                            s_renameState.isRename  = false;
                            // 预填：去掉 assets/ 前缀和扩展名
                            std::string rp = StripAssetsPrefix(e.relativePath);
                            auto dotPos = rp.rfind('.');
                            if (dotPos != std::string::npos) rp = rp.substr(0, dotPos);
                            snprintf(s_renameState.inputBuf, sizeof(s_renameState.inputBuf),
                                     "%s", rp.c_str());
                            s_renameState.openNextFrame = true;
                        }
                        ImGui::Separator();
                        if (ImGui::MenuItem(ICON_FA_XMARK " Remove from Project"))
                            removeIdx = i;
                        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.45f, 0.4f, 1.0f));
                        if (ImGui::MenuItem(ICON_FA_TRASH " Delete from Disk"))
                        {
                            s_deleteState.targetIdx = i;
                            s_deleteState.openNextFrame = true;
                        }
                        ImGui::PopStyleColor();
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

#ifdef BLUEPRINT_HAS_LUA
    // ── Lua 脚本 Section ────────────────────────────────────────────────
    {
        ImGui::PushID("##sec_lua");
        ImGuiID stateId = ImGui::GetID("##sec_lua");
        bool* pOpen = ImGui::GetStateStorage()->GetBoolRef(stateId, false);

        float secH   = lineH + 6.0f;
        ImVec2 secMin = ImGui::GetCursorScreenPos();

        dl->AddRectFilled(secMin, ImVec2(secMin.x + panelW, secMin.y + secH),
                          IM_COL32(28, 30, 36, 220));
        dl->AddLine(ImVec2(secMin.x, secMin.y + secH - 1),
                    ImVec2(secMin.x + panelW, secMin.y + secH - 1),
                    IM_COL32(55, 60, 70, 200));

        const char* arrow = *pOpen ? ICON_FA_CARET_DOWN : ICON_FA_CARET_RIGHT;
        char headerText[80];
        std::snprintf(headerText, sizeof(headerText), "%s  " ICON_FA_CODE_BRANCH "  LUA SCRIPTS  (%d)",
                      arrow, (int)m_Project.luaExtensions.size());
        ImVec2 textPos(secMin.x + 6.0f, secMin.y + (secH - lineH) * 0.5f);
        dl->AddText(textPos, IM_COL32(200, 170, 100, 230), headerText);

        // [+] 按钮（添加 Lua 脚本）
        float plusW = ImGui::CalcTextSize(ICON_FA_PLUS).x + 8.0f;
        float plusX = secMin.x + panelW - plusW - 4.0f;
        float plusY = secMin.y + (secH - lineH) * 0.5f;
        ImVec2 plusMin(plusX - 2.0f, secMin.y + 1.0f);
        ImVec2 plusMax(plusX + plusW, secMin.y + secH - 1.0f);
        bool plusHov  = ImGui::IsMouseHoveringRect(plusMin, plusMax);
        bool plusClick = plusHov && ImGui::IsMouseClicked(ImGuiMouseButton_Left);
        if (plusHov)
            dl->AddRectFilled(plusMin, plusMax, IM_COL32(180, 140, 0, 70), 3.0f);
        dl->AddText(ImVec2(plusX + 2.0f, plusY), IM_COL32(200, 170, 100, 220), ICON_FA_PLUS);
        if (plusClick)
        {
            // 打开文件选择对话框，选择 .lua 文件加入工程
            std::string startDir = m_Project.projectDir.empty() ? "." : m_Project.projectDir;
            std::string luaPath = OpenFileDialog("Lua Script (*.lua)\0*.lua\0All Files (*.*)\0*.*\0", "Add Lua Script");
            if (!luaPath.empty())
            {
                std::string relPath = m_Project.RelPath(luaPath);
                // 去重
                bool exists = false;
                for (const auto& p : m_Project.luaExtensions)
                    if (p == relPath) { exists = true; break; }
                if (!exists)
                {
                    m_Project.luaExtensions.push_back(relPath);
                    SaveProject();
                    SyncProjectLibrariesToRegistry();
                }
            }
        }
        if (plusHov && ImGui::BeginTooltip())
        {
            ImGui::TextUnformatted("Add Lua Script to project");
            ImGui::EndTooltip();
        }

        // Header 点击展开/折叠
        ImRect headerRect(secMin, ImVec2(plusMin.x - 2.0f, secMin.y + secH));
        if (ImGui::IsMouseHoveringRect(headerRect.Min, headerRect.Max))
        {
            dl->AddRectFilled(secMin, ImVec2(plusMin.x - 2.0f, secMin.y + secH),
                              IM_COL32(255, 255, 255, 8));
            if (ImGui::IsMouseClicked(ImGuiMouseButton_Left))
                *pOpen = !*pOpen;
        }

        ImGui::SetCursorScreenPos(ImVec2(secMin.x, secMin.y + secH));
        ImGui::Dummy(ImVec2(panelW, 0.0f));

        if (*pOpen)
        {
            // 热重载状态指示
            const auto& lastErr = m_LuaNodeRegistrar.GetLastError();
            if (!lastErr.empty())
            {
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.4f, 0.3f, 1.0f));
                ImGui::TextWrapped("  %s %s", ICON_FA_TRIANGLE_EXCLAMATION, lastErr.c_str());
                ImGui::PopStyleColor();
            }

            // 自动热重载切换
            bool autoReload = m_LuaNodeRegistrar.GetAutoReload();
            if (ImGui::Checkbox("  Auto hot-reload##lua", &autoReload))
                m_LuaNodeRegistrar.SetAutoReload(autoReload);
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("Reload Lua scripts automatically when files change");

            ImGui::SameLine();
            if (ImGui::SmallButton(ICON_FA_ARROWS_ROTATE " Reload All##lua"))
            {
                int n = m_LuaNodeRegistrar.ReloadAll();
                m_CachedDefCount = 0;
                if (n >= 0)
                    BPLOG("[Lua] Reloaded " + std::to_string(n) + " node definitions");
                else
                    BPLOG("[Lua] Reload failed: " + m_LuaNodeRegistrar.GetLastError());
            }
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("Force reload all Lua scripts now");

            ImGui::Spacing();

            int removeIdx = -1;
            for (int i = 0; i < (int)m_Project.luaExtensions.size(); ++i)
            {
                ImGui::PushID(i);
                const auto& relPath = m_Project.luaExtensions[i];
                std::string fname = fs::path(relPath).filename().string();

                float rowH   = lineH + 4.0f;
                ImVec2 rowMin = ImGui::GetCursorScreenPos();
                bool rowHov  = ImGui::IsMouseHoveringRect(rowMin, ImVec2(rowMin.x + panelW, rowMin.y + rowH));

                if (rowHov)
                    dl->AddRectFilled(rowMin, ImVec2(rowMin.x + panelW, rowMin.y + rowH),
                                      IM_COL32(255, 255, 255, 12));

                // Lua 文件图标（橙黄色）
                bool fileExists = fs::exists(m_Project.AbsPath(relPath));
                ImU32 textCol = fileExists ? IM_COL32(220, 180, 60, 230) : IM_COL32(160, 60, 60, 200);
                std::string rowText = std::string("    " ICON_FA_CODE_BRANCH "  ") + fname;
                dl->AddText(ImVec2(rowMin.x + 4.0f, rowMin.y + 2.0f), textCol, rowText.c_str());

                // Tooltip：完整路径 + 错误提示
                if (rowHov && ImGui::BeginTooltip())
                {
                    ImGui::TextUnformatted(relPath.c_str());
                    if (!fileExists)
                        ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.3f, 1.0f), "File not found!");
                    ImGui::EndTooltip();
                }

                // 右键：移除
                if (rowHov && ImGui::IsMouseClicked(ImGuiMouseButton_Right))
                    ImGui::OpenPopup("##luaEntryCtx");
                if (ImGui::BeginPopup("##luaEntryCtx"))
                {
                    if (ImGui::MenuItem(ICON_FA_XMARK " Remove from project"))
                        removeIdx = i;
                    if (ImGui::MenuItem(ICON_FA_ARROWS_ROTATE " Reload this script"))
                    {
                        std::string absPath = m_Project.AbsPath(relPath);
                        int n = m_LuaNodeRegistrar.ReloadFile(absPath);
                        m_CachedDefCount = 0;
                        (void)n;
                    }
                    ImGui::EndPopup();
                }

                ImGui::SetCursorScreenPos(ImVec2(rowMin.x, rowMin.y + rowH));
                ImGui::Dummy(ImVec2(panelW, 0.0f));
                ImGui::PopID();
            }

            if (removeIdx >= 0)
            {
                m_Project.luaExtensions.erase(m_Project.luaExtensions.begin() + removeIdx);
                SaveProject();
                m_LuaNodeRegistrar.UnregisterAll();
                SyncProjectLibrariesToRegistry();
            }

            ImGui::Spacing();
        }
        ImGui::PopID();
    }
#endif // BLUEPRINT_HAS_LUA

    ImGui::PopStyleVar(2);  // FramePadding + ItemSpacing
}
