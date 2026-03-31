// Runtime/BlueprintExporter.h - 蓝图数据导出接口
// 该文件定义了蓝图数据的导入导出接口，支持多种格式
//
// 导出设计（单文件模式）:
//   - 蓝图文件 (.bjson) 内部结构：
//       { "runtime": { ... }, "editor": { ... } }
//   - 运行时只读取 "runtime" 字段，"editor" 字段被忽略（零开销）
//   - 编辑器读写完整文件

#pragma once
#include "BlueprintExport.h"

#ifdef _MSC_VER
#   pragma warning(push)
#   pragma warning(disable: 4251)
#endif

#include "BlueprintData.h"
#include "FileSystem.h"
#include <string>
#include <functional>
#include <memory>

namespace NodeEditor {
namespace Runtime {

// ============================================================================
// 导出格式
// ============================================================================

enum class ExportFormat
{
    JSON,       // JSON 格式（默认）
    Binary,     // 二进制格式（高效）
    XML,        // XML 格式（兼容性好）
    Custom      // 自定义格式
};

// ============================================================================
// 导出选项
// ============================================================================

struct ExportOptions
{
    ExportFormat     format = ExportFormat::JSON;
    bool             prettyPrint = true;           // JSON 是否美化输出
    bool             includeMetadata = true;       // 是否包含元数据
    bool             compress = false;             // 是否压缩
    int              indent = 4;                   // JSON 缩进空格数
    
    // 自定义属性过滤器
    std::function<bool(const std::string& key)> propertyFilter;
    
    // 自定义节点过滤器
    std::function<bool(const NodeInstance& node)> nodeFilter;
};

// ============================================================================
// 导入选项
// ============================================================================

struct ImportOptions
{
    bool             validateOnLoad = true;       // 加载时是否验证
    bool             mergeWithExisting = false;   // 是否与现有数据合并
    bool             generateNewIds = false;      // 是否生成新ID（避免冲突）
    bool             repairBrokenLinks = true;    // 是否修复断开的链接
};

// ============================================================================
// 导入导出结果
// ============================================================================

struct ExportResult
{
    bool                success = false;
    std::string         errorMessage;
    std::string         outputPath;               // 成功时的输出路径
    size_t              bytesWritten = 0;         // 写入的字节数
};

// 编辑器导出结果（单文件模式）
struct EditorExportResult
{
    bool                success = false;
    std::string         errorMessage;
    std::string         filePath;                 // 单文件路径
    size_t              bytesWritten = 0;
};

struct ImportResult
{
    bool                success = false;
    std::string         errorMessage;
    BlueprintData       data;                     // 成功时加载的数据
    size_t              bytesRead = 0;            // 读取的字节数
    std::vector<std::string> warnings;            // 警告信息
};

// ============================================================================
// 蓝图导入导出接口
// ============================================================================

class BLUEPRINT_API IBlueprintExporter
{
public:
    virtual ~IBlueprintExporter() = default;
    
    // ---- Runtime 文件导出/导入 ----

    // 导出 Runtime 数据到字符串（只包含执行所需的最小数据集，内部无 editor 段）
    virtual std::string exportRuntimeToString(const BlueprintData& data, const ExportOptions& options = ExportOptions()) const = 0;

    // 导出 Runtime 数据到文件
    virtual ExportResult exportRuntimeToFile(const BlueprintData& data, const std::string& filePath, const ExportOptions& options = ExportOptions()) const = 0;

    // 从字符串导入（自动兼容：有 "runtime" 根节点则同时合并 editor 数据）
    virtual ImportResult importRuntimeFromString(const std::string& content, const ImportOptions& options = ImportOptions()) const = 0;

    // 从文件导入（自动兼容）
    virtual ImportResult importRuntimeFromFile(const std::string& filePath, const ImportOptions& options = ImportOptions()) const = 0;

    // ---- 单文件导出（runtime + editor 合并）----

    // 导出完整数据到单 .bjson 文件：{ "runtime": {...}, "editor": {...} }
    virtual EditorExportResult exportEditorFiles(const BlueprintData& data, const std::string& filePath, const ExportOptions& options = ExportOptions()) const = 0;

