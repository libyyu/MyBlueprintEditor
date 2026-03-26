// Runtime/BlueprintRunner.cpp - 蓝图运行时执行器实现

#include "BlueprintRunner.h"
#include "BlueprintExporter.h"
#include <sstream>
#include <chrono>
#include <algorithm>
#include <cassert>
#include <unordered_set>

namespace NodeEditor {
namespace Runtime {

// ============================================================================
// 加载
// ============================================================================

bool BlueprintRunner::IsWithEditor() const
{
    return m_withEditor;
}

bool BlueprintRunner::Load(const BlueprintData& data)
{
    m_blueprint = data;
    m_loaded = true;
    invalidateTopoCache();

    // 预建哈希索引，加速后续查找
    m_blueprint.rebuildIndices();

    // 关联 context 和 runner
    m_context.m_runner = this;
    m_context.m_state  = &m_state;

    // 初始化蓝图变量到执行上下文
    m_state.metadata = data.metadata;
    for (const auto& var : data.variables)
    {
        m_state.variables[var.name] = var.defaultValue;
    }

    // 初始化所有引脚的默认值
    for (const auto& node : data.nodes)
    {
        for (const auto& pin : node.pins)
        {
            if (pin.kind == PinKind::Input && pin.defaultValue.type != PinDataType::Unknown)
            {
                m_state.pinValues[pin.id] = pin.defaultValue;
            }
        }
    }

    return true;
}

bool BlueprintRunner::LoadFromJson(const std::string& jsonContent)
{
    JsonBlueprintExporter exporter(m_fileSystem);
    auto result = exporter.importRuntimeFromString(jsonContent);
    if (!result.success)
    {
        m_lastError = "JSON import failed: " + result.errorMessage;
        return false;
    }
    return Load(result.data);
}

bool BlueprintRunner::LoadFromFile(const std::string& filePath)
{
    JsonBlueprintExporter exporter(m_fileSystem);
    auto result = exporter.importRuntimeFromFile(filePath);
    if (!result.success)
    {
        m_lastError = "File import failed: " + result.errorMessage;
        return false;
    }
    return Load(result.data);
}

// ============================================================================
// 处理器注册
// ============================================================================

void BlueprintRunner::RegisterHandler(const std::string& definitionId, NodeHandler handler)
{
    m_handlers[definitionId] = std::move(handler);
}

void BlueprintRunner::RegisterHandlers(const std::unordered_map<std::string, NodeHandler>& handlers)
{
    for (const auto& pair : handlers)
    {
        m_handlers[pair.first] = pair.second;
    }
}

void BlueprintRunner::UnregisterHandler(const std::string& definitionId)
{
    m_handlers.erase(definitionId);
}

bool BlueprintRunner::HasHandler(const std::string& definitionId) const
{
    return m_handlers.find(definitionId) != m_handlers.end();
}

void BlueprintRunner::SetDefaultHandler(NodeHandler handler)
{
    m_defaultHandler = std::move(handler);
}

// ============================================================================
// 拓扑排序（Kahn 算法）
// ============================================================================

bool BlueprintRunner::buildTopologicalOrder(std::vector<NodeId>& order) const
{
    order.clear();

    if (m_blueprint.nodes.empty())
        return true;

    // 确保 BlueprintData 索引已建好
    m_blueprint.ensureIndices();

    // 建立 nodeId -> index 映射（使用已有索引）
    std::unordered_map<NodeId, size_t> idToIndex;
    for (size_t i = 0; i < m_blueprint.nodes.size(); ++i)
    {
        idToIndex[m_blueprint.nodes[i].id] = i;
    }

    size_t n = m_blueprint.nodes.size();

    // 邻接表和入度
    std::vector<std::vector<size_t>> adj(n);
    std::vector<int> inDeg(n, 0);

    for (const auto& link : m_blueprint.links)
    {
        if (!link.isEnabled) continue;

        const auto& pinNodeIndex = m_blueprint.getPinToNodeIndex();
        auto itStart = pinNodeIndex.find(link.startPinId);
        auto itEnd = pinNodeIndex.find(link.endPinId);
        if (itStart == pinNodeIndex.end() || itEnd == pinNodeIndex.end())
            continue;

        size_t from = itStart->second;
        size_t to = itEnd->second;
        if (from == to) continue; // 自环跳过

        adj[from].push_back(to);
        inDeg[to]++;
    }

    // 去重邻接表中的重复边
    for (auto& neighbors : adj)
    {
        std::sort(neighbors.begin(), neighbors.end());
        neighbors.erase(std::unique(neighbors.begin(), neighbors.end()), neighbors.end());
    }

    // 重新计算入度（去重后）
    std::fill(inDeg.begin(), inDeg.end(), 0);
    for (size_t i = 0; i < n; ++i)
    {
        for (size_t j : adj[i])
        {
            inDeg[j]++;
        }
    }

    // BFS (Kahn)
    std::queue<size_t> q;
    for (size_t i = 0; i < n; ++i)
    {
        if (inDeg[i] == 0)
        {
            q.push(i);
        }
    }

    while (!q.empty())
    {
        size_t cur = q.front();
        q.pop();
        order.push_back(m_blueprint.nodes[cur].id);

        for (size_t next : adj[cur])
        {
            if (--inDeg[next] == 0)
            {
                q.push(next);
            }
        }
    }

    // 检查是否有环
    if (order.size() != n)
    {
        return false; // 有环
    }

    return true;
}

// ============================================================================
// 执行内部辅助
// ============================================================================

void BlueprintRunner::prepareNodeContext(const NodeInstance& node)
{
    m_state.currentNode = &node;
    m_state.nodeData = node.nodeData;
    m_state.pinNameToId.clear();

    for (const auto& pin : node.pins)
    {
        m_state.pinNameToId[pin.name] = pin.id;
    }
}

void BlueprintRunner::propagatePinValues(const NodeInstance& node)
{
    // 将该节点的输出引脚值通过链接传播到下游节点的输入引脚
    m_blueprint.ensureIndices();

    for (const auto& pin : node.pins)
    {
        if (pin.kind != PinKind::Output) continue;

        auto valueIt = m_state.pinValues.find(pin.id);
        if (valueIt == m_state.pinValues.end()) continue;

        // 必须先拷贝值！下方 operator[] 插入新 key 时可能触发 rehash，
        // 导致 valueIt 迭代器失效。
        Variant value = valueIt->second;

        // 使用索引快速查找从此输出引脚出发的所有链接
        auto downstreamPins = m_blueprint.getDownstreamPinIds(pin.id);
        for (PinId endPinId : downstreamPins)
        {
            m_state.pinValues[endPinId] = value;
        }
    }
}

bool BlueprintRunner::executeNodeInternal(const NodeInstance& node)
{
    if (!node.isEnabled) return true; // 跳过禁用的节点

    // 准备执行上下文
    prepareNodeContext(node);

    // 查找处理器
    auto it = m_handlers.find(node.definitionId);
    NodeHandler handler;

    if (it != m_handlers.end())
    {
        handler = it->second;
    }
    else if (m_defaultHandler)
    {
        handler = m_defaultHandler;
    }
    else
    {
        // 没有处理器，跳过但记录警告
        if (m_logCallback)
        {
            m_logCallback(LogLevel::Warning, "No handler for node '" + node.name +
                "' (def: " + node.definitionId + "), skipping");
        }
        return true;
    }

    // 执行
    bool ok = handler(m_context);

    if (ok)
    {
        // 执行成功，传播输出值到下游
        propagatePinValues(node);
    }

    return ok;
}

// ============================================================================
// 执行
// ============================================================================

ExecutionResult BlueprintRunner::Execute()
{
    ExecutionResult result;

    if (!m_loaded)
    {
        result.errorMessage = "No blueprint loaded";
        return result;
    }

    auto startTime = std::chrono::high_resolution_clock::now();

    // 拓扑排序（使用缓存）
    if (!ensureTopologicalOrder())
    {
        result.errorMessage = "Blueprint contains a cycle, cannot execute";
        return result;
    }
    const auto& order = m_topoCache;

    // 按拓扑顺序执行
    m_flowExecutedNodes.clear(); // 清空控制流已执行记录

    // 收集所有事件源节点及其 exec 下游子图（如 CustomEvent → Print String）
    // 这些节点不在主循环中执行，只在被外部触发（如 Timer 回调）时执行
    auto eventSubgraph = m_blueprint.collectEventSubgraphs();

    for (NodeId nodeId : order)
    {
        // 跳过已被控制流（ActivateOutputFlow）递归执行过的节点
        if (m_flowExecutedNodes.count(nodeId))
            continue;

        // 跳过事件子图中的节点（它们只在被外部触发时执行）
        if (eventSubgraph.count(nodeId))
            continue;

        const NodeInstance* node = m_blueprint.findNode(nodeId);
        if (!node) continue;

        if (m_logCallback)
        {
            m_logCallback(LogLevel::Verbose, "Node '" + node->name +
                "' (id=" + std::to_string(node->id) +
                ", def=" + node->definitionId + ")");
        }

        if (!executeNodeInternal(*node))
        {
            result.errorMessage = "Execution failed at node '" + node->name +
                "' (id=" + std::to_string(node->id) + ")";
            result.nodesExecuted++;

            auto endTime = std::chrono::high_resolution_clock::now();
            result.elapsedMs = std::chrono::duration<double, std::milli>(endTime - startTime).count();
            return result;
        }

        result.nodesExecuted++;
    }

    // 收集所有输出引脚的最终值
    for (const auto& node : m_blueprint.nodes)
    {
        for (const auto& pin : node.pins)
        {
            if (pin.kind == PinKind::Output)
            {
                auto it = m_state.pinValues.find(pin.id);
                if (it != m_state.pinValues.end())
                {
                    result.outputValues[pin.id] = it->second;
                }
            }
        }
    }

    auto endTime = std::chrono::high_resolution_clock::now();
    result.elapsedMs = std::chrono::duration<double, std::milli>(endTime - startTime).count();
    result.success = true;

    return result;
}

ExecutionResult BlueprintRunner::ExecuteNode(NodeId nodeId)
{
    // 获取上游节点（含自身），按拓扑序执行
    auto upstreamIds = GetUpstreamNodes(nodeId);
    upstreamIds.push_back(nodeId);
    return ExecuteNodes(upstreamIds);
}

ExecutionResult BlueprintRunner::ExecuteNodes(const std::vector<NodeId>& nodeIds)
{
    ExecutionResult result;

    if (!m_loaded)
    {
        result.errorMessage = "No blueprint loaded";
        return result;
    }

    auto startTime = std::chrono::high_resolution_clock::now();

    // 获取完整拓扑序（使用缓存）
    if (!ensureTopologicalOrder())
    {
        result.errorMessage = "Blueprint contains a cycle, cannot execute";
        return result;
    }
    const auto& fullOrder = m_topoCache;

    // 过滤：只保留指定的节点，但保持拓扑顺序
    std::unordered_set<NodeId> targetSet(nodeIds.begin(), nodeIds.end());
    std::vector<NodeId> filteredOrder;
    for (NodeId id : fullOrder)
    {
        if (targetSet.count(id))
        {
            filteredOrder.push_back(id);
        }
    }

    // 执行
    for (NodeId id : filteredOrder)
    {
        const NodeInstance* node = m_blueprint.findNode(id);
        if (!node) continue;

        if (!executeNodeInternal(*node))
        {
            result.errorMessage = "Execution failed at node '" + node->name +
                "' (id=" + std::to_string(node->id) + ")";
            auto endTime = std::chrono::high_resolution_clock::now();
            result.elapsedMs = std::chrono::duration<double, std::milli>(endTime - startTime).count();
            return result;
        }
        result.nodesExecuted++;
    }

    auto endTime = std::chrono::high_resolution_clock::now();
    result.elapsedMs = std::chrono::duration<double, std::milli>(endTime - startTime).count();
    result.success = true;
    return result;
}

// ============================================================================
// 变量与引脚值操作
// ============================================================================

void BlueprintRunner::SetVariable(const std::string& name, const Variant& value)
{
    m_state.variables[name] = value;
}

Variant BlueprintRunner::GetVariable(const std::string& name) const
{
    auto it = m_state.variables.find(name);
    return (it != m_state.variables.end()) ? it->second : Variant();
}

const std::unordered_map<std::string, Variant>& BlueprintRunner::GetAllVariables() const
{
    return m_state.variables;
}

Variant BlueprintRunner::GetPinValue(PinId pinId) const
{
    auto it = m_state.pinValues.find(pinId);
    return (it != m_state.pinValues.end()) ? it->second : Variant();
}

void BlueprintRunner::SetPinValue(PinId pinId, const Variant& value)
{
    m_state.pinValues[pinId] = value;
}

// ============================================================================
// 图查询工具方法
// ============================================================================

bool BlueprintRunner::ensureTopologicalOrder() const
{
    if (m_topoCacheDirty)
    {
        m_topoCacheHasCycle = !buildTopologicalOrder(m_topoCache);
        m_topoCacheDirty = false;
    }
    return !m_topoCacheHasCycle;
}

std::vector<NodeId> BlueprintRunner::GetTopologicalOrder() const
{
    ensureTopologicalOrder();
    return m_topoCache;
}

std::vector<NodeId> BlueprintRunner::GetUpstreamNodes(NodeId nodeId) const
{
    std::vector<NodeId> result;
    std::unordered_set<NodeId> visited;
    std::queue<NodeId> queue;

    // 获取目标节点的所有输入节点
    auto inputNodes = m_blueprint.getInputNodes(nodeId);
    for (auto id : inputNodes) queue.push(id);

    while (!queue.empty())
    {
        NodeId cur = queue.front();
        queue.pop();

        if (visited.count(cur)) continue;
        visited.insert(cur);
        result.push_back(cur);

        auto upstream = m_blueprint.getInputNodes(cur);
        for (auto id : upstream) queue.push(id);
    }

    return result;
}

std::vector<NodeId> BlueprintRunner::GetDownstreamNodes(NodeId nodeId) const
{
    std::vector<NodeId> result;
    std::unordered_set<NodeId> visited;
    std::queue<NodeId> queue;

    auto outputNodes = m_blueprint.getOutputNodes(nodeId);
    for (auto id : outputNodes) queue.push(id);

    while (!queue.empty())
    {
        NodeId cur = queue.front();
        queue.pop();

        if (visited.count(cur)) continue;
        visited.insert(cur);
        result.push_back(cur);

        auto downstream = m_blueprint.getOutputNodes(cur);
        for (auto id : downstream) queue.push(id);
    }

    return result;
}

// ============================================================================
// 其他
// ============================================================================

void BlueprintRunner::SetLogCallback(std::function<void(LogLevel, const std::string&)> callback)
{
    m_logCallback = std::move(callback);
    m_context.OnLog = m_logCallback;
}

void BlueprintRunner::SetPrintCallback(std::function<void(LogLevel, const std::string&)> callback)
{
    m_printCallback = std::move(callback);
    m_context.OnPrint = m_printCallback;
}

void BlueprintRunner::ResetState()
{
    m_state.pinValues.clear();
    m_state.variables.clear();
    m_state.currentNode = nullptr;
    m_state.activatedInputPinId = InvalidPinId;
    m_state.nodeData.clear();
    m_state.pinNameToId.clear();
    m_flowDepth = 0;

    // 清理保持存活的子蓝图 runner，重置异步计数
    m_keepAliveRunners.clear();
    m_pendingAsyncCount = 0;

    // 重新初始化默认值
    if (m_loaded)
    {
        for (const auto& var : m_blueprint.variables)
        {
            m_state.variables[var.name] = var.defaultValue;
        }
        for (const auto& node : m_blueprint.nodes)
        {
            for (const auto& pin : node.pins)
            {
                if (pin.kind == PinKind::Input && pin.defaultValue.type != PinDataType::Unknown)
                {
                    m_state.pinValues[pin.id] = pin.defaultValue;
                }
            }
        }
    }
}

bool ExecutionContext::FireConnectedNode(PinId inputPinId)
{
    if (m_runner)
        return m_runner->FireConnectedNode(inputPinId);
    return false;
}

// ============================================================================
// Timer context 辅助：保存/恢复 context 状态的 RAII guard + 统一包装
// ============================================================================

// 内部辅助：将用户回调包装为带 context save/restore 的 TimerCallback
// timer 回调在未来帧触发时 m_state->currentNode / pinNameToId / nodeData
// 已被其他节点覆盖，需要先恢复再调用用户回调
TimerCallback ExecutionContext::wrapCallbackWithContextRestore(TimerCallback callback)
{
    auto savedNode        = m_state->currentNode;
    auto savedPinNameToId = m_state->pinNameToId;
    auto savedNodeData    = m_state->nodeData;

    if (m_runner)
        m_runner->AcquireAsync();

    BlueprintRunner* runner = m_runner;

    return [this, runner, callback = std::move(callback),
            savedNode, savedPinNameToId, savedNodeData]() mutable -> bool
    {
        // 保存当前状态
        auto prevNode        = m_state->currentNode;
        auto prevPinNameToId = m_state->pinNameToId;
        auto prevNodeData    = m_state->nodeData;

        // 恢复注册时的状态
        m_state->currentNode  = savedNode;
        m_state->pinNameToId  = savedPinNameToId;
        m_state->nodeData     = savedNodeData;

        const bool ret = callback();

        // 恢复调用前的状态
        m_state->currentNode  = prevNode;
        m_state->pinNameToId  = prevPinNameToId;
        m_state->nodeData     = prevNodeData;

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

bool BlueprintRunner::executeDownstreamFromPin(PinId outputPinId)
{
    if (++m_flowDepth > kMaxFlowDepth)
    {
        if (m_logCallback)
            m_logCallback(LogLevel::Error, "Flow depth exceeded limit (" + std::to_string(kMaxFlowDepth) + "), possible infinite loop");
        --m_flowDepth;
        return false;
    }

    // 保存当前 context 状态（递归执行会修改它）
    auto savedNode = m_state.currentNode;
    auto savedPinNameToId = m_state.pinNameToId;
    auto savedNodeData = m_state.nodeData;
    auto savedActivatedInputPinId = m_state.activatedInputPinId;

    // 传播当前节点的所有输出引脚值到下游（不仅是触发的 exec 引脚）
    if (savedNode)
        propagatePinValues(*savedNode);

    // 找到通过该输出引脚连接的所有直接下游节点
    std::vector<NodeId> directTargets;
    // 记录直接目标节点对应的被激活输入引脚 ID
    // （用于 DoN 的 Enter/Reset、Gate 的 Enter/Open/Close/Toggle 等多 exec 输入引脚节点）
    std::unordered_map<NodeId, PinId> nodeToActivatedInputPin;
    // 预先拷贝引脚值，避免下方 operator[] 插入新 key 触发 rehash 导致迭代器失效
    Variant outputPinValue;
    bool hasOutputPinValue = false;
    {
        auto valIt = m_state.pinValues.find(outputPinId);
        if (valIt != m_state.pinValues.end())
        {
            outputPinValue = valIt->second;
            hasOutputPinValue = true;
        }
    }
    m_blueprint.ensureIndices();
    auto downstreamEndPins = m_blueprint.getDownstreamPinIds(outputPinId);
    for (PinId endPinId : downstreamEndPins)
    {
        // 传播当前引脚值到目标输入引脚
        if (hasOutputPinValue)
            m_state.pinValues[endPinId] = outputPinValue;

        const NodeInstance* targetNode = m_blueprint.findNodeByPin(endPinId);
        if (targetNode)
        {
            directTargets.push_back(targetNode->id);
            // 记录该目标节点是通过哪个输入引脚被激活的
            nodeToActivatedInputPin[targetNode->id] = endPinId;
        }
    }

    // 去重
    std::sort(directTargets.begin(), directTargets.end());
    directTargets.erase(std::unique(directTargets.begin(), directTargets.end()), directTargets.end());

    // 收集需要执行的节点：
    //   从直接目标开始，沿 exec 链展开下游节点。但遇到"多 exec 输出引脚"
    //   的控制流节点（如 Branch、ForLoop）时，收集该节点本身但**不继续展开**
    //   它的 exec 下游，因为这些节点的 handler 会通过 ActivateOutputFlow 自行
    //   管理其子图的执行。
    //   最后，为所有收集到的节点补充数据上游依赖。
    std::unordered_set<NodeId> targetSet(directTargets.begin(), directTargets.end());

    // 沿 exec 链展开
    {
        std::queue<NodeId> expandQueue;
        for (auto id : directTargets) expandQueue.push(id);
        while (!expandQueue.empty())
        {
            NodeId cur = expandQueue.front();
            expandQueue.pop();

            int execOutCount = m_blueprint.countExecOutputPins(cur);

            // 如果该节点有多个已连接的 exec 输出引脚，说明是控制流节点
            // 不继续展开其 exec 下游（由其 handler 管理）
            if (execOutCount > 1)
                continue;

            // 单 exec 输出（或无 exec 输出）的普通节点，继续沿 exec 链展开
            auto execDownstream = m_blueprint.getExecOutputNodes(cur);
            for (auto id : execDownstream)
            {
                if (targetSet.insert(id).second)
                {
                    expandQueue.push(id);
                }
            }
        }
    }

    // 递归收集所有目标节点的数据上游依赖（排除触发当前流的源节点自身）
    // 同时排除事件源节点（无 exec 输入、有 exec 输出的节点，如 CustomEvent）
    // 它们虽然通过数据连接（Function/Delegate引脚）被引用，但不应作为数据依赖执行
    {
        NodeId sourceNodeId = savedNode ? savedNode->id : 0;
        std::queue<NodeId> depQueue;
        for (auto id : targetSet) depQueue.push(id);
        while (!depQueue.empty())
        {
            NodeId cur = depQueue.front();
            depQueue.pop();
            auto dataInputs = m_blueprint.getDataInputNodes(cur);
            for (auto depId : dataInputs)
            {
                if (depId == sourceNodeId) continue;
                if (m_blueprint.isEventSourceNode(depId)) continue;
                if (targetSet.insert(depId).second)
                {
                    depQueue.push(depId);
                }
            }
        }
    }

    // 获取拓扑序并过滤（使用缓存）
    if (!ensureTopologicalOrder())
    {
        // 恢复 context
        m_state.currentNode = savedNode;
        m_state.pinNameToId = savedPinNameToId;
        m_state.nodeData = savedNodeData;
        m_state.activatedInputPinId = savedActivatedInputPinId;
        --m_flowDepth;
        return false; // cycle
    }
    const auto& fullOrder = m_topoCache;

    // executedHere: 追踪在本次执行循环中，被内层 ActivateOutputFlow 递归执行过的节点
    // 用于防止"单 exec 输出节点的 handler 调用 ActivateOutputFlow 后，
    // 其下游在当前循环中又被重复执行"的问题。
    // 注意：不能用全局 m_flowExecutedNodes 做此判断，因为 ForLoop 等控制流节点
    // 会多次调用 ActivateOutputFlow 重复执行同一批循环体节点，全局集合会导致
    // 第二次迭代开始所有节点被误跳过。
    std::unordered_set<NodeId> executedHere;

    bool ok = true;
    for (NodeId id : fullOrder)
    {
        if (targetSet.find(id) == targetSet.end())
            continue;

        // 跳过在本次循环中已被内层 ActivateOutputFlow 递归执行过的节点
        if (executedHere.count(id))
            continue;

        const NodeInstance* node = m_blueprint.findNode(id);
        if (!node) continue;

        if (m_logCallback)
        {
            m_logCallback(LogLevel::Verbose, "Node '" + node->name +
                "' (id=" + std::to_string(node->id) +
                ", def=" + node->definitionId + ")");
        }

        // 记录执行前 m_flowExecutedNodes 的快照，用于检测内层递归新增的节点
        auto snapshotBefore = m_flowExecutedNodes;

        // 设置触发该节点的输入引脚 ID（仅直接目标节点有此信息）
        auto activatedIt = nodeToActivatedInputPin.find(id);
        m_state.activatedInputPinId = (activatedIt != nodeToActivatedInputPin.end())
            ? activatedIt->second : InvalidPinId;

        if (!executeNodeInternal(*node))
        {
            ok = false;
            break;
        }

        // 清除已用完的激活引脚信息
        m_state.activatedInputPinId = InvalidPinId;

        // 标记为全局已执行（供主循环 execute() 使用）
        m_flowExecutedNodes.insert(id);

        // 如果该节点的 handler 通过 ActivateOutputFlow 递归执行了更多节点，
        // 把这些新增节点加入 executedHere，在当前循环中跳过（防止重复执行）
        if (m_flowExecutedNodes.size() > snapshotBefore.size() + 1)
        {
            for (auto flowId : m_flowExecutedNodes)
            {
                if (flowId != id && !snapshotBefore.count(flowId))
                    executedHere.insert(flowId);
            }
        }
    }

    // 恢复 context 状态
    m_state.currentNode = savedNode;
    m_state.pinNameToId = savedPinNameToId;
    m_state.nodeData = savedNodeData;
    m_state.activatedInputPinId = savedActivatedInputPinId;

    --m_flowDepth;
    return ok;
}

// ============================================================================
// FireConnectedNode: 通过输入引脚ID找到连接的源节点并执行
// 用于 SetTimer 等节点在 timer 回调中触发 Function Name 引脚连接的回调节点
// ============================================================================

bool BlueprintRunner::FireConnectedNode(PinId inputPinId)
{
    if (!m_loaded || inputPinId == InvalidPinId) return false;

    // 使用索引查找连接到该输入引脚的链接（O(k) 而非 O(L)）
    auto connectedLinks = m_blueprint.findLinksByPin(inputPinId);
    for (const auto* link : connectedLinks)
    {
        if (!link->isEnabled) continue;
        if (link->endPinId != inputPinId) continue;

        // 找到源节点（连接到该输入引脚的输出端节点）
        const NodeInstance* sourceNode = m_blueprint.findNodeByPin(link->startPinId);
        if (!sourceNode) continue;

        if (m_logCallback)
        {
            m_logCallback(LogLevel::Verbose, "Node '" + sourceNode->name +
                "' (id=" + std::to_string(sourceNode->id) +
                ", def=" + sourceNode->definitionId + ")");
        }

        // 执行该节点（handler 内部会通过 ActivateOutputFlow 自行管理下游执行）
        if (!executeNodeInternal(*sourceNode))
            return false;

        // 传播输出值
        propagatePinValues(*sourceNode);
    }

    return true;
}

// ============================================================================
// Tick — 驱动计时器并回收已完成的子蓝图 runner
// ============================================================================

void BlueprintRunner::Tick(float deltaTime)
{
    m_timerManager->Tick(deltaTime);

    // 清理 m_keepAliveRunners 中所有 timer 已全部触发完的子 runner。
    //
    // 判断条件：子 runner 自身的 m_timerManager 活跃 timer 数为 0。
    // 注意：子 runner 使用 SetParentTimerManager 把 timer 注册到父 runner 的
    //       m_timerManager 里，因此子 runner 自身的 m_timerManager 始终为空；
    //       我们真正需要等待的是"父 timerManager 里由该子 runner 注册的 timer"。
    //
    // 当前实现的折中方案：
    //   - Execute() 完成后立即 KeepAlive，此时子 runner 自身无 timer
    //   - 如果子 runner 内部也会再派生 timer（例如子蓝图里有 Delay 节点），
    //     则这些 timer 实际上注册在父 timerManager，子 runner 的 m_timerManager
    //     依然是空的 —— 这意味着子 runner 在下一个 Tick 就会被清理，
    //     而 timer 回调仍持有对它的 shared_ptr 引用，所以不会真正析构，
    //     只是提前从 m_keepAliveRunners 里移除。
    //
    // 结论：用 use_count() == 1（只有 m_keepAliveRunners 持有引用）作为
    //       "可以释放"的判断更准确：timer 回调持有 shared_ptr 时
    //       use_count >= 2，回调全部触发完毕后 use_count 降回 1。
    //
    if (!m_keepAliveRunners.empty())
    {
        m_keepAliveRunners.erase(
            std::remove_if(
                m_keepAliveRunners.begin(),
                m_keepAliveRunners.end(),
                [](const std::shared_ptr<BlueprintRunner>& sub) {
                    // 没有正在进行的异步操作时可以释放。
                    // 涵盖 timer、网络、IO 等所有通过 AcquireAsync/ReleaseAsync 登记的操作。
                    return !sub->HasPendingAsync();
                }),
            m_keepAliveRunners.end());
    }
}

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

