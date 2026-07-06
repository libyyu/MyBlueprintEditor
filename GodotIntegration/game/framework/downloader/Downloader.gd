# Downloader.gd
#
# 基于 HTTPClient 的流式下载器。相较 HTTPRequest：
#   * 真正边收边写，避免大文件时 request_completed(body) 把整包塞进内存；
#   * 进度来自每帧 poll 出的 chunk 大小，不依赖 HTTPRequest 未公开的 progress 信号；
#   * 支持断点续传（Range），服务器不认（返回 200）时会自动回退到从头写。
#
# 对外 API 与旧版保持一致：
#   download_async(url, save_path, md5, retry, timeout)
#   download(...)               # 协程版，await 到 download_finished
#   cancel()
#   信号: precheck_result / download_progress / download_success / download_failed /
#         download_canceled / download_finished / unzip_*
#
# 下载前预校验：若本地已有目标文件且传入了 md5，先做 MD5 比对——
#   命中则跳过网络下载直接成功；不命中则删除旧文件从头下载。
#   通过 precheck_before_download 开关控制（默认开）。
extends Node
class_name Downloader

enum DownloadResult { OK, FAILED, CANCELED }

signal download_progress(downloaded: int, total: int, speed: String)
signal download_success(save_path: String)
signal download_failed(reason: String)
signal download_canceled()
signal download_finished(result: DownloadResult, reason: String)

# 下载正式开始前，对磁盘上已存在的文件做 MD5 预校验的结果通知。
# 仅在设置了 expect_md5 且本地文件存在时发射。
#   matched=true  -> 命中缓存，跳过下载直接成功
#   matched=false -> 文件损坏/过期，将删除并从头下载
signal precheck_result(matched: bool, local_md5: String, expect_md5: String)

signal unzip_failed(reason: String)
signal unzip_progress(current: int, total: int)
signal unzip_finished(path: String)

# --------------- 配置 ---------------
var max_retry   : int   = 3
var timeout_sec : float = 30.0
var check_md5   : bool  = false
var expect_md5  : String = ""

# 下载前预校验：若本地已存在目标文件且提供了 md5，先做一次 MD5 校验。
#   通过  -> 直接判定成功，跳过网络下载（秒开）；
#   不通过-> 删除本地文件，从头完整下载。
# 只有在传入了 md5 时才生效。
var precheck_before_download : bool = true

# HTTP 头配置：user_agent 单独一项方便使用；
# custom_headers 是额外键值对（如 Authorization、Cookie 等），会覆盖同名默认头。
var user_agent  : String = "BlueprintDownloader/1.0 (Godot)"
var custom_headers : Dictionary = {}   # { "Header-Name": "value" }

# --------------- 内部状态 ---------------
var _client: HTTPClient
var _file: FileAccess

var _url: String
var _save_path: String
var _host: String
var _port: int = 80
var _use_tls: bool = false
var _request_path: String = "/"
var _request_headers: PackedStringArray = PackedStringArray()

var _canceled: bool = false
var _retry_count: int = 0
var _running: bool = false
var _request_sent: bool = false
var _body_started: bool = false

var _downloaded_bytes: int = 0     # 已经落盘的字节数
var _resume_offset: int = 0        # 本次请求开始时磁盘上已有的字节数
var _total_bytes: int = 0          # Content-Length + resume_offset

var _last_bytes: int = 0
var _last_time_ms: int = 0
var _last_activity_ms: int = 0

var _redirect_count: int = 0

const _CHUNK_EMIT_INTERVAL_MS := 200   # 进度信号至少这么久发一次，避免 spam
const _MAX_REDIRECTS := 5

# ================================
# 对外 API
# ================================

func _ready() -> void:
	set_process(false)

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
	_redirect_count = 0
	_canceled = false

	# 下载前预校验：本地已有文件 + 提供了 md5 时，先校验
	if precheck_before_download and check_md5 and FileAccess.file_exists(_save_path):
		var local_md5 := FileAccess.get_md5(_save_path)
		var matched := local_md5 == expect_md5
		precheck_result.emit(matched, local_md5, expect_md5)
		if matched:
			# 命中缓存，跳过下载
			var size := 0
			var f := FileAccess.open(_save_path, FileAccess.READ)
			if f:
				size = f.get_length()
				f.close()
			print("[Downloader] 预校验命中，跳过下载: %s" % _save_path)
			download_progress.emit(size, size, "")
			download_success.emit(_save_path)
			download_finished.emit(DownloadResult.OK, "")
			return
		else:
			# 文件损坏/过期，删除后从头下载
			push_warning("[Downloader] 预校验失败(local=%s expect=%s)，删除并重新下载" % [local_md5, expect_md5])
			DirAccess.remove_absolute(_save_path)

	_start_request()

