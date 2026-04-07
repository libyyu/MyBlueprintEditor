// Runtime/BlueprintRunner.h - 蓝图运行时执行器
// 该模块完全独立于编辑器(ImGui)，可在任意环境中加载和执行蓝图数据
//
// 使用方式:
//   1. 编辑器导出 JSON 文件
//   2. 其他环境加载该 JSON
//   3. 注册节点处理器（每种 definitionId 对应一个执行函数）
//   4. 调用 Execute() 按拓扑顺序执行所有节点

#pragma once
#include "BlueprintExport.h"

// MSVC C4251: 'member': class 'std::...' needs to have dll-interface
// Safe to suppress when DLL and consumer share the same CRT/compiler.
#ifdef _MSC_VER
#   pragma warning(push)
#   pragma warning(disable: 4251)
#endif

#include "BlueprintData.h"
#include "NodeDefinition.h"
#include "FrameTimerManager.h"
#include "FileSystem.h"
#ifdef BLUEPRINT_HAS_LUA
#include "LuaScriptEngine.h"
#endif
#include <string>
#include <vector>
#include <unordered_map>
#include <functional>
#include <memory>
#include <queue>
#include <atomic>

// 前置声明（编辑器类在全局命名空间）
struct BlueprintEditor;
#include <algorithm>
#include <unordered_set>
#include <thread>
#include <chrono>

namespace NodeEditor {
namespace Runtime {

// ============================================================================
// 前向声明
// ============================================================================
class BlueprintRunner;
#ifdef BLUEPRINT_HAS_LUA
class LuaScriptEngine;
#endif

// ============================================================================
// NodeExecutionState —— 单次节点执行的纯数据上下文
//
// 职责：存储一次节点执行所需的所有可变状态
//   · 引脚值（输入/输出共享，通过链接传播）
//   · 引脚名→ID 映射（每次执行节点前重建）
//   · 当前节点指针及其自定义数据
//   · 蓝图级别的变量
//   · 触发当前节点的输入引脚 ID
//   · 蓝图元数据
//
// 设计原则：
//   · 无行为（无 Timer/ActivateOutputFlow 等）
//   · 无对 BlueprintRunner 的反向引用
//   · 可独立构造，便于单元测试
// ============================================================================

struct NodeExecutionState  // 不加 BLUEPRINT_API：通过 ExecutionContext 方法访问，无需直接 DLL 导出
{
    // 所有引脚的当前值（输入和输出共用，通过链接传播）
    std::unordered_map<PinId, Variant>              pinValues;

    // 当前节点的引脚名到ID的映射（每次执行节点前重建）
    // 注意：输入/输出分开，防止同名引脚（如 "JSON" 同时作为 in/out）互相覆盖
    std::unordered_map<std::string, PinId>          pinNameToId;         // 兼容旧代码（输出优先）
    std::unordered_map<std::string, PinId>          inputPinNameToId;    // 仅输入引脚
    std::unordered_map<std::string, PinId>          outputPinNameToId;   // 仅输出引脚

    // 当前节点的自定义数据
    std::unordered_map<std::string, Variant>        nodeData;

    // 蓝图级别的变量
    std::unordered_map<std::string, Variant>        variables;

    // 当前正在执行的节点指针（不拥有，生命周期由 BlueprintData 保证）
    const NodeInstance*                             currentNode = nullptr;

    // 触发当前节点执行的输入引脚 ID
    // 用于区分 DoN 的 Enter/Reset、Gate 的 Enter/Open/Close/Toggle 等
    PinId                                           activatedInputPinId = InvalidPinId;

    // 蓝图元数据（名称、版本等）
    BlueprintMetadata                               metadata;
};

// ============================================================================
// ExecutionContext —— 节点处理函数的执行上下文（公开 API 层）
//
// 职责：
//   · 数据访问：通过 NodeExecutionState 提供引脚读写、变量读写等
//   · 行为触发：Timer、控制流激活（ActivateOutputFlow）、日志
//   · 节点状态查询：当前节点、激活引脚、元数据等
//
// 与 NodeExecutionState 的关系：
//   · ExecutionContext 持有 NodeExecutionState 的引用（由 BlueprintRunner 拥有）
//   · handler 只通过 ExecutionContext 访问状态，不直接操作 NodeExecutionState
// ============================================================================

class BLUEPRINT_API ExecutionContext
{
public:
    // ------------------------------------------------------------------
    // 数据层：引脚 / 变量 / 节点状态
    // ------------------------------------------------------------------

    // 获取输入引脚的值（按 ID）
    Variant GetInputValue(PinId pinId) const
    {
        auto it = m_state->pinValues.find(pinId);
        return (it != m_state->pinValues.end()) ? it->second : Variant();
    }

