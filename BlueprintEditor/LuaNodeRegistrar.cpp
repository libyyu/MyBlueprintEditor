// BlueprintEditor/LuaNodeRegistrar.cpp
// Phase 3：编辑器侧 Lua 节点注册器实现

#ifdef BLUEPRINT_HAS_LUA


#include "LuaNodeRegistrar.h"
#include "../Runtime/NodeDefinition.h"
#include "../Runtime/BlueprintRunner.h"  // NodeHandler / ExecutionContext
#include "../Runtime/Types.h"
#include "../Runtime/LuaBindings.h"
#include "BpLogger.h"
#include <lua.hpp>
#include <filesystem>
#include <chrono>
#include <sstream>
#include <cstring>

#ifdef _WIN32
#include <windows.h>
#ifdef LoadString
#  undef LoadString
#endif
#endif

namespace fs = std::filesystem;

// ── 析构函数 ──────────────────────────────────────────────────────────────
LuaNodeRegistrar::~LuaNodeRegistrar()
{
    resetLuaState();
}

// ============================================================================
// 内部：Lua 工具
// ============================================================================

// 从 Lua table 中读取 string 字段（栈上 table 索引为 tableIdx）
static std::string tableGetString(lua_State* L, int tableIdx, const char* key,
                                  const std::string& def = "")
{
    lua_getfield(L, tableIdx, key);
    std::string result = def;
    if (lua_isstring(L, -1))
        result = lua_tostring(L, -1);
    lua_pop(L, 1);
    return result;
}

static bool tableGetBool(lua_State* L, int tableIdx, const char* key, bool def = false)
{
    lua_getfield(L, tableIdx, key);
    bool result = def;
    if (lua_isboolean(L, -1))
        result = lua_toboolean(L, -1) != 0;
    lua_pop(L, 1);
    return result;
}

// 将 Lua string → PinDataType
static NodeEditor::Runtime::PinDataType parsePinDataType(const std::string& s)
{
    using namespace NodeEditor::Runtime;
    if (s == "bool"    || s == "boolean") return PinDataType::Boolean;
    if (s == "int"     || s == "integer") return PinDataType::Integer;
    if (s == "float"   || s == "number")  return PinDataType::Float;
    if (s == "string")                    return PinDataType::String;
    if (s == "object")                    return PinDataType::Object;
    if (s == "array")                     return PinDataType::Array;
    if (s == "map")                       return PinDataType::Map;
    if (s == "set")                       return PinDataType::Set;
    if (s == "flow"    || s == "exec")    return PinDataType::Unknown; // Flow 由 isExec 标志控制
    return PinDataType::Unknown;
}

// 从 Lua table (栈顶) 解析引脚定义列表
static std::vector<NodeEditor::Runtime::PinDefinition> parsePinList(
    lua_State* L, int tableIdx, NodeEditor::Runtime::PinKind kind)
{
    using namespace NodeEditor::Runtime;
    std::vector<PinDefinition> result;

    if (!lua_istable(L, tableIdx)) return result;

    int len = static_cast<int>(lua_rawlen(L, tableIdx));
    for (int i = 1; i <= len; ++i)
    {
        lua_rawgeti(L, tableIdx, i);   // 压入 pins[i]
        if (!lua_istable(L, -1)) { lua_pop(L, 1); continue; }

        PinDefinition pin;
        pin.kind    = kind;
        pin.name    = tableGetString(L, -1, "name");
        pin.tooltip = tableGetString(L, -1, "tooltip");

        std::string typeStr = tableGetString(L, -1, "type", "float");
        pin.isExec    = (typeStr == "flow" || typeStr == "exec");
        pin.dataType  = pin.isExec ? PinDataType::Unknown : parsePinDataType(typeStr);
        pin.isRequired = tableGetBool(L, -1, "required", false);

        // defaultValue
        lua_getfield(L, -1, "default");
        if (!lua_isnil(L, -1))
        {
            switch (lua_type(L, -1)) {
            case LUA_TBOOLEAN: pin.defaultValue = Variant(static_cast<bool>(lua_toboolean(L, -1))); break;
            case LUA_TNUMBER:
                if (lua_isinteger(L, -1))
                    pin.defaultValue = Variant(static_cast<int64_t>(lua_tointeger(L, -1)));
                else
                    pin.defaultValue = Variant(lua_tonumber(L, -1));
                break;
            case LUA_TSTRING:
                pin.defaultValue = Variant(std::string(lua_tostring(L, -1)));
                break;
            default: break;
            }
        }
        lua_pop(L, 1);  // pop default

        if (!pin.name.empty())
            result.push_back(std::move(pin));

        lua_pop(L, 1);  // pop pins[i]
    }
    return result;
}

