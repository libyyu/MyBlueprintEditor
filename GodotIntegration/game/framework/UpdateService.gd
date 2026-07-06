extends Node
## UpdateService —— Native 热更服务（Autoload 单例，名为 UpdateService）
##
## 职责（对应 docs/06 / docs/08）：
##   ① check_version()   拉远端 version.json
##   ② plan              交给 UpdatePlanner 做纯运算：兼容闸 + 版本 diff + 逐条 hash diff
##   ③ download          按 planner 输出的下载队列，只下变动条目
##   ④ verify + mount    sha256 校验 → load_resource_pack(replace=true, 按 order 升序)
##                       → 写本地版本号
##
## 弱网哲学：任何网络/校验失败都不阻断启动，回退用内置资源，返回 result 让 Boot 照常进主逻辑。
##
## 本地假 CDN：cdn_base_url 支持 res:// 或 user:// 或 http(s)://。res:///user:// 走 FileAccess，
## http(s):// 走 HTTPRequest —— 这样最小 Demo 无需真服务器即可端到端验证。
##
## 平台边界：本类使用 Godot pck 机制，仅适用于 Native（桌面/Android/iOS）与 Web（待 Web 编译修通）。
## 小游戏形态无 res:// 与 pck，需另实现 MiniGameUpdater（见 docs/08 §4）。

signal update_progress(p: float, status: String)     # p ∈ [0,1]
signal store_upgrade_required(store_url: String)     # 客户端过低，需整包升级

## 结果结构（Boot 据此决定进主逻辑用哪份资源）
class UpdateResult:
	var success: bool = false        # 是否成功走完（含"无需更新"）
	var updated: bool = false        # 是否真的挂载了新补丁
	var version: String = ""         # 最终生效版本号
	var reason: String = ""          # 结果原因（日志/调试）
	var need_store_upgrade: bool = false
	var store_url: String = ""

const LOCAL_VER_PATH := "user://installed_version.txt"
const PATCH_DIR := "user://patches"

## CDN 根：默认指向仓库内置的"假 CDN"目录（res://），无需真服务器
var cdn_base_url: String = "res://fake_cdn"
var version_timeout := 5.0

## 平台键（用于取 manifest.store_urls 里的地址）
var platform_key: String = "default"

var _http: HTTPRequest = null

func _ready() -> void:
	# HTTPRequest 只在需要走真 http 时才用；res://user:// 直接 FileAccess。
	_http = HTTPRequest.new()
	add_child(_http)
	DirAccess.make_dir_recursive_absolute(PATCH_DIR)
	platform_key = _detect_platform_key()

# ---------------------------------------------------------------------------
# 对外主入口：跑完整更新流程，返回 UpdateResult（协程，Boot 里 await）
# ---------------------------------------------------------------------------
func run_update() -> UpdateResult:
	var res := UpdateResult.new()
	res.version = _read_local_version()

	_emit(0.02, "正在检查版本...")

	# ① 拉 version.json
	var manifest: Variant = await _fetch_json(_join(cdn_base_url, "version.json"), version_timeout)
	if manifest == null or typeof(manifest) != TYPE_DICTIONARY:
		res.success = true
		res.reason = "no_manifest_fallback"
		_emit(1.0, "无法获取更新信息，使用内置资源")
		return res

	# ② 交给 planner 做纯决策
	var plan := UpdatePlanner.plan(
		manifest,
		_read_local_version(),
		CommonDefine.CLIENT_API_LEVEL,
		CommonDefine.CLIENT_VERSION,
		platform_key,
		func(name: String, _kind: String) -> String:
			# 本地已下载补丁的 hash 查询：从 user://patches/<name>.pck 读
			var path := _entry_local_path(name)
			if not FileAccess.file_exists(path):
				return ""
			return FileAccess.get_sha256(path)
	)

	if not plan.ok:
		res.success = true
		res.reason = "invalid_manifest_fallback"
		_emit(1.0, "更新信息无效，使用内置资源")
		return res

	_emit(0.08, "远端版本 %s" % plan.remote_version)

	# ---- 闸一：需整包升级 ----
	if plan.need_store_upgrade:
		res.success = false
		res.need_store_upgrade = true
		res.store_url = plan.store_url
		res.reason = "need_store_upgrade"
		_emit(1.0, "客户端版本过低，请前往应用商店更新")
		store_upgrade_required.emit(plan.store_url)
		return res

	# ---- 闸二：版本未变 → 只需重挂已下载的本地包 ----
	if plan.up_to_date:
		var remounted := _mount_entries(plan.all_entries, true)
		res.success = true
		res.version = plan.remote_version if plan.remote_version != "" else res.version
		res.reason = "up_to_date"
		res.updated = remounted
		_emit(1.0, "已是最新 %s" % res.version)
		return res

	# ---- ③ 下载：只下 to_download，跳过的条目复用本地 ----
	if plan.to_download.is_empty() and plan.skipped.is_empty():
		res.success = true
		res.reason = "no_entries"
		_emit(1.0, "无需下载")
		return res

	var total := plan.to_download.size()
	for i in range(total):
		var e: UpdatePlanner.Entry = plan.to_download[i]
		var base_p: float = 0.1 + 0.6 * (float(i) / max(1, total))
		_emit(base_p, "下载 %s (%d/%d)" % [e.name, i + 1, total])

		var dst := _entry_local_path(e.name)
		var ok: bool = await _download_to(e.url, dst)
		if not ok:
			res.success = true
			res.reason = "download_failed_fallback"
			_emit(1.0, "下载失败，使用内置资源")
			return res

		# ④a 校验
		if e.sha256 != "":
			var got := FileAccess.get_sha256(dst)
			if got != e.sha256:
				push_error("[UpdateService] sha256 mismatch %s: want=%s got=%s" % [e.name, e.sha256, got])
				res.success = true
				res.reason = "verify_failed_fallback"
				_emit(1.0, "校验失败，使用内置资源")
				return res

	# ---- ④b 挂载（按 order 升序，最新补丁最后挂）----
	_emit(0.85, "挂载补丁...")
	if not _mount_entries(plan.all_entries, false):
		res.success = true
		res.reason = "mount_failed_fallback"
		_emit(1.0, "挂载失败，使用内置资源")
		return res

	# ---- ④c 写本地版本号 ----
	_write_local_version(plan.remote_version)
	res.success = true
	res.updated = true
	res.version = plan.remote_version
	res.reason = "updated"
	_emit(1.0, "更新完成 %s" % plan.remote_version)
	return res

