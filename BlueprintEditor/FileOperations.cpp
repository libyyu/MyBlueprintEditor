// FileOperations.cpp -- 蓝图文件操作（新建/打开/保存）
#include "BlueprintEditor.h"

#ifdef _WIN32
#include <windows.h>
#include <commdlg.h>
#include <shlobj.h>
#pragma comment(lib, "comdlg32.lib")
#pragma comment(lib, "shell32.lib")
#endif

#include <sstream>

// ============================================================================
// 平台文件对话框
// ============================================================================

namespace {

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
// Linux/Mac 平台（简单实现）
std::string OpenFileDialog(const char*, const char*) { return ""; }
std::string SaveFileDialog(const char*, const char*, const char*) { return ""; }
#endif

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

// 获取 editor.json 路径
std::string GetEditorFilePath(const std::string& runtimePath)
{
    // blueprint.json -> blueprint.editor.json
    size_t lastDot = runtimePath.find_last_of('.');
    if (lastDot != std::string::npos)
        return runtimePath.substr(0, lastDot) + ".editor.json";
    return runtimePath + ".editor.json";
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

void BlueprintEditor::NewFile()
{
    CreateNewDocument();

    // 切换到新文档的编辑器上下文
    ed::SetCurrentEditor(ActiveDoc()->editorContext);

    // 更新窗口标题
    SetTitle("Blueprint Editor - [New]");
}

// ============================================================================
// 打开蓝图文件
// ============================================================================

void BlueprintEditor::OpenFile()
{
    std::string path = OpenFileDialog(
        "Blueprint Files (*.json)\0*.json\0Editor Files (*.editor.json)\0*.editor.json\0All Files (*.*)\0*.*\0",
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
    
    // 判断用户选择的是 .editor.json 还是 .json
    bool isEditorFile = (path.size() > 12 && path.substr(path.size() - 12) == ".editor.json");
    
    if (isEditorFile)
    {
        // 直接从 .editor.json 加载（包含嵌入的 runtime 数据）
        result = exporter.importFromEditorFile(path);
        
        if (!result.success)
        {
            if (ActiveDoc())
            {
                ActiveDoc()->executionLog.push_back("[ERROR] Failed to open editor file: " + result.errorMessage);
                ActiveDoc()->executionLogDirty = true;
            }
            return;
        }
        
        // 创建新标签页
        CreateNewDocument();
        ed::SetCurrentEditor(ActiveDoc()->editorContext);
        
        // 加载数据到编辑器
        LoadEditorData(result.data);
        
        // 保存对应的 runtime 文件路径（去掉 .editor 部分）
        std::string runtimePath = path.substr(0, path.size() - 12) + ".json";
        ActiveDoc()->filePath = runtimePath;
    }
    else
    {
        // 传统逻辑：从 .json 加载，可选合并 .editor.json
        std::string editorPath = GetEditorFilePath(path);
        
        // 检查 editor 文件是否存在
        auto fs = ::NodeEditor::Runtime::GetDefaultFileSystem();
        bool hasEditorFile = fs->FileExists(editorPath);
        
        if (hasEditorFile)
        {
            result = exporter.importEditorFromFiles(path, editorPath);
        }
        else
        {
            // 只有 Runtime 文件
            result = exporter.importRuntimeFromFile(path);
        }
        
        if (!result.success)
        {
            if (ActiveDoc())
            {
                ActiveDoc()->executionLog.push_back("[ERROR] Failed to open: " + result.errorMessage);
                ActiveDoc()->executionLogDirty = true;
            }
            return;
        }
        
        // 创建新标签页
        CreateNewDocument();
        ed::SetCurrentEditor(ActiveDoc()->editorContext);
        
        // 加载数据到编辑器
        LoadEditorData(result.data);
        
        ActiveDoc()->filePath = path;
    }
    
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
    std::string path = SaveFileDialog(
        "Blueprint Files (*.json)\0*.json\0All Files (*.*)\0*.*\0",
        "Save Blueprint As",
        "json"
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
    std::string editorPath = GetEditorFilePath(path);
    
    auto result = exporter.exportEditorFiles(data, path, editorPath);
    
    if (result.success)
    {
        ActiveDoc()->isDirty = false;
        std::string title = "Blueprint Editor - " + GetFileBaseName(path);
        SetTitle(title.c_str());

        // 添加到最近文件列表
        AddRecentFile(path);
        
        ActiveDoc()->executionLog.push_back("[INFO] Saved: " + path + " (" + 
            std::to_string(result.runtimeBytes) + " + " + 
            std::to_string(result.editorBytes) + " bytes)");
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

    // 映射旧 ID -> 新 ID
    std::unordered_map<uint64_t, int> nodeIdMap;  // old nodeId -> new nodeId
    std::unordered_map<uint64_t, int> pinIdMap;   // old pinId -> new pinId
    
    // 创建节点
    for (const auto& rtNode : data.nodes)
    {
        // 尝试通过 definitionId 创建节点
        auto* def = m_NodeRegistry.getNodeDefinition(rtNode.definitionId);
        
        // 如果 definitionId 找不到，可能是旧文件用了显示名（如 "For Loop" 而非 "ForLoop"）
        // 尝试按 name 在注册表中反查
        std::string resolvedDefId = rtNode.definitionId;
        if (!def)
        {
            auto& allDefs = m_NodeRegistry.getAllNodeDefinitions();
            for (const auto* d : allDefs)
            {
                if (d->name == rtNode.definitionId || d->name == rtNode.name)
                {
                    def = m_NodeRegistry.getNodeDefinition(d->id);
                    resolvedDefId = d->id;
                    break;
                }
            }
        }
        
        int newNodeId = GetNextId();
        nodeIdMap[rtNode.id] = newNodeId;
        
        if (def)
        {
            // 使用定义创建节点
            ImColor color(255, 255, 255);
            NodeType ntype = NodeType::Blueprint;
            if (!def->color.empty())
            {
                // 解析颜色（简单实现）
                std::string s = def->color;
                if (!s.empty() && s[0] == '#') s = s.substr(1);
                if (s.size() == 6)
                {
                    unsigned int r = 0, g = 0, b = 0;
                    for (int i = 0; i < 6; ++i)
                    {
                        char c = s[i];
                        unsigned int v = 0;
                        if (c >= '0' && c <= '9') v = c - '0';
                        else if (c >= 'a' && c <= 'f') v = 10 + c - 'a';
                        else if (c >= 'A' && c <= 'F') v = 10 + c - 'A';
                        if (i < 2) r = r * 16 + v;
                        else if (i < 4) g = g * 16 + v;
                        else b = b * 16 + v;
                    }
                    color = ImColor((int)r, (int)g, (int)b);
                }
            }
            
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
            // 定义未找到 → 标记为错误节点（UE4 风格）
            ActiveDoc()->nodes.emplace_back(newNodeId, rtNode.name.c_str());
            auto& node = ActiveDoc()->nodes.back();
            node.DefinitionId = rtNode.definitionId;
            node.HasError = true;
            node.ErrorMessage = "Node definition '" + rtNode.definitionId + "' not found";
            node.Color = ImColor(180, 0, 0); // 深红色表示错误
        }
        
        auto& node = ActiveDoc()->nodes.back();

        // 恢复折叠状态：v2+ 读顶层 isCollapsed；v1 兼容读 customProperties["__collapsed"]
        // （schema 迁移通常在 importRuntimeFromString 里已处理，此处 fallback 仅防御性保留）
        {
            node.isCollapsed = rtNode.isCollapsed;
            if (!node.isCollapsed)
            {
                auto it = rtNode.customProperties.find("__collapsed");
                if (it != rtNode.customProperties.end() && it->second == "1")
                    node.isCollapsed = true;
            }
        }

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
        
        if (def)
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
    
    // 保存加载数据和 ID 映射，用于延迟设置节点位置
    ActiveDoc()->pendingLoadData = data;
    ActiveDoc()->needSetNodePositions = true;
    
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

// 辅助函数：获取路径的"基础路径"用于去重比较
// .editor.json 和 .json 视为同一文件
static std::string GetCanonicalPath(const std::string& normalizedPath)
{
    const std::string editorSuffix = ".editor.json";
    if (normalizedPath.size() > editorSuffix.size() &&
        normalizedPath.substr(normalizedPath.size() - editorSuffix.size()) == editorSuffix)
    {
        return normalizedPath.substr(0, normalizedPath.size() - editorSuffix.size()) + ".json";
    }
    return normalizedPath;
}

void BlueprintEditor::AddRecentFile(const std::string& path)
{
    // 规范化路径
    std::string normalized = NormalizePath(path);
    std::string canonical = GetCanonicalPath(normalized);

    // 移除同一文件的旧记录（.editor.json 和 .json 视为同一文件）
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

        // 去重检查（.editor.json 和 .json 视为同一文件）
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
