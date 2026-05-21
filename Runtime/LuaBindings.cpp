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
// Any 类型支持：Lua 对象 ↔ Variant.opaqueRef
// =========================================================================
//
// 设计：把 Lua 栈顶值用 luaL_ref 注册到 LUA_REGISTRYINDEX，得到一个 int ref；
// 用 shared_ptr<int> 持有 ref，自定义 deleter 在引用计数归零时 luaL_unref。
//
// 注意：lua_State* 必须在 deleter 闭包里捕获。这要求 Variant 使用期间
// lua_State 必须仍然有效（同一 VM）。跨 VM 不应使用 Any。

namespace {

// Deleter 闭包：持有 lua_State 弱引用，析构时释放 registry 中的 ref
struct LuaRefDeleter {
    lua_State* L;
    void operator()(void* p) const {
        if (!p || !L) return;
        int ref = *static_cast<int*>(p);
        delete static_cast<int*>(p);
        luaL_unref(L, LUA_REGISTRYINDEX, ref);
    }
};

// 从栈位置 idx 取一个值（不弹栈），注册到 LUA_REGISTRYINDEX，返回 Any Variant
static Variant makeAnyFromLuaIndex(lua_State* L, int idx)
{
    lua_pushvalue(L, idx);                                      // 复制到栈顶
    int ref = luaL_ref(L, LUA_REGISTRYINDEX);                   // pop + 存 registry
    if (ref == LUA_REFNIL) return Variant();                    // nil 直接返回空 Variant

    auto* refSlot = new int(ref);
    std::shared_ptr<void> sp(refSlot, LuaRefDeleter{L});
    return Variant::MakeAny(std::move(sp));
}

// 把 Any Variant 持有的 ref 推到栈顶；非 Any 或空引用时推 nil
static void pushAnyToLua(lua_State* L, const Variant& v)
{
    if (v.type != PinDataType::Any || !v.asAny()) {
        lua_pushnil(L);
        return;
    }
    int ref = *static_cast<int*>(v.asAny());
    lua_rawgeti(L, LUA_REGISTRYINDEX, ref);
}

} // namespace

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
            // 按键的原始类型推送，保持 Lua 端可以用正确类型索引
            pushVariantRaw(L, kv.first);
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
    case PinDataType::Any: {
        // Any 类型：还原 Lua registry 中的原始对象（table / userdata / function / 等）
        // 走 pushAnyToLua —— Variant 持有 LUA_REGISTRYINDEX ref，rawgeti 还原即可。
        pushAnyToLua(L, v);
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

// 把 Any Variant 还原为 Lua 原生对象推到栈顶。非 Any 类型推 nil。
static int variant_asAny(lua_State* L)
{
    pushAnyToLua(L, *checkVariant(L, 1));
    return 1;
}

static int variant_tostring(lua_State* L)
{
    lua_pushstring(L, checkVariant(L, 1)->asString().c_str());
    return 1;
}

static int variant_isValid(lua_State* L)
{
    lua_pushboolean(L, checkVariant(L, 1)->type != PinDataType::Unknown ? 1 : 0);
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
        {"asAny",      variant_asAny},
        {"isValid",    variant_isValid},
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

// =========================================================================
// Any 类型专用接口（ctx:GetInputAny / ctx:SetOutputAny）
// =========================================================================
//
// 用法（Lua 节点之间透传任意 Lua 对象，包括 table / userdata / function）：
//
//   -- 节点 A：输出一个 xLua userdata（GameObject 等）
//   ctx:SetOutputAny("Target", some_go)
//
//   -- 节点 B：拿到原始引用（identity 一致，无拷贝）
//   local go = ctx:GetInputAny("Target")
//   go.transform.position = ...
//
// 与 GetInput/SetOutput 的差别：
//   · GetInput 走 toVariant，遇到 table 会启发式拍平为 Map/Array Variant（值拷贝）
//   · GetInputAny 把对象直接放进 LUA_REGISTRYINDEX 持有引用，原样还原（引用一致）
//
// 约束：
//   · 仅在同一 lua_State 生命周期内有效（Variant 的 deleter 持有 lua_State*）
//   · 不可序列化（保存/加载蓝图时丢失）
//   · 不应被 C++ handler 消费（C++ 不知道里面是什么）

static int ctx_setOutputAny(lua_State* L)
{
    auto* ctx = checkCtx(L, 1);
    const char* name = luaL_checkstring(L, 2);
    // 第 3 个参数：任意 Lua 值（table / userdata / function / nil 均可）
    Variant val = makeAnyFromLuaIndex(L, 3);
    ctx->SetOutputValue(name, val);
    return 0;
}

static int ctx_getInputAny(lua_State* L)
{
    auto* ctx = checkCtx(L, 1);
    const char* name = luaL_checkstring(L, 2);
    pushAnyToLua(L, ctx->GetInputValue(name));
    return 1;
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
        {"GetInputAny",               ctx_getInputAny},
        {"SetOutputAny",              ctx_setOutputAny},
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
    // 标记为 Lua 注册的 handler，确保编辑器执行时能同步到 persistentRunner
    runner->MarkLuaRegisteredNode(defId);
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
    runner->MarkLuaRegisteredNode(def.id);  // 标记为 Lua 注册节点，用于热重载清理
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

    // Blueprint.AcquireAsync() — 通知 runner 有一个异步操作正在进行
    // Blueprint.ReleaseAsync() — 通知 runner 一个异步操作已完成
    // 配对使用，使 runner.HasPendingWork() 在 Lua 异步期间返回 true，驱动 Tick 循环。
    lua_pushcfunction(L, [](lua_State* Lx) -> int {
        lua_getfield(Lx, LUA_REGISTRYINDEX, "__blueprint_runner");
        auto* r = static_cast<BlueprintRunner*>(lua_touserdata(Lx, -1));
        lua_pop(Lx, 1);
        if (r) r->AcquireAsync();
        return 0;
    });
    lua_setfield(L, -2, "AcquireAsync");

    lua_pushcfunction(L, [](lua_State* Lx) -> int {
        lua_getfield(Lx, LUA_REGISTRYINDEX, "__blueprint_runner");
        auto* r = static_cast<BlueprintRunner*>(lua_touserdata(Lx, -1));
        lua_pop(Lx, 1);
        if (r) r->ReleaseAsync();
        return 0;
    });
    lua_setfield(L, -2, "ReleaseAsync");

    // Blueprint.GetVariable(name) → boolean | integer | number | string | table | nil
    // 完整支持所有 Variant 类型：
    //   Boolean → boolean, Integer → integer, Float → number, String/Object → string
    //   Array → table (array), Map → table (hash), Set → table (array), nil/Unknown → nil
    lua_pushcfunction(L, [](lua_State* Lx) -> int {
        const char* name = luaL_checkstring(Lx, 1);
        lua_getfield(Lx, LUA_REGISTRYINDEX, "__blueprint_runner");
        auto* r = static_cast<BlueprintRunner*>(lua_touserdata(Lx, -1));
        lua_pop(Lx, 1);
        if (!r) { lua_pushnil(Lx); return 1; }
        auto v = r->GetVariable(name);
        if (v.type == PinDataType::Unknown) { lua_pushnil(Lx); return 1; }
        pushVariantRaw(Lx, v);
        return 1;
    });
    lua_setfield(L, -2, "GetVariable");

    // Blueprint.SetVariable(name, value)
    // 完整支持所有类型：
    //   boolean → Boolean, integer → Integer, number → Float, string → String
    //   table (array) → Array, table (hash) → Map
    //   nil → 清除变量（设为 Unknown）
    lua_pushcfunction(L, [](lua_State* Lx) -> int {
        const char* name = luaL_checkstring(Lx, 1);
        lua_getfield(Lx, LUA_REGISTRYINDEX, "__blueprint_runner");
        auto* r = static_cast<BlueprintRunner*>(lua_touserdata(Lx, -1));
        lua_pop(Lx, 1);
        if (!r) return 0;
        if (lua_type(Lx, 2) == LUA_TNIL) {
            // nil → 清除（设为空 Unknown）
            r->SetVariable(name, Variant());
        } else {
            r->SetVariable(name, toVariant(Lx, 2));
        }
        return 0;
    });
    lua_setfield(L, -2, "SetVariable");

    lua_setglobal(L, "Blueprint");
}

} // namespace Runtime
} // namespace NodeEditor


#endif // BLUEPRINT_HAS_LUA
