// BlueprintEditor/LuaNodeRegistrar.h
// Phase 3：编辑器侧 Lua 节点注册器
//
// 职责：
//   · 向 Lua 暴露 Blueprint.RegisterNode() API
//   · 将 Lua 脚本中定义的节点模板注册到 INodeRegistry
//   · 支持热重载（重新加载脚本 → 清理旧定义 → 注册新定义）
//
#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <functional>

// 前向声明（避免 include BlueprintEditor.h 造成循环依赖和 namespace 污染）
namespace NodeEditor { namespace Runtime {
    class INodeRegistry;
    class ExecutionContext;
    using NodeHandler = std::function<bool(ExecutionContext&)>;
} }

#ifdef BLUEPRINT_HAS_LUA

struct lua_State;
typedef int (*lua_CFunction)(lua_State*);  // matches lua.h definition

// ============================================================================
// LuaNodeRegistrar — 编辑器侧 Lua 扩展管理器
// ============================================================================
class LuaNodeRegistrar
{
public:
    LuaNodeRegistrar() = default;
    ~LuaNodeRegistrar();

    // 禁止拷贝
    LuaNodeRegistrar(const LuaNodeRegistrar&) = delete;
    LuaNodeRegistrar& operator=(const LuaNodeRegistrar&) = delete;

    // ── 初始化 ────────────────────────────────────────────────────────────
    void Initialize(NodeEditor::Runtime::INodeRegistry* registry,
                    std::unordered_map<std::string, NodeEditor::Runtime::NodeHandler>* handlerMap);

    // ── Lua 路径 / Searcher ───────────────────────────────────────────
    // 向 package.searchers[1] 插入自定义 loader（优先级最高）
    void SetSearcher(lua_CFunction loader);
    // 向 package.path 追加搜索目录（自动补 /?.lua;/?/init.lua）
    void AddLuaPath(const std::string& dir);

    // ── Entry Script 加载（直接 loadfile，不走 require，支持独立 chunkName）
    // 立即尝试加载 filePath；文件不存在或执行出错均静默忽略。
    // chunkName 用于区分同名脚本（建议用工程名，如 "MyGame:BlueprintEntry"）。
    void LoadEntrySilent(const std::string& filePath, const std::string& chunkName = "");

    // ── Entry Script Watcher ─────────────────────────────────────────
    // 注册一个待监视的入口脚本。
    // 若文件已存在则立即加载；否则在 PollFileChanges() 中轮询，出现后自动加载。
    // 同一 filePath 重复注册时更新 chunkName 并重置加载状态（支持工程切换）。
    void WatchEntryScript(const std::string& filePath, const std::string& chunkName = "");

    // ── 加载脚本 ─────────────────────────────────────────────────────────
    int  LoadScript(const std::string& filePath);
    int  LoadString(const std::string& code, const std::string& chunkName = "=string");

    // ── 热重载 ──────────────────────────────────────────────────────────
    int  ReloadAll();
    int  ReloadFile(const std::string& filePath);

    // ── 查询 ──────────────────────────────────────────────────────────
    const std::vector<std::string>&          GetLoadedFiles()    const { return m_loadedFiles; }
    const std::unordered_set<std::string>&   GetRegisteredIds()  const { return m_registeredIds; }
    const std::string&                       GetLastError()      const { return m_lastError; }
    bool                                     IsInitialized()     const { return m_registry != nullptr; }
    bool                                     GetAutoReload()     const { return m_autoReload; }

    // ── 清理 ──────────────────────────────────────────────────────────
    void UnregisterAll();
    void PollFileChanges();
    void SetAutoReload(bool enable) { m_autoReload = enable; }

    // 供 l_registerNode 回调使用（内部用）
    std::unordered_set<std::string>& GetRegisteredIds_Mutable() { return m_registeredIds; }
    void IncrPendingCount() { ++m_pendingCount; }

private:
    bool ensureLuaState();
    int  executeFile(const std::string& filePath);
    int  executeString(const std::string& code, const std::string& chunkName);
    void registerEditorBindings(lua_State* L);
    void resetLuaState();

    NodeEditor::Runtime::INodeRegistry*                                    m_registry   = nullptr;
    std::unordered_map<std::string, NodeEditor::Runtime::NodeHandler>*     m_handlerMap = nullptr;

    lua_State*                      m_L            = nullptr;
    std::vector<std::string>        m_loadedFiles;
    std::unordered_set<std::string> m_registeredIds;
    std::string                     m_lastError;

    std::unordered_map<std::string, int64_t> m_fileModTimes;
    bool   m_autoReload      = true;
    float  m_pollIntervalSec = 1.0f;
    float  m_pollAccum       = 0.0f;
    int    m_pendingCount    = 0;

    // Entry script watcher（BlueprintEntry.lua per-project）
    struct EntryWatch {
        std::string filePath;
        std::string chunkName;
        bool        loaded = false;   // true = 已成功加载（不重复加载）
    };
    std::vector<EntryWatch> m_entryWatches;
};

#else // !BLUEPRINT_HAS_LUA

// 无 Lua 支持时的空桩
class LuaNodeRegistrar
{
public:
    void Initialize(NodeEditor::Runtime::INodeRegistry*, 
                    std::unordered_map<std::string, NodeEditor::Runtime::NodeHandler>*) {}
    int  LoadScript(const std::string&)                            { return 0; }
    int  LoadString(const std::string&, const std::string& = "")  { return 0; }
    int  ReloadAll()                                               { return 0; }
    int  ReloadFile(const std::string&)                            { return 0; }
    void UnregisterAll()                                           {}
    void PollFileChanges()                                         {}
    void SetAutoReload(bool)                                       {}
    bool GetAutoReload()    const { return false; }
    bool IsInitialized()    const { return false; }
    const std::string& GetLastError() const
        { static const std::string s = "Lua not enabled"; return s; }
    const std::vector<std::string>& GetLoadedFiles() const
        { static const std::vector<std::string> v; return v; }
};

#endif // BLUEPRINT_HAS_LUA