    // 获取输入引脚的值（按名称，优先在输入引脚 map 中搜索，降级到全量 map）
    Variant GetInputValue(const std::string& pinName) const
    {
        // 优先从 inputPinNameToId 查，避免与同名 output 引脚冲突
        auto it = m_state->inputPinNameToId.find(pinName);
        if (it != m_state->inputPinNameToId.end())
            return GetInputValue(it->second);
        // 降级兼容：旧节点可能只注册了 pinNameToId
        auto it2 = m_state->pinNameToId.find(pinName);
        if (it2 != m_state->pinNameToId.end())
            return GetInputValue(it2->second);
        return Variant();
    }

    // 设置输出引脚的值（按 ID）
    void SetOutputValue(PinId pinId, const Variant& value)
    {
        m_state->pinValues[pinId] = value;
    }

    // 设置输出引脚的值（按名称，优先在输出引脚 map 中搜索，降级到全量 map）
    void SetOutputValue(const std::string& pinName, const Variant& value)
    {
        // 优先从 outputPinNameToId 查，避免与同名 input 引脚冲突
        auto it = m_state->outputPinNameToId.find(pinName);
        if (it != m_state->outputPinNameToId.end()) {
            SetOutputValue(it->second, value);
            return;
        }
        // 降级兼容
        auto it2 = m_state->pinNameToId.find(pinName);
        if (it2 != m_state->pinNameToId.end())
            SetOutputValue(it2->second, value);
    }

    // 获取当前节点指定引脚名对应的 PinId（优先输出引脚，用于 ActivateOutputFlow）
    PinId GetPinId(const std::string& pinName) const
    {
        // exec output 引脚优先（用于 ActivateOutputFlow）
        auto it = m_state->outputPinNameToId.find(pinName);
        if (it != m_state->outputPinNameToId.end()) return it->second;
        auto it2 = m_state->inputPinNameToId.find(pinName);
        if (it2 != m_state->inputPinNameToId.end()) return it2->second;
        // 兼容旧路径
        auto it3 = m_state->pinNameToId.find(pinName);
        return (it3 != m_state->pinNameToId.end()) ? it3->second : InvalidPinId;
    }

    // 获取节点自定义数据
    Variant GetNodeData(const std::string& key) const
    {
        auto it = m_state->nodeData.find(key);
        return (it != m_state->nodeData.end()) ? it->second : Variant();
    }

    // 获取蓝图变量
    Variant GetVariable(const std::string& name) const
    {
        auto it = m_state->variables.find(name);
        return (it != m_state->variables.end()) ? it->second : Variant();
    }

    // 设置蓝图变量
    void SetVariable(const std::string& name, const Variant& value)
    {
        m_state->variables[name] = value;
    }

    // 获取当前正在执行的节点
    const NodeInstance* GetCurrentNode() const { return m_state->currentNode; }

    // 获取蓝图元数据
    const BlueprintMetadata& GetMetadata() const { return m_state->metadata; }

    // 获取触发当前节点执行的输入引脚 ID
    PinId GetActivatedInputPinId() const { return m_state->activatedInputPinId; }

    // 获取触发当前节点的输入引脚名称
    std::string GetActivatedInputPinName() const
    {
        if (m_state->activatedInputPinId == InvalidPinId) return "";
        if (!m_state->currentNode) return "";
        for (const auto& pin : m_state->currentNode->pins)
        {
            if (pin.id == m_state->activatedInputPinId)
                return pin.name;
        }
        return "";
    }

    // ------------------------------------------------------------------
    // 行为层：调试日志（仅用于运行时内部诊断，受 loggingEnabled 开关控制）
    // ------------------------------------------------------------------

    /// Called for internal debug/diagnostic messages.
    /// Gated by loggingEnabled – silent when false (default in Release builds).
    /// Signature: (level, message)
    std::function<void(LogLevel, const std::string&)> OnLog;

    /// Master switch for internal debug logging.
    /// Default: OFF in release (NDEBUG defined), ON otherwise.
#if defined(NDEBUG)
    bool loggingEnabled = false;
#else
    bool loggingEnabled = true;
#endif

    void Log(const std::string& message, LogLevel level = LogLevel::Verbose) const;
    void LogWarning(const std::string& message) const { Log(message, LogLevel::Warning); }
    void LogError  (const std::string& message) const { Log(message, LogLevel::Error);   }

    // ------------------------------------------------------------------
    // 行为层：Print（逻辑输出，第三方引擎可接管）
    // ------------------------------------------------------------------

    /// Called by PrintString / Log / FormatLog / Assert nodes.
    /// Independent of loggingEnabled – always fires when set.
    /// Signature: (level, message)
    /// If not set, falls back to OnLog (if loggingEnabled), then stderr.
    std::function<void(LogLevel, const std::string&)> OnPrint;

