// MainThreadDispatcher.h
//
// 将后台线程的任务安全地派发回主线程（渲染/Tick线程）执行。
//
// 用法（后台线程）：
//   MainThreadDispatcher::Get().Post([...]() { /* 主线程逻辑 */ });
//
// 主线程每帧调用（在 BlueprintRunner::Tick 里）：
//   MainThreadDispatcher::Get().DrainQueue();
//
// Emscripten 下：Post() 直接同步执行，DrainQueue() 为空操作。
// 非 Emscripten 下：真正的线程安全队列 + mutex 双缓冲。
//
#pragma once
#include "BlueprintExport.h"

// MSVC C4251: STL members in DLL-exported class
#ifdef _MSC_VER
#   pragma warning(push)
#   pragma warning(disable: 4251)
#endif

#include <functional>
#include <vector>

#ifndef __EMSCRIPTEN__
#  include <mutex>
#  include <atomic>
#endif

namespace NodeEditor {
namespace Runtime {

class BLUEPRINT_API MainThreadDispatcher
{
public:
    static MainThreadDispatcher& Get();

    // 后台线程调用：将任务投递到主线程队列
    // Emscripten 下直接同步执行
    void Post(std::function<void()> task);

    // 主线程每帧调用：消费队列中所有待执行任务
    // Emscripten 下为空操作
    void DrainQueue();

private:
    MainThreadDispatcher() = default;
    ~MainThreadDispatcher() = default;
    MainThreadDispatcher(const MainThreadDispatcher&) = delete;
    MainThreadDispatcher& operator=(const MainThreadDispatcher&) = delete;

#ifndef __EMSCRIPTEN__
    std::mutex                         m_mutex;
    std::vector<std::function<void()>> m_queue;    // 后台线程写入
    std::vector<std::function<void()>> m_draining; // 主线程消费（双缓冲）
#endif
};

} // namespace Runtime
} // namespace NodeEditor

#ifdef _MSC_VER
#   pragma warning(pop)
#endif