func download(
	url: String,
	save_path: String,
	md5: String = "",
	retry: int = 3,
	timeout: float = 30.0
) -> DownloadResult:
	download_async(url, save_path, md5, retry, timeout)
	var result = await download_finished
	return result[0]


# --------- Header 设置接口 ---------
# 设置 User-Agent。传空串则不发送 User-Agent（连默认也不发）。
func set_user_agent(ua: String) -> void:
	user_agent = ua

# 覆盖式设置全部自定义头。
# headers 形如 { "Authorization": "Bearer xxx", "Cookie": "sid=..." }
func set_headers(headers: Dictionary) -> void:
	custom_headers = headers.duplicate()

# 追加/覆盖单条自定义头。
func set_header(name: String, value: String) -> void:
	custom_headers[name] = value

# 删除自定义头。
func remove_header(name: String) -> void:
	custom_headers.erase(name)

# 清空自定义头（但不影响 user_agent）。
func clear_headers() -> void:
	custom_headers.clear()


func cancel() -> void:
	if _canceled:
		return
	if not _running:
		return
	_canceled = true
	_teardown_client(true)
	download_canceled.emit()
	download_finished.emit(DownloadResult.CANCELED, "download is canceled")


# ================================
# 核心逻辑
# ================================

func _start_request() -> void:
	if _canceled:
		return

	# 解析 URL
	if not _parse_url(_url):
		_try_retry("URL 解析失败: %s" % _url)
		return

	# 确保保存目录存在
	var dir_path := _save_path.get_base_dir()
	if dir_path != "" and not DirAccess.dir_exists_absolute(dir_path):
		var err := DirAccess.make_dir_recursive_absolute(dir_path)
		if err != OK:
			var errmsg := "下载失败。无法创建目录 " + dir_path
			download_failed.emit(errmsg)
			download_finished.emit(DownloadResult.FAILED, errmsg)
			return

	# 断点续传：读取本地已下载大小
	_resume_offset = 0
	if FileAccess.file_exists(_save_path):
		var f := FileAccess.open(_save_path, FileAccess.READ)
		if f:
			_resume_offset = f.get_length()
			f.close()
	_downloaded_bytes = _resume_offset
	_total_bytes = 0

	_request_headers = _build_headers(_resume_offset)

	# 建立 HTTPClient
	_client = HTTPClient.new()
	var connect_err := OK
	if _use_tls:
		connect_err = _client.connect_to_host(_host, _port, TLSOptions.client())
	else:
		connect_err = _client.connect_to_host(_host, _port)
	if connect_err != OK:
		_try_retry("connect_to_host 失败: %d" % connect_err)
		return

	# 重置泵状态
	_request_sent = false
	_body_started = false
	_last_bytes = _downloaded_bytes
	_last_time_ms = Time.get_ticks_msec()
	_last_activity_ms = _last_time_ms
	_running = true
	set_process(true)


func _process(_dt: float) -> void:
	if not _running or _canceled or _client == null:
		return

	_client.poll()
	var status := _client.get_status()

	# 超时检测（无任何进度活动超过 timeout_sec）
	var now := Time.get_ticks_msec()
	if timeout_sec > 0 and now - _last_activity_ms > int(timeout_sec * 1000.0):
		_teardown_client(true)
		_try_retry("下载超时")
		return

	match status:
		HTTPClient.STATUS_CONNECTING, HTTPClient.STATUS_RESOLVING:
			return
		HTTPClient.STATUS_CONNECTED:
			if not _request_sent:
				var err := _client.request(HTTPClient.METHOD_GET, _request_path, _request_headers)
				if err != OK:
					_teardown_client(true)
					_try_retry("HTTPClient.request 失败: %d" % err)
					return
				_request_sent = true
				_last_activity_ms = now
			else:
				# 请求已发但状态回到 CONNECTED —— 说明 body 已经收完（keep-alive）
				if _body_started:
					_finish_ok()
		HTTPClient.STATUS_REQUESTING:
			return
		HTTPClient.STATUS_BODY:
			_pump_body()
		HTTPClient.STATUS_DISCONNECTED:
			# 若 body 阶段结束后断开，也算收完
			if _body_started:
				_finish_ok()
			else:
				_teardown_client(true)
				_try_retry("连接断开")
		HTTPClient.STATUS_CONNECTION_ERROR, \
		HTTPClient.STATUS_CANT_CONNECT, \
		HTTPClient.STATUS_CANT_RESOLVE, \
		HTTPClient.STATUS_TLS_HANDSHAKE_ERROR:
			_teardown_client(true)
			_try_retry("网络错误 status=%d" % status)