    void Print(const std::string& message, LogLevel level = LogLevel::Info) const;
    void PrintWarning(const std::string& message) const { Print(message, LogLevel::Warning); }
    void PrintError  (const std::string& message) const { Print(message, LogLevel::Error);   }

    // ------------------------------------------------------------------
    // 行为层：Timer
    // ------------------------------------------------------------------

    TimerHandle Delay(float seconds, const std::function<void()>& callback);

    TimerHandle SetTimer(float delay, TimerCallback callback);

    // 完整版：指定间隔、重复次数（-1=无限循环）
    TimerHandle SetTimer(float interval, int repeatCount, TimerCallback callback);

    // 带名称版：可通过名称查找/取消
    TimerHandle SetTimerByName(const std::string& name, float interval, int repeatCount, TimerCallback callback);

    // ------------------------------------------------------------------
    // 行为层：异步调度
    // ------------------------------------------------------------------

    // 跨平台异步执行框架
    //
    // dispatcher(resolve) 负责发起异步操作，完成时调用 resolve()。
    // onComplete(ctx)     在主线程 Tick/DrainQueue 上下文中执行，可安全访问蓝图状态。
    //
    // 语义在所有平台完全一致：
    //   - 非 Emscripten：dispatcher 在后台线程中调用；resolve() 将 onComplete Post 到
    //                     MainThreadDispatcher，由 Tick() → DrainQueue() 消费。
    //   - Emscripten：    dispatcher 在当前（主）线程中调用；调用方在 dispatcher 内用
    //                     emscripten_fetch 等原生异步 API，完成回调里调 resolve()；
    //                     resolve() 同样 Post 到 MainThreadDispatcher，由 Tick() 消费。
    //                     这样无论哪个平台，onComplete 都不在 dispatcher 内同步执行，
    //                     避免了 WebGL 下的重入风险。
    //
    // 规则：
    //   - dispatcher 不能访问 ExecutionContext / 任何蓝图状态（线程不安全）
    //   - onComplete 在主线程 Tick 期间执行，可安全访问 ctx
    //   - AcquireAsync / ReleaseAsync 由 RunAsync 内部自动管理，无需手动调用
    //
    // 旧签名（background, onComplete）已废弃，保留为重载以兼容现有调用方，
    // 内部桥接到新签名（background 包装成 dispatcher，不使用 resolve）。
    using AsyncResolve = std::function<void()>;
    using AsyncDispatcher = std::function<void(AsyncResolve resolve)>;

    void RunAsync(
        AsyncDispatcher                          dispatcher,
        std::function<void(ExecutionContext&)>   onComplete);

    // 兼容旧签名：background() 在后台线程同步执行后自动 resolve
    // WebGL 下等价于同步执行 background() + 将 onComplete Post 到下一帧
    void RunAsync(
        std::function<void()>                    background,
        std::function<void(ExecutionContext&)>   onComplete);

    // ------------------------------------------------------------------
    // 行为层：控制流
    // ------------------------------------------------------------------

    // 按引脚名称激活输出流（执行连接到该输出 exec 引脚的所有下游节点）
    bool ActivateOutputFlow(const std::string& pinName);

    // 按引脚 ID 激活输出流
    bool ActivateOutputFlow(PinId pinId);

    // 重新执行 pinName 对应输入引脚的上游数据节点，然后返回引脚值的 bool 结果。
    // 用于 WhileLoop 等需要每次迭代重新求值条件的节点。
    bool EvaluateConditionPin(const std::string& pinName);

    // 标记指定输出引脚的所有下游节点为"已被控制流接管"，
    // 主循环会跳过这些节点。用于异步节点（如 Delay）预先占位。
    void MarkDownstreamAsHandled(const std::string& pinName);
    void MarkDownstreamAsHandled(PinId pinId);

    // 请求暂停当前 runner（供调试节点使用）
    void PauseRunner();

    // 通过输入引脚 ID 找到连接的源节点并执行
    bool FireConnectedNode(PinId inputPinId);

private:
    friend class BlueprintRunner;

    // 内部辅助：将用户回调包装为带 context save/restore 的 TimerCallback
    TimerCallback wrapCallbackWithContextRestore(TimerCallback callback);

    // 数据层：由 BlueprintRunner 拥有，ExecutionContext 持有指针（不拥有）
    // 使用指针而非引用，以便 BlueprintRunner 可以默认构造 ExecutionContext
    NodeExecutionState*  m_state  = nullptr;

    // 所属 runner（行为层回调用；生命周期由 BlueprintRunner 保证）
    BlueprintRunner*     m_runner = nullptr;
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
    std::vector<NodeId>         executedNodeIds;        // 实际执行的节点 ID 列表（按顺序）