    // ---- 通用 ----

    // 验证数据
    virtual bool validate(const BlueprintData& data, std::vector<std::string>& errors) const = 0;

    // 获取支持的格式
    virtual std::vector<ExportFormat> getSupportedFormats() const = 0;
};

// ============================================================================
// JSON 导出器实现
// ============================================================================

class BLUEPRINT_API JsonBlueprintExporter : public IBlueprintExporter
{
public:
    // 默认构造：使用全局默认文件系统
    JsonBlueprintExporter() : m_fileSystem(GetDefaultFileSystem()) {}

    // 自定义文件系统构造
    explicit JsonBlueprintExporter(std::shared_ptr<IFileSystem> fs)
        : m_fileSystem(fs ? std::move(fs) : GetDefaultFileSystem()) {}

    // 获取/设置文件系统
    std::shared_ptr<IFileSystem> GetFileSystem() const { return m_fileSystem; }
    void SetFileSystem(std::shared_ptr<IFileSystem> fs) { m_fileSystem = fs ? std::move(fs) : GetDefaultFileSystem(); }

    // Runtime
    std::string  exportRuntimeToString(const BlueprintData& data, const ExportOptions& options = ExportOptions()) const override;
    ExportResult exportRuntimeToFile(const BlueprintData& data, const std::string& filePath, const ExportOptions& options = ExportOptions()) const override;
    ImportResult importRuntimeFromString(const std::string& content, const ImportOptions& options = ImportOptions()) const override;
    ImportResult importRuntimeFromFile(const std::string& filePath, const ImportOptions& options = ImportOptions()) const override;

    // 单文件导出（runtime + editor 合并）
    EditorExportResult exportEditorFiles(const BlueprintData& data, const std::string& filePath, const ExportOptions& options = ExportOptions()) const override;



    // 通用
    bool validate(const BlueprintData& data, std::vector<std::string>& errors) const override;
    std::vector<ExportFormat> getSupportedFormats() const override;

private:
    std::shared_ptr<IFileSystem> m_fileSystem;

    // 内部辅助（不对外暴露）
    std::string exportEditorToString(const BlueprintData& data, const ExportOptions& options = ExportOptions()) const;
    ImportResult importEditorFromStrings(const std::string& runtimeContent, const std::string& editorContent, const ImportOptions& options = ImportOptions()) const;

    std::string variantToJson(const Variant& value) const;
    Variant jsonToVariant(const std::string& json, PinDataType type) const;
};

// ============================================================================
// 二进制导出器实现（头文件声明，实现在 cpp 中）
// ============================================================================

class BLUEPRINT_API BinaryBlueprintExporter : public IBlueprintExporter
{
public:
    std::string  exportRuntimeToString(const BlueprintData& data, const ExportOptions& options = ExportOptions()) const override;
    ExportResult exportRuntimeToFile(const BlueprintData& data, const std::string& filePath, const ExportOptions& options = ExportOptions()) const override;
    ImportResult importRuntimeFromString(const std::string& content, const ImportOptions& options = ImportOptions()) const override;
    ImportResult importRuntimeFromFile(const std::string& filePath, const ImportOptions& options = ImportOptions()) const override;

    EditorExportResult exportEditorFiles(const BlueprintData& data, const std::string& filePath, const ExportOptions& options = ExportOptions()) const override;



    bool validate(const BlueprintData& data, std::vector<std::string>& errors) const override;
    std::vector<ExportFormat> getSupportedFormats() const override;
};

// ============================================================================
// 工厂函数
// ============================================================================

// 创建导出器
inline std::unique_ptr<IBlueprintExporter> createExporter(ExportFormat format)
{
    switch (format)
    {
    case ExportFormat::JSON:
        return std::make_unique<JsonBlueprintExporter>();
    case ExportFormat::Binary:
        return std::make_unique<BinaryBlueprintExporter>();
    default:
        return nullptr;
    }
}

} // namespace Runtime
} // namespace NodeEditor