func _pump_body() -> void:
	# 首次进入 BODY：处理响应码 / 打开文件
	if not _body_started:
		var code := _client.get_response_code()

		# 3xx 重定向：读 Location，重开一次请求（不消耗 retry 次数）
		if code >= 300 and code < 400:
			var location := _find_header(_client.get_response_headers(), "location")
			if location == "":
				_teardown_client(true)
				_try_retry("HTTP %d 缺少 Location 头" % code)
				return
			_redirect_count += 1
			if _redirect_count > _MAX_REDIRECTS:
				_teardown_client(true)
				_try_retry("重定向超过 %d 次" % _MAX_REDIRECTS)
				return
			var new_url := _resolve_redirect_url(_url, location)
			print("[Downloader] redirect %d -> %s" % [code, new_url])
			_teardown_client(false)
			_url = new_url
			# 重定向后重新开始，别累加 retry_count
			_start_request()
			return

		# 416 Range Not Satisfiable：请求的续传起点 >= 服务器文件大小。
		# 绝大多数情况是"本地文件已经下完了"，不应作为失败重试。
		# 处理：关闭连接，用本地文件长度 / MD5 判定是否真的完成。
		if code == 416:
			# teardown 前先把 Content-Range 里的服务器总大小读出来
			var server_total := _parse_content_range_total(_client.get_response_headers())
			_teardown_client(false)
			_handle_range_not_satisfiable(server_total)
			return

		if code >= 400:
			_teardown_client(true)
			_try_retry("HTTP %d" % code)
			return

		# 206 表示服务器接受了续传；200 表示忽略了 Range，得从头写
		var from_scratch := (_resume_offset > 0 and code == 200) or _resume_offset == 0

		var mode := FileAccess.WRITE if from_scratch else FileAccess.READ_WRITE
		_file = FileAccess.open(_save_path, mode)
		if _file == null:
			_teardown_client(true)
			_try_retry("无法写入文件: %s" % _save_path)
			return

		if from_scratch:
			_downloaded_bytes = 0
		else:
			_file.seek_end()
			_downloaded_bytes = _resume_offset

		var body_len := _client.get_response_body_length()   # Content-Length；-1 = 未知
		if body_len > 0:
			_total_bytes = body_len + (_downloaded_bytes if not from_scratch else 0)
		else:
			_total_bytes = 0

		_body_started = true
		_last_bytes = _downloaded_bytes
		_last_time_ms = Time.get_ticks_msec()
		_last_activity_ms = _last_time_ms

	# 循环读取当前帧就绪的所有 chunk
	var chunks_this_frame := 0
	while _body_started and _client.get_status() == HTTPClient.STATUS_BODY:
		_client.poll()
		var chunk := _client.read_response_body_chunk()
		if chunk.size() == 0:
			break
		_file.store_buffer(chunk)
		_downloaded_bytes += chunk.size()
		_last_activity_ms = Time.get_ticks_msec()
		chunks_this_frame += 1
		# 单帧读得太多也让出去，避免阻塞主线程
		if chunks_this_frame >= 32:
			break

	_maybe_emit_progress()


func _maybe_emit_progress() -> void:
	var now := Time.get_ticks_msec()
	var dt_ms := now - _last_time_ms
	if dt_ms < _CHUNK_EMIT_INTERVAL_MS and _downloaded_bytes < _total_bytes:
		return

	var speed := ""
	if dt_ms > 0:
		var db := _downloaded_bytes - _last_bytes
		var sec := dt_ms / 1000.0
		speed = _format_size(db / sec) + "/s"

	_last_time_ms = now
	_last_bytes = _downloaded_bytes
	download_progress.emit(_downloaded_bytes, _total_bytes, speed)


