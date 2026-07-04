// Runtime/LuaScriptEngine.cpp -- Lua 脚本引擎实现

#ifdef BLUEPRINT_HAS_LUA

#include "LuaScriptEngine.h"
#include "LuaBindings.h"
#include "BlueprintRunner.h"
#include "SharedRegistry.h"

#include <lua.hpp>
#include <mutex>          // LuaScriptEngineRegistry 锁
#include <algorithm>      // std::find

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
    , m_loadedFiles(std::move(other.m_loadedFiles))
    , m_loadedCount(other.m_loadedCount)
    , m_registeredNodeIds(std::move(other.m_registeredNodeIds))
    , m_fileMtimes(std::move(other.m_fileMtimes))
{
    other.m_L = nullptr;
    other.m_runner = nullptr;
    other.m_ownsState = true;
    other.m_loadedCount = 0;
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
        m_loadedFiles = std::move(other.m_loadedFiles);
        m_loadedCount = other.m_loadedCount;
        m_registeredNodeIds = std::move(other.m_registeredNodeIds);
        m_fileMtimes = std::move(other.m_fileMtimes);
        other.m_L = nullptr;
        other.m_runner = nullptr;
        other.m_ownsState = true;
        other.m_loadedCount = 0;
    }
    return *this;
}
static std::string _luaArgsToString(lua_State* L)
{
    std::string msg;
	int n = lua_gettop(L);
	lua_getglobal(L, "tostring");
	for (int i = 1; i <= n; ++i)
	{
		lua_pushvalue(L, -1);   // tostring 函数
		lua_pushvalue(L, i);    // 参数
		lua_pcall(L, 1, 1, 0);
		const char* s = lua_tostring(L, -1);
		if (s) msg += s;
		if (i < n) msg += "\t";
		lua_pop(L, 1);
	}
	lua_pop(L, 1); // pop tostring
	return msg;
}