# ---------------------------------------------------------------------------
# 挂载条目列表（按 order 升序遍历，只挂 kind=="pck" 且文件存在的）
#   tolerant=true  → 允许中间挂载失败，只要有一个成功就返回 true（up_to_date 场景）
#   tolerant=false → 任一失败即视为整体失败（首次更新场景，严格）
# ---------------------------------------------------------------------------
func _mount_entries(entries: Array, tolerant: bool) -> bool:
	var any_mounted := false
	for e in entries:
		var entry: UpdatePlanner.Entry = e
		if entry.kind != "pck":
			continue
		var path := _entry_local_path(entry.name)
		if not FileAccess.file_exists(path):
			if tolerant:
				continue
			push_error("[UpdateService] mount missing local file: " + path)
			return false
		var mounted := ProjectSettings.load_resource_pack(path, true)
		if not mounted:
			if tolerant:
				continue
			push_error("[UpdateService] mount failed: " + path)
			return false
		any_mounted = true
	return any_mounted or tolerant

func _entry_local_path(entry_name: String) -> String:
	# 目前所有条目都当 .pck 存；后续支持 bjson 等裸资源时按 kind 分目录
	return "%s/%s.pck" % [PATCH_DIR, entry_name]

# ---------------------------------------------------------------------------
# 版本号持久化
# ---------------------------------------------------------------------------
func _read_local_version() -> String:
	if not FileAccess.file_exists(LOCAL_VER_PATH):
		return "0.0.0"
	var f := FileAccess.open(LOCAL_VER_PATH, FileAccess.READ)
	if f == null: return "0.0.0"
	var v := f.get_as_text().strip_edges()
	f.close()
	return v if v != "" else "0.0.0"

func _write_local_version(v: String) -> void:
	var f := FileAccess.open(LOCAL_VER_PATH, FileAccess.WRITE)
	if f:
		f.store_string(v)
		f.close()

# ---------------------------------------------------------------------------
# 传输层：res:// / user:// 走 FileAccess；http(s):// 走 HTTPRequest
# ---------------------------------------------------------------------------
func _is_local(url: String) -> bool:
	return url.begins_with("res://") or url.begins_with("user://")

func _fetch_json(url: String, _timeout: float) -> Variant:
	var bytes := await _fetch_bytes(url)
	if bytes.is_empty(): return null
	var txt := bytes.get_string_from_utf8()
	var parsed = JSON.parse_string(txt)
	return parsed  # Dictionary 或 null

func _fetch_bytes(url: String) -> PackedByteArray:
	if _is_local(url):
		if not FileAccess.file_exists(url):
			return PackedByteArray()
		var f := FileAccess.open(url, FileAccess.READ)
		if f == null: return PackedByteArray()
		var b := f.get_buffer(f.get_length())
		f.close()
		return b
	# http(s)://
	var err := _http.request(url)
	if err != OK:
		return PackedByteArray()
	var result: Array = await _http.request_completed
	# result = [result_code, response_code, headers, body]
	var response_code: int = result[1]
	if response_code != 200:
		return PackedByteArray()
	return result[3]

func _download_to(url: String, dst: String) -> bool:
	var bytes := await _fetch_bytes(url)
	if bytes.is_empty():
		return false
	var f := FileAccess.open(dst, FileAccess.WRITE)
	if f == null:
		return false
	f.store_buffer(bytes)
	f.close()
	return true

# ---------------------------------------------------------------------------
# 工具
# ---------------------------------------------------------------------------
func _join(base: String, rel: String) -> String:
	if base.ends_with("/"):
		return base + rel
	return base + "/" + rel

func _emit(p: float, status: String) -> void:
	update_progress.emit(p, status)

func _detect_platform_key() -> String:
	match OS.get_name():
		"Android": return "android"
		"iOS": return "ios"
		"Web": return "web"
		_: return "default"
