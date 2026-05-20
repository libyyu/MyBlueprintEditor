// CrashHandler.cpp
// Windows-only: 在 DLL 加载时注册崩溃处理器，崩溃时自动写 MiniDump
//
// 生成路径（优先级从高到低）：
//   1. BP_SetCrashDumpDir() 显式指定的目录
//   2. 环境变量 BP_CRASH_DUMP_DIR
//   3. 可执行文件所在目录（Unity Editor 下就是 Unity.exe 同级）
//
// 文件名格式：BlueprintRuntime_YYYYMMDD_HHmmss_<pid>.dmp
//
// 也可在 C# 侧手动触发：
//   [DllImport("BlueprintRuntime")] static extern void BP_SetCrashDumpDir(string dir);
//   [DllImport("BlueprintRuntime")] static extern void BP_WriteCrashDump(string reason);

#include "BlueprintExport.h"
#include "BlueprintCAPI.h"
#if defined(_WIN32) || defined(_WIN64)

#ifndef WIN32_LEAN_AND_MEAN
#  define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#  define NOMINMAX
#endif
#include <windows.h>
#include <dbghelp.h>      // MiniDumpWriteDump
#include <shlobj.h>       // SHGetFolderPath (fallback)
#include <string>
#include <mutex>
#include <ctime>
#include <cstdio>

#pragma comment(lib, "dbghelp.lib")

// ─────────────────────────────────────────────────────────────────────────────
// 全局状态
// ─────────────────────────────────────────────────────────────────────────────
namespace {
    std::mutex              g_mutex;
    std::wstring            g_dumpDir;          // 自定义 dump 目录（宽字符）
    bool                    g_registered = false;
    LPTOP_LEVEL_EXCEPTION_FILTER g_prevFilter = nullptr;

    // ── 工具函数 ──────────────────────────────────────────────────────────────

    // 获取当前进程可执行文件所在目录
    std::wstring GetExeDir()
    {
        wchar_t buf[MAX_PATH] = {};
        GetModuleFileNameW(nullptr, buf, MAX_PATH);
        std::wstring path(buf);
        auto pos = path.rfind(L'\\');
        if (pos != std::wstring::npos)
            path = path.substr(0, pos);
        return path;
    }

    // 生成带时间戳的 dump 文件路径
    std::wstring BuildDumpPath(const std::wstring& dir)
    {
        std::time_t t = std::time(nullptr);
        struct tm tm_info;
        localtime_s(&tm_info, &t);
        wchar_t ts[32] = { 0x0 };
        swprintf(ts, 32, L"%04d%02d%02d_%02d%02d%02d",
            tm_info.tm_year + 1900, tm_info.tm_mon + 1, tm_info.tm_mday,
            tm_info.tm_hour, tm_info.tm_min, tm_info.tm_sec);
        DWORD pid = GetCurrentProcessId();
        return dir + L"\\BlueprintRuntime_" + ts + L"_" + std::to_wstring(pid) + L".dmp";
    }

    // 选定最终 dump 目录
    std::wstring ResolveDumpDir()
    {
        // 1. 显式设置
        if (!g_dumpDir.empty()) return g_dumpDir;

        // 2. 环境变量
        wchar_t env[MAX_PATH] = {};
        if (GetEnvironmentVariableW(L"BP_CRASH_DUMP_DIR", env, MAX_PATH) > 0)
            return std::wstring(env);

        // 3. exe 同级目录
        return GetExeDir();
    }

