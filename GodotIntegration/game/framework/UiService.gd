extends CanvasLayer
## UiService —— 最简 UI 框架（Godot 原生 Control 实现）
##
## 替代 Unity 侧 ui/FGUIMan.lua 的角色（面板注册/开关/层级），
## 但不移植 FairyGUI/UGUI/UITK —— 用 Godot 的 CanvasLayer + Control 场景。
## 注册为 Autoload 单例（名为 UiService）。
##
## 面板 = 一个 .tscn（根为 Control）。open_panel 实例化并加入本层，close_panel 移除。

signal panel_opened(panel_id: String)
signal panel_closed(panel_id: String)

## 已打开面板：panel_id -> Control 实例
var _panels: Dictionary = {}

func _ready() -> void:
	# UI 层置于较高，保证盖在游戏画面之上
	layer = 100

# ---------------------------------------------------------------------------
# 打开面板：panel_id 唯一标识；scene_path 为面板 .tscn（省略则用 panel_id 推断）
# 已打开则置顶返回现有实例。
# ---------------------------------------------------------------------------
func open_panel(panel_id: String, scene_path: String = "") -> Control:
	if _panels.has(panel_id):
		var existing: Control = _panels[panel_id]
		if is_instance_valid(existing):
			move_child(existing, get_child_count() - 1) # 置顶
			existing.visible = true
			return existing
		_panels.erase(panel_id)

	var path := scene_path if scene_path != "" else "res://ui/%s.tscn" % panel_id
	if not ResourceLoader.exists(path):
		push_error("[UiService] panel scene not found: " + path)
		return null
	var packed := load(path) as PackedScene
	var panel := packed.instantiate() as Control
	if panel == null:
		push_error("[UiService] panel root is not Control: " + path)
		return null
	add_child(panel)
	_panels[panel_id] = panel
	panel_opened.emit(panel_id)
	return panel

func close_panel(panel_id: String) -> void:
	if _panels.has(panel_id):
		var p: Control = _panels[panel_id]
		if is_instance_valid(p):
			p.queue_free()
		_panels.erase(panel_id)
		panel_closed.emit(panel_id)

func close_all() -> void:
	for id in _panels.keys():
		var p: Control = _panels[id]
		if is_instance_valid(p):
			p.queue_free()
	_panels.clear()

func is_open(panel_id: String) -> bool:
	return _panels.has(panel_id) and is_instance_valid(_panels[panel_id])

func get_panel(panel_id: String) -> Control:
	return _panels.get(panel_id, null)
