// FileOperations.cpp -- 蓝图文件操作（新建/打开/保存）
#include "BlueprintEditor.h"
// FileDialogs.h 中只是声明，本文件是实现，不需要 include

#ifdef _WIN32
#include <windows.h>
#include <commdlg.h>
#include <shlobj.h>
#pragma comment(lib, "comdlg32.lib")
#pragma comment(lib, "shell32.lib")
#endif

#include <sstream>
#include <chrono>
#include <filesystem>
#include <cstring>

namespace fs = std::filesystem;

// ============================================================================
// 平台文件对话框（外部链接，供 EditorUI / ProjectOps 调用）
// ============================================================================

#ifdef _WIN32
std::string OpenFileDialog(const char* filter, const char* title)
{
    char filename[MAX_PATH] = "";
    OPENFILENAMEA ofn = {};
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = GetActiveWindow();
    ofn.lpstrFilter = filter;
    ofn.lpstrFile = filename;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrTitle = title;
    ofn.Flags = OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR;
    
    if (GetOpenFileNameA(&ofn))
        return std::string(filename);
    return "";
}

std::string SaveFileDialog(const char* filter, const char* title, const char* defaultExt)
{
    char filename[MAX_PATH] = "";
    OPENFILENAMEA ofn = {};
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = GetActiveWindow();
    ofn.lpstrFilter = filter;
    ofn.lpstrFile = filename;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrTitle = title;
    ofn.lpstrDefExt = defaultExt;
    ofn.Flags = OFN_OVERWRITEPROMPT | OFN_NOCHANGEDIR;
    
    if (GetSaveFileNameA(&ofn))
        return std::string(filename);
    return "";
}
#else
// Linux / macOS 文件对话框实现
// Linux  → zenity（GNOME 自带），若不存在则回退 kdialog（KDE）
// macOS  → osascript (AppleScript)，系统自带，无额外依赖
//
// filter 参数（Windows 格式: "Desc\0*.ext\0..."）在此解析第一个 *.ext 用作过滤。

#include <cstdio>
#include <cstring>

namespace {

// 从 Windows 格式 filter 字符串里提取第一个扩展名（如 "bjson"）
static std::string ExtractFirstExt(const char* filter)
{
    if (!filter) return "";
    // 格式: "desc\0*.ext\0All\0*.*\0"
    // 第二个 \0 后是模式串，形如 "*.bjson"
    const char* p = filter;
    // 跳过第一段描述
    while (*p) ++p;
    ++p; // 跳过 '\0'
    if (!*p) return "";
    // p 指向 "*.bjson"
    if (*p == '*' && *(p+1) == '.')
        p += 2;  // 跳过 "*."
    std::string ext;
    while (*p && *p != '\0' && *p != ';')
        ext += *p++;
    return ext;  // e.g. "bjson"
}

// 执行 shell 命令，读取 stdout，返回第一行（去掉末尾换行）
static std::string RunDialog(const std::string& cmd)
{
    FILE* f = popen(cmd.c_str(), "r");
    if (!f) return "";
    char buf[4096] = {};
    if (!fgets(buf, sizeof(buf), f)) { pclose(f); return ""; }
    pclose(f);
    std::string s(buf);
    while (!s.empty() && (s.back() == '\n' || s.back() == '\r')) s.pop_back();
    return s;
}

// 检测可执行文件是否在 PATH 里
static bool HasExe(const char* name)
{
    std::string cmd = std::string("command -v ") + name + " >/dev/null 2>&1";
    return system(cmd.c_str()) == 0;
}

} // anonymous namespace

#if defined(__APPLE__)

std::string OpenFileDialog(const char* /*filter*/, const char* title)
{
    std::string script =
        "tell application \"System Events\"\n"
        "  activate\n"
        "  set f to POSIX path of (choose file with prompt \"" + std::string(title ? title : "Open") + "\")\n"
        "end tell";
    // Single-quoted to avoid shell expansion inside the AppleScript
    std::string cmd = "osascript -e '" + script + "' 2>/dev/null";
    return RunDialog(cmd);
}

std::string SaveFileDialog(const char* filter, const char* title, const char* defaultExt)
{
    std::string ext = defaultExt ? defaultExt : ExtractFirstExt(filter);
    std::string script =
        "tell application \"System Events\"\n"
        "  activate\n"
        "  set f to POSIX path of (choose file name with prompt \"" + std::string(title ? title : "Save") + "\""
        + (ext.empty() ? "" : " default name \"Untitled." + ext + "\"")
        + ")\n"
        "end tell";
    std::string cmd = "osascript -e '" + script + "' 2>/dev/null";
    std::string path = RunDialog(cmd);
    // 确保扩展名
    if (!path.empty() && !ext.empty())
    {
        std::string suffix = "." + ext;
        if (path.size() < suffix.size() ||
            path.compare(path.size() - suffix.size(), suffix.size(), suffix) != 0)
            path += suffix;
    }
    return path;
}

#else  // Linux

