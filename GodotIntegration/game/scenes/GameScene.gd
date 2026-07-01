extends Node2D
## 主游戏场景（更新完成后由 SceneService.change_scene 加载）。
## 这里只证明"场景加载能力"——放一个标题和一个由蓝图驱动的挂点。
## 不含 CutRope 玩法。

func _ready() -> void:
	print("[GameScene] loaded via SceneService.change_scene")
	# 延一帧再弹 UI，确保场景已完全进入树（autoload UiService 始终在）
	call_deferred("_open_menu")

func _open_menu() -> void:
	var ui = get_node_or_null("/root/UiService")
	if ui == null:
		push_error("[GameScene] UiService autoload missing")
		return
	var menu: Control = ui.open_panel("MainMenuPanel")
	if menu and menu.has_signal("start_pressed"):
		menu.start_pressed.connect(_on_start)
	print("[GameScene] MainMenuPanel opened via UiService")

func _on_start() -> void:
	print("[GameScene] start pressed — 此处后续接蓝图玩法")
