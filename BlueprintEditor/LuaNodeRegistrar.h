// BlueprintEditor/LuaNodeRegistrar.h
// Phase 3：编辑器侧 Lua 节点注册器
//
// 职责：
//   · 向 Lua 暴露 Blueprint.RegisterNode() API
//   · 将 Lua 脚本中定义的节点模板注册到 INodeRegistry
//   · 支持热重载（重新加载脚本 → 清理旧定义 → 注册新定义）
//   · 同时加载到 runner（RegisterHandler）+ 编辑器（RegisterNode）
//
// 条件编译：仅在 BLUEPRINT_HAS_LUA 定义时提供完整实现
//
#pragma once

#include "BlueprintEditor.h"

#ifdef BLUEPRINT_HAS_LUA

#include <string>
#include <vector>
#include <unordered_set>

struct lua_State;

namespace NodeEditor { namespace Runtime { class INodeRegistry; } }

// ============================================================================
// LuaNodeRegistrar — 编辑器侧 Lua 扩展管理器
// ============================================================================
//
// 生命周期与 BlueprintEditor 相同（作为成员变量持有）
// 管理：
//   - 已加载的 Lua 脚本列表（按加载顺序）
//   - 由这些脚本注册进来的 definitionId 集合（用于热重载时撤销）
//
class LuaNodeRegistrar
{
public:
    LuaNodeRegistrar() = default;
    ~LuaNodeRegistrar() = default;

    // 禁止拷贝
    LuaNodeRegistrar(const LuaNodeRegistrar&) = delete;
    LuaNodeRegistrar& operator=(const LuaNodeRegistrar&) = delete;

    // ── 初始化 ────────────────────────────────────────────────────────────
    // 必须在使用前调用，绑定注册表和 handler 表
    void Initialize(NodeEditor::Runtime::INodeRegistry* registry,
                    std::unordered_map<std::string, NodeEditor::Runtime::NodeHandler>* handlerMap);

    // ── 加载脚本 ─────────────────────────────────────────────────────────
    // 加载 Lua 脚本文件（同时注册 handler + 节点定义）
    // 返回：注册成功的节点数量，失败返回 -1
    int LoadScript(const std::string& filePath);

    // 加载 Lua 代码字符串（主要用于测试）
    int LoadString(const std::string& code, const std::string& chunkName = "=string");

    // ── 热重载 ──────────────────────────────────────────────────────────
    // 重新加载所有已注册脚本文件（清除旧定义 → 重新加载）
    // 返回：成功重载的文件数
    int ReloadAll();

    // 重新加载单个脚本文件（需要已在加载列表中）
    // 返回：注册节点数，失败返回 -1
    int ReloadFile(const std::string& filePath);

    // ── 查询 ──────────────────────────────────────────────────────────
    const std::vector<std::string>& GetLoadedFiles() const { return m_loadedFiles; }
    const std::unordered_set<std::string>& GetRegisteredIds() const { return m_registeredIds; }
    const std::string& GetLastError() const { return m_lastError; }
    bool IsInitialized() const { return m_registry != nullptr; }

    // ── 清理 ──────────────────────────────────────────────────────────
    // 从 registry 中移除所有由本 registrar 注册的节点定义
    void UnregisterAll();

    // 检查文件修改时间，若有变化则自动热重载
    // 由编辑器 Tick() 每帧或每秒调用
    void PollFileChanges();

    // 供 l_registerNode 回调使用（内部用）
    std::unordered_set<std::string>& GetRegisteredIds_Mutable();
    void IncrPendingCount();

    // 启用/禁用自动热重载（默认启用）
    void SetAutoReload(bool enable) { m_autoReload = enable; }
    bool GetAutoReload() const { return m_autoReload; }

private:
    // 内部：创建/复用 lua_State，注册编辑器绑定
    bool ensureLuaState();

    // 内部：在已有 lua_State 上执行一个脚本文件，收集 RegisterNode 调用
    int executeFile(const std::string& filePath);

    // 内部：在已有 lua_State 上执行代码字符串
    int executeString(const std::string& code, const std::string& chunkName);

    // 内部：注册 Blueprint.RegisterNode Lua API（附加在 Blueprint 全局表上）
    void registerEditorBindings(lua_State* L);

    // 内部：清理当前 lua_State，重新创建
    void resetLuaState();

    NodeEditor::Runtime::INodeRegistry*                           m_registry    = nullptr;
    std::unordered_map<std::string, NodeEditor::Runtime::NodeHandler>* m_handlerMap = nullptr;

    lua_State*                     m_L              = nullptr;
    std::vector<std::string>       m_loadedFiles;           // 已加载文件（按顺序）
    std::unordered_set<std::string> m_registeredIds;        // 本 registrar 注册的 definitionId
    std::string                    m_lastError;

    // 热重载：文件最后修改时间缓存（path → last_write_time as int64）
    std::unordered_map<std::string, int64_t> m_fileModTimes;
    bool   m_autoReload     = true;
    float  m_pollIntervalSec = 1.0f;   // 每 1 秒检查一次
    float  m_pollAccum       = 0.0f;   // 累积时间

    // 用于 RegisterNode 收集：每次 executeFile/executeString 期间
    // lua 回调会写入此成员（不用传指针）
    int    m_pendingCount    = 0;  // 本次加载中注册的节点数
};

#else // !BLUEPRINT_HAS_LUA

// 无 Lua 支持时的空桩，保持编译兼容
class LuaNodeRegistrar
{
public:
    void Initialize(void*, void*) {}
    int  LoadScript(const std::string&) { return 0; }
    int  LoadString(const std::string&, const std::string& = "") { return 0; }
    int  ReloadAll() { return 0; }
    int  ReloadFile(const std::string&) { return 0; }
    void UnregisterAll() {}
    void PollFileChanges(float = 0) {}
    void SetAutoReload(bool) {}
    bool GetAutoReload() const { return false; }
    bool IsInitialized() const { return false; }
    const std::string& GetLastError() const { static std::string s = "Lua not enabled"; return s; }
    const std::vector<std::string>& GetLoadedFiles() const { static std::vector<std::string> v; return v; }
};

#endif // BLUEPRINT_HAS_LUA