std::string OpenFileDialog(const char* filter, const char* /*title*/)
{
    std::string ext = ExtractFirstExt(filter);
    if (HasExe("zenity"))
    {
        std::string cmd = "zenity --file-selection --title='Open Blueprint'";
        if (!ext.empty()) cmd += " --file-filter='*." + ext + "'";
        cmd += " 2>/dev/null";
        return RunDialog(cmd);
    }
    if (HasExe("kdialog"))
    {
        std::string cmd = "kdialog --getopenfilename . '";
        cmd += ext.empty() ? "*" : ("*." + ext);
        cmd += "' 2>/dev/null";
        return RunDialog(cmd);
    }
    // 无 GUI 工具：返回空（调用方会处理）
    return "";
}

std::string SaveFileDialog(const char* filter, const char* /*title*/, const char* defaultExt)
{
    std::string ext = defaultExt ? defaultExt : ExtractFirstExt(filter);
    if (HasExe("zenity"))
    {
        std::string cmd = "zenity --file-selection --save --confirm-overwrite --title='Save Blueprint'";
        if (!ext.empty()) cmd += " --file-filter='*." + ext + "'";
        cmd += " 2>/dev/null";
        std::string path = RunDialog(cmd);
        if (!path.empty() && !ext.empty())
        {
            std::string suffix = "." + ext;
            if (path.size() < suffix.size() ||
                path.compare(path.size() - suffix.size(), suffix.size(), suffix) != 0)
                path += suffix;
        }
        return path;
    }
    if (HasExe("kdialog"))
    {
        std::string cmd = "kdialog --getsavefilename . '";
        cmd += ext.empty() ? "*" : ("*." + ext);
        cmd += "' 2>/dev/null";
        std::string path = RunDialog(cmd);
        if (!path.empty() && !ext.empty())
        {
            std::string suffix = "." + ext;
            if (path.size() < suffix.size() ||
                path.compare(path.size() - suffix.size(), suffix.size(), suffix) != 0)
                path += suffix;
        }
        return path;
    }
    return "";
}

#endif // __APPLE__ / Linux
#endif // _WIN32

// ============================================================================
// 内部辅助（匿名 namespace）
// ============================================================================

namespace {

// 从路径中提取文件名（不含扩展名）
std::string GetFileBaseName(const std::string& path)
{
    size_t lastSlash = path.find_last_of("/\\");
    std::string name = (lastSlash != std::string::npos) ? path.substr(lastSlash + 1) : path;
    size_t lastDot = name.find_last_of('.');
    if (lastDot != std::string::npos)
        name = name.substr(0, lastDot);
    return name;
}

} // anonymous namespace

// ============================================================================
// 清空编辑器（清空当前活跃文档）
// ============================================================================

void BlueprintEditor::ClearEditor()
{
    if (!ActiveDoc()) return;
    ActiveDoc()->nodes.clear();
    ActiveDoc()->links.clear();
    ActiveDoc()->nodeTouchTime.clear();
    ActiveDoc()->executionLog.clear();
    ActiveDoc()->executionLogText.clear();
    ActiveDoc()->executionLogDirty = false;
    ActiveDoc()->lastExecutionStatus.clear();
    ActiveDoc()->flowLinks.clear();
    ActiveDoc()->nextId = 1;
    ActiveDoc()->isExecuting = false;
}

// ============================================================================
// 新建蓝图（创建新标签页）
// ============================================================================

void BlueprintEditor::NewFile(RTBlueprintClass bpClass)
{
    // 时间防抖：300ms 内多次调用（如双击按钮）只生效一次
    using Clock = std::chrono::steady_clock;
    static Clock::time_point s_lastNewFileTime{};
    auto now = Clock::now();
    if (std::chrono::duration_cast<std::chrono::milliseconds>(now - s_lastNewFileTime).count() < 300)
        return;
    s_lastNewFileTime = now;

    if (m_Project.IsOpen())
    {
        // 有工程：先创建临时文档，弹出编辑器内命名对话框，用户确认后再保存到 assets/
        CreateNewDocument();

        int maxNum = 0;
        for (auto& doc : m_Documents)
        {
            if (doc->untitledName.rfind("Untitled-", 0) == 0)
            {
                int num = std::atoi(doc->untitledName.c_str() + 9);
                if (num > maxNum) maxNum = num;
            }
        }
        ActiveDoc()->untitledName = "Untitled-" + std::to_string(maxNum + 1);
        ActiveDoc()->blueprintClass = bpClass;
        ed::SetCurrentEditor(ActiveDoc()->editorContext);
        SetTitle("Blueprint Editor");

        // 打开命名对话框，预填类型对应的建议文件名
        OpenSaveNameDialog(/*isNew=*/true, bpClass);
        return;
    }

    // 无工程：原有逻辑，直接创建临时文档（首次保存时走系统 Dialog）
    CreateNewDocument();

    // 动态计算下一个可用 Untitled 编号（基于当前已打开的文档）
    int maxNum = 0;
    for (auto& doc : m_Documents)
    {
        if (doc->untitledName.rfind("Untitled-", 0) == 0)
        {
            int num = std::atoi(doc->untitledName.c_str() + 9);
            if (num > maxNum) maxNum = num;
        }
    }
    ActiveDoc()->untitledName = "Untitled-" + std::to_string(maxNum + 1);

    // 设置蓝图类型
    ActiveDoc()->blueprintClass = bpClass;

    // 切换到新文档的编辑器上下文
    ed::SetCurrentEditor(ActiveDoc()->editorContext);

    // 更新窗口标题
    SetTitle("Blueprint Editor");
}

