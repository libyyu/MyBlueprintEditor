// BlueprintEditor/PathUtils.h
// 公共路径工具函数（仅编辑器层使用，全部 inline）
// 所有返回路径字符串的函数均保证使用正斜杠 '/'
#pragma once
#include <string>
#include <filesystem>

namespace BpPath {

namespace fs = std::filesystem;

// ── 分隔符规范化 ──────────────────────────────────────────────────────────────

/// 将路径中所有反斜杠替换为正斜杠（in-place 版本）
inline void NormSlashInPlace(std::string& s)
{
    for (char& c : s) if (c == '\\') c = '/';
}

/// 将路径中所有反斜杠替换为正斜杠（返回副本）
inline std::string NormSlash(std::string s)
{
    NormSlashInPlace(s);
    return s;
}

/// fs::path → 正斜杠 string（替代 .string()，避免 Windows 反斜杠）
inline std::string ToStr(const fs::path& p)
{
    return NormSlash(p.string());
}

// ── 路径分解 ──────────────────────────────────────────────────────────────────

/// 取文件名（不含目录、不含扩展名），e.g. "/a/b/Foo.bjson" → "Foo"
inline std::string Stem(const std::string& path)
{
    return fs::path(path).stem().string();
}

/// 取扩展名（含点），e.g. "/a/Foo.bjson" → ".bjson"
inline std::string Extension(const std::string& path)
{
    return fs::path(path).extension().string();
}

/// 取所在目录（正斜杠），e.g. "/a/b/Foo.bjson" → "/a/b"
inline std::string ParentDir(const std::string& path)
{
    return ToStr(fs::path(path).parent_path());
}

/// 取文件名（不含扩展名），同 Stem，语义更明确
inline std::string BaseName(const std::string& path)
{
    return Stem(path);
}

// ── 工程相对路径工具 ──────────────────────────────────────────────────────────

/// 去掉 "assets/" 前缀（如果存在），结果使用正斜杠
/// e.g. "assets/sub/Foo.bjson" → "sub/Foo.bjson"
inline std::string StripAssetsPrefix(std::string rp)
{
    NormSlashInPlace(rp);
    if (rp.size() > 7 && rp.substr(0, 7) == "assets/")
        rp = rp.substr(7);
    return rp;
}

/// 构建工程相对路径（带 assets/ 前缀），结果使用正斜杠
/// e.g. relNoExt="sub/Foo", ext=".bjson" → "assets/sub/Foo.bjson"
inline std::string BuildRelPath(const std::string& relNoExt, const std::string& ext)
{
    return "assets/" + NormSlash(relNoExt) + ext;
}

/// 取相对路径的目录部分（已去掉 assets/ 前缀），结果使用正斜杠
/// e.g. "assets/sub/dir/Foo.bjson" → "sub/dir"
inline std::string RelDir(const std::string& relPath)
{
    std::string rp = StripAssetsPrefix(relPath);
    auto slash = rp.rfind('/');
    return (slash != std::string::npos) ? rp.substr(0, slash) : "";
}

// ── 路径比较 ──────────────────────────────────────────────────────────────────

/// 规范化后比较两路径是否指向同一文件（跨平台安全）
inline bool SamePath(const std::string& a, const std::string& b)
{
    return fs::path(a).lexically_normal() == fs::path(b).lexically_normal();
}

/// 规范化绝对路径（lexically_normal + 正斜杠）
inline std::string NormAbs(const std::string& path)
{
    return ToStr(fs::path(path).lexically_normal());
}

} // namespace BpPath
