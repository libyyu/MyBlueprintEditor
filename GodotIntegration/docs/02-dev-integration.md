# 开发接入指南

把 BlueprintRuntime 蓝图引擎接入一个 Godot 4.5 工程，从零到能跑。

---

## 1. 工程结构约定

```
your_game/                      # Godot 工程根（用 Godot 4.5 打开此目录）
├── project.godot
├── bin/
│   ├── blueprint.gdextension   # 扩展清单
│   ├── blueprint_gdext.*.dll   # GDExtension 绑定（编译产物）
│   └── BlueprintRuntime.dll    # 引擎核心（编译产物）
├── lua/                        # Lua 模块根（require 从这里找）
│   ├── blueprints/             #   蓝图节点注册 (BlueprintEntry.lua 等)
│   ├── game/ ui/ utility/ ...  #   玩法 / UI / 工具
├── blueprints/                 # .bjson 蓝图数据 + 关卡
└── main.tscn                   # 主场景
```

> Lua 模块根放在 `res://lua`。`require "ui.FGUIMan"` → `res://lua/ui/FGUIMan.lua`。

---

## 2. 两个核心类

GDExtension 注册了两个节点类型，对应 Unity 时期的两个职责：

| Godot 类 | 职责 | 对应 Unity |
|----------|------|-----------|
| **GameLauncher** | 进程级启动：装文件桥 + Lua require 解析器，**全局只跑一次** | `GameLauncher` 四阶段 |
| **BlueprintNode** | 加载并驱动**单个**蓝图：`_ready` 建 runner、`_process` 每帧 `BP_Tick` | `BlueprintBehaviour` |

设计原则：**进程级初始化归 GameLauncher，单蓝图运行归 BlueprintNode。** 因为 Lua VM 是进程级单例，require 解析器只需设一次，与蓝图数量无关。

---

## 3. 最小接入（5 步）

### 第 1 步：放置二进制
把 `blueprint.gdextension` + 两个 dll 放进 `res://bin/`。重启 Godot 编辑器，`BlueprintNode` / `GameLauncher` 会出现在节点列表。

### 第 2 步：场景里放一个 GameLauncher（全局一次）
```
Main (Node)
├── GameLauncher        # lua_roots = ["res://lua"], auto_setup = true
└── ...
```
`auto_setup=true` 时它在自己的 `_ready` 里自动完成：
```
BP_InitDefaultHttpClient()
install_godot_file_reader("")          # 所有读文件 → Godot FileAccess
install_godot_lua_loader(null, roots)  # require → res://lua/...，进程级一次
```

### 第 3 步：放 BlueprintNode 运行蓝图
```gdscript
extends Node
@onready var launcher: GameLauncher = $GameLauncher
@onready var bp: BlueprintNode = $BlueprintNode

func _ready():
    if not launcher.is_ready():
        launcher.setup()                 # 幂等，保证已初始化
    bp.load_blueprint("res://blueprints/level_1.bjson")
    bp.execute()
```

### 第 4 步：多个蓝图（不需要重复设 searcher）
```gdscript
# GameLauncher 已设好 searcher；后面任意多个 BlueprintNode 直接 load
bp_a.load_blueprint("res://blueprints/npc_dialogue.bjson")
bp_b.load_blueprint("res://blueprints/quest.bjson")
# 它们共享同一个进程级 Lua VM 与 require 解析器
```

### 第 5 步：与蓝图交互
```gdscript
bp.set_var_int("Health", 100)
var hp := bp.get_var_int("Health")
bp.dispatch_event("on_win")              # 触发蓝图事件
var deps := bp.get_dependencies(path)    # 查依赖图
```

---

## 4. BlueprintNode 属性 / 方法

| 成员 | 说明 |
|------|------|
| `blueprint_path` (属性) | 蓝图文件 res:// 路径 |
| `autorun` (属性) | true 时在 `_ready` 自动 load + execute（仅适合无 require 的自包含蓝图） |
| `tick_enabled` (属性) | 每帧是否 `BP_Tick` |
| `load_blueprint(path)` | 加载蓝图（含自动加载 `metadata.dependencies`） |
| `load_from_json(json)` | 从字符串加载 |
| `execute()` | 数据流求值 + 派发 `OnBeginPlay` |
| `dispatch_event(id)` | 派发命名事件 |
| `set_var_*/get_var_*` | int/float/string/bool 变量读写 |
| `get_dependencies(path)` | 返回蓝图声明的依赖路径数组 |
| `get_last_error()` | 最近一次错误详情 |

## 5. GameLauncher 属性 / 方法

| 成员 | 说明 |
|------|------|
| `lua_roots` (属性) | require 搜索根，按序尝试，如 `["res://lua","res://blueprints"]` |
| `auto_setup` (属性) | true 时 `_ready` 自动 `setup()` |
| `setup()` | 幂等的进程级初始化 |
| `is_ready()` | 是否已初始化 |
| `begin_update_phase()` | 双 VM 热更：进入更新阶段（独立 VM），返回更新 BlueprintNode（见 03 文档） |
| `end_update_phase()` | 双 VM 热更：销毁更新 VM，准备全新主 VM |
| `in_update_phase()` | 当前是否在更新阶段 |

> 推荐把 GameLauncher 设为 **autoload 单例**（项目设置 → Autoload），这样整个游戏全局唯一、最先初始化。

---

## 6. 资源加载（重要）

| 资源类型 | 加载方式 | 说明 |
|----------|----------|------|
| **蓝图 `.bjson`** | `BlueprintNode.load_blueprint` | 经 `godot_file_bridge` 走 FileAccess；依赖自动加载 |
| **Lua 模块** | `require "x.y"` | 经 `godot_lua_loader` 走 FileAccess，按需即时编译 |
| **游戏资产**（贴图/场景/音频） | Godot `ResourceLoader.load()` / `preload()` | **不移植 YooAsset**，用 Godot 原生资源系统 |

三类资源在 desktop / WebGL / 打包(.pck) 下都走同一套 Godot `FileAccess`，**无需为不同平台改代码**。

> Lua/蓝图里原 Unity 的 `AsyncLoad("xxx")` 应改为映射到 `ResourceLoader.load("res://...")`（通过一个节点回调暴露给 Lua）。这是 Lua 迁移阶段的工作。

---

## 7. 与 Unity 版的对照

| 维度 | Unity (xLua) | Godot (本工程) |
|------|--------------|----------------|
| 宿主语言 | C# | GDScript / C++ |
| 蓝图引擎 | BlueprintRuntime.dll（同一个） | BlueprintRuntime.dll（同一个） |
| Lua VM | xLua 拥有，引擎借用（共享 lua_State） | **引擎自管单 VM**（无注入、无野指针风险） |
| 资源系统 | YooAsset | Godot ResourceLoader + .pck |
| 文件读取 | YooAsset TextAsset | `BP_SetFileReader` → FileAccess |
| Lua 加载 | 预载全部 | `BP_SetLuaModuleResolver` → require 按需 |
| 启动 | GameLauncher 四阶段 | GameLauncher（精简，见 03 文档） |
| 证书 | 需要（团结引擎） | 无（MIT） |
