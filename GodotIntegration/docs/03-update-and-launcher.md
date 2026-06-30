# 更新阶段接入与 GameLauncher 机制

本文说明：① Unity 为什么用两个 Lua VM；② 在 Godot 下如何接入更新（热更）阶段——**含"更新逻辑/UI 也用 Lua 写"的双 VM 方案**；③ GameLauncher 的启动机制。

---

## 1. Unity 为什么有两个 VM

Unity 版 `GameLauncher` 四阶段，Phase 3 用临时"更新 VM"跑 `UpdateLogic.lua`，跑完销毁，Phase 4 再建"主 VM"跑 `GameLogic.lua`。

为什么必须两个 VM？

> **热更代码与资源必须在主逻辑启动之前完成替换。** 若用同一个 VM：更新阶段已把旧版本 Lua 模块 `require` 进 VM 并缓存进 `package.loaded`，下载到新版本后想替换时，`require` 直接返回缓存的旧模块，**热更不生效**。所以用一个**一次性临时 VM** 专跑"查版本 → 下载 → 落盘"，跑完整个 `lua_close` 销毁，新资源就位后用**全新主 VM** 加载更新后的代码。

---

## 2. 关键决策：更新逻辑/UI 用什么写

| 更新阶段实现 | 是否需要独立 VM | 方案 |
|--------------|----------------|------|
| 纯 GDScript | **否** | 延迟 `setup()`（见 §3） |
| **Lua 写更新逻辑 + UI** | **是** | **双 VM**（见 §4）—— 与 Unity 等价 |

本工程**两种都支持**。下面分别给出。

---

## 3. 方案 A：更新阶段用 GDScript（单 VM，最简单）

主 runner（含主 Lua VM）由 `GameLauncher.setup()` 创建。把 `setup()` 推迟到更新完成后调用，更新阶段主 VM 还不存在，自然不被污染。

```gdscript
# Boot.gd —— 启动场景；GameLauncher.auto_setup = false
extends Node
@onready var launcher: GameLauncher = $GameLauncher

func _ready():
    await run_update_phase()        # 纯 GDScript：进度UI + 下载 .pck
    launcher.setup()                # 更新完成后才建主 VM
    var bp: BlueprintNode = $BlueprintNode
    bp.load_blueprint("res://blueprints/main.bjson")
    bp.execute()

func run_update_phase() -> void:
    var ver := await fetch_remote_version(5.0)
    if ver == "" or ver == local_version(): return
    var pck := await download_patch(ver)
    if pck != "":
        ProjectSettings.load_resource_pack(pck)  # 补丁覆盖 res://
```

资源热更用 Godot 原生 `.pck` 补丁，挂载后 `FileAccess`/`ResourceLoader` 自动读新文件，**文件桥/require 零改动**。

---

## 4. 方案 B：更新逻辑/UI 也用 Lua（双 VM，等价 Unity）

当更新阶段也用 Lua（复用你的 `UpdateLogic.lua` + FairyGUI 更新面板）时，**需要独立 VM**。引擎与 GameLauncher 已提供完整支持。

### 4.1 引擎侧机制

| C API | 作用 |
|-------|------|
| `BP_ResetSharedLuaVM()` | 销毁进程默认共享 Lua VM（`lua_close`），下个 runner 建全新 VM |

底层是 `LuaScriptEngine` 的 `SetDefault(nullptr)` —— 引擎注释里"双 VM 隔离（Update/Game 阶段切换）"那套，现以 C API 暴露。

### 4.2 GameLauncher 两阶段接口

| 方法 | 作用 |
|------|------|
| `begin_update_phase() -> BlueprintNode` | 装好环境 + 创建**专用更新 runner**（其 Lua VM 仅服务更新阶段），返回该节点 |
| `end_update_phase()` | 销毁更新 runner → `BP_ResetSharedLuaVM()` 关旧 VM → 重装 resolver 到新 VM |
| `setup()` | （更新后）建主环境；之后正常 `load_blueprint` 走全新 VM |

### 4.3 完整启动流程（GDScript）

```gdscript
# Boot.gd —— GameLauncher.auto_setup = false
extends Node
@onready var launcher: GameLauncher = $GameLauncher

func _ready():
    # ── 环境先行：setup() 装文件桥 + require 解析器，更新和主阶段都依赖它 ──
    launcher.setup()                                  # 幂等；放最前面

    # ── Phase 3 等价：更新阶段，独立 VM 跑 Lua ──
    var upd := launcher.begin_update_phase()          # 专用更新 runner（内部确保 setup 已跑）
    upd.load_blueprint("res://blueprints/update_logic.bjson")  # 内部 require UI 等 Lua
    upd.execute()
    await _wait_update_done(upd)                       # 轮询更新蓝图的完成变量
    launcher.end_update_phase()                        # 销毁更新 VM（lua_close）+ 重绑解析器

    # ── Phase 4 等价：全新主 VM 跑游戏（环境已就绪，直接 load）──
    var game: BlueprintNode = $GameNode
    game.load_blueprint("res://blueprints/main.bjson") # 加载更新后的代码
    game.execute()

func _wait_update_done(upd: BlueprintNode) -> void:
    while upd.get_var_bool("UpdateLogicDone") == false:
        await get_tree().process_frame
```