bool LuaScriptEngine::Initialize(BlueprintRunner* runner)
{
    if (m_L)
    {
        m_lastError = "Lua VM already initialized";
        return false;
    }
    // runner 允许为 nullptr（编辑器模式：不需要执行节点 handler）
    fprintf(stdout, "LuaScriptEngine Initialize\n");
    m_runner = runner;

    // 创建 Lua 虚拟机
    m_L = luaL_newstate();
    if (!m_L)
    {
        m_lastError = "Failed to create Lua state";
        return false;
    }

    // Any Variant 生命周期保护：注册 lua_State 到全局 guard 表，
    // Shutdown 时 Unregister 让残留 Variant 析构变成安全 no-op
    RegisterLuaState(m_L);

    // 打开标准库
    luaL_openlibs(m_L);

    // 注册 Blueprint.* API 和 metatable
    RegisterLuaBindings(m_L, m_runner);

    // 注册 json.* 和 http.* 全局库
    RegisterLuaJsonHttpLibs(m_L);

    // 重定向 Lua print() → runner 的 PrintCallback（若无 runner 则保持 stdout）
    // 这样 Lua 脚本里的 print() 输出能在编辑器控制台和 runtime-example 都能看到
    if (!m_runner)
    {
        // 通用 print 实现：upvalue[1] = LogLevel（int），通过 __blueprint_runner 路由输出
        static auto luaPrintImpl = [](lua_State* L) -> int {
            int level = static_cast<int>(lua_tointeger(L, lua_upvalueindex(1)));
            std::string msg = _luaArgsToString(L);
            
            if (auto eng = NodeEditor::Runtime::LuaScriptEngineRegistry::GetDefault(true))
            {
                auto callback = eng->m_logCallback;
                if (callback)
                {
                    callback(level, msg.c_str());
                    return 0;
                }
            }
            
            lua_getfield(L, LUA_REGISTRYINDEX, "__blueprint_runner");
            auto* r = static_cast<BlueprintRunner*>(lua_touserdata(L, -1));
            lua_pop(L, 1);
            if (r)
                r->Print(msg, static_cast<LogLevel>(level));
            else
                fprintf(level >= static_cast<int>(LogLevel::Error) ? stderr : stdout, "[lua]%s\n", msg.c_str());
            return 0;
        };

        // print → Info, warn → Warning, printerror → Error
        lua_pushinteger(m_L, static_cast<lua_Integer>(LogLevel::Info));
        lua_pushcclosure(m_L, luaPrintImpl, 1);
        lua_setglobal(m_L, "print");

        lua_pushinteger(m_L, static_cast<lua_Integer>(LogLevel::Warning));
        lua_pushcclosure(m_L, luaPrintImpl, 1);
        lua_setglobal(m_L, "warn");

        lua_pushinteger(m_L, static_cast<lua_Integer>(LogLevel::Error));
        lua_pushcclosure(m_L, luaPrintImpl, 1);
        lua_setglobal(m_L, "printerror");
        
        static auto ll_script_loadfile = [](lua_State* L) -> int
        {
            std::string s_fileName = luaL_checkstring(L, 1);
            std::replace(s_fileName.begin(), s_fileName.end(), '.', '/');
            std::string fileName = s_fileName;
            if (fileName.find(".lua") == std::string::npos) fileName += ".lua";
            auto fs = GetDefaultFileSystem();
            if (!fs)
            {
                lua_pushnil(L);
                lua_pushfstring(L, "failed to loadfile: %s, FileSystem not valid.", fileName.c_str());
                return 2;
            }
            std::string outBuffer;
            std::string outError;
            if (!fs->ReadFile(fileName.c_str(), outBuffer, outError))
            {
                lua_pushnil(L);
                lua_pushfstring(L, "failed to loadfile: %s, file load error %s.", fileName.c_str(), outError.c_str());
                return 2;
            }
            std::string chunk = "@"; chunk += fileName;
            if (luaL_loadbuffer(L, outBuffer.c_str(), outBuffer.size(), chunk.c_str()) != 0)
            {
                lua_pushnil(L);
                lua_pushfstring(L, "failed to loadfile: %s", fileName.c_str());
                return 2;
            }
            
            return 1;
        };
        lua_pushcfunction(m_L, ll_script_loadfile);
        lua_setglobal(m_L, "loadfile");
        
        static auto ll_script_dofile = [](lua_State* L) -> int
        {
            int n = lua_gettop(L);
            std::string s_fileName = luaL_checkstring(L, 1);
            std::replace(s_fileName.begin(), s_fileName.end(), '.', '/');
            std::string fileName = s_fileName;
            if (fileName.find(".lua") == std::string::npos) fileName += ".lua";
            auto fs = GetDefaultFileSystem();
            if (!fs)
            {
                luaL_error(L, "failed to dofile: %s, FileSystem not valid.", fileName.c_str());
                return 0;
            }
            std::string outBuffer;
            std::string outError;
            if (!fs->ReadFile(fileName.c_str(), outBuffer, outError))
            {
                luaL_error(L, "failed to dofile: %s, file load error %s.", fileName.c_str(), outError.c_str());
                return 0;
            }
            std::string chunk = "@"; chunk += fileName;
            if (luaL_loadbuffer(L, outBuffer.c_str(), outBuffer.size(), chunk.c_str()) != 0)
            {
                luaL_error(L, "failed to dofile: %s", fileName.c_str());
                return 0;
            }
            
            lua_replace(L, 1);
            if (lua_pcall(L, n-1, LUA_MULTRET, 0) != 0)
            {
                luaL_error(L, "failed to dofile: %s", fileName.c_str());
                return 0;
            }
            return lua_gettop(L);
        };
        lua_pushcfunction(m_L, ll_script_dofile);
        lua_setglobal(m_L, "dofile");
        
        // static auto luaSearcher = [](lua_State* L) -> int {
        //     std::string s_fileName = luaL_checkstring(L, 1);
        //     std::replace(s_fileName.begin(), s_fileName.end(), '.', '/');
        //     std::string fileName = s_fileName;
        //     if (fileName.find(".lua") == std::string::npos) fileName += ".lua";
        //     
        //     auto fs = GetDefaultFileSystem();
        //     if (!fs)
        //     {
        //         return 0;
        //     }
        //     
        //     std::string outBuffer;
        //     std::string outError;
        //     if (fs->ReadFile(fileName.c_str(), outBuffer, outError))
        //     {
        //         std::string chunk = "@"; chunk += fileName;
        //         if (luaL_loadbuffer(L, outBuffer.c_str(), outBuffer.size(), chunk.c_str()) != 0)
        //         {
        //             lua_error(L);
        //             return 0;
        //         }
        //         return 1;
        //     }
        //     return 0;
        // };
        // SetSearcher(luaSearcher);
    }

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

    // 注册外部 VM 到 guard 表：让 Any Variant 的 deleter 能查询到 guard。
    // Unregister 不在 Shutdown 里调（外部 VM 由 host 管理生命周期），
    // 而是由 host close VM 之前显式调用 UnregisterLuaState(L)；
    // 如未调用也不会立刻崩溃——只是后续 Any Variant 析构时
    // luaL_unref 会作用于已关闭 VM。host 可通过 BP_*** C API 解决。
    RegisterLuaState(L);

    // 不调用 luaL_openlibs（外部 VM 已初始化，重复 open 可能覆盖全局表）
    // 只注册 Blueprint.* 绑定 + json/http 库
    RegisterLuaBindings(m_L, m_runner);
    RegisterLuaJsonHttpLibs(m_L);

    m_lastError.clear();
    return true;
}

