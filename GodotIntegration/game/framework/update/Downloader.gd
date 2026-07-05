# Downloader.gd
extends Node
class_name Downloader

enum DownloadResult { OK, FAILED, CANCELED }

signal download_progress(downloaded: int, total: int, speed: String)
signal download_success(save_path: String)
signal download_failed(reason: String)
signal download_canceled()
signal download_finished(result: DownloadResult, reason: String)

signal unzip_failed(reason: String)
signal unzip_progress(current: int, total: int)
signal unzip_finished(path: String)

# --------------- 配置 ---------------
var max_retry   : int   = 3
var timeout_sec : float = 30.0
var check_md5   : bool  = false
var expect_md5  : String = ""

# --------------- 内部状态 ---------------
var _http: HTTPRequest
var _url: String
var _save_path: String
var _canceled: bool = false
var _retry_count: int = 0

var _downloaded_bytes: int = 0
var _total_bytes: int = 0
var _last_bytes: int = 0
var _last_time: int = 0

# ================================
# 对外 API
# ================================

func download_async(
	url: String,
	save_path: String,
	md5: String = "",
	retry: int = 3,
	timeout: float = 30.0
) -> void:
	_url = url
	_save_path = save_path
	expect_md5 = md5
	check_md5 = md5 != ""
	max_retry = retry
	timeout_sec = timeout
	_retry_count = 0
	_canceled = false

	_downloaded_bytes = 0
	_total_bytes = 0

	_start_request()

func download(
	url: String,
	save_path: String,
	md5: String = "",
	retry: int = 3,
	timeout: float = 30.0
) -> DownloadResult:
	_url = url
	_save_path = save_path
	expect_md5 = md5
	check_md5 = md5 != ""
	max_retry = retry
	timeout_sec = timeout
	_retry_count = 0
	_canceled = false

	_downloaded_bytes = 0
	_total_bytes = 0

	_start_request()
	var result = await download_finished
	return result[0]


func cancel() -> void:
	if not _http:
		return
	if _canceled:
		return
	_canceled = true
	if _http.is_inside_tree():
		_http.cancel_request()
	download_canceled.emit()
	download_finished.emit(DownloadResult.CANCELED, "download is canceled")


# ================================
# 核心逻辑
# ================================

func _start_request() -> void:
	if _canceled:
		return

	_http = HTTPRequest.new()
	add_child(_http)
	_http.timeout = timeout_sec
	_http.download_file = _save_path

	_http.request_completed.connect(_on_completed)
	if _http.has_signal("download_progress"):
		_http.download_progress.connect(_on_progress)
	elif _http.has_signal("progress"):
		_http.progress.connect(_on_progress)
	else:
		push_error("[Downloader] no progress singal")
	

	# 确保保存目录存在
	var dir_path = _save_path.get_base_dir()
	if not DirAccess.dir_exists_absolute(dir_path):
		var err = DirAccess.make_dir_recursive_absolute(dir_path)
		if err != OK:
			var errmsg: String = "下载失败。无法创建目录"+dir_path
			download_failed.emit(errmsg)
			download_finished.emit(DownloadResult.FAILED, errmsg)
			return

	# 断点续传
	var file = FileAccess.open(_save_path, FileAccess.READ)
	if file:
		_downloaded_bytes = file.get_length()
		file.close()

	var headers: PackedStringArray = []
	if _downloaded_bytes > 0:
		headers.append("Range: bytes=%d-" % _downloaded_bytes)

	var err = _http.request(_url, headers)
	if err != OK:
		_try_retry("HTTPRequest 启动失败")


# ================================
# 进度回调
# ================================

func _on_progress(total: int, downloaded: int) -> void:
	if _canceled:
		return

	_downloaded_bytes = downloaded
	_total_bytes = total

	# 计算速度
	var now = Time.get_ticks_msec()
	var speed := ""
	if _last_time > 0:
		var dt = (now - _last_time) / 1000.0
		var db = downloaded - _last_bytes
		if dt > 0:
			speed = _format_size(db / dt) + "/s"

	_last_bytes = downloaded
	_last_time = now

	download_progress.emit(downloaded, total, speed)


# ================================
# 完成 / 失败处理
# ================================

func _on_completed(result: int, code: int, _headers: PackedStringArray, _body: PackedByteArray) -> void:
	_http.queue_free()

	if _canceled:
		print("downloader" + _url + " is canceld")
		return

	# 超时
	if result == HTTPRequest.RESULT_TIMEOUT:
		_try_retry("下载超时")
		return

	# 网络 / 协议失败
	if result != HTTPRequest.RESULT_SUCCESS:
		_try_retry("网络错误 code=%d" % result)
		return

	# HTTP 失败
	if code >= 400:
		_try_retry("HTTP %d" % code)
		return

	# 206 没触发 Range（服务器不支持续传）
	if code == 200:
		_downloaded_bytes = 0  # 重新开始

	# MD5 校验
	if check_md5 and not _check_md5():
		_try_retry("MD5 校验失败")
		return
		
	download_success.emit(_save_path)
	download_finished.emit(DownloadResult.OK, "")


func _try_retry(reason: String) -> void:
	_retry_count += 1
	if _retry_count <= max_retry:
		push_warning("下载失败，重试 %d/%d -> %s" % [_retry_count, max_retry, reason])
		_start_request()
	else:
		download_failed.emit(reason)
		download_finished.emit(DownloadResult.FAILED, reason)


# ================================
# MD5 校验
# ================================

func _check_md5() -> bool:
	var file = FileAccess.file_exists(_save_path)
	if not file:
		return false
	return FileAccess.get_md5(_save_path) == expect_md5


# ================================
# ZIP 解压
# ================================

func unzip(path: String, out_dir: String) -> void:
	var zip = ZIPReader.new()
	if zip.open(path) != OK:
		unzip_failed.emit("无法打开 ZIP 文件")
		return

	var files = zip.get_files()
	var total = files.size()
	var count = 0

	for f in files:
		if _canceled:
			break

		var data = zip.read_file(f)
		var full = out_dir + "/" + f
		var dir = full.get_base_dir()

		DirAccess.make_dir_recursive_absolute(dir)

		var out = FileAccess.open(full, FileAccess.WRITE)
		if out:
			out.store_buffer(data)
			out.close()

		count += 1
		unzip_progress.emit(count, total)

	zip.close()
	unzip_finished.emit(out_dir)


# ================================
# 工具函数
# ================================

func _format_size(bytes: float) -> String:
	if bytes < 1024:
		return "%.0f B" % bytes
	if bytes < 1024 * 1024:
		return "%.1f KB" % (bytes / 1024.0)
	return "%.2f MB" % (bytes / (1024.0 * 1024.0))
