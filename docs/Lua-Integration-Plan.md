# Lua 集成方案（修订版）

> **核心原则**：Runtime 只管"执行"，不管"定义"。  
> `RegisterHandler` 放 Runtime，`RegisterNode` 放 Editor（可选/后期）。

---

## 1. 架构总览

```
┌───────────────────────────────────────────────────────────┐
│                    BlueprintEditor（编辑器）                 │
│                                                           │
│  ┌─────────────────────────────────────────────────────┐  │
│  │  [后期] Lua RegisterNode → INodeRegistry             │  │
│  │  编辑器右键菜单 / 搜索面板可见 Lua 定义的新节点        │  │
│  └─────────────────────────────────────────────────────┘  │
│                        │ 导出 JSON                        │
│                        ▼                                  │
├───────────────────────────────────────────────────────────┤
│                  BlueprintRuntime（运行时）                  │
│                                                           │
│  ┌───────────────┐  ┌───────────────┐  ┌───────────────┐  │
│  │ C++ Builtin   │  │ LuaScript     │  │ 用户 C++      │  │
│  │ Handlers      │  │ Engine        │  │ 自定义 Handler │  │
│  │ (12 类别)     │  │ (新增)        │  │               │  │
│  └──────┬────────┘  └──────┬────────┘  └──────┬────────┘  │
│         │                  │                   │           │
│         └─────── RegisterHandler ──────────────┘           │
│                        │                                   │
│                        ▼                                   │
│              BlueprintRunner.m_handlers                     │
│              { definitionId → NodeHandler }                 │
│                        │                                   │
│                   Execute()                                │
│         (拓扑排序 → executeNodeInternal)                    │
└───────────────────────────────────────────────────────────┘
```

**关键洞察**：`BlueprintRunner` 执行时**零次调用** `getNodeDefinition()`。  
它只需要：
- `node.definitionId` → 匹配 handler
- `node.pins` → 读写引脚值
- `BlueprintData.links` → 传播值 + 控制流

`NodeDefinition` / `INodeRegistry` 是编辑器用来**创建**节点实例的模板，Runtime 拿到的是已经实例化好的数据。

---

## 2. 文件变更清单

### 2.1 新增文件（Runtime 侧）

| 文件 | 职责 |
|------|------|
| `Runtime/LuaScriptEngine.h` | `LuaScriptEngine` 类声明（管理 `lua_State*` 生命周期） |
| `Runtime/LuaScriptEngine.cpp` | 实现：初始化/关闭 VM、加载脚本、调用函数 |
| `Runtime/LuaBindings.h` | Lua ↔ C++ 绑定工具函数声明 |
| `Runtime/LuaBindings.cpp` | `ExecutionContext` Lua 绑定、`Variant ↔ Lua` 类型转换、`Blueprint.RegisterHandler()` 实现 |

### 2.2 修改文件（Runtime 侧）

| 文件 | 变更内容 |
|------|---------|
| `Runtime/CMakeLists.txt` | 添加 `BLUEPRINT_LUA` 选项 + Lua 5.4 依赖（FetchContent 或 find_package） |
| `Runtime/BlueprintRunner.h` | 添加 `LoadLuaScript()` / `LoadLuaString()` 接口（`#ifdef BLUEPRINT_HAS_LUA`） |
| `Runtime/BlueprintRunner.cpp` | 实现上述接口：创建 LuaScriptEngine → 加载脚本 → 自动注册 handler |

### 2.3 新增文件（编辑器侧，后期可选）

| 文件 | 职责 |
|------|------|
| `BlueprintEditor/LuaNodeRegistrar.h` | 编辑器 Lua 扩展：`Blueprint.RegisterNode()` 向 `INodeRegistry` 注册新节点定义 |
| `BlueprintEditor/LuaNodeRegistrar.cpp` | 实现：解析 Lua 表 → 构建 `NodeDefinition` → 注册到编辑器 + 刷新 UI |

---

## 3. Runtime 侧详细设计

### 3.1 LuaScriptEngine 类

