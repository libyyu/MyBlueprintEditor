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
#include <memory>           // std::shared_ptr

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

    // 心跳 Tick：每帧/每定时器调用。
    // 若 Lua 全局存在 onTick(deltaSeconds) 函数则调用它。
    // deltaSeconds 为距上次调用的秒数（由调用方计算传入）。
    // 线程安全：必须在拥有 lua_State 的线程（主线程）调用。
    void Tick(double deltaSeconds);

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

// =========================================================================
// 进程级默认 Lua Engine（共享 VM，避免每个 BlueprintRunner 各自创建一个 lua_State）
// =========================================================================
//
// 设计目标：
//   · 默认情况下所有 BlueprintRunner 共享同一个 lua_State，
//     避免 N 个 Runner = N 个 VM 的浪费（每个 VM 50KB+ 基础开销，
//     加上加载的 Lua 模块、handler 表、xLua 桥接对象，可能数 MB）。
//   · 共享后：所有 Lua 模块只需加载一次，Lua 节点定义可跨 Runner 复用。
//
// 用法：
//
//   1. 默认行为（不调用任何接口）：
//      第一个 BlueprintRunner 触发 Lua 时自动创建默认 Engine 并注册为全局；
//      之后所有 Runner 自动共享。析构时引用计数归零，自动释放 VM。
//
//   2. 接入外部 VM（xLua / Unity 宿主）：
//      auto eng = std::make_shared<LuaScriptEngine>();
//      eng->InitializeWithExternalState(xluaState, nullptr);   // runner 可空
//      LuaScriptEngine::SetDefault(eng);
//
//   3. 双 VM 隔离（Update/Game 阶段切换）：
//      auto upd = std::make_shared<LuaScriptEngine>();
//      upd->Initialize(nullptr);
//      LuaScriptEngine::SetDefault(upd);
//      // ... 跑 update 阶段 ...
//      LuaScriptEngine::SetDefault(nullptr);  // 释放 upd（如果没人持有）
//      auto game = std::make_shared<LuaScriptEngine>();
//      game->Initialize(nullptr);
//      LuaScriptEngine::SetDefault(game);
//
//   4. 显式独立 VM（不走默认）：
//      auto isolated = std::make_shared<LuaScriptEngine>();
//      isolated->Initialize(nullptr);
//      runner.SetSharedLuaEngine(isolated);

class BLUEPRINT_API LuaScriptEngineRegistry
{
public:
    /// 取默认共享 Engine。首次调用时会自动创建并 Initialize（runner=nullptr，
    /// 各 BlueprintRunner 通过 m_runner 独立持有）。
    static std::shared_ptr<LuaScriptEngine> GetDefault();

    /// 设置默认共享 Engine。传 nullptr 则清除（下次 GetDefault() 会重建）。
    /// 调用时机要求：在第一个 BlueprintRunner 触发 Lua 之前，
    /// 否则之前已创建的默认引擎不会被替换（但已绑定的 Runner 不受影响）。
    static void SetDefault(std::shared_ptr<LuaScriptEngine> engine);

    /// 是否已经创建了默认 Engine
    static bool HasDefault();
};

} // namespace Runtime
} // namespace NodeEditor

#ifdef _MSC_VER
#   pragma warning(pop)
#endif

#endif // BLUEPRINT_HAS_LUA