// ============================================================================
// Lua API 实现：Blueprint.RegisterNode(def_table)
//
// Lua 调用示例：
//   Blueprint.RegisterNode({
//       id       = "MyMath.Lerp",
//       name     = "Lerp",
//       category = "MyMath",
//       description = "线性插值",
//       color    = "3A7A3A",
//       icon     = "\uF53F",
//       inputs   = {
//           { name="A",     type="float", default=0.0 },
//           { name="B",     type="float", default=1.0 },
//           { name="Alpha", type="float", default=0.5 },
//       },
//       outputs  = {
//           { name="Result", type="float" },
//       },
//   })
// ============================================================================

static int l_registerNode(lua_State* L)
{
    using namespace NodeEditor::Runtime;

    luaL_checktype(L, 1, LUA_TTABLE);
    int tableIdx = 1;

    // 获取 registry 指针（存在 Lua registry 中）
    lua_getfield(L, LUA_REGISTRYINDEX, "__editor_node_registry");
    auto* registry = static_cast<INodeRegistry*>(lua_touserdata(L, -1));
    lua_pop(L, 1);

    // 获取 handlerMap 指针
    lua_getfield(L, LUA_REGISTRYINDEX, "__editor_handler_map");
    auto* handlerMap = static_cast<std::unordered_map<std::string, NodeHandler>*>(
        lua_touserdata(L, -1));
    lua_pop(L, 1);

    // 获取 registrar 指针（用于记录 registeredIds）
    lua_getfield(L, LUA_REGISTRYINDEX, "__editor_registrar");
    auto* registrar = static_cast<LuaNodeRegistrar*>(lua_touserdata(L, -1));
    lua_pop(L, 1);

    if (!registry)
        return luaL_error(L, "Blueprint.RegisterNode: registry not initialized");

    NodeDefinition def;
    def.id          = tableGetString(L, tableIdx, "id");
    def.name        = tableGetString(L, tableIdx, "name");
    def.category    = tableGetString(L, tableIdx, "category");
    def.description = tableGetString(L, tableIdx, "description");
    def.color       = tableGetString(L, tableIdx, "color");
    def.icon        = tableGetString(L, tableIdx, "icon");
    def.isPure      = tableGetBool(L, tableIdx, "pure", true);

    if (def.id.empty())
        return luaL_error(L, "Blueprint.RegisterNode: 'id' is required");
    if (def.name.empty())
        def.name = def.id;

    // 输入引脚
    lua_getfield(L, tableIdx, "inputs");
    def.inputPins = parsePinList(L, lua_gettop(L), PinKind::Input);
    lua_pop(L, 1);

    // 输出引脚
    lua_getfield(L, tableIdx, "outputs");
    def.outputPins = parsePinList(L, lua_gettop(L), PinKind::Output);
    lua_pop(L, 1);

    // 注册到 registry
    if (!registry->registerNode(def))
    {
        // 已存在同名节点时覆盖（先注销再注册）
        registry->unregisterNode(def.id);
        registry->registerNode(def);
    }

    // 记录已注册 ID（用于热重载时清理）
    if (registrar)
    {
        registrar->GetRegisteredIds_Mutable().insert(def.id);
        // 同时将 id 归入当前加载文件的节点集合（用于单文件精确热重载）
        const std::string& loadingFile = registrar->GetCurrentLoadingFile();
        if (!loadingFile.empty())
            registrar->GetFileNodeIds_Mutable()[loadingFile].insert(def.id);
    }

    // 如果 Lua 脚本同时提供了 handler 函数（第二个参数），自动注册 handler
    if (lua_isfunction(L, 2) && handlerMap)
    {
        // 获取 runner 用于 wrapLuaHandler（存在 registry 中）
        lua_getfield(L, LUA_REGISTRYINDEX, "__blueprint_runner_editor");
        auto* runner = static_cast<BlueprintRunner*>(lua_touserdata(L, -1));
        lua_pop(L, 1);

        if (runner)
        {
            lua_pushvalue(L, 2);
            int funcRef = luaL_ref(L, LUA_REGISTRYINDEX);

            // 创建 handler lambda（与 LuaBindings.cpp 中的 wrapLuaHandler 相同逻辑）
            lua_State* capturedL = L;
            NodeHandler handler = [capturedL, funcRef](ExecutionContext& ctx) -> bool {
                lua_rawgeti(capturedL, LUA_REGISTRYINDEX, funcRef);
                auto** udata = static_cast<ExecutionContext**>(
                    lua_newuserdata(capturedL, sizeof(ExecutionContext*)));
                *udata = &ctx;
                luaL_setmetatable(capturedL, "Blueprint.ExecutionContext");

                if (lua_pcall(capturedL, 1, 1, 0) != LUA_OK) {
                    const char* err = lua_tostring(capturedL, -1);
                    ctx.PrintError(std::string("[Lua] ") + (err ? err : "unknown error"));
                    lua_pop(capturedL, 1);
                    return false;
                }
                bool result = lua_isboolean(capturedL, -1) ? lua_toboolean(capturedL, -1) : true;
                lua_pop(capturedL, 1);
                return result;
            };

            (*handlerMap)[def.id] = std::move(handler);
        }
    }

    // 递增计数（registrar 的 m_pendingCount）
    if (registrar)
        registrar->IncrPendingCount();

    return 0;
}
static std::string on_print_handler(lua_State* L)
{
	static char sL[20] = { 0 };
	memset(sL, 0x0, sizeof(sL));
	snprintf(sL, 20, "%p|", L);
    static std::string message;
    message = sL;
	int n = lua_gettop(L);
	lua_getglobal(L, "tostring");

	for (int i = 1; i <= n; ++i)
	{
		lua_pushvalue(L, -1); // function to be called
		lua_pushvalue(L, i);  // value to print
		lua_pcall(L, 1, 1, 0);

		const char* ret = lua_tostring(L, -1);
		if (ret)
            message.append(ret);

		if (i < n)
            message.append("\t");

		lua_pop(L, 1); //pop result
	}

	return std::move(message);
}
static int l_panic(lua_State* L)
{
	char sL[20] = { 0 };
	snprintf(sL, 20, "%p|", L);
	std::string reason(sL);
	reason += "unprotected error in call to Lua API (";
	const char* s = lua_tostring(L, -1);
	reason += s ? s : "?";
	reason += ")";

    // 尝试通过 registrar 的 logCallback 输出
    lua_getfield(L, LUA_REGISTRYINDEX, "__editor_registrar");
    auto* registrar = static_cast<LuaNodeRegistrar*>(lua_touserdata(L, -1));
    lua_pop(L, 1);
    if (registrar && registrar->GetLogCallback())
        registrar->GetLogCallback()(2, "[LUA PANIC] " + reason);
    else
        BPERROR("[LUA PANIC] " + reason);
#if defined(_WIN32) && defined(_DEBUG)
	OutputDebugStringA(("[LUA PANIC] " + reason + "\n").c_str());
#endif
	throw std::runtime_error(reason);
	return 0;
}
static int l_print(lua_State* L)
{
	std::string s = on_print_handler(L);

    lua_getfield(L, LUA_REGISTRYINDEX, "__editor_registrar");
    auto* registrar = static_cast<LuaNodeRegistrar*>(lua_touserdata(L, -1));
    lua_pop(L, 1);
    if (registrar && registrar->GetLogCallback())
        registrar->GetLogCallback()(0, "[Lua] " + s);
    else
        BPLOG("[Lua] " + s);

#if defined(_WIN32) && defined(_DEBUG)
	OutputDebugStringA(("[Lua] " + s + "\n").c_str());
#endif

	return 0;
}
static int l_warn(lua_State* L)
{
	std::string s = on_print_handler(L);

    lua_getfield(L, LUA_REGISTRYINDEX, "__editor_registrar");
    auto* registrar = static_cast<LuaNodeRegistrar*>(lua_touserdata(L, -1));
    lua_pop(L, 1);
    if (registrar && registrar->GetLogCallback())
        registrar->GetLogCallback()(1, "[Lua WARN] " + s);
    else
        BPWARN("[Lua WARN] " + s);

#if defined(_WIN32) && defined(_DEBUG)
	OutputDebugStringA(("[Lua] " + s + "\n").c_str());
#endif

	return 0;
}