    // 核心：写 MiniDump
    bool WriteDump(EXCEPTION_POINTERS* exInfo, const std::wstring& path)
    {
        HANDLE hFile = CreateFileW(
            path.c_str(),
            GENERIC_WRITE, 0, nullptr,
            CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
        if (hFile == INVALID_HANDLE_VALUE) return false;

        MINIDUMP_TYPE dumpType = (MINIDUMP_TYPE)(
            MiniDumpWithDataSegs          |   // 全局/静态变量
            MiniDumpWithPrivateReadWriteMemory |  // 堆
            MiniDumpWithHandleData        |
            MiniDumpWithFullMemoryInfo    |
            MiniDumpWithThreadInfo        |
            MiniDumpWithUnloadedModules);

        MINIDUMP_EXCEPTION_INFORMATION mei = {};
        mei.ThreadId          = GetCurrentThreadId();
        mei.ExceptionPointers = exInfo;
        mei.ClientPointers    = FALSE;

        BOOL ok = MiniDumpWriteDump(
            GetCurrentProcess(), GetCurrentProcessId(),
            hFile, dumpType,
            exInfo ? &mei : nullptr,
            nullptr, nullptr);

        CloseHandle(hFile);
        return ok == TRUE;
    }

    // 向 stderr + OutputDebugString 双输出
    void Log(const wchar_t* msg)
    {
        OutputDebugStringW(msg);
        OutputDebugStringW(L"\n");
        fwprintf(stderr, L"%s\n", msg);
        fflush(stderr);
    }

    // ── 异常过滤器 ───────────────────────────────────────────────────────────

    LONG WINAPI BPCrashFilter(EXCEPTION_POINTERS* exInfo)
    {
        // 防止递归
        static LONG s_entered = 0;
        if (InterlockedCompareExchange(&s_entered, 1, 0) != 0)
            return EXCEPTION_CONTINUE_SEARCH;

        std::wstring dir  = ResolveDumpDir();
        std::wstring path = BuildDumpPath(dir);

        // 确保目录存在
        CreateDirectoryW(dir.c_str(), nullptr);

        bool ok = WriteDump(exInfo, path);

        wchar_t msg[1024];
        if (ok)
            swprintf_s(msg, L"[BlueprintRuntime] Crash dump written: %s", path.c_str());
        else
            swprintf_s(msg, L"[BlueprintRuntime] Failed to write crash dump to: %s", path.c_str());
        Log(msg);

        // 转给之前的 filter（Unity 自己的崩溃处理器）
        if (g_prevFilter)
            return g_prevFilter(exInfo);

        return EXCEPTION_CONTINUE_SEARCH;
    }

} // anonymous namespace

// ─────────────────────────────────────────────────────────────────────────────
// DllMain：DLL 加载时自动注册崩溃处理器
// ─────────────────────────────────────────────────────────────────────────────
BOOL WINAPI DllMain(HINSTANCE /*hInst*/, DWORD reason, LPVOID /*reserved*/)
{
    if (reason == DLL_PROCESS_ATTACH)
    {
        std::lock_guard<std::mutex> lk(g_mutex);
        if (!g_registered)
        {
            g_prevFilter  = SetUnhandledExceptionFilter(BPCrashFilter);
            g_registered  = true;
            OutputDebugStringW(L"[BlueprintRuntime] Crash handler registered\n");
        }
    }
    else if (reason == DLL_PROCESS_DETACH)
    {
        std::lock_guard<std::mutex> lk(g_mutex);
        if (g_registered)
        {
            SetUnhandledExceptionFilter(g_prevFilter);
            g_registered = false;
        }
    }
    return TRUE;
}

// ─────────────────────────────────────────────────────────────────────────────
// 公开 C API（C# 侧可选调用）
// ─────────────────────────────────────────────────────────────────────────────
extern "C" {

/// <summary>
/// 设置 dump 文件输出目录（UTF-8 路径）。
/// 建议在 C# Awake() 最早调用，指向 Application.persistentDataPath。
/// </summary>
BLUEPRINT_API void BLUEPRINT_CAPI_CALL BP_SetCrashDumpDir(const char* dir)
{
    if (!dir || !*dir) return;
    int len = MultiByteToWideChar(CP_UTF8, 0, dir, -1, nullptr, 0);
    std::wstring wdir(len, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, dir, -1, wdir.data(), len);
    if (!wdir.empty() && wdir.back() == L'\0')
        wdir.pop_back();

    std::lock_guard<std::mutex> lk(g_mutex);
    g_dumpDir = wdir;
    CreateDirectoryW(wdir.c_str(), nullptr);

    wchar_t msg[512];
    swprintf_s(msg, L"[BlueprintRuntime] Crash dump dir set: %s", wdir.c_str());
    OutputDebugStringW(msg); OutputDebugStringW(L"\n");
}

/// <summary>
/// 手动触发写一个 dump（不需要真正崩溃，用于诊断卡顿/异常状态）。
/// reason：写入日志的说明字符串，可为 nullptr。
/// </summary>
BLUEPRINT_API void BLUEPRINT_CAPI_CALL BP_WriteCrashDump(const char* reason)
{
    std::wstring dir  = ResolveDumpDir();
    std::wstring path = BuildDumpPath(dir);
    CreateDirectoryW(dir.c_str(), nullptr);

    bool ok = WriteDump(nullptr, path);   // nullptr = 无异常信息，只写内存快照

    wchar_t msg[1024];
    const char* r = reason ? reason : "(manual)";
    if (ok)
        swprintf_s(msg, L"[BlueprintRuntime] Manual dump written (%hs): %s", r, path.c_str());
    else
        swprintf_s(msg, L"[BlueprintRuntime] Manual dump FAILED (%hs): %s", r, path.c_str());
    Log(msg);
}

} // extern "C"

#else // 非 Windows：空实现，保持链接兼容

extern "C" {
BLUEPRINT_API void BLUEPRINT_CAPI_CALL BP_SetCrashDumpDir(const char* /*dir*/)  {}
BLUEPRINT_API void BLUEPRINT_CAPI_CALL BP_WriteCrashDump(const char* /*reason*/) {}
}

#endif // _WIN32
