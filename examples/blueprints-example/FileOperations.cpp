// FileOperations.cpp -- 蓝图文件操作（新建/打开/保存）
#include "BlueprintEditor.h"

#ifdef _WIN32
#include <windows.h>
#include <commdlg.h>
#include <shlobj.h>
#pragma comment(lib, "comdlg32.lib")
#pragma comment(lib, "shell32.lib")
#endif

#include <fstream>
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
    m_Nodes.clear();
    m_Links.clear();
    m_NodeTouchTime.clear();
    m_ExecutionLog.clear();
    m_ExecutionLogText.clear();
    m_ExecutionLogDirty = false;
    m_LastExecutionStatus.clear();
    m_FlowLinks.clear();
    m_NextId = 1;
    m_IsExecuting = false;
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
                m_ExecutionLog.push_back("[ERROR] Failed to open editor file: " + result.errorMessage);
                m_ExecutionLogDirty = true;
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
        m_CurrentFilePath = runtimePath;
    }
    else
    {
        // 传统逻辑：从 .json 加载，可选合并 .editor.json
        std::string editorPath = GetEditorFilePath(path);
        
        // 检查 editor 文件是否存在
        std::ifstream editorCheck(editorPath);
        bool hasEditorFile = editorCheck.good();
        editorCheck.close();
        
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
                m_ExecutionLog.push_back("[ERROR] Failed to open: " + result.errorMessage);
                m_ExecutionLogDirty = true;
            }
            return;
        }
        
        // 创建新标签页
        CreateNewDocument();
        ed::SetCurrentEditor(ActiveDoc()->editorContext);
        
        // 加载数据到编辑器
        LoadEditorData(result.data);
        
        m_CurrentFilePath = path;
    }
    
    m_IsDirty = false;
    
    std::string title = "Blueprint Editor - " + GetFileBaseName(m_CurrentFilePath);
    SetTitle(title.c_str());
    
    m_ExecutionLog.push_back("[INFO] Opened: " + path);
    if (!result.warnings.empty())
    {
        for (const auto& w : result.warnings)
            m_ExecutionLog.push_back("[WARN] " + w);
    }
    m_ExecutionLogDirty = true;
}

// ============================================================================
// 保存蓝图文件
// ============================================================================

void BlueprintEditor::SaveFile()
{
    if (m_CurrentFilePath.empty())
    {
        SaveFileAs();
        return;
    }
    
    DoSaveFile(m_CurrentFilePath);
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
        m_CurrentFilePath = path;
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
        m_IsDirty = false;
        std::string title = "Blueprint Editor - " + GetFileBaseName(path);
        SetTitle(title.c_str());
        
        m_ExecutionLog.push_back("[INFO] Saved: " + path + " (" + 
            std::to_string(result.runtimeBytes) + " + " + 
            std::to_string(result.editorBytes) + " bytes)");
    }
    else
    {
        m_ExecutionLog.push_back("[ERROR] Save failed: " + result.errorMessage);
    }
    m_ExecutionLogDirty = true;
}

// ============================================================================
// 构建完整编辑器数据（含节点位置等 Editor 信息）
// ============================================================================

RTBlueprintData BlueprintEditor::BuildFullEditorData()
{
    RTBlueprintData bp = BuildRuntimeData();
    
    // 补充编辑器专属数据：节点位置和尺寸
    for (size_t i = 0; i < m_Nodes.size() && i < bp.nodes.size(); ++i)
    {
        auto& rtNode = bp.nodes[i];
        auto& edNode = m_Nodes[i];
        
        auto pos = ed::GetNodePosition(edNode.ID);
        auto size = ed::GetNodeSize(edNode.ID);
        
        rtNode.position.x = pos.x;
        rtNode.position.y = pos.y;
        rtNode.size.width = size.x;
        rtNode.size.height = size.y;
    }
    
    // 元数据
    if (!m_CurrentFilePath.empty())
        bp.metadata.name = GetFileBaseName(m_CurrentFilePath);
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
            auto allDefs = m_NodeRegistry.getAllNodeDefinitions();
            for (const auto& d : allDefs)
            {
                if (d.name == rtNode.definitionId || d.name == rtNode.name)
                {
                    def = m_NodeRegistry.getNodeDefinition(d.id);
                    resolvedDefId = d.id;
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
            
            m_Nodes.emplace_back(newNodeId, rtNode.name.c_str(), color);
            auto& node = m_Nodes.back();
            node.Type = ntype;
            node.DefinitionId = resolvedDefId;
            
            if (ntype == NodeType::Comment && def->defaultSize.width > 0)
                node.Size = ImVec2(def->defaultSize.width, def->defaultSize.height);
        }
        else
        {
            // 定义未找到 → 标记为错误节点（UE4 风格）
            m_Nodes.emplace_back(newNodeId, rtNode.name.c_str());
            auto& node = m_Nodes.back();
            node.DefinitionId = rtNode.definitionId;
            node.HasError = true;
            node.ErrorMessage = "Node definition '" + rtNode.definitionId + "' not found";
            node.Color = ImColor(180, 0, 0); // 深红色表示错误
        }
        
        auto& node = m_Nodes.back();
        
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
        // 引脚调和（Reconcile）—— UE4 风格
        // 1) 标记孤立引脚（JSON 有但 NodeDef 没有）→ 不删除，保留但标记
        // 2) 补全新增引脚（NodeDef 有但 JSON 没有）→ 追加
        // 3) 标记必须连接的引脚（Delegate 类型等）
        // 4) 如果发生任何变化 → m_IsDirty = true
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
            m_IsDirty = true;
        
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
            
            m_Links.emplace_back(Link(GetNextId(), startPinId, endPinId));
            
            // 设置链接颜色
            auto* startPin = FindPin(startPinId);
            if (startPin)
                m_Links.back().Color = GetIconColor(startPin->Type);
        }
    }
    
    // 保存加载数据和 ID 映射，用于延迟设置节点位置
    m_PendingLoadData = data;
    m_NeedSetNodePositions = true;
    
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
            for (auto& pendNode : m_PendingLoadData.nodes)
            {
                if (pendNode.id == rtNode.id)
                {
                    pendNode.customProperties["__newEditorId"] = std::to_string(it->second);
                    break;
                }
            }
        }
    }
}
