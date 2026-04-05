// Runtime/LuaBindings.cpp -- Lua ↔ C++ 绑定实现
//
// 内容：
//   · Variant userdata metatable（asBool/asInt/asFloat/asString + __gc）
//   · ExecutionContext userdata metatable（GetInput/SetOutput/...）
//   · Variant ↔ Lua 类型双向转换（pushVariant / toVariant）
//   · Blueprint.RegisterHandler(definitionId, luaFunction)
//   · wrapLuaHandler: Lua function → C++ NodeHandler
//   · json.*  全局库（parse/stringify/get/set）
//   · http.*  全局库（get/post — 同步包装，仅限非 Emscripten）

#ifdef BLUEPRINT_HAS_LUA

#include "LuaBindings.h"
#include "BlueprintRunner.h"
#include "Http/IHttpClient.h"
#include "../../Utils/Json/crude_json.h"

#include <lua.hpp>
#include <new>       // placement new
#include <string>
#include <vector>
#include <unordered_map>
#include <mutex>
#include <condition_variable>
#include <functional>

namespace NodeEditor {
namespace Runtime {

// =========================================================================
// Metatable 名称常量
// =========================================================================
static const char* VARIANT_MT = "Blueprint.Variant";
static const char* CTX_MT     = "Blueprint.ExecutionContext";

// =========================================================================
// Variant → Lua 压栈（原生 Lua 类型，非 userdata）
// =========================================================================
static void pushVariantRaw(lua_State* L, const Variant& v)
{
    switch (v.type) {
    case PinDataType::Boolean: lua_pushboolean(L, v.asBool());                   break;
    case PinDataType::Integer: lua_pushinteger(L, static_cast<lua_Integer>(v.asInt())); break;
    case PinDataType::Float:   lua_pushnumber(L, v.asFloat());                   break;
    case PinDataType::String:  lua_pushstring(L, v.asString().c_str());          break;
    case PinDataType::Object:  lua_pushstring(L, v.asObjectId().c_str());        break;
    case PinDataType::Array: {
        lua_createtable(L, static_cast<int>(v.arraySize()), 0);
        for (size_t i = 0; i < v.arraySize(); ++i) {
            pushVariantRaw(L, v.arrayGet(i));
            lua_rawseti(L, -2, static_cast<int>(i + 1));
        }
        break;
    }
    case PinDataType::Map: {
        lua_createtable(L, 0, static_cast<int>(v.mapSize()));
        for (const auto& kv : v.asMap()) {
            // kv.first は Variant — Lua テーブルキーは文字列に変換
            std::string keyStr = kv.first.asString();
            lua_pushstring(L, keyStr.c_str());
            pushVariantRaw(L, kv.second);
            lua_rawset(L, -3);
        }
        break;
    }
    case PinDataType::Set: {
        lua_createtable(L, static_cast<int>(v.setSize()), 0);
        auto elements = v.setToArray();
        for (size_t i = 0; i < elements.size(); ++i) {
            pushVariantRaw(L, elements[i]);
            lua_rawseti(L, -2, static_cast<int>(i + 1));
        }
        break;
    }
    default: lua_pushnil(L); break;
    }
}

// =========================================================================
// Lua 栈值 → Variant
// =========================================================================
static Variant toVariant(lua_State* L, int idx)
{
    switch (lua_type(L, idx)) {
    case LUA_TBOOLEAN:
        return Variant(static_cast<bool>(lua_toboolean(L, idx)));
    case LUA_TNUMBER:
        if (lua_isinteger(L, idx))
            return Variant(static_cast<int64_t>(lua_tointeger(L, idx)));
        else
            return Variant(lua_tonumber(L, idx));
    case LUA_TSTRING:
        return Variant(std::string(lua_tostring(L, idx)));
    case LUA_TTABLE: {
        // 启发式：若 key 1 存在则视为数组，否则视为 Map
        int absIdx = lua_absindex(L, idx);
        lua_rawgeti(L, absIdx, 1);
        bool isArray = !lua_isnil(L, -1);
        lua_pop(L, 1);

        if (isArray) {
            std::vector<Variant> arr;
            int len = static_cast<int>(lua_rawlen(L, absIdx));
            arr.reserve(static_cast<size_t>(len));
            for (int i = 1; i <= len; ++i) {
                lua_rawgeti(L, absIdx, i);
                arr.push_back(toVariant(L, -1));
                lua_pop(L, 1);
            }
            return Variant(std::move(arr));
        } else {
            Variant result;
            result.type = PinDataType::Map;
            lua_pushnil(L);
            while (lua_next(L, absIdx) != 0) {
                if (lua_type(L, -2) == LUA_TSTRING) {
                    Variant keyVar(std::string(lua_tostring(L, -2)));
                    result.mapSet(keyVar, toVariant(L, -1));
                }
                lua_pop(L, 1);
            }
            return result;
        }
    }
    case LUA_TUSERDATA: {
        // 如果是 Variant userdata，直接返回副本
        auto* vp = static_cast<Variant*>(luaL_testudata(L, idx, VARIANT_MT));
        if (vp) return *vp;
        return Variant();
    }
    default:
        return Variant();
    }
}

// =========================================================================
// Variant userdata 辅助
// =========================================================================

static Variant* pushNewVariant(lua_State* L, const Variant& v)
{
    void* mem = lua_newuserdata(L, sizeof(Variant));
    auto* uv = new (mem) Variant(v);   // placement new
    luaL_setmetatable(L, VARIANT_MT);
    return uv;
}

static Variant* checkVariant(lua_State* L, int idx)
{
    return static_cast<Variant*>(luaL_checkudata(L, idx, VARIANT_MT));
}

// =========================================================================
// Variant metatable 方法
// =========================================================================

static int variant_asBool(lua_State* L)
{
    lua_pushboolean(L, checkVariant(L, 1)->asBool());
    return 1;
}

static int variant_asInt(lua_State* L)
{
    lua_pushinteger(L, static_cast<lua_Integer>(checkVariant(L, 1)->asInt()));
    return 1;
}

static int variant_asFloat(lua_State* L)
{
    lua_pushnumber(L, checkVariant(L, 1)->asFloat());
    return 1;
}

static int variant_asString(lua_State* L)
{
    lua_pushstring(L, checkVariant(L, 1)->asString().c_str());
    return 1;
}

static int variant_tostring(lua_State* L)
{
    lua_pushstring(L, checkVariant(L, 1)->asString().c_str());
    return 1;
}

static int variant_gc(lua_State* L)
{
    checkVariant(L, 1)->~Variant();
    return 0;
}

static void registerVariantMetatable(lua_State* L)
{
    luaL_newmetatable(L, VARIANT_MT);

    // mt.__index = mt
    lua_pushvalue(L, -1);
    lua_setfield(L, -2, "__index");

    static const luaL_Reg methods[] = {
        {"asBool",     variant_asBool},
        {"asInt",      variant_asInt},
        {"asFloat",    variant_asFloat},
        {"asString",   variant_asString},
        {"__tostring", variant_tostring},
        {"__gc",       variant_gc},
        {nullptr, nullptr}
    };
    luaL_setfuncs(L, methods, 0);
    lua_pop(L, 1);
}

// =========================================================================
// ExecutionContext userdata 方法
// =========================================================================

static ExecutionContext* checkCtx(lua_State* L, int idx)
{
    return *static_cast<ExecutionContext**>(luaL_checkudata(L, idx, CTX_MT));
}

static int ctx_getInput(lua_State* L)
{
    auto* ctx = checkCtx(L, 1);
    const char* name = luaL_checkstring(L, 2);
    pushNewVariant(L, ctx->GetInputValue(name));
    return 1;
}

static int ctx_setOutput(lua_State* L)
{
    auto* ctx = checkCtx(L, 1);
    const char* name = luaL_checkstring(L, 2);

    // 支持 Variant userdata 或原生 Lua 类型
    Variant val;
    if (luaL_testudata(L, 3, VARIANT_MT))
        val = *checkVariant(L, 3);
    else
        val = toVariant(L, 3);

    ctx->SetOutputValue(name, val);
    return 0;
}

static int ctx_getVariable(lua_State* L)
{
    auto* ctx = checkCtx(L, 1);
    const char* name = luaL_checkstring(L, 2);
    pushNewVariant(L, ctx->GetVariable(name));
    return 1;
}

static int ctx_setVariable(lua_State* L)
{
    auto* ctx = checkCtx(L, 1);
    const char* name = luaL_checkstring(L, 2);
    ctx->SetVariable(name, toVariant(L, 3));
    return 0;
}

static int ctx_activateOutputFlow(lua_State* L)
{
    auto* ctx = checkCtx(L, 1);
    const char* pinName = luaL_checkstring(L, 2);
    lua_pushboolean(L, ctx->ActivateOutputFlow(pinName));
    return 1;
}

static int ctx_print(lua_State* L)
{
    auto* ctx = checkCtx(L, 1);
    const char* msg = luaL_checkstring(L, 2);
    ctx->Print(msg);
    return 0;
}

static int ctx_log(lua_State* L)
{
    auto* ctx = checkCtx(L, 1);
    const char* msg = luaL_checkstring(L, 2);
    ctx->Log(msg);
    return 0;
}

static int ctx_logWarning(lua_State* L)
{
    auto* ctx = checkCtx(L, 1);
    const char* msg = luaL_checkstring(L, 2);
    ctx->LogWarning(msg);
    return 0;
}

static int ctx_logError(lua_State* L)
{
    auto* ctx = checkCtx(L, 1);
    const char* msg = luaL_checkstring(L, 2);
    ctx->LogError(msg);
    return 0;
}

static int ctx_getCurrentNode(lua_State* L)
{
    auto* ctx = checkCtx(L, 1);
    const auto* node = ctx->GetCurrentNode();
    if (node) {
        lua_createtable(L, 0, 3);
        lua_pushinteger(L, static_cast<lua_Integer>(node->id));
        lua_setfield(L, -2, "id");
        lua_pushstring(L, node->definitionId.c_str());
        lua_setfield(L, -2, "definitionId");
        lua_pushstring(L, node->name.c_str());
        lua_setfield(L, -2, "name");
    } else {
        lua_pushnil(L);
    }
    return 1;
}

static int ctx_getNodeData(lua_State* L)
{
    auto* ctx = checkCtx(L, 1);
    const char* key = luaL_checkstring(L, 2);
    pushNewVariant(L, ctx->GetNodeData(key));
    return 1;
}

static int ctx_getPinId(lua_State* L)
{
    auto* ctx = checkCtx(L, 1);
    const char* name = luaL_checkstring(L, 2);
    lua_pushinteger(L, static_cast<lua_Integer>(ctx->GetPinId(name)));
    return 1;
}

static int ctx_getActivatedInputPinName(lua_State* L)
{
    auto* ctx = checkCtx(L, 1);
    lua_pushstring(L, ctx->GetActivatedInputPinName().c_str());
    return 1;
}

static void registerCtxMetatable(lua_State* L)
{
    luaL_newmetatable(L, CTX_MT);

    // mt.__index = mt
    lua_pushvalue(L, -1);
    lua_setfield(L, -2, "__index");

    static const luaL_Reg methods[] = {
        {"GetInput",                  ctx_getInput},
        {"SetOutput",                 ctx_setOutput},
        {"GetVariable",               ctx_getVariable},
        {"SetVariable",               ctx_setVariable},
        {"ActivateOutputFlow",        ctx_activateOutputFlow},
        {"Print",                     ctx_print},
        {"Log",                       ctx_log},
        {"LogWarning",                ctx_logWarning},
        {"LogError",                  ctx_logError},
        {"GetCurrentNode",            ctx_getCurrentNode},
        {"GetNodeData",               ctx_getNodeData},
        {"GetPinId",                  ctx_getPinId},
        {"GetActivatedInputPinName",  ctx_getActivatedInputPinName},
        {nullptr, nullptr}
    };
    luaL_setfuncs(L, methods, 0);
    lua_pop(L, 1);
}

// =========================================================================
// wrapLuaHandler — 将 Lua 函数包装为 C++ NodeHandler
// =========================================================================

// 自定义错误处理函数：附加 traceback
static int luaErrorHandler(lua_State* L)
{
    const char* msg = lua_tostring(L, 1);
    luaL_traceback(L, L, msg, 1);
    return 1;
}

static NodeHandler wrapLuaHandler(lua_State* L, int funcRef, const std::string& defId)
{
    // lambda 捕获 lua_State* 和 funcRef
    // 生命周期安全：handler 存在 m_handlers 中，runner 析构时 handler 销毁，
    //              此时 LuaScriptEngine 尚未析构（析构顺序：成员逆序声明顺序），
    //              所以 lua_State* 仍然有效。
    return [L, funcRef, defId](ExecutionContext& ctx) -> bool {

        // 压入错误处理函数
        lua_pushcfunction(L, luaErrorHandler);
        int errFuncIdx = lua_gettop(L);

        // 压入 Lua 函数
        lua_rawgeti(L, LUA_REGISTRYINDEX, funcRef);

        // 创建 ExecutionContext userdata（指针-to-指针，不拥有）
        auto** udata = static_cast<ExecutionContext**>(
            lua_newuserdata(L, sizeof(ExecutionContext*)));
        *udata = &ctx;
        luaL_setmetatable(L, CTX_MT);

        // 调用 Lua 函数（1 参数，1 返回值，错误函数在 errFuncIdx）
        if (lua_pcall(L, 1, 1, errFuncIdx) != LUA_OK)
        {
            const char* err = lua_tostring(L, -1);
            ctx.PrintError(std::string("[Lua Handler '") + defId + "'] " + (err ? err : "unknown error"));
            lua_pop(L, 2);  // pop error + errFunc
            return false;
        }

        // 获取返回值（默认 true）
        bool result = true;
        if (lua_isboolean(L, -1))
            result = lua_toboolean(L, -1) != 0;
        lua_pop(L, 2);  // pop result + errFunc

        return result;
    };
}

// =========================================================================
// Blueprint.RegisterHandler(definitionId, luaFunction)
// =========================================================================

static int l_registerHandler(lua_State* L)
{
    const char* defId = luaL_checkstring(L, 1);
    luaL_checktype(L, 2, LUA_TFUNCTION);

    // 将 Lua 函数存入 registry（获取引用）
    lua_pushvalue(L, 2);
    int funcRef = luaL_ref(L, LUA_REGISTRYINDEX);

    // 获取 runner（存储在 registry["__blueprint_runner"]）
    lua_getfield(L, LUA_REGISTRYINDEX, "__blueprint_runner");
    auto* runner = static_cast<BlueprintRunner*>(lua_touserdata(L, -1));
    lua_pop(L, 1);

    if (!runner)
        return luaL_error(L, "Blueprint.RegisterHandler: runner not available");

    // 如果已有同名 handler，通过 runner 日志通道输出警告
    if (runner->HasHandler(defId))
    {
        runner->LogWarning(std::string("[Lua] Overriding existing handler '") + defId + "'");
    }

    // 包装并注册（传入 defId 用于错误信息上下文）
    runner->RegisterHandler(defId, wrapLuaHandler(L, funcRef, defId));
    return 0;
}

// =========================================================================
// Blueprint.HasHandler(definitionId) → bool
// =========================================================================

static int l_hasHandler(lua_State* L)
{
    const char* defId = luaL_checkstring(L, 1);

    lua_getfield(L, LUA_REGISTRYINDEX, "__blueprint_runner");
    auto* runner = static_cast<BlueprintRunner*>(lua_touserdata(L, -1));
    lua_pop(L, 1);

    lua_pushboolean(L, runner && runner->HasHandler(defId));
    return 1;
}

// =========================================================================
// Blueprint.RegisterNodeDef(tbl)
//
// tbl 格式：
//   {
//     id       = "MyNode",           -- 必填，唯一 ID
//     name     = "My Node",          -- 可选，显示名称（默认同 id）
//     category = "Custom/Math",      -- 可选
//     color    = "FF6600",           -- 可选，RRGGBB
//     inputs   = {                   -- 可选
//       { name="A", type="Float" },
//       { name="In", type="Flow" },  -- Flow 引脚（isExec=true）
//     },
//     outputs  = {                   -- 可选
//       { name="Result", type="Float" },
//       { name="Out", type="Flow" },
//     },
//   }
// =========================================================================

static PinDataType luaParsePinType(const std::string& t, bool& isExec)
{
    isExec = false;
    if (t == "Flow")    { isExec = true; return PinDataType::Unknown; }
    if (t == "Integer") return PinDataType::Integer;
    if (t == "Float")   return PinDataType::Float;
    if (t == "Boolean") return PinDataType::Boolean;
    if (t == "String")  return PinDataType::String;
    if (t == "Array")   return PinDataType::Array;
    if (t == "Map")     return PinDataType::Map;
    if (t == "Set")     return PinDataType::Set;
    if (t == "Object")  return PinDataType::Object;
    return PinDataType::Any;
}

static void parsePinArray(lua_State* L, int tableIdx, PinKind kind,
                          std::vector<PinDefinition>& out)
{
    int n = static_cast<int>(lua_rawlen(L, tableIdx));
    for (int i = 1; i <= n; ++i)
    {
        lua_rawgeti(L, tableIdx, i);
        if (!lua_istable(L, -1)) { lua_pop(L, 1); continue; }

        PinDefinition pin;
        pin.kind = kind;

        lua_getfield(L, -1, "name");
        if (lua_isstring(L, -1)) pin.name = lua_tostring(L, -1);
        lua_pop(L, 1);

        std::string typeStr;
        lua_getfield(L, -1, "type");
        if (lua_isstring(L, -1)) typeStr = lua_tostring(L, -1);
        lua_pop(L, 1);

        bool isExec = false;
        lua_getfield(L, -1, "isExec");
        if (lua_isboolean(L, -1) && lua_toboolean(L, -1)) isExec = true;
        lua_pop(L, 1);

        if (!isExec)
            pin.dataType = luaParsePinType(typeStr, isExec);
        pin.isExec = isExec;

        // tooltip（可选）
        lua_getfield(L, -1, "tooltip");
        if (lua_isstring(L, -1)) pin.tooltip = lua_tostring(L, -1);
        lua_pop(L, 1);

        out.push_back(std::move(pin));
        lua_pop(L, 1); // pop pin table
    }
}

static int l_registerNodeDef(lua_State* L)
{
    luaL_checktype(L, 1, LUA_TTABLE);

    lua_getfield(L, LUA_REGISTRYINDEX, "__blueprint_runner");
    auto* runner = static_cast<BlueprintRunner*>(lua_touserdata(L, -1));
    lua_pop(L, 1);

    if (!runner)
        return luaL_error(L, "Blueprint.RegisterNodeDef: runner not available");

    NodeDefinition def;

    lua_getfield(L, 1, "id");
    if (!lua_isstring(L, -1))
    {
        lua_pop(L, 1);
        return luaL_error(L, "Blueprint.RegisterNodeDef: 'id' field is required (string)");
    }
    def.id = lua_tostring(L, -1);
    lua_pop(L, 1);

    lua_getfield(L, 1, "name");
    def.name = lua_isstring(L, -1) ? lua_tostring(L, -1) : def.id;
    lua_pop(L, 1);

    lua_getfield(L, 1, "category");
    if (lua_isstring(L, -1)) def.category = lua_tostring(L, -1);
    lua_pop(L, 1);

    lua_getfield(L, 1, "color");
    if (lua_isstring(L, -1)) def.color = lua_tostring(L, -1);
    lua_pop(L, 1);

    lua_getfield(L, 1, "description");
    if (lua_isstring(L, -1)) def.description = lua_tostring(L, -1);
    lua_pop(L, 1);

    lua_getfield(L, 1, "inputs");
    if (lua_istable(L, -1))
        parsePinArray(L, lua_gettop(L), PinKind::Input, def.inputPins);
    lua_pop(L, 1);

    lua_getfield(L, 1, "outputs");
    if (lua_istable(L, -1))
        parsePinArray(L, lua_gettop(L), PinKind::Output, def.outputPins);
    lua_pop(L, 1);

    if (runner->HasNodeDef(def.id))
        runner->LogWarning(std::string("[Lua] Overriding existing node def '") + def.id + "'");

    runner->RegisterNodeDef(def);
    return 0;
}

// =========================================================================
// Blueprint.HasNodeDef(id) → bool
// =========================================================================

static int l_hasNodeDef(lua_State* L)
{
    const char* id = luaL_checkstring(L, 1);

    lua_getfield(L, LUA_REGISTRYINDEX, "__blueprint_runner");
    auto* runner = static_cast<BlueprintRunner*>(lua_touserdata(L, -1));
    lua_pop(L, 1);

    lua_pushboolean(L, runner && runner->HasNodeDef(id));
    return 1;
}

// =========================================================================
// 入口：注册所有 Lua 绑定
// =========================================================================

void RegisterLuaBindings(lua_State* L, BlueprintRunner* runner)
{
    // 注册 metatable
    registerVariantMetatable(L);
    registerCtxMetatable(L);

    // 将 runner 指针存入 registry
    lua_pushlightuserdata(L, runner);
    lua_setfield(L, LUA_REGISTRYINDEX, "__blueprint_runner");

    // 创建 Blueprint 全局表
    lua_newtable(L);

    lua_pushcfunction(L, l_registerHandler);
    lua_setfield(L, -2, "RegisterHandler");

    lua_pushcfunction(L, l_hasHandler);
    lua_setfield(L, -2, "HasHandler");

    lua_pushcfunction(L, l_registerNodeDef);
    lua_setfield(L, -2, "RegisterNodeDef");

    lua_pushcfunction(L, l_hasNodeDef);
    lua_setfield(L, -2, "HasNodeDef");

    lua_setglobal(L, "Blueprint");

    // ================================================================
    // json.* 全局库
    //   json.encode(value) → string      将 Lua 值序列化为 JSON 字符串
    //   json.decode(str)   → table/value 将 JSON 字符串解析为 Lua 值
    //   json.get(str, path)→ value       按点路径（含数组下标）取值
    // ================================================================
    {
        // ── json.encode ─────────────────────────────────────────────
        auto l_json_encode = [](lua_State* LS) -> int {
            // 把栈顶 Lua 值转成 crude_json::value，再 dump
            std::function<crude_json::value(lua_State*, int)> toJson;
            toJson = [&toJson](lua_State* LS2, int idx) -> crude_json::value {
                int t = lua_type(LS2, idx);
                if (t == LUA_TNIL)     return crude_json::value(); // null
                if (t == LUA_TBOOLEAN) return crude_json::value((bool)lua_toboolean(LS2, idx));
                if (t == LUA_TNUMBER) {
                    if (lua_isinteger(LS2, idx))
                        return crude_json::value((double)lua_tointeger(LS2, idx));
                    return crude_json::value(lua_tonumber(LS2, idx));
                }
                if (t == LUA_TSTRING)
                    return crude_json::value(std::string(lua_tostring(LS2, idx)));
                if (t == LUA_TTABLE) {
                    // 检测是数组还是对象：key 全为连续整数 1..N → array
                    bool isArray = true;
                    lua_Integer arrLen = (lua_Integer)lua_rawlen(LS2, idx);
                    if (arrLen == 0) {
                        // 看第一个 key 是否是整数
                        lua_pushnil(LS2);
                        if (lua_next(LS2, idx < 0 ? idx - 1 : idx) != 0) {
                            if (lua_type(LS2, -2) != LUA_TNUMBER) isArray = false;
                            lua_pop(LS2, 2);
                        }
                    }
                    int absIdx = idx < 0
                        ? lua_gettop(LS2) + idx + 1
                        : idx;
                    if (isArray && arrLen > 0) {
                        crude_json::array arr;
                        for (lua_Integer i = 1; i <= arrLen; i++) {
                            lua_rawgeti(LS2, absIdx, i);
                            arr.push_back(toJson(LS2, -1));
                            lua_pop(LS2, 1);
                        }
                        return crude_json::value(std::move(arr));
                    } else {
                        crude_json::object obj;
                        lua_pushnil(LS2);
                        while (lua_next(LS2, absIdx) != 0) {
                            std::string key;
                            if (lua_type(LS2, -2) == LUA_TSTRING)
                                key = lua_tostring(LS2, -2);
                            else
                                key = std::to_string((int)lua_tonumber(LS2, -2));
                            obj[key] = toJson(LS2, -1);
                            lua_pop(LS2, 1);
                        }
                        return crude_json::value(std::move(obj));
                    }
                }
                return crude_json::value(); // null for unsupported types
            };

            if (lua_gettop(LS) < 1) {
                lua_pushstring(LS, "null");
                return 1;
            }
            std::string s = toJson(LS, 1).dump();
            lua_pushstring(LS, s.c_str());
            return 1;
        };

        // ── json.decode ─────────────────────────────────────────────
        auto l_json_decode = [](lua_State* LS) -> int {
            const char* str = lua_tostring(LS, 1);
            if (!str) { lua_pushnil(LS); return 1; }

            crude_json::value v = crude_json::value::parse(std::string(str));

            std::function<void(lua_State*, const crude_json::value&)> pushVal;
            pushVal = [&pushVal](lua_State* LS2, const crude_json::value& v2) {
                if (v2.is_null())    { lua_pushnil(LS2); return; }
                if (v2.is_boolean())    { lua_pushboolean(LS2, (bool)v2.get<crude_json::boolean>() ? 1 : 0); return; }
                if (v2.is_number())  { lua_pushnumber(LS2, v2.get<double>()); return; }
                if (v2.is_string())  { lua_pushstring(LS2, v2.get<std::string>().c_str()); return; }
                if (v2.is_array()) {
                    const auto& arr = v2.get<crude_json::array>();
                    lua_createtable(LS2, (int)arr.size(), 0);
                    for (int i = 0; i < (int)arr.size(); i++) {
                        pushVal(LS2, arr[i]);
                        lua_rawseti(LS2, -2, i + 1);
                    }
                    return;
                }
                if (v2.is_object()) {
                    const auto& obj = v2.get<crude_json::object>();
                    lua_createtable(LS2, 0, (int)obj.size());
                    for (const auto& kv : obj) {
                        lua_pushstring(LS2, kv.first.c_str());
                        pushVal(LS2, kv.second);
                        lua_rawset(LS2, -3);
                    }
                    return;
                }
                lua_pushnil(LS2);
            };

            pushVal(LS, v);
            return 1;
        };

        // ── json.get ────────────────────────────────────────────────
        // json.get(jsonStr, "choices[0].message.content") → value
        auto l_json_get = [](lua_State* LS) -> int {
            const char* jsonStr  = lua_tostring(LS, 1);
            const char* pathStr  = lua_tostring(LS, 2);
            if (!jsonStr || !pathStr) { lua_pushnil(LS); return 1; }

            crude_json::value v = crude_json::value::parse(std::string(jsonStr));

            // 路径遍历（复用 JSON.GetPath 节点的逻辑）
            std::string path(pathStr);
            size_t pos = 0;
            while (pos < path.size() && !v.is_null()) {
                // 数组下标 [N]
                if (path[pos] == '[') {
                    size_t end = path.find(']', pos);
                    if (end == std::string::npos) { v = crude_json::value(); break; }
                    int idx = std::stoi(path.substr(pos + 1, end - pos - 1));
                    pos = end + 1;
                    if (path[pos] == '.') pos++;
                    if (!v.is_array()) { v = crude_json::value(); break; }
                    const auto& arr = v.get<crude_json::array>();
                    if (idx < 0 || idx >= (int)arr.size()) { v = crude_json::value(); break; }
                    v = arr[idx];
                    continue;
                }
                size_t dot   = path.find('.', pos);
                size_t brack = path.find('[', pos);
                size_t end   = std::min(dot, brack);
                std::string key = path.substr(pos, end == std::string::npos ? std::string::npos : end - pos);
                pos = (end == std::string::npos) ? path.size() : end;
                if (path[pos] == '.') pos++;
                if (!v.is_object() || !v.contains(key)) { v = crude_json::value(); break; }
                v = v[key];
            }

            if (v.is_null())   { lua_pushnil(LS); return 1; }
            if (v.is_boolean())   { lua_pushboolean(LS, (bool)v.get<crude_json::boolean>() ? 1 : 0); return 1; }
            if (v.is_number()) { lua_pushnumber(LS, v.get<double>()); return 1; }
            if (v.is_string()) { lua_pushstring(LS, v.get<std::string>().c_str()); return 1; }
            // 复杂类型 → dump 为字符串
            std::string dumped = v.dump();
            lua_pushstring(LS, dumped.c_str());
            return 1;
        };

        lua_newtable(L);
        lua_pushcfunction(L, l_json_encode); lua_setfield(L, -2, "encode");
        lua_pushcfunction(L, l_json_decode); lua_setfield(L, -2, "decode");
        lua_pushcfunction(L, l_json_get);    lua_setfield(L, -2, "get");
        lua_setglobal(L, "json");
    }

    // ================================================================
    // http.* 全局库（同步，仅非 Emscripten；WebGL 下返回 nil + error）
    //   http.request(url, method, body, headers) → body, statusCode, error
    //   http.get(url, headers)                   → body, statusCode, error
    //   http.post(url, body, headers)            → body, statusCode, error
    // ================================================================
    {
        // ── lua_http_doRequest：静态辅助，lua_CFunction 兼容（无捕获）──
        // 读取 headers table（栈位置 headersIdx，0=没有）
        static auto readHeaders = [](lua_State* LS, int headersIdx) {
            std::map<std::string,std::string> h;
            if (headersIdx > 0 && lua_istable(LS, headersIdx)) {
                lua_pushnil(LS);
                while (lua_next(LS, headersIdx)) {
                    if (lua_type(LS, -2) == LUA_TSTRING && lua_type(LS, -1) == LUA_TSTRING)
                        h[lua_tostring(LS, -2)] = lua_tostring(LS, -1);
                    lua_pop(LS, 1);
                }
            }
            return h;
        };

        // 内部同步请求实现（文件作用域静态 lambda，可退化为函数指针）
        static auto doHttpRequest = [](
            lua_State* LS,
            const std::string& url,
            const std::string& method,
            const std::string& body,
            const std::map<std::string,std::string>& headers) -> int
        {
#ifdef __EMSCRIPTEN__
            lua_pushnil(LS);
            lua_pushinteger(LS, 0);
            lua_pushstring(LS, "http.* not available on WebGL (use HTTP.Request node)");
            return 3;
#else
            auto* client = BP_GetHttpClient();
            if (!client) {
                lua_pushnil(LS);
                lua_pushinteger(LS, 0);
                lua_pushstring(LS, "No HTTP client registered");
                return 3;
            }
            struct SyncResult {
                HttpResponse resp;
                bool done = false;
                std::mutex mu;
                std::condition_variable cv;
            };
            auto result = std::make_shared<SyncResult>();

            HttpRequest req;
            req.url     = url;
            req.method  = method;
            req.body    = body;
            req.headers = headers;
            if (req.headers.find("Content-Type") == req.headers.end() && !body.empty())
                req.headers["Content-Type"] = "application/json";

            client->SendAsync(req, [result](HttpResponse r) {
                std::unique_lock<std::mutex> lk(result->mu);
                result->resp = std::move(r);
                result->done = true;
                result->cv.notify_one();
            });
            {
                std::unique_lock<std::mutex> lk(result->mu);
                result->cv.wait(lk, [&result]{ return result->done; });
            }
            const auto& resp = result->resp;
            if (!resp.error.empty()) {
                lua_pushnil(LS);
                lua_pushinteger(LS, resp.statusCode);
                lua_pushstring(LS, resp.error.c_str());
                return 3;
            }
            lua_pushstring(LS, resp.body.c_str());
            lua_pushinteger(LS, resp.statusCode);
            lua_pushnil(LS);
            return 3;
#endif
        };

        // 无捕获 lambda → 可隐式转 lua_CFunction
        static const lua_CFunction l_http_request = [](lua_State* LS) -> int {
            std::string url    = lua_tostring(LS, 1) ? lua_tostring(LS, 1) : "";
            std::string method = lua_tostring(LS, 2) ? lua_tostring(LS, 2) : "GET";
            std::string body   = lua_tostring(LS, 3) ? lua_tostring(LS, 3) : "";
            return doHttpRequest(LS, url, method, body, readHeaders(LS, 4));
        };

        static const lua_CFunction l_http_get = [](lua_State* LS) -> int {
            std::string url = lua_tostring(LS, 1) ? lua_tostring(LS, 1) : "";
            return doHttpRequest(LS, url, "GET", "", readHeaders(LS, 2));
        };

        static const lua_CFunction l_http_post = [](lua_State* LS) -> int {
            std::string url  = lua_tostring(LS, 1) ? lua_tostring(LS, 1) : "";
            std::string body = lua_tostring(LS, 2) ? lua_tostring(LS, 2) : "";
            return doHttpRequest(LS, url, "POST", body, readHeaders(LS, 3));
        };

        lua_newtable(L);
        lua_pushcfunction(L, l_http_request); lua_setfield(L, -2, "request");
        lua_pushcfunction(L, l_http_get);     lua_setfield(L, -2, "get");
        lua_pushcfunction(L, l_http_post);    lua_setfield(L, -2, "post");
        lua_setglobal(L, "http");
    }
}


} // namespace Runtime
} // namespace NodeEditor

#endif // BLUEPRINT_HAS_LUA
