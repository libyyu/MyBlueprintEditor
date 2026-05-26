// Runtime/BlueprintExporter.cpp - 蓝图数据导出器入口
//
// 该文件仅包含 BinaryBlueprintExporter 的占位实现。
//
// JsonBlueprintExporter 的实现已按职责拆分到 Runtime/exporter/ 子目录：
//
//   BlueprintExporter_Internal.h         — 共享 helpers（escapeJson / indentJson / ...）
//   BlueprintExporter_RuntimeExport.cpp  — exportRuntimeToString / exportRuntimeToFile
//   BlueprintExporter_EditorExport.cpp   — exportEditorToString / exportEditorFiles
//   BlueprintExporter_RuntimeImport.cpp  — importRuntimeFromString/File + importMetadataFrom*
//   BlueprintExporter_EditorImport.cpp   — importEditorFromStrings
//   BlueprintExporter_Util.cpp           — validate / variantToJson / jsonToVariant / getSupportedFormats

#include "BlueprintExporter.h"

namespace NodeEditor {
namespace Runtime {

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

ImportMetaResult BinaryBlueprintExporter::importMetadataFromFile(const std::string& filePath, const ImportOptions& /*options*/) const
{
    ImportMetaResult result;
    result.errorMessage = "Binary import not yet implemented";
    return result;
}

ImportMetaResult BinaryBlueprintExporter::importMetadataFromString(const std::string& content, const ImportOptions& /*options*/) const
{
    ImportMetaResult result;
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