// ============================================================================
// 打开蓝图文件
// ============================================================================

void BlueprintEditor::OpenFile()
{
    std::string path = OpenFileDialog(
        "Blueprint Files (*.bjson)\0*.bjson\0All Files (*.*)\0*.*\0",
        "Open Blueprint"
    );
    
    if (!path.empty())
    {
        DoOpenFile(path);
    }
}

void BlueprintEditor::DoOpenFile(const std::string& path)
{
    ::NodeEditor::Runtime::JsonBlueprintExporter exporter;
    ::NodeEditor::Runtime::ImportResult result;
    result = exporter.importRuntimeFromFile(path);

    if (!result.success)
    {
        if (ActiveDoc())
        {
            ActiveDoc()->executionLog.push_back("[ERROR] Failed to open: " + result.errorMessage);
            ActiveDoc()->executionLogDirty = true;
        }
        return;
    }

    CreateNewDocument();
    ed::SetCurrentEditor(ActiveDoc()->editorContext);
    LoadEditorData(result.data);
    ActiveDoc()->filePath = path;
    
    ActiveDoc()->isDirty = false;
    
    std::string title = "Blueprint Editor - " + GetFileBaseName(ActiveDoc()->filePath);
    SetTitle(title.c_str());

    // 添加到最近文件列表（记录用户实际打开的文件路径）
    AddRecentFile(path);
    
    ActiveDoc()->executionLog.push_back("[INFO] Opened: " + path);
    if (!result.warnings.empty())
    {
        for (const auto& w : result.warnings)
            ActiveDoc()->executionLog.push_back("[WARN] " + w);
    }
    ActiveDoc()->executionLogDirty = true;

    // 如果没有打开工程，自动扫描蓝图同目录下的 FunctionLibrary 文件并注册节点
    // （有工程时由 SyncProjectLibrariesToRegistry 负责）
    if (!m_Project.IsOpen())
    {
        std::string dir;
        const std::string& fp = ActiveDoc()->filePath;
        auto slash = fp.find_last_of("/\\");
        dir = (slash != std::string::npos) ? fp.substr(0, slash) : ".";

        int libCount = ::NodeEditor::Runtime::LoadFunctionLibrary(m_NodeRegistry, dir);
        if (libCount > 0)
        {
            ActiveDoc()->executionLog.push_back(
                "[INFO] Auto-registered " + std::to_string(libCount) +
                " library function(s) from: " + dir);
            // 确保 FunctionLibrary 根分类存在于菜单树
            ::NodeEditor::Runtime::NodeCategory cat;
            cat.id   = "FunctionLibrary";
            cat.name = "FunctionLibrary";
            m_NodeRegistry.registerCategory(cat);
            m_CachedDefCount = 0;  // 强制重建菜单缓存
        }
    }
}

// ============================================================================
// 保存蓝图文件
// ============================================================================

void BlueprintEditor::SaveFile()
{
    if (ActiveDoc()->filePath.empty())
    {
        SaveFileAs();
        return;
    }
    
    DoSaveFile(ActiveDoc()->filePath);
}

void BlueprintEditor::SaveFileAs()
{
    if (m_Project.IsOpen())
    {
        // 有工程：弹出编辑器内命名对话框，限定在 assets/ 目录下
        RTBlueprintClass bpClass = ActiveDoc() ? ActiveDoc()->blueprintClass : RTBlueprintClass::Actor;
        OpenSaveNameDialog(/*isNew=*/false, bpClass);
        return;
    }

    // 无工程：原有系统 Dialog
    std::string path = SaveFileDialog(
        "Blueprint Files (*.bjson)\0*.bjson\0All Files (*.*)\0*.*\0",
        "Save Blueprint As",
        "bjson"
    );

    if (!path.empty())
    {
        ActiveDoc()->filePath = path;
        DoSaveFile(path);
    }
}

void BlueprintEditor::DoSaveFile(const std::string& path)
{
    // 构建完整编辑器数据（含位置信息）
    RTBlueprintData data = BuildFullEditorData();
    
    ::NodeEditor::Runtime::JsonBlueprintExporter exporter;
    // 单文件模式：runtime + editor 合并写入同一个 .bjson 文件
    auto result = exporter.exportEditorFiles(data, path);
    
    if (result.success)
    {
        ActiveDoc()->isDirty = false;
        std::string title = "Blueprint Editor - " + GetFileBaseName(path);
        SetTitle(title.c_str());

        // 添加到最近文件列表
        AddRecentFile(path);

        // 自动将文档加入当前工程（去重由 AddCurrentDocToProject 处理）
        AddCurrentDocToProject();

        // 新建/另存后，通知工程面板展开到该文件所在目录
        if (m_Project.IsOpen())
            m_PendingExpandToPath = m_Project.RelPath(path);
        
        ActiveDoc()->executionLog.push_back("[INFO] Saved: " + path + " ("
            + std::to_string(result.bytesWritten) + " bytes)");
    }
    else
    {
        ActiveDoc()->executionLog.push_back("[ERROR] Save failed: " + result.errorMessage);
    }
    ActiveDoc()->executionLogDirty = true;
}

