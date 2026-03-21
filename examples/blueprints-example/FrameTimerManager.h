// FrameTimerManager.h -- 主线程计时器管理器
//
// 基于 OnFrame(deltaTime) 驱动的轻量计时器系统。
// 所有回调在主线程（渲染线程）中执行，无多线程竞争问题。
//
// 用法：
//   1. 在编辑器类中持有 FrameTimerManager 实例
//   2. 每帧调用 Tick(deltaTime)
//   3. 通过 SetTimer / SetTimerByName 注册计时器
//   4. 通过 ClearTimer / ClearTimerByName 取消计时器
//
#pragma once

#include <string>
#include <vector>
#include <functional>
#include <cstdint>
#include <unordered_map>

// ============================================================================
// 计时器句柄（用于引用特定计时器）
// ============================================================================

using TimerHandle = uint32_t;
static constexpr TimerHandle InvalidTimerHandle = 0;

// ============================================================================
// 计时器回调签名
// ============================================================================

// 回调返回 true 表示继续（对循环计时器有效），返回 false 会提前取消
using TimerCallback = std::function<bool()>;

// ============================================================================
// 计时器条目
// ============================================================================

struct FrameTimerEntry
{
    TimerHandle     handle      = InvalidTimerHandle;
    std::string     name;           // 可选名称（便于按名查找/取消）
    float           interval    = 0.0f;  // 间隔（秒）
    float           remaining   = 0.0f;  // 剩余时间（秒）
    int             repeatCount = 1;     // 重复次数：-1=无限循环, 0=已完成, N=剩余次数
    bool            paused      = false; // 暂停状态
    bool            pendingKill = false; // 标记待删除
    TimerCallback   callback;

    // 调试/统计
    int             fireCount   = 0;     // 已触发次数
    float           totalElapsed = 0.0f; // 累计已过时间
};

// ============================================================================
// 主线程计时器管理器
// ============================================================================

class FrameTimerManager
{
public:
    FrameTimerManager() = default;
    ~FrameTimerManager() = default;

    // ------------------------------------------------------------------
    // 核心：每帧调用（在 OnFrame 中调用）
    // ------------------------------------------------------------------
    void Tick(float deltaTime);

    // ------------------------------------------------------------------
    // 创建计时器
    // ------------------------------------------------------------------

    // 基本版：延迟 delay 秒后执行一次
    TimerHandle SetTimer(float delay, TimerCallback callback);

    // 完整版：指定间隔、重复次数（-1=无限循环）
    TimerHandle SetTimer(float interval, int repeatCount, TimerCallback callback);

    // 带名称版：可通过名称查找/取消
    TimerHandle SetTimerByName(const std::string& name, float interval, int repeatCount, TimerCallback callback);

    // ------------------------------------------------------------------
    // 取消计时器
    // ------------------------------------------------------------------

    // 按句柄取消
    bool ClearTimer(TimerHandle handle);

    // 按名称取消（取消所有同名计时器）
    int  ClearTimerByName(const std::string& name);

    // 取消所有计时器
    void ClearAllTimers();

    // ------------------------------------------------------------------
    // 暂停 / 恢复
    // ------------------------------------------------------------------

    bool PauseTimer(TimerHandle handle);
    bool ResumeTimer(TimerHandle handle);
    bool PauseTimerByName(const std::string& name);
    bool ResumeTimerByName(const std::string& name);

    // 全局暂停/恢复
    void PauseAll();
    void ResumeAll();

    // ------------------------------------------------------------------
    // 查询
    // ------------------------------------------------------------------

    // 是否存在有效的计时器
    bool IsTimerActive(TimerHandle handle) const;
    bool IsTimerPaused(TimerHandle handle) const;

    // 获取剩余时间
    float GetRemainingTime(TimerHandle handle) const;

    // 按名称查找句柄（返回第一个匹配的）
    TimerHandle FindTimerByName(const std::string& name) const;

    // 活跃计时器数量
    int GetActiveTimerCount() const;

    // 获取所有计时器（供 UI 显示）
    const std::vector<FrameTimerEntry>& GetAllTimers() const { return m_timers; }

    // ------------------------------------------------------------------
    // 时间缩放（慢动作/快进）
    // ------------------------------------------------------------------

    void  SetTimeScale(float scale) { m_timeScale = scale; }
    float GetTimeScale() const { return m_timeScale; }

private:
    TimerHandle AllocHandle();
    FrameTimerEntry* FindEntry(TimerHandle handle);
    const FrameTimerEntry* FindEntry(TimerHandle handle) const;

    std::vector<FrameTimerEntry>    m_timers;
    uint32_t                        m_nextHandle = 1;
    float                           m_timeScale  = 1.0f;
    bool                            m_globalPause = false;
    bool                            m_ticking     = false; // 防止 Tick 中嵌套修改
};