    // 获取最终的输出值（通过引脚ID）
    std::unordered_map<PinId, Variant> outputValues;
};

// ============================================================================
// 蓝图运行时执行器
// ============================================================================

class BLUEPRINT_API BlueprintRunner
{
public:
    BlueprintRunner()
        : m_fileSystem(GetDefaultFileSystem())
        , m_timerManager(std::make_shared<FrameTimerManager>())
        , m_alive(std::make_shared<std::atomic<bool>>(true))
    {}
    explicit BlueprintRunner(std::shared_ptr<IFileSystem> fs)
        : m_fileSystem(fs ? std::move(fs) : GetDefaultFileSystem())
        , m_timerManager(std::make_shared<FrameTimerManager>())
        , m_alive(std::make_shared<std::atomic<bool>>(true))
    {}
    ~BlueprintRunner();

    // 获取存活标志（用于异步回调安全检查）
    std::shared_ptr<std::atomic<bool>> GetAliveFlag() const { return m_alive; }

    // 获取/设置文件系统
    std::shared_ptr<IFileSystem> GetFileSystem() const { return m_fileSystem; }
    void SetFileSystem(std::shared_ptr<IFileSystem> fs) { m_fileSystem = fs ? std::move(fs) : GetDefaultFileSystem(); }

    bool IsWithEditor() const;

    // ------------------------------------------------------------------
    // 加载蓝图数据
    // ------------------------------------------------------------------

    // 从 BlueprintData 直接加载
    bool Load(const BlueprintData& data);

    // 从 JSON 字符串加载（不加载依赖）
    bool LoadFromJson(const std::string& jsonContent);

    // 从 JSON 字符串加载，并按 metadata.dependencies 自动加载依赖 Library
    // baseDir: 依赖文件的搜索基准目录（相对路径将相对于此目录解析）
    bool LoadFromJsonWithDeps(const std::string& jsonContent, const std::string& baseDir);

    // 从 JSON 文件加载
    bool LoadFromFile(const std::string& filePath);

    // 从文件加载，并按 metadata.dependencies 自动加载依赖 Library（Runtime 无需工程文件）
    bool LoadFromFileWithDeps(const std::string& filePath);

    // 获取已加载的蓝图数据
    const BlueprintData& GetBlueprintData() const { return m_blueprint; }

    // 是否已加载
    bool IsLoaded() const { return m_loaded; }

    // ------------------------------------------------------------------
    // 脚本动态节点定义注册（Lua / C# 共用）
    // ------------------------------------------------------------------

    // 注册一个节点定义（同时对 Lua 和 C# 脚本开放）
    // 重复注册同一 id 会覆盖旧定义
    void RegisterNodeDef(const NodeDefinition& def);

    // 注销一个节点定义
    void UnregisterNodeDef(const std::string& id);

    // 检查节点定义是否已注册
    bool HasNodeDef(const std::string& id) const;

    // 获取节点定义（未注册返回 nullptr）
    const NodeDefinition* GetNodeDef(const std::string& id) const;

    // 获取脚本注册表（编辑器/外部合并用）
    INodeRegistry& GetScriptRegistry() { return m_scriptRegistry; }
    const INodeRegistry& GetScriptRegistry() const { return m_scriptRegistry; }

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

    // 触发指定事件（通过 definitionId 找到事件源节点，执行其 exec 下游）
    // 例如：DispatchEvent("OnBeginPlay"), DispatchEvent("OnTick")
    // 若蓝图中没有对应的事件源节点，返回 success=true（静默忽略）
    ExecutionResult DispatchEvent(const std::string& eventDefinitionId);

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

    // 注册外部库函数（供编辑器在 Load 后注入工程库函数，用于 FuncLib.* 节点执行）
    void RegisterExternalFunctions(const std::vector<FunctionDefinition>& funcs);
    void RegisterExternalFunction(const FunctionDefinition& func);

    // 创建子 runner，继承当前 runner 的全部配置（handlers/callbacks/timer/external libs）
    // 用于 Function.Call / Function.CallLibrary / FuncLib.* 三条路径，消除重复代码
    // 返回 unique_ptr 以避免拷贝构造（BlueprintRunner 含 atomic，不可拷贝）
    std::unique_ptr<BlueprintRunner> CreateChildRunner() const;

    // 获取已注册的外部库函数表（供子 runner 继承，确保 FuncLib.* 节点在子蓝图中可用）
    std::vector<FunctionDefinition> GetExternalFunctions() const;

    // 注册完整函数库蓝图数据（含顶层节点图），供 FuncLib.* 节点执行时构建函数子图
    // 同一个库的多个函数共享同一个 shared_ptr<BlueprintData>，避免冗余拷贝
    void RegisterExternalLibrary(const BlueprintData& libData);

    // 获取完整函数库数据表（供子 runner 继承）
    const std::unordered_map<std::string, std::shared_ptr<BlueprintData>>& GetExternalLibraries() const
    { return m_externalLibraries; }