// ============================================================================
// 构建完整编辑器数据（含节点位置等 Editor 信息）
// ============================================================================

RTBlueprintData BlueprintEditor::BuildFullEditorData()
{
    RTBlueprintData bp = BuildRuntimeData();
    
    // 补充编辑器专属数据：节点位置和尺寸
    for (size_t i = 0; i < ActiveDoc()->nodes.size() && i < bp.nodes.size(); ++i)
    {
        auto& rtNode = bp.nodes[i];
        auto& edNode = ActiveDoc()->nodes[i];
        
        auto pos = ed::GetNodePosition(edNode.ID);
        auto size = ed::GetNodeSize(edNode.ID);
        
        rtNode.position.x = pos.x;
        rtNode.position.y = pos.y;
        rtNode.size.width = size.x;
        rtNode.size.height = size.y;
    }
    
    // 元数据
    if (!ActiveDoc()->filePath.empty())
        bp.metadata.name = GetFileBaseName(ActiveDoc()->filePath);
    else
        bp.metadata.name = "Untitled";
    bp.metadata.description = "Blueprint Editor file";
    // 蓝图类型写入元数据
    bp.metadata.blueprintClass = ActiveDoc()->blueprintClass;

    // 视图状态：保存当前 zoom 和 canvas origin，下次打开时精确恢复
    // ed::ScreenToCanvas(ImVec2(0,0)) 返回屏幕左上角对应的 canvas 坐标（即 view origin）
    // GetCurrentZoom() 返回当前缩放倍率
    // 注：这两个 API 必须在 ed::Begin/End 之间调用才有效；
    //     DoSaveFile 由 UI 层（帧内）调用，此时 ed::Begin/End 已经执行过，结果有效
    bp.viewInfo.viewScale      = ed::GetCurrentZoom();
    ImVec2 origin              = ed::ScreenToCanvas(ImVec2(0.f, 0.f));
    bp.viewInfo.viewPosition.x = origin.x;
    bp.viewInfo.viewPosition.y = origin.y;

    return bp;
}

// ============================================================================
// 从数据恢复编辑器状态
// ============================================================================

