// tests/test_helpers.h — 测试公共辅助工具
//
// RunWithTick: Execute() 后驱动实时 Tick 循环直到所有异步节点完成。
// 所有需要异步节点（Delay / ExecuteBlueprint / Timer 等）的测试都应使用此函数。
#pragma once

#include <chrono>
#include <thread>
#include "BlueprintRunner.h"

namespace TestHelpers {

using namespace NodeEditor::Runtime;
using Clock = std::chrono::steady_clock;

// 最大 Tick 次数（防止死循环）：5000 * 16ms ≈ 80 秒
static constexpr int kMaxTicks = 5000;
// 每 tick 睡眠时长
static constexpr int kTickMs   = 16;

/// Execute() 并驱动实时时钟 Tick 循环，直到 HasPendingAsync()==false 或超时。
/// 返回最终 ExecutionResult。
inline ExecutionResult RunWithTick(BlueprintRunner& runner)
{
    auto result = runner.Execute();

    if (runner.HasPendingAsync())
    {
        auto prev = Clock::now();
        int ticks = 0;
        while (runner.HasPendingAsync() && ticks < kMaxTicks)
        {
            auto now = Clock::now();
            float dt = std::chrono::duration<float>(now - prev).count();
            prev = now;
            runner.Tick(dt);
            std::this_thread::sleep_for(std::chrono::milliseconds(kTickMs));
            ++ticks;
        }
    }

    return result;
}

} // namespace TestHelpers