// ============================================================================
// LuaNodeRegistrar 实现
// ============================================================================

void LuaNodeRegistrar::Initialize(NodeEditor::Runtime::INodeRegistry* registry,
                                   std::unordered_map<std::string, NodeEditor::Runtime::NodeHandler>* handlerMap)
{
    m_registry   = registry;
    m_handlerMap = handlerMap;
}

bool LuaNodeRegistrar::ensureLuaState()
{
    if (m_L) return true;

    m_L = luaL_newstate();
    if (!m_L) { m_lastError = "Failed to create Lua state"; return false; }
    lua_atpanic(m_L, l_panic);
    luaL_openlibs(m_L);

    lua_register(m_L, "print", l_print);
    lua_register(m_L, "warn", l_warn);
	
    // 注册运行时 Lua 绑定（Variant + ExecutionContext metatables）
    // 需要一个临时 runner 只为注册 metatables 用
    // 注：Blueprint.RegisterHandler 不在编辑器侧使用，但 metatables 需要
    lua_pushlightuserdata(m_L, nullptr);  // runner=null，RegisterHandler 不可用
    lua_setfield(m_L, LUA_REGISTRYINDEX, "__blueprint_runner");

    registerEditorBindings(m_L);

    return true;
}

void LuaNodeRegistrar::registerEditorBindings(lua_State* L)
{
    // 存储指针到 Lua registry
    lua_pushlightuserdata(L, m_registry);
    lua_setfield(L, LUA_REGISTRYINDEX, "__editor_node_registry");

    lua_pushlightuserdata(L, m_handlerMap);
    lua_setfield(L, LUA_REGISTRYINDEX, "__editor_handler_map");

    lua_pushlightuserdata(L, this);
    lua_setfield(L, LUA_REGISTRYINDEX, "__editor_registrar");

    lua_pushlightuserdata(L, nullptr);  // runner for handler wrapping (null = no handler)
    lua_setfield(L, LUA_REGISTRYINDEX, "__blueprint_runner_editor");

    // 向 Blueprint 全局表追加 RegisterNode
    lua_getglobal(L, "Blueprint");
    if (!lua_istable(L, -1))
    {
        // 若 Blueprint 表不存在则新建（纯编辑器模式，没有 LuaBindings.cpp 的初始化）
        lua_pop(L, 1);
        lua_newtable(L);
    }

    lua_pushcfunction(L, l_registerNode);
    lua_setfield(L, -2, "RegisterNode");

    lua_setglobal(L, "Blueprint");

    // 注册运行时绑定（Variant / ExecutionContext metatables）以便 Lua 脚本引用
    // 用一个临时 dummy runner（nullptr 安全）
    NodeEditor::Runtime::RegisterLuaBindings(L, nullptr);
}