void BlueprintEditor::LoadEditorData(const RTBlueprintData& data)
{
    // 检查 schemaVersion（版本高于当前版本时输出警告）
    if (data.metadata.schemaVersion > ::NodeEditor::Runtime::BLUEPRINT_CURRENT_SCHEMA_VERSION)
    {
        ActiveDoc()->executionLog.push_back(
            "[WARN] File schema version " + std::to_string(data.metadata.schemaVersion) +
            " is newer than current version " + std::to_string(::NodeEditor::Runtime::BLUEPRINT_CURRENT_SCHEMA_VERSION) +
            ". Some features may not load correctly.");
    }

    ActiveDoc()->blueprintClass = data.metadata.blueprintClass;
    ActiveDoc()->dependencies = data.metadata.dependencies;

    std::unordered_map<uint64_t, int> nodeIdMap;
    std::unordered_map<uint64_t, int> pinIdMap;
    
    for (const auto& rtNode : data.nodes)
    {
        auto* def = m_NodeRegistry.getNodeDefinition(rtNode.definitionId);
        std::string resolvedDefId = rtNode.definitionId;
        
        int newNodeId = GetNextId();
        nodeIdMap[rtNode.id] = newNodeId;
        
        if (def)
        {
            ImColor color = GetNodeColor(def);
            NodeType ntype = NodeType::Blueprint;

            auto it = def->customProperties.find("editorType");
            if (it != def->customProperties.end())
            {
                if (it->second == "Simple") ntype = NodeType::Simple;
                else if (it->second == "Tree") ntype = NodeType::Tree;
                else if (it->second == "Comment") ntype = NodeType::Comment;
                else if (it->second == "Houdini") ntype = NodeType::Houdini;
            }
            
            ActiveDoc()->nodes.emplace_back(newNodeId, rtNode.name.c_str(), color);
            auto& node = ActiveDoc()->nodes.back();
            node.Type = ntype;
            node.DefinitionId = resolvedDefId;
            
            if (ntype == NodeType::Comment && def->defaultSize.width > 0)
                node.Size = ImVec2(def->defaultSize.width, def->defaultSize.height);
        }
        else
        {
            ActiveDoc()->nodes.emplace_back(newNodeId, rtNode.name.c_str());
            auto& node = ActiveDoc()->nodes.back();
            node.DefinitionId = rtNode.definitionId;
            node.HasError = true;
            node.ErrorMessage = "Node definition '" + rtNode.definitionId + "' not found";
            node.Color = ImColor(180, 0, 0);
        }
        
        auto& node = ActiveDoc()->nodes.back();

        // 恢复折叠状态
        node.isCollapsed = rtNode.isCollapsed;

        // 恢复 Comment 节点颜色
        if (node.Type == NodeType::Comment)
        {
            auto it = rtNode.customProperties.find("__color");
            if (it != rtNode.customProperties.end() && it->second.size() == 6)
            {
                // 用 strtoul 替代 sscanf，避免 MSVC C4996 安全警告
                const char* hex = it->second.c_str();
                char buf[3] = { hex[0], hex[1], '\0' };
                unsigned int r = static_cast<unsigned int>(std::strtoul(buf, nullptr, 16));
                buf[0] = hex[2]; buf[1] = hex[3];
                unsigned int g = static_cast<unsigned int>(std::strtoul(buf, nullptr, 16));
                buf[0] = hex[4]; buf[1] = hex[5];
                unsigned int b = static_cast<unsigned int>(std::strtoul(buf, nullptr, 16));
                node.Color = ImColor((int)r, (int)g, (int)b);
            }
            else
            {
                // 默认白色
                node.Color = ImColor(255, 255, 255);
            }
        }
        
        // 创建引脚
        for (const auto& rtPin : rtNode.pins)
        {
            PinType pt;
            if (rtPin.isExec)
                pt = PinType::Flow;
            else
                pt = MapRTPinDataType(rtPin.dataType, rtPin.isExec);
            
            int newPinId = GetNextId();
            pinIdMap[rtPin.id] = newPinId;
            
            if (rtPin.kind == ::NodeEditor::Runtime::PinKind::Input)
            {
                node.Inputs.emplace_back(newPinId, rtPin.name.c_str(), pt);
                auto& pin = node.Inputs.back();
                
                // 恢复默认值
                if (rtPin.dataType == RTPinDataType::Boolean)
                    pin.BoolValue = rtPin.defaultValue.asBool();
                else if (rtPin.dataType == RTPinDataType::Integer)
                    pin.IntValue = static_cast<int>(rtPin.defaultValue.asInt());
                else if (rtPin.dataType == RTPinDataType::Float)
                    pin.FloatValue = static_cast<float>(rtPin.defaultValue.asFloat());
                else if (rtPin.dataType == RTPinDataType::String)
                    pin.StringValue = rtPin.defaultValue.asString();
                else if (rtPin.dataType == RTPinDataType::Object)
                    pin.ObjectValue = rtPin.defaultValue.asObjectId();
            }
            else
            {
                node.Outputs.emplace_back(newPinId, rtPin.name.c_str(), pt);
            }
        }
        
        // ============================================================
        // 恢复动态输入引脚属性（必须在引脚调和之前，否则 DynamicInputFixedCount 为 0）
        // ============================================================
        if (def)
        {
            auto dynIt = def->customProperties.find("dynamicInputs");
            if (dynIt != def->customProperties.end() && !dynIt->second.empty())
            {
                node.HasDynamicInputs = true;
                node.DynamicInputFixedCount = static_cast<int>(def->inputPins.size());
                const std::string& dynType = dynIt->second;
                if (dynType == "String")       node.DynamicInputPinType = PinType::String;
                else if (dynType == "Float")   node.DynamicInputPinType = PinType::Float;
                else if (dynType == "Int")     node.DynamicInputPinType = PinType::Int;
                else if (dynType == "Bool")    node.DynamicInputPinType = PinType::Bool;
                else if (dynType == "Object")  node.DynamicInputPinType = PinType::Object;
                else if (dynType == "Any")     node.DynamicInputPinType = PinType::Any;
                else if (dynType == "Array")   node.DynamicInputPinType = PinType::Array;
                else if (dynType == "Map")     node.DynamicInputPinType = PinType::Map;
                else                           node.DynamicInputPinType = PinType::String;
            }
        }
        
        // ============================================================
        // 引脚调和（Reconcile）—— UE4 风格
        // 1) 标记孤立引脚（JSON 有但 NodeDef 没有）→ 不删除，保留但标记
        // 2) 补全新增引脚（NodeDef 有但 JSON 没有）→ 追加
        // 3) 标记必须连接的引脚（Delegate 类型等）
        // 4) 如果发生任何变化 → ActiveDoc()->isDirty = true
        // ============================================================
        bool reconcileChanged = false;
        
        // Function.Entry / Function.Return 节点的引脚由 SyncFunctionPinsToNodes 动态管理，
        // 不参与静态定义调和（否则自定义参数引脚会被误判为孤立引脚）
        bool skipReconcile = (node.DefinitionId == "Function.Entry" ||
                              node.DefinitionId == "Function.Return");

        if (def && !skipReconcile)
        {
            // ---- 辅助 lambda：判断引脚名是否存在于 PinDef 列表中 ----
            auto pinExistsInDef = [](const std::string& pinName, PinType pinType,
                                     const std::vector<::NodeEditor::Runtime::PinDefinition>& pinDefs) -> bool
            {
                if (!pinName.empty())
                {
                    for (const auto& pd : pinDefs)
                    {
                        if (pd.name == pinName)
                            return true;
                    }
                    return false;
                }
                // 无名 exec 引脚：只要 def 中还有任何无名 exec 引脚，就视为存在
                if (pinType == PinType::Flow)
                {
                    for (const auto& pd : pinDefs)
                    {
                        if (pd.name.empty() && pd.isExec)
                            return true;
                    }
                }
                return false;
            };

            // --- 1) 标记孤立输入引脚（NodeDef 中已不存在的引脚）---
            {
                size_t fixedCount = node.HasDynamicInputs
                    ? (size_t)node.DynamicInputFixedCount
                    : node.Inputs.size();
                for (size_t i = 0; i < fixedCount && i < node.Inputs.size(); ++i)
                {
                    if (!pinExistsInDef(node.Inputs[i].Name, node.Inputs[i].Type, def->inputPins))
                    {
                        node.Inputs[i].IsOrphaned = true;
                        reconcileChanged = true;
                    }
                }
            }

            // --- 2) 标记孤立输出引脚 ---
            for (size_t i = 0; i < node.Outputs.size(); ++i)
            {
                if (!pinExistsInDef(node.Outputs[i].Name, node.Outputs[i].Type, def->outputPins))
                {
                    node.Outputs[i].IsOrphaned = true;
                    reconcileChanged = true;
                }
            }

            // --- 3) 补全缺失的输入引脚 ---
            for (size_t di = 0; di < def->inputPins.size(); ++di)
            {
                const auto& pinDef = def->inputPins[di];
                bool found = false;
                for (const auto& existingPin : node.Inputs)
                {
                    if (!existingPin.IsOrphaned && existingPin.Name == pinDef.name)
                    {
                        found = true;
                        break;
                    }
                }
                if (!found && pinDef.name.empty())
                {
                    size_t emptyNameIdx = 0;
                    for (size_t k = 0; k <= di; ++k)
                    {
                        if (def->inputPins[k].name.empty() && def->inputPins[k].isExec)
                            ++emptyNameIdx;
                    }
                    size_t existingEmptyCount = 0;
                    for (const auto& existingPin : node.Inputs)
                    {
                        if (!existingPin.IsOrphaned && existingPin.Name.empty() && existingPin.Type == PinType::Flow)
                            ++existingEmptyCount;
                    }
                    if (emptyNameIdx <= existingEmptyCount)
                        found = true;
                }
                if (!found)
                {
                    PinType pt = MapRTPinDataType(pinDef.dataType, pinDef.isExec);
                    int newPinId = GetNextId();
                    // 计算插入位置：跳过孤立引脚，按 def 顺序插入
                    size_t insertPos = 0;
                    size_t defIdx = 0;
                    for (size_t j = 0; j < node.Inputs.size() && defIdx < di; ++j)
                    {
                        if (!node.Inputs[j].IsOrphaned)
                            ++defIdx;
                        insertPos = j + 1;
                    }
                    if (insertPos > node.Inputs.size())
                        insertPos = node.Inputs.size();
                    node.Inputs.emplace(node.Inputs.begin() + insertPos, newPinId, pinDef.name.c_str(), pt);
                    auto& pin = node.Inputs[insertPos];
                    if (pinDef.dataType == RTPinDataType::Boolean)
                        pin.BoolValue = pinDef.defaultValue.asBool();
                    else if (pinDef.dataType == RTPinDataType::Integer)
                        pin.IntValue = pinDef.defaultValue.asInt();
                    else if (pinDef.dataType == RTPinDataType::Float)
                        pin.FloatValue = static_cast<float>(pinDef.defaultValue.asFloat());
                    else if (pinDef.dataType == RTPinDataType::String)
                        pin.StringValue = pinDef.defaultValue.asString();
                    else if (pinDef.dataType == RTPinDataType::Object)
                        pin.ObjectValue = pinDef.defaultValue.asObjectId();
                    reconcileChanged = true;
                }
            }
            
            // --- 4) 补全缺失的输出引脚 ---
            for (size_t di = 0; di < def->outputPins.size(); ++di)
            {
                const auto& pinDef = def->outputPins[di];
                bool found = false;
                for (const auto& existingPin : node.Outputs)
                {
                    if (!existingPin.IsOrphaned && existingPin.Name == pinDef.name)
                    {
                        found = true;
                        break;
                    }
                }
                if (!found && pinDef.name.empty())
                {
                    size_t emptyNameIdx = 0;
                    for (size_t k = 0; k <= di; ++k)
                    {
                        if (def->outputPins[k].name.empty() && def->outputPins[k].isExec)
                            ++emptyNameIdx;
                    }
                    size_t existingEmptyCount = 0;
                    for (const auto& existingPin : node.Outputs)
                    {
                        if (!existingPin.IsOrphaned && existingPin.Name.empty() && existingPin.Type == PinType::Flow)
                            ++existingEmptyCount;
                    }
                    if (emptyNameIdx <= existingEmptyCount)
                        found = true;
                }
                if (!found)
                {
                    PinType pt = MapRTPinDataType(pinDef.dataType, pinDef.isExec);
                    int newPinId = GetNextId();
                    size_t insertPos = 0;
                    size_t defIdx = 0;
                    for (size_t j = 0; j < node.Outputs.size() && defIdx < di; ++j)
                    {
                        if (!node.Outputs[j].IsOrphaned)
                            ++defIdx;
                        insertPos = j + 1;
                    }
                    if (insertPos > node.Outputs.size())
                        insertPos = node.Outputs.size();
                    node.Outputs.emplace(node.Outputs.begin() + insertPos, newPinId, pinDef.name.c_str(), pt);
                    reconcileChanged = true;
                }
            }

            // --- 5) 标记必须连接的引脚（Delegate/Function 类型输入引脚）---
            for (size_t i = 0; i < def->inputPins.size() && i < node.Inputs.size(); ++i)
            {
                auto pinTypeIt = def->inputPins[i].customProperties.find("pinType");
                if (pinTypeIt != def->inputPins[i].customProperties.end() &&
                    (pinTypeIt->second == "Delegate" || pinTypeIt->second == "Function"))
                {
                    // 找到对应的编辑器引脚（按名称匹配）
                    for (auto& edPin : node.Inputs)
                    {
                        if (edPin.Name == def->inputPins[i].name && !edPin.IsOrphaned)
                        {
                            edPin.IsRequired = true;
                            break;
                        }
                    }
                }
            }

            // --- 6) 恢复 hiddenWhen 声明式可见性规则 ---
            for (size_t i = 0; i < def->inputPins.size(); ++i)
            {
                auto hwIt = def->inputPins[i].customProperties.find("hiddenWhen");
                if (hwIt != def->inputPins[i].customProperties.end())
                {
                    for (auto& edPin : node.Inputs)
                    {
                        if (edPin.Name == def->inputPins[i].name && !edPin.IsOrphaned)
                        {
                            edPin.HiddenWhen = hwIt->second;
                            break;
                        }
                    }
                }
            }
            for (size_t i = 0; i < def->outputPins.size(); ++i)
            {
                auto hwIt = def->outputPins[i].customProperties.find("hiddenWhen");
                if (hwIt != def->outputPins[i].customProperties.end())
                {
                    for (auto& edPin : node.Outputs)
                    {
                        if (edPin.Name == def->outputPins[i].name && !edPin.IsOrphaned)
                        {
                            edPin.HiddenWhen = hwIt->second;
                            break;
                        }
                    }
                }
            }

            // --- 如果有孤立引脚，标记节点有警告 ---
            bool hasOrphaned = false;
            for (const auto& p : node.Inputs)
                if (p.IsOrphaned) { hasOrphaned = true; break; }
            if (!hasOrphaned)
                for (const auto& p : node.Outputs)
                    if (p.IsOrphaned) { hasOrphaned = true; break; }
            if (hasOrphaned)
            {
                node.HasError = true;
                node.ErrorMessage = "Node has orphaned pins (removed from definition)";
            }
        }
        
        // 如果引脚调和发生了变化，标记文件为"未保存"
        if (reconcileChanged)
            ActiveDoc()->isDirty = true;
        
        // 应用特殊引脚类型
        if (def)
            FixupSpecialPinTypes(&node, def);
    }
    
    BuildNodes();

    // 创建链接
    for (const auto& rtLink : data.links)
    {
        auto startIt = pinIdMap.find(rtLink.startPinId);
        auto endIt = pinIdMap.find(rtLink.endPinId);
        
        if (startIt != pinIdMap.end() && endIt != pinIdMap.end())
        {
            ed::PinId startPinId(startIt->second);
            ed::PinId endPinId(endIt->second);
            
            ActiveDoc()->links.emplace_back(Link(GetNextId(), startPinId, endPinId));
            
            // 设置链接颜色（Any 引脚使用对端类型颜色）
            auto* startPin = FindPin(startPinId);
            auto* endPin   = FindPin(endPinId);
            if (startPin)
                ActiveDoc()->links.back().Color = GetIconColor(GetLinkColor(startPin, endPin));
        }
    }

    // 链接创建完成后，同步 Function.Entry/Return 节点引脚与函数定义一致
    // （SyncFunctionPinsToNodes 会复用同名同类型的旧引脚，保持现有连线有效）
    for (const auto& func : ActiveDoc()->functions)
        SyncFunctionPinsToNodes(func);
    
    // 保存加载数据和 ID 映射，用于延迟设置节点位置
    ActiveDoc()->pendingLoadData = data;
    ActiveDoc()->needSetNodePositions = true;

    // 记录文件中保存的视图状态（origin + scale）
    // viewScale > 0 且 origin 任意值都算有效（包括 0,0）
    ActiveDoc()->hasSavedView    = (data.viewInfo.viewScale > 0.f);
    ActiveDoc()->savedViewOrigin = ImVec2(data.viewInfo.viewPosition.x, data.viewInfo.viewPosition.y);
    ActiveDoc()->savedViewScale  = (data.viewInfo.viewScale > 0.f) ? data.viewInfo.viewScale : 1.f;

    // 保存 nodeIdMap 到成员中以便 OnFrame 使用
    // 直接在这里使用：在编辑器初始化后设置位置
    // 注意：ed::SetNodePosition 需要在 ed::Begin/End 之间调用，
    // 所以我们保存映射关系，在下一帧的 OnFrame 中设置位置
    // 暂时用 customProperties 传递映射
    for (const auto& rtNode : data.nodes)
    {
        auto it = nodeIdMap.find(rtNode.id);
        if (it != nodeIdMap.end())
        {
            // 在 PendingLoadData 中记录新 ID
            for (auto& pendNode : ActiveDoc()->pendingLoadData.nodes)
            {
                if (pendNode.id == rtNode.id)
                {
                    pendNode.customProperties["__newEditorId"] = std::to_string(it->second);
                    break;
                }
            }
        }
    }

    // 加载变量定义
    ActiveDoc()->variables = data.variables;

    // 加载函数定义
    ActiveDoc()->functions = data.functions;
}

