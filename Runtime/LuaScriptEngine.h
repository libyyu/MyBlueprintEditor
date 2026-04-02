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
#include <vector>

// MSVC C4251: 'member': class 'std::...' needs to have dll-interface
// Safe to suppress when DLL and consumer share the same CRT/compiler.
#ifdef _MSC_VER
#   pragma warning(push)
#   pragma warning(disable: 4251)
#endif

// 前向声明，避免暴露 lua.h 给使用者
struct lua_State;
// lua_CFunction: int (*)(lua_State*) — 与 lua.h 一致，此处手动前向声明以免引入 lua.h
typedef int (*lua_CFunction)(lua_State*);

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

    // 初始化 VM（自建 lua_State，打开标准库 + 注册 Blueprint.* API）
    // runner: handler 注册目标（生命周期由调用者保证）
    bool Initialize(BlueprintRunner* runner);

    // 使用外部 lua_State 初始化（不自建 VM，不关闭 VM）
    // 用于 iOS/Emscripten 等静态链接平台，复用宿主（如 xLua）的 Lua VM，避免符号冲突。
    // 注意：外部 lua_State 的生命周期由调用者管理，Blueprint 不会调用 lua_close()。
    bool InitializeWithExternalState(lua_State* L, BlueprintRunner* runner);

    // 关闭 VM，释放所有资源（外部 State 模式下只注销绑定，不 close VM）
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

    // 向 package.searchers 头部插入一个自定义 loader（插入到索引 1，优先级最高）
    // 必须在 Initialize / InitializeWithExternalState 之后调用。
    // loader 的 Lua 签名：function(modname) -> chunk | string（见 Lua 5.4 searchers 规范）
    void SetSearcher(lua_CFunction loader);

    // 向 package.path 追加搜索路径（dir 末尾自动补 /?.lua;/?.lua）
    // 如 AddLuaPath("/home/user/scripts") => "/home/user/scripts/?.lua"
    void AddLuaPath(const std::string& dir);

    // 获取已加载的脚本文件列表（按加载顺序）
    const std::vector<std::string>& GetLoadedFiles() const { return m_loadedFiles; }

    // 获取已加载的脚本数量（含字符串加载）
    int GetLoadedCount() const { return m_loadedCount; }

private:
    // 带 traceback 的 pcall 执行已编译 chunk（栈顶）
    bool ExecuteChunk(const std::string& source);

    lua_State*                 m_L = nullptr;
    BlueprintRunner*           m_runner = nullptr;
    bool                       m_ownsState = true;  // false = 外部传入，Shutdown 时不 close
    std::string                m_lastError;
    std::vector<std::string>   m_loadedFiles;   // 按顺序记录已加载的文件路径
    int                        m_loadedCount = 0; // 总加载次数（文件 + 字符串）
};

} // namespace Runtime
} // namespace NodeEditor

#ifdef _MSC_VER
#   pragma warning(pop)
#endif

#endif // BLUEPRINT_HAS_LUA