void LuaScriptEngine::SetSearcher(lua_CFunction loader)
{
    if (!m_L || !loader) return;
    if (!IsLuaStateAlive(m_L)) { m_L = nullptr; return; }
    lua_State* L = m_L;
    int top = lua_gettop(L);

    lua_pushcfunction(L, loader);
    int loaderFunc = lua_gettop(L);
    lua_getglobal(L, "package");
    lua_getfield(L, -1, "searchers");
    int loaderTable = lua_gettop(L);
    // 把现有 searchers 整体后移一格
    for (lua_Integer e = (lua_Integer)lua_rawlen(L, loaderTable) + 1; e > 1; e--)
    {
        lua_rawgeti(L, loaderTable, (int)(e - 1));
        lua_rawseti(L, loaderTable, (int)e);
    }
    lua_pushvalue(L, loaderFunc);
    lua_rawseti(L, loaderTable, 1);

    lua_settop(L, top);
}

void LuaScriptEngine::AddLuaPath(const std::string& dir)
{
    if (!m_L || dir.empty()) return;
    if (!IsLuaStateAlive(m_L)) { m_L = nullptr; return; }
    lua_State* L = m_L;
    int top = lua_gettop(L);

    lua_getglobal(L, "package");
    lua_getfield(L, -1, "path");
    const char* cur = lua_tostring(L, -1);
    std::string newPath = std::string(cur ? cur : "");

    // 追加 dir/?.lua 和 dir/?/init.lua
    auto append = [&](const std::string& pat) {
        if (newPath.find(pat) == std::string::npos)
        {
            if (!newPath.empty()) newPath += ";";
            newPath += pat;
            fprintf(stdout, "AddLuaPath: %s\n", pat.c_str());
        }
    };
    append(dir + "/?.lua");
    append(dir + "/?/init.lua");

    lua_pop(L, 1);  // pop old path
    lua_pushstring(L, newPath.c_str());
    lua_setfield(L, -2, "path");

    lua_settop(L, top);
}

void LuaScriptEngine::InvalidateLuaState()
{
    if (m_L)
    {
        fprintf(stdout, "InvalidateLuaState to null\n");
        m_L = nullptr;
    }
    // m_ownsState 保持不变（false），确保 Shutdown 不会尝试 lua_close
}

void LuaScriptEngine::Shutdown()
{
    if (m_L)
    {
        fprintf(stdout, "LuaScriptEngine Shutdown\n");
        // 自有 VM：先 Unregister guard（让残留 Any Variant 析构变 no-op），
        // 再 lua_close。外部 VM 不动，host 自己负责 lifecycle。
        if (m_ownsState)
        {
            // 注销脚本注册的所有节点（否则下次创建 VM 后注册表里仍残留旧 handler）
            UnregisterAllScriptedNodes();
            UnregisterLuaState(m_L);
            lua_close(m_L);
        }
        m_L = nullptr;
    }
    m_runner = nullptr;
    m_ownsState = true;
    m_loadedFiles.clear();
    m_loadedCount = 0;
    m_registeredNodeIds.clear();
    m_fileMtimes.clear();
}

// ---------------------------------------------------------------------------
// Per-VM 注册元数据访问
// ---------------------------------------------------------------------------
bool LuaScriptEngine::HasLoadedFile(const std::string& filePath) const
{
    return std::find(m_loadedFiles.begin(), m_loadedFiles.end(), filePath) != m_loadedFiles.end();
}

