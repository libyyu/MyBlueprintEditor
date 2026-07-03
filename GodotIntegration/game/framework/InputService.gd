extends Node
## InputService —— 统一输入抽象层（Autoload 单例）
##
## 目标：GDScript 和 Lua 用同一套输入 API，三端一致。
##   - 桌面：键鼠 → InputMap action
##   - 移动：触摸/虚拟摇杆 → 同一批 action（映射在 project.godot 的 InputMap 里）
##   - 小游戏：宿主 JS 把触摸事件转成 action（引擎侧走同一抽象）
##
## 一切以「action 名」为中心，业务层不直接碰具体按键/设备，从而跨平台。
##
## 两种用法：
##   1) 轮询（每帧查状态）——适合移动/持续输入：
##        InputService.is_pressed("move_left")
##        InputService.get_axis("move_left", "move_right")
##   2) 订阅（事件驱动）——适合按钮/单次触发：
##        InputService.on_action("ui_confirm", callable)   # 按下瞬间触发
##        或直接连 action_triggered(action, edge) 信号

## action 触发信号：edge = "pressed" | "released"
signal action_triggered(action: String, edge: String)

# 订阅表：action -> Array[Callable]，仅在 pressed 边沿触发（最常用）
var _subs: Dictionary = {}

# 上一帧各 action 的按下状态，用于自己算边沿（覆盖 InputMap 未定义的 action 也安全）
var _prev: Dictionary = {}

# 需要每帧检测边沿并派发的 action 集合（被订阅的 + 显式关注的）
var _watched: Dictionary = {}


func _ready() -> void:
	set_process(true)
	# 默认关注几个通用 action（若 InputMap 里没有则查询返回 false，不报错）
	for a in ["ui_confirm", "ui_cancel", "move_left", "move_right", "move_up", "move_down"]:
		_watched[a] = true
		_prev[a] = false


# ---------------------------------------------------------------------------
# 轮询式查询
# ---------------------------------------------------------------------------

## action 当前是否按下
func is_pressed(action: String) -> bool:
	if not InputMap.has_action(action):
		return false
	return Input.is_action_pressed(action)

## action 本帧刚按下
func just_pressed(action: String) -> bool:
	if not InputMap.has_action(action):
		return false
	return Input.is_action_just_pressed(action)

## action 本帧刚松开
func just_released(action: String) -> bool:
	if not InputMap.has_action(action):
		return false
	return Input.is_action_just_released(action)

## 轴值：正向 action 强度 - 负向 action 强度，范围 [-1, 1]
func get_axis(negative_action: String, positive_action: String) -> float:
	var neg := Input.get_action_strength(negative_action) if InputMap.has_action(negative_action) else 0.0
	var pos := Input.get_action_strength(positive_action) if InputMap.has_action(positive_action) else 0.0
	return pos - neg

## 二维向量：由四个 action 组成（左右/上下），已归一化，适合移动
func get_vector(neg_x: String, pos_x: String, neg_y: String, pos_y: String) -> Vector2:
	return Input.get_vector(neg_x, pos_x, neg_y, pos_y)


# ---------------------------------------------------------------------------
# 指针（鼠标/触摸统一）
# ---------------------------------------------------------------------------

## 当前指针位置（视口坐标）。移动端为最后一次触摸点。
func get_pointer_position() -> Vector2:
	var vp := get_viewport()
	if vp == null:
		return Vector2.ZERO
	return vp.get_mouse_position()

## 指针主键是否按下（鼠标左键 / 触摸中）
func is_pointer_pressed() -> bool:
	return Input.is_mouse_button_pressed(MOUSE_BUTTON_LEFT)


# ---------------------------------------------------------------------------
# 事件订阅（GDScript 侧）
# ---------------------------------------------------------------------------

## 订阅一个 action 的按下边沿。返回一个取消订阅用的句柄（Callable 本身）。
func on_action(action: String, cb: Callable) -> void:
	if not _subs.has(action):
		_subs[action] = []
	(_subs[action] as Array).append(cb)
	_watched[action] = true
	if not _prev.has(action):
		_prev[action] = false

## 取消某 action 的一个订阅
func off_action(action: String, cb: Callable) -> void:
	if _subs.has(action):
		(_subs[action] as Array).erase(cb)

## 清空某 action 的所有订阅
func clear_action(action: String) -> void:
	_subs.erase(action)


# ---------------------------------------------------------------------------
# 每帧边沿检测 + 派发
# ---------------------------------------------------------------------------

func _process(_delta: float) -> void:
	for action in _watched.keys():
		var a := String(action)
		if not InputMap.has_action(a):
			continue
		var now := Input.is_action_pressed(a)
		var was: bool = _prev.get(a, false)
		if now and not was:
			_dispatch(a, "pressed")
		elif was and not now:
			_dispatch(a, "released")
		_prev[a] = now


func _dispatch(action: String, edge: String) -> void:
	action_triggered.emit(action, edge)
	# 订阅回调只在 pressed 边沿触发（最常见的"按下即响应"）
	if edge == "pressed" and _subs.has(action):
		for cb in (_subs[action] as Array):
			if cb is Callable and (cb as Callable).is_valid():
				(cb as Callable).call()


# ---------------------------------------------------------------------------
# 测试/宿主注入辅助：手动触发一次 action 边沿（headless 验证、小游戏宿主转发用）
# ---------------------------------------------------------------------------

## 手动派发一次 action 事件（不经真实设备）。小游戏宿主可用它把 JS 触摸转成 action。
func fire_action(action: String, edge: String = "pressed") -> void:
	_dispatch(action, edge)
