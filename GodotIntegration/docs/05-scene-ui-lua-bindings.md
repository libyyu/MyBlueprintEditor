# Scene / UI 能力：GDScript 与 Lua 双端接入

本篇说明「资源/场景」与「UI」两套框架**如何同时被 GDScript 原生代码和 Lua 脚本使用**，
并明确一条铁律：**业务能力全部在 GDExtension（项目层）实现，BlueprintRuntime 引擎零改动**。

---

## 1. 架构总览

```
                     ┌─────────────────────────────────────────┐
   GDScript ────────▶│  Autoload: SceneService / UiService (.gd)│
   (直接调用)         │   change_scene / open_panel / set_text …  │
                     └───────────────▲─────────────────────────┘
                                     │ 转发
   Lua 脚本            ┌─────────────┴──────────────┐
   Scene.load(...) ──▶│ 项目层 godot_lua_bindings.cpp│ (GDExtension)
   UI.open_panel(...) │  用 BP_GetLuaState 拿 lua_State│
   UI.on_click(...)   │  注册 Scene.*/UI.* 全局表      │
                     └─────────────▲──────────────┘
                                   │ 仅用引擎「已有通用能力」
                     ┌─────────────┴──────────────┐
                     │ BlueprintRuntime (引擎，未改)│
                     │  BP_GetLuaState(runner)      │
                     │  静态链接 Lua → dll 顺带导出   │
                     │  lua_newtable / luaL_ref …    │
                     └──────────────────────────────┘
```

**关键点**：引擎既不知道「场景」，也不知道「面板」。项目层通过引擎唯一暴露的
`BP_GetLuaState(runner)` 拿到 `lua_State*`，自己用 Lua C API 注册 `Scene`/`UI` 全局表，
表里的 C 函数转发到 Godot 的 autoload Service。GDScript 直接用 Service，两端殊途同归。

> 为什么项目层能调 Lua C API？引擎**静态链接** Lua，导致 `BlueprintRuntime.dll` 顺带
> 导出了 `lua_newtable` / `luaL_ref` / `lua_pcall` 等全部 lua 符号。项目层链接
> `BlueprintRuntime.lib` 即可使用，无需自己再编一份 Lua。CMake 里需要 include
> `build-windows/_deps/lua54-src/src`（lua 头）并定义 `BLUEPRINT_HAS_LUA`。

---

## 2. GDScript 路径（原生）

```gdscript
# 场景 / 资源
SceneService.change_scene("res://scenes/GameScene.tscn")
SceneService.load_additive("res://ui/Hud.tscn")
SceneService.unload("res://ui/Hud.tscn")
SceneService.instance_scene("res://actors/Enemy.tscn")
SceneService.preload_resource("res://textures/bg.png")

# UI（同步）
var panel := UiService.open_panel("MainMenuPanel")   # 同步：load()+instantiate()+add_child，返回即可用
UiService.close_panel("MainMenuPanel")
var p := UiService.get_panel("MainMenuPanel")         # 已打开的面板实例（供查询/操作）

# UI（异步，后台线程加载，不卡主线程）
var panel2 := await UiService.open_panel_async("BigPanel")   # 可 await 拿实例
```

> **同步 vs 异步**：
> - `open_panel(id)` **同步**：内部 `load()`+`instantiate()`+`add_child` 一次做完，返回时面板已在树上、已进 `_panels`，可立即操作（但 anchor/容器布局要等下一帧结算，依赖 `size` 的逻辑需 `await get_tree().process_frame`）。
> - `open_panel_async(id)` **异步**：用 `ResourceLoader.load_threaded_request` 后台线程加载，`_process` 轮询完成后 `add_child` 并 `emit panel_ready(id, panel)`。GDScript 可 `await` 返回值；面板大时避免卡帧。
> - `close_panel` 用 `queue_free()`，节点在帧末才真正销毁（`_panels` 记录立即移除，`is_open` 立即为 false）。

---

## 3. Lua 路径（脚本）

前置：某个 `BlueprintNode` 已 `load_blueprint`（此时它的 runner 有 Lua VM），
然后 `Launcher.bind_lua_api(node)` 把 `Scene`/`UI` 注册进该 VM（每个 VM 注册一次）。

```lua
-- lua/demo/main_menu_logic.lua
local M = {}
function M.open()
    UI.open_panel("MainMenuPanel")
    UI.set_text("MainMenuPanel", "Center/VBox/Title", "主菜单 (Lua 驱动)")

    -- 订阅按钮点击 —— Godot 的 pressed 信号回流到这个 Lua 闭包
    UI.on_click("MainMenuPanel", "Center/VBox/StartButton", function()
        UI.set_text("MainMenuPanel", "Center/VBox/Hint", "已点击开始 (来自 Lua)")
        -- Scene.change("res://scenes/Level_1.tscn")  -- 也可切场景
    end)
end
return M
```

### Lua 全局表 API（第一批）