    // 将父 runner 的 externalLibraries 共享给子 runner（shared_ptr 共享，零拷贝）
    void InheritExternalLibraries(const std::unordered_map<std::string, std::shared_ptr<BlueprintData>>& libs)
    {
        for (const auto& kv : libs)
            m_externalLibraries[kv.first] = kv.second;
    }

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

    // ------------------------------------------------------------------
    // Lua 脚本扩展（需要 BLUEPRINT_HAS_LUA 编译选项）
    // ------------------------------------------------------------------
#ifdef BLUEPRINT_HAS_LUA

    // 加载 Lua 脚本文件并执行
    // 脚本中调用 Blueprint.RegisterHandler() 会自动注册 handler 到本 runner
    bool LoadLuaScript(const std::string& filePath);

    // 加载 Lua 代码字符串并执行
    bool LoadLuaString(const std::string& code, const std::string& name = "=string");

    // 获取 Lua 引擎实例（高级用途：注册自定义 C 函数等）
    LuaScriptEngine* GetLuaEngine();

    // 心跳 Tick：驱动 Lua 脚本的 onTick(deltaSeconds) 全局函数（若存在）。
    // 建议在游戏循环 / Editor 定时器里每帧或固定间隔调用。
    // deltaSeconds：距上次调用的秒数。
    void TickLua(double deltaSeconds);

#endif // BLUEPRINT_HAS_LUA

    // ------------------------------------------------------------------
    // 日志 / 输出配置
    // ------------------------------------------------------------------

    /// Log a message through the runner's log callback (if set and logging enabled).
    /// Can be called outside of ExecutionContext (e.g., during Lua script loading).
    void Log(const std::string& message, LogLevel level = LogLevel::Verbose) const;
    void LogWarning(const std::string& message) const { Log(message, LogLevel::Warning); }
    void LogError  (const std::string& message) const { Log(message, LogLevel::Error);   }

    /// Set callback for internal debug/diagnostic messages (LogLevel, message).
    /// Has no effect when logging is disabled (see EnableLogging).
    void SetLogCallback(std::function<void(LogLevel, const std::string&)> callback);

    /// Enable or disable internal debug logging (OnLog).
    /// Default: OFF when NDEBUG is defined (Release), ON otherwise.
    void EnableLogging(bool enable) { m_context.loggingEnabled = enable; }
    bool IsLoggingEnabled() const   { return m_context.loggingEnabled; }

    /// Set callback for PrintString / Log / FormatLog node output (LogLevel, message).
    /// This is the "application-level" print channel that engines like Unity
    /// should override.  Independent of EnableLogging.
    void SetPrintCallback(std::function<void(LogLevel, const std::string&)> callback);

    /// Set callback called BEFORE each node executes. If callback returns true, execution pauses at that node.
    /// nodeId is the runtime NodeId (same value as editor node's ID.AsPointer()).
    void SetNodePreExecuteCallback(std::function<bool(NodeId)> callback);

    // 重置执行状态（保留蓝图数据和处理器注册）
    void ResetState();

    // ------------------------------------------------------------------
    // 运行时控制（Stop / Pause / Resume）
    // ------------------------------------------------------------------

    // 停止执行：清空所有计时器，重置到 Idle 状态
    // 安全地从任意线程调用；正在执行的节点会在当前节点完成后停止
    void Stop();

    // 暂停：Tick 不再推进计时器（但不清除它们）
    void Pause();

    // 恢复：继续 Tick 推进计时器
    void Resume();

    // 状态查询
    bool IsRunning() const { return m_runState.load() == RunState::Running; }
    bool IsPaused()  const { return m_runState.load() == RunState::Paused;  }
    bool IsStopped() const { return m_runState.load() == RunState::Stopped; }
    bool IsIdle()    const { return m_runState.load() == RunState::Idle;    }

    // ── 断点单步调试 ─────────────────────────────────────────────────────────
    // 在 Paused 状态下执行下一个拓扑节点，然后再次 Pause。
    // 返回 true = 成功执行了一个节点；false = 无更多节点（执行完毕）。
    bool StepNextNode();

    // 获取拓扑排序缓存（供编辑器在 Step 后高亮节点使用）
    const std::vector<NodeId>& GetTopoCache() const { return m_topoCache; }
    size_t GetStepTopoIndex() const { return m_stepTopoIndex; }

    // 返回下一步将要执行的节点 id（暂停时有效）
    // 优先从 m_stepPendingNodes 中按拓扑序取最靠前的节点，
    // 没有则返回 topo[m_stepTopoIndex]（兜底路径），0 表示无
    NodeId GetNextStepNodeId() const;

    // 返回上一次 StepNextNode() 实际执行的节点 id（0 表示未执行过）
    NodeId GetLastSteppedNodeId() const { return m_lastSteppedNodeId; }

