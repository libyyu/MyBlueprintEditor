# settings/game_settings.gd
extends Node
class_name CommonDefine
# ============ 版本信息 ============
# 客户端版本（语义化版本，与应用商店一致）
const CLIENT_VERSION = "1.2.0"
# 客户端API Level（用于简化版本比较）
const CLIENT_API_LEVEL = 102

# ============ 服务器地址 ============
const SERVER_BASE_URL = "https://your-cdn.com/game"
const VERSION_CONFIG_URL = SERVER_BASE_URL + "/version_config.json"
const MANIFEST_URL = SERVER_BASE_URL + "/manifests/manifest_{version}.json"
const PATCH_URL = SERVER_BASE_URL + "/patches/patch_{version}.zip"

# ============ 本地存储路径 ============
const LOCAL_VERSION_FILE = "user://version.txt"
const LOCAL_LUA_DIR = "user://lua/"
const LOCAL_TEMP_DIR = "user://temp/"
const LOCAL_BACKUP_DIR = "user://backup/"

# ============ 更新策略 ============
const TIMEOUT_SECONDS = 30          # 下载超时时间
const MAX_RETRY_COUNT = 3           # 最大重试次数
const CHECK_INTERVAL = 300          # 后台检查间隔（秒），0表示不启用
const FORCE_UPDATE = false          # 是否强制更新（跳过UI选择）

# ============ 平台标识 ============
enum Platform {
	ANDROID,
	IOS,
	WINDOWS,
	MACOS,
	LINUX,
	WEB,
	MINIPROGRAM
}

static func get_current_platform() -> Platform:
	var os_name = OS.get_name()
	match os_name:
		"Android":
			return Platform.ANDROID
		"iOS":
			return Platform.IOS
		"Windows":
			return Platform.WINDOWS
		"macOS":
			return Platform.MACOS
		"Linux":
			return Platform.LINUX
		"Web":
			return Platform.WEB
		_:
			return Platform.WEB  # 小程序降级为Web处理