```cpp
// Runtime/LuaScriptEngine.h
#pragma once
#include "BlueprintExport.h"
#include <string>
#include <memory>
#include <functional>

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

    // 禁止拷贝，允许移动
    LuaScriptEngine(const LuaScriptEngine&) = delete;
    LuaScriptEngine& operator=(const LuaScriptEngine&) = delete;
    LuaScriptEngine(LuaScriptEngine&& other) noexcept;
    LuaScriptEngine& operator=(LuaScriptEngine&& other) noexcept;

    // 初始化 VM（打开标准库 + 注册 Blueprint.* API）
    // runner: 用于 RegisterHandler 回调注册
    bool Initialize(BlueprintRunner* runner);

    // 关闭 VM
    void Shutdown();

    // 是否已初始化
    bool IsInitialized() const { return m_L != nullptr; }

    // 加载并执行 Lua 文件
    bool LoadFile(const std::string& filePath);

    // 加载并执行 Lua 字符串
    bool LoadString(const std::string& code, const std::string& chunkName = "=string");

    // 获取最后的错误信息
    const std::string& GetLastError() const { return m_lastError; }

    // 获取底层 lua_State（高级用途：注册自定义 C 函数等）
    lua_State* GetState() const { return m_L; }

private:
    lua_State*       m_L = nullptr;
    BlueprintRunner* m_runner = nullptr;
    std::string      m_lastError;
};

} // namespace Runtime
} // namespace NodeEditor
```

**设计要点**：
- `lua.h` 只在 `.cpp` 中 include，头文件只有前向声明 `struct lua_State`，**不污染使用者**
- 通过指针持有 `BlueprintRunner`，生命周期由调用者保证
- 支持移动语义，可作为 `BlueprintRunner` 的 `unique_ptr` 成员

### 3.2 Lua API — `Blueprint.RegisterHandler()`

这是 **Runtime 唯一需要的 Lua API**，对标 C++ 的 `runner.RegisterHandler(id, handler)`。

```lua
-- Lua 脚本示例：注册一个自定义 handler
Blueprint.RegisterHandler("MyCustomAdd", function(ctx)
    local a = ctx:GetInput("A"):asFloat()
    local b = ctx:GetInput("B"):asFloat()
    ctx:SetOutput("Result", a + b)
    return true  -- 成功
end)

-- 也可以注册带控制流的 handler
Blueprint.RegisterHandler("MyConditional", function(ctx)
    local condition = ctx:GetInput("Condition"):asBool()
    if condition then
        ctx:ActivateOutputFlow("True")
    else
        ctx:ActivateOutputFlow("False")
    end
    return true
end)
```

### 3.3 C++ 实现 — `Blueprint.RegisterHandler` 桥接

