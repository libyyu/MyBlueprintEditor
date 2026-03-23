// Runtime/BlueprintRunner.cpp - 蓝图运行时执行器实现

#include "BlueprintRunner.h"
#include "BlueprintExporter.h"
#include <fstream>
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
    return false;
}

bool BlueprintRunner::Load(const BlueprintData& data)
{
    m_blueprint = data;
    m_loaded = true;
    m_topoCacheDirty = true;

    // 关联 context 和 runner
    m_context.m_runner = this;

    // 初始化蓝图变量到执行上下文
    m_context.m_metadata = data.metadata;
    for (const auto& var : data.variables)
    {
        m_context.m_variables[var.name] = var.defaultValue;
    }

    // 初始化所有引脚的默认值
    for (const auto& node : data.nodes)
    {
        for (const auto& pin : node.pins)
        {
            if (pin.kind == PinKind::Input && pin.defaultValue.type != PinDataType::Unknown)
            {
                m_context.m_pinValues[pin.id] = pin.defaultValue;
            }
        }
    }

    return true;
}

bool BlueprintRunner::LoadFromJson(const std::string& jsonContent)
{
    JsonBlueprintExporter exporter;
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
    JsonBlueprintExporter exporter;
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

    // 建立 nodeId -> index 映射
    std::unordered_map<NodeId, size_t> idToIndex;
    for (size_t i = 0; i < m_blueprint.nodes.size(); ++i)
    {
        idToIndex[m_blueprint.nodes[i].id] = i;
    }

    // 建立 pinId -> 所属节点 index 映射
    std::unordered_map<PinId, size_t> pinToNodeIndex;
    for (size_t i = 0; i < m_blueprint.nodes.size(); ++i)
    {
        for (const auto& pin : m_blueprint.nodes[i].pins)
        {
            pinToNodeIndex[pin.id] = i;
        }
    }

    size_t n = m_blueprint.nodes.size();

    // 邻接表和入度
    std::vector<std::vector<size_t>> adj(n);
    std::vector<int> inDeg(n, 0);

    for (const auto& link : m_blueprint.links)
    {
        if (!link.isEnabled) continue;

        auto itStart = pinToNodeIndex.find(link.startPinId);
        auto itEnd = pinToNodeIndex.find(link.endPinId);
        if (itStart == pinToNodeIndex.end() || itEnd == pinToNodeIndex.end())
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
    m_context.m_currentNode = &node;
    m_context.m_currentNodeData = node.nodeData;
    m_context.m_pinNameToId.clear();

    for (const auto& pin : node.pins)
    {
        m_context.m_pinNameToId[pin.name] = pin.id;
    }
}

void BlueprintRunner::propagatePinValues(const NodeInstance& node)
{
    // 将该节点的输出引脚值通过链接传播到下游节点的输入引脚
    for (const auto& pin : node.pins)
    {
        if (pin.kind != PinKind::Output) continue;

        auto valueIt = m_context.m_pinValues.find(pin.id);
        if (valueIt == m_context.m_pinValues.end()) continue;

        // 必须先拷贝值！下方 operator[] 插入新 key 时可能触发 rehash，
        // 导致 valueIt 迭代器失效。
        Variant value = valueIt->second;

        // 查找从此输出引脚出发的所有链接
        for (const auto& link : m_blueprint.links)
        {
            if (!link.isEnabled) continue;
            if (link.startPinId == pin.id)
            {
                // 将值传播到目标输入引脚
                m_context.m_pinValues[link.endPinId] = value;
            }
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
            m_logCallback("[WARN] No handler for node '" + node.name +
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

    // 拓扑排序
    std::vector<NodeId> order;
    if (!buildTopologicalOrder(order))
    {
        result.errorMessage = "Blueprint contains a cycle, cannot execute";
        return result;
    }

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
            m_logCallback("[EXEC] Node '" + node->name +
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
                auto it = m_context.m_pinValues.find(pin.id);
                if (it != m_context.m_pinValues.end())
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

    // 获取完整拓扑序
    std::vector<NodeId> fullOrder;
    if (!buildTopologicalOrder(fullOrder))
    {
        result.errorMessage = "Blueprint contains a cycle, cannot execute";
        return result;
    }

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
    m_context.m_variables[name] = value;
}

Variant BlueprintRunner::GetVariable(const std::string& name) const
{
    auto it = m_context.m_variables.find(name);
    return (it != m_context.m_variables.end()) ? it->second : Variant();
}

const std::unordered_map<std::string, Variant>& BlueprintRunner::GetAllVariables() const
{
    return m_context.m_variables;
}

Variant BlueprintRunner::GetPinValue(PinId pinId) const
{
    auto it = m_context.m_pinValues.find(pinId);
    return (it != m_context.m_pinValues.end()) ? it->second : Variant();
}

void BlueprintRunner::SetPinValue(PinId pinId, const Variant& value)
{
    m_context.m_pinValues[pinId] = value;
}

// ============================================================================
// 图查询工具方法
// ============================================================================

std::vector<NodeId> BlueprintRunner::GetTopologicalOrder() const
{
    if (m_topoCacheDirty)
    {
        buildTopologicalOrder(m_topoCache);
        m_topoCacheDirty = false;
    }
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

void BlueprintRunner::SetLogCallback(std::function<void(const std::string&)> callback)
{
    m_logCallback = std::move(callback);
    m_context.OnLog = m_logCallback;
}

void BlueprintRunner::ResetState()
{
    m_context.m_pinValues.clear();
    m_context.m_variables.clear();
    m_context.m_currentNode = nullptr;
    m_context.m_activatedInputPinId = InvalidPinId;
    m_context.m_currentNodeData.clear();
    m_context.m_pinNameToId.clear();
    m_flowDepth = 0;

    // 清理保持存活的子蓝图 runner
    m_keepAliveRunners.clear();

    // 重新初始化默认值
    if (m_loaded)
    {
        for (const auto& var : m_blueprint.variables)
        {
            m_context.m_variables[var.name] = var.defaultValue;
        }
        for (const auto& node : m_blueprint.nodes)
        {
            for (const auto& pin : node.pins)
            {
                if (pin.kind == PinKind::Input && pin.defaultValue.type != PinDataType::Unknown)
                {
                    m_context.m_pinValues[pin.id] = pin.defaultValue;
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

TimerHandle ExecutionContext::Delay(float seconds, const std::function<void()>& callback)
{
    if (m_runner)
    {
        // 保存当前节点的 context 状态，因为 timer 回调在未来帧触发时
        // m_currentNode / m_pinNameToId / m_currentNodeData 已被其他节点覆盖
        auto savedNode = m_currentNode;
        auto savedPinNameToId = m_pinNameToId;
        auto savedNodeData = m_currentNodeData;

        return m_runner->GetTimerManager().SetTimer(seconds, [this, callback, savedNode, savedPinNameToId, savedNodeData]()->bool 
        {
            // 恢复 Delay 节点的 context 状态
            auto* mutableThis = this;
            auto prevNode = mutableThis->m_currentNode;
            auto prevPinNameToId = mutableThis->m_pinNameToId;
            auto prevNodeData = mutableThis->m_currentNodeData;

            mutableThis->m_currentNode = savedNode;
            mutableThis->m_pinNameToId = savedPinNameToId;
            mutableThis->m_currentNodeData = savedNodeData;

            callback();

            // 恢复之前的状态
            mutableThis->m_currentNode = prevNode;
            mutableThis->m_pinNameToId = prevPinNameToId;
            mutableThis->m_currentNodeData = prevNodeData;

            return false;
        });
    }

    return  InvalidTimerHandle;
}

TimerHandle ExecutionContext::SetTimer(float seconds, TimerCallback callback)
{
    if (m_runner)
    {
        // 保存当前节点的 context 状态，因为 timer 回调在未来帧触发时
        // m_currentNode / m_pinNameToId / m_currentNodeData 已被其他节点覆盖
        auto savedNode = m_currentNode;
        auto savedPinNameToId = m_pinNameToId;
        auto savedNodeData = m_currentNodeData;

        return m_runner->GetTimerManager().SetTimer(seconds, [this, callback, savedNode, savedPinNameToId, savedNodeData]()->bool 
        {
            // 恢复 Delay 节点的 context 状态
            auto* mutableThis = this;
            auto prevNode = mutableThis->m_currentNode;
            auto prevPinNameToId = mutableThis->m_pinNameToId;
            auto prevNodeData = mutableThis->m_currentNodeData;

            mutableThis->m_currentNode = savedNode;
            mutableThis->m_pinNameToId = savedPinNameToId;
            mutableThis->m_currentNodeData = savedNodeData;

            const bool ret = callback();

            // 恢复之前的状态
            mutableThis->m_currentNode = prevNode;
            mutableThis->m_pinNameToId = prevPinNameToId;
            mutableThis->m_currentNodeData = prevNodeData;

            return ret;
        });
    }

    return  InvalidTimerHandle;
}

// 完整版：指定间隔、重复次数（-1=无限循环）
TimerHandle ExecutionContext::SetTimer(float seconds, int repeatCount, TimerCallback callback)
{
    if (m_runner)
    {
        // 保存当前节点的 context 状态，因为 timer 回调在未来帧触发时
        // m_currentNode / m_pinNameToId / m_currentNodeData 已被其他节点覆盖
        auto savedNode = m_currentNode;
        auto savedPinNameToId = m_pinNameToId;
        auto savedNodeData = m_currentNodeData;

        return m_runner->GetTimerManager().SetTimer(seconds, repeatCount, [this, callback, savedNode, savedPinNameToId, savedNodeData]()->bool 
        {
            // 恢复 Delay 节点的 context 状态
            auto* mutableThis = this;
            auto prevNode = mutableThis->m_currentNode;
            auto prevPinNameToId = mutableThis->m_pinNameToId;
            auto prevNodeData = mutableThis->m_currentNodeData;

            mutableThis->m_currentNode = savedNode;
            mutableThis->m_pinNameToId = savedPinNameToId;
            mutableThis->m_currentNodeData = savedNodeData;

            const bool ret = callback();

            // 恢复之前的状态
            mutableThis->m_currentNode = prevNode;
            mutableThis->m_pinNameToId = prevPinNameToId;
            mutableThis->m_currentNodeData = prevNodeData;

            return ret;
        });
    }

    return  InvalidTimerHandle;
}

// 带名称版：可通过名称查找/取消
TimerHandle ExecutionContext::SetTimerByName(const std::string& name, float seconds, int repeatCount, TimerCallback callback)
{
    if (m_runner)
    {
        // 保存当前节点的 context 状态，因为 timer 回调在未来帧触发时
        // m_currentNode / m_pinNameToId / m_currentNodeData 已被其他节点覆盖
        auto savedNode = m_currentNode;
        auto savedPinNameToId = m_pinNameToId;
        auto savedNodeData = m_currentNodeData;

        return m_runner->GetTimerManager().SetTimerByName(name, seconds, repeatCount, [this, callback, savedNode, savedPinNameToId, savedNodeData]()->bool 
        {
            // 恢复 Delay 节点的 context 状态
            auto* mutableThis = this;
            auto prevNode = mutableThis->m_currentNode;
            auto prevPinNameToId = mutableThis->m_pinNameToId;
            auto prevNodeData = mutableThis->m_currentNodeData;

            mutableThis->m_currentNode = savedNode;
            mutableThis->m_pinNameToId = savedPinNameToId;
            mutableThis->m_currentNodeData = savedNodeData;

            const bool ret = callback();

            // 恢复之前的状态
            mutableThis->m_currentNode = prevNode;
            mutableThis->m_pinNameToId = prevPinNameToId;
            mutableThis->m_currentNodeData = prevNodeData;

            return ret;
        });
    }

    return  InvalidTimerHandle;
}


// ============================================================================
// 控制流：ActivateOutputFlow
// ============================================================================

bool ExecutionContext::ActivateOutputFlow(const std::string& pinName)
{
    auto it = m_pinNameToId.find(pinName);
    if (it == m_pinNameToId.end())
    {
        Log("[WARN] ActivateOutputFlow: pin '" + pinName + "' not found");
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
    auto it = m_pinNameToId.find(pinName);
    if (it != m_pinNameToId.end())
        MarkDownstreamAsHandled(it->second);
}

void ExecutionContext::MarkDownstreamAsHandled(PinId pinId)
{
    if (!m_runner) return;

    // 找到通过该输出引脚连接的所有直接下游节点
    std::vector<NodeId> directTargets;
    for (const auto& link : m_runner->m_blueprint.links)
    {
        if (!link.isEnabled) continue;
        if (link.startPinId == pinId)
        {
            const NodeInstance* targetNode = m_runner->m_blueprint.findNodeByPin(link.endPinId);
            if (targetNode)
                directTargets.push_back(targetNode->id);
        }
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
            m_logCallback("[ERROR] Flow depth exceeded limit (" + std::to_string(kMaxFlowDepth) + "), possible infinite loop");
        --m_flowDepth;
        return false;
    }

    // 保存当前 context 状态（递归执行会修改它）
    auto savedNode = m_context.m_currentNode;
    auto savedPinNameToId = m_context.m_pinNameToId;
    auto savedNodeData = m_context.m_currentNodeData;
    auto savedActivatedInputPinId = m_context.m_activatedInputPinId;

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
        auto valIt = m_context.m_pinValues.find(outputPinId);
        if (valIt != m_context.m_pinValues.end())
        {
            outputPinValue = valIt->second;
            hasOutputPinValue = true;
        }
    }
    for (const auto& link : m_blueprint.links)
    {
        if (!link.isEnabled) continue;
        if (link.startPinId == outputPinId)
        {
            // 传播当前引脚值到目标输入引脚
            if (hasOutputPinValue)
                m_context.m_pinValues[link.endPinId] = outputPinValue;

            const NodeInstance* targetNode = m_blueprint.findNodeByPin(link.endPinId);
            if (targetNode)
            {
                directTargets.push_back(targetNode->id);
                // 记录该目标节点是通过哪个输入引脚被激活的
                nodeToActivatedInputPin[targetNode->id] = link.endPinId;
            }
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

    // 获取拓扑序并过滤
    std::vector<NodeId> fullOrder;
    if (!buildTopologicalOrder(fullOrder))
    {
        // 恢复 context
        m_context.m_currentNode = savedNode;
        m_context.m_pinNameToId = savedPinNameToId;
        m_context.m_currentNodeData = savedNodeData;
        m_context.m_activatedInputPinId = savedActivatedInputPinId;
        --m_flowDepth;
        return false; // cycle
    }

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
            m_logCallback("[EXEC] Node '" + node->name +
                "' (id=" + std::to_string(node->id) +
                ", def=" + node->definitionId + ")");
        }

        // 记录执行前 m_flowExecutedNodes 的快照，用于检测内层递归新增的节点
        auto snapshotBefore = m_flowExecutedNodes;

        // 设置触发该节点的输入引脚 ID（仅直接目标节点有此信息）
        auto activatedIt = nodeToActivatedInputPin.find(id);
        m_context.m_activatedInputPinId = (activatedIt != nodeToActivatedInputPin.end())
            ? activatedIt->second : InvalidPinId;

        if (!executeNodeInternal(*node))
        {
            ok = false;
            break;
        }

        // 清除已用完的激活引脚信息
        m_context.m_activatedInputPinId = InvalidPinId;

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
    m_context.m_currentNode = savedNode;
    m_context.m_pinNameToId = savedPinNameToId;
    m_context.m_currentNodeData = savedNodeData;
    m_context.m_activatedInputPinId = savedActivatedInputPinId;

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

    // 找到连接到该输入引脚的链接（startPin -> inputPinId）
    for (const auto& link : m_blueprint.links)
    {
        if (!link.isEnabled) continue;
        if (link.endPinId != inputPinId) continue;

        // 找到源节点（连接到该输入引脚的输出端节点）
        const NodeInstance* sourceNode = m_blueprint.findNodeByPin(link.startPinId);
        if (!sourceNode) continue;

        if (m_logCallback)
        {
            m_logCallback("[FIRE] Node '" + sourceNode->name +
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

} // namespace Runtime
} // namespace NodeEditor
