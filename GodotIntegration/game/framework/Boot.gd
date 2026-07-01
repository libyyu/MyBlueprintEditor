extends Node
## Boot —— 框架总入口，串起：环境 → 双 VM 更新（含真实热更）→ 场景加载。
##
## 流程（方案 A：更新逻辑用 GDScript）：
##   0) launcher.setup()              装环境（文件桥 + Lua 解析器），所有阶段共用，最先跑
##   1) launcher.begin_update_phase() 建独立更新 VM（本 Demo 更新逻辑用 GDScript，
##                                    该 VM 仅用于隔离，主逻辑在 end 后的新 VM）
##   2) UpdateService.run_update()    真实热更：拉 version.json → diff → 下载 pck →
##                                    sha256 校验 → load_resource_pack 挂载 → 写版本号
##                                    （弱网/失败自动回退内置资源，不阻断）
##   3) launcher.end_update_phase()   销毁更新 VM → 全新主 VM（读到挂载后的新资源）
##   4) SceneService.change_scene     加载主场景

@onready var launcher := get_node("/root/Launcher")

func _ready() -> void:
	print("========== Boot: framework + hot-update ==========")
	await _run()

func _run() -> void:
	# 阶段 0：环境
	launcher.setup()
	print("[Boot] env ready")

	# 阶段 1：更新阶段（独立 VM 隔离）
	launcher.begin_update_phase()

	# 更新 UI
	var panel := UiService.open_panel("UpdatePanel")
	UpdateService.progress.connect(func(p: float, s: String):
		if is_instance_valid(panel):
			panel.set_progress(p)
			panel.set_status(s))

	# 阶段 2：真实热更
	var result = await UpdateService.run_update()
	print("[Boot] update result: success=%s updated=%s version=%s reason=%s" % [
		result.success, result.updated, result.version, result.reason])
	await get_tree().create_timer(0.2).timeout

	# 阶段 3：销毁更新 VM，切全新主 VM
	UiService.close_panel("UpdatePanel")
	launcher.end_update_phase()
	print("[Boot] game VM fresh, in_update_phase=", launcher.in_update_phase())

	# 阶段 4：主场景
	var ok := SceneService.change_scene("res://scenes/GameScene.tscn")
	print("[Boot] change_scene -> ", ok)
	print("========== Boot: done ==========")