> **`setup()` 必须最先调**：它安装的文件桥 + require 解析器是**所有阶段（更新 + 主）共用的环境**，不属于某个阶段。`begin_update_phase()` 内部也会 `setup()`（幂等）保证环境就绪；`end_update_phase()` 只换 Lua VM、不动文件桥，并自动把解析器重绑到新 VM。所以主阶段 `load_blueprint` 前无需再 `setup()`。

> 你的 `UpdateLogic.lua` 约定 `UpdateLogicDone=true` 表示可退出，这里原样沿用——通过 `upd.get_var_bool("UpdateLogicDone")` 读取，无需改 Lua。

### 4.4 时序保证（为什么这样是安全的）

```
begin_update_phase()  →  VM#1 (update)  →  require 更新UI/逻辑 (缓存进 VM#1)
        ↓ 更新跑完
end_update_phase()    →  销毁 update runner → BP_ResetSharedLuaVM() → lua_close(VM#1)
        ↓ 下载的 .pck 已覆盖 res://
setup()/load_blueprint → VM#2 (game)    →  require 加载【更新后】的 Lua（VM#2 无旧缓存）
```

VM#1 与 VM#2 完全隔离，旧模块缓存随 VM#1 一起销毁，主逻辑加载到的是更新后的代码——与 Unity 双 VM 行为一致。

> 注意：调用 `end_update_phase()` 前，更新阶段的 runner 必须先销毁（GameLauncher 已在内部 `queue_free` 处理）；不要在更新 VM 上残留其它 BlueprintNode。

---

## 5. GameLauncher 机制总览

对应 Unity 四阶段，精简如下：

| Unity 阶段 | Godot 对应 |
|-----------|-----------|
| Phase 1 YooAsset 初始化 | 省略（Godot 资源系统内建） |
| Phase 2 预载所有 Lua | 省略（`require` 按需，LuaModuleResolver） |
| Phase 3 更新临时 VM | `begin_update_phase()` / `end_update_phase()`（方案 B）或 GDScript 更新（方案 A） |
| Phase 4 主 VM + GameLogic | `setup()` + `BlueprintNode.load_blueprint` |

`setup()` 做的事（幂等，**所有阶段共用的进程级环境，应最先调用**）：
```cpp
BP_InitDefaultHttpClient();              // LLM/http 节点
install_godot_file_reader("");           // 读文件 → FileAccess
install_godot_lua_loader(nullptr, roots);// require → res://lua/...
```
更新阶段与主阶段都依赖这套环境；`begin_update_phase()` 内部会确保 `setup()` 已执行，
`end_update_phase()` 只重建 Lua VM 并把解析器重绑到新 VM，不重装文件桥。

### 用法选择
- **无热更**：`auto_setup = true`，GameLauncher 自动初始化。
- **热更 + GDScript 更新**：`auto_setup = false`，Boot.gd 跑更新 → `setup()`（方案 A）。
- **热更 + Lua 更新**：`auto_setup = false`，`begin/end_update_phase()` + `setup()`（方案 B）。

### 建议设为 Autoload
项目设置 → Autoload 注册 GameLauncher，保证全局唯一、最先初始化。

---

## 6. GameLauncher API 速查

| 成员 | 说明 |
|------|------|
| `lua_roots`（属性） | require 搜索根，如 `["res://lua","res://blueprints"]` |
| `auto_setup`（属性） | true 时 `_ready` 自动 `setup()` |
| `setup()` | 幂等进程级初始化（建主环境） |
| `is_ready()` | 是否已 setup |
| `begin_update_phase()` | 进入更新阶段，返回专用更新 BlueprintNode |
| `end_update_phase()` | 结束更新阶段，销毁更新 VM，准备全新主 VM |
| `in_update_phase()` | 当前是否在更新阶段 |

---

## 7. 一句话总结

- 更新逻辑/UI 用 **GDScript** → 单 VM + 延迟 `setup()`（方案 A）。
- 更新逻辑/UI 用 **Lua** → **双 VM**（方案 B）：`begin_update_phase` 跑独立更新 VM，`end_update_phase` 销毁它（`BP_ResetSharedLuaVM`），`setup` 建全新主 VM 加载更新后的代码。与 Unity 双 VM 等价，且 `UpdateLogic.lua`/`GameLogic.lua` 几乎可原样搬。