```cpp
// Runtime/LuaBindings.cpp（核心片段）

#include <lua.hpp>
#include "BlueprintRunner.h"
#include "LuaScriptEngine.h"

namespace NodeEditor {
namespace Runtime {

// =========================================================================
// Variant → Lua 压栈
// =========================================================================
static void pushVariant(lua_State* L, const Variant& v)
{
    switch (v.type) {
    case PinDataType::Boolean: lua_pushboolean(L, v.asBool());       break;
    case PinDataType::Integer: lua_pushinteger(L, v.asInt());        break;
    case PinDataType::Float:   lua_pushnumber(L, v.asFloat());       break;
    case PinDataType::String:  lua_pushstring(L, v.asString().c_str()); break;
    case PinDataType::Object:  lua_pushstring(L, v.asObjectId().c_str()); break;
    case PinDataType::Array: {
        lua_createtable(L, (int)v.arraySize(), 0);
        for (size_t i = 0; i < v.arraySize(); ++i) {
            pushVariant(L, v.arrayGet(i));
            lua_rawseti(L, -2, (int)(i + 1));  // Lua 数组从 1 开始
        }
        break;
    }
    case PinDataType::Map: {
        lua_createtable(L, 0, (int)v.mapSize());
        for (const auto& kv : v.asMap()) {
            lua_pushstring(L, kv.first.c_str());
            pushVariant(L, kv.second);
            lua_rawset(L, -3);
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
    case LUA_TBOOLEAN: return Variant(static_cast<bool>(lua_toboolean(L, idx)));
    case LUA_TNUMBER:
        if (lua_isinteger(L, idx))
            return Variant(static_cast<int64_t>(lua_tointeger(L, idx)));
        else
            return Variant(lua_tonumber(L, idx));
    case LUA_TSTRING:  return Variant(std::string(lua_tostring(L, idx)));
    case LUA_TTABLE: {
        // 判断是数组还是 Map：如果有 [1] 键就当数组
        lua_rawgeti(L, idx, 1);
        bool isArray = !lua_isnil(L, -1);
        lua_pop(L, 1);

        if (isArray) {
            std::vector<Variant> arr;
            int len = (int)lua_rawlen(L, idx);
            arr.reserve(len);
            for (int i = 1; i <= len; ++i) {
                lua_rawgeti(L, idx, i);
                arr.push_back(toVariant(L, -1));
                lua_pop(L, 1);
            }
            return Variant(std::move(arr));
        } else {
            std::unordered_map<std::string, Variant> map;
            lua_pushnil(L);
            int absIdx = (idx > 0) ? idx : (lua_gettop(L) + idx);
            while (lua_next(L, absIdx) != 0) {
                if (lua_type(L, -2) == LUA_TSTRING) {
                    map[lua_tostring(L, -2)] = toVariant(L, -1);
                }
                lua_pop(L, 1);
            }
            return Variant(std::move(map));
        }
    }
    default: return Variant();
    }
}

// =========================================================================
// Variant userdata — 封装 Variant 对象给 Lua 使用
// =========================================================================

static const char* VARIANT_MT = "Blueprint.Variant";

static Variant* pushNewVariant(lua_State* L, const Variant& v)
{
    auto* uv = static_cast<Variant*>(lua_newuserdata(L, sizeof(Variant)));
    new (uv) Variant(v);                    // placement new
    luaL_setmetatable(L, VARIANT_MT);
    return uv;
}

static Variant* checkVariant(lua_State* L, int idx)
{
    return static_cast<Variant*>(luaL_checkudata(L, idx, VARIANT_MT));
}

// Variant:asBool() / asInt() / asFloat() / asString()
static int variant_asBool(lua_State* L)   { lua_pushboolean(L, checkVariant(L,1)->asBool()); return 1; }
static int variant_asInt(lua_State* L)    { lua_pushinteger(L, checkVariant(L,1)->asInt());   return 1; }
static int variant_asFloat(lua_State* L)  { lua_pushnumber(L, checkVariant(L,1)->asFloat());  return 1; }
static int variant_asString(lua_State* L) { lua_pushstring(L, checkVariant(L,1)->asString().c_str()); return 1; }
static int variant_gc(lua_State* L)       { checkVariant(L,1)->~Variant(); return 0; }

static void registerVariantMetatable(lua_State* L)
{
    luaL_newmetatable(L, VARIANT_MT);
    lua_pushstring(L, "__index");
    lua_pushvalue(L, -2);
    lua_settable(L, -3);  // mt.__index = mt

    static const luaL_Reg methods[] = {
        {"asBool",   variant_asBool},
        {"asInt",    variant_asInt},
        {"asFloat",  variant_asFloat},
        {"asString", variant_asString},
        {"__gc",     variant_gc},
        {nullptr, nullptr}
    };
    luaL_setfuncs(L, methods, 0);
    lua_pop(L, 1);
}

// =========================================================================
// ExecutionContext userdata — Lua 中的 ctx 对象
// =========================================================================

static const char* CTX_MT = "Blueprint.ExecutionContext";

static int ctx_getInput(lua_State* L)
{
    auto* ctx = *static_cast<ExecutionContext**>(luaL_checkudata(L, 1, CTX_MT));
    const char* name = luaL_checkstring(L, 2);
    pushNewVariant(L, ctx->GetInputValue(name));
    return 1;
}

static int ctx_setOutput(lua_State* L)
{
    auto* ctx = *static_cast<ExecutionContext**>(luaL_checkudata(L, 1, CTX_MT));
    const char* name = luaL_checkstring(L, 2);

    // 支持直接传入 Lua 原生类型或 Variant userdata
    Variant val;
    if (luaL_testudata(L, 3, VARIANT_MT)) {
        val = *checkVariant(L, 3);
    } else {
        val = toVariant(L, 3);
    }
    ctx->SetOutputValue(name, val);
    return 0;
}

static int ctx_getVariable(lua_State* L)
{
    auto* ctx = *static_cast<ExecutionContext**>(luaL_checkudata(L, 1, CTX_MT));
    const char* name = luaL_checkstring(L, 2);
    pushNewVariant(L, ctx->GetVariable(name));
    return 1;
}

static int ctx_setVariable(lua_State* L)
{
    auto* ctx = *static_cast<ExecutionContext**>(luaL_checkudata(L, 1, CTX_MT));
    const char* name = luaL_checkstring(L, 2);
    ctx->SetVariable(name, toVariant(L, 3));
    return 0;
}

static int ctx_activateOutputFlow(lua_State* L)
{
    auto* ctx = *static_cast<ExecutionContext**>(luaL_checkudata(L, 1, CTX_MT));
    const char* pinName = luaL_checkstring(L, 2);
    lua_pushboolean(L, ctx->ActivateOutputFlow(pinName));
    return 1;
}

static int ctx_print(lua_State* L)
{
    auto* ctx = *static_cast<ExecutionContext**>(luaL_checkudata(L, 1, CTX_MT));
    const char* msg = luaL_checkstring(L, 2);
    ctx->Print(msg);
    return 0;
}

static int ctx_log(lua_State* L)
{
    auto* ctx = *static_cast<ExecutionContext**>(luaL_checkudata(L, 1, CTX_MT));
    const char* msg = luaL_checkstring(L, 2);
    ctx->Log(msg);
    return 0;
}

static int ctx_getCurrentNode(lua_State* L)
{
    auto* ctx = *static_cast<ExecutionContext**>(luaL_checkudata(L, 1, CTX_MT));
    const auto* node = ctx->GetCurrentNode();
    if (node) {
        lua_createtable(L, 0, 3);
        lua_pushinteger(L, node->id);
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
    auto* ctx = *static_cast<ExecutionContext**>(luaL_checkudata(L, 1, CTX_MT));
    const char* key = luaL_checkstring(L, 2);
    pushNewVariant(L, ctx->GetNodeData(key));
    return 1;
}

static void registerCtxMetatable(lua_State* L)
{
    luaL_newmetatable(L, CTX_MT);
    lua_pushstring(L, "__index");
    lua_pushvalue(L, -2);
    lua_settable(L, -3);

    static const luaL_Reg methods[] = {
        {"GetInput",            ctx_getInput},
        {"SetOutput",           ctx_setOutput},
        {"GetVariable",         ctx_getVariable},
        {"SetVariable",         ctx_setVariable},
        {"ActivateOutputFlow",  ctx_activateOutputFlow},
        {"Print",               ctx_print},
        {"Log",                 ctx_log},
        {"GetCurrentNode",      ctx_getCurrentNode},
        {"GetNodeData",         ctx_getNodeData},
        {nullptr, nullptr}
    };
    luaL_setfuncs(L, methods, 0);
    lua_pop(L, 1);
}

// =========================================================================
// wrapLuaHandler — 将 Lua 函数包装为 C++ NodeHandler
// =========================================================================

// Lua 函数引用存储在 registry 中（luaL_ref），
// 每次 handler 被调用时 push 函数 + 创建 ctx userdata + lua_pcall
static NodeHandler wrapLuaHandler(lua_State* L, int funcRef)
{
    // 共享 lua_State 指针和函数引用
    // 注意：lua_State 生命周期由 LuaScriptEngine 管理，
    //       LuaScriptEngine 生命周期由 BlueprintRunner 管理，
    //       handler 被调用时 runner 必然存活 → L 必然有效
    return [L, funcRef](ExecutionContext& ctx) -> bool {
        // 1. 压入 Lua 函数
        lua_rawgeti(L, LUA_REGISTRYINDEX, funcRef);

        // 2. 创建 ctx userdata（指针-to-指针，不拥有）
        auto** udata = static_cast<ExecutionContext**>(
            lua_newuserdata(L, sizeof(ExecutionContext*)));
        *udata = &ctx;
        luaL_setmetatable(L, CTX_MT);

        // 3. 调用 Lua 函数（1 参数，1 返回值）
        if (lua_pcall(L, 1, 1, 0) != LUA_OK) {
            const char* err = lua_tostring(L, -1);
            ctx.PrintError(std::string("[Lua] ") + (err ? err : "unknown error"));
            lua_pop(L, 1);
            return false;
        }

        // 4. 获取返回值（默认 true）
        bool result = lua_isboolean(L, -1) ? lua_toboolean(L, -1) : true;
        lua_pop(L, 1);
        return result;
    };
}

// =========================================================================
// Blueprint.RegisterHandler(definitionId, luaFunction)
// =========================================================================

static int l_registerHandler(lua_State* L)
{
    // 参数检查
    const char* defId = luaL_checkstring(L, 1);
    luaL_checktype(L, 2, LUA_TFUNCTION);

    // 将 Lua 函数存入 registry（获取引用）
    lua_pushvalue(L, 2);
    int funcRef = luaL_ref(L, LUA_REGISTRYINDEX);

    // 获取 runner（存储在 registry["__blueprint_runner"]）
    lua_getfield(L, LUA_REGISTRYINDEX, "__blueprint_runner");
    auto* runner = static_cast<BlueprintRunner*>(lua_touserdata(L, -1));
    lua_pop(L, 1);

    if (!runner) {
        return luaL_error(L, "Blueprint.RegisterHandler: runner not initialized");
    }

    // 包装并注册
    runner->RegisterHandler(defId, wrapLuaHandler(L, funcRef));
    return 0;
}

// =========================================================================
// 初始化所有 Lua 绑定
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
    lua_setglobal(L, "Blueprint");
}

} // namespace Runtime
} // namespace NodeEditor
```

