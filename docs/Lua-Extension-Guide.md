# Lua 扩展节点开发指南

本文档面向希望用 Lua 脚本扩展 BlueprintRuntime 节点的开发者。

---

## 目录

1. [概述](#1-概述)
2. [启用 Lua 支持](#2-启用-lua-支持)
3. [注册节点 Handler（Runtime 侧）](#3-注册节点-handlerruntime-侧)
4. [注册节点定义（JSON 方式）](#4-注册节点定义json-方式)
5. [ExecutionContext API 参考](#5-executioncontext-api-参考)
6. [Variant 类型系统](#6-variant-类型系统)
7. [完整示例](#7-完整示例)
8. [在编辑器中使用 Lua 节点](#8-在编辑器中使用-lua-节点)
9. [调试与错误处理](#9-调试与错误处理)
10. [注意事项与限制](#10-注意事项与限制)

---

## 1. 概述

BlueprintRuntime 支持两种扩展节点的方式：

| 方式 | 文件类型 | 说明 |
|------|---------|------|
| **Lua Handler** | `.lua` 脚本 | 在 Lua 中实现节点的执行逻辑，Runtime 以 C++ `NodeHandler` 的形式调用 |
| **JSON 节点定义** | `.json` 文件 | 声明节点的元数据（名称/引脚/分类/颜色），供编辑器渲染和注册表管理 |

通常两者配合使用：**JSON 定义告诉编辑器节点长什么样，Lua Handler 决定运行时节点做什么**。

```
nodes.json          →  编辑器注册表（INodeRegistry）→ 节点在右键菜单/Library 中可见
my_handlers.lua     →  BlueprintRunner.m_handlers    → 节点在执行时调用 Lua 函数
```

---

## 2. 启用 Lua 支持

### CMake 编译选项

Lua 支持默认关闭，需要在构建时显式开启：

```cmake
cmake -B build -DBLUEPRINT_LUA=ON
```

或在 `CMakeLists.txt` 中：

```cmake
set(BLUEPRINT_LUA ON CACHE BOOL "" FORCE)
add_subdirectory(MyBlueprintEditor/Runtime)
target_link_libraries(YourTarget PRIVATE BlueprintRuntime)
```

启用后，`BLUEPRINT_HAS_LUA` 宏会被定义，`BlueprintRunner` 提供以下新接口：

```cpp
runner.LoadLuaScript("path/to/my_handlers.lua");  // 加载 Lua 文件
runner.LoadLuaString("Blueprint.RegisterHandler('MyNode', function(ctx) ... end)");  // 加载字符串
runner.GetLuaEngine();  // 获取底层 LuaScriptEngine（高级用途）
```

### 条件编译

如果你的代码需要在有无 Lua 的两种环境中编译：

```cpp
#ifdef BLUEPRINT_HAS_LUA
    runner.LoadLuaScript("extensions/my_handlers.lua");
#endif
```

---

## 3. 注册节点 Handler（Runtime 侧）

### 3.1 基础用法

在 Lua 脚本中调用 `Blueprint.RegisterHandler(definitionId, function)` 注册一个节点的执行逻辑：

```lua
-- 纯数据节点：Lerp（线性插值）
Blueprint.RegisterHandler("Math.Lerp", function(ctx)
    local a = ctx:GetInput("A"):asFloat()
    local b = ctx:GetInput("B"):asFloat()
    local t = ctx:GetInput("Alpha"):asFloat()
    ctx:SetOutput("Result", a + (b - a) * t)
    return true
end)
```

`definitionId` 必须与蓝图 JSON 中节点的 `definitionId` 字段完全一致。

### 3.2 带控制流的节点

控制流节点（有执行引脚）需要调用 `ctx:ActivateOutputFlow(pinName)` 触发下一步执行：

```lua
-- 条件分支节点
Blueprint.RegisterHandler("Flow.Branch", function(ctx)
    local cond = ctx:GetInput("Condition"):asBool()
    if cond then
        ctx:ActivateOutputFlow("True")
    else
        ctx:ActivateOutputFlow("False")
    end
    return true
end)

-- 顺序执行节点（激活多个输出流）
Blueprint.RegisterHandler("Flow.Sequence", function(ctx)
    ctx:ActivateOutputFlow("Then 0")
    ctx:ActivateOutputFlow("Then 1")
    ctx:ActivateOutputFlow("Then 2")
    return true
end)
```

### 3.3 读写蓝图变量

```lua
Blueprint.RegisterHandler("Vars.Accumulate", function(ctx)
    -- 读取蓝图变量
    local total = ctx:GetVariable("total"):asFloat()
    local delta = ctx:GetInput("Delta"):asFloat()

    -- 修改蓝图变量
    total = total + delta
    ctx:SetVariable("total", total)

    ctx:SetOutput("Value", total)
    ctx:ActivateOutputFlow("")  -- "" 代表默认执行输出引脚
    return true
end)
```

### 3.4 从 C++ 加载脚本

```cpp
#include "BlueprintRunner.h"
#include "BuiltinHandlers.h"

using namespace NodeEditor::Runtime;

BlueprintRunner runner;
RegisterBuiltinHandlers(runner, ".");
runner.LoadFromFile("my_blueprint.json");

#ifdef BLUEPRINT_HAS_LUA
// 加载扩展 handler，脚本里的 Blueprint.RegisterHandler 自动生效
runner.LoadLuaScript("extensions/my_handlers.lua");
runner.LoadLuaScript("extensions/custom_math.lua");
#endif

runner.Execute();
```

**注意**：`LoadLuaScript` 必须在 `Execute()` 之前调用。多次调用可加载多个脚本文件，handler 按注册顺序，后注册的覆盖先注册的。

---

## 4. 注册节点定义（JSON 方式）

节点定义描述节点的外观和引脚结构，供编辑器使用。通过 `LoadCustomNodesFromFile` 加载：

```cpp
#include "ScriptNodeLoader.h"
LoadCustomNodesFromFile(registry, "extensions/custom_nodes.json");
```

### 4.1 JSON 格式

```json
{
  "nodes": [
    {
      "id": "Math.Lerp",
      "name": "Lerp",
      "category": "Math",
      "color": "10A010",
      "inputs": [
        { "name": "A",     "type": "Float" },
        { "name": "B",     "type": "Float" },
        { "name": "Alpha", "type": "Float" }
      ],
      "outputs": [
        { "name": "Result", "type": "Float" }
      ]
    },
    {
      "id": "Flow.Branch",
      "name": "Branch",
      "category": "Flow Control",
      "color": "白色留空即可",
      "inputs": [
        { "name": "", "type": "Flow" },
        { "name": "Condition", "type": "Boolean" }
      ],
      "outputs": [
        { "name": "True",  "type": "Flow" },
        { "name": "False", "type": "Flow" }
      ]
    }
  ]
}
```

### 4.2 字段说明

| 字段 | 类型 | 必填 | 说明 |
|------|------|------|------|
| `id` | string | ✅ | 节点唯一标识，与 Lua Handler 的 `definitionId` 一致 |
| `name` | string | | 编辑器显示名称，默认等于 `id` |
| `category` | string | | 分类路径，支持 `"Math/Trigonometry"` 多级分类 |
| `color` | string | | 标题栏颜色，6位十六进制 RGB，如 `"1080A0"` |
| `inputs` | array | | 输入引脚列表 |
| `outputs` | array | | 输出引脚列表 |

### 4.3 引脚类型

| `type` 值 | 对应 PinDataType | 说明 |
|-----------|-----------------|------|
| `"Flow"` | 执行引脚 | 控制流引脚（无数据类型） |
| `"Integer"` | Integer | 64位整数 |
| `"Float"` | Float | 双精度浮点 |
| `"Boolean"` | Boolean | 布尔值 |
| `"String"` | String | 字符串 |
| `"Array"` | Array | 数组 |
| `"Map"` | Map | 哈希表 |
| `"Object"` | Object | 对象引用（objectId 字符串） |
| `"Any"` | Any | 任意类型 |

执行引脚也可以用 `"isExec": true` 显式标记，等价于 `"type": "Flow"`。

---

## 5. ExecutionContext API 参考

在 Lua handler 中，`ctx` 是 `ExecutionContext` 对象，提供以下方法：

### 5.1 输入/输出

| 方法 | 说明 | 返回值 |
|------|------|--------|
| `ctx:GetInput(name)` | 读取输入引脚的值 | `Variant` userdata |
| `ctx:SetOutput(name, value)` | 设置输出引脚的值 | 无 |

`SetOutput` 的 `value` 参数接受两种形式：
- Lua 原生类型：`42`、`3.14`、`true`、`"hello"`、`{1,2,3}`（table）
- `Variant` userdata（来自另一个 `GetInput` 的返回值）

```lua
-- 直接传值
ctx:SetOutput("Result", 42)

-- 转发引脚值（不做类型转换）
ctx:SetOutput("Out", ctx:GetInput("In"))
```

### 5.2 控制流

| 方法 | 说明 | 返回值 |
|------|------|--------|
| `ctx:ActivateOutputFlow(pinName)` | 激活指定执行输出引脚，驱动后续节点执行 | `bool`（是否成功）|

- `pinName` 为空字符串 `""` 时，激活第一个执行输出引脚（适用于只有单个执行输出的节点）
- 控制流节点**必须**调用此方法，否则执行链会中断

### 5.3 变量

| 方法 | 说明 | 返回值 |
|------|------|--------|
| `ctx:GetVariable(name)` | 读取蓝图变量 | `Variant` userdata |
| `ctx:SetVariable(name, value)` | 写入蓝图变量 | 无 |

变量在整个蓝图执行周期内持久存在（跨节点）。

### 5.4 调试输出

| 方法 | 说明 |
|------|------|
| `ctx:Print(msg)` | 逻辑输出（对应 `PrintString` 节点的行为，走 `SetPrintCallback`）|
| `ctx:Log(msg)` | 调试日志（走 `SetLogCallback`，用于开发调试）|

### 5.5 节点信息

| 方法 | 说明 | 返回值 |
|------|------|--------|
| `ctx:GetCurrentNode()` | 获取当前节点信息 | `{id, definitionId, name}` table 或 nil |
| `ctx:GetNodeData(key)` | 读取节点本地数据（跨执行帧持久化） | `Variant` userdata |

```lua
local node = ctx:GetCurrentNode()
if node then
    ctx:Log("Executing node: " .. node.definitionId .. " (id=" .. node.id .. ")")
end

-- 利用节点 id 作为唯一 key 存储节点本地状态
local key = "counter_" .. ctx:GetCurrentNode().id
local n = ctx:GetNodeData(key):asInt()
ctx:SetVariable(key, n + 1)
```

---

## 6. Variant 类型系统

`ctx:GetInput()` 等方法返回的 `Variant` userdata 提供以下转换方法：

| 方法 | 说明 | 失败时返回 |
|------|------|-----------|
| `:asBool()` | 转为 boolean | `false` |
| `:asInt()` | 转为 integer | `0` |
| `:asFloat()` | 转为 number | `0.0` |
| `:asString()` | 转为 string | `""` |

### 类型映射表

| C++ Variant 类型 | Lua 传入类型 | Lua 接收类型 |
|-----------------|-------------|-------------|
| Boolean | `true`/`false` | `:asBool()` |
| Integer | `42`（整数） | `:asInt()` |
| Float | `3.14`（浮点） | `:asFloat()` |
| String | `"hello"` | `:asString()` |
| Array | `{1, 2, 3}`（序列 table） | 暂不支持直接拆包 |
| Map | `{key=val}`（哈希 table） | 暂不支持直接拆包 |

**自动类型转换规则**：Variant 会尽量做隐式转换，例如整数可以 `:asFloat()` 得到浮点值。

---

## 7. 完整示例

### 7.1 纯数据节点

```lua
-- extensions/math_nodes.lua

-- 钳制值
Blueprint.RegisterHandler("Math.Clamp", function(ctx)
    local value = ctx:GetInput("Value"):asFloat()
    local min   = ctx:GetInput("Min"):asFloat()
    local max   = ctx:GetInput("Max"):asFloat()
    ctx:SetOutput("Result", math.max(min, math.min(max, value)))
    return true
end)

-- 格式化浮点数为字符串
Blueprint.RegisterHandler("String.FloatToString", function(ctx)
    local value     = ctx:GetInput("Value"):asFloat()
    local precision = ctx:GetInput("Precision"):asInt()
    ctx:SetOutput("Result", string.format("%." .. precision .. "f", value))
    return true
end)
```

### 7.2 带控制流节点

```lua
-- extensions/flow_nodes.lua

-- For 循环（简化版，同步执行 N 次）
Blueprint.RegisterHandler("Flow.ForLoop", function(ctx)
    local first = ctx:GetInput("First Index"):asInt()
    local last  = ctx:GetInput("Last Index"):asInt()

    for i = first, last do
        ctx:SetVariable("__loop_index", i)
        ctx:ActivateOutputFlow("Loop Body")
    end

    ctx:ActivateOutputFlow("Completed")
    return true
end)
```

对应的节点 JSON 定义：

```json
{
  "id": "Flow.ForLoop",
  "name": "For Loop",
  "category": "Flow Control",
  "inputs": [
    { "name": "",            "type": "Flow"    },
    { "name": "First Index", "type": "Integer" },
    { "name": "Last Index",  "type": "Integer" }
  ],
  "outputs": [
    { "name": "Loop Body", "type": "Flow" },
    { "name": "Completed", "type": "Flow" },
    { "name": "Index",     "type": "Integer" }
  ]
}
```

### 7.3 完整的 C++ 集成代码

```cpp
#include "BlueprintRunner.h"
#include "BuiltinHandlers.h"
#include "ScriptNodeLoader.h"

using namespace NodeEditor::Runtime;

// 创建 runner
BlueprintRunner runner;

// 注册内置 C++ handler
RegisterBuiltinHandlers(runner, ".");

// 加载 Lua 扩展 handler
#ifdef BLUEPRINT_HAS_LUA
runner.LoadLuaScript("extensions/math_nodes.lua");
runner.LoadLuaScript("extensions/flow_nodes.lua");
#endif

// 加载蓝图并执行
runner.LoadFromFile("my_blueprint.json");
runner.Execute();
```

---

## 8. 在编辑器中使用 Lua 节点

### 8.1 加载自定义节点定义到编辑器

编辑器启动时可以从 JSON 文件加载自定义节点定义，让它们出现在节点库（Library）面板中：

```cpp
// 在 BlueprintEditor 初始化时调用
#include "ScriptNodeLoader.h"

int count = LoadCustomNodesFromFile(m_NodeRegistry, "extensions/custom_nodes.json");
m_CachedDefCount = 0;  // 清除缓存，触发 UI 刷新
```

加载后，自定义节点会出现在 Library 面板的对应分类下，可以拖拽添加到蓝图中。

### 8.2 执行时加载 Lua Handler

编辑器执行蓝图时，需要确保 Lua handler 已加载。在执行前添加：

```cpp
void YourEditor::OnExecuteBlueprint()
{
    RegisterBuiltinHandlers(m_runner, m_ProjectBasePath);

#ifdef BLUEPRINT_HAS_LUA
    for (const auto& luaFile : m_LuaExtensionFiles)
        m_runner.LoadLuaScript(luaFile);
#endif

    m_runner.Execute();
}
```

### 8.3 节点 ID 命名建议

为避免与内置节点 ID 冲突，自定义节点建议使用命名空间前缀：

```
✅ 推荐：  "MyGame.Enemies.SpawnEnemy"
✅ 推荐：  "Utils.Math.Clamp"
❌ 避免：  "Add"（可能与内置节点冲突）
❌ 避免：  "Flow.Branch"（已被内置节点占用）
```

---

## 9. 调试与错误处理

### 9.1 Lua 错误自动输出

Lua handler 中的运行时错误会被自动捕获（通过 `lua_pcall`），错误信息包含完整调用栈：

```
[Lua] [my_handlers.lua:15] attempt to perform arithmetic on a nil value
stack traceback:
    my_handlers.lua:15: in function <my_handlers.lua:10>
    (...tail calls...)
```

错误通过 `ctx.PrintError()` 输出，不会导致程序崩溃。

### 9.2 使用 Print 和 Log 调试

```lua
Blueprint.RegisterHandler("MyDebugNode", function(ctx)
    local val = ctx:GetInput("Value"):asFloat()

    ctx:Log("DEBUG: Value = " .. val)   -- 走 LogCallback，用于诊断

    if val > 0 then
        ctx:Print("Value is positive: " .. val)  -- 走 PrintCallback，逻辑输出
    end

    ctx:SetOutput("Result", val * 2)
    return true
end)
```

在 C++ 侧注册回调捕获输出：

```cpp
runner.SetLogCallback([](LogLevel level, const std::string& msg) {
    std::cout << "[LOG] " << msg << std::endl;
});
runner.SetPrintCallback([](LogLevel level, const std::string& msg) {
    std::cout << "[PRINT] " << msg << std::endl;
});
```

### 9.3 检查 LoadLuaScript 返回值

```cpp
if (!runner.LoadLuaScript("my_handlers.lua"))
{
    // GetLastError() 返回加载失败的错误信息
    fprintf(stderr, "Lua load error: %s\n", runner.GetLastError().c_str());
}
```

---

## 10. 注意事项与限制

### 10.1 Handler 注册顺序

- 多次调用 `LoadLuaScript` 按顺序执行，后注册的 handler **覆盖**先注册的
- Lua handler 也可以覆盖内置 C++ handler（如需覆盖，在 `RegisterBuiltinHandlers` 之后加载 Lua 脚本）

### 10.2 线程安全

- 一个 `lua_State` 对应一个 `BlueprintRunner`，**不支持多线程并发调用同一个 runner**
- `RunAsync` 模式下，Lua handler 在工作线程中执行——如需在 Lua 中访问主线程资源，需要自行加锁

### 10.3 性能

- Lua handler 相比 C++ handler 有约 **10x 的调用开销**（`lua_pcall` + 类型转换）
- 对于频繁调用（每帧执行）的节点，建议用 C++ handler 代替
- 一次性或低频逻辑（如初始化、事件响应）使用 Lua 完全没有问题

### 10.4 Lua 标准库

Lua VM 初始化时打开了**全部标准库**（`luaL_openlibs`），包括：
- `io`、`os`——可以读写文件，需注意安全性
- `debug`——可以访问 Lua 内部信息
- `math`、`string`、`table`——常用工具库

### 10.5 编辑器侧节点定义（Phase 3，待实现）

目前编辑器侧的 `Blueprint.RegisterNode()` Lua API（在 Lua 中直接声明节点外观）尚未实现，需要通过 JSON 文件方式注册节点定义。参见 [Lua-Integration-Plan.md](./Lua-Integration-Plan.md) Phase 3 章节。
