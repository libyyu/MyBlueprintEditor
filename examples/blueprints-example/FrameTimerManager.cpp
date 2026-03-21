// FrameTimerManager.cpp -- 主线程计时器管理器实现
#include "FrameTimerManager.h"
#include <algorithm>

// ============================================================================
// 核心 Tick
// ============================================================================

void FrameTimerManager::Tick(float deltaTime)
{
    if (m_globalPause || m_timers.empty())
        return;

    // 应用时间缩放
    float scaledDt = deltaTime * m_timeScale;
    if (scaledDt <= 0.0f)
        return;

    // 标记正在 tick，防止回调中增删计时器导致迭代器失效
    m_ticking = true;

    for (auto& timer : m_timers)
    {
        if (timer.pendingKill || timer.paused || timer.repeatCount == 0)
            continue;

        timer.totalElapsed += scaledDt;
        timer.remaining -= scaledDt;

        // 检查是否到期
        while (timer.remaining <= 0.0f && !timer.pendingKill && timer.repeatCount != 0)
        {
            timer.fireCount++;

            // 执行回调
            bool continueTimer = true;
            if (timer.callback)
                continueTimer = timer.callback();

            // 回调返回 false 则提前取消
            if (!continueTimer)
            {
                timer.pendingKill = true;
                break;
            }

            // 更新重复次数
            if (timer.repeatCount > 0)
            {
                timer.repeatCount--;
                if (timer.repeatCount == 0)
                {
                    timer.pendingKill = true;
                    break;
                }
            }

            // 循环计时器：重置剩余时间（累加方式，避免丢失时间精度）
            timer.remaining += timer.interval;

            // 安全保护：如果 interval 为 0 或极小，防止无限循环
            if (timer.interval < 0.001f)
            {
                timer.remaining = timer.interval;
                break;
            }
        }
    }

    m_ticking = false;

    // 清理已完成/已取消的计时器
    m_timers.erase(
        std::remove_if(m_timers.begin(), m_timers.end(),
            [](const FrameTimerEntry& e) { return e.pendingKill; }),
        m_timers.end());
}

// ============================================================================
// 创建计时器
// ============================================================================

TimerHandle FrameTimerManager::SetTimer(float delay, TimerCallback callback)
{
    return SetTimer(delay, 1, std::move(callback));
}

TimerHandle FrameTimerManager::SetTimer(float interval, int repeatCount, TimerCallback callback)
{
    return SetTimerByName("", interval, repeatCount, std::move(callback));
}

TimerHandle FrameTimerManager::SetTimerByName(const std::string& name, float interval, int repeatCount, TimerCallback callback)
{
    FrameTimerEntry entry;
    entry.handle      = AllocHandle();
    entry.name        = name;
    entry.interval    = interval;
    entry.remaining   = interval;
    entry.repeatCount = repeatCount;
    entry.paused      = false;
    entry.pendingKill = false;
    entry.callback    = std::move(callback);
    entry.fireCount   = 0;
    entry.totalElapsed = 0.0f;

    m_timers.push_back(std::move(entry));
    return m_timers.back().handle;
}

// ============================================================================
// 取消计时器
// ============================================================================

bool FrameTimerManager::ClearTimer(TimerHandle handle)
{
    auto* entry = FindEntry(handle);
    if (!entry) return false;
    entry->pendingKill = true;
    return true;
}

int FrameTimerManager::ClearTimerByName(const std::string& name)
{
    int count = 0;
    for (auto& t : m_timers)
    {
        if (!t.pendingKill && t.name == name)
        {
            t.pendingKill = true;
            ++count;
        }
    }
    return count;
}

void FrameTimerManager::ClearAllTimers()
{
    for (auto& t : m_timers)
        t.pendingKill = true;
}

// ============================================================================
// 暂停 / 恢复
// ============================================================================

bool FrameTimerManager::PauseTimer(TimerHandle handle)
{
    auto* entry = FindEntry(handle);
    if (!entry || entry->pendingKill) return false;
    entry->paused = true;
    return true;
}

bool FrameTimerManager::ResumeTimer(TimerHandle handle)
{
    auto* entry = FindEntry(handle);
    if (!entry || entry->pendingKill) return false;
    entry->paused = false;
    return true;
}

bool FrameTimerManager::PauseTimerByName(const std::string& name)
{
    bool found = false;
    for (auto& t : m_timers)
    {
        if (!t.pendingKill && t.name == name)
        {
            t.paused = true;
            found = true;
        }
    }
    return found;
}

bool FrameTimerManager::ResumeTimerByName(const std::string& name)
{
    bool found = false;
    for (auto& t : m_timers)
    {
        if (!t.pendingKill && t.name == name)
        {
            t.paused = false;
            found = true;
        }
    }
    return found;
}

void FrameTimerManager::PauseAll()
{
    m_globalPause = true;
}

void FrameTimerManager::ResumeAll()
{
    m_globalPause = false;
}

// ============================================================================
// 查询
// ============================================================================

bool FrameTimerManager::IsTimerActive(TimerHandle handle) const
{
    const auto* entry = FindEntry(handle);
    return entry && !entry->pendingKill && entry->repeatCount != 0;
}

bool FrameTimerManager::IsTimerPaused(TimerHandle handle) const
{
    const auto* entry = FindEntry(handle);
    return entry && entry->paused;
}

float FrameTimerManager::GetRemainingTime(TimerHandle handle) const
{
    const auto* entry = FindEntry(handle);
    if (!entry || entry->pendingKill) return 0.0f;
    return entry->remaining > 0.0f ? entry->remaining : 0.0f;
}

TimerHandle FrameTimerManager::FindTimerByName(const std::string& name) const
{
    for (const auto& t : m_timers)
    {
        if (!t.pendingKill && t.name == name)
            return t.handle;
    }
    return InvalidTimerHandle;
}

int FrameTimerManager::GetActiveTimerCount() const
{
    int count = 0;
    for (const auto& t : m_timers)
    {
        if (!t.pendingKill && t.repeatCount != 0)
            ++count;
    }
    return count;
}

// ============================================================================
// 内部辅助
// ============================================================================

TimerHandle FrameTimerManager::AllocHandle()
{
    return m_nextHandle++;
}

FrameTimerEntry* FrameTimerManager::FindEntry(TimerHandle handle)
{
    for (auto& t : m_timers)
        if (t.handle == handle)
            return &t;
    return nullptr;
}

const FrameTimerEntry* FrameTimerManager::FindEntry(TimerHandle handle) const
{
    for (const auto& t : m_timers)
        if (t.handle == handle)
            return &t;
    return nullptr;
}
