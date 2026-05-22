// BlueprintEditor/ScriptExtensionManager.h
//
// 编辑器侧的"扩展脚本管理器"（薄包装层）。
//
// 设计原则：编辑器对脚本后端（Lua / 未来可能 JS / Python）保持透明。
// 这里只暴露通用概念：
//   · 加载 / 重载 / 卸载扩展脚本
//   · 扩展脚本搜索路径（解析 require 等用，由后端解释）
//   · 入口脚本监视（出现后自动加载）
//   · 帧 Tick（驱动后端的 OnGlobalTick）
//   · 文件 watch 热重载
//
// 所有调用最终委托给绑定的 BlueprintRunner 的 LoadExtensionScript /
// AddScriptSearchPath / TickScriptExtensions / ... 接口。
// Runtime 未启用脚本后端时，这些接口为空操作，行为安静退化。
//
// 用法：
//   1. 调用 BindRunner(runner) 绑定目标 runner
//   2. 调用 LoadScript / WatchEntryScript 加载脚本
//   3. 在编辑器帧循环中调用 PollFileChanges() 驱动热重载
//   4. 调用 Tick(dt) 驱动后端 OnGlobalTick
//
#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <functional>

namespace NodeEditor { namespace Runtime {
    class BlueprintRunner;
} }

// ============================================================================
// ScriptExtensionManager
// ============================================================================
class ScriptExtensionManager
{
public:
    ScriptExtensionManager() = default;
    ~ScriptExtensionManager() = default;

    ScriptExtensionManager(const ScriptExtensionManager&) = delete;
    ScriptExtensionManager& operator=(const ScriptExtensionManager&) = delete;

    // ── 绑定 Runner ──────────────────────────────────────────────────────
    // 必须在使用其他接口前调用。runner 生命周期由调用方保证。
    void BindRunner(NodeEditor::Runtime::BlueprintRunner* runner);

    // ── 脚本搜索路径（由后端按需解释，如 Lua: package.path）─────────────
    void AddSearchPath(const std::string& dir);

    // ── 入口脚本加载（静默，不存在则跳过）──────────────────────────────────
    bool LoadEntrySilent(const std::string& filePath, const std::string& chunkName = "");

    // ── 入口脚本监视（出现后自动加载，工程切换时重置） ───────────────────────
    void WatchEntryScript(const std::string& filePath, const std::string& chunkName = "");

    // ── 加载脚本文件（加入工程扩展脚本列表） ────────────────────────────────
    bool LoadScript(const std::string& filePath);

    // ── 热重载 ───────────────────────────────────────────────────────────
    bool ReloadAll();
    bool ReloadFile(const std::string& filePath);

    // ── 注销所有脚本注册的节点 ────────────────────────────────────────────
    void UnregisterAll();

    // ── 文件 Watch 轮询（每帧调用，传入 deltaTime 实现节流） ─────────────
    void PollFileChanges(float deltaTime = 0.0f);

    // ── 心跳 Tick（驱动后端 OnGlobalTick） ──────────────────────────────
    void Tick(float deltaTime);

    // ── 日志回调（编辑器控制台输出） ────────────────────────────────────────
    using LogCallback = std::function<void(int level, const std::string& msg)>;
    void SetLogCallback(LogCallback cb);
    const LogCallback& GetLogCallback() const { return m_logCallback; }

    // ── 查询 ─────────────────────────────────────────────────────────────
    bool               IsInitialized()  const { return m_runner != nullptr; }
    bool               GetAutoReload()  const { return m_autoReload; }
    void               SetAutoReload(bool v)  { m_autoReload = v; }
    const std::string& GetLastError()   const { return m_lastError; }
    const std::vector<std::string>& GetLoadedFiles() const { return m_loadedFiles; }

    // 兼容旧接口（ProjectOps 中用于工程加载时重新初始化）
    void Initialize(NodeEditor::Runtime::BlueprintRunner* runner) { BindRunner(runner); }

private:
    NodeEditor::Runtime::BlueprintRunner*   m_runner       = nullptr;
    LogCallback                             m_logCallback;
    std::string                             m_lastError;

    // 工程扩展脚本列表（工程文件中的 scriptExtensions）
    std::vector<std::string>                m_loadedFiles;
    // 文件修改时间（热重载用）
    std::unordered_map<std::string, int64_t> m_fileModTimes;

    bool    m_autoReload      = true;
    float   m_pollIntervalSec = 1.0f;
    float   m_pollAccum       = 0.0f;

    // 入口脚本监视列表（per-project 入口脚本）
    struct EntryWatch {
        std::string filePath;
        std::string chunkName;
        bool        loaded = false;
    };
    std::vector<EntryWatch> m_entryWatches;
};