int64_t LuaScriptEngine::GetFileMtime(const std::string& filePath) const
{
    auto it = m_fileMtimes.find(filePath);
    return (it != m_fileMtimes.end()) ? it->second : 0;
}

void LuaScriptEngine::SetFileMtime(const std::string& filePath, int64_t mtime)
{
    m_fileMtimes[filePath] = mtime;
}

void LuaScriptEngine::UnregisterAllScriptedNodes()
{
    // 进程退出时 registry 可能先析构，检查后跳过避免 use-after-free
    if (!HandlerRegistry::IsAlive() || !NodeDefRegistry::IsAlive())
        return;

    auto& nodeReg    = NodeDefRegistry::Instance();
    auto& handlerReg = HandlerRegistry::Instance();
    for (const auto& id : m_registeredNodeIds)
    {
        nodeReg.Unregister(id);
        handlerReg.Unregister(id);
    }
    m_registeredNodeIds.clear();
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
    if (!IsLuaStateAlive(m_L))
    {
        m_L = nullptr;
        m_lastError = "Lua VM has been closed by host";
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

    // 记录加载顺序（去重——共享 VM 多 Runner 时可能反复调进来）
    if (std::find(m_loadedFiles.begin(), m_loadedFiles.end(), filePath) == m_loadedFiles.end())
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
    if (!IsLuaStateAlive(m_L))
    {
        m_L = nullptr;
        m_lastError = "Lua VM has been closed by host";
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


void LuaScriptEngine::Tick(double deltaSeconds)
{
    if (!m_L) return;

    // 防御性检查：外部 host（如 xLua）可能已经 lua_close(m_L) 但忘记调
    // BP_NotifyLuaStateClosing；或者 InvalidateLuaState 没命中本 engine。
    // 通过全局 LuaStateRegistry 的 guard 判断 VM 是否仍存活，避免访问野指针。
    if (!IsLuaStateAlive(m_L))
    {
        m_L = nullptr;  // 主动同步，避免后续重复检查
        return;
    }

    // 查找全局 onTick 函数
    lua_getglobal(m_L, "OnGlobalTick");
    if (!lua_isfunction(m_L, -1))
    {
        lua_pop(m_L, 1);   // onTick 不存在，忽略
        return;
    }

    lua_pushnumber(m_L, deltaSeconds);

    // pcall with 1 arg, 0 results
    if (lua_pcall(m_L, 1, 0, 0) != LUA_OK)
    {
        const char* err = lua_tostring(m_L, -1);
        m_lastError = err ? err : "OnGlobalTick error";
        lua_pop(m_L, 1);
    }
}

// =========================================================================
// LuaScriptEngineRegistry：进程级默认共享 Engine
// =========================================================================
// GetDefault() 先 TouchLuaStateRegistry() 强制 guard 表先构造，保证析构顺序：
// default engine 先死 → Shutdown → UnregisterLuaState → guard 表仍存活。
namespace {
    struct DefaultEngineHolder {
        std::mutex                       mutex;
        std::shared_ptr<LuaScriptEngine> engine;
        static DefaultEngineHolder& Instance() { static DefaultEngineHolder h; return h; }
    };
}

std::shared_ptr<LuaScriptEngine> LuaScriptEngineRegistry::GetDefault(bool skipCreate)
{
    TouchLuaStateRegistry();

    auto& holder = DefaultEngineHolder::Instance();
    std::lock_guard<std::mutex> lk(holder.mutex);
    if (!holder.engine && !skipCreate)
    {
        holder.engine = std::make_shared<LuaScriptEngine>();
        if (!holder.engine->Initialize(nullptr))
            holder.engine.reset();
    }
    return holder.engine;
}

void LuaScriptEngineRegistry::SetDefault(std::shared_ptr<LuaScriptEngine> engine)
{
    TouchLuaStateRegistry();
    auto& holder = DefaultEngineHolder::Instance();
    std::lock_guard<std::mutex> lk(holder.mutex);
    holder.engine = std::move(engine);
}

bool LuaScriptEngineRegistry::HasDefault()
{
    auto& holder = DefaultEngineHolder::Instance();
    std::lock_guard<std::mutex> lk(holder.mutex);
    return holder.engine != nullptr;
}

} // namespace Runtime
} // namespace NodeEditor

#endif // BLUEPRINT_HAS_LUA