| 表.函数 | 参数 | 说明 |
|---|---|---|
| `Scene.load/change(path)` | res:// 路径 | 替换主场景，返回 bool |
| `Scene.load_additive(path)` | | 叠加载入（HUD/overlay），返回 bool |
| `Scene.unload(path)` | | 卸载叠加场景 |
| `Scene.instance(path)` | | 实例化为独立节点，返回 instance_id（整数）|
| `Scene.preload(path)` | | 预载资源到缓存，返回 bool |
| `UI.open_panel(id)` | 面板 id | **同步**打开面板（UiService 解析为 .tscn），返回 bool |
| `UI.open_panel_async(id, cb)` | cb = `function(ok)` | **异步**打开（后台线程加载）；加载完成后回调 `cb(ok)`。Lua 无 await，用回调 |
| `UI.close_panel(id)` | | 关闭面板 |
| `UI.set_text(id, node, text)` | 面板内相对节点路径 | 设置 Label/Button/LineEdit 文本 |
| `UI.get_input_text(id, node)` | | 读取输入框文本，返回 string |
| `UI.on_click(id, node, fn)` | fn = Lua 函数 | 订阅按钮点击，点击时回调 fn |

---

## 4. 事件回流机制（按钮点击 → Lua）

`UI.on_click` 内部：
1. 把 Lua 回调函数 `luaL_ref` 存进注册表，key = `"panel\0node"`
2. 在目标 `Button` 上 `add_child` 一个 `GdUiClickRelay`（继承 `Node`），
   把 `pressed` 信号 connect 到它的 `_on_pressed`
3. 点击时 `_on_pressed` 按 key 取出 Lua 回调 ref，`lua_pcall` 执行

> **关键坑**：`GdUiClickRelay` 是 GDExtension 类，**必须在模块初始化时注册**
> （`register_types.cpp` 的 `initialize` 里 `GDREGISTER_INTERNAL_CLASS`）。
> 若在运行时才 `register_internal_class` 会**直接崩溃**（本阶段踩过）。

### 4.1 异步加载回流（UI.open_panel_async → Lua cb）

`UI.open_panel_async(id, cb)` 内部：
1. 把 Lua 回调 `luaL_ref` 存注册表
2. 建一个 `GdUiPanelReadyRelay`（也是 init 时注册的 GDExtension 类），`add_child` 到 UiService，
   把 `panel_ready(panel_id, panel)` 信号 connect 到它的 `_on_panel_ready`
3. 调 GDScript `UiService.open_panel_async(id)` 发起后台加载
4. 加载完成 → `_process` 轮询到 `THREAD_LOAD_LOADED` → 实例化+add_child → `emit panel_ready`
5. relay 的 `_on_panel_ready` 只处理匹配 `want_id` 的那次（一个 `panel_ready` 信号被所有并发请求共享），
   取出 Lua 回调 `lua_pcall(cb, ok)`，然后 `luaL_unref` + `queue_free` 自身（一次性）

> `GdUiPanelReadyRelay` 同样**必须在 init 时 `GDREGISTER_INTERNAL_CLASS`**。
> 连接**不用 `CONNECT_ONE_SHOT`**：多个并发 async 共享同一个 `panel_ready` 信号，每个 relay 按
> `want_id` 过滤，只在自己那次到达时才 free，避免被别的 panel 的完成事件误触发而提前断连。

---

## 5. 生命周期约定（重要）

- **`Launcher` 是 autoload 单例**（`project.godot` → `[autoload]`），跨 `change_scene` 常驻。
  这样任意场景都能 `get_node("/root/Launcher").bind_lua_api(...)`。
  若把它放进普通场景，`change_scene` 后会被释放，Lua 绑定就丢了。
- **autoload 名不要与 GDExtension 类名同名**。类名是 `GameLauncher`，autoload 名取 `Launcher`。
  同名会让 `@onready var x: GameLauncher = get_node("/root/GameLauncher")` 的类型解析崩
  （`gdscript_vm.cpp: Condition '!nc' is true`）。本阶段踩过。
- Lua VM 是进程级共享；`bind_lua_api` 对同一 VM 重复调用是幂等的（覆盖同名全局表）。
  双 VM 热更时，`end_update_phase()` 后主 VM 是新建的，需要对主阶段的 node 再 `bind_lua_api` 一次。

---

## 6. 涉及文件

| 文件 | 角色 |
|---|---|
| `src/godot_lua_bindings.{h,cpp}` | 项目层 Scene/UI Lua 绑定 + GdUiClickRelay |
| `src/blueprint_node.cpp` | 新增 `run_lua(code)` / `run_lua_file(path)`（用 BP_GetLuaState 执行 Lua）|
| `src/game_launcher.cpp` | `bind_lua_api(node)` 调 `register_godot_lua_bindings` |
| `src/register_types.cpp` | `GDREGISTER_INTERNAL_CLASS(GdUiClickRelay)`（init 时注册）|
| `framework/SceneService.gd` | 场景/资源加载（GDScript 原生实现）|
| `framework/UiService.gd` | UI 面板管理（GDScript 原生实现）|
| `framework/GameLauncher.tscn` | autoload 承载体（名 `Launcher`）|
| `lua/demo/main_menu_logic.lua` | 纯 Lua 编写的 UI 逻辑 Demo |
| `CMakeLists.txt` | include lua 头 + 定义 `BLUEPRINT_HAS_LUA` |

---

## 7. 验证结果（headless 实跑）

```
[GodotLua] Scene/UI Lua bindings registered on this VM
[GameScene] Lua 打开面板 MainMenuPanel = true          ← Lua 调 UI.open_panel 真开了面板
[GameScene] Lua 设置的标题 = '主菜单 (Lua 驱动)'        ← Lua 调 UI.set_text 改了 Godot 控件
[GameScene] 点击后 Lua 回调改写的提示 = '已点击开始 (来自 Lua)'
                                                      ↑ 按钮 pressed → Lua on_click 回调 → 再改控件
```

Lua ⇄ Godot 双向通道闭环，引擎零改动。
