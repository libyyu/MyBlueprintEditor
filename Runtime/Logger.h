// Runtime/Logger.h -- 统一日志系统
// 提供全局 Logger 单例 + BP_LOG/BP_WARN/BP_ERROR 便捷宏
// 使用方：设置 callback 后即可接收所有日志，不设则静默（不依赖 ImGui）
#pragma once
#include "BlueprintExport.h"
#include "Types.h"   // LogLevel 定义在此，避免重复定义
#include <string>
#include <functional>

// MSVC C4251: STL members in DLL-exported class
#ifdef _MSC_VER
#   pragma warning(push)
#   pragma warning(disable: 4251)
#endif

namespace NodeEditor {
namespace Runtime {

// LogLevel 已定义在 Types.h，此处不再重复声明

class BLUEPRINT_API Logger
{
public:
    static Logger& Get();

    // 设置日志回调（线程安全：callback 需自行处理并发）
    void SetCallback(std::function<void(LogLevel, const std::string& tag, const std::string& msg)> cb)
    { m_callback = std::move(cb); }

    // 设置最低输出级别（低于此级别的日志被丢弃）
    void SetMinLevel(LogLevel level) { m_minLevel = level; }
    LogLevel GetMinLevel() const { return m_minLevel; }

    void Log(LogLevel level, const std::string& tag, const std::string& msg)
    {
        if (level < m_minLevel) return;
        if (m_callback)
            m_callback(level, tag, msg);
    }

private:
    Logger() = default;
    std::function<void(LogLevel, const std::string&, const std::string&)> m_callback;
    LogLevel m_minLevel = LogLevel::Info;
};

} // namespace Runtime
} // namespace NodeEditor

#ifdef _MSC_VER
#   pragma warning(pop)
#endif

// 便捷宏（tag 默认为当前文件名）
#define BP_LOG_V(tag, msg) ::NodeEditor::Runtime::Logger::Get().Log(::NodeEditor::Runtime::LogLevel::Verbose, (tag), (msg))
#define BP_LOG(tag, msg)   ::NodeEditor::Runtime::Logger::Get().Log(::NodeEditor::Runtime::LogLevel::Info,    (tag), (msg))
#define BP_WARN(tag, msg)  ::NodeEditor::Runtime::Logger::Get().Log(::NodeEditor::Runtime::LogLevel::Warning, (tag), (msg))
#define BP_ERROR(tag, msg) ::NodeEditor::Runtime::Logger::Get().Log(::NodeEditor::Runtime::LogLevel::Error,   (tag), (msg))
