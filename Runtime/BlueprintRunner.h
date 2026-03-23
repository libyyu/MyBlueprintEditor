// Runtime/BlueprintRunner.h - 蓝图运行时执行器
// 该模块完全独立于编辑器(ImGui)，可在任意环境中加载和执行蓝图数据
//
// 使用方式:
//   1. 编辑器导出 JSON 文件
//   2. 其他环境加载该 JSON
//   3. 注册节点处理器（每种 definitionId 对应一个执行函数）
//   4. 调用 Execute() 按拓扑顺序执行所有节点

#pragma once

#include "BlueprintData.h"
#include "NodeDefinition.h"
#include "FrameTimerManager.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <functional>
#include <memory>
#include <queue>
#include <algorithm>
#include <unordered_set>
#include <thread>
#include <chrono>

namespace NodeEditor {
namespace Runtime {

// ============================================================================
// 执行上下文 —— 节点处理函数可通过它读写引脚数据
// ============================================================================

class ExecutionContext
{
public:
    // 获取输入引脚的值
    Variant GetInputValue(PinId pinId) const
    {
        auto it = m_pinValues.find(pinId);
        return (it != m_pinValues.end()) ? it->second : Variant();
    }

    // 获取输入引脚的值（按名称查找，在当前节点的输入引脚中搜索）
    Variant GetInputValue(const std::string& pinName) const
    {
        auto it = m_pinNameToId.find(pinName);
        if (it != m_pinNameToId.end())
        {
            return GetInputValue(it->second);
        }
        return Variant();
    }

    // 设置输出引脚的值
    void SetOutputValue(PinId pinId, const Variant& value)
    {
        m_pinValues[pinId] = value;
    }

    // 设置输出引脚的值（按名称查找）
    void SetOutputValue(const std::string& pinName, const Variant& value)
    {
        auto it = m_pinNameToId.find(pinName);
        if (it != m_pinNameToId.end())
        {
            SetOutputValue(it->second, value);
        }
    }

    // 获取节点自定义数据
    Variant GetNodeData(const std::string& key) const
    {
        auto it = m_currentNodeData.find(key);
        return (it != m_currentNodeData.end()) ? it->second : Variant();
    }

    // 获取蓝图变量
    Variant GetVariable(const std::string& name) const
    {
        auto it = m_variables.find(name);
        return (it != m_variables.end()) ? it->second : Variant();
    }

    // 设置蓝图变量
    void SetVariable(const std::string& name, const Variant& value)
    {
        m_variables[name] = value;
    }

    // 获取当前正在执行的节点
    const NodeInstance* GetCurrentNode() const { return m_currentNode; }

    // 获取蓝图元数据
    const BlueprintMetadata& GetMetadata() const { return m_metadata; }

    // 日志输出（可被外部替换）
    std::function<void(const std::string& message)> OnLog;

    void Log(const std::string& message) const
    {
        if (OnLog) OnLog(message);
    }

    TimerHandle Delay(float seconds, const std::function<void()>& callback);

    TimerHandle SetTimer(float delay, TimerCallback callback);

    // 完整版：指定间隔、重复次数（-1=无限循环）
    TimerHandle SetTimer(float interval, int repeatCount, TimerCallback callback);

    // 带名称版：可通过名称查找/取消
    TimerHandle SetTimerByName(const std::string& name, float interval, int repeatCount, TimerCallback callback);

    // ----------------------------------------------------------------
    // 控制流 API —— 允许 handler 触发指定输出 exec 引脚连接的下游子图
    // ----------------------------------------------------------------

    // 按引脚名称激活输出流（执行连接到该输出 exec 引脚的所有下游节点）
    // 可多次调用以实现循环；返回 false 表示执行失败
    bool ActivateOutputFlow(const std::string& pinName);

    // 按引脚 ID 激活输出流
    bool ActivateOutputFlow(PinId pinId);

    // 标记指定输出引脚的所有下游节点为"已被控制流接管"，
    // 主循环会跳过这些节点。用于异步节点（如 Delay）预先占位，
    // 防止主循环在异步回调之前就执行了下游节点。
    void MarkDownstreamAsHandled(const std::string& pinName);
    void MarkDownstreamAsHandled(PinId pinId);

    // 获取当前节点指定引脚名对应的 PinId（用于在异步回调中捕获引脚ID）
    PinId GetPinId(const std::string& pinName) const
    {
        auto it = m_pinNameToId.find(pinName);
        return (it != m_pinNameToId.end()) ? it->second : InvalidPinId;
    }

    // 获取触发当前节点执行的输入引脚 ID
    // 用于有多个 exec 输入引脚的节点（如 DoN 的 Enter/Reset, Gate 的 Enter/Open/Close/Toggle）
    // 区分是从哪个输入引脚触发的
    PinId GetActivatedInputPinId() const { return m_activatedInputPinId; }

