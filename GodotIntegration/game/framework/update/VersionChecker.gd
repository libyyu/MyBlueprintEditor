extends Node
class_name VersionChecker

# ============ 数据结构 ============
class ResourceVersion:
	var resource_version: String      # 资源版本号，如 "r102"
	var min_client_version: String    # 最低客户端版本，如 "1.2.0"
	var min_api_level: int            # 最低API Level（数字比较更简单）
	var base_version: String          # 基线资源版本（用于增量更新）
	var file_count: int               # 更新文件数量
	var total_size: int               # 总大小（字节）
	
	func _init(data: Dictionary):
		resource_version = data.get("resource_version", "")
		min_client_version = data.get("min_client_version", "0.0.0")
		min_api_level = data.get("min_api_level", 0)
		base_version = data.get("base_version", "")
		file_count = data.get("file_count", 0)
		total_size = data.get("total_size", 0)

class VersionConfig:
	var latest_version: String                    # 最新资源版本
	var resource_versions: Dictionary             # 所有资源版本信息
	var store_urls: Dictionary                    # 各平台商店链接
	
	func _init(data: Dictionary):
		latest_version = data.get("latest_resource_version", "")
		resource_versions = {}
		for key in data.get("resource_versions", {}):
			resource_versions[key] = ResourceVersion.new(data.resource_versions[key])
		store_urls = data.get("store_urls", {})

# ============ 版本比较工具 ============
static func parse_version(version: String) -> Array:
	'''解析语义化版本号 "1.2.3" -> [1, 2, 3]'''
	var parts = version.split(".")
	var result = []
	for part in parts: result.append(part.to_int())
	return result

enum VersionCompareState {
	Less,
	Equal,
	Great
}

static func compare_version(v1: String, v2: String) -> VersionCompareState:
	'''比较两个语义化版本号 返回：-1 (v1 < v2), 0 (v1 == v2), 1 (v1 > v2)'''
	var parts1 = parse_version(v1)
	var parts2 = parse_version(v2)
	var max_len = max(parts1.size(), parts2.size())
	for i in range(max_len):
		var p1 = parts1[i] if i < parts1.size() else 0
		var p2 = parts2[i] if i < parts2.size() else 0
		if p1 < p2:
			return VersionCompareState.Less
		elif p1 > p2:
			return VersionCompareState.Great
	return VersionCompareState.Equal

static func is_client_compatible(client_version: String, min_version: String) -> bool:
	'''检查客户端是否兼容资源包 规则：主版本号必须相同，次版本号 >= 最低要求'''
	var v_client = parse_version(client_version)
	var v_min = parse_version(min_version)
	
	# 主版本号必须一致
	if v_client[0] != v_min[0]:
		return false
	
	# 次版本号必须 >=
	if v_client[1] < v_min[1]:
		return false
	
	# 补丁版本号必须 >=（仅当次版本相同时）
	if v_client[1] == v_min[1] and v_client[2] < v_min[2]:
		return false
	
	return true

# ============ 存储操作 ============
static func save_local_version(version: String) -> void:
	'''保存本地资源版本号'''
	var file = FileAccess.open(CommonDefine.LOCAL_VERSION_FILE, FileAccess.WRITE)
	if file:
		file.store_string(version)
		file.close()

static func load_local_version() -> String:
	'''读取本地资源版本号'''
	if not FileAccess.file_exists(CommonDefine.LOCAL_VERSION_FILE):
		return "r0"  # 初始版本
	
	var file = FileAccess.open(CommonDefine.LOCAL_VERSION_FILE, FileAccess.READ)
	if file:
		var version = file.get_as_text().strip_edges()
		file.close()
		return version
	return "r0"

static func ensure_directories() -> void:
	'''确保所有必要的目录存在'''
	var dirs = [
		CommonDefine.LOCAL_LUA_DIR,
		CommonDefine.LOCAL_TEMP_DIR,
		CommonDefine.LOCAL_BACKUP_DIR
	]
	for dir_path in dirs:
		var dir = DirAccess.open(dir_path)
		if dir:
			continue
		DirAccess.make_dir_recursive_absolute(dir_path)