### 3.4 Variant ↔ Lua 类型映射

| C++ Variant.type | Lua 类型 | C++ → Lua | Lua → C++ |
|---|---|---|---|
| `Boolean` | `boolean` | `lua_pushboolean` | `lua_toboolean` |
| `Integer` | `integer` | `lua_pushinteger` | `lua_tointeger` |
| `Float` | `number` | `lua_pushnumber` | `lua_tonumber` |
| `String` | `string` | `lua_pushstring` | `lua_tostring` |
| `Object` | `string`（objectId） | `lua_pushstring` | `Variant::MakeObject()` |
| `Array` | `table`（序列） | `lua_createtable` + rawseti | 遍历 rawgeti |
| `Map` | `table`（哈希） | `lua_createtable` + rawset | 遍历 lua_next |
| `Unknown` | `nil` | `lua_pushnil` | `Variant()` |

**Lua 中 `ctx:GetInput()` 返回 Variant userdata**，提供 `:asBool()` / `:asInt()` / `:asFloat()` / `:asString()` 方法，与 C++ `Variant` API 一一对应。

**`ctx:SetOutput()` 同时支持两种写法**：
```lua
ctx:SetOutput("Result", 42)              -- 直接传 Lua 原生类型
ctx:SetOutput("Result", ctx:GetInput("A"))  -- 传 Variant userdata
```