    // 获取触发当前节点的输入引脚名称
    std::string GetActivatedInputPinName() const
    {
        if (m_activatedInputPinId == InvalidPinId) return "";
        if (!m_currentNode) return "";
        for (const auto& pin : m_currentNode->pins)
        {
            if (pin.id == m_activatedInputPinId)
                return pin.name;
        }
        return "";
    }

    // 通过输入引脚 ID 找到连接的源节点并执行
    // 用于 SetTimer 等节点在 timer 回调中触发 Function Name 引脚连接的回调节点
    // 内部转发到当前 runner（而非注册 handler 时捕获的 runner），
    // 这样在子蓝图中使用时也能正确找到子蓝图的节点
    bool FireConnectedNode(PinId inputPinId);

private:
    friend class BlueprintRunner;

    // 所有引脚的当前值（输入和输出共用，通过链接传播）
    std::unordered_map<PinId, Variant>              m_pinValues;

    // 当前节点的引脚名到ID的映射（每次执行节点前重建）
    std::unordered_map<std::string, PinId>          m_pinNameToId;

    // 当前节点的自定义数据
    std::unordered_map<std::string, Variant>         m_currentNodeData;

    // 蓝图级别的变量
    std::unordered_map<std::string, Variant>         m_variables;

    // 当前正在执行的节点指针
    const NodeInstance*                             m_currentNode = nullptr;

    // 触发当前节点执行的输入引脚 ID（由 executeDownstreamFromPin 设置）
    // 用于区分 DoN 的 Enter/Reset、Gate 的 Enter/Open/Close/Toggle 等
    PinId                                           m_activatedInputPinId = InvalidPinId;

    // 蓝图元数据
    BlueprintMetadata                               m_metadata;

    // 所属 runner（用于 ActivateOutputFlow 回调）
    BlueprintRunner*                                m_runner = nullptr;
};

// ============================================================================
// 节点处理器类型
// ============================================================================

// 节点处理函数签名: 接收执行上下文，返回是否成功
using NodeHandler = std::function<bool(ExecutionContext& context)>;

// ============================================================================
// 执行结果
// ============================================================================

struct ExecutionResult
{
    bool                        success = false;
    std::string                 errorMessage;
    std::vector<std::string>    warnings;
    int                         nodesExecuted = 0;      // 执行了多少个节点
    double                      elapsedMs = 0.0;        // 执行耗时(毫秒)

    // 获取最终的输出值（通过引脚ID）
    std::unordered_map<PinId, Variant> outputValues;
};

// ============================================================================
// 蓝图运行时执行器
// ============================================================================

class BlueprintRunner
{
public:
    BlueprintRunner() = default;
    ~BlueprintRunner() = default;

    bool IsWithEditor() const;

    // ------------------------------------------------------------------
    // 加载蓝图数据
    // ------------------------------------------------------------------

    // 从 BlueprintData 直接加载
    bool Load(const BlueprintData& data);

    // 从 JSON 字符串加载
    bool LoadFromJson(const std::string& jsonContent);

    // 从 JSON 文件加载
    bool LoadFromFile(const std::string& filePath);

    // 获取已加载的蓝图数据
    const BlueprintData& GetBlueprintData() const { return m_blueprint; }

    // 是否已加载
    bool IsLoaded() const { return m_loaded; }

    // ------------------------------------------------------------------
    // 注册节点处理器
    // ------------------------------------------------------------------

    // 注册单个节点类型的处理器
    void RegisterHandler(const std::string& definitionId, NodeHandler handler);

    // 批量注册处理器
    void RegisterHandlers(const std::unordered_map<std::string, NodeHandler>& handlers);

    // 注销处理器
    void UnregisterHandler(const std::string& definitionId);

    // 检查处理器是否已注册
    bool HasHandler(const std::string& definitionId) const;

    // 获取所有已注册的处理器映射表（用于传递给子蓝图）
    const std::unordered_map<std::string, NodeHandler>& GetHandlers() const { return m_handlers; }

    // 设置默认处理器（用于没有注册处理器的节点）
    void SetDefaultHandler(NodeHandler handler);

    // ------------------------------------------------------------------
    // 执行
    // ------------------------------------------------------------------

    // 执行整个蓝图（按拓扑排序）
    ExecutionResult Execute();

    // 执行指定节点（及其所有上游依赖节点）
    ExecutionResult ExecuteNode(NodeId nodeId);

    // 执行指定范围的节点
    ExecutionResult ExecuteNodes(const std::vector<NodeId>& nodeIds);

    // ------------------------------------------------------------------
    // 变量操作
    // ------------------------------------------------------------------