    // ── Pin 快照（调试/可视化）──────────────────────────────────────────────
    // 启用后，每个节点执行完毕时自动记录所有输入/输出引脚的当前值快照。
    // 编辑器可读取这些值，在节点上方悬停显示"上次执行时的引脚值"。

    struct PinSnapshot {
        Variant                               value;
        std::chrono::steady_clock::time_point timestamp;
    };
    using NodePinMap = std::unordered_map<std::string, PinSnapshot>;

    // 开关（默认关闭，避免生产运行时内存开销）
    void  SetSnapshotEnabled(bool enabled) { m_snapshotEnabled = enabled; }
    bool  IsSnapshotEnabled()  const       { return m_snapshotEnabled;    }

    // 清空全部快照
    void ClearSnapshots()
    {
        std::lock_guard<std::mutex> lk(m_snapshotMutex);
        m_pinSnapshots.clear();
    }

    // 读取某节点的全部引脚快照（线程安全拷贝）
    NodePinMap GetNodePinSnapshot(NodeId nodeId) const
    {
        std::lock_guard<std::mutex> lk(m_snapshotMutex);
        auto it = m_pinSnapshots.find(nodeId);
        return it != m_pinSnapshots.end() ? it->second : NodePinMap{};
    }

    // 快速读取单个引脚值（找不到返回空 Variant）
    Variant GetSnapshotPinValue(NodeId nodeId, const std::string& pinName) const
    {
        std::lock_guard<std::mutex> lk(m_snapshotMutex);
        auto nit = m_pinSnapshots.find(nodeId);
        if (nit == m_pinSnapshots.end()) return {};
        auto pit = nit->second.find(pinName);
        return pit != nit->second.end() ? pit->second.value : Variant{};
    }

    // 返回所有有快照的节点 id 列表
    std::vector<NodeId> GetSnapshotNodeIds() const
    {
        std::lock_guard<std::mutex> lk(m_snapshotMutex);
        std::vector<NodeId> ids;
        ids.reserve(m_pinSnapshots.size());
        for (const auto& kv : m_pinSnapshots) ids.push_back(kv.first);
        return ids;
    }

    // ------------------------------------------------------------------
    // 主线程计时器（由外部每帧调用 Tick 驱动）
    // ------------------------------------------------------------------

    // 每帧调用，驱动计时器
    void Tick(float deltaTime);

    // 获取计时器管理器（可读写）
    // 如果设置了父 timer manager，优先使用父级（子蓝图场景）；
    // 父已析构时 weak_ptr lock 失败，自动回退到自身的 m_timerManager。
    FrameTimerManager& GetTimerManager()
    {
        if (auto p = m_parentTimerManager.lock()) return *p;
        return *m_timerManager;
    }
    const FrameTimerManager& GetTimerManager() const
    {
        if (auto p = m_parentTimerManager.lock()) return *p;
        return *m_timerManager;
    }

    // 设置父级 timer manager（用于子蓝图场景）
    // 使用 shared_ptr，子 runner 以 weak_ptr 持有，不延长父的生命周期；
    // 父析构后 weak_ptr 自动失效，GetTimerManager() 回退到自身 manager。
    void SetParentTimerManager(std::shared_ptr<FrameTimerManager> parent)
    {
        m_parentTimerManager = std::move(parent);
    }

    // 获取自身 timer manager 的 shared_ptr（供子蓝图调用 SetParentTimerManager）
    std::shared_ptr<FrameTimerManager> GetTimerManagerPtr() { return m_timerManager; }

    // 通过输入引脚ID找到连接的源节点并执行（用于 Function/Delegate 引脚的异步回调触发）
    // 例如 SetTimer 的 Function Name 引脚连接了一个回调节点，timer 触发时调用此方法
    bool FireConnectedNode(PinId inputPinId);

    // 保持子蓝图 runner 存活（直到其所有异步操作完成）
    // 子蓝图的异步回调（timer、网络等）引用了 subRunner 的 context，
    // 需要确保 subRunner 在所有回调期间不被销毁。
    // Tick() 会在 subRunner->HasPendingAsync() == false 时自动释放。
    void KeepAlive(std::shared_ptr<BlueprintRunner> subRunner)
    {
        m_keepAliveRunners.push_back(std::move(subRunner));
    }

    // ----------------------------------------------------------------
    // 通用异步计数 API
    // 任何异步操作（timer、网络、IO 等）开始时调用 AcquireAsync()，
    // 完成或取消时调用 ReleaseAsync()。
    // Tick() 依靠 HasPendingAsync() == false 来决定是否回收子 runner。
    // ----------------------------------------------------------------
    void AcquireAsync() { m_pendingAsyncCount.fetch_add(1, std::memory_order_acq_rel); }
    void ReleaseAsync()
    {
        int prev = m_pendingAsyncCount.fetch_sub(1, std::memory_order_acq_rel);
        if (prev <= 0)
            m_pendingAsyncCount.store(0, std::memory_order_release); // 防止减到负数
    }
    bool HasPendingAsync() const { return m_pendingAsyncCount.load(std::memory_order_acquire) > 0; }
    int  PendingAsyncCount() const { return m_pendingAsyncCount.load(std::memory_order_acquire); }

