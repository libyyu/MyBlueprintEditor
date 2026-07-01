extends Node
## Boot —— 框架总入口，串起：双 VM 更新 → UI 展示 → 场景加载。
## 对应 Unity GameLauncher 的四阶段，但用 Godot 原生能力精简实现。
##
## 流程：
##   1) setup() 装环境（文件桥 + Lua 解析器）——所有阶段共用，最先跑
##   2) 更新阶段：begin_update_phase() 建独立更新 VM，弹更新 UI + 模拟进度
##   3) end_update_phase() 销毁更新 VM（lua_close），准备全新主 VM
##   4) 主阶段：SceneService.change_scene 加载主场景（内部再弹主菜单 UI）
##
## 本 Demo 用 GDScript 模拟"更新进度"以聚焦框架；真实项目中更新逻辑可写在
## 更新蓝图/Lua 里，通过 upd.get_var_bool("UpdateLogicDone") 轮询（见 docs/03）。

@onready var launcher: GameLauncher = $GameLauncher

func _ready() -> void:
	print("========== Boot: framework smoke test ==========")
	await _run()

func _run() -> void:
	# ── 阶段 0：环境（所有阶段共用，最先无条件执行）──
	launcher.setup()
	print("[Boot] env ready (file reader + lua resolver)")

	# ── 阶段 1：更新阶段（独立 VM）──
	var upd: BlueprintNode = launcher.begin_update_phase()
	print("[Boot] update VM created: ", upd != null, " in_update_phase=", launcher.in_update_phase())

	var panel := UiService.open_panel("UpdatePanel")   # UI 框架：弹更新面板
	panel.set_status("正在检查版本...")

	# 模拟下载进度（真实项目里这段由更新蓝图/Lua 或下载器驱动）
	for i in range(0, 11):
		var p := i / 10.0
		panel.set_progress(p)
		panel.set_status("下载资源 %d%%" % int(p * 100))
		await get_tree().create_timer(0.12).timeout
	panel.set_status("更新完成")
	await get_tree().create_timer(0.3).timeout

	# ── 阶段 2：销毁更新 VM，切换到全新主 VM ──
	UiService.close_panel("UpdatePanel")
	launcher.end_update_phase()
	print("[Boot] update VM destroyed; game VM fresh. in_update_phase=", launcher.in_update_phase())

	# ── 阶段 3：主场景加载（证明场景加载能力）──
	var ok := SceneService.change_scene("res://scenes/GameScene.tscn")
	print("[Boot] change_scene -> ", ok)
	print("========== Boot: done ==========")
