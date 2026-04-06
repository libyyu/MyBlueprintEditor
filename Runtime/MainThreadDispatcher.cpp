// MainThreadDispatcher.cpp
#include "MainThreadDispatcher.h"
#include <cstdio>

namespace NodeEditor {
namespace Runtime {

MainThreadDispatcher& MainThreadDispatcher::Get()
{
    static MainThreadDispatcher instance;
    return instance;
}

#ifdef __EMSCRIPTEN__

// Emscripten：单线程，Post 直接同步执行，DrainQueue 是空操作
void MainThreadDispatcher::Post(std::function<void()> task)
{
    if (task) task();
}

void MainThreadDispatcher::DrainQueue()
{
    // no-op
}

#else

// 非 Emscripten：真正的线程安全实现
void MainThreadDispatcher::Post(std::function<void()> task)
{
    if (!task) return;
    std::lock_guard<std::mutex> lock(m_mutex);
    m_queue.push_back(std::move(task));
    fprintf(stderr, "[MainThreadDispatcher] Post: queue size=%zu\n", m_queue.size());
}

void MainThreadDispatcher::DrainQueue()
{
    // 双缓冲：把队列 swap 出来再执行，避免执行过程中 Post 新任务导致死锁
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_draining.swap(m_queue);
    }

    if (!m_draining.empty())
        fprintf(stderr, "[MainThreadDispatcher] DrainQueue: executing %zu tasks\n", m_draining.size());

    for (auto& task : m_draining)
    {
        if (task) task();
    }
    m_draining.clear();
}

#endif // __EMSCRIPTEN__

} // namespace Runtime
} // namespace NodeEditor
