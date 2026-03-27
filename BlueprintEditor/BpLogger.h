// BlueprintEditor/BpLogger.h
// 异步日志单例：主线程只 push，后台线程负责写文件，完全不阻塞 OnFrame
#pragma once
#include <string>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <fstream>
#include <atomic>
#include <cstdio>

enum class BpLogLevel { Info, Warn, Error };

class BpLogger
{
public:
    static BpLogger& Get()
    {
        static BpLogger s_instance;
        return s_instance;
    }

    // ── 生命周期 ────────────────────────────────────────────────────────────
    /// 启动后台写线程，日志写入 logsDir/YYYY-MM-DD.log
    void Start(const std::string& logsDir = "logs");
    /// 等待队列清空后停止后台线程（OnStop 时调用）
    void Stop();

    // ── 主线程写入接口（非阻塞）────────────────────────────────────────────
    void Log(BpLogLevel level, const std::string& message);

    void Info (const std::string& msg) { Log(BpLogLevel::Info,  msg); }
    void Warn (const std::string& msg) { Log(BpLogLevel::Warn,  msg); }
    void Error(const std::string& msg) { Log(BpLogLevel::Error, msg); }

    // ── 状态查询 ────────────────────────────────────────────────────────────
    bool IsRunning() const { return m_running.load(); }
    const std::string& GetLogFilePath() const { return m_logFilePath; }

private:
    BpLogger() = default;
    ~BpLogger() { Stop(); }

    BpLogger(const BpLogger&) = delete;
    BpLogger& operator=(const BpLogger&) = delete;

    // 后台线程主循环
    void WorkerLoop();

    // 获取当前时间戳字符串 [YYYY-MM-DD HH:MM:SS]
    static std::string Timestamp();
    // 获取当天日期字符串 YYYY-MM-DD
    static std::string TodayDate();
    // 将 BpLogLevel 转为短字符串
    static const char* LevelStr(BpLogLevel lv)
    {
        switch (lv)
        {
            case BpLogLevel::Info:  return "INFO ";
            case BpLogLevel::Warn:  return "WARN ";
            case BpLogLevel::Error: return "ERROR";
            default:                return "?????";
        }
    }

    struct LogEntry { BpLogLevel level; std::string message; };

    std::queue<LogEntry>    m_queue;
    std::mutex              m_mutex;
    std::condition_variable m_cv;
    std::thread             m_thread;
    std::atomic<bool>       m_running  { false };
    std::atomic<bool>       m_stopFlag { false };
    std::string             m_logsDir;
    std::string             m_logFilePath;
};

// ── 便捷宏 ──────────────────────────────────────────────────────────────────
#define BPLOG(msg)   BpLogger::Get().Info(msg)
#define BPWARN(msg)  BpLogger::Get().Warn(msg)
#define BPERROR(msg) BpLogger::Get().Error(msg)
