// Runtime/BlueprintExporter.cpp - 蓝图数据导出器实现
//
// 导出策略:
//   Runtime 文件:  元数据(schemaVersion/name/desc/version)、节点(id/definitionId/definitionVersion/name/pins/isEnabled/nodeData/customProperties)、链接、变量(核心)
//   Editor 文件:   节点(position/size/isCollapsed) 通过 nodeId 关联、注释、视图信息、元数据(author/timestamps)、变量(category/tooltip)

#include "BlueprintExporter.h"
#include "../Utils/Json/crude_json.h"
#include <sstream>
#include <iomanip>
#include <ctime>
#include <cstdio>   // std::snprintf / ::remove
#include <iostream>
#include <algorithm>
#include <unordered_map>
#include <unordered_set>
#ifdef _WIN32
#  define WIN32_LEAN_AND_MEAN
#  ifndef NOMINMAX
#    define NOMINMAX
#  endif
#  include <windows.h>   // DeleteFileA
#endif
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
            if (static_cast<unsigned char>(c) <= 0x1f)
            {
                oss << "\\u" << std::hex << std::setw(4) << std::setfill('0') << static_cast<int>(static_cast<unsigned char>(c));
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
// Schema 迁移（预留框架，当前无需迁移）
// ============================================================================

static void ApplySchemaMigrations(BlueprintData& /*data*/, int /*fromVersion*/)
{
    // 当前 schema 版本为 BLUEPRINT_CURRENT_SCHEMA_VERSION，无历史格式需要迁移
}

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
    // 元数据（运行时只保留 name/description/version/schemaVersion）
    // ================================================================
    if (options.includeMetadata)
    {
        writeIndent();
        oss << "\"metadata\": {";
        writeNewline();
        indentLevel++;
        
        writeIndent();
        oss << "\"schemaVersion\": " << data.metadata.schemaVersion << ",";
        writeNewline();

        writeIndent();
        // blueprintClass 以整数存储（0=Actor, 1=FunctionLibrary），节省空间，枚举扩展向后兼容
        oss << "\"blueprintClass\": " << static_cast<int>(data.metadata.blueprintClass) << ",";
        writeNewline();
        
        writeIndent();
        oss << "\"name\": \"" << escapeJson(data.metadata.name) << "\",";
        writeNewline();
        
        writeIndent();
        oss << "\"description\": \"" << escapeJson(data.metadata.description) << "\",";
        writeNewline();
        
        writeIndent();
        oss << "\"version\": \"" << escapeJson(data.metadata.version) << "\"";

        // dependencies（可选，非空时写入）
        if (!data.metadata.dependencies.empty())
        {
            oss << ",";
            writeNewline();
            writeIndent();
            oss << "\"dependencies\": [";
            for (size_t di = 0; di < data.metadata.dependencies.size(); ++di)
            {
                oss << "\"" << escapeJson(data.metadata.dependencies[di]) << "\"";
                if (di + 1 < data.metadata.dependencies.size()) oss << ", ";
            }
            oss << "]";
        }
        
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
        oss << "\"definitionVersion\": " << node.definitionVersion << ",";
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

        // 折叠状态（v2+，仅在 true 时写入，默认 false）
        if (node.isCollapsed)
        {
            oss << ","; writeNewline();
            writeIndent();
            oss << "\"isCollapsed\": true";
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
    
    // ── functions 数组 ───────────────────────────────────────────────────────
    if (!data.functions.empty())
    {
        oss << ","; writeNewline();
        writeIndent();
        oss << "\"functions\": [";
        writeNewline();
        indentLevel++;

        for (size_t fi = 0; fi < data.functions.size(); ++fi)
        {
            const auto& func = data.functions[fi];
            writeIndent(); oss << "{"; writeNewline();
            indentLevel++;

            writeIndent(); oss << "\"id\": \""          << escapeJson(func.id)          << "\","; writeNewline();
            writeIndent(); oss << "\"name\": \""         << escapeJson(func.name)         << "\","; writeNewline();
            writeIndent(); oss << "\"category\": \""     << escapeJson(func.category)     << "\","; writeNewline();
            writeIndent(); oss << "\"description\": \""  << escapeJson(func.description)  << "\","; writeNewline();
            writeIndent(); oss << "\"isPublic\": "       << (func.isPublic ? "true" : "false") << ","; writeNewline();

            // inputs
            writeIndent(); oss << "\"inputs\": [";
            for (size_t ii = 0; ii < func.inputs.size(); ++ii)
            {
                const auto& inp = func.inputs[ii];
                oss << "{\"name\":\"" << escapeJson(inp.name) << "\",\"dataType\":" << static_cast<int>(inp.dataType) << "}";
                if (ii + 1 < func.inputs.size()) oss << ",";
            }
            oss << "],"; writeNewline();

            // outputs
            writeIndent(); oss << "\"outputs\": [";
            for (size_t oi = 0; oi < func.outputs.size(); ++oi)
            {
                const auto& out = func.outputs[oi];
                oss << "{\"name\":\"" << escapeJson(out.name) << "\",\"dataType\":" << static_cast<int>(out.dataType) << "}";
                if (oi + 1 < func.outputs.size()) oss << ",";
            }
            oss << "],"; writeNewline();

            // nodes (sub-graph) — serialize with full pin data so the runtime
            // can execute func.nodes directly when top-level nodes are unavailable
            writeIndent(); oss << "\"nodes\": [";
            for (size_t ni = 0; ni < func.nodes.size(); ++ni)
            {
                const auto& nd = func.nodes[ni];
                oss << "{\"id\":" << nd.id
                    << ",\"definitionId\":\"" << escapeJson(nd.definitionId) << "\""
                    << ",\"name\":\"" << escapeJson(nd.name) << "\"";
                if (!nd.pins.empty())
                {
                    oss << ",\"pins\":[";
                    for (size_t pi = 0; pi < nd.pins.size(); ++pi)
                    {
                        const auto& p = nd.pins[pi];
                        oss << "{\"id\":" << p.id
                            << ",\"kind\":" << static_cast<int>(p.kind)
                            << ",\"dataType\":" << static_cast<int>(p.dataType)
                            << ",\"isExec\":" << (p.isExec ? "true" : "false");
                        if (!p.name.empty())
                            oss << ",\"name\":\"" << escapeJson(p.name) << "\"";
                        if (p.defaultValue.type != PinDataType::Unknown)
                            oss << ",\"defaultValue\":" << variantToJson(p.defaultValue);
                        oss << "}";
                        if (pi + 1 < nd.pins.size()) oss << ",";
                    }
                    oss << "]";
                }
                oss << "}";
                if (ni + 1 < func.nodes.size()) oss << ",";
            }
            oss << "],"; writeNewline();

            // links
            writeIndent(); oss << "\"links\": [";
            for (size_t li2 = 0; li2 < func.links.size(); ++li2)
            {
                const auto& lk = func.links[li2];
                oss << "{\"id\":" << lk.id << ",\"startPinId\":" << lk.startPinId << ",\"endPinId\":" << lk.endPinId << "}";
                if (li2 + 1 < func.links.size()) oss << ",";
            }
            oss << "]"; writeNewline();

            indentLevel--;
            writeIndent(); oss << "}";
            if (fi + 1 < data.functions.size()) oss << ",";
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
    
#ifndef __EMSCRIPTEN__
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
#else
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
#endif
    
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
    
    // 结束根对象
    writeNewline();
    indentLevel--;
    writeIndent();
    oss << "}";
    
    return oss.str();
}


// ============================================================================
// JsonBlueprintExporter — 编辑器单文件导出
// ============================================================================

EditorExportResult JsonBlueprintExporter::exportEditorFiles(const BlueprintData& data,
    const std::string& runtimeFilePath,
    const ExportOptions& options) const
{
    EditorExportResult result;

#ifndef __EMSCRIPTEN__
    try
    {
#endif
        std::string runtimeJson = exportRuntimeToString(data, options);
        std::string editorJson  = exportEditorToString(data, options);

        // 单文件格式：{ "runtime": {...}, "editor": {...} }
        std::ostringstream merged;
        bool pretty = options.prettyPrint;
        const std::string nl  = pretty ? "\n" : "";
        const std::string ind = pretty ? "    " : "";

        merged << "{" << nl;
        merged << ind << "\"runtime\": " << runtimeJson << "," << nl;
        merged << ind << "\"editor\": "  << editorJson  << nl;
        merged << "}" << nl;

        std::string mergedStr = merged.str();

        std::string errorMsg;
        if (!m_fileSystem->WriteFile(runtimeFilePath, mergedStr, errorMsg))
        {
            result.errorMessage = "Write failed: " + errorMsg;
            return result;
        }

        result.success      = true;
        result.filePath     = runtimeFilePath;
        result.bytesWritten = mergedStr.size();

#ifndef __EMSCRIPTEN__
    }
    catch (const std::exception& e)
    {
        result.errorMessage = std::string("Exception: ") + e.what();
    }
#endif

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

    // 自动检测单文件新格式：顶层包含 "runtime" 子对象
    if (rootObj.contains("runtime") && rootObj["runtime"].type() == crude_json::type_t::object)
    {
        std::string runtimeJson = rootObj["runtime"].dump();
        // 先加载 runtime 数据
        result = importRuntimeFromString(runtimeJson, options);
        result.bytesRead = content.size();
        if (!result.success)
            return result;

        // 若顶层还有 "editor" 字段，合并编辑器附加数据（节点位置等）
        if (rootObj.contains("editor") && rootObj["editor"].type() == crude_json::type_t::object)
        {
            std::string editorJson = rootObj["editor"].dump();
            ImportResult merged = importEditorFromStrings(runtimeJson, editorJson, options);
            if (merged.success)
            {
                merged.bytesRead = content.size();
                return merged;
            }
            // 合并失败时仍返回 runtime 数据（editor 数据丢失但不致命）
            result.warnings.push_back("Editor data merge failed: " + merged.errorMessage);
        }
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
        result.data.metadata.schemaVersion = static_cast<int>(getNumber(meta, "schemaVersion", 0));
        result.data.metadata.blueprintClass = static_cast<BlueprintClass>(
            static_cast<int>(getNumber(meta, "blueprintClass", 0)));
        result.data.metadata.name        = getString(meta, "name");
        result.data.metadata.description  = getString(meta, "description");
        result.data.metadata.version      = getString(meta, "version");
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

        // dependencies：加载时按序读取（Runtime 用来决定 Library 加载顺序）
        if (meta.contains("dependencies") && meta["dependencies"].type() == crude_json::type_t::array)
        {
            for (auto& dep : meta["dependencies"].get<crude_json::array>())
            {
                if (dep.type() == crude_json::type_t::string)
                    result.data.metadata.dependencies.push_back(dep.get<std::string>());
            }
        }
    }

    // ---- Schema 版本检查 ----
    {
        int fileSchema = result.data.metadata.schemaVersion;
        if (fileSchema > BLUEPRINT_CURRENT_SCHEMA_VERSION)
        {
            result.errorMessage = "Blueprint file requires schema version "
                + std::to_string(fileSchema)
                + ", but this runtime only supports up to version "
                + std::to_string(BLUEPRINT_CURRENT_SCHEMA_VERSION)
                + ". Please update the application.";
            return result;
        }
        if (fileSchema > 0 && fileSchema < BLUEPRINT_CURRENT_SCHEMA_VERSION)
        {
            result.warnings.push_back(
                "Blueprint file uses older schema version "
                + std::to_string(fileSchema)
                + " (current: " + std::to_string(BLUEPRINT_CURRENT_SCHEMA_VERSION)
                + "). Data will be migrated automatically.");
        }
        // fileSchema == 0 表示老文件没有 schemaVersion 字段，视为版本 1
        if (fileSchema == 0)
        {
            result.data.metadata.schemaVersion = 1;
        }
    }

    // ---- 解析节点 ----
    if (rootObj.contains("nodes") && rootObj["nodes"].type() == crude_json::type_t::array)
    {
        for (auto& nodeJson : rootObj["nodes"].get<crude_json::array>())
        {
            if (nodeJson.type() != crude_json::type_t::object) continue;

            NodeInstance node;
            node.id                = static_cast<NodeId>(getNumber(nodeJson, "id"));
            node.definitionId      = getString(nodeJson, "definitionId");
            node.definitionVersion = static_cast<int>(getNumber(nodeJson, "definitionVersion", 1));
            node.name              = getString(nodeJson, "name");
            node.isEnabled         = getBool(nodeJson, "isEnabled", true);
            node.isCollapsed       = getBool(nodeJson, "isCollapsed", false);

            // 位置（编辑器数据，单文件格式从 editor 段读取；此处兼容直接在 runtime 段写入的场景）
            if (nodeJson.contains("position") && nodeJson["position"].type() == crude_json::type_t::object)
            {
                auto& pos = nodeJson["position"];
                node.position.x = static_cast<float>(getNumber(pos, "x"));
                node.position.y = static_cast<float>(getNumber(pos, "y"));
            }

            // 尺寸
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

                    // kind: int 格式（0=Input, 1=Output）
                    if (pinJson.contains("kind") && pinJson["kind"].type() == crude_json::type_t::number)
                        pin.kind = (static_cast<int>(pinJson["kind"].get<double>()) == 1) ? PinKind::Output : PinKind::Input;

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

    // ---- 解析函数定义 ----
    if (rootObj.contains("functions") && rootObj["functions"].type() == crude_json::type_t::array)
    {
        for (auto& funcJson : rootObj["functions"].get<crude_json::array>())
        {
            if (funcJson.type() != crude_json::type_t::object) continue;

            FunctionDefinition func;
            func.id          = getString(funcJson, "id");
            func.name        = getString(funcJson, "name");
            func.category    = getString(funcJson, "category");
            func.description = getString(funcJson, "description");
            func.isPublic    = getBool(funcJson, "isPublic", true);

            // inputs
            if (funcJson.contains("inputs") && funcJson["inputs"].type() == crude_json::type_t::array)
            {
                for (auto& inpJson : funcJson["inputs"].get<crude_json::array>())
                {
                    if (inpJson.type() != crude_json::type_t::object) continue;
                    VariableDefinition vd;
                    vd.name     = getString(inpJson, "name");
                    vd.dataType = static_cast<PinDataType>(static_cast<int>(getNumber(inpJson, "dataType")));
                    func.inputs.push_back(std::move(vd));
                }
            }
            // outputs
            if (funcJson.contains("outputs") && funcJson["outputs"].type() == crude_json::type_t::array)
            {
                for (auto& outJson : funcJson["outputs"].get<crude_json::array>())
                {
                    if (outJson.type() != crude_json::type_t::object) continue;
                    VariableDefinition vd;
                    vd.name     = getString(outJson, "name");
                    vd.dataType = static_cast<PinDataType>(static_cast<int>(getNumber(outJson, "dataType")));
                    func.outputs.push_back(std::move(vd));
                }
            }
            // nodes (sub-graph)
            if (funcJson.contains("nodes") && funcJson["nodes"].type() == crude_json::type_t::array)
            {
                for (auto& ndJson : funcJson["nodes"].get<crude_json::array>())
                {
                    if (ndJson.type() != crude_json::type_t::object) continue;
                    NodeInstance nd;
                    nd.id           = static_cast<NodeId>(getNumber(ndJson, "id"));
                    nd.definitionId = getString(ndJson, "definitionId");
                    nd.name         = getString(ndJson, "name");
                    // 反序列化完整 pin 数据（新格式；旧格式无 pins 字段，跳过兼容）
                    if (ndJson.contains("pins") && ndJson["pins"].type() == crude_json::type_t::array)
                    {
                        for (auto& pinJson : ndJson["pins"].get<crude_json::array>())
                        {
                            if (pinJson.type() != crude_json::type_t::object) continue;
                            PinInfo pin;
                            pin.id        = static_cast<PinId>(getNumber(pinJson, "id"));
                            pin.kind      = static_cast<PinKind>(static_cast<int>(getNumber(pinJson, "kind")));
                            pin.dataType  = static_cast<PinDataType>(static_cast<int>(getNumber(pinJson, "dataType")));
                            pin.isExec    = pinJson.contains("isExec") && pinJson["isExec"].type() == crude_json::type_t::boolean
                                            ? pinJson["isExec"].get<bool>() : false;
                            pin.name      = getString(pinJson, "name");
                            if (pinJson.contains("defaultValue"))
                                pin.defaultValue = jsonToVariant(pinJson["defaultValue"].dump(), pin.dataType);
                            nd.pins.push_back(std::move(pin));
                        }
                    }
                    func.nodes.push_back(std::move(nd));
                }
            }
            // links
            if (funcJson.contains("links") && funcJson["links"].type() == crude_json::type_t::array)
            {
                for (auto& lkJson : funcJson["links"].get<crude_json::array>())
                {
                    if (lkJson.type() != crude_json::type_t::object) continue;
                    LinkInstance lk;
                    lk.id         = static_cast<LinkId>(getNumber(lkJson, "id"));
                    lk.startPinId = static_cast<PinId>(getNumber(lkJson, "startPinId"));
                    lk.endPinId   = static_cast<PinId>(getNumber(lkJson, "endPinId"));
                    func.links.push_back(std::move(lk));
                }
            }

            result.data.functions.push_back(std::move(func));
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

    // ---- Schema 迁移（在所有数据解析完成后执行）----
    {
        int fileSchema = result.data.metadata.schemaVersion;
        if (fileSchema < BLUEPRINT_CURRENT_SCHEMA_VERSION)
            ApplySchemaMigrations(result.data, fileSchema);
    }

    result.success = true;
    return result;
}

ImportResult JsonBlueprintExporter::importRuntimeFromFile(const std::string& filePath, const ImportOptions& options) const
{
    ImportResult result;
    
#ifndef __EMSCRIPTEN__
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
#else
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
#endif
    
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
        return std::get<bool>(value.numericValue) ? "true" : "false";
    case PinDataType::Integer:
        return std::to_string(std::get<int64_t>(value.numericValue));
    case PinDataType::Float:
    {
        // %.17g: 最短精确往返表示，去除尾零；不用 std::to_string 的 %.6f
        char buf[64];
        std::snprintf(buf, sizeof(buf), "%.17g", std::get<double>(value.numericValue));
        return std::string(buf);
    }
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
            result.numericValue = val.get<bool>();
        break;
    case PinDataType::Integer:
        if (val.type() == crude_json::type_t::number)
            result.numericValue = static_cast<int64_t>(val.get<double>());
        break;
    case PinDataType::Float:
        if (val.type() == crude_json::type_t::number)
            result.numericValue = val.get<double>();
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

std::string BinaryBlueprintExporter::exportRuntimeToString(const BlueprintData& /*data*/, const ExportOptions& /*options*/) const
{
    return "";
}

ExportResult BinaryBlueprintExporter::exportRuntimeToFile(const BlueprintData& /*data*/, const std::string& /*filePath*/, const ExportOptions& /*options*/) const
{
    ExportResult result;
    result.errorMessage = "Binary export not yet implemented";
    return result;
}

ImportResult BinaryBlueprintExporter::importRuntimeFromString(const std::string& /*content*/, const ImportOptions& /*options*/) const
{
    ImportResult result;
    result.errorMessage = "Binary import not yet implemented";
    return result;
}

ImportResult BinaryBlueprintExporter::importRuntimeFromFile(const std::string& /*filePath*/, const ImportOptions& /*options*/) const
{
    ImportResult result;
    result.errorMessage = "Binary import not yet implemented";
    return result;
}

EditorExportResult BinaryBlueprintExporter::exportEditorFiles(const BlueprintData& /*data*/, const std::string& /*filePath*/, const ExportOptions& /*options*/) const
{
    EditorExportResult result;
    result.errorMessage = "Binary editor export not yet implemented";
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
