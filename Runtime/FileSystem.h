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
#include <vector>
#include <memory>

// 平台头文件
#if defined(__EMSCRIPTEN__)
#   include <emscripten.h>
#   include <cstdlib>   // malloc / free
#   include <cstring>   // memcpy
#elif !defined(BLUEPRINT_NO_FILESYSTEM)
#   include <fstream>
#   include <sstream>
#   include <filesystem>
#   include <cerrno>
#   include <cstring>
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

    // ── 文件读写 ──────────────────────────────────────────────────────────────
    // 读取文件内容到字符串（二进制安全）
    virtual bool ReadFile(const std::string& path,
                          std::string&       outContent,
                          std::string&       outError) = 0;

    // 写入内容到文件（覆盖）
    virtual bool WriteFile(const std::string& path,
                           const std::string& content,
                           std::string&       outError) = 0;

    // 追加内容到文件
    virtual bool AppendFile(const std::string& path,
                            const std::string& content,
                            std::string&       outError) = 0;

    // ── 文件/目录查询 ─────────────────────────────────────────────────────────
    // 检查路径是否存在（文件或目录）
    virtual bool FileExists(const std::string& path) = 0;

    // 返回文件字节数，-1 表示不存在或失败
    virtual int64_t GetFileSize(const std::string& path) = 0;

    // ── 目录操作 ──────────────────────────────────────────────────────────────
    // 列出目录内容；pattern 为简单子串过滤（空=全部）
    virtual bool ListDir(const std::string& path,
                         const std::string& pattern,
                         std::vector<std::string>& outNames,
                         std::string& outError) = 0;

    // 递归创建目录
    virtual bool MakeDir(const std::string& path, std::string& outError) = 0;

    // 删除文件（ignoreNotFound=true 时不存在不算错误）
    virtual bool DeleteFile(const std::string& path,
                            bool ignoreNotFound,
                            std::string& outError) = 0;

    // ── 路径辅助（纯字符串，不访问磁盘）────────────────────────────────────
    // 返回路径最后一段（文件名含扩展名）
    virtual std::string GetBaseName(const std::string& path) = 0;

    // 返回父目录路径
    virtual std::string GetDirName(const std::string& path) = 0;

    // 拼接路径
    virtual std::string JoinPath(const std::string& base, const std::string& part) = 0;
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
    // ReadFile  : emscripten_wget_data()（同步 XHR）从 StreamingAssets 加载
    // WriteFile / AppendFile : 不支持（WebGL 无持久磁盘）
    // ListDir / MakeDir / DeleteFile : 不支持
    // GetBaseName / GetDirName / JoinPath : 纯字符串，不访问磁盘
    // ------------------------------------------------------------------

#   ifndef BLUEPRINT_STREAMING_ASSETS_BASE
#       define BLUEPRINT_STREAMING_ASSETS_BASE "StreamingAssets"
#   endif

    bool ReadFile(const std::string& path,
                  std::string&       outContent,
                  std::string&       outError) override
    {
#if defined(BP_NODE_ENV)
        FILE* f = fopen(path.c_str(), "rb");
        if (!f) { outError = "File not found: " + path; return false; }
        fseek(f, 0, SEEK_END);
        long len = ftell(f); fseek(f, 0, SEEK_SET);
        outContent.resize(static_cast<size_t>(len));
        if (len > 0) fread(&outContent[0], 1, static_cast<size_t>(len), f);
        fclose(f);
        return true;
#else
        std::string url = std::string(BLUEPRINT_STREAMING_ASSETS_BASE) + "/" + path;
        void* buf = nullptr; int size = 0, err = 0;
        emscripten_wget_data(url.c_str(), &buf, &size, &err);
        if (err != 0 || buf == nullptr || size <= 0) {
            outError = "Failed to fetch: " + url +
                       " (emscripten_wget_data err=" + std::to_string(err) + ")";
            if (buf) free(buf);
            return false;
        }
        outContent.assign(static_cast<const char*>(buf), static_cast<size_t>(size));
        free(buf);
        return true;
#endif
    }

    bool WriteFile(const std::string&, const std::string&, std::string& outError) override {
        outError = "WriteFile not supported on WebGL. Use SetDefaultFileSystem() to inject a custom IFileSystem.";
        return false;
    }
    bool AppendFile(const std::string&, const std::string&, std::string& outError) override {
        outError = "AppendFile not supported on WebGL.";
        return false;
    }

    bool FileExists(const std::string& path) override
    {
        std::string url = std::string(BLUEPRINT_STREAMING_ASSETS_BASE) + "/" + path;
        int status = EM_ASM_INT({
            var url = UTF8ToString($0);
            var xhr = new XMLHttpRequest();
            xhr.open('HEAD', url, false);
            try { xhr.send(); } catch(e) { return 0; }
            return xhr.status;
        }, url.c_str());
        return (status >= 200 && status < 300);
    }

    int64_t GetFileSize(const std::string&) override { return -1; }

    bool ListDir(const std::string&, const std::string&,
                 std::vector<std::string>&, std::string& outError) override {
        outError = "ListDir not supported on WebGL.";
        return false;
    }
    bool MakeDir(const std::string&, std::string& outError) override {
        outError = "MakeDir not supported on WebGL.";
        return false;
    }
    bool DeleteFile(const std::string&, bool, std::string& outError) override {
        outError = "DeleteFile not supported on WebGL.";
        return false;
    }

    std::string GetBaseName(const std::string& path) override { return _baseName(path); }
    std::string GetDirName(const std::string& path)  override { return _dirName(path); }
    std::string JoinPath(const std::string& base, const std::string& part) override { return _join(base, part); }

