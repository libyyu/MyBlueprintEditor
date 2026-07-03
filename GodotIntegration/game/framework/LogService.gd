extends Node
## LogService —— 分级日志（Autoload 单例），异步（多线程）写盘。
##
## 对齐 Unity 的 FileLoggger，并增强两点：
##   1) 变参、print 风格：info("a", 1, true, obj) —— 任意个参数，各自 str() 后以空格连接
##   2) 多线程写盘：日志入内存队列，专用后台线程批量落盘，主线程/游戏线程零阻塞、零频繁开关文件
##
## 级别 debug / info / warn / error，每条带 [yyyyMMddHHmmss]，格式 "[time]-[level]message"。
## 文件：user://logs/gamelog.log（启动清空重建）。控制台同步输出（error 走 printerr）。
## Web/小游戏无持久文件系统或无线程：file_enabled / threaded 关掉即可退化为仅控制台/同步写。
##
## 用法：
##   GDScript： LogService.info("hp=", hp, " pos=", pos);  LogService.error("boom", err)
##   Lua（经 Log 表）： Log.info("hp=", hp)  -- 变参在 C 侧已拼好传入单串

const LOG_DIR := "user://logs"
const LOG_FILE := "user://logs/gamelog.log"

## 是否写文件。桌面/移动 true；Web/小游戏可设 false（仅控制台）。
@export var file_enabled: bool = true
## 是否用后台线程写盘。为 false 时同步写（线程不可用的平台，如单线程 Web/小游戏）。
@export var threaded: bool = true
## 最低输出级别（低于此级别的丢弃）。0=debug 1=info 2=warn 3=error
@export var min_level: int = 0
## 队列达到该条数时唤醒写线程立即落盘（否则按 flush 间隔）。
@export var batch_size: int = 32

const LV_DEBUG := 0
const LV_INFO := 1
const LV_WARN := 2
const LV_ERROR := 3
const _LV_NAME := ["debug", "info", "warn", "error"]

# ---- 多线程写盘 ----
var _thread: Thread = null
var _mutex: Mutex = null
var _sem: Semaphore = null
var _queue: PackedStringArray = PackedStringArray()   # 待落盘的成行日志
var _exit: bool = false                                # 通知写线程退出


func _ready() -> void:
	if file_enabled:
		DirAccess.make_dir_recursive_absolute(LOG_DIR)
		# 启动清空重建
		var f := FileAccess.open(LOG_FILE, FileAccess.WRITE)
		if f:
			f.close()
		else:
			push_warning("[LogService] cannot open log file, console only: " + LOG_FILE)
			file_enabled = false

	if file_enabled and threaded:
		_mutex = Mutex.new()
		_sem = Semaphore.new()
		_thread = Thread.new()
		_thread.start(_writer_loop)

	print("[LogService] ready, file_enabled=", file_enabled, " threaded=", (threaded and file_enabled),
		  " path=", ProjectSettings.globalize_path(LOG_FILE) if file_enabled else "(console only)")


func _exit_tree() -> void:
	_shutdown()


## 主动 flush 并停止写线程（退出时自动调用；也可手动调）。
func _shutdown() -> void:
	if _thread == null:
		return
	_mutex.lock()
	_exit = true
	_mutex.unlock()
	_sem.post()
	_thread.wait_to_finish()
	_thread = null


# ---------------------------------------------------------------------------
# 公共 API —— 变参（print 风格）
# ---------------------------------------------------------------------------

func debug(...args) -> void: _log_v(LV_DEBUG, args)
func info(...args) -> void:  _log_v(LV_INFO, args)
func warn(...args) -> void:  _log_v(LV_WARN, args)
func error(...args) -> void: _log_v(LV_ERROR, args)

## 通用入口：level 用 LV_* 常量，msg 支持变参
func log_at(level: int, ...args) -> void:
	_log_v(level, args)


# ---------------------------------------------------------------------------
# 内部
# ---------------------------------------------------------------------------

func _log_v(level: int, args: Array) -> void:
	if level < min_level:
		return
	# 变参拼接：各 str() 后空格连接（print 风格）
	var parts := PackedStringArray()
	for a in args:
		parts.append(str(a))
	_emit_line(level, " ".join(parts))


func _emit_line(level: int, msg: String) -> void:
	var lv_name: String = _LV_NAME[level] if level >= 0 and level < _LV_NAME.size() else "info"
	var line := "[%s]-[%s]%s" % [_timestamp(), lv_name, msg]

	# 控制台（主线程）
	if level >= LV_ERROR:
		printerr(line)
	else:
		print(line)

	if not file_enabled:
		return

	if _thread != null:
		# 异步：入队 + 唤醒写线程
		_mutex.lock()
		_queue.append(line)
		var should_post := _queue.size() >= 1
		_mutex.unlock()
		if should_post:
			_sem.post()
	else:
		# 同步兜底（threaded=false）
		_append_lines(PackedStringArray([line]))


# 写线程主循环：等信号 → 取走整批 → 一次打开文件顺序落盘。
func _writer_loop() -> void:
	while true:
		_sem.wait()
		var batch: PackedStringArray
		var exiting := false
		_mutex.lock()
		batch = _queue.duplicate()
		_queue.clear()
		exiting = _exit
		_mutex.unlock()

		if not batch.is_empty():
			_append_lines(batch)

		if exiting:
			# 退出前把可能残留的再落一次
			_mutex.lock()
			var rest := _queue.duplicate()
			_queue.clear()
			_mutex.unlock()
			if not rest.is_empty():
				_append_lines(rest)
			return


# 一次打开文件、批量追加（写线程或同步兜底调用）。
func _append_lines(lines: PackedStringArray) -> void:
	var f := FileAccess.open(LOG_FILE, FileAccess.READ_WRITE)
	if f == null:
		f = FileAccess.open(LOG_FILE, FileAccess.WRITE)
	if f == null:
		return
	f.seek_end()
	for ln in lines:
		f.store_line(ln)
	f.close()


func _timestamp() -> String:
	var t := Time.get_datetime_dict_from_system()
	return "%04d%02d%02d%02d%02d%02d" % [t.year, t.month, t.day, t.hour, t.minute, t.second]
