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

# ---------------------------------------------------------------------------
# 异步打开面板：后台线程加载 .tscn，不卡主线程。返回一个可 await 的信号名对象，
# 或直接通过回调拿到实例。两种用法：
#   GDScript:  var panel = await UiService.open_panel_async("BigPanel")
#   Lua:       UI.open_panel_async("BigPanel", function(ok) ... end)  (见 UI 绑定)
# 已打开则同步返回现有实例（走 open_panel 的置顶逻辑），不重复加载。
# ---------------------------------------------------------------------------
signal panel_ready(panel_id: String, panel: Control)

func open_panel_async(panel_id: String, scene_path: String = "") -> Control:
	# 已打开：同步返回（并在下一帧 emit panel_ready 以统一回调时序）
	if is_open(panel_id):
		var existing: Control = _panels[panel_id]
		move_child(existing, get_child_count() - 1)
		existing.visible = true
		call_deferred("emit_signal", "panel_ready", panel_id, existing)
		return existing

	var path := scene_path if scene_path != "" else "res://ui/%s.tscn" % panel_id
	if not ResourceLoader.exists(path):
		push_error("[UiService] panel scene not found: " + path)
		call_deferred("emit_signal", "panel_ready", panel_id, null)
		return null

	# 发起后台线程加载，交给 _process 轮询完成
	ResourceLoader.load_threaded_request(path)
	_pending[path] = panel_id
	set_process(true)
	# 精确等待“本次 panel_id”的完成事件，避免并发请求互相错乱。
	while true:
		var arr: Array = await panel_ready
		if arr[0] == panel_id:
			return arr[1]
	return null  # 不可达，安抚静态分析

## path -> panel_id，_process 轮询中的进行中请求
var _pending: Dictionary = {}

func _process(_delta: float) -> void:
	if _pending.is_empty():
		set_process(false)
		return
	for path in _pending.keys().duplicate():
		var status := ResourceLoader.load_threaded_get_status(path)
		if status == ResourceLoader.THREAD_LOAD_LOADED:
			var packed := ResourceLoader.load_threaded_get(path) as PackedScene
			var pid: String = _pending[path]
			_pending.erase(path)
			var panel: Control = null
			if packed:
				panel = packed.instantiate() as Control
			if panel:
				add_child(panel)
				_panels[pid] = panel
				panel_opened.emit(pid)
			else:
				push_error("[UiService] async panel root is not Control: " + path)
			panel_ready.emit(pid, panel)
		elif status == ResourceLoader.THREAD_LOAD_FAILED or status == ResourceLoader.THREAD_LOAD_INVALID_RESOURCE:
			var pid2: String = _pending[path]
			_pending.erase(path)
			push_error("[UiService] async load failed: " + path)
			panel_ready.emit(pid2, null)

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
