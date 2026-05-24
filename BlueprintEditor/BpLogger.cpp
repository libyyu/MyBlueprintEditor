// BlueprintEditor/BpLogger.cpp
#include "BpLogger.h"

#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <filesystem>
#include <iostream>
#ifdef _WIN32
#include <windows.h>
#endif
namespace fs = std::filesystem;

// ============================================================================
// 时间工具
// ============================================================================

std::string BpLogger::Timestamp()
{
    auto now   = std::chrono::system_clock::now();
    auto t     = std::chrono::system_clock::to_time_t(now);
    std::tm tm_buf{};
#if defined(_WIN32)
    localtime_s(&tm_buf, &t);
#else
    localtime_r(&t, &tm_buf);
#endif
    char buf[32];
    std::strftime(buf, sizeof(buf), "[%Y-%m-%d %H:%M:%S]", &tm_buf);
    return buf;
}

std::string BpLogger::TodayDate()
{
    auto now = std::chrono::system_clock::now();
    auto t   = std::chrono::system_clock::to_time_t(now);
    std::tm tm_buf{};
#if defined(_WIN32)
    localtime_s(&tm_buf, &t);
#else
    localtime_r(&t, &tm_buf);
#endif
    char buf[16];
    std::strftime(buf, sizeof(buf), "%Y-%m-%d", &tm_buf);
    return buf;
}

// ============================================================================
// 生命周期
// ============================================================================

void BpLogger::Start(const std::string& logsDir)
{
    if (m_running.load()) return;   // 防止重复启动

    m_logsDir = logsDir;

    // 确保日志目录存在
    try { fs::create_directories(logsDir); }
    catch (...) { /* 创建失败时退化为 stderr 输出 */ }

    m_logFilePath = logsDir + "/" + TodayDate() + ".log";

    m_stopFlag.store(false);
    m_running.store(true);
    m_thread = std::thread(&BpLogger::WorkerLoop, this);
}

void BpLogger::Stop()
{
    if (!m_running.load()) return;

    // 通知后台线程退出
    {
        std::lock_guard<std::mutex> lk(m_mutex);
        m_stopFlag.store(true);
    }
    m_cv.notify_all();

    if (m_thread.joinable())
        m_thread.join();

    m_running.store(false);
}

// ============================================================================
// 主线程写入（非阻塞）
// ============================================================================

void BpLogger::Log(BpLogLevel level, const std::string& message)
{
    {
        std::lock_guard<std::mutex> lk(m_mutex);
        m_queue.push({ level, message });
    }
    m_cv.notify_one();
}

// ============================================================================
// 后台工作线程
// ============================================================================

void BpLogger::WorkerLoop()
{
    std::ofstream ofs;
    std::string   curDate;

    auto ensureFile = [&]()
    {
        std::string today = TodayDate();
        if (today != curDate)
        {
            // 跨天：重新打开新文件
            if (ofs.is_open()) ofs.close();
            curDate = today;
            m_logFilePath = m_logsDir + "/" + curDate + ".log";
            ofs.open(m_logFilePath, std::ios::app);
            if (!ofs.is_open())
                std::cerr << "[BpLogger] Failed to open log file: " << m_logFilePath << "\n";
        }
        else if (!ofs.is_open())
        {
            ofs.open(m_logFilePath, std::ios::app);
        }
    };

    while (true)
    {
        std::queue<LogEntry> batch;
        {
            std::unique_lock<std::mutex> lk(m_mutex);
            m_cv.wait(lk, [this]{ return !m_queue.empty() || m_stopFlag.load(); });
            std::swap(batch, m_queue);  // 批量取出，减少锁持有时间
        }

        // 刷写批次
        ensureFile();
        while (!batch.empty())
        {
            const auto& entry = batch.front();
            std::string line = Timestamp() + " [" + LevelStr(entry.level) + "] " + entry.message + "\n";
            if (ofs.is_open())
                ofs << line;
            else
                std::cerr << line;  // fallback
            batch.pop();
#ifdef _WIN32
			::OutputDebugStringA(line.c_str());  // 同时输出到调试器（如 Visual Studio 输出窗口）
#endif
        }
        if (ofs.is_open()) ofs.flush();

        // 停止信号且队列已空则退出
        if (m_stopFlag.load())
        {
            std::lock_guard<std::mutex> lk(m_mutex);
            if (m_queue.empty()) break;
        }
    }

    if (ofs.is_open()) ofs.close();
}