    // RAII 封装：构造时 Acquire，析构时 Release（支持移动，不可拷贝）
    struct AsyncGuard
    {
        explicit AsyncGuard(BlueprintRunner* runner) : m_runner(runner)
        {
            if (m_runner) m_runner->AcquireAsync();
        }
        ~AsyncGuard() { release(); }

        // 可移动（转让所有权）
        AsyncGuard(AsyncGuard&& other) noexcept : m_runner(other.m_runner)
        {
            other.m_runner = nullptr;
        }
        AsyncGuard& operator=(AsyncGuard&& other) noexcept
        {
            if (this != &other) { release(); m_runner = other.m_runner; other.m_runner = nullptr; }
            return *this;
        }

        // 不可拷贝
        AsyncGuard(const AsyncGuard&)            = delete;
        AsyncGuard& operator=(const AsyncGuard&) = delete;

        void release()
        {
            if (m_runner) { m_runner->ReleaseAsync(); m_runner = nullptr; }
        }

    private:
        BlueprintRunner* m_runner = nullptr;
    };

private:
    friend class ExecutionContext;
    friend struct ::BlueprintEditor;  // 仅编辑器可设置 m_withEditor

    // 是否在编辑器环境下运行
    bool                                                m_withEditor = false;

    // 文件系统抽象（用于文件读写）
    std::shared_ptr<IFileSystem>                        m_fileSystem;

    // 蓝图数据
    BlueprintData                                       m_blueprint;
    bool                                                m_loaded = false;

    // 依赖 Library 中注册的外部函数（LoadFromFileWithDeps 时填充）
    // key: funcDef.id, value: FunctionDefinition
    std::unordered_map<std::string, FunctionDefinition> m_externalFunctions;

    // 依赖 Library 的完整蓝图数据（LoadFromFileWithDeps 时填充）
    // key: funcDef.id, value: shared_ptr<BlueprintData>（同一个库的多个函数共享一份数据）
    // 用于 FuncLib.* / Function.Call 节点执行时从完整节点图中构建函数子图（而非空索引）
    std::unordered_map<std::string, std::shared_ptr<BlueprintData>> m_externalLibraries;

    // 节点处理器注册表
    std::unordered_map<std::string, NodeHandler>        m_handlers;
    NodeHandler                                         m_defaultHandler;

    // 脚本动态节点定义注册表（Lua / C# 通过 RegisterNodeDef 注册）
    DefaultNodeRegistry                                 m_scriptRegistry;

    // 数据层：节点执行状态（引脚值、变量、当前节点等纯数据）
    NodeExecutionState                                  m_state;

    // 行为层：执行上下文（handler 的公开 API，持有 m_state 的指针）
    ExecutionContext                                    m_context;

    // 错误信息
    std::string                                         m_lastError;

    // 调试日志回调 / 逻辑输出回调
    std::function<void(LogLevel, const std::string&)>   m_logCallback;
    std::function<void(LogLevel, const std::string&)>   m_printCallback;
    // 节点预执行回调（用于断点检测）：返回 true 则在此节点处暂停
    std::function<bool(NodeId)>                         m_nodePreExecuteCb;

    // 主线程计时器管理器（shared_ptr，可共享给子 runner）
    std::shared_ptr<FrameTimerManager>                  m_timerManager;

    // 父级 timer manager（weak_ptr：借用，不拥有；父析构后自动失效）
    std::weak_ptr<FrameTimerManager>                    m_parentTimerManager;

    // 存活标志：shared_ptr<atomic<bool>>，供异步回调检查 runner 是否已析构
    // 构造时为 true，析构时设为 false；异步回调持有 shared_ptr 副本，可安全判断
    std::shared_ptr<std::atomic<bool>>                  m_alive;
    std::string                                         m_loadedFileDir;  // LoadFromFile/WithDeps 记录的文件目录，供 Function.CallLibrary 解析相对路径

    // 保持子蓝图 runner 存活的容器
    std::vector<std::shared_ptr<BlueprintRunner>>       m_keepAliveRunners;

    // 正在进行的异步操作计数（timer、网络、IO 等）
    // AcquireAsync/ReleaseAsync 维护；Tick() 用于判断是否可回收
    // 使用 atomic：AcquireAsync/ReleaseAsync 可能从后台线程调用
    std::atomic<int>                                    m_pendingAsyncCount { 0 };

    // ── Pin 快照存储 ─────────────────────────────────────────────────────────
    bool                                                m_snapshotEnabled { false };
    mutable std::mutex                                  m_snapshotMutex;
    std::unordered_map<NodeId, NodePinMap>              m_pinSnapshots;