### 3.5 ExecutionContext Lua 绑定 API

Lua 中 `ctx` 对象提供的方法：

| Lua 方法 | 对应 C++ | 说明 |
|---|---|---|
| `ctx:GetInput(name)` | `GetInputValue(name)` | 返回 Variant userdata |
| `ctx:SetOutput(name, value)` | `SetOutputValue(name, value)` | 接受原生类型或 Variant |
| `ctx:GetVariable(name)` | `GetVariable(name)` | 返回 Variant userdata |
| `ctx:SetVariable(name, value)` | `SetVariable(name, value)` | |
| `ctx:ActivateOutputFlow(pinName)` | `ActivateOutputFlow(pinName)` | 返回 bool |
| `ctx:Print(msg)` | `Print(msg)` | 逻辑输出 |
| `ctx:Log(msg)` | `Log(msg)` | 调试日志 |
| `ctx:GetCurrentNode()` | `GetCurrentNode()` | 返回 `{id, definitionId, name}` 表 |
| `ctx:GetNodeData(key)` | `GetNodeData(key)` | 返回 Variant userdata |

### 3.6 BlueprintRunner 新增接口

```cpp
// Runtime/BlueprintRunner.h 新增（#ifdef BLUEPRINT_HAS_LUA 保护）

#ifdef BLUEPRINT_HAS_LUA

    // 加载 Lua 脚本文件并执行（脚本中调用 Blueprint.RegisterHandler 自动注册）
    bool LoadLuaScript(const std::string& filePath);

    // 加载 Lua 代码字符串并执行
    bool LoadLuaString(const std::string& code, const std::string& name = "=string");

    // 获取 Lua 引擎实例（高级用途）
    LuaScriptEngine* GetLuaEngine();

private:
    std::unique_ptr<LuaScriptEngine> m_luaEngine;

#endif // BLUEPRINT_HAS_LUA
```