void LuaNodeRegistrar::resetLuaState()
{
    if (m_L)
    {
        lua_close(m_L);
        m_L = nullptr;
    }
}

int LuaNodeRegistrar::executeFile(const std::string& filePath)
{
    m_pendingCount = 0;

    // 设置当前加载文件路径，l_registerNode 回调用它归因节点 id 到文件
    m_currentLoadingFile = filePath;

    if (luaL_loadfile(m_L, filePath.c_str()) != LUA_OK)
    {
        const char* err = lua_tostring(m_L, -1);
        m_lastError = "Load error [" + filePath + "]: " + (err ? err : "unknown");
        lua_pop(m_L, 1);
        return -1;
    }

    // 带 traceback 的 pcall
    lua_pushcfunction(m_L, [](lua_State* L) -> int {
        const char* msg = lua_tostring(L, 1);
        luaL_traceback(L, L, msg, 1);
        return 1;
    });
    lua_insert(m_L, -2);  // 把 errFunc 移到 chunk 之前

    if (lua_pcall(m_L, 0, 0, lua_gettop(m_L) - 1) != LUA_OK)
    {
        const char* err = lua_tostring(m_L, -1);
        m_lastError = "Runtime error [" + filePath + "]: " + (err ? err : "unknown");
        lua_pop(m_L, 2);
        m_currentLoadingFile.clear();
        return -1;
    }
    lua_pop(m_L, 1);  // pop errFunc

    m_currentLoadingFile.clear();
    m_lastError.clear();
    return m_pendingCount;
}

