// Runtime/FileSystem.h - 文件系统抽象接口
// 该文件定义了文件读写的抽象接口，允许第三方引擎提供自定义的资源加载方式
//
// 使用方式:
//   1. 默认使用 DefaultFileSystem（std::ifstream/std::ofstream 磁盘读写）
//   2. 第三方引擎可继承 IFileSystem 并实现自己的资源加载逻辑
//   3. 通过 SetFileSystem() 或构造函数注入自定义实现
//
// 示例:
//   class MyEngineFileSystem : public NodeEditor::Runtime::IFileSystem {
//       bool ReadFile(const std::string& path, std::string& outContent, std::string& outError) override {
//           // 使用引擎的资源管理器加载文件
//           outContent = MyEngine::ResourceManager::LoadText(path);
//           return !outContent.empty();
//       }
//       bool WriteFile(const std::string& path, const std::string& content, std::string& outError) override {
//           return MyEngine::ResourceManager::SaveText(path, content);
//       }
//       bool FileExists(const std::string& path) override {
//           return MyEngine::ResourceManager::Exists(path);
//       }
//   };
//
//   auto fs = std::make_shared<MyEngineFileSystem>();
//   NodeEditor::Runtime::SetDefaultFileSystem(fs);

#pragma once

#include <string>
#include <memory>
#include <fstream>
#include <sstream>

namespace NodeEditor {
namespace Runtime {

// ============================================================================
// 文件系统抽象接口
// ============================================================================

class IFileSystem
{
public:
    virtual ~IFileSystem() = default;

    // 读取文件内容到字符串
    // 返回 true 表示成功，false 表示失败（错误信息写入 outError）
    virtual bool ReadFile(const std::string& path, std::string& outContent, std::string& outError) = 0;

    // 写入字符串内容到文件
    // 返回 true 表示成功，false 表示失败（错误信息写入 outError）
    virtual bool WriteFile(const std::string& path, const std::string& content, std::string& outError) = 0;

    // 检查文件是否存在
    virtual bool FileExists(const std::string& path) = 0;
};

// ============================================================================
// 默认文件系统实现（使用标准 C++ 文件流，直接磁盘读写）
// ============================================================================

class DefaultFileSystem : public IFileSystem
{
public:
    bool ReadFile(const std::string& path, std::string& outContent, std::string& outError) override
    {
        std::ifstream file(path, std::ios::binary);
        if (!file.is_open())
        {
            outError = "Failed to open file: " + path;
            return false;
        }
        std::stringstream buffer;
        buffer << file.rdbuf();
        file.close();
        outContent = buffer.str();
        return true;
    }

    bool WriteFile(const std::string& path, const std::string& content, std::string& outError) override
    {
        std::ofstream file(path, std::ios::binary);
        if (!file.is_open())
        {
            outError = "Failed to open file for writing: " + path;
            return false;
        }
        file << content;
        file.close();
        return true;
    }

    bool FileExists(const std::string& path) override
    {
        std::ifstream file(path);
        return file.good();
    }
};

// ============================================================================
// 全局默认文件系统（单例模式）
// ============================================================================

// 获取当前全局默认文件系统
// 如果未设置自定义实现，返回内置的 DefaultFileSystem
inline std::shared_ptr<IFileSystem>& GetDefaultFileSystemRef()
{
    static std::shared_ptr<IFileSystem> s_defaultFS = std::make_shared<DefaultFileSystem>();
    return s_defaultFS;
}

// 获取全局默认文件系统（只读）
inline std::shared_ptr<IFileSystem> GetDefaultFileSystem()
{
    return GetDefaultFileSystemRef();
}

// 设置全局默认文件系统
// 传入 nullptr 将恢复为内置的 DefaultFileSystem
inline void SetDefaultFileSystem(std::shared_ptr<IFileSystem> fs)
{
    if (fs)
        GetDefaultFileSystemRef() = std::move(fs);
    else
        GetDefaultFileSystemRef() = std::make_shared<DefaultFileSystem>();
}

} // namespace Runtime
} // namespace NodeEditor
