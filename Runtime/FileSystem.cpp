// Runtime/FileSystem.cpp - 全局文件系统单例实现
//
// 这些函数之前在 FileSystem.h 中以 inline 方式定义。
// 为了在动态库（DLL/SO）场景下保证全局只有一份 IFileSystem 实例，
// 将实现移到此编译单元并标记为 BLUEPRINT_API 导出。

#include "FileSystem.h"

namespace NodeEditor {
namespace Runtime {

std::shared_ptr<IFileSystem>& GetDefaultFileSystemRef()
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

std::shared_ptr<IFileSystem> GetDefaultFileSystem()
{
    return GetDefaultFileSystemRef();
}

void SetDefaultFileSystem(std::shared_ptr<IFileSystem> fs)
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
