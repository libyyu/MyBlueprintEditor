extends Node
class_name UpdateManager

signal update_progress(progress: float, status: String)
signal update_completed(success: bool, error: String)
signal store_redirect_required(url: String)

enum UpdateState {
	IDLE,
	CHECKING_VERSION,
	FETCHING_CONFIG,
	COMPATIBILITY_CHECK,
	DOWNLOADING_MANIFEST,
	PROCESS_MANIFEST,
	DOWNLOADING_PATCH,
	EXTRACTING,
	RELOADING,
	COMPLETED,
	ERROR
}

var current_state: UpdateState = UpdateState.IDLE
var version_checker: VersionChecker
var downloader: Downloader

# 缓存数据
var latest_version_config: VersionChecker.VersionConfig
var target_resource_version: String = ""
var local_client_version: String = ""
var local_resource_version: String = ""
var current_update_progress: float = 0.0

func _ready():
	version_checker = VersionChecker.new()
	add_child(version_checker)
	
	downloader = Downloader.new()
	add_child(downloader)
	downloader.download_finished.connect(_on_download_complete)
	downloader.download_progress.connect(_on_download_progress)
	
	# 确保目录存在
	VersionChecker.ensure_directories()
	
	# 读取本地版本
	local_client_version = CommonDefine.CLIENT_VERSION
	local_resource_version = VersionChecker.load_local_version()
	current_update_progress = 0.0

# ============ 对外接口 ============
func start_update_flow() -> void:
	'''启动完整的更新流程'''
	print("=== 开始热更新流程 ===")
	# 读取本地版本
	local_client_version = CommonDefine.CLIENT_VERSION
	local_resource_version = VersionChecker.load_local_version()
	current_update_progress = 0.0
	print("客户端版本: " + local_client_version)
	print("本地资源版本: " + local_resource_version)

	current_state = UpdateState.CHECKING_VERSION
	_fetch_version_config()
	
func check_update_silent() -> void:
	'''静默检查更新（后台定时任务）'''
	if current_state != UpdateState.IDLE:
		return
	
	start_update_flow()
	
# ============ 核心流程 ============
func _fetch_version_config() -> void:
	'''步骤1：获取服务端版本配置'''
	current_state = UpdateState.FETCHING_CONFIG
	_emit_progress(0.0, "正在检查更新...")
	
	var http = HTTPRequest.new()
	add_child(http)
	http.request_completed.connect(_on_config_received.bind(http))
	
	var error = http.request(CommonDefine.VERSION_CONFIG_URL)
	if error != OK:
		_handle_error("无法连接服务器")
		http.queue_free()
		
func _on_config_received(
	result: int, 
	response_code: int, 
	headers: PackedStringArray, 
	body: PackedByteArray, 
	http: HTTPRequest):
	http.queue_free()
	
	if result != HTTPRequest.RESULT_SUCCESS or response_code != 200:
		_handle_error("获取版本配置失败")
		return
	
	var json = JSON.new()
	var parse_error = json.parse(body.get_string_from_utf8())
	if parse_error != OK:
		_handle_error("版本配置解析失败")
		return
	
	var data = json.data
	if typeof(data) != TYPE_DICTIONARY:
		_handle_error("版本配置格式错误")
		return
	
	latest_version_config = VersionChecker.VersionConfig.new(data)
	target_resource_version = latest_version_config.latest_version
	
	print("最新资源版本: " + target_resource_version)
	
	_check_compatibility()
	
func _check_compatibility() -> void:
	'''步骤2：检查客户端兼容性'''
	current_state = UpdateState.COMPATIBILITY_CHECK
	_emit_progress(0.1, "检查兼容性...")
	
	var resource_info = latest_version_config.resource_versions.get(target_resource_version)
	if not resource_info:
		_handle_error("资源版本信息不存在")
		return
	
	# 检查API Level（优先使用数字比较）
	if resource_info.min_api_level > 0:
		if CommonDefine.CLIENT_API_LEVEL < resource_info.min_api_level:
			_redirect_to_store()
			return
	else:
		# 降级使用语义化版本比较
		if not VersionChecker.is_client_compatible(local_client_version, resource_info.min_client_version):
			_redirect_to_store()
			return
	
	# 检查是否需要更新
	if target_resource_version == local_resource_version:
		current_state = UpdateState.COMPLETED
		_emit_progress(1.0, "已是最新版本")
		update_completed.emit(true, "")
		return
	
	# 开始下载
	_download_manifest()
	
func _redirect_to_store() -> void:
	'''跳转到应用商店'''
	var platform = CommonDefine.get_current_platform()
	var store_url = ""
	
	match platform:
		CommonDefine.Platform.ANDROID:
			store_url = latest_version_config.store_urls.get("android", "")
		CommonDefine.Platform.IOS:
			store_url = latest_version_config.store_urls.get("ios", "")
		CommonDefine.Platform.WEB:
			store_url = latest_version_config.store_urls.get("web", "")
		_:
			store_url = latest_version_config.store_urls.get("default", "")
	
	if store_url.is_empty():
		store_url = "https://your-game.com/download"
	
	_emit_progress(0.0, "客户端版本过低，请前往应用商店更新")
	store_redirect_required.emit(store_url)
	update_completed.emit(false, "需要更新客户端")
	
func _get_manifest_local_path() -> String:
	var manifest_path = CommonDefine.LOCAL_TEMP_DIR + "manifest_" + target_resource_version + ".json"
	return manifest_path
	
func _get_manifest_remote_path() -> String:
	var manifest_url = CommonDefine.MANIFEST_URL.replace("{version}", target_resource_version)
	return manifest_url
	
