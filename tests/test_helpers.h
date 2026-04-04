// tests/test_helpers.h — 测试公共辅助工具
//
// RunWithTick:        Execute() 后驱动实时 Tick 循环直到所有异步节点完成。
// RunWithBeginPlay:   Execute() + DispatchEvent("OnBeginPlay") + Tick 循环。
// AddBeginPlayEntry:  给 BlueprintData 添加 OnBeginPlay 事件源节点，并连接到指定 execIn 引脚。
#pragma once

#include <chrono>
#include <thread>
#include "BlueprintRunner.h"
#include "BlueprintData.h"

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

/// Execute() + DispatchEvent("OnBeginPlay") + Tick 循环。
/// 全严格模式下，所有 exec 链必须从 OnBeginPlay 触发时使用此函数。
inline ExecutionResult RunWithBeginPlay(BlueprintRunner& runner)
{
    // 先执行数据流（Execute 只处理可达的数据节点，严格模式下实际为空）
    auto result = runner.Execute();

    // 触发 BeginPlay 链
    auto bpResult = runner.DispatchEvent("OnBeginPlay");
    // 合并执行结果
    for (auto nid : bpResult.executedNodeIds)
        result.executedNodeIds.push_back(nid);
    result.nodesExecuted += bpResult.nodesExecuted;
    if (!bpResult.success && result.success)
    {
        result.success = bpResult.success;
        result.errorMessage = bpResult.errorMessage;
    }

    // 驱动异步 Tick
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

/// 给 BlueprintData 添加 OnBeginPlay 事件源节点，并用 exec 连线连到目标 execIn 引脚。
/// nodeId:      OnBeginPlay 节点的 ID（需唯一，建议用大数避免冲突）
/// execOutPinId: OnBeginPlay 的 exec 输出引脚 ID
/// linkId:       连线 ID
/// targetExecInPinId: 目标节点的 exec 输入引脚 ID
inline void AddBeginPlayEntry(BlueprintData& bp,
    NodeId nodeId, PinId execOutPinId, LinkId linkId,
    PinId targetExecInPinId)
{
    NodeInstance n;
    n.id = nodeId;
    n.definitionId = "OnBeginPlay";
    n.name = "On Begin Play";

    PinInfo execOut;
    execOut.id   = execOutPinId;
    execOut.kind = PinKind::Output;
    execOut.isExec = true;
    n.pins.push_back(execOut);

    bp.nodes.push_back(n);

    LinkInstance lnk;
    lnk.id        = linkId;
    lnk.startPinId = execOutPinId;
    lnk.endPinId   = targetExecInPinId;
    bp.links.push_back(lnk);
}

} // namespace TestHelpers