    // 设置蓝图变量（在执行前预设输入值）
    void SetVariable(const std::string& name, const Variant& value);

    // 获取蓝图变量
    Variant GetVariable(const std::string& name) const;

    // 获取所有变量
    const std::unordered_map<std::string, Variant>& GetAllVariables() const;

    // ------------------------------------------------------------------
    // 引脚值操作（执行后读取输出）
    // ------------------------------------------------------------------

    // 获取引脚的当前值
    Variant GetPinValue(PinId pinId) const;

    // 预设引脚值（可用于注入外部输入）
    void SetPinValue(PinId pinId, const Variant& value);

    // ------------------------------------------------------------------
    // 工具方法
    // ------------------------------------------------------------------

    // 获取拓扑排序结果
    std::vector<NodeId> GetTopologicalOrder() const;

    // 获取上游依赖节点（递归）
    std::vector<NodeId> GetUpstreamNodes(NodeId nodeId) const;

    // 获取下游节点（递归）
    std::vector<NodeId> GetDownstreamNodes(NodeId nodeId) const;

    // 获取错误信息
    const std::string& GetLastError() const { return m_lastError; }

    // 设置日志回调
    void SetLogCallback(std::function<void(const std::string&)> callback);

    // 重置执行状态（保留蓝图数据和处理器注册）
    void ResetState();

    // ------------------------------------------------------------------
    // 主线程计时器（由外部每帧调用 Tick 驱动）
    // ------------------------------------------------------------------

    // 每帧调用，驱动计时器
    void Tick(float deltaTime) { m_timerManager.Tick(deltaTime); }

    // 获取计时器管理器（可读写）
    // 如果设置了父 timer manager，则使用父级的（子蓝图场景）
    FrameTimerManager&       GetTimerManager()       { return m_parentTimerManager ? *m_parentTimerManager : m_timerManager; }
    const FrameTimerManager& GetTimerManager() const { return m_parentTimerManager ? *m_parentTimerManager : m_timerManager; }

    // 设置父级 timer manager（用于子蓝图场景：子蓝图的 timer 注册到父 runner 的 manager 中）
    void SetParentTimerManager(FrameTimerManager* parent) { m_parentTimerManager = parent; }

    // 通过输入引脚ID找到连接的源节点并执行（用于 Function/Delegate 引脚的异步回调触发）
    // 例如 SetTimer 的 Function Name 引脚连接了一个回调节点，timer 触发时调用此方法
    bool FireConnectedNode(PinId inputPinId);

    // 保持子蓝图 runner 存活（直到其所有异步 timer 完成）
    // 子蓝图的 Delay/SetTimer 回调引用了 subRunner 的 context，
    // 需要确保 subRunner 在回调期间不被销毁
    void KeepAlive(std::shared_ptr<BlueprintRunner> subRunner)
    {
        m_keepAliveRunners.push_back(std::move(subRunner));
    }

private:
    friend class ExecutionContext;

    // 蓝图数据
    BlueprintData                                       m_blueprint;
    bool                                                m_loaded = false;

    // 节点处理器注册表
    std::unordered_map<std::string, NodeHandler>        m_handlers;
    NodeHandler                                         m_defaultHandler;

    // 执行上下文
    ExecutionContext                                    m_context;

    // 错误信息
    std::string                                         m_lastError;

    // 日志回调
    std::function<void(const std::string&)>             m_logCallback;

    // 主线程计时器管理器
    FrameTimerManager                                   m_timerManager;

    // 父级 timer manager（子蓝图场景）
    FrameTimerManager*                                  m_parentTimerManager = nullptr;

    // 保持子蓝图 runner 存活的容器
    std::vector<std::shared_ptr<BlueprintRunner>>       m_keepAliveRunners;

    // 缓存：拓扑排序结果
    mutable std::vector<NodeId>                         m_topoCache;
    mutable bool                                        m_topoCacheDirty = true;

    // 内部方法
    bool buildTopologicalOrder(std::vector<NodeId>& order) const;
    void propagatePinValues(const NodeInstance& node);
    void prepareNodeContext(const NodeInstance& node);
    bool executeNodeInternal(const NodeInstance& node);

    // 控制流：从指定输出引脚执行其连接的下游子图（拓扑序）
    bool executeDownstreamFromPin(PinId outputPinId);

    // 控制流已执行节点集合：记录被 ActivateOutputFlow 递归执行过的节点，
    // 主循环跳过这些节点以避免重复执行
    std::unordered_set<NodeId>                          m_flowExecutedNodes;

    // 递归深度保护
    int m_flowDepth = 0;
    static const int kMaxFlowDepth = 256;
};

} // namespace Runtime
} // namespace NodeEditor