# ================================
# 完成 / 失败处理
# ================================

func _finish_ok() -> void:
	_teardown_client(false)

	# 最后再发一次 100% 进度，UI 好收尾
	download_progress.emit(_downloaded_bytes, _total_bytes if _total_bytes > 0 else _downloaded_bytes, "")

	if check_md5 and not _check_md5():
		_try_retry("MD5 校验失败")
		return

	download_success.emit(_save_path)
	download_finished.emit(DownloadResult.OK, "")


# 处理 416：请求的续传起点越界，通常意味着本地文件已下载完整。
# server_total: 从 Content-Range 解析出的服务器文件总大小，-1 表示未知。
func _handle_range_not_satisfiable(server_total: int) -> void:
	var local_size := 0
	if FileAccess.file_exists(_save_path):
		var f := FileAccess.open(_save_path, FileAccess.READ)
		if f:
			local_size = f.get_length()
			f.close()

	# 情况 A：本地根本没文件，却收到 416 —— 服务器行为异常，按失败重试
	if local_size == 0:
		_redo_from_scratch("HTTP 416 但本地无文件")
		return

	# 情况 B：开了 MD5 校验，直接以 MD5 为准（最可靠）
	if check_md5:
		if _check_md5():
			_downloaded_bytes = local_size
			_total_bytes = local_size
			download_progress.emit(local_size, local_size, "")
			print("[Downloader] 416 -> 本地文件已完整（MD5 通过）")
			download_success.emit(_save_path)
			download_finished.emit(DownloadResult.OK, "")
		else:
			# 本地文件损坏/过期，删掉从头下
			_redo_from_scratch("HTTP 416 且 MD5 不匹配，重新下载")
		return

	# 情况 C：没开 MD5，但拿到了服务器总大小 —— 比对长度
	if server_total > 0:
		if local_size == server_total:
			_downloaded_bytes = local_size
			_total_bytes = local_size
			download_progress.emit(local_size, local_size, "")
			print("[Downloader] 416 -> 本地文件大小与服务器一致（%d 字节），视为完成" % local_size)
			download_success.emit(_save_path)
			download_finished.emit(DownloadResult.OK, "")
		else:
			# 本地比服务器大（或不一致）说明文件是坏的/换版了
			_redo_from_scratch("HTTP 416：本地 %d 字节 != 服务器 %d 字节，重新下载" % [local_size, server_total])
		return

	# 情况 D：既没 MD5 又拿不到服务器大小 —— 保守认定已完成
	# （有本地文件 + 416 越界，最可能就是下完了）
	_downloaded_bytes = local_size
	_total_bytes = local_size
	download_progress.emit(local_size, local_size, "")
	push_warning("[Downloader] 416 且无法获取服务器大小，按本地文件已完整处理（建议开启 MD5 校验）")
	download_success.emit(_save_path)
	download_finished.emit(DownloadResult.OK, "")


# 删除本地文件并从头重新下载（消耗一次 retry）
func _redo_from_scratch(reason: String) -> void:
	if FileAccess.file_exists(_save_path):
		DirAccess.remove_absolute(_save_path)
	_resume_offset = 0
	_downloaded_bytes = 0
	_try_retry(reason)


# 从响应头解析 Content-Range 的总大小。
# 形如 "Content-Range: bytes */12345" 或 "bytes 0-99/12345"，返回斜杠后的总大小。
func _parse_content_range_total(headers: PackedStringArray) -> int:
	var cr := _find_header(headers, "content-range")
	if cr == "":
		return -1
	var slash := cr.rfind("/")
	if slash < 0:
		return -1
	var total_str := cr.substr(slash + 1).strip_edges()
	if total_str == "" or total_str == "*":
		return -1
	if not total_str.is_valid_int():
		return -1
	return total_str.to_int()


func _teardown_client(_error: bool) -> void:
	_running = false
	set_process(false)
	if _file != null:
		_file.close()
		_file = null
	if _client != null:
		_client.close()
		_client = null


func _try_retry(reason: String) -> void:
	_retry_count += 1
	if _retry_count <= max_retry:
		push_warning("下载失败，重试 %d/%d -> %s" % [_retry_count, max_retry, reason])
		_start_request()
	else:
		download_failed.emit(reason)
		download_finished.emit(DownloadResult.FAILED, reason)


