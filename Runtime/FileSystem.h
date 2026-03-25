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

#include "BlueprintExport.h"  // BLUEPRINT_API, BLUEPRINT_PLATFORM_EMSCRIPTEN

#include <string>
#include <memory>
#ifndef BLUEPRINT_NO_FILESYSTEM
#   include <fstream>
#   include <sstream>
#endif

namespace NodeEditor {
namespace Runtime {

// ============================================================================
// 文件系统抽象接口
// ============================================================================

class BLUEPRINT_API IFileSystem
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
// 在 Emscripten/WebGL 下需要注意：std::fstream 映射到 Emscripten 的虚拟文件系统，
// 需要通过 FS.mount(MEMFS/IDBFS, ...) 挂载才能正常工作。
// 若定义了 BLUEPRINT_NO_FILESYSTEM，则不编译此实现，
// 调用者须通过 SetDefaultFileSystem() 注入自定义 IFileSystem。
// ============================================================================

#if defined(BLUEPRINT_NO_FILESYSTEM)

// 占位：不提供默认实现，运行时必须注入 IFileSystem
// class DefaultFileSystem intentionally omitted.

#else

class BLUEPRINT_API DefaultFileSystem : public IFileSystem
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

#endif // !BLUEPRINT_NO_FILESYSTEM

// ============================================================================
// 全局默认文件系统（单例模式）
// ============================================================================

// 获取当前全局默认文件系统
// 如果未设置自定义实现，返回内置的 DefaultFileSystem
// 注意：当 BLUEPRINT_NO_FILESYSTEM 定义时，初始值为 nullptr，
//       必须在使用前调用 SetDefaultFileSystem() 注入实现。
inline std::shared_ptr<IFileSystem>& GetDefaultFileSystemRef()
{
#if defined(BLUEPRINT_NO_FILESYSTEM)
    static std::shared_ptr<IFileSystem> s_defaultFS;  // nullptr – caller must inject
#else
    static std::shared_ptr<IFileSystem> s_defaultFS = std::make_shared<DefaultFileSystem>();
#endif
    return s_defaultFS;
}

// 获取全局默认文件系统（只读）
inline std::shared_ptr<IFileSystem> GetDefaultFileSystem()
{
    return GetDefaultFileSystemRef();
}

// 设置全局默认文件系统
// 传入 nullptr 将恢复为内置的 DefaultFileSystem（若可用）
inline void SetDefaultFileSystem(std::shared_ptr<IFileSystem> fs)
{
    if (fs)
    {
        GetDefaultFileSystemRef() = std::move(fs);
    }
    else
    {
#if !defined(BLUEPRINT_NO_FILESYSTEM)
        GetDefaultFileSystemRef() = std::make_shared<DefaultFileSystem>();
#else
        GetDefaultFileSystemRef() = nullptr;
#endif
    }
}

} // namespace Runtime
} // namespace NodeEditor