    // 运行时控制状态
    enum class RunState { Idle, Running, Paused, Stopped };
    std::atomic<RunState>                               m_runState { RunState::Idle };

    // 断点单步调试：记录当前已执行到拓扑序的第几个节点（Paused 时有效）
    size_t                                              m_stepTopoIndex = 0;
    // 断点命中时暂存的节点 id（由 executeDownstreamFromPin 设置，Execute 用于定位 stepTopoIndex）
    NodeId                                              m_pausedAtNodeId = 0;
    // 上一次 StepNextNode() 实际执行的节点 id（用于 UI 蓝色高亮）
    NodeId                                              m_lastSteppedNodeId = 0;
    // Step 模式标志：为 true 时 executeNodeInternal 跳过断点检测，让节点实际执行
    bool                                                m_bypassBreakpoint = false;
    // 数据依赖求值标志：防止 executeNodeInternal 递归求值死循环
    bool                                                m_evaluatingDataDeps = false;
    // 单步模式标志：StepNextNode 执行期间为 true
    // ActivateOutputFlow 在此模式下且处于顶层（m_flowDepth==0）时，
    // 只记录 exec 下游到 m_stepPendingNodes，不递归执行（让 StepNext 逐步进入）
    // ForLoop/Branch 等内部的嵌套调用（m_flowDepth>0）不受影响，正常执行
    bool                                                m_stepMode = false;
    // 单步模式下 exec 下游节点暂存集合：供下次 StepNextNode 优先执行
    std::unordered_set<NodeId>                          m_stepPendingNodes;

    // 缓存：拓扑排序结果
    mutable std::vector<NodeId>                         m_topoCache;
    mutable bool                                        m_topoCacheDirty = true;
    mutable bool                                        m_topoCacheHasCycle = false;

    // 缓存：事件子图集合（随拓扑缓存一起失效）
    mutable std::unordered_set<NodeId>                  m_eventSubgraphCache;
    mutable bool                                        m_eventSubgraphDirty = true;

    // 缓存：主循环可达节点集合（随拓扑缓存一起失效）
    // 从 topo 主循环入口节点出发，exec 向下游 + data 向上游 BFS 的可达集
    mutable std::unordered_set<NodeId>                  m_reachableCache;
    mutable bool                                        m_reachableDirty = true;

    // 缓存：FuncLib 函数子图（funcId → BlueprintData）
    // key = FunctionDefinition::id，value = BuildFuncSubGraph 构建的子图
    // ForLoop 内多次调用同一 FuncLib 函数时只构建一次，避免重复 BFS
    // invalidateTopoCache() 时同步清空（蓝图结构变更时失效）
    mutable std::unordered_map<std::string, BlueprintData> m_funcSubGraphCache;

    // 标记拓扑缓存失效（图结构变更时调用）
    void invalidateTopoCache()
    {
        m_topoCacheDirty     = true;
        m_eventSubgraphDirty = true;
        m_reachableDirty     = true;
        m_funcSubGraphCache.clear();  // 子图缓存随图结构失效
    }

    // 确保拓扑缓存有效，返回是否无环
    bool ensureTopologicalOrder() const;

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

    // flowInsertLog：按插入顺序记录加入 m_flowExecutedNodes 的节点。
    // 提升为成员变量，使嵌套的 executeDownstreamFromPin 调用共享同一序列，
    // 确保内层递归的插入对外层的 executedHere 更新可见。
    // 每次 Execute() / DispatchEvent() 开始时与 m_flowExecutedNodes 同步清空。
    std::vector<NodeId>                                 m_flowInsertLog;

    // 递归深度保护
    int m_flowDepth = 0;
    // 控制流递归深度上限（Branch/ForLoop/Sequence 每层 +1）。
    // 提高到 512 以支持合法的深层嵌套；超出时输出带节点信息的错误日志并终止执行。
    static const int kMaxFlowDepth = 512;

#ifdef BLUEPRINT_HAS_LUA
    // Lua 脚本引擎（延迟创建：首次 LoadLuaScript 时初始化）
    std::unique_ptr<LuaScriptEngine>                    m_luaEngine;
#endif

    // ── 私有辅助方法 ─────────────────────────────────────────────────────────

    // 遍历 deps 列表，按 baseDir 解析路径，加载 FunctionLibrary 并注册外部函数/库。
    // 返回 false 表示某个依赖加载失败，m_lastError 已写入错误信息。
    // 注意：仅在非 Emscripten 平台编译时有意义（内部使用 std::filesystem）。
    bool loadDependencies(const std::vector<std::string>& deps,
                          const std::string& baseDir);
};

} // namespace Runtime
} // namespace NodeEditor

#ifdef _MSC_VER
#   pragma warning(pop)
#endif
