// CrashHandler.h -- Windows SEH 崩溃处理，生成 .dmp 文件便于排查
// 只在 BlueprintEditor 中使用，Runtime 不包含此文件
#pragma once

#ifdef _WIN32
#include <windows.h>
#include <dbghelp.h>
#include <ctime>
#include <string>
#include <filesystem>

#pragma comment(lib, "dbghelp.lib")

namespace CrashHandler {

// 生成 minidump 文件
// dumpDir: dump 文件输出目录（默认程序目录下的 crashes/ 子目录）
inline bool WriteMiniDump(EXCEPTION_POINTERS* ep, const std::wstring& dumpPath)
{
    HANDLE hFile = CreateFileW(dumpPath.c_str(), GENERIC_WRITE, 0, nullptr,
                               CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (hFile == INVALID_HANDLE_VALUE)
        return false;

    MINIDUMP_EXCEPTION_INFORMATION mei{};
    mei.ThreadId          = GetCurrentThreadId();
    mei.ExceptionPointers = ep;
    mei.ClientPointers    = FALSE;

    MINIDUMP_TYPE dumpType = (MINIDUMP_TYPE)(
        MiniDumpWithDataSegs         |
        MiniDumpWithIndirectlyReferencedMemory |
        MiniDumpWithUnloadedModules  |
        MiniDumpWithProcessThreadData);

    BOOL ok = MiniDumpWriteDump(
        GetCurrentProcess(), GetCurrentProcessId(),
        hFile, dumpType, ep ? &mei : nullptr, nullptr, nullptr);

    CloseHandle(hFile);
    return ok == TRUE;
}

// SEH 过滤函数（在 __except 表达式中调用）
inline LONG WINAPI UnhandledExceptionFilter(EXCEPTION_POINTERS* ep)
{
    // 计算 dump 路径：exe 目录 / crashes / BP_YYYYMMDD_HHMMSS.dmp
    wchar_t exePath[MAX_PATH] = {};
    GetModuleFileNameW(nullptr, exePath, MAX_PATH);

    std::wstring dir = std::filesystem::path(exePath).parent_path().wstring()
                       + L"\\crashes";
    std::filesystem::create_directories(dir);

    // 时间戳
    time_t t = time(nullptr);
    struct tm tm_local{};
    localtime_s(&tm_local, &t);
    wchar_t ts[32];
    swprintf_s(ts, L"%04d%02d%02d_%02d%02d%02d",
               tm_local.tm_year + 1900, tm_local.tm_mon + 1, tm_local.tm_mday,
               tm_local.tm_hour, tm_local.tm_min, tm_local.tm_sec);

    std::wstring dumpPath = dir + L"\\BlueprintEditor_" + ts + L".dmp";

    WriteMiniDump(ep, dumpPath);

    // 弹窗通知用户（MessageBox 在崩溃时仍然可用）
    std::wstring msg = L"Blueprint Editor has crashed!\r\n\r\nA crash dump has been saved to:\r\n"
                       + dumpPath
                       + L"\r\n\r\nPlease send this file for debugging.";
    MessageBoxW(nullptr, msg.c_str(), L"Crash Report", MB_OK | MB_ICONERROR | MB_SYSTEMMODAL);

    return EXCEPTION_EXECUTE_HANDLER;
}

// 安装全局崩溃处理器
inline void Install()
{
    SetUnhandledExceptionFilter(UnhandledExceptionFilter);
}

} // namespace CrashHandler

#else
// 非 Windows 平台：空实现
namespace CrashHandler {
    inline void Install() {}
}
#endif // _WIN32
