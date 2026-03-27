// Runtime/LuaScriptEngine.h -- Lua 脚本引擎（管理 lua_State 生命周期）
//
// 职责：
//   · 创建 / 关闭 Lua 虚拟机
//   · 注册 Blueprint.* Lua API
//   · 加载并执行 Lua 脚本文件或字符串
//
// 条件编译：仅在 BLUEPRINT_HAS_LUA 定义时可用
//
#pragma once

#ifdef BLUEPRINT_HAS_LUA

#include "BlueprintExport.h"
#include <string>

// MSVC C4251: 'member': class 'std::...' needs to have dll-interface
// Safe to suppress when DLL and consumer share the same CRT/compiler.
#ifdef _MSC_VER
#   pragma warning(push)
#   pragma warning(disable: 4251)
#endif

// 前向声明，避免暴露 lua.h 给使用者
struct lua_State;

namespace NodeEditor {
namespace Runtime {

class BlueprintRunner;

class BLUEPRINT_API LuaScriptEngine
{
public:
    LuaScriptEngine();
    ~LuaScriptEngine();

    // 禁止拷贝
    LuaScriptEngine(const LuaScriptEngine&) = delete;
    LuaScriptEngine& operator=(const LuaScriptEngine&) = delete;

    // 允许移动
    LuaScriptEngine(LuaScriptEngine&& other) noexcept;
    LuaScriptEngine& operator=(LuaScriptEngine&& other) noexcept;

    // 初始化 VM（打开标准库 + 注册 Blueprint.* API）
    // runner: handler 注册目标（生命周期由调用者保证）
    bool Initialize(BlueprintRunner* runner);

    // 关闭 VM，释放所有资源
    void Shutdown();

    // 是否已初始化
    bool IsInitialized() const { return m_L != nullptr; }

    // 加载并执行 Lua 文件
    bool LoadFile(const std::string& filePath);

    // 加载并执行 Lua 字符串
    bool LoadString(const std::string& code, const std::string& chunkName = "=string");

    // 获取最后的错误信息
    const std::string& GetLastError() const { return m_lastError; }

    // 获取底层 lua_State（高级用途）
    lua_State* GetState() const { return m_L; }

private:
    lua_State*       m_L = nullptr;
    BlueprintRunner* m_runner = nullptr;
    std::string      m_lastError;
};

} // namespace Runtime
} // namespace NodeEditor

#ifdef _MSC_VER
#   pragma warning(pop)
#endif

#endif // BLUEPRINT_HAS_LUA
