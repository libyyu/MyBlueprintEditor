// BlueprintEditor/LuaNodeRegistrar.cpp
// 编辑器侧 Lua 扩展管理器实现
// 所有 Lua 操作委托给绑定的 BlueprintRunner（Runtime 层统一管理 Lua VM）

#include "LuaNodeRegistrar.h"
#include "../Runtime/BlueprintRunner.h"
#include "BpLogger.h"
#include <filesystem>
#include <algorithm>

namespace fs = std::filesystem;

// ============================================================================
// BindRunner
// ============================================================================

void LuaNodeRegistrar::BindRunner(NodeEditor::Runtime::BlueprintRunner* runner)
{
    m_runner = runner;

    if (!runner) return;

#ifdef BLUEPRINT_HAS_LUA
    // 将编辑器的日志回调注入 runner：Lua print/warn 输出到编辑器控制台
    if (m_logCallback)
    {
        runner->SetLogCallback([this](NodeEditor::Runtime::LogLevel level, const std::string& msg) {
            int lv = (level == NodeEditor::Runtime::LogLevel::Error)   ? 2
                   : (level == NodeEditor::Runtime::LogLevel::Warning) ? 1 : 0;
            m_logCallback(lv, msg);
        });
    }
#endif
}

// ============================================================================
// SetLogCallback
// ============================================================================

void LuaNodeRegistrar::SetLogCallback(LogCallback cb)
{
    m_logCallback = std::move(cb);

    // 如果已绑定 runner，立即更新其日志回调
    if (m_runner && m_logCallback)
    {
#ifdef BLUEPRINT_HAS_LUA
        auto& logCb = m_logCallback;
        m_runner->SetLogCallback([logCb](NodeEditor::Runtime::LogLevel level, const std::string& msg) {
            int lv = (level == NodeEditor::Runtime::LogLevel::Error)   ? 2
                   : (level == NodeEditor::Runtime::LogLevel::Warning) ? 1 : 0;
            logCb(lv, msg);
        });
#endif
    }
}

// ============================================================================
// AddLuaPath
// ============================================================================

void LuaNodeRegistrar::AddLuaPath(const std::string& dir)
{
#ifdef BLUEPRINT_HAS_LUA
    if (m_runner) m_runner->AddLuaPath(dir);
#else
    (void)dir;
#endif
}

// ============================================================================
// LoadEntrySilent — 加载全局 / 工程 BlueprintEntry.lua（不存在则静默跳过）
// ============================================================================

bool LuaNodeRegistrar::LoadEntrySilent(const std::string& filePath, const std::string& /*chunkName*/)
{
    if (!m_runner || filePath.empty()) return false;
    if (!fs::exists(filePath)) return false;

#ifdef BLUEPRINT_HAS_LUA
    if (!m_runner->LoadLuaScript(filePath))
    {
        m_lastError = m_runner->GetLastError();
        if (m_logCallback)
            m_logCallback(1, "[BlueprintEntry] " + m_lastError);
        return false;
    }
    return true;
#else
    return false;
#endif
}

// ============================================================================
// WatchEntryScript
// ============================================================================

void LuaNodeRegistrar::WatchEntryScript(const std::string& filePath, const std::string& chunkName)
{
    if (filePath.empty()) return;

    // 同路径已存在则重置 loaded（工程切换时支持重新加载）
    for (auto& w : m_entryWatches)
    {
        if (w.filePath == filePath)
        {
            w.chunkName = chunkName;
            w.loaded    = false;
            break;
        }
    }

    bool found = false;
    for (auto& w : m_entryWatches)
        if (w.filePath == filePath) { found = true; break; }
    if (!found)
        m_entryWatches.push_back({ filePath, chunkName, false });

    // 立即尝试加载（文件已存在）
    if (fs::exists(filePath))
    {
        bool ok = LoadEntrySilent(filePath, chunkName);
        for (auto& w : m_entryWatches)
            if (w.filePath == filePath) { w.loaded = ok; break; }
    }
}