// ============================================================================
// 最近文件列表
// ============================================================================

// 辅助函数：规范化路径（统一使用 / 分隔符）
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
        size_t lastSlash = path.find_last_of("/\\");
        std::string displayName = (lastSlash != std::string::npos) ? path.substr(lastSlash + 1) : path;
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
        size_t lastSlash = path.find_last_of("/\\");
        std::string name = (lastSlash != std::string::npos) ? path.substr(lastSlash + 1) : path;
        // 去掉 .bproj 后缀，显示更干净
        if (name.size() > 6 && name.substr(name.size() - 6) == ".bproj")
            name = name.substr(0, name.size() - 6);

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

// ============================================================================
// 工程内命名对话框 — OpenSaveNameDialog / ResolveSaveDialogPath
// ============================================================================

void BlueprintEditor::OpenSaveNameDialog(bool isNew, RTBlueprintClass bpClass)
{
    auto& d = m_SaveNameDialog;
    d.open    = true;
    d.isNew   = isNew;
    d.bpClass = bpClass;
    d.errorMsg.clear();

    // 预填默认文件名：当前文档已有路径时，取相对于 assets/ 的路径（去掉扩展名），
    // 否则填空让用户自行输入
    std::memset(d.inputBuf, 0, sizeof(d.inputBuf));
    if (!isNew && ActiveDoc() && !ActiveDoc()->filePath.empty())
    {
        std::string assetsDir = (fs::path(m_Project.projectDir) / "assets").string();
        fs::path fp(ActiveDoc()->filePath);
        // 去掉 .bjson 扩展名
        std::string stem = fp.filename().string();
        if (stem.size() > 6 && stem.substr(stem.size() - 6) == ".bjson")
            stem = stem.substr(0, stem.size() - 6);

        // 若当前文件已在 assets 下，计算子目录路径
        try {
            fs::path rel = fs::relative(fp.parent_path(), assetsDir);
            std::string relStr = rel.string();
            if (!relStr.empty() && relStr != "." && relStr.find("..") == std::string::npos)
                stem = (fs::path(relStr) / stem).string();
        } catch (...) {}

#ifdef _MSC_VER
        strncpy_s(d.inputBuf, sizeof(d.inputBuf), stem.c_str(), sizeof(d.inputBuf) - 1);
#else
        std::snprintf(d.inputBuf, sizeof(d.inputBuf), "%s", stem.c_str());
#endif
    }
}