int LuaNodeRegistrar::executeString(const std::string& code, const std::string& chunkName)
{
    m_pendingCount = 0;

    if (luaL_loadbuffer(m_L, code.c_str(), code.size(), chunkName.c_str()) != LUA_OK)
    {
        const char* err = lua_tostring(m_L, -1);
        m_lastError = "Load error [" + chunkName + "]: " + (err ? err : "unknown");
        lua_pop(m_L, 1);
        return -1;
    }

    lua_pushcfunction(m_L, [](lua_State* L) -> int {
        const char* msg = lua_tostring(L, 1);
        luaL_traceback(L, L, msg, 1);
        return 1;
    });
    lua_insert(m_L, -2);

    if (lua_pcall(m_L, 0, 0, lua_gettop(m_L) - 1) != LUA_OK)
    {
        const char* err = lua_tostring(m_L, -1);
        m_lastError = "Runtime error [" + chunkName + "]: " + (err ? err : "unknown");
        lua_pop(m_L, 2);
        return -1;
    }
    lua_pop(m_L, 1);

    m_lastError.clear();
    return m_pendingCount;
}

void LuaNodeRegistrar::SetSearcher(lua_CFunction loader)
{
    if (!ensureLuaState() || !loader) return;
    lua_State* L = m_L;
    int top = lua_gettop(L);

    lua_pushcfunction(L, loader);
    int loaderFunc = lua_gettop(L);
    lua_getglobal(L, "package");
    lua_getfield(L, -1, "searchers");
    int loaderTable = lua_gettop(L);
    for (lua_Integer e = (lua_Integer)lua_rawlen(L, loaderTable) + 1; e > 1; e--)
    {
        lua_rawgeti(L, loaderTable, (int)(e - 1));
        lua_rawseti(L, loaderTable, (int)e);
    }
    lua_pushvalue(L, loaderFunc);
    lua_rawseti(L, loaderTable, 1);

    lua_settop(L, top);
}

void LuaNodeRegistrar::AddLuaPath(const std::string& dir)
{
    if (!ensureLuaState() || dir.empty()) return;
    lua_State* L = m_L;
    int top = lua_gettop(L);

    lua_getglobal(L, "package");
    lua_getfield(L, -1, "path");
    const char* cur = lua_tostring(L, -1);
    std::string newPath(cur ? cur : "");

    auto append = [&](const std::string& pat) {
        if (newPath.find(pat) == std::string::npos)
        {
            if (!newPath.empty()) newPath += ";";
            newPath += pat;
        }
    };
    append(dir + "/?.lua");
    append(dir + "/?/init.lua");

    lua_pop(L, 1);
    lua_pushstring(L, newPath.c_str());
    lua_setfield(L, -2, "path");

    lua_settop(L, top);
}

bool LuaNodeRegistrar::LoadEntrySilent(const std::string& filePath, const std::string& chunkName)
{
    if (!ensureLuaState() || filePath.empty()) return false;
    if (!fs::exists(filePath)) return false;  // 文件不存在静默跳过

    lua_State* L = m_L;
    int top = lua_gettop(L);

    // 用指定 chunkName（或文件路径）标识，避免与其他同名脚本混淆
    const std::string& name = chunkName.empty() ? filePath : chunkName;

    // luaL_loadfilex 加载，错误函数包裹执行
    if (luaL_loadfilex(L, filePath.c_str(), nullptr) != LUA_OK)
    {
        // 语法错误静默忽略（不影响编辑器启动）
        lua_pop(L, 1);
        lua_settop(L, top);
        return false;
    }

    // 设置 chunk 名（@前缀表示文件名，此处用自定义 name 覆盖）
    // luaL_loadfilex 已设置 chunk 名为 @filePath，若需覆盖需要额外操作；
    // 直接执行即可——chunk 名仅用于错误信息，不影响隔离性
    (void)name;  // chunk 名已由调用方通过 chunkName 语义区分

    // 错误处理函数
    lua_pushcfunction(L, [](lua_State* Ls) -> int {
        return 1;  // 直接返回错误对象，外层 pcall 捕获
    });
    lua_insert(L, -2);  // errFunc 置于 chunk 前

    if (lua_pcall(L, 0, 0, lua_gettop(L) - 1) != LUA_OK)
    {
        // 运行时错误静默忽略
        lua_pop(L, 1);
    }
    lua_pop(L, 1);  // pop errFunc
    lua_settop(L, top);
    return true;
}

void LuaNodeRegistrar::WatchEntryScript(const std::string& filePath, const std::string& chunkName)
{
    if (filePath.empty()) return;

    // 同一路径已存在则更新（工程切换时重置 loaded 状态）
    for (auto& w : m_entryWatches)
    {
        if (w.filePath == filePath)
        {
            w.chunkName = chunkName;
            w.loaded    = false;  // 重置，强制重新加载
            break;
        }
    }
    // 新增监视项
    bool found = false;
    for (auto& w : m_entryWatches)
        if (w.filePath == filePath) { found = true; break; }
    if (!found)
        m_entryWatches.push_back({ filePath, chunkName, false });

    // 立即尝试加载（若文件已存在）
    if (fs::exists(filePath))
    {
        LoadEntrySilent(filePath, chunkName);
        for (auto& w : m_entryWatches)
            if (w.filePath == filePath) { w.loaded = true; break; }
    }
}