// ============================================================================
// LoadScript
// ============================================================================

bool LuaNodeRegistrar::LoadScript(const std::string& filePath)
{
    if (!m_runner)
    {
        m_lastError = "Runner not bound";
        return false;
    }

#ifdef BLUEPRINT_HAS_LUA
    if (!m_runner->LoadLuaScript(filePath))
    {
        m_lastError = m_runner->GetLastError();
        return false;
    }

    // 记录文件（去重）
    if (std::find(m_loadedFiles.begin(), m_loadedFiles.end(), filePath) == m_loadedFiles.end())
        m_loadedFiles.push_back(filePath);

    // 记录修改时间
    try {
        auto t = fs::last_write_time(filePath);
        m_fileModTimes[filePath] = t.time_since_epoch().count();
    } catch (...) {}

    m_lastError.clear();
    return true;
#else
    return false;
#endif
}

// ============================================================================
// ReloadAll
// ============================================================================

bool LuaNodeRegistrar::ReloadAll()
{
    if (!m_runner || m_loadedFiles.empty()) return true;

#ifdef BLUEPRINT_HAS_LUA
    if (!m_runner->ReloadLuaScript(m_loadedFiles[0]))
    {
        m_lastError = m_runner->GetLastError();
        return false;
    }

    // ReloadLuaScript 内部重新加载所有 m_luaLoadedFiles（即 runner 侧的列表）
    // 同步编辑器侧修改时间
    for (const auto& f : m_loadedFiles)
    {
        try {
            auto t = fs::last_write_time(f);
            m_fileModTimes[f] = t.time_since_epoch().count();
        } catch (...) {}
    }

    m_lastError.clear();
    return true;
#else
    return false;
#endif
}

// ============================================================================
// ReloadFile
// ============================================================================

bool LuaNodeRegistrar::ReloadFile(const std::string& filePath)
{
    (void)filePath;  // TODO: 优化为精确单文件重载（当前实现为全量重载）
    return ReloadAll();
}

// ============================================================================
// UnregisterAll
// ============================================================================

void LuaNodeRegistrar::UnregisterAll()
{
#ifdef BLUEPRINT_HAS_LUA
    if (m_runner) m_runner->UnregisterAllLuaNodes();
#endif
    m_loadedFiles.clear();
    m_fileModTimes.clear();
}

// ============================================================================
// PollFileChanges
// ============================================================================

void LuaNodeRegistrar::PollFileChanges(float deltaTime)
{
    m_pollAccum += deltaTime;
    if (m_pollAccum < m_pollIntervalSec) return;
    m_pollAccum = 0.0f;

#ifdef BLUEPRINT_HAS_LUA
    // ── 热重载：检测已加载脚本变更 ────────────────────────────────────────
    if (m_autoReload && !m_loadedFiles.empty())
    {
        for (const auto& f : m_loadedFiles)
        {
            try {
                auto t = fs::last_write_time(f);
                int64_t tval = t.time_since_epoch().count();
                auto it = m_fileModTimes.find(f);
                if (it == m_fileModTimes.end() || it->second != tval)
                {
                    if (m_logCallback)
                        m_logCallback(0, "[Lua] Hot-reload: " + f);
                    ReloadFile(f);
                    break;  // ReloadAll 已处理全部文件，退出循环
                }
            } catch (...) {}
        }
    }

    // ── Entry Script Watcher：轮询待加载入口脚本 ──────────────────────────
    for (auto& w : m_entryWatches)
    {
        if (w.loaded) continue;
        try {
            if (fs::exists(w.filePath))
            {
                LoadEntrySilent(w.filePath, w.chunkName);
                w.loaded = true;
            }
        } catch (...) {}
    }
#endif
}

// ============================================================================
// Tick
// ============================================================================

void LuaNodeRegistrar::Tick(float deltaTime)
{
#ifdef BLUEPRINT_HAS_LUA
    if (m_runner) m_runner->TickLua(static_cast<double>(deltaTime));
#else
    (void)deltaTime;
#endif
}
