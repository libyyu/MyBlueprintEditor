# 运行时框架能力（场景 / UI / 双 VM 更新）

阶段 3 目标：跑通**框架能力**（不含具体玩法）。四块能力全部用 Godot 原生实现，
替代 Unity 侧对应模块，均已 headless 验证通过。

| 能力 | Godot 实现 | 替代的 Unity 模块 |
|------|-----------|------------------|
| 场景 / 资源加载 | `framework/SceneService.gd`（Autoload） | `game/scene_manager.lua` + `SceneLoader`(C#) + YooAsset |
| UI 展示框架 | `framework/UiService.gd`（Autoload） | `ui/FGUIMan.lua` + FairyGUI/UGUI/UITK |
| 双 VM 更新 | `GameLauncher.begin/end_update_phase` + `BP_ResetSharedLuaVM` | GameLauncher Phase 3/4 双 VM |
| 总入口编排 | `framework/Boot.gd` + `scenes/Boot.tscn` | GameLauncher 四阶段 |

---

## 1. SceneService（场景 / 资源）

Autoload 单例，纯 Godot `SceneTree` + `ResourceLoader`，与 BlueprintRuntime 无耦合。

```gdscript
SceneService.change_scene("res://scenes/GameScene.tscn")   # 单场景切换
SceneService.load_additive("res://ui/Hud.tscn")            # 叠加加载(HUD/弹窗)
SceneService.unload("res://ui/Hud.tscn")                   # 卸载叠加
SceneService.preload_resource("res://x.tres")              # 同步预载
SceneService.request_load_async(path)                      # 异步预载(后台线程)
SceneService.load_async_progress(path)                     # 查进度[0,1]
SceneService.take_load_async(path)                         # 取异步结果
SceneService.instance_scene(path)                          # 实例化不入树
```
信号：`scene_loaded(path)` / `scene_unloaded(path)`（对应 Unity 的 `on_scene_loaded` 钩子）。

> YooAsset 的 AsyncLoad/热更能力 → 用 Godot `ResourceLoader` + `.pck` 补丁包，无需移植。

## 2. UiService（UI 框架）

Autoload 的 `CanvasLayer`（layer=100，盖在游戏之上）。面板 = 根为 `Control` 的 `.tscn`。

```gdscript
var panel: Control = UiService.open_panel("MainMenuPanel")  # 默认找 res://ui/MainMenuPanel.tscn
UiService.open_panel("Hud", "res://ui/custom/Hud.tscn")     # 指定路径
UiService.close_panel("MainMenuPanel")
UiService.close_all()
UiService.is_open("Hud")
```
信号：`panel_opened(id)` / `panel_closed(id)`。已内置两个演示面板：
- `ui/UpdatePanel.tscn`（标题+进度条+状态，`set_progress`/`set_status`）
- `ui/MainMenuPanel.tscn`（标题+开始按钮，`start_pressed` 信号）

> 这是 FGUIMan 的角色替代，不移植 FairyGUI。复杂 UI 后续用 Godot Control/Theme 精修。

## 3. 双 VM 更新（见 03 文档细节）

`GameLauncher.begin_update_phase()` 建独立更新 VM，`end_update_phase()` 销毁它
（`BP_ResetSharedLuaVM` 关旧 `lua_State`）后建全新主 VM。更新逻辑/UI 可用 GDScript
或 Lua 写（Lua 写时需要这个隔离，见 03）。

## 4. Boot：总入口编排

`scenes/Boot.tscn`（主场景）挂 `Boot.gd` + 一个 `GameLauncher`（`auto_setup=false`）。
流程：

```
setup()  ──────────────  装文件桥 + Lua 解析器（所有阶段共用，最先）
  ↓
begin_update_phase()  ──  建独立更新 VM
UiService.open_panel("UpdatePanel")  ── 弹更新 UI
  ↓  模拟/真实下载进度 → set_progress
end_update_phase()  ────  销毁更新 VM → 全新主 VM
  ↓
SceneService.change_scene("res://scenes/GameScene.tscn")  ── 主场景加载
  ↓
GameScene._ready → UiService.open_panel("MainMenuPanel")   ── 主菜单展示
```

### 验证结果（headless 实跑）
```
========== Boot: framework smoke test ==========
[Boot] env ready (file reader + lua resolver)
[Boot] update VM created: true in_update_phase=true
[Boot] update VM destroyed; game VM fresh. in_update_phase=false
[Boot] change_scene -> true
[GameScene] loaded via SceneService.change_scene
[GameScene] MainMenuPanel opened via UiService
```
更新双 VM 切换、UI 展示、场景加载三条能力串起来跑通。

---

## 5. 工程文件清单（本阶段新增）

```
game/
├── project.godot            # main_scene=Boot.tscn; autoload SceneService/UiService
├── framework/
│   ├── Boot.gd              # 总入口编排
│   ├── SceneService.gd      # 场景/资源加载
│   └── UiService.gd         # UI 框架
├── scenes/
│   ├── Boot.tscn            # 启动场景(挂 Boot + GameLauncher)
│   ├── GameScene.tscn/.gd   # 主场景(含 BlueprintNode 挂点)
└── ui/
    ├── UpdatePanel.tscn/.gd    # 更新进度面板
    └── MainMenuPanel.tscn/.gd  # 主菜单面板
```

## 6. 下一步（阶段 3 剩余 / 阶段 4）
- 把更新阶段的 GDScript 模拟进度替换为真实"版本检查 + `.pck` 下载"，或改用更新蓝图/Lua。
- Lua 侧通过 C API 节点回调 SceneService / UiService（让蓝图能"切场景/开面板"）。
- 具体玩法（CutRope 等）与物理，UI 精修。
