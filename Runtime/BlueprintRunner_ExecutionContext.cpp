// Runtime/BlueprintRunner_ExecutionContext.cpp
// All ExecutionContext:: method implementations.
//
// Extracted from BlueprintRunner.cpp to keep that file focused on the core
// BlueprintRunner:: methods (loading, execution, topology, control flow).
//
// ExecutionContext is BlueprintRunner's friend, so accessing private members
// (m_blueprint, m_state, executeNodeInternal, propagatePinValues, ...) from
// this TU works without further qualification.

#include "BlueprintRunner.h"
#include "MainThreadDispatcher.h"   // MainThreadDispatcher::Get() used inside RunAsync
#include <cstdio>   // fprintf fallback inside Log/Print

namespace NodeEditor {
namespace Runtime {

// ============================================================================
// FireConnectedNode + PauseRunner (was at line ~1590 in BlueprintRunner.cpp)
// ============================================================================
bool ExecutionContext::FireConnectedNode(PinId inputPinId)
{
    if (m_runner)
        return m_runner->FireConnectedNode(inputPinId);
    return false;
}

void ExecutionContext::PauseRunner()
{
    if (m_runner)
        m_runner->Pause();
}

// ============================================================================
// Timer context helpers + Log/Print/Delay + RunAsync + SetTimer + ActivateOutputFlow
// (was at lines 1754..2071 in BlueprintRunner.cpp)
// ============================================================================
// ============================================================================
// Timer context 辅助：保存/恢复 context 状态的 RAII guard + 统一包装
// ============================================================================

// 内部辅助：将用户回调包装为带 context save/restore 的 TimerCallback
// timer 回调在未来帧触发时 m_state->currentNode / pinNameToId / nodeData
// 已被其他节点覆盖，需要先恢复再调用用户回调
//
// 优化（P3）：不再深拷贝 savedPinNameToId unordered_map，
// 只保存 savedNode 指针，回调触发时从节点 pins 重建，避免高频 Timer 的 map 深拷贝开销。
TimerCallback ExecutionContext::wrapCallbackWithContextRestore(TimerCallback callback)
{
    auto savedNode     = m_state->currentNode;
    auto savedNodeData = m_state->nodeData;

    if (m_runner)
        m_runner->AcquireAsync();

    BlueprintRunner* runner = m_runner;
    // 捕获 alive 标志的 shared_ptr 副本，用于在回调时检查 runner 是否仍存活
    auto alive = runner ? runner->GetAliveFlag() : nullptr;
    NodeExecutionState* state = m_state;

    return [state, runner, alive, callback = std::move(callback),
            savedNode, savedNodeData]() mutable -> bool
    {
        // 检查 runner 是否已析构
        if (alive && !alive->load(std::memory_order_acquire))
        {
            return false; // runner 已销毁，放弃执行
        }

        // 保存当前状态
        auto prevNode     = state->currentNode;
        auto prevNodeData = state->nodeData;

        // 恢复注册时的状态
        state->currentNode = savedNode;
        state->nodeData    = savedNodeData;
        // 从节点 pins 重建 pinNameToId（避免深拷贝 map，直接按需重建）
        state->pinNameToId.clear();
        state->inputPinNameToId.clear();
        state->outputPinNameToId.clear();
        if (savedNode)
        {
            for (const auto& pin : savedNode->pins)
            {
                state->pinNameToId[pin.name] = pin.id;
                if (pin.kind == PinKind::Input)
                    state->inputPinNameToId[pin.name] = pin.id;
                else
                    state->outputPinNameToId[pin.name] = pin.id;
            }
        }

        const bool ret = callback();

        // 恢复调用前的状态
        state->currentNode = prevNode;
        state->nodeData    = prevNodeData;
        // 重建前一个节点的 pinNameToId
        state->pinNameToId.clear();
        state->inputPinNameToId.clear();
        state->outputPinNameToId.clear();
        if (prevNode)
        {
            for (const auto& pin : prevNode->pins)
            {
                state->pinNameToId[pin.name] = pin.id;
                if (pin.kind == PinKind::Input)
                    state->inputPinNameToId[pin.name] = pin.id;
                else
                    state->outputPinNameToId[pin.name] = pin.id;
            }
        }

        if (!ret && runner)
            runner->ReleaseAsync();

        return ret;
    };
}

void ExecutionContext::Log(const std::string& message, LogLevel level/* = LogLevel::Verbose*/) const
{
    if (loggingEnabled)
    {
        if (OnLog)
        {
            OnLog(level, message);
        }
        else
        {
            // Last-resort fallback so output is never silently discarded.
            fprintf(stderr, "[Blueprint]%s%s\n", LogLevelPrefix(level), message.c_str());
        }
    }
}

void ExecutionContext::Print(const std::string& message, LogLevel level/* = LogLevel::Info*/) const
{
    if (OnPrint)
    {
        OnPrint(level, message);
    }
    else if (m_runner && m_runner->IsWithEditor() && OnLog)
    {// 编辑器环境下 Print 直接走 Log 回调，确保输出可见且带颜色
        OnLog(level, message);
    }
    else
    {
        // Last-resort fallback so output is never silently discarded.
        fprintf(stderr, "%s%s\n", LogLevelPrefix(level), message.c_str());
    }
}

TimerHandle ExecutionContext::Delay(float seconds, const std::function<void()>& callback)
{
    if (!m_runner) return InvalidTimerHandle;

    // 将 void callback 包装为 TimerCallback（返回 false 表示不重复）
    return m_runner->GetTimerManager().SetTimer(seconds,
        wrapCallbackWithContextRestore([callback]() -> bool { callback(); return false; }));
}

// ============================================================================
// RunAsync — 跨平台异步执行框架
// ============================================================================

void ExecutionContext::RunAsync(
    AsyncDispatcher                          dispatcher,
    std::function<void(ExecutionContext&)>   onComplete)
{
    if (!m_runner) return;

    // 优化（P3）：只保存节点指针，回调时从 pins 重建 pinNameToId，避免深拷贝 map
    auto savedNode     = m_state->currentNode;
    auto savedNodeData = m_state->nodeData;

    m_runner->AcquireAsync();
    BlueprintRunner*    runner = m_runner;
    auto                alive  = runner->GetAliveFlag();
    NodeExecutionState* state  = m_state;
    ExecutionContext*   ctx    = this;

    // 辅助：从节点 pins 重建三个 pinName→ID map
    auto rebuildPinMaps = [](NodeExecutionState* st, const NodeInstance* node)
    {
        st->pinNameToId.clear();
        st->inputPinNameToId.clear();
        st->outputPinNameToId.clear();
        if (!node) return;
        for (const auto& pin : node->pins)
        {
            st->pinNameToId[pin.name] = pin.id;
            if (pin.kind == PinKind::Input)
                st->inputPinNameToId[pin.name] = pin.id;
            else
                st->outputPinNameToId[pin.name] = pin.id;
        }
    };

    // resolve：无论哪个平台，都 Post 到 MainThreadDispatcher，
    // 由 Tick() → DrainQueue() 在安全上下文中执行 onComplete。
    auto resolve = [ctx, state, runner, alive, onComplete, savedNode, savedNodeData,
                    rebuildPinMaps]() mutable
    {
        MainThreadDispatcher::Get().Post(
            [ctx, state, runner, alive, onComplete = std::move(onComplete),
             savedNode, savedNodeData, rebuildPinMaps]() mutable
            {
                if (!alive->load(std::memory_order_acquire))
                {
                    runner->ReleaseAsync();
                    return;
                }

                auto prevNode     = state->currentNode;
                auto prevNodeData = state->nodeData;

                state->currentNode = savedNode;
                state->nodeData    = savedNodeData;
                rebuildPinMaps(state, savedNode);

                if (onComplete) onComplete(*ctx);

                state->currentNode = prevNode;
                state->nodeData    = prevNodeData;
                rebuildPinMaps(state, prevNode);

                runner->ReleaseAsync();
            });
    };

#ifdef __EMSCRIPTEN__
    // WebGL：dispatcher 在当前（主）线程调用。
    // 调用方负责在 dispatcher 内用 emscripten_fetch 等原生异步 API，
    // 完成时调 resolve()，resolve 再 Post onComplete 到 Tick。
    // 不在这里同步执行 onComplete，保证与桌面行为一致。
    if (dispatcher) dispatcher(std::move(resolve));
#else
    // 桌面/移动：dispatcher 在后台线程中运行，完成后调 resolve()。
    std::thread([dispatcher = std::move(dispatcher),
                 resolve    = std::move(resolve)]() mutable
    {
        if (dispatcher) dispatcher(std::move(resolve));
    }).detach();
#endif
}

// 兼容旧签名：background() 在后台线程/当前线程同步执行后自动 resolve
void ExecutionContext::RunAsync(
    std::function<void()>                    background,
    std::function<void(ExecutionContext&)>   onComplete)
{
    RunAsync(
        [bg = std::move(background)](AsyncResolve resolve) {
            if (bg) bg();
            resolve();
        },
        std::move(onComplete));
}

TimerHandle ExecutionContext::SetTimer(float seconds, TimerCallback callback)
{
    if (!m_runner) return InvalidTimerHandle;

    return m_runner->GetTimerManager().SetTimer(seconds,
        wrapCallbackWithContextRestore(std::move(callback)));
}

// 完整版：指定间隔、重复次数（-1=无限循环）
TimerHandle ExecutionContext::SetTimer(float seconds, int repeatCount, TimerCallback callback)
{
    if (!m_runner) return InvalidTimerHandle;

    return m_runner->GetTimerManager().SetTimer(seconds, repeatCount,
        wrapCallbackWithContextRestore(std::move(callback)));
}

// 带名称版：可通过名称查找/取消
TimerHandle ExecutionContext::SetTimerByName(const std::string& name, float seconds, int repeatCount, TimerCallback callback)
{
    if (!m_runner) return InvalidTimerHandle;

    return m_runner->GetTimerManager().SetTimerByName(name, seconds, repeatCount,
        wrapCallbackWithContextRestore(std::move(callback)));
}


// ============================================================================
// 控制流：ActivateOutputFlow
// ============================================================================

bool ExecutionContext::ActivateOutputFlow(const std::string& pinName)
{
    auto it = m_state->pinNameToId.find(pinName);
    if (it == m_state->pinNameToId.end())
    {
        LogWarning("ActivateOutputFlow: pin '" + pinName + "' not found");
        return true; // 引脚不存在不视为致命错误
    }
    return ActivateOutputFlow(it->second);
}

bool ExecutionContext::ActivateOutputFlow(PinId pinId)
{
    if (!m_runner) return false;

    // 单步模式下，且处于顶层（flowDepth==0，即 StepNext 直接执行的节点发出的 exec 输出）：
    // 不递归执行下游，只把直接下游节点记录到 m_stepPendingNodes，由下次 StepNext 执行。
    // 深层嵌套调用（ForLoop 循环体、Branch 分支等，flowDepth>0）不受影响，正常递归执行。
    if (m_runner->m_stepMode && m_runner->m_flowDepth == 0)
    {
        m_runner->m_blueprint.ensureIndices();
        auto downstreamEndPins = m_runner->m_blueprint.getDownstreamPinIds(pinId);
        for (PinId endPinId : downstreamEndPins)
        {
            const NodeInstance* targetNode = m_runner->m_blueprint.findNodeByPin(endPinId);
            if (targetNode)
                m_runner->m_stepPendingNodes.insert(targetNode->id);
        }
        return true;
    }

    return m_runner->executeDownstreamFromPin(pinId);
}

void ExecutionContext::MarkDownstreamAsHandled(const std::string& pinName)
{
    auto it = m_state->pinNameToId.find(pinName);
    if (it != m_state->pinNameToId.end())
        MarkDownstreamAsHandled(it->second);
}

void ExecutionContext::MarkDownstreamAsHandled(PinId pinId)
{
    if (!m_runner) return;

    // 找到通过该输出引脚连接的所有直接下游节点（使用索引加速）
    std::vector<NodeId> directTargets;
    auto downstreamPins = m_runner->m_blueprint.getDownstreamPinIds(pinId);
    for (PinId endPinId : downstreamPins)
    {
        const NodeInstance* targetNode = m_runner->m_blueprint.findNodeByPin(endPinId);
        if (targetNode)
            directTargets.push_back(targetNode->id);
    }

    // 标记直接目标及其所有下游为已执行
    for (auto nodeId : directTargets)
    {
        m_runner->m_flowExecutedNodes.insert(nodeId);
        auto downstream = m_runner->GetDownstreamNodes(nodeId);
        for (auto id : downstream)
            m_runner->m_flowExecutedNodes.insert(id);
    }
}

// ============================================================================
// EvaluateConditionPin (was at line ~2430 in BlueprintRunner.cpp)
// ============================================================================
// ExecutionContext::EvaluateConditionPin
// ============================================================================
// Re-executes all upstream data nodes feeding into the named input pin,
// propagates their output values, then returns GetInputValue(pinName).asBool().
// Used by WhileLoop so the condition is re-evaluated each iteration rather
// than using the stale value from the initial pin-value propagation pass.

bool ExecutionContext::EvaluateConditionPin(const std::string& pinName)
{
    if (!m_runner) return GetInputValue(pinName).asBool();

    // Find the PinId for the condition pin on the current node
    auto it = m_state->pinNameToId.find(pinName);
    if (it == m_state->pinNameToId.end())
        return GetInputValue(pinName).asBool();

    PinId condPinId = it->second;

    // Find links going INTO this pin (upstream data nodes)
    auto links = m_runner->m_blueprint.findLinksByPin(condPinId);
    for (const auto* link : links)
    {
        if (!link || !link->isEnabled) continue;
        if (link->endPinId != condPinId) continue;   // must be incoming

        const NodeInstance* srcNode = m_runner->m_blueprint.findNodeByPin(link->startPinId);
        if (!srcNode) continue;

        // Re-execute the upstream data node and propagate its outputs
        m_runner->executeNodeInternal(*srcNode);
        m_runner->propagatePinValues(*srcNode);
    }

    return GetInputValue(pinName).asBool();
}

} // namespace Runtime
} // namespace NodeEditor