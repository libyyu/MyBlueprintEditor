// BlueprintEditor/ScriptExtensionManager.cpp
// 编辑器侧的扩展脚本管理器实现。
// 所有操作委托给绑定的 BlueprintRunner（Runtime 层统一管理脚本后端 VM）。
// 编辑器对具体后端（Lua / JS / ...）无感知；运行时未启用后端时全部为空操作。

#include "ScriptExtensionManager.h"
#include "../Runtime/BlueprintRunner.h"
#include "BpLogger.h"
#include <filesystem>
#include <algorithm>

namespace fs = std::filesystem;

// ============================================================================
// BindRunner
// ============================================================================

void ScriptExtensionManager::BindRunner(NodeEditor::Runtime::BlueprintRunner* runner)
{
    m_runner = runner;

    if (!runner) return;

    // 将编辑器的日志回调注入 runner：脚本 print/warn 输出到编辑器控制台
    if (m_logCallback)
    {
        runner->SetLogCallback([this](NodeEditor::Runtime::LogLevel level, const std::string& msg) {
            int lv = (level == NodeEditor::Runtime::LogLevel::Error)   ? 2
                   : (level == NodeEditor::Runtime::LogLevel::Warning) ? 1 : 0;
            m_logCallback(lv, msg);
        });
    }
}

// ============================================================================
// SetLogCallback
// ============================================================================

void ScriptExtensionManager::SetLogCallback(LogCallback cb)
{
    m_logCallback = std::move(cb);

    // 如果已绑定 runner，立即更新其日志回调
    if (m_runner && m_logCallback)
    {
        auto& logCb = m_logCallback;
        m_runner->SetLogCallback([logCb](NodeEditor::Runtime::LogLevel level, const std::string& msg) {
            int lv = (level == NodeEditor::Runtime::LogLevel::Error)   ? 2
                   : (level == NodeEditor::Runtime::LogLevel::Warning) ? 1 : 0;
            logCb(lv, msg);
        });
    }
}

// ============================================================================
// AddSearchPath
// ============================================================================

void ScriptExtensionManager::AddSearchPath(const std::string& dir)
{
    if (m_runner) m_runner->AddScriptSearchPath(dir);
}

// ============================================================================
// LoadEntrySilent — 加载全局/工程入口脚本（不存在则静默跳过）
// ============================================================================

bool ScriptExtensionManager::LoadEntrySilent(const std::string& filePath, const std::string& /*chunkName*/)
{
    if (!m_runner || filePath.empty()) return false;
    if (!fs::exists(filePath)) return false;

    if (!m_runner->LoadExtensionScript(filePath))
    {
        m_lastError = m_runner->GetLastError();
        if (m_logCallback)
            m_logCallback(1, "[BlueprintEntry] " + m_lastError);
        return false;
    }
    return true;
}

// ============================================================================
// WatchEntryScript
// ============================================================================

void ScriptExtensionManager::WatchEntryScript(const std::string& filePath, const std::string& chunkName)
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

bool ScriptExtensionManager::LoadScript(const std::string& filePath)
{
    if (!m_runner)
    {
        m_lastError = "Runner not bound";
        return false;
    }

    if (!m_runner->LoadExtensionScript(filePath))
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
}

// ============================================================================
// ReloadAll
// ============================================================================

bool ScriptExtensionManager::ReloadAll()
{
    if (!m_runner || m_loadedFiles.empty()) return true;

    if (!m_runner->ReloadExtensionScript(m_loadedFiles[0]))
    {
        m_lastError = m_runner->GetLastError();
        return false;
    }

    // ReloadExtensionScript 内部重新加载所有已加载脚本
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
}

// ============================================================================
// ReloadFile
// ============================================================================

bool ScriptExtensionManager::ReloadFile(const std::string& filePath)
{
    (void)filePath;  // TODO: 优化为精确单文件重载（当前实现为全量重载）
    return ReloadAll();
}

// ============================================================================
// UnregisterAll
// ============================================================================

void ScriptExtensionManager::UnregisterAll()
{
    if (m_runner) m_runner->UnregisterAllScriptedNodes();
    m_loadedFiles.clear();
    m_fileModTimes.clear();
}

// ============================================================================
// PollFileChanges
// ============================================================================

void ScriptExtensionManager::PollFileChanges(float deltaTime)
{
    m_pollAccum += deltaTime;
    if (m_pollAccum < m_pollIntervalSec) return;
    m_pollAccum = 0.0f;

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
                        m_logCallback(0, "[Script] Hot-reload: " + f);
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
}

// ============================================================================
// Tick
// ============================================================================

void ScriptExtensionManager::Tick(float deltaTime)
{
    if (m_runner) m_runner->TickScriptExtensions(static_cast<double>(deltaTime));
}