**实现逻辑**：
```cpp
bool BlueprintRunner::LoadLuaScript(const std::string& filePath)
{
    if (!m_luaEngine) {
        m_luaEngine = std::make_unique<LuaScriptEngine>();
        if (!m_luaEngine->Initialize(this)) {
            m_lastError = "Failed to initialize Lua: " + m_luaEngine->GetLastError();
            return false;
        }
    }
    if (!m_luaEngine->LoadFile(filePath)) {
        m_lastError = "Lua load error: " + m_luaEngine->GetLastError();
        return false;
    }
    return true;
}
```

---

## 4. CMake 集成

### 4.1 `Runtime/CMakeLists.txt` 变更

```cmake
# ---- Lua 集成（可选）----
option(BLUEPRINT_LUA "Enable Lua scripting support" OFF)

if(BLUEPRINT_LUA)
    # 方式一：FetchContent 自动下载 Lua 5.4
    include(FetchContent)
    FetchContent_Declare(lua
        URL https://www.lua.org/ftp/lua-5.4.7.tar.gz
        URL_HASH SHA256=...  # 填入实际 hash
    )
    FetchContent_MakeAvailable(lua)

    # 方式二（替代）：find_package
    # find_package(Lua 5.4 REQUIRED)

    # 添加 Lua 源文件
    list(APPEND RUNTIME_SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/LuaScriptEngine.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/LuaBindings.cpp
    )
    list(APPEND RUNTIME_PUBLIC_HEADERS
        ${CMAKE_CURRENT_SOURCE_DIR}/LuaScriptEngine.h
        ${CMAKE_CURRENT_SOURCE_DIR}/LuaBindings.h
    )

    # 编译定义
    target_compile_definitions(BlueprintRuntime PUBLIC BLUEPRINT_HAS_LUA)

    # 链接 Lua
    target_link_libraries(BlueprintRuntime PRIVATE lua::lua)  # 或 ${LUA_LIBRARIES}

    message(STATUS "[BlueprintRuntime] Lua scripting enabled")
endif()
```

### 4.2 条件编译策略

```
BLUEPRINT_LUA=OFF（默认）
  → 不编译 LuaScriptEngine.cpp / LuaBindings.cpp
  → 不链接 Lua 库
  → BlueprintRunner 没有 LoadLuaScript 等方法
  → 零开销，不影响现有构建

BLUEPRINT_LUA=ON
  → 编译 Lua 绑定文件
  → BLUEPRINT_HAS_LUA 宏对消费者可见（PUBLIC）
  → BlueprintRunner 拥有 Lua 扩展 API
```

---

## 5. 使用流程

### 5.1 独立 Runtime 使用

```cpp
#include "BlueprintRunner.h"
#include "BuiltinHandlers.h"

int main()
{
    using namespace NodeEditor::Runtime;

    BlueprintRunner runner;
    runner.LoadFromFile("my_blueprint.json");

    // 注册 C++ 内置 handlers
    RegisterBuiltinHandlers(runner);

    // 加载 Lua 扩展 handlers
#ifdef BLUEPRINT_HAS_LUA
    runner.LoadLuaScript("extensions/my_handlers.lua");
    runner.LoadLuaScript("extensions/custom_math.lua");
#endif

    // Lua handler 和 C++ handler 完全平等，统一存储在 m_handlers 中
    runner.Execute();
}
```

### 5.2 Lua 脚本示例

```lua
-- extensions/my_handlers.lua

-- 1. 简单数据节点
Blueprint.RegisterHandler("Lerp", function(ctx)
    local a = ctx:GetInput("A"):asFloat()
    local b = ctx:GetInput("B"):asFloat()
    local t = ctx:GetInput("Alpha"):asFloat()
    ctx:SetOutput("Result", a + (b - a) * t)
    return true
end)

-- 2. 带控制流的节点
Blueprint.RegisterHandler("StringSwitch", function(ctx)
    local value = ctx:GetInput("Value"):asString()
    local cases = {"Case1", "Case2", "Case3", "Default"}
    
    local matched = false
    for _, caseName in ipairs(cases) do
        if caseName ~= "Default" then
            local caseValue = ctx:GetInput(caseName):asString()
            if value == caseValue then
                ctx:ActivateOutputFlow(caseName)
                matched = true
                break
            end
        end
    end
    
    if not matched then
        ctx:ActivateOutputFlow("Default")
    end
    return true
end)

-- 3. 使用变量
Blueprint.RegisterHandler("Accumulator", function(ctx)
    local key = "accumulator_" .. ctx:GetCurrentNode().id
    local current = ctx:GetNodeData(key):asFloat()
    local delta = ctx:GetInput("Delta"):asFloat()
    current = current + delta
    ctx:SetVariable(key, current)  -- 持久化到蓝图变量
    ctx:SetOutput("Value", current)
    ctx:ActivateOutputFlow("")
    return true
end)
```

