// Runtime/LuaScriptEngine.cpp -- Lua 脚本引擎实现

#ifdef BLUEPRINT_HAS_LUA

#include "LuaScriptEngine.h"
#include "LuaBindings.h"
#include "BlueprintRunner.h"

#include <lua.hpp>

namespace NodeEditor {
namespace Runtime {

LuaScriptEngine::LuaScriptEngine() = default;

LuaScriptEngine::~LuaScriptEngine()
{
    Shutdown();
}

LuaScriptEngine::LuaScriptEngine(LuaScriptEngine&& other) noexcept
    : m_L(other.m_L)
    , m_runner(other.m_runner)
    , m_ownsState(other.m_ownsState)
    , m_lastError(std::move(other.m_lastError))
{
    other.m_L = nullptr;
    other.m_runner = nullptr;
    other.m_ownsState = true;
}

LuaScriptEngine& LuaScriptEngine::operator=(LuaScriptEngine&& other) noexcept
{
    if (this != &other)
    {
        Shutdown();
        m_L = other.m_L;
        m_runner = other.m_runner;
        m_ownsState = other.m_ownsState;
        m_lastError = std::move(other.m_lastError);
        other.m_L = nullptr;
        other.m_runner = nullptr;
        other.m_ownsState = true;
    }
    return *this;
}

bool LuaScriptEngine::Initialize(BlueprintRunner* runner)
{
    if (m_L)
    {
        m_lastError = "Lua VM already initialized";
        return false;
    }
    if (!runner)
    {
        m_lastError = "runner is null";
        return false;
    }

    m_runner = runner;

    // 创建 Lua 虚拟机
    m_L = luaL_newstate();
    if (!m_L)
    {
        m_lastError = "Failed to create Lua state";
        return false;
    }

    // 打开标准库
    luaL_openlibs(m_L);

    // 注册 Blueprint.* API 和 metatable
    RegisterLuaBindings(m_L, m_runner);

    m_lastError.clear();
    return true;
}

bool LuaScriptEngine::InitializeWithExternalState(lua_State* L, BlueprintRunner* runner)
{
    if (m_L)
    {
        m_lastError = "Lua VM already initialized";
        return false;
    }
    if (!L)
    {
        m_lastError = "external lua_State is null";
        return false;
    }
    if (!runner)
    {
        m_lastError = "runner is null";
        return false;
    }

    m_L         = L;
    m_runner    = runner;
    m_ownsState = false;  // 不拥有这个 VM，Shutdown 时不 close

    // 不调用 luaL_openlibs（外部 VM 已初始化，重复 open 可能覆盖全局表）
    // 只注册 Blueprint.* 绑定
    RegisterLuaBindings(m_L, m_runner);

    m_lastError.clear();
    return true;
}

void LuaScriptEngine::Shutdown()
{
    if (m_L)
    {
        // 外部 State 模式：不关闭 VM，只清空引用
        if (m_ownsState)
            lua_close(m_L);
        m_L = nullptr;
    }
    m_runner = nullptr;
    m_ownsState = true;
    m_loadedFiles.clear();
    m_loadedCount = 0;
}

// ---------------------------------------------------------------------------
// 内部：lua_pcall 的错误处理函数（追加 traceback 到错误消息）
// ---------------------------------------------------------------------------
static int luaTraceback(lua_State* L)
{
    const char* msg = lua_tostring(L, 1);
    luaL_traceback(L, L, msg, 1);  // level 1 = 跳过本函数
    return 1;
}

// ---------------------------------------------------------------------------
// 内部：带 traceback 的 pcall（加载后执行）
//   栈顶必须是待执行的 chunk，执行后栈已清理
//   成功返回 true，失败写入 m_lastError 并返回 false
// ---------------------------------------------------------------------------
bool LuaScriptEngine::ExecuteChunk(const std::string& source)
{
    // 压入错误处理函数
    lua_pushcfunction(m_L, luaTraceback);
    int errFuncIdx = lua_gettop(m_L) - 1;  // chunk 在栈顶，errFunc 在其下方
    // 调整顺序：errFunc 必须在 chunk 之前
    lua_insert(m_L, errFuncIdx);            // 把 errFunc 移到 chunk 之前

    // 执行 chunk（0 参数，0 返回值，errFuncIdx 指定错误处理函数）
    if (lua_pcall(m_L, 0, 0, errFuncIdx) != LUA_OK)
    {
        const char* err = lua_tostring(m_L, -1);
        m_lastError = std::string("[") + source + "] " + (err ? err : "unknown error");
        lua_pop(m_L, 2);  // pop error msg + errFunc
        return false;
    }

    lua_pop(m_L, 1);  // pop errFunc
    return true;
}

bool LuaScriptEngine::LoadFile(const std::string& filePath)
{
    if (!m_L)
    {
        m_lastError = "Lua VM not initialized";
        return false;
    }

    // 加载文件（编译为 chunk，压入栈顶）
    if (luaL_loadfile(m_L, filePath.c_str()) != LUA_OK)
    {
        const char* err = lua_tostring(m_L, -1);
        m_lastError = std::string("Lua load error [") + filePath + "]: " + (err ? err : "unknown");
        lua_pop(m_L, 1);
        return false;
    }

    // 执行（带 traceback）
    if (!ExecuteChunk(filePath))
        return false;

    // 记录加载顺序
    m_loadedFiles.push_back(filePath);
    ++m_loadedCount;

    m_lastError.clear();
    return true;
}

bool LuaScriptEngine::LoadString(const std::string& code, const std::string& chunkName)
{
    if (!m_L)
    {
        m_lastError = "Lua VM not initialized";
        return false;
    }

    // 加载字符串（编译为 chunk，压入栈顶）
    if (luaL_loadbuffer(m_L, code.c_str(), code.size(), chunkName.c_str()) != LUA_OK)
    {
        const char* err = lua_tostring(m_L, -1);
        m_lastError = std::string("Lua load error [") + chunkName + "]: " + (err ? err : "unknown");
        lua_pop(m_L, 1);
        return false;
    }

    // 执行（带 traceback）
    if (!ExecuteChunk(chunkName))
        return false;

    // 记录加载计数（字符串不记入 m_loadedFiles，只计数）
    ++m_loadedCount;

    m_lastError.clear();
    return true;
}

} // namespace Runtime
} // namespace NodeEditor

#endif // BLUEPRINT_HAS_LUA
