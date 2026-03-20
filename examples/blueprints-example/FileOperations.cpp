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
// 清空编辑器
// ============================================================================

void BlueprintEditor::ClearEditor()
{
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
// 新建蓝图
// ============================================================================

void BlueprintEditor::NewFile()
{
    ClearEditor();
    m_CurrentFilePath.clear();
    m_IsDirty = false;
    
    // 重新设置编辑器标题
    SetTitle("Blueprint Editor - [New]");
    
    ed::NavigateToContent();
}

// ============================================================================
// 打开蓝图文件
// ============================================================================

void BlueprintEditor::OpenFile()
{
    std::string path = OpenFileDialog(
        "Blueprint Files (*.json)\0*.json\0All Files (*.*)\0*.*\0",
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
    
    std::string editorPath = GetEditorFilePath(path);
    
    // 尝试加载 Runtime + Editor 双文件
    ::NodeEditor::Runtime::ImportResult result;
    
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
        m_ExecutionLog.push_back("[ERROR] Failed to open: " + result.errorMessage);
        m_ExecutionLogDirty = true;
        return;
    }
    
    // 清空当前编辑器
    ClearEditor();
    
    // 加载数据到编辑器
    LoadEditorData(result.data);
    
    m_CurrentFilePath = path;
    m_IsDirty = false;
    
    std::string title = "Blueprint Editor - " + GetFileBaseName(path);
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
            
            if (ntype == NodeType::Comment && def->defaultSize.width > 0)
                node.Size = ImVec2(def->defaultSize.width, def->defaultSize.height);
        }
        else
        {
            // 定义未找到，创建一个通用节点
            m_Nodes.emplace_back(newNodeId, rtNode.name.c_str());
            auto& node = m_Nodes.back();
            
            // 检查是否是 Execute Blueprint 节点
            if (rtNode.definitionId == "ExecuteBlueprint")
            {
                ImColor color(255, 165, 0); // 橙色
                node.Color = color;
            }
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
            }
            else
            {
                node.Outputs.emplace_back(newPinId, rtPin.name.c_str(), pt);
            }
        }
        
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