### 5.3 编辑器集成（使用者视角）

```cpp
// BlueprintEditor 内（已有 runner 实例）
// 编辑器只需在执行前调用 LoadLuaScript
void BlueprintEditor::OnExecuteBlueprint()
{
    RegisterBuiltinHandlers(m_runner, basePath);

#ifdef BLUEPRINT_HAS_LUA
    // 加载项目中的所有 Lua 扩展
    for (const auto& luaFile : m_project.luaExtensions)
        m_runner.LoadLuaScript(luaFile);
#endif

    m_runner.Execute();
}
```

---

## 6. 实现优先级

### Phase 1：最小可用（Runtime Handler 注册）
1. `Runtime/CMakeLists.txt` — 添加 `BLUEPRINT_LUA` 选项 + Lua 5.4 依赖
2. `Runtime/LuaScriptEngine.h/.cpp` — VM 生命周期管理
3. `Runtime/LuaBindings.h/.cpp` — `Variant ↔ Lua` 转换 + `ExecutionContext` 绑定 + `Blueprint.RegisterHandler()`
4. `Runtime/BlueprintRunner.h/.cpp` — 添加 `LoadLuaScript()` / `LoadLuaString()`
5. 测试：写一个 Lua 脚本注册 handler，验证 `runner.Execute()` 能正确调用

### Phase 2：健壮性
1. Lua 错误处理完善（pcall 错误堆栈、行号）
2. `luaL_traceback` 集成到 `ctx.PrintError`
3. handler 覆盖警告（Lua handler 覆盖 C++ handler 时 Log Warning）
4. 多文件加载顺序保证

### Phase 3：编辑器侧节点定义 ✅ **已完成**

#### 新增文件

| 文件 | 职责 |
|------|------|
| `BlueprintEditor/LuaNodeRegistrar.h` | `LuaNodeRegistrar` 类声明；无 Lua 时提供空桩保持编译兼容 |
| `BlueprintEditor/LuaNodeRegistrar.cpp` | `Blueprint.RegisterNode()` 实现；热重载；`UnregisterAll()` |

#### 修改文件

| 文件 | 变更内容 |
|------|---------|
| `BlueprintEditor/BpProject.h/cpp` | 添加 `luaExtensions: vector<string>`，序列化/反序列化 `luaExtensions` 数组 |
| `BlueprintEditor/BlueprintEditor.h` | include `LuaNodeRegistrar.h`；添加 `m_LuaNodeRegistrar` 成员变量 |
| `BlueprintEditor/BlueprintEditor.cpp` | `OnStart()` 调用 `m_LuaNodeRegistrar.Initialize()` |
| `BlueprintEditor/ProjectOps.cpp` | `SyncProjectLibrariesToRegistry()` 末尾加载 `luaExtensions`；`CloseProject()` 调用 `UnregisterAll()`；工程面板新增 **LUA SCRIPTS** 折叠 section |
| `BlueprintEditor/EditorUI.cpp` | `OnFrame()` 每帧调用 `PollFileChanges()` 驱动热重载 |
| `CMakeLists.txt` | `BLUEPRINT_LUA=ON` 时 `BlueprintEditor` target 也链接 Lua 并定义 `BLUEPRINT_HAS_LUA` |

#### `Blueprint.RegisterNode()` Lua API