int LuaNodeRegistrar::LoadScript(const std::string& filePath)
{
    if (!m_registry) { m_lastError = "Not initialized"; return -1; }
    if (!ensureLuaState()) return -1;

    int n = executeFile(filePath);
    if (n < 0) return -1;

    // 记录加载列表（去重）
    if (std::find(m_loadedFiles.begin(), m_loadedFiles.end(), filePath) == m_loadedFiles.end())
        m_loadedFiles.push_back(filePath);

    // 记录文件修改时间
    try {
        auto t = fs::last_write_time(filePath);
        m_fileModTimes[filePath] = t.time_since_epoch().count();
    } catch (...) {}

    return n;
}

int LuaNodeRegistrar::LoadString(const std::string& code, const std::string& chunkName)
{
    if (!m_registry) { m_lastError = "Not initialized"; return -1; }
    if (!ensureLuaState()) return -1;
    return executeString(code, chunkName);
}

void LuaNodeRegistrar::UnregisterAll()
{
    if (!m_registry) return;
    for (const auto& id : m_registeredIds)
        m_registry->unregisterNode(id);

    if (m_handlerMap)
    {
        for (const auto& id : m_registeredIds)
            m_handlerMap->erase(id);
    }

    m_registeredIds.clear();
    m_fileNodeIds.clear();  // 清空文件→节点id映射
}

int LuaNodeRegistrar::ReloadAll()
{
    if (!m_registry || m_loadedFiles.empty()) return 0;

    // 1. 清除已注册的旧定义
    UnregisterAll();

    // 2. 重置 Lua VM（清除所有函数引用）
    resetLuaState();

    // 3. 重新加载所有文件
    auto files = m_loadedFiles;
    m_loadedFiles.clear();

    int total = 0;
    for (const auto& f : files)
    {
        int n = LoadScript(f);
        if (n >= 0) total += n;
    }
    return total;
}

int LuaNodeRegistrar::ReloadFile(const std::string& filePath)
{
    if (!m_registry) return -1;

    // 精确单文件热重载：
    // 1. 只卸载该文件注册的节点定义（不影响其他文件）
    auto fit = m_fileNodeIds.find(filePath);
    if (fit != m_fileNodeIds.end())
    {
        for (const auto& id : fit->second)
        {
            m_registry->unregisterNode(id);
            m_registeredIds.erase(id);
            if (m_handlerMap) m_handlerMap->erase(id);
        }
        fit->second.clear();
    }

    // 2. 重新执行该文件（Lua VM 保持，其他文件的函数引用不受影响）
    int n = executeFile(filePath);
    if (n < 0) return -1;

    // 3. 更新文件修改时间
    try {
        auto t = fs::last_write_time(filePath);
        m_fileModTimes[filePath] = t.time_since_epoch().count();
    } catch (...) {}

    return n;
}

void LuaNodeRegistrar::PollFileChanges()
{
    // ── 热重载：已加载脚本变更检测 ───────────────────────────────────────
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
                    // 单文件精确重载，不影响其他文件的注册状态
                    ReloadFile(f);
                }
            } catch (...) {}
        }
    }

    // ── Entry Script Watcher：轮询未加载的入口脚本 ───────────────────────
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

void LuaNodeRegistrar::Tick(float deltaTime)
{
    if (!m_L) return;

    lua_getglobal(m_L, "OnGlobalTick");
    if (!lua_isfunction(m_L, -1))
    {
        lua_pop(m_L, 1);
        return;
    }

    lua_pushnumber(m_L, static_cast<double>(deltaTime));
    if (lua_pcall(m_L, 1, 0, 0) != LUA_OK)
    {
        const char* err = lua_tostring(m_L, -1);
        std::string msg = err ? err : "OnGlobalTick error";
        if (m_logCallback)
            m_logCallback(2, "[Lua ERROR] " + msg);
        else
            BPERROR("[Lua ERROR] " + msg);
        lua_pop(m_L, 1);
    }
}

#endif // BLUEPRINT_HAS_LUA

