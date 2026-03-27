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
    , m_lastError(std::move(other.m_lastError))
{
    other.m_L = nullptr;
    other.m_runner = nullptr;
}

LuaScriptEngine& LuaScriptEngine::operator=(LuaScriptEngine&& other) noexcept
{
    if (this != &other)
    {
        Shutdown();
        m_L = other.m_L;
        m_runner = other.m_runner;
        m_lastError = std::move(other.m_lastError);
        other.m_L = nullptr;
        other.m_runner = nullptr;
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

void LuaScriptEngine::Shutdown()
{
    if (m_L)
    {
        lua_close(m_L);
        m_L = nullptr;
    }
    m_runner = nullptr;
}

bool LuaScriptEngine::LoadFile(const std::string& filePath)
{
    if (!m_L)
    {
        m_lastError = "Lua VM not initialized";
        return false;
    }

    // 加载文件
    if (luaL_loadfile(m_L, filePath.c_str()) != LUA_OK)
    {
        const char* err = lua_tostring(m_L, -1);
        m_lastError = std::string("Lua load error: ") + (err ? err : "unknown");
        lua_pop(m_L, 1);
        return false;
    }

    // 执行
    if (lua_pcall(m_L, 0, 0, 0) != LUA_OK)
    {
        const char* err = lua_tostring(m_L, -1);
        m_lastError = std::string("Lua exec error: ") + (err ? err : "unknown");
        lua_pop(m_L, 1);
        return false;
    }

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

    // 加载字符串
    if (luaL_loadbuffer(m_L, code.c_str(), code.size(), chunkName.c_str()) != LUA_OK)
    {
        const char* err = lua_tostring(m_L, -1);
        m_lastError = std::string("Lua load error: ") + (err ? err : "unknown");
        lua_pop(m_L, 1);
        return false;
    }

    // 执行
    if (lua_pcall(m_L, 0, 0, 0) != LUA_OK)
    {
        const char* err = lua_tostring(m_L, -1);
        m_lastError = std::string("Lua exec error: ") + (err ? err : "unknown");
        lua_pop(m_L, 1);
        return false;
    }

    m_lastError.clear();
    return true;
}

} // namespace Runtime
} // namespace NodeEditor

#endif // BLUEPRINT_HAS_LUA