std::string BlueprintEditor::ResolveSaveDialogPath() const
{
    const auto& d = m_SaveNameDialog;
    if (!m_Project.IsOpen()) return "";

    std::string raw = d.inputBuf;
    // 去掉首尾空白
    while (!raw.empty() && (raw.front() == ' ' || raw.front() == '\t')) raw.erase(raw.begin());
    while (!raw.empty() && (raw.back()  == ' ' || raw.back()  == '\t')) raw.pop_back();
    if (raw.empty()) return "";

    // 规范化分隔符（Windows 反斜杠 → 正斜杠）
    for (char& c : raw) if (c == '\\') c = '/';

    // 不允许绝对路径或上溯
    if (raw.front() == '/' || raw.find("..") != std::string::npos)
        return "";

    // 拼接 assets/ 根目录
    fs::path assetsDir = fs::path(m_Project.projectDir) / "assets";
    fs::path full      = assetsDir / raw;

    // 追加扩展名（去掉用户自己加的，统一加 .bjson）
    std::string fullStr = full.string();
    if (fullStr.size() > 6 && fullStr.substr(fullStr.size() - 6) == ".bjson")
        fullStr = fullStr.substr(0, fullStr.size() - 6);


    fullStr += ".bjson";
    return fullStr;
}

// ============================================================================
// 文件拖拽打开（WM_DROPFILES 回调）
// ============================================================================

void BlueprintEditor::OnDropFile(const std::string& filePath)
{
    if (filePath.empty()) return;

    auto hasSuffix = [](const std::string& s, const std::string& suf) {
        if (s.size() < suf.size()) return false;
        std::string t = s.substr(s.size() - suf.size());
        for (auto& c : t) c = static_cast<char>(::tolower((unsigned char)c));
        return t == suf;
    };

    if (hasSuffix(filePath, ".bproj"))
    {
        // 拖拽工程文件：打开工程
        BpProject proj;
        if (LoadBpProject(proj, filePath))
        {
            CloseProject();
            m_Project = std::move(proj);
            SyncProjectLibrariesToRegistry();
            SetTitle(("Blueprint Editor - [" + m_Project.name + "]").c_str());
            AddRecentProject(m_Project.filePath);
        }
    }
    else if (hasSuffix(filePath, ".bjson"))
    {
        DoOpenFile(filePath);
    }
    // 其他格式静默忽略
}
