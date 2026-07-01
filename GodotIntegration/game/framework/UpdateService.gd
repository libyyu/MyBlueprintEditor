extends Node
## UpdateService —— 方案 A 热更服务（纯 GDScript，Autoload 单例，名为 UpdateService）
##
## 职责（对应 docs/06 的 ①~④）：
##   ① check_version()  拉远端 version.json（HTTPRequest）
##   ② diff             比对本地已装版本号
##   ③ download_pcks()  逐个下载补丁 pck 到 user://patches/
##   ④ verify + mount   sha256 校验 → load_resource_pack(replace=true) → 写本地版本号
##
## 弱网哲学：任何网络/校验失败都不阻断启动，回退用内置资源，返回 result 让 Boot 照常进主逻辑。
##
## 本地假 CDN：cdn_base_url 支持 res:// 或 user:// 或 http(s)://。res:///user:// 走 FileAccess，
## http(s):// 走 HTTPRequest —— 这样最小 Demo 无需真服务器即可端到端验证。

signal progress(p: float, status: String)   # p ∈ [0,1]

## 结果结构（Boot 据此决定进主逻辑用哪份资源）
class UpdateResult:
	var success: bool = false        # 是否成功走完（含"无需更新"）
	var updated: bool = false        # 是否真的挂载了新补丁
	var version: String = ""         # 最终生效版本号
	var reason: String = ""          # 结果原因（日志/调试）

const LOCAL_VER_PATH := "user://installed_version.txt"
const PATCH_DIR := "user://patches"

## CDN 根：默认指向仓库内置的“假 CDN”目录（res://），无需真服务器
var cdn_base_url: String = "res://fake_cdn"
var version_timeout := 5.0

var _http: HTTPRequest = null

func _ready() -> void:
	# HTTPRequest 只在需要走真 http 时才用；res://user:// 直接 FileAccess。
	_http = HTTPRequest.new()
	add_child(_http)
	DirAccess.make_dir_recursive_absolute(PATCH_DIR)

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
		res.success = true; res.reason = "no_manifest_fallback"
		_emit(1.0, "无法获取更新信息，使用内置资源")
		return res

	var remote_ver := str(manifest.get("version", ""))
	_emit(0.08, "远端版本 %s" % remote_ver)

	# ② diff
	var local_ver := _read_local_version()
	if remote_ver != "" and remote_ver == local_ver:
		# 版本相同：无需下载，但**必须重新挂载已下载的本地补丁**——
		# load_resource_pack 是运行时挂载，不持久化，每次启动都要重挂，
		# 否则 res:// 读到的仍是内置旧版。
		var remounted := _remount_local_packs(manifest)
		res.success = true; res.version = local_ver; res.reason = "up_to_date"
		res.updated = remounted
		_emit(1.0, "已是最新 %s" % local_ver)
		return res

	# ③ 下载 packs
	var packs: Array = manifest.get("packs", [])
	if packs.is_empty():
		res.success = true; res.reason = "no_packs"
		_emit(1.0, "无需下载")
		return res

	var downloaded_paths: Array[String] = []
	var n := packs.size()
	for i in range(n):
		var pk: Dictionary = packs[i]
		var pack_name: String = str(pk.get("name", "patch_%d.pck" % i))
		var url: String = str(pk.get("url", _join(cdn_base_url, "%s/%s" % [remote_ver, pack_name])))
		var base_p: float = 0.1 + 0.6 * (float(i) / n)
		_emit(base_p, "下载 %s (%d/%d)" % [pack_name, i + 1, n])

		var dst: String = "%s/%s" % [PATCH_DIR, pack_name]
		var ok: bool = await _download_to(url, dst)
		if not ok:
			# 下载失败 → 弱网回退（不阻断）
			res.success = true; res.reason = "download_failed_fallback"
			_emit(1.0, "下载失败，使用内置资源")
			return res

		# ④a 校验
		var want_hash: String = str(pk.get("sha256", ""))
		if want_hash != "":
			var got: String = FileAccess.get_sha256(dst)
			if got != want_hash:
				push_error("[UpdateService] sha256 mismatch %s: want=%s got=%s" % [name, want_hash, got])
				res.success = true; res.reason = "verify_failed_fallback"
				_emit(1.0, "校验失败，使用内置资源")
				return res
		downloaded_paths.append(dst)

	# ④b 挂载（必须在 end_update_phase 之前完成，主 VM 才能读到新文件）
	_emit(0.85, "挂载补丁...")
	for p in downloaded_paths:
		var mounted := ProjectSettings.load_resource_pack(p, true)  # replace=true 覆盖 res://
		if not mounted:
			push_error("[UpdateService] mount failed: " + p)
			res.success = true; res.reason = "mount_failed_fallback"
			_emit(1.0, "挂载失败，使用内置资源")
			return res

	# ④c 写本地版本号
	_write_local_version(remote_ver)
	res.success = true; res.updated = true; res.version = remote_ver; res.reason = "updated"
	_emit(1.0, "更新完成 %s" % remote_ver)
	return res

# ---------------------------------------------------------------------------
# 重新挂载已下载到 user://patches 的本地补丁（版本未变但需重挂时用）
# load_resource_pack 不持久 —— 每次进程启动都要重挂一次。
# ---------------------------------------------------------------------------
func _remount_local_packs(manifest: Dictionary) -> bool:
	var packs: Array = manifest.get("packs", [])
	var any := false
	for pk in packs:
		var pack_name: String = str(pk.get("name", ""))
		if pack_name == "": continue
		var dst: String = "%s/%s" % [PATCH_DIR, pack_name]
		if FileAccess.file_exists(dst):
			if ProjectSettings.load_resource_pack(dst, true):
				any = true
	return any

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
	progress.emit(p, status)