# ================================
# URL 解析
# ================================

func _parse_url(url: String) -> bool:
	# 支持 http://host[:port]/path 与 https://host[:port]/path
	var lower := url.to_lower()
	if lower.begins_with("https://"):
		_use_tls = true
		_port = 443
		url = url.substr(8)
	elif lower.begins_with("http://"):
		_use_tls = false
		_port = 80
		url = url.substr(7)
	else:
		return false

	var slash := url.find("/")
	var host_part := ""
	if slash < 0:
		host_part = url
		_request_path = "/"
	else:
		host_part = url.substr(0, slash)
		_request_path = url.substr(slash)

	var colon := host_part.find(":")
	if colon >= 0:
		_host = host_part.substr(0, colon)
		_port = int(host_part.substr(colon + 1))
	else:
		_host = host_part

	return _host != ""


# ================================
# 请求头组装
# ================================

func _build_headers(resume_offset: int) -> PackedStringArray:
	# 收集"我们准备发的头"，key 用小写做去重键。
	# 优先级: custom_headers > 内建头（Range / User-Agent）
	var lower_keys := {}   # lower_name -> true
	var out := PackedStringArray()

	# 1. custom_headers 先入，占坑
	for k in custom_headers.keys():
		var name := String(k)
		var val := String(custom_headers[k])
		if name.strip_edges() == "":
			continue
		out.append("%s: %s" % [name, val])
		lower_keys[name.to_lower()] = true

	# 2. Range —— 若用户没自己指定
	if resume_offset > 0 and not lower_keys.has("range"):
		out.append("Range: bytes=%d-" % resume_offset)
		lower_keys["range"] = true

	# 3. User-Agent —— 若用户没自己指定且默认非空
	if user_agent != "" and not lower_keys.has("user-agent"):
		out.append("User-Agent: %s" % user_agent)
		lower_keys["user-agent"] = true

	return out


# ================================
# 重定向辅助
# ================================

func _find_header(headers: PackedStringArray, name_lower: String) -> String:
	# Godot 返回的头形如 "Location: http://..."，大小写按服务器给的，
	# 这里按 name 忽略大小写查找。
	for h in headers:
		var colon := h.find(":")
		if colon < 0:
			continue
		var key := h.substr(0, colon).strip_edges().to_lower()
		if key == name_lower:
			return h.substr(colon + 1).strip_edges()
	return ""


func _resolve_redirect_url(base: String, location: String) -> String:
	# 1. 绝对 URL
	var loc_lower := location.to_lower()
	if loc_lower.begins_with("http://") or loc_lower.begins_with("https://"):
		return location

	# 2. 协议相对: //host/path
	if location.begins_with("//"):
		var scheme := "https:" if base.to_lower().begins_with("https://") else "http:"
		return scheme + location

	# 3. 主机相对: /path 或 相对路径
	# 从 base 里抽出 scheme://host[:port]
	var scheme_end := base.find("://")
	if scheme_end < 0:
		return location
	var after_scheme := scheme_end + 3
	var host_end := base.find("/", after_scheme)
	var origin := base if host_end < 0 else base.substr(0, host_end)

	if location.begins_with("/"):
		return origin + location

	# 纯相对路径（少见）：接在 base 目录后
	var base_dir := base
	var last_slash := base.rfind("/")
	if last_slash > after_scheme:
		base_dir = base.substr(0, last_slash + 1)
	else:
		base_dir = origin + "/"
	return base_dir + location


# ================================
# MD5 校验
# ================================

func _check_md5() -> bool:
	if not FileAccess.file_exists(_save_path):
		return false
	return FileAccess.get_md5(_save_path) == expect_md5


# ================================
# ZIP 解压
# ================================

func unzip(path: String, out_dir: String) -> void:
	var zip := ZIPReader.new()
	if zip.open(path) != OK:
		unzip_failed.emit("无法打开 ZIP 文件")
		return

	var files := zip.get_files()
	var total := files.size()
	var count := 0

	for f in files:
		if _canceled:
			break

		var data := zip.read_file(f)
		var full := out_dir + "/" + f
		var dir := full.get_base_dir()

		DirAccess.make_dir_recursive_absolute(dir)

		var out := FileAccess.open(full, FileAccess.WRITE)
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
