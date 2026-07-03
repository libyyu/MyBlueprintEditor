extends Node
## LogService —— 分级文件日志（Autoload 单例）
##
## 对齐 Unity 的 FileLoggger：
##   - 级别 debug / info / warn / error
##   - 每条带时间戳 [yyyyMMddHHmmss]，格式 "[time]-[level]message"
##   - 写到 user://logs/gamelog.log（启动时清空重建）
##   - 同时 print 到控制台（error 走 printerr）
##   - Web/小游戏无持久文件系统：file_enabled=false 时仅控制台（不写文件）
##
## 用法：
##   GDScript： LogService.info("hello");  LogService.error("boom")
##   Lua（经 Log 表）： Log.info("hello");  Log.warn("...")

const LOG_DIR := "user://logs"
const LOG_FILE := "user://logs/gamelog.log"

## 是否写文件。桌面/移动 true；Web/小游戏可设 false（仅控制台）。
@export var file_enabled: bool = true
## 最低输出级别（低于此级别的丢弃）。0=debug 1=info 2=warn 3=error
@export var min_level: int = 0

const LV_DEBUG := 0
const LV_INFO := 1
const LV_WARN := 2
const LV_ERROR := 3
const _LV_NAME := ["debug", "info", "warn", "error"]

var _ready_ok: bool = false


func _ready() -> void:
	if file_enabled:
		DirAccess.make_dir_recursive_absolute(LOG_DIR)
		# 启动清空重建
		var f := FileAccess.open(LOG_FILE, FileAccess.WRITE)
		if f:
			f.store_string("")   # 截断
			f.close()
			_ready_ok = true
		else:
			push_warning("[LogService] cannot open log file, console only: " + LOG_FILE)
			file_enabled = false
	print("[LogService] ready, file_enabled=", file_enabled,
		  " path=", ProjectSettings.globalize_path(LOG_FILE) if file_enabled else "(console only)")


# ---------------------------------------------------------------------------
# 公共 API
# ---------------------------------------------------------------------------

func debug(msg: String) -> void:
	_log(LV_DEBUG, msg)

func info(msg: String) -> void:
	_log(LV_INFO, msg)

func warn(msg: String) -> void:
	_log(LV_WARN, msg)

func error(msg: String) -> void:
	_log(LV_ERROR, msg)

## 通用入口：level 用上面的 LV_* 常量
func log_at(level: int, msg: String) -> void:
	_log(level, msg)


# ---------------------------------------------------------------------------
# 内部
# ---------------------------------------------------------------------------

func _log(level: int, msg: String) -> void:
	if level < min_level:
		return
	var lv_name: String = _LV_NAME[level] if level >= 0 and level < _LV_NAME.size() else "info"
	var line := "[%s]-[%s]%s" % [_timestamp(), lv_name, msg]

	# 控制台
	if level >= LV_ERROR:
		printerr(line)
	else:
		print(line)

	# 文件（追加）
	if file_enabled:
		var f := FileAccess.open(LOG_FILE, FileAccess.READ_WRITE)
		if f == null:
			f = FileAccess.open(LOG_FILE, FileAccess.WRITE)
		if f:
			f.seek_end()
			f.store_line(line)
			f.close()


func _timestamp() -> String:
	var t := Time.get_datetime_dict_from_system()
	return "%04d%02d%02d%02d%02d%02d" % [t.year, t.month, t.day, t.hour, t.minute, t.second]
