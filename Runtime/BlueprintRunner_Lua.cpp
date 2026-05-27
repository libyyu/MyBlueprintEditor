// Runtime/BlueprintRunner_Lua.cpp -- BlueprintRunner Lua extension methods
// Extracted from BlueprintRunner.cpp to keep the core runtime independent of the Lua backend.
// All methods here are conditionally compiled via BLUEPRINT_HAS_LUA.

#include "BlueprintRunner.h"
#ifdef BLUEPRINT_HAS_LUA
#  include "LuaBindings.h"      // BindRunnerToLuaState (must be included outside namespace)
#  include "LuaScriptEngine.h"
#endif

namespace NodeEditor {
namespace Runtime {

// ============================================================================
// Lua 脚本扩展
// ============================================================================
#ifdef BLUEPRINT_HAS_LUA

bool BlueprintRunner::LoadExtensionScript(const std::string& filePath)
{
    // 确保脚本引擎已初始化（首次调用时创建/绑定共享，后续复用）
    if (!EnsureScriptEngine()) return false;

    // 共享 VM 场景：把当前活跃 Runner 切换为 this，
    // 让脚本里的 Blueprint.RegisterHandler / print 能路由到本 Runner
    BindRunnerToLuaState(m_luaEngine->GetState(), this);

    // 共享 VM 时同一脚本可能已被先前的 Runner 加载过 —— 直接复用注册结果，
    // 避免重复执行脚本顶层副作用、重复注册 handler
    if (m_luaEngine->HasLoadedFile(filePath))
    {
        Log("[Script] Already loaded in shared VM, reusing: " + filePath,
            LogLevel::Verbose);
        return true;
    }

    Log("[Script] Loading: " + filePath + " (order: " +
        std::to_string(m_luaEngine->GetLoadedCount() + 1) + ")", LogLevel::Verbose);

    if (!m_luaEngine->LoadFile(filePath))
    {
        m_lastError = "Script load error: " + m_luaEngine->GetLastError();
        LogError("[Script] " + m_lastError);
        return false;
    }

#ifndef __EMSCRIPTEN__
	namespace fs = std::filesystem;
	std::string dataDir = fs::path(fs::absolute(filePath)).parent_path().string();
	std::string code = "do _G.DataPath=\"" + dataDir + "\" end";
    m_luaEngine->LoadString(code);
#endif

    // 记录修改时间到 engine（热重载用；WebGL 无文件系统，跳过）
#if !defined(__EMSCRIPTEN__) && !defined(BLUEPRINT_NO_FILESYSTEM)
    try {
        namespace fs = std::filesystem;
        auto t = fs::last_write_time(filePath);
        m_luaEngine->SetFileMtime(filePath, t.time_since_epoch().count());
    } catch (const std::exception& ex) {
        // 读取 mtime 失败：本次跳过热重载注册（不阻塞脚本加载）；
        // 多在网络盘 / 文件正被外部进程写入时发生，verbose 级别避免干扰。
        Log(std::string("[Script] last_write_time failed for ") + filePath +
            ": " + ex.what() + " (hot-reload tracking disabled for this file)",
            LogLevel::Verbose);
    } catch (...) {
        Log(std::string("[Script] last_write_time threw unknown exception for ") +
            filePath + " (hot-reload tracking disabled for this file)",
            LogLevel::Verbose);
    }
#endif

    Log("[Script] Loaded OK: " + filePath, LogLevel::Verbose);
    return true;
}

bool BlueprintRunner::ReloadExtensionScript(const std::string& filePath)
{
    if (!m_luaEngine) return LoadExtensionScript(filePath);

    // 注销脚本注册的所有节点（共享 engine 模式下会影响其他 Runner，
    // 但调用方已知会触发全量重载）
    UnregisterAllScriptedNodes();

    if (m_luaEngine.use_count() > 1)
    {
        Log("[Script] Reload on shared engine — will rebind, "
            "old function refs in shared VM remain until GC.", LogLevel::Verbose);
    }

    // 拷贝旧的文件列表，再 reset engine，重新逐个加载
    auto files = m_luaEngine->GetLoadedFiles();
    m_luaEngine.reset();

    for (const auto& f : files)
    {
        if (!LoadExtensionScript(f))
            return false;  // 报错止步
    }
    (void)filePath;  // 当前实现总是全量重载，保留参数以备将来精确单文件实现
    return true;
}

void BlueprintRunner::UnregisterAllScriptedNodes()
{
    if (m_luaEngine)
        m_luaEngine->UnregisterAllScriptedNodes();
}

const std::unordered_set<std::string>& BlueprintRunner::GetScriptRegisteredNodeIds() const
{
    static const std::unordered_set<std::string> kEmpty;
    return m_luaEngine ? m_luaEngine->GetRegisteredNodeIds() : kEmpty;
}

const std::vector<std::string>& BlueprintRunner::GetLoadedExtensionScripts() const
{
    static const std::vector<std::string> kEmpty;
    return m_luaEngine ? m_luaEngine->GetLoadedFiles() : kEmpty;
}

void BlueprintRunner::MarkScriptRegisteredNode(const std::string& id)
{
    // 必须由脚本绑定层在 Runner 已绑定 engine 之后回调；防御性兜底
    if (m_luaEngine)
        m_luaEngine->MarkRegisteredNode(id);
}

void BlueprintRunner::UnmarkScriptRegisteredNode(const std::string& id)
{
    if (m_luaEngine)
        m_luaEngine->UnmarkRegisteredNode(id);
}

void BlueprintRunner::AddScriptSearchPath(const std::string& dir)
{
    if (!EnsureScriptEngine()) return;
    m_luaEngine->AddLuaPath(dir);
}

bool BlueprintRunner::LoadExtensionScriptString(const std::string& code, const std::string& name)
{
    // 延迟创建/绑定脚本引擎（默认走共享）
    if (!EnsureScriptEngine())
    {
        return false;
    }

    // 共享 VM 场景：切换到本 Runner（影响 Blueprint.RegisterHandler 路由）
    BindRunnerToLuaState(m_luaEngine->GetState(), this);

    Log("[Script] Loading string: " + name + " (order: " +
        std::to_string(m_luaEngine->GetLoadedCount() + 1) + ")", LogLevel::Verbose);

    if (!m_luaEngine->LoadString(code, name))
    {
        m_lastError = "Script exec error: " + m_luaEngine->GetLastError();
        LogError("[Script] " + m_lastError);
        return false;
    }

    Log("[Script] String loaded OK: " + name, LogLevel::Verbose);
    return true;
}

LuaScriptEngine* BlueprintRunner::GetLuaEngine()
{
    return m_luaEngine.get();
}

bool BlueprintRunner::SetSharedLuaEngine(std::shared_ptr<LuaScriptEngine> engine)
{
    if (m_luaEngine)
    {
        // 已经持有引擎，不允许热替换（避免 handler/state 不一致）
        m_lastError = "BlueprintRunner already has a script engine; cannot replace";
        return false;
    }
    m_luaEngine = std::move(engine);
    return m_luaEngine != nullptr;
}

bool BlueprintRunner::EnsureScriptEngine()
{
    if (m_luaEngine) return true;

    // 优先使用进程级默认共享 Engine（多 Runner 共享一个 VM）
    // 注意：IsInitialized() 检查 m_L != nullptr，外部 VM 被 InvalidateLuaState() 置空后
    // 不应再绑定到该 Engine，否则 Tick 会访问野指针（lua_close 后的悬空 lua_State）。
    auto shared = LuaScriptEngineRegistry::GetDefault();
    if (shared && shared->IsInitialized())
    {
        m_luaEngine = shared;
        Log("[Script] Bound to shared engine (lua_State=" +
            std::to_string(reinterpret_cast<uintptr_t>(shared->GetState())) + ")",
            LogLevel::Verbose);
        return true;
    }

    // 默认 Engine 不可用 —— 极少见，回退到独立 VM（不与其他 Runner 共享）
    m_luaEngine = std::make_shared<LuaScriptEngine>();
    if (!m_luaEngine->Initialize(this))
    {
        m_lastError = "Failed to initialize script engine: " + m_luaEngine->GetLastError();
        LogError("[Script] " + m_lastError);
        m_luaEngine.reset();
        return false;
    }
    Log("[Script] Standalone engine initialized (default registry unavailable)", LogLevel::Verbose);
    return true;
}

void BlueprintRunner::TickScriptExtensions(double deltaSeconds)
{
    if (m_luaEngine)
        m_luaEngine->Tick(deltaSeconds);
}

#endif // BLUEPRINT_HAS_LUA
} // namespace Runtime
} // namespace NodeEditor