#else // !__EMSCRIPTEN__
    // ------------------------------------------------------------------
    // 普通平台（Windows / Linux / macOS / Android / iOS）
    // ------------------------------------------------------------------

    bool ReadFile(const std::string& path,
                  std::string&       outContent,
                  std::string&       outError) override
    {
        std::ifstream file(path, std::ios::binary);
        if (!file.is_open()) {
            outError = "Failed to open file: " + path;
            return false;
        }
        std::stringstream buf;
        buf << file.rdbuf();
        outContent = buf.str();
        return true;
    }

    bool WriteFile(const std::string& path,
                   const std::string& content,
                   std::string&       outError) override
    {
        _ensureParentDir(path);
        std::ofstream file(path, std::ios::binary | std::ios::trunc);
        if (!file.is_open()) {
            outError = "Failed to open file for writing: " + path + " (errno=" + std::to_string(errno) + ")";
            return false;
        }
        file << content;
        return true;
    }

    bool AppendFile(const std::string& path,
                    const std::string& content,
                    std::string&       outError) override
    {
        _ensureParentDir(path);
        std::ofstream file(path, std::ios::binary | std::ios::app);
        if (!file.is_open()) {
            outError = "Failed to open file for append: " + path + " (errno=" + std::to_string(errno) + ")";
            return false;
        }
        file << content;
        return true;
    }

    bool FileExists(const std::string& path) override
    {
        std::error_code ec;
        return std::filesystem::exists(std::filesystem::path(path), ec);
    }

    int64_t GetFileSize(const std::string& path) override
    {
        std::error_code ec;
        auto sz = std::filesystem::file_size(std::filesystem::path(path), ec);
        return ec ? -1LL : static_cast<int64_t>(sz);
    }

    bool ListDir(const std::string& path,
                 const std::string& pattern,
                 std::vector<std::string>& outNames,
                 std::string& outError) override
    {
        std::error_code ec;
        if (!std::filesystem::is_directory(std::filesystem::path(path), ec)) {
            outError = "Not a directory: " + path;
            return false;
        }
        for (const auto& entry : std::filesystem::directory_iterator(std::filesystem::path(path), ec)) {
            std::string name = entry.path().filename().string();
            if (pattern.empty() || name.find(pattern) != std::string::npos)
                outNames.push_back(name);
        }
        return true;
    }

    bool MakeDir(const std::string& path, std::string& outError) override
    {
        std::error_code ec;
        std::filesystem::create_directories(std::filesystem::path(path), ec);
        if (ec) { outError = ec.message(); return false; }
        return true;
    }

    bool DeleteFile(const std::string& path, bool ignoreNotFound, std::string& outError) override
    {
        std::error_code ec;
        bool removed = std::filesystem::remove(std::filesystem::path(path), ec);
        if (!removed && !ignoreNotFound && ec) {
            outError = ec.message();
            return false;
        }
        return true;
    }

    std::string GetBaseName(const std::string& path) override {
        return std::filesystem::path(path).filename().string();
    }
    std::string GetDirName(const std::string& path) override {
        return std::filesystem::path(path).parent_path().string();
    }
    std::string JoinPath(const std::string& base, const std::string& part) override {
        return (std::filesystem::path(base) / std::filesystem::path(part)).string();
    }

private:
    static void _ensureParentDir(const std::string& path) {
        std::error_code ec;
        auto p = std::filesystem::path(path);
        if (p.has_parent_path())
            std::filesystem::create_directories(p.parent_path(), ec);
    }
#endif // __EMSCRIPTEN__

private:
    // 纯字符串路径辅助（跨平台，不依赖 std::filesystem）
    static std::string _baseName(const std::string& path) {
        auto p = path;
        while (!p.empty() && (p.back() == '/' || p.back() == '\\')) p.pop_back();
        auto pos = p.find_last_of("/\\");
        return (pos == std::string::npos) ? p : p.substr(pos + 1);
    }
    static std::string _dirName(const std::string& path) {
        auto p = path;
        while (!p.empty() && (p.back() == '/' || p.back() == '\\')) p.pop_back();
        auto pos = p.find_last_of("/\\");
        return (pos == std::string::npos) ? "." : p.substr(0, pos);
    }
    static std::string _join(const std::string& base, const std::string& part) {
        if (base.empty()) return part;
        char last = base.back();
        if (last == '/' || last == '\\') return base + part;
        return base + "/" + part;
    }
};

#endif // !BLUEPRINT_NO_FILESYSTEM

// ============================================================================
// 全局默认文件系统（单例）
// ============================================================================

/// 获取全局 IFileSystem 引用（内部使用）
BLUEPRINT_API std::shared_ptr<IFileSystem>& GetDefaultFileSystemRef();

/// 获取当前全局文件系统
BLUEPRINT_API std::shared_ptr<IFileSystem> GetDefaultFileSystem();

/// 替换全局文件系统。传入 nullptr 则恢复为内置 DefaultFileSystem（若可用）。
BLUEPRINT_API void SetDefaultFileSystem(std::shared_ptr<IFileSystem> fs);

} // namespace Runtime
} // namespace NodeEditor
