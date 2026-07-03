extends Node
## TimerService —— 逻辑计时器（Autoload 单例）
##
## 对齐 Unity 的 FTimerListBehavior / timer.lua：
##   - id 化的一次性(after) / 循环(every) 定时器
##   - 回调可为 GDScript Callable 或（经 Lua 桥转发的）Lua 函数
##   - Tick 期间新增/删除安全（暂存到 temp 列表，Tick 结束后合并）
##   - 两条链：process（_process 阶段）/ late（_process 之后、同帧末）
##
## 两种用法：
##   GDScript：
##     var id = TimerService.after(3.0, func(): print("hi"))
##     var id2 = TimerService.every(1.0, func(): print("tick"))
##     TimerService.cancel(id2); TimerService.reset(id)
##   Lua（经 godot_lua_bindings 的 Timer 表）：
##     Timer.after(3.0, function() ... end)
##     Timer.every(1.0, function() ... end)
##
## 计时基于累计的帧 delta（get_process_delta_time），与 Godot 帧率一致，
## 跨平台（桌面/移动/Web/小游戏宿主）行为一致。

# 单个计时器
class _Timer:
	var id: int
	var ttl: float          # 间隔秒数
	var remaining: float    # 剩余秒数
	var once: bool          # true=触发一次后移除；false=循环
	var late: bool          # true=在 late 链
	var cb: Callable

var _next_id: int = 1
var _timers: Array[_Timer] = []        # process 链
var _late_timers: Array[_Timer] = []   # late 链

# Tick 期间的增删暂存（避免遍历中修改）
var _pending_add: Array[_Timer] = []
var _pending_del: Dictionary = {}      # id -> true
var _ticking: bool = false


func _ready() -> void:
	set_process(true)


# ---------------------------------------------------------------------------
# 公共 API
# ---------------------------------------------------------------------------

## 延迟 ttl 秒后触发一次。返回 timer id。
func after(ttl: float, cb: Callable, late: bool = false) -> int:
	return _add(ttl, true, cb, late)

## 每隔 ttl 秒循环触发。返回 timer id。
func every(ttl: float, cb: Callable, late: bool = false) -> int:
	return _add(ttl, false, cb, late)

## 取消计时器
func cancel(id: int) -> void:
	if id <= 0:
		return
	if _ticking:
		_pending_del[id] = true
		return
	_remove_now(id)

## 重置计时器（重新开始倒计时）
func reset(id: int) -> void:
	for t in _timers:
		if t.id == id:
			t.remaining = t.ttl
			return
	for t in _late_timers:
		if t.id == id:
			t.remaining = t.ttl
			return

## 清空全部
func clear() -> void:
	_timers.clear()
	_late_timers.clear()
	_pending_add.clear()
	_pending_del.clear()

## 当前活跃计时器数
func count() -> int:
	return _timers.size() + _late_timers.size()


# ---------------------------------------------------------------------------
# 内部
# ---------------------------------------------------------------------------

func _add(ttl: float, once: bool, cb: Callable, late: bool) -> int:
	if not cb.is_valid():
		push_error("[TimerService] add: callback invalid")
		return 0
	var t := _Timer.new()
	t.id = _next_id
	_next_id += 1
	t.ttl = maxf(ttl, 0.0)
	t.remaining = t.ttl
	t.once = once
	t.late = late
	t.cb = cb
	if _ticking:
		_pending_add.append(t)
	else:
		if late:
			_late_timers.append(t)
		else:
			_timers.append(t)
	return t.id

func _remove_now(id: int) -> void:
	for i in range(_timers.size()):
		if _timers[i].id == id:
			_timers.remove_at(i)
			return
	for i in range(_late_timers.size()):
		if _late_timers[i].id == id:
			_late_timers.remove_at(i)
			return


func _process(delta: float) -> void:
	_tick_list(_timers, delta)
	# late 链在同一帧、process 链之后跑（用 call_deferred 保证在本帧末）
	_tick_list(_late_timers, delta)
	_flush_pending()


func _tick_list(list: Array[_Timer], delta: float) -> void:
	if list.is_empty():
		return
	_ticking = true
	var i := 0
	while i < list.size():
		var t := list[i]
		# 跳过本 Tick 内被标记删除的
		if _pending_del.has(t.id):
			i += 1
			continue
		t.remaining -= delta
		if t.remaining <= 0.0:
			if t.cb.is_valid():
				t.cb.call()
			if t.once:
				list.remove_at(i)
				continue   # 不 i++，当前位置已是下一个
			else:
				# 循环：补上超出的时间（避免长帧漂移）
				t.remaining += t.ttl
				if t.remaining <= 0.0:
					t.remaining = t.ttl
		i += 1
	_ticking = false


func _flush_pending() -> void:
	# 处理延迟删除
	if not _pending_del.is_empty():
		for id in _pending_del.keys():
			_remove_now(int(id))
		_pending_del.clear()
	# 补入延迟新增
	if not _pending_add.is_empty():
		for t in _pending_add:
			if t.late:
				_late_timers.append(t)
			else:
				_timers.append(t)
		_pending_add.clear()