func _get_patch_remote_path(version: String) -> String:
	var patch_url = CommonDefine.PATCH_URL.replace("{version}", version)
	return patch_url


func _download_manifest() -> void:
	'''步骤3：下载资源清单'''
	current_state = UpdateState.DOWNLOADING_MANIFEST
	_emit_progress(0.2, "下载资源清单...")
	
	var manifest_url = _get_manifest_remote_path()
	var manifest_path = _get_manifest_local_path()
	
	downloader.download_async(manifest_url, manifest_path, "", CommonDefine.MAX_RETRY_COUNT, 10)
	
func _on_download_complete(result: Downloader.DownloadResult, error: String) -> void:
	if result != Downloader.DownloadResult.OK:
		_handle_error("下载失败: " + error)
		return
	match current_state:
		UpdateState.DOWNLOADING_MANIFEST:
			_process_manifest()
		UpdateState.DOWNLOADING_PATCH:
			_apply_patch()
			
func _on_download_progress(downloaded: int, total: int, speed: String) -> void:
	match current_state:
		UpdateState.DOWNLOADING_MANIFEST:
			return
		UpdateState.DOWNLOADING_PATCH:
			print("[UpdateManager]", downloaded, total, speed)
			
func _process_manifest() -> void:
	'''步骤4：处理清单并下载增量包'''
	current_state = UpdateState.PROCESS_MANIFEST
	
	var manifest_path = _get_manifest_local_path()
	
	var file = FileAccess.open(manifest_path, FileAccess.READ)
	if not file:
		_handle_error("无法读取清单文件")
		return
	
	var json = JSON.new()
	var parse_error = json.parse(file.get_as_text())
	file.close()
	
	if parse_error != OK:
		_handle_error("清单解析失败")
		return
	
	var manifest_data = json.data
	
	# 验证签名（重要！）
	if not _verify_signature(manifest_data):
		_handle_error("签名验证失败")
		return
	
	# 开始下载增量包
	current_state = UpdateState.DOWNLOADING_PATCH
	var patch_url = _get_patch_remote_path(target_resource_version)
	
	_emit_progress(0.3, "下载更新包...")
	downloader.download_and_extract_zip(patch_url, CommonDefine.LOCAL_LUA_DIR)
	
func _verify_signature(data: Dictionary) -> bool:
	'''验证ED25519签名（示例实现）'''
	# 实际项目中应使用 Crypto 类验证
	# 这里简化为检查是否有signature字段
	if not data.has("signature"):
		return false
	
	# TODO: 实现ED25519签名验证
	# 可以使用 Godot 的 Crypto 类
	return true
	
func _apply_patch() -> void:
	'''步骤5：应用更新包'''
	current_state = UpdateState.EXTRACTING
	_emit_progress(0.7, "应用更新...")
	
	# 备份旧版本（用于回滚）
	_backup_old_version()
	
	# 保存新版本号
	VersionChecker.save_local_version(target_resource_version)
	local_resource_version = target_resource_version
	
	_reload_ifneed()
	
func _reload_ifneed() -> void:
	current_state = UpdateState.RELOADING
	_emit_progress(0.9, "热重载游戏逻辑...")
	
	_on_reload_completed(true)
	
func _backup_old_version() -> void:
	'''备份旧版本（回滚用）'''
	var backup_dir = CommonDefine.LOCAL_BACKUP_DIR + local_resource_version + "/"
	var lua_dir = CommonDefine.LOCAL_LUA_DIR
	
	if DirAccess.dir_exists_absolute(lua_dir):
		# 复制Lua目录到备份
		var dir = DirAccess.open(lua_dir)
		if dir:
			_copy_directory(lua_dir, backup_dir)

func _copy_directory(src: String, dst: String) -> void:
	'''递归复制目录'''
	var dir = DirAccess.open(src)
	if not dir:
		return
	
	DirAccess.make_dir_recursive_absolute(dst)
	
	dir.list_dir_begin()
	var file_name = dir.get_next()
	while file_name != "":
		if file_name == "." or file_name == "..":
			file_name = dir.get_next()
			continue
		
		var src_path = src + "/" + file_name
		var dst_path = dst + "/" + file_name
		
		if dir.current_is_dir():
			_copy_directory(src_path, dst_path)
		else:
			var file = FileAccess.open(src_path, FileAccess.READ)
			if file:
				var data = file.get_buffer(file.get_length())
				file.close()
				
				var dst_file = FileAccess.open(dst_path, FileAccess.WRITE)
				if dst_file:
					dst_file.store_buffer(data)
					dst_file.close()
		
		file_name = dir.get_next()
	
	dir.list_dir_end()
	
func _on_reload_completed(success: bool) -> void:
	if success:
		current_state = UpdateState.COMPLETED
		_emit_progress(1.0, "更新完成！")
		update_completed.emit(true, "")
	else:
		_handle_error("Lua热重载失败")
		_rollback()

func _rollback() -> void:
	'''回滚到旧版本'''
	print("执行回滚...")
	# 恢复版本号
	VersionChecker.save_local_version(local_resource_version)
	
	update_completed.emit(false, "更新失败，已回滚")
	
func _handle_error(error: String) -> void:
	'''处理错误'''
	current_state = UpdateState.ERROR
	print("更新错误: " + error)
	update_progress.emit(current_update_progress, "错误: " + error)
	update_completed.emit(false, error)
	
func _emit_progress(progress: float, reason: String) -> void:
	current_update_progress = progress
	update_progress.emit(progress, reason)
