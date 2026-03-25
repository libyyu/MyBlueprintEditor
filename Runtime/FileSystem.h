// Runtime/FileSystem.h - 文件系统抽象接口
// 该文件定义了文件读写的抽象接口，允许第三方引擎提供自定义的资源加载方式。
//
// 平台说明
// --------
// 普通平台（Windows / Linux / macOS）:
//   DefaultFileSystem 使用 std::fstream 直接读写磁盘，开箱即用。
//
// Emscripten / Unity WebGL:
//   DefaultFileSystem 自动切换为基于 emscripten_wget_data() 的同步 HTTP
//   加载实现，从 Unity 的 StreamingAssets 路径加载文件，无需任何额外初始化。
//   - 读取路径自动补全为 StreamingAssets/<path>（可通过
//     BLUEPRINT_STREAMING_ASSETS_BASE 宏在编译期覆盖，默认 "StreamingAssets"）
//   - WriteFile 在 WebGL 下不支持（返回 false），如需持久化请自行注入实现
//   - 若需要完全自定义加载，仍可调用 SetDefaultFileSystem() 注入任意 IFileSystem
//
// BLUEPRINT_NO_FILESYSTEM:
//   定义此宏可把整个 DefaultFileSystem 排除出编译（极度裁剪场景）。
//   此时全局指针为 nullptr，调用方必须在使用前注入实现。
//
// 自定义示例:
//   class MyFS : public NodeEditor::Runtime::IFileSystem { ... };
//   NodeEditor::Runtime::SetDefaultFileSystem(std::make_shared<MyFS>());

#pragma once

#include "BlueprintExport.h"

#include <string>
#include <memory>

// 平台头文件
#if defined(__EMSCRIPTEN__)
#   include <emscripten.h>
#   include <cstdlib>   // malloc / free
#   include <cstring>   // memcpy
#elif !defined(BLUEPRINT_NO_FILESYSTEM)
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
    virtual bool ReadFile(const std::string& path,
                          std::string&       outContent,
                          std::string&       outError) = 0;

    // 写入字符串内容到文件
    // 返回 true 表示成功，false 表示失败（错误信息写入 outError）
    virtual bool WriteFile(const std::string& path,
                           const std::string& content,
                           std::string&       outError) = 0;

    // 检查文件是否存在
    virtual bool FileExists(const std::string& path) = 0;
};

// ============================================================================
// 默认文件系统实现
// ============================================================================

#if defined(BLUEPRINT_NO_FILESYSTEM)

// 极度裁剪模式：不提供默认实现，调用方须注入 IFileSystem
// class DefaultFileSystem intentionally omitted.

#else // !BLUEPRINT_NO_FILESYSTEM

class BLUEPRINT_API DefaultFileSystem : public IFileSystem
{
public:

#if defined(__EMSCRIPTEN__)
    // ------------------------------------------------------------------
    // Emscripten / Unity WebGL 实现
    // ------------------------------------------------------------------
    // 使用 emscripten_wget_data()（同步 XHR）从 StreamingAssets 加载文件。
    // Unity WebGL 在构建时会把 StreamingAssets 发布到 HTTP 服务器，
    // 路径格式为：  <base>/<relpath>
    // 默认 base 为 "StreamingAssets"，可在编译期通过宏覆盖：
    //   -DBLUEPRINT_STREAMING_ASSETS_BASE=\"MyGame/StreamingAssets\"
    // ------------------------------------------------------------------

#   ifndef BLUEPRINT_STREAMING_ASSETS_BASE
#       define BLUEPRINT_STREAMING_ASSETS_BASE "StreamingAssets"
#   endif

    bool ReadFile(const std::string& path,
                  std::string&       outContent,
                  std::string&       outError) override
    {
        // 拼接 URL：StreamingAssets/<path>
        std::string url = std::string(BLUEPRINT_STREAMING_ASSETS_BASE) + "/" + path;

        void* buf  = nullptr;
        int   size = 0;
        int   err  = 0;

        // 同步 XHR：阻塞直到完成（Unity WebGL 主线程可用，Worker 线程同样可用）
        emscripten_wget_data(url.c_str(), &buf, &size, &err);

        if (err != 0 || buf == nullptr || size <= 0)
        {
            outError = "Failed to fetch: " + url +
                       " (emscripten_wget_data err=" + std::to_string(err) + ")";
            if (buf) { free(buf); }
            return false;
        }

        outContent.assign(static_cast<const char*>(buf),
                          static_cast<size_t>(size));
        free(buf);
        return true;
    }

    bool WriteFile(const std::string& /*path*/,
                   const std::string& /*content*/,
                   std::string&       outError) override
    {
        // WebGL 无持久文件系统；如需写入请自行注入 IFileSystem 实现
        // （可用 IDBFS / localStorage 桥接）
        outError = "DefaultFileSystem::WriteFile is not supported on WebGL. "
                   "Inject a custom IFileSystem via SetDefaultFileSystem().";
        return false;
    }

    bool FileExists(const std::string& path) override
    {
        // 同步 HEAD 请求判断资源是否存在
        std::string url = std::string(BLUEPRINT_STREAMING_ASSETS_BASE) + "/" + path;

        // emscripten_wget_data 本身不提供 HEAD，用轻量 JS 检查
        // 通过 EM_ASM_INT 发起同步 XMLHttpRequest HEAD
        int status = EM_ASM_INT({
            var url = UTF8ToString($0);
            var xhr = new XMLHttpRequest();
            xhr.open('HEAD', url, false);   // false = synchronous
            try { xhr.send(); } catch(e) { return 0; }
            return xhr.status;
        }, url.c_str());

        return (status >= 200 && status < 300);
    }

#else
    // ------------------------------------------------------------------
    // 普通平台（Windows / Linux / macOS）实现 —— std::fstream
    // ------------------------------------------------------------------

    bool ReadFile(const std::string& path,
                  std::string&       outContent,
                  std::string&       outError) override
    {
        std::ifstream file(path, std::ios::binary);
        if (!file.is_open())
        {
            outError = "Failed to open file: " + path;
            return false;
        }
        std::stringstream buffer;
        buffer << file.rdbuf();
        outContent = buffer.str();
        return true;
    }

    bool WriteFile(const std::string& path,
                   const std::string& content,
                   std::string&       outError) override
    {
        std::ofstream file(path, std::ios::binary);
        if (!file.is_open())
        {
            outError = "Failed to open file for writing: " + path;
            return false;
        }
        file << content;
        return true;
    }

    bool FileExists(const std::string& path) override
    {
        std::ifstream file(path);
        return file.good();
    }

#endif // __EMSCRIPTEN__
};

#endif // !BLUEPRINT_NO_FILESYSTEM

// ============================================================================
// 全局默认文件系统（单例）
// ============================================================================

/// 获取全局 IFileSystem 引用（内部使用）
inline std::shared_ptr<IFileSystem>& GetDefaultFileSystemRef()
{
#if defined(BLUEPRINT_NO_FILESYSTEM)
    // 裁剪模式：初始 nullptr，调用方必须先 SetDefaultFileSystem()
    static std::shared_ptr<IFileSystem> s_fs;
#else
    static std::shared_ptr<IFileSystem> s_fs =
        std::make_shared<DefaultFileSystem>();
#endif
    return s_fs;
}

/// 获取当前全局文件系统
inline std::shared_ptr<IFileSystem> GetDefaultFileSystem()
{
    return GetDefaultFileSystemRef();
}

/// 替换全局文件系统。传入 nullptr 则恢复为内置 DefaultFileSystem（若可用）。
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