```lua
-- 注册节点定义（同时注册 handler：第二个参数传函数）
Blueprint.RegisterNode({
    id          = "MyMath.Lerp",      -- 唯一 definitionId（必填）
    name        = "Lerp",             -- 显示名称
    category    = "MyMath",           -- 分类路径（支持 "MyMath/Arithmetic"）
    description = "线性插值 A→B",
    color       = "3A8C3A",           -- 标题栏颜色（十六进制 RRGGBB，可选）
    icon        = "\uF53F",           -- FontAwesome 图标（可选）
    pure        = true,               -- 是否为纯数据节点（无 exec 流，默认 true）
    inputs  = {
        { name="A",     type="float",   default=0.0 },
        { name="B",     type="float",   default=1.0 },
        { name="Alpha", type="float",   default=0.5 },
    },
    outputs = {
        { name="Result", type="float" },
    },
}, function(ctx)
    local a = ctx:GetInput("A"):asFloat()
    local b = ctx:GetInput("B"):asFloat()
    local t = ctx:GetInput("Alpha"):asFloat()
    ctx:SetOutput("Result", a + (b - a) * t)
    return true
end)
```

`type` 支持的值：`bool`/`boolean`、`int`/`integer`、`float`/`number`、`string`、`object`、`array`、`map`、`flow`/`exec`

#### 热重载机制

```
文件写入磁盘
  ↓ （最多 1 秒内检测到）
PollFileChanges() 发现 last_write_time 变化
  ↓
UnregisterAll()  — 从 INodeRegistry 和 m_HandlerRegistry 移除旧定义
  ↓
resetLuaState()  — 关闭旧 lua_State，清除所有函数引用
  ↓
重新 LoadScript() 所有文件  — 重新执行脚本，RegisterNode/RegisterHandler 重新注册
  ↓
m_CachedDefCount = 0  — 触发右键菜单分类树重建（下一帧生效）
```

#### 工程面板 LUA SCRIPTS Section

- **折叠/展开** 显示脚本列表（橙黄色图标区分 `.lua` 文件）
- **[+] 按钮** 打开文件对话框，选择 `.lua` 文件加入工程
- **右键菜单** 支持「Remove from project」和「Reload this script」
- **Auto hot-reload 开关** + **Reload All 按钮**
- 文件不存在时红色提示，编译错误显示在 section 顶部

### Phase 4：高级功能
1. Lua 协程支持（异步节点）
2. Lua 调试器接口（断点、单步）
3. Lua 沙箱（限制文件/网络访问）
4. 性能分析（handler 耗时统计）

---

## 7. 安全与生命周期

### 7.1 生命周期链

```
BlueprintRunner
  └── owns unique_ptr<LuaScriptEngine>
        └── owns lua_State*
              └── registry 中存储：
                    · __blueprint_runner → BlueprintRunner* (lightuserdata)
                    · handler funcRef → Lua function (luaL_ref)
```

- `BlueprintRunner` 析构 → `LuaScriptEngine` 析构 → `lua_close()` 释放所有 Lua 函数引用
- handler lambda 捕获的 `lua_State*` 在 runner 析构后不会被调用（handler 存在 `m_handlers` 中，与 runner 同生命周期）

### 7.2 线程安全

- **单线程模型**：一个 `lua_State` 对应一个 `BlueprintRunner`，无需加锁
- `RunAsync` 的 background 回调不会访问 `lua_State`（设计规则已保证）
- `onComplete` 回调在主线程 Tick 中执行，可安全访问 Lua

### 7.3 错误隔离

- Lua `pcall` 捕获所有 Lua 运行时错误
- 错误信息通过 `ctx.PrintError()` 输出，不会 crash
- 单个 handler 的 Lua 错误不影响其他节点执行（返回 false 标记失败）

---

## 8. 对比：Lua Handler vs C++ Handler

| 维度 | C++ Handler | Lua Handler |
|------|------------|-------------|
| 注册方式 | `runner.RegisterHandler("id", lambda)` | `Blueprint.RegisterHandler("id", function)` |
| 存储位置 | 同一个 `m_handlers` map | 同一个 `m_handlers` map |
| 执行路径 | 直接调用 lambda | lambda → lua_pcall → Lua function |
| 性能 | 最高 | ~10x 开销（lua_pcall + 类型转换），绝大多数场景可忽略 |
| 热重载 | 需要重新编译 | 重新 LoadLuaScript 即可覆盖 |
| 调试 | C++ debugger | Lua traceback + Print |
| 优先级 | 后注册的覆盖先注册的 | 同规则，Lua handler 可覆盖 C++ handler |

**核心设计：Lua handler 和 C++ handler 对 `BlueprintRunner` 完全透明，都是 `NodeHandler = std::function<bool(ExecutionContext&)>`。**
