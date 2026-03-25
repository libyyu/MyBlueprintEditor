// Runtime/BlueprintExporter.cpp - 蓝图数据导出器实现
//
// 导出策略:
//   Runtime 文件:  节点(id/definitionId/name/pins/isEnabled/nodeData/customProperties)、链接、变量(核心)、元数据(name/desc/version)
//   Editor 文件:   节点(position/size/isCollapsed) 通过 nodeId 关联、注释、视图信息、元数据(author/timestamps)、变量(category/tooltip)

#include "BlueprintExporter.h"
#include "../Utils/Json/crude_json.h"
#include <sstream>
#include <iomanip>
#include <ctime>
#include <iostream>
#include <algorithm>
#include <unordered_map>
#include <unordered_set>
namespace NodeEditor {
namespace Runtime {

// ============================================================================
// JSON 辅助函数
// ============================================================================

namespace {

std::string escapeJson(const std::string& str)
{
    std::ostringstream oss;
    for (char c : str)
    {
        switch (c)
        {
        case '"': oss << "\\\""; break;
        case '\\': oss << "\\\\"; break;
        case '\b': oss << "\\b"; break;
        case '\f': oss << "\\f"; break;
        case '\n': oss << "\\n"; break;
        case '\r': oss << "\\r"; break;
        case '\t': oss << "\\t"; break;
        default:
            if ('\x00' <= c && c <= '\x1f')
            {
                oss << "\\u" << std::hex << std::setw(4) << std::setfill('0') << (int)c;
            }
            else
            {
                oss << c;
            }
        }
    }
    return oss.str();
}

std::string indentJson(int level, int spaces = 4)
{
    return std::string(level * spaces, ' ');
}

// JSON 字段安全读取辅助函数（消除 importRuntimeFromString / importEditorFromStrings 中的重复 lambda）
double getJsonNumber(const crude_json::value& obj, const char* key, double defaultVal = 0.0)
{
    if (obj.contains(key) && obj[key].type() == crude_json::type_t::number)
        return obj[key].get<double>();
    return defaultVal;
}

std::string getJsonString(const crude_json::value& obj, const char* key)
{
    if (obj.contains(key) && obj[key].type() == crude_json::type_t::string)
        return obj[key].get<std::string>();
    return "";
}

bool getJsonBool(const crude_json::value& obj, const char* key, bool defaultVal = false)
{
    if (obj.contains(key) && obj[key].type() == crude_json::type_t::boolean)
        return obj[key].get<bool>();
    return defaultVal;
}

} // anonymous namespace

// ============================================================================
// JsonBlueprintExporter — Runtime 文件导出
// ============================================================================

std::string JsonBlueprintExporter::exportRuntimeToString(const BlueprintData& data, const ExportOptions& options) const
{
    std::ostringstream oss;
    int indentLevel = 0;
    bool pretty = options.prettyPrint;
    
    auto writeIndent = [&]() {
        if (pretty) oss << indentJson(indentLevel, options.indent);
    };
    
    auto writeNewline = [&]() {
        if (pretty) oss << "\n";
    };
    
    // 开始根对象
    oss << "{";
    writeNewline();
    indentLevel++;
    
    // 文件类型标识
    writeIndent();
    oss << "\"fileType\": \"runtime\",";
    writeNewline();
    
    // ================================================================
    // 元数据（运行时只保留 name/description/version）
    // ================================================================
    if (options.includeMetadata)
    {
        writeIndent();
        oss << "\"metadata\": {";
        writeNewline();
        indentLevel++;
        
        writeIndent();
        oss << "\"name\": \"" << escapeJson(data.metadata.name) << "\",";
        writeNewline();
        
        writeIndent();
        oss << "\"description\": \"" << escapeJson(data.metadata.description) << "\",";
        writeNewline();
        
        writeIndent();
        oss << "\"version\": \"" << escapeJson(data.metadata.version) << "\"";
        
        writeNewline();
        indentLevel--;
        writeIndent();
        oss << "},";
        writeNewline();
    }
    
    // ================================================================
    // 节点列表（运行时数据：不含 position/size/isCollapsed）
    // ================================================================
    writeIndent();
    oss << "\"nodes\": [";
    writeNewline();
    indentLevel++;
    
    bool firstNode = true;
    for (size_t i = 0; i < data.nodes.size(); ++i)
    {
        const auto& node = data.nodes[i];
        
        // 应用节点过滤器
        if (options.nodeFilter && !options.nodeFilter(node))
            continue;
        
        if (!firstNode) { oss << ","; writeNewline(); }
        firstNode = false;
        
        writeIndent();
        oss << "{";
        writeNewline();
        indentLevel++;
        
        // 节点基础信息
        writeIndent();
        oss << "\"id\": " << node.id << ",";
        writeNewline();
        
        writeIndent();
        oss << "\"definitionId\": \"" << escapeJson(node.definitionId) << "\",";
        writeNewline();
        
        writeIndent();
        oss << "\"name\": \"" << escapeJson(node.name) << "\",";
        writeNewline();
        
        // 引脚列表
        writeIndent();
        oss << "\"pins\": [";
        writeNewline();
        indentLevel++;
        
        for (size_t j = 0; j < node.pins.size(); ++j)
        {
            const auto& pin = node.pins[j];
            
            writeIndent();
            oss << "{";
            writeNewline();
            indentLevel++;
            
            // 必选字段：id, kind(int), dataType(int)
            writeIndent(); oss << "\"id\": " << pin.id << ","; writeNewline();
            writeIndent(); oss << "\"kind\": " << static_cast<int>(pin.kind) << ","; writeNewline();
            writeIndent(); oss << "\"dataType\": " << static_cast<int>(pin.dataType);
            // 可选字段：仅在非默认值时写入（减少 JSON 体积）
            if (!pin.name.empty())
            {
                oss << ","; writeNewline();
                writeIndent(); oss << "\"name\": \"" << escapeJson(pin.name) << "\"";
            }
            if (pin.isExec)
            {
                oss << ","; writeNewline();
                writeIndent(); oss << "\"isExec\": true";
            }
            if (pin.allowMultiple)
            {
                oss << ","; writeNewline();
                writeIndent(); oss << "\"allowMultiple\": true";
            }
            if (pin.defaultValue.type != PinDataType::Unknown)
            {
                oss << ","; writeNewline();
                writeIndent(); oss << "\"defaultValue\": " << variantToJson(pin.defaultValue);
            }
            writeNewline();
            indentLevel--;
            writeIndent();
            oss << "}";
            if (j < node.pins.size() - 1) oss << ",";
            writeNewline();
        }
        
        indentLevel--;
        writeIndent();
        oss << "]";
        
        // 节点启用状态（仅在 false 时写入，默认 true）
        if (!node.isEnabled)
        {
            oss << ","; writeNewline();
            writeIndent();
            oss << "\"isEnabled\": false";
        }
        
        // 节点数据（配置参数，如常量值）
        if (!node.nodeData.empty())
        {
            oss << ","; writeNewline();
            writeIndent();
            oss << "\"nodeData\": {";
            writeNewline();
            indentLevel++;
            
            size_t idx = 0;
            for (const auto& kv : node.nodeData)
            {
                writeIndent();
                oss << "\"" << escapeJson(kv.first) << "\": " << variantToJson(kv.second);
                if (idx < node.nodeData.size() - 1) oss << ",";
                writeNewline();
                idx++;
            }
            
            indentLevel--;
            writeIndent();
            oss << "}";
        }
        
        // 自定义属性
        if (!node.customProperties.empty())
        {
            oss << ","; writeNewline();
            writeIndent();
            oss << "\"customProperties\": {";
            writeNewline();
            indentLevel++;
            
            size_t idx = 0;
            for (const auto& kv : node.customProperties)
            {
                if (options.propertyFilter && !options.propertyFilter(kv.first))
                {
                    idx++;
                    continue;
                }
                writeIndent();
                oss << "\"" << escapeJson(kv.first) << "\": \"" << escapeJson(kv.second) << "\"";
                if (idx < node.customProperties.size() - 1) oss << ",";
                writeNewline();
                idx++;
            }
            
            indentLevel--;
            writeIndent();
            oss << "}";
        }
        
        writeNewline();
        indentLevel--;
        writeIndent();
        oss << "}";
    }
    
    writeNewline();
    indentLevel--;
    writeIndent();
    oss << "],";
    writeNewline();
    
    // ================================================================
    // 链接列表
    // ================================================================
    writeIndent();
    oss << "\"links\": [";
    writeNewline();
    indentLevel++;
    
    for (size_t i = 0; i < data.links.size(); ++i)
    {
        const auto& link = data.links[i];
        
        writeIndent();
        oss << "{";
        writeNewline();
        indentLevel++;
        
        writeIndent(); oss << "\"id\": " << link.id << ","; writeNewline();
        writeIndent(); oss << "\"startPinId\": " << link.startPinId << ","; writeNewline();
        writeIndent(); oss << "\"endPinId\": " << link.endPinId << ","; writeNewline();
        writeIndent(); oss << "\"isEnabled\": " << (link.isEnabled ? "true" : "false"); writeNewline();
        
        indentLevel--;
        writeIndent();
        oss << "}";
        if (i < data.links.size() - 1) oss << ",";
        writeNewline();
    }
    
    indentLevel--;
    writeIndent();
    oss << "]";
    
    // ================================================================
    // 变量（运行时核心：name/dataType/isExposed/defaultValue）
    // ================================================================
    if (!data.variables.empty())
    {
        oss << ","; writeNewline();
        
        writeIndent();
        oss << "\"variables\": [";
        writeNewline();
        indentLevel++;
        
        for (size_t i = 0; i < data.variables.size(); ++i)
        {
            const auto& var = data.variables[i];
            
            writeIndent();
            oss << "{";
            writeNewline();
            indentLevel++;
            
            writeIndent(); oss << "\"name\": \"" << escapeJson(var.name) << "\","; writeNewline();
            writeIndent(); oss << "\"dataType\": " << static_cast<int>(var.dataType) << ","; writeNewline();
            writeIndent(); oss << "\"isExposed\": " << (var.isExposed ? "true" : "false");
            
            if (var.defaultValue.type != PinDataType::Unknown)
            {
                oss << ","; writeNewline();
                writeIndent(); oss << "\"defaultValue\": " << variantToJson(var.defaultValue);
            }
            
            writeNewline();
            indentLevel--;
            writeIndent();
            oss << "}";
            if (i < data.variables.size() - 1) oss << ",";
            writeNewline();
        }
        
        indentLevel--;
        writeIndent();
        oss << "]";
    }
    
    // 结束根对象
    writeNewline();
    indentLevel--;
    writeIndent();
    oss << "}";
    
    return oss.str();
}

ExportResult JsonBlueprintExporter::exportRuntimeToFile(const BlueprintData& data, const std::string& filePath, const ExportOptions& options) const
{
    ExportResult result;
    
    try
    {
        std::string jsonContent = exportRuntimeToString(data, options);
        
        std::string errorMsg;
        if (!m_fileSystem->WriteFile(filePath, jsonContent, errorMsg))
        {
            result.errorMessage = errorMsg;
            return result;
        }
        
        result.success = true;
        result.outputPath = filePath;
        result.bytesWritten = jsonContent.size();
    }
    catch (const std::exception& e)
    {
        result.errorMessage = std::string("Exception: ") + e.what();
    }
    
    return result;
}

// ============================================================================
// JsonBlueprintExporter — Editor 附加文件导出
// ============================================================================

std::string JsonBlueprintExporter::exportEditorToString(const BlueprintData& data, const ExportOptions& options) const
{
    std::ostringstream oss;
    int indentLevel = 0;
    bool pretty = options.prettyPrint;
    
    auto writeIndent = [&]() {
        if (pretty) oss << indentJson(indentLevel, options.indent);
    };
    
    auto writeNewline = [&]() {
        if (pretty) oss << "\n";
    };
    
    // 开始根对象
    oss << "{";
    writeNewline();
    indentLevel++;
    
    // 文件类型标识
    writeIndent();
    oss << "\"fileType\": \"editor\",";
    writeNewline();
    
    // ================================================================
    // 元数据附加（author、时间戳）
    // ================================================================
    writeIndent();
    oss << "\"metadata\": {";
    writeNewline();
    indentLevel++;
    
    writeIndent();
    oss << "\"author\": \"" << escapeJson(data.metadata.author) << "\",";
    writeNewline();
    
    writeIndent();
    oss << "\"createdAt\": \"" << escapeJson(data.metadata.createdAt) << "\",";
    writeNewline();
    
    writeIndent();
    oss << "\"updatedAt\": \"" << escapeJson(data.metadata.updatedAt) << "\"";
    
    writeNewline();
    indentLevel--;
    writeIndent();
    oss << "},";
    writeNewline();
    
    // ================================================================
    // 节点编辑器数据（position/size/isCollapsed），通过 nodeId 关联
    // ================================================================
    writeIndent();
    oss << "\"nodes\": [";
    writeNewline();
    indentLevel++;
    
    bool firstNode = true;
    for (size_t i = 0; i < data.nodes.size(); ++i)
    {
        const auto& node = data.nodes[i];
        
        if (options.nodeFilter && !options.nodeFilter(node))
            continue;
        
        if (!firstNode) { oss << ","; writeNewline(); }
        firstNode = false;
        
        writeIndent();
        oss << "{";
        writeNewline();
        indentLevel++;
        
        // 节点 ID（用于与 Runtime 文件中的节点关联）
        writeIndent();
        oss << "\"id\": " << node.id << ",";
        writeNewline();
        
        // 位置
        writeIndent();
        oss << "\"position\": {";
        writeNewline();
        indentLevel++;
        writeIndent(); oss << "\"x\": " << node.position.x << ","; writeNewline();
        writeIndent(); oss << "\"y\": " << node.position.y; writeNewline();
        indentLevel--;
        writeIndent();
        oss << "},";
        writeNewline();
        
        // 尺寸
        writeIndent();
        oss << "\"size\": {";
        writeNewline();
        indentLevel++;
        writeIndent(); oss << "\"width\": " << node.size.width << ","; writeNewline();
        writeIndent(); oss << "\"height\": " << node.size.height; writeNewline();
        indentLevel--;
        writeIndent();
        oss << "},";
        writeNewline();
        
        // 折叠状态
        writeIndent();
        oss << "\"isCollapsed\": " << (node.isCollapsed ? "true" : "false");
        
        writeNewline();
        indentLevel--;
        writeIndent();
        oss << "}";
    }
    
    writeNewline();
    indentLevel--;
    writeIndent();
    oss << "]";
    
    // ================================================================
    // 变量附加信息（category/tooltip），通过 name 关联
    // ================================================================
    if (!data.variables.empty())
    {
        // 检查是否有非空的 category 或 tooltip
        bool hasEditorVarData = false;
        for (const auto& var : data.variables)
        {
            if (!var.category.empty() || !var.tooltip.empty())
            {
                hasEditorVarData = true;
                break;
            }
        }
        
        if (hasEditorVarData)
        {
            oss << ","; writeNewline();
            
            writeIndent();
            oss << "\"variables\": [";
            writeNewline();
            indentLevel++;
            
            bool firstVar = true;
            for (size_t i = 0; i < data.variables.size(); ++i)
            {
                const auto& var = data.variables[i];
                if (var.category.empty() && var.tooltip.empty())
                    continue;
                
                if (!firstVar) { oss << ","; writeNewline(); }
                firstVar = false;
                
                writeIndent();
                oss << "{";
                writeNewline();
                indentLevel++;
                
                writeIndent(); oss << "\"name\": \"" << escapeJson(var.name) << "\","; writeNewline();
                writeIndent(); oss << "\"category\": \"" << escapeJson(var.category) << "\","; writeNewline();
                writeIndent(); oss << "\"tooltip\": \"" << escapeJson(var.tooltip) << "\""; writeNewline();
                
                indentLevel--;
                writeIndent();
                oss << "}";
            }
            
            writeNewline();
            indentLevel--;
            writeIndent();
            oss << "]";
        }
    }
    
    // ================================================================
    // 注释
    // ================================================================
    if (!data.comments.empty())
    {
        oss << ","; writeNewline();
        
        writeIndent();
        oss << "\"comments\": [";
        writeNewline();
        indentLevel++;
        
        for (size_t i = 0; i < data.comments.size(); ++i)
        {
            const auto& comment = data.comments[i];
            
            writeIndent();
            oss << "{";
            writeNewline();
            indentLevel++;
            
            writeIndent(); oss << "\"id\": \"" << escapeJson(comment.id) << "\","; writeNewline();
            writeIndent(); oss << "\"text\": \"" << escapeJson(comment.text) << "\","; writeNewline();
            writeIndent(); oss << "\"position\": { \"x\": " << comment.position.x << ", \"y\": " << comment.position.y << " },"; writeNewline();
            writeIndent(); oss << "\"size\": { \"width\": " << comment.size.width << ", \"height\": " << comment.size.height << " },"; writeNewline();
            writeIndent(); oss << "\"color\": \"" << escapeJson(comment.color) << "\","; writeNewline();
            writeIndent(); oss << "\"alpha\": " << comment.alpha; writeNewline();
            
            indentLevel--;
            writeIndent();
            oss << "}";
            if (i < data.comments.size() - 1) oss << ",";
            writeNewline();
        }
        
        indentLevel--;
        writeIndent();
        oss << "]";
    }
    
    // ================================================================
    // 视图信息
    // ================================================================
    {
        oss << ","; writeNewline();
        
        writeIndent();
        oss << "\"viewInfo\": {";
        writeNewline();
        indentLevel++;
        
        writeIndent(); oss << "\"viewPosition\": { \"x\": " << data.viewInfo.viewPosition.x << ", \"y\": " << data.viewInfo.viewPosition.y << " },"; writeNewline();
        writeIndent(); oss << "\"viewScale\": " << data.viewInfo.viewScale; writeNewline();
        
        indentLevel--;
        writeIndent();
        oss << "}";
    }
    
    // ================================================================
    // 嵌入完整 Runtime 数据（使 .editor.json 成为 .json 的超集）
    // ================================================================
    {
        oss << ","; writeNewline();
        writeIndent();
        // 将完整 runtime JSON 嵌入为 "runtime" 字段的字符串值
        // 这里直接内联 runtime 结构，而非嵌套字符串，以便导入时直接解析
        std::string runtimeJson = exportRuntimeToString(data, options);
        // 解析 runtime JSON 为对象，嵌入为 "runtime" 字段
        oss << "\"runtime\": " << runtimeJson;
    }
    
    // 结束根对象
    writeNewline();
    indentLevel--;
    writeIndent();
    oss << "}";
    
    return oss.str();
}

ExportResult JsonBlueprintExporter::exportEditorToFile(const BlueprintData& data, const std::string& filePath, const ExportOptions& options) const
{
    ExportResult result;
    
    try
    {
        std::string jsonContent = exportEditorToString(data, options);
        
        std::string errorMsg;
        if (!m_fileSystem->WriteFile(filePath, jsonContent, errorMsg))
        {
            result.errorMessage = errorMsg;
            return result;
        }
        
        result.success = true;
        result.outputPath = filePath;
        result.bytesWritten = jsonContent.size();
    }
    catch (const std::exception& e)
    {
        result.errorMessage = std::string("Exception: ") + e.what();
    }
    
    return result;
}

// ============================================================================
// JsonBlueprintExporter — 编辑器双文件导出
// ============================================================================

EditorExportResult JsonBlueprintExporter::exportEditorFiles(const BlueprintData& data, const std::string& runtimeFilePath, const std::string& editorFilePath, const ExportOptions& options) const
{
    EditorExportResult result;
    
    try
    {
        // 导出 Runtime 文件
        auto runtimeResult = exportRuntimeToFile(data, runtimeFilePath, options);
        if (!runtimeResult.success)
        {
            result.errorMessage = "Runtime export failed: " + runtimeResult.errorMessage;
            return result;
        }
        
        // 导出 Editor 附加文件
        auto editorResult = exportEditorToFile(data, editorFilePath, options);
        if (!editorResult.success)
        {
            result.errorMessage = "Editor export failed: " + editorResult.errorMessage;
            return result;
        }
        
        result.success = true;
        result.runtimePath = runtimeFilePath;
        result.editorPath = editorFilePath;
        result.runtimeBytes = runtimeResult.bytesWritten;
        result.editorBytes = editorResult.bytesWritten;
    }
    catch (const std::exception& e)
    {
        result.errorMessage = std::string("Exception: ") + e.what();
    }
    
    return result;
}

// ============================================================================
// JsonBlueprintExporter — Runtime 导入
// ============================================================================

ImportResult JsonBlueprintExporter::importRuntimeFromString(const std::string& content, const ImportOptions& options) const
{
    ImportResult result;
    result.bytesRead = content.size();

    // 使用 crude_json 解析
    crude_json::value root = crude_json::value::parse(content);
    if (root.is_discarded())
    {
        result.errorMessage = "JSON parse error";
        return result;
    }

    if (root.type() != crude_json::type_t::object)
    {
        result.errorMessage = "Root JSON element must be an object";
        return result;
    }

    auto& rootObj = root;

    // 自动检测 .editor.json 格式：如果包含 "runtime" 子对象，
    // 说明这是 editor 文件，真正的 runtime 数据嵌套在 "runtime" 字段中。
    // 递归调用自身来解析内嵌的 runtime 数据。
    if (rootObj.contains("runtime") && rootObj["runtime"].type() == crude_json::type_t::object)
    {
        std::string runtimeJson = rootObj["runtime"].dump();
        result = importRuntimeFromString(runtimeJson, options);
        result.bytesRead = content.size();
        return result;
    }

    // ---- 辅助别名（使用文件级辅助函数，通过 lambda 保留默认参数）----
    auto getNumber = [](const crude_json::value& obj, const char* key, double defaultVal = 0.0) { return getJsonNumber(obj, key, defaultVal); };
    auto getString = [](const crude_json::value& obj, const char* key) { return getJsonString(obj, key); };
    auto getBool   = [](const crude_json::value& obj, const char* key, bool defaultVal = false) { return getJsonBool(obj, key, defaultVal); };

    // ---- 解析元数据 ----
    if (rootObj.contains("metadata") && rootObj["metadata"].type() == crude_json::type_t::object)
    {
        auto& meta = rootObj["metadata"];
        result.data.metadata.name        = getString(meta, "name");
        result.data.metadata.description  = getString(meta, "description");
        result.data.metadata.version      = getString(meta, "version");
        // Runtime 文件中可能不包含 author/timestamps，但仍兼容读取
        result.data.metadata.author       = getString(meta, "author");
        result.data.metadata.createdAt    = getString(meta, "createdAt");
        result.data.metadata.updatedAt    = getString(meta, "updatedAt");

        if (meta.contains("tags") && meta["tags"].type() == crude_json::type_t::array)
        {
            for (auto& tag : meta["tags"].get<crude_json::array>())
            {
                if (tag.type() == crude_json::type_t::string)
                    result.data.metadata.tags.push_back(tag.get<std::string>());
            }
        }
    }

    // ---- 解析节点 ----
    if (rootObj.contains("nodes") && rootObj["nodes"].type() == crude_json::type_t::array)
    {
        for (auto& nodeJson : rootObj["nodes"].get<crude_json::array>())
        {
            if (nodeJson.type() != crude_json::type_t::object) continue;

            NodeInstance node;
            node.id           = static_cast<NodeId>(getNumber(nodeJson, "id"));
            node.definitionId = getString(nodeJson, "definitionId");
            node.name         = getString(nodeJson, "name");
            node.isEnabled    = getBool(nodeJson, "isEnabled", true);
            node.isCollapsed  = getBool(nodeJson, "isCollapsed", false);

            // 位置（Runtime 文件不含，但兼容老格式）
            if (nodeJson.contains("position") && nodeJson["position"].type() == crude_json::type_t::object)
            {
                auto& pos = nodeJson["position"];
                node.position.x = static_cast<float>(getNumber(pos, "x"));
                node.position.y = static_cast<float>(getNumber(pos, "y"));
            }

            // 尺寸（同上）
            if (nodeJson.contains("size") && nodeJson["size"].type() == crude_json::type_t::object)
            {
                auto& sz = nodeJson["size"];
                node.size.width  = static_cast<float>(getNumber(sz, "width"));
                node.size.height = static_cast<float>(getNumber(sz, "height"));
            }

            // 引脚
            if (nodeJson.contains("pins") && nodeJson["pins"].type() == crude_json::type_t::array)
            {
                for (auto& pinJson : nodeJson["pins"].get<crude_json::array>())
                {
                    if (pinJson.type() != crude_json::type_t::object) continue;

                    PinInfo pin;
                    pin.id       = static_cast<PinId>(getNumber(pinJson, "id"));
                    pin.name     = getString(pinJson, "name");  // 缺失时返回空字符串（默认值）
                    pin.dataType = static_cast<PinDataType>(static_cast<int>(getNumber(pinJson, "dataType")));

                    // kind: 兼容新格式(int: 0=Input, 1=Output) 和旧格式(string: "input"/"output")
                    if (pinJson.contains("kind"))
                    {
                        if (pinJson["kind"].type() == crude_json::type_t::number)
                            pin.kind = (static_cast<int>(pinJson["kind"].get<double>()) == 1) ? PinKind::Output : PinKind::Input;
                        else
                        {
                            std::string kindStr = getString(pinJson, "kind");
                            pin.kind = (kindStr == "output") ? PinKind::Output : PinKind::Input;
                        }
                    }

                    pin.allowMultiple = getBool(pinJson, "allowMultiple", false);
                    pin.isExec        = getBool(pinJson, "isExec", false);

                    if (pinJson.contains("defaultValue"))
                    {
                        pin.defaultValue = jsonToVariant(pinJson["defaultValue"].dump(), pin.dataType);
                    }

                    node.pins.push_back(std::move(pin));
                }
            }

            // 自定义属性
            if (nodeJson.contains("customProperties") && nodeJson["customProperties"].type() == crude_json::type_t::object)
            {
                for (auto& kv : nodeJson["customProperties"].get<crude_json::object>())
                {
                    if (kv.second.type() == crude_json::type_t::string)
                        node.customProperties[kv.first] = kv.second.get<std::string>();
                }
            }

            // 节点数据
            if (nodeJson.contains("nodeData") && nodeJson["nodeData"].type() == crude_json::type_t::object)
            {
                for (auto& kv : nodeJson["nodeData"].get<crude_json::object>())
                {
                    auto& val = kv.second;
                    Variant v;
                    if (val.type() == crude_json::type_t::boolean)
                    {
                        v = Variant(val.get<bool>());
                    }
                    else if (val.type() == crude_json::type_t::number)
                    {
                        v = Variant(val.get<double>());
                    }
                    else if (val.type() == crude_json::type_t::string)
                    {
                        v = Variant(val.get<std::string>());
                    }
                    node.nodeData[kv.first] = v;
                }
            }

            result.data.nodes.push_back(std::move(node));
        }
    }

    // ---- 解析链接 ----
    if (rootObj.contains("links") && rootObj["links"].type() == crude_json::type_t::array)
    {
        for (auto& linkJson : rootObj["links"].get<crude_json::array>())
        {
            if (linkJson.type() != crude_json::type_t::object) continue;

            LinkInstance link;
            link.id         = static_cast<LinkId>(getNumber(linkJson, "id"));
            link.startPinId = static_cast<PinId>(getNumber(linkJson, "startPinId"));
            link.endPinId   = static_cast<PinId>(getNumber(linkJson, "endPinId"));
            link.isEnabled  = getBool(linkJson, "isEnabled", true);

            result.data.links.push_back(std::move(link));
        }
    }

    // ---- 解析变量 ----
    if (rootObj.contains("variables") && rootObj["variables"].type() == crude_json::type_t::array)
    {
        for (auto& varJson : rootObj["variables"].get<crude_json::array>())
        {
            if (varJson.type() != crude_json::type_t::object) continue;

            VariableDefinition var;
            var.name      = getString(varJson, "name");
            var.dataType  = static_cast<PinDataType>(static_cast<int>(getNumber(varJson, "dataType")));
            var.isExposed = getBool(varJson, "isExposed", true);
            var.category  = getString(varJson, "category");
            var.tooltip   = getString(varJson, "tooltip");

            if (varJson.contains("defaultValue"))
            {
                var.defaultValue = jsonToVariant(varJson["defaultValue"].dump(), var.dataType);
            }

            result.data.variables.push_back(std::move(var));
        }
    }

    // ---- 解析注释（兼容老格式） ----
    if (rootObj.contains("comments") && rootObj["comments"].type() == crude_json::type_t::array)
    {
        for (auto& cmtJson : rootObj["comments"].get<crude_json::array>())
        {
            if (cmtJson.type() != crude_json::type_t::object) continue;

            CommentRegion cmt;
            cmt.id    = getString(cmtJson, "id");
            cmt.text  = getString(cmtJson, "text");
            cmt.color = getString(cmtJson, "color");
            cmt.alpha = static_cast<float>(getNumber(cmtJson, "alpha", 0.5));

            if (cmtJson.contains("position") && cmtJson["position"].type() == crude_json::type_t::object)
            {
                cmt.position.x = static_cast<float>(getNumber(cmtJson["position"], "x"));
                cmt.position.y = static_cast<float>(getNumber(cmtJson["position"], "y"));
            }
            if (cmtJson.contains("size") && cmtJson["size"].type() == crude_json::type_t::object)
            {
                cmt.size.width  = static_cast<float>(getNumber(cmtJson["size"], "width"));
                cmt.size.height = static_cast<float>(getNumber(cmtJson["size"], "height"));
            }

            result.data.comments.push_back(std::move(cmt));
        }
    }

    // ---- 解析视图信息（兼容老格式） ----
    if (rootObj.contains("viewInfo") && rootObj["viewInfo"].type() == crude_json::type_t::object)
    {
        auto& viewJson = rootObj["viewInfo"];
        if (viewJson.contains("viewPosition") && viewJson["viewPosition"].type() == crude_json::type_t::object)
        {
            result.data.viewInfo.viewPosition.x = static_cast<float>(getNumber(viewJson["viewPosition"], "x"));
            result.data.viewInfo.viewPosition.y = static_cast<float>(getNumber(viewJson["viewPosition"], "y"));
        }
        result.data.viewInfo.viewScale = static_cast<float>(getNumber(viewJson, "viewScale", 1.0));
    }

    // ---- 验证 ----
    if (options.validateOnLoad)
    {
        std::vector<std::string> errors;
        if (!validate(result.data, errors))
        {
            for (auto& e : errors)
                result.warnings.push_back(e);
        }
    }

    // ---- 修复断开的链接 ----
    if (options.repairBrokenLinks)
    {
        std::unordered_set<PinId> pinIds;
        for (const auto& node : result.data.nodes)
        {
            for (const auto& pin : node.pins)
            {
                pinIds.insert(pin.id);
            }
        }

        auto& links = result.data.links;
        links.erase(
            std::remove_if(links.begin(), links.end(), [&pinIds](const LinkInstance& link) {
                return pinIds.find(link.startPinId) == pinIds.end() ||
                       pinIds.find(link.endPinId) == pinIds.end();
            }),
            links.end()
        );
    }

    result.success = true;
    return result;
}

ImportResult JsonBlueprintExporter::importRuntimeFromFile(const std::string& filePath, const ImportOptions& options) const
{
    ImportResult result;
    
    try
    {
        std::string errorMsg;
        std::string content;
        if (!m_fileSystem->ReadFile(filePath, content, errorMsg))
        {
            result.errorMessage = errorMsg;
            return result;
        }
        
        result = importRuntimeFromString(content, options);
    }
    catch (const std::exception& e)
    {
        result.errorMessage = std::string("Exception: ") + e.what();
    }
    
    return result;
}

// ============================================================================
// JsonBlueprintExporter — 编辑器合并加载
// ============================================================================

ImportResult JsonBlueprintExporter::importEditorFromStrings(const std::string& runtimeContent, const std::string& editorContent, const ImportOptions& options) const
{
    // 先加载 Runtime 数据
    ImportResult result = importRuntimeFromString(runtimeContent, options);
    if (!result.success)
    {
        result.errorMessage = "Runtime import failed: " + result.errorMessage;
        return result;
    }
    
    // 解析 Editor 附加数据
    crude_json::value editorRoot = crude_json::value::parse(editorContent);
    if (editorRoot.is_discarded())
    {
        result.warnings.push_back("Editor JSON parse error, editor data ignored");
        return result;
    }
    
    if (editorRoot.type() != crude_json::type_t::object)
    {
        result.warnings.push_back("Editor JSON root must be an object, editor data ignored");
        return result;
    }
    
    // ---- 辅助别名（使用文件级辅助函数，通过 lambda 保留默认参数）----
    auto getNumber = [](const crude_json::value& obj, const char* key, double defaultVal = 0.0) { return getJsonNumber(obj, key, defaultVal); };
    auto getString = [](const crude_json::value& obj, const char* key) { return getJsonString(obj, key); };
    auto getBool   = [](const crude_json::value& obj, const char* key, bool defaultVal = false) { return getJsonBool(obj, key, defaultVal); };
    
    // ---- 合并元数据（author、时间戳）----
    if (editorRoot.contains("metadata") && editorRoot["metadata"].type() == crude_json::type_t::object)
    {
        auto& meta = editorRoot["metadata"];
        std::string author = getString(meta, "author");
        std::string createdAt = getString(meta, "createdAt");
        std::string updatedAt = getString(meta, "updatedAt");
        
        if (!author.empty())    result.data.metadata.author    = author;
        if (!createdAt.empty()) result.data.metadata.createdAt = createdAt;
        if (!updatedAt.empty()) result.data.metadata.updatedAt = updatedAt;
    }
    
    // ---- 合并节点编辑器数据（position/size/isCollapsed）----
    if (editorRoot.contains("nodes") && editorRoot["nodes"].type() == crude_json::type_t::array)
    {
        // 建立 nodeId -> node 指针映射
        std::unordered_map<NodeId, NodeInstance*> nodeMap;
        for (auto& node : result.data.nodes)
        {
            nodeMap[node.id] = &node;
        }
        
        for (auto& editorNode : editorRoot["nodes"].get<crude_json::array>())
        {
            if (editorNode.type() != crude_json::type_t::object) continue;
            
            NodeId nodeId = static_cast<NodeId>(getNumber(editorNode, "id"));
            auto it = nodeMap.find(nodeId);
            if (it == nodeMap.end()) continue;
            
            NodeInstance* node = it->second;
            
            // 位置
            if (editorNode.contains("position") && editorNode["position"].type() == crude_json::type_t::object)
            {
                auto& pos = editorNode["position"];
                node->position.x = static_cast<float>(getNumber(pos, "x"));
                node->position.y = static_cast<float>(getNumber(pos, "y"));
            }
            
            // 尺寸
            if (editorNode.contains("size") && editorNode["size"].type() == crude_json::type_t::object)
            {
                auto& sz = editorNode["size"];
                node->size.width  = static_cast<float>(getNumber(sz, "width"));
                node->size.height = static_cast<float>(getNumber(sz, "height"));
            }
            
            // 折叠状态
            if (editorNode.contains("isCollapsed"))
            {
                node->isCollapsed = getBool(editorNode, "isCollapsed", false);
            }
        }
    }
    
    // ---- 合并变量附加信息（category/tooltip）----
    if (editorRoot.contains("variables") && editorRoot["variables"].type() == crude_json::type_t::array)
    {
        // 建立 name -> variable 指针映射
        std::unordered_map<std::string, VariableDefinition*> varMap;
        for (auto& var : result.data.variables)
        {
            varMap[var.name] = &var;
        }
        
        for (auto& editorVar : editorRoot["variables"].get<crude_json::array>())
        {
            if (editorVar.type() != crude_json::type_t::object) continue;
            
            std::string name = getString(editorVar, "name");
            auto it = varMap.find(name);
            if (it == varMap.end()) continue;
            
            VariableDefinition* var = it->second;
            
            std::string category = getString(editorVar, "category");
            std::string tooltip  = getString(editorVar, "tooltip");
            
            if (!category.empty()) var->category = category;
            if (!tooltip.empty())  var->tooltip  = tooltip;
        }
    }
    
    // ---- 合并注释 ----
    if (editorRoot.contains("comments") && editorRoot["comments"].type() == crude_json::type_t::array)
    {
        for (auto& cmtJson : editorRoot["comments"].get<crude_json::array>())
        {
            if (cmtJson.type() != crude_json::type_t::object) continue;
            
            CommentRegion cmt;
            cmt.id    = getString(cmtJson, "id");
            cmt.text  = getString(cmtJson, "text");
            cmt.color = getString(cmtJson, "color");
            cmt.alpha = static_cast<float>(getNumber(cmtJson, "alpha", 0.5));
            
            if (cmtJson.contains("position") && cmtJson["position"].type() == crude_json::type_t::object)
            {
                cmt.position.x = static_cast<float>(getNumber(cmtJson["position"], "x"));
                cmt.position.y = static_cast<float>(getNumber(cmtJson["position"], "y"));
            }
            if (cmtJson.contains("size") && cmtJson["size"].type() == crude_json::type_t::object)
            {
                cmt.size.width  = static_cast<float>(getNumber(cmtJson["size"], "width"));
                cmt.size.height = static_cast<float>(getNumber(cmtJson["size"], "height"));
            }
            
            result.data.comments.push_back(std::move(cmt));
        }
    }
    
    // ---- 合并视图信息 ----
    if (editorRoot.contains("viewInfo") && editorRoot["viewInfo"].type() == crude_json::type_t::object)
    {
        auto& viewJson = editorRoot["viewInfo"];
        if (viewJson.contains("viewPosition") && viewJson["viewPosition"].type() == crude_json::type_t::object)
        {
            result.data.viewInfo.viewPosition.x = static_cast<float>(getNumber(viewJson["viewPosition"], "x"));
            result.data.viewInfo.viewPosition.y = static_cast<float>(getNumber(viewJson["viewPosition"], "y"));
        }
        result.data.viewInfo.viewScale = static_cast<float>(getNumber(viewJson, "viewScale", 1.0));
    }
    
    result.bytesRead += editorContent.size();
    return result;
}

ImportResult JsonBlueprintExporter::importEditorFromFiles(const std::string& runtimeFilePath, const std::string& editorFilePath, const ImportOptions& options) const
{
    ImportResult result;
    
    try
    {
        std::string errorMsg;
        
        std::string runtimeContent;
        if (!m_fileSystem->ReadFile(runtimeFilePath, runtimeContent, errorMsg))
        {
            result.errorMessage = errorMsg;
            return result;
        }
        
        std::string editorContent;
        if (!m_fileSystem->ReadFile(editorFilePath, editorContent, errorMsg))
        {
            result.errorMessage = errorMsg;
            return result;
        }
        
        result = importEditorFromStrings(runtimeContent, editorContent, options);
    }
    catch (const std::exception& e)
    {
        result.errorMessage = std::string("Exception: ") + e.what();
    }
    
    return result;
}

// ============================================================================
// JsonBlueprintExporter — 从单个 .editor.json 加载完整数据
// ============================================================================

ImportResult JsonBlueprintExporter::importFromEditorFile(const std::string& editorFilePath, const ImportOptions& options) const
{
    ImportResult result;
    
    try
    {
        std::string errorMsg;
        std::string editorContent;
        if (!m_fileSystem->ReadFile(editorFilePath, editorContent, errorMsg))
        {
            result.errorMessage = errorMsg;
            return result;
        }
        
        result.bytesRead = editorContent.size();
        
        // 解析 editor JSON
        crude_json::value editorRoot = crude_json::value::parse(editorContent);
        if (editorRoot.is_discarded() || editorRoot.type() != crude_json::type_t::object)
        {
            result.errorMessage = "Editor JSON parse error";
            return result;
        }
        
        // 检查是否包含嵌入的 runtime 数据
        if (!editorRoot.contains("runtime") || editorRoot["runtime"].type() != crude_json::type_t::object)
        {
            result.errorMessage = "Editor file does not contain embedded runtime data. Please open the .json file instead.";
            return result;
        }
        
        // 从嵌入的 runtime 对象中提取 runtime JSON 字符串
        // 为了复用 importRuntimeFromString，需要将 runtime 对象序列化回字符串
        std::string runtimeJson = editorRoot["runtime"].dump();
        
        // 先导入 runtime 数据
        result = importRuntimeFromString(runtimeJson, options);
        if (!result.success)
        {
            result.errorMessage = "Embedded runtime import failed: " + result.errorMessage;
            return result;
        }
        
        // 然后合并 editor 数据（复用 importEditorFromStrings 的逻辑）
        // 但因为 editor 数据就在同一个文件中，直接用 editorContent 作为 editor 部分
        // importEditorFromStrings 会忽略它不认识的字段（如 "runtime"），所以可以直接用
        ImportResult mergedResult = importEditorFromStrings(runtimeJson, editorContent, options);
        if (mergedResult.success)
        {
            result = mergedResult;
        }
        // 即使合并失败，runtime 数据仍然可用
    }
    catch (const std::exception& e)
    {
        result.errorMessage = std::string("Exception: ") + e.what();
    }
    
    return result;
}

// ============================================================================
// JsonBlueprintExporter — 验证和通用方法
// ============================================================================

bool JsonBlueprintExporter::validate(const BlueprintData& data, std::vector<std::string>& errors) const
{
    errors.clear();
    
    // 验证节点 ID 唯一性 & 一次性收集所有 pinId（用于后续链接验证）
    std::unordered_map<NodeId, int> nodeIds;
    std::unordered_set<PinId> allPinIds;
    for (const auto& node : data.nodes)
    {
        if (node.id == InvalidNodeId)
        {
            errors.push_back("Node has invalid ID");
        }
        
        if (nodeIds.find(node.id) != nodeIds.end())
        {
            errors.push_back("Duplicate node ID: " + std::to_string(node.id));
        }
        nodeIds[node.id]++;
        
        for (const auto& pin : node.pins)
            allPinIds.insert(pin.id);
    }
    
    // 验证链接 — O(L) 而非 O(L*N*P)
    for (const auto& link : data.links)
    {
        if (link.id == InvalidLinkId)
        {
            errors.push_back("Link has invalid ID");
        }
        
        if (link.startPinId == InvalidPinId || link.endPinId == InvalidPinId)
        {
            errors.push_back("Link has invalid pin IDs");
        }
        
        if (allPinIds.find(link.startPinId) == allPinIds.end())
        {
            errors.push_back("Link references non-existent start pin: " + std::to_string(link.startPinId));
        }
        
        if (allPinIds.find(link.endPinId) == allPinIds.end())
        {
            errors.push_back("Link references non-existent end pin: " + std::to_string(link.endPinId));
        }
    }
    
    return errors.empty();
}

std::vector<ExportFormat> JsonBlueprintExporter::getSupportedFormats() const
{
    return { ExportFormat::JSON };
}

std::string JsonBlueprintExporter::variantToJson(const Variant& value) const
{
    switch (value.type)
    {
    case PinDataType::Boolean:
        return value.boolValue ? "true" : "false";
    case PinDataType::Integer:
        return std::to_string(value.intValue);
    case PinDataType::Float:
        return std::to_string(value.floatValue);
    case PinDataType::String:
        return "\"" + escapeJson(value.stringValue) + "\"";
    case PinDataType::Object:
        return "\"" + escapeJson(value.stringValue) + "\"";
    case PinDataType::Array:
    {
        std::string s = "[";
        for (size_t i = 0; i < value.arrayValue.size(); ++i)
        {
            if (i > 0) s += ",";
            s += variantToJson(value.arrayValue[i]);
        }
        s += "]";
        return s;
    }
    case PinDataType::Map:
    {
        std::string s = "{";
        bool first = true;
        for (const auto& kv : value.mapValue)
        {
            if (!first) s += ",";
            first = false;
            s += "\"" + escapeJson(kv.first) + "\":" + variantToJson(kv.second);
        }
        s += "}";
        return s;
    }
    default:
        return "null";
    }
}

Variant JsonBlueprintExporter::jsonToVariant(const std::string& json, PinDataType type) const
{
    Variant result;
    result.type = type;

    crude_json::value val = crude_json::value::parse(json);
    if (val.is_discarded())
        return result;

    switch (type)
    {
    case PinDataType::Boolean:
        if (val.type() == crude_json::type_t::boolean)
            result.boolValue = val.get<bool>();
        break;
    case PinDataType::Integer:
        if (val.type() == crude_json::type_t::number)
            result.intValue = static_cast<int64_t>(val.get<double>());
        break;
    case PinDataType::Float:
        if (val.type() == crude_json::type_t::number)
            result.floatValue = val.get<double>();
        break;
    case PinDataType::String:
        if (val.type() == crude_json::type_t::string)
            result.stringValue = val.get<std::string>();
        break;
    case PinDataType::Object:
        if (val.type() == crude_json::type_t::string)
            result.stringValue = val.get<std::string>();
        break;
    case PinDataType::Array:
        if (val.type() == crude_json::type_t::array)
        {
            for (auto& elem : val.get<crude_json::array>())
            {
                // 数组元素反序列化为 Any（通过 JSON 类型推断）
                Variant elemVar;
                if (elem.type() == crude_json::type_t::boolean)
                    elemVar = Variant(elem.get<bool>());
                else if (elem.type() == crude_json::type_t::number)
                    elemVar = Variant(elem.get<double>());
                else if (elem.type() == crude_json::type_t::string)
                    elemVar = Variant(elem.get<std::string>());
                result.arrayValue.push_back(std::move(elemVar));
            }
        }
        break;
    case PinDataType::Map:
        if (val.type() == crude_json::type_t::object)
        {
            for (auto& kv : val.get<crude_json::object>())
            {
                Variant elemVar;
                if (kv.second.type() == crude_json::type_t::boolean)
                    elemVar = Variant(kv.second.get<bool>());
                else if (kv.second.type() == crude_json::type_t::number)
                    elemVar = Variant(kv.second.get<double>());
                else if (kv.second.type() == crude_json::type_t::string)
                    elemVar = Variant(kv.second.get<std::string>());
                result.mapValue[kv.first] = std::move(elemVar);
            }
        }
        break;
    default:
        break;
    }

    return result;
}

// ============================================================================
// BinaryBlueprintExporter 实现（占位）
// ============================================================================

std::string BinaryBlueprintExporter::exportRuntimeToString(const BlueprintData& data, const ExportOptions& options) const
{
    return "";
}

ExportResult BinaryBlueprintExporter::exportRuntimeToFile(const BlueprintData& data, const std::string& filePath, const ExportOptions& options) const
{
    ExportResult result;
    result.errorMessage = "Binary export not yet implemented";
    return result;
}

ImportResult BinaryBlueprintExporter::importRuntimeFromString(const std::string& content, const ImportOptions& options) const
{
    ImportResult result;
    result.errorMessage = "Binary import not yet implemented";
    return result;
}

ImportResult BinaryBlueprintExporter::importRuntimeFromFile(const std::string& filePath, const ImportOptions& options) const
{
    ImportResult result;
    result.errorMessage = "Binary import not yet implemented";
    return result;
}

std::string BinaryBlueprintExporter::exportEditorToString(const BlueprintData& data, const ExportOptions& options) const
{
    return "";
}

ExportResult BinaryBlueprintExporter::exportEditorToFile(const BlueprintData& data, const std::string& filePath, const ExportOptions& options) const
{
    ExportResult result;
    result.errorMessage = "Binary editor export not yet implemented";
    return result;
}

EditorExportResult BinaryBlueprintExporter::exportEditorFiles(const BlueprintData& data, const std::string& runtimeFilePath, const std::string& editorFilePath, const ExportOptions& options) const
{
    EditorExportResult result;
    result.errorMessage = "Binary editor export not yet implemented";
    return result;
}

ImportResult BinaryBlueprintExporter::importEditorFromStrings(const std::string& runtimeContent, const std::string& editorContent, const ImportOptions& options) const
{
    ImportResult result;
    result.errorMessage = "Binary editor import not yet implemented";
    return result;
}

ImportResult BinaryBlueprintExporter::importEditorFromFiles(const std::string& runtimeFilePath, const std::string& editorFilePath, const ImportOptions& options) const
{
    ImportResult result;
    result.errorMessage = "Binary editor import not yet implemented";
    return result;
}

bool BinaryBlueprintExporter::validate(const BlueprintData& data, std::vector<std::string>& errors) const
{
    JsonBlueprintExporter jsonExporter;
    return jsonExporter.validate(data, errors);
}

std::vector<ExportFormat> BinaryBlueprintExporter::getSupportedFormats() const
{
    return { ExportFormat::Binary };
}

} // namespace Runtime
} // namespace NodeEditor
