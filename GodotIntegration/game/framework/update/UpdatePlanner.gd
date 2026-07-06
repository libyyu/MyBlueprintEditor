extends RefCounted
## UpdatePlanner —— 平台无关的热更决策层。
##
## 只做「要不要更新、更新什么」的纯运算，不碰网络、不碰文件系统、不碰平台 API。
## 三端（Native / Web / 小游戏）共用同一份决策逻辑，各自的下载/落地/挂载由对应 Updater 实现。
##
## 输入：远端 manifest(Dictionary) + 本地状态（版本号 + 一个「本地条目 hash 查询」回调）
## 输出：UpdatePlan —— 是否需整包升级、要下载哪些条目、跳过哪些。
##
## 依赖注入：本类不直接读文件，调用方传入 local_hash_getter: Callable(name, kind) -> String
##          （本地不存在或无法计算时返回 ""）。这样 planner 在任何平台都能跑，可单测。
class_name UpdatePlanner

## 单个待处理条目（来自 manifest.entries[] 或旧版 packs[]）
class Entry:
	var name: String = ""
	var kind: String = "pck"        # "pck" | "bjson" | 其它裸资源类型
	var url: String = ""
	var sha256: String = ""
	var size: int = 0
	var order: int = 0              # 挂载/覆盖优先级，越大越晚挂、优先级越高

	static func from_dict(d: Dictionary, index: int) -> Entry:
		var e := Entry.new()
		e.name = str(d.get("name", "entry_%d" % index))
		e.kind = str(d.get("kind", "pck"))
		e.url = str(d.get("url", ""))
		e.sha256 = str(d.get("sha256", ""))
		e.size = int(d.get("size", 0))
		# order 缺省时用数组下标兜底，保证「书写顺序即优先级」的旧约定仍成立
		e.order = int(d.get("order", index))
		return e

## 决策结果
class UpdatePlan:
	var ok: bool = false                    # manifest 是否有效可用
	var need_store_upgrade: bool = false    # 客户端过低，需整包升级（热更止步）
	var store_url: String = ""              # 整包升级引导地址（按平台取）
	var up_to_date: bool = false            # 资源版本未变
	var remote_version: String = ""
	var to_download: Array[Entry] = []      # 需下载的条目（已按 order 升序）
	var skipped: Array[Entry] = []          # 本地已有且 hash 一致、跳过的条目
	var all_entries: Array[Entry] = []      # 全部条目（按 order 升序，供 up_to_date 时重挂用）
	var reason: String = ""                 # 结果原因（日志）

# ---------------------------------------------------------------------------
# 主入口
#   manifest          : 远端拉到的 version.json（Dictionary）
#   local_version     : 本地已装资源版本号
#   client_api_level  : 当前客户端 API Level（CommonDefine.CLIENT_API_LEVEL）
#   client_version    : 当前客户端语义版本（CommonDefine.CLIENT_VERSION）
#   platform_key      : "android"/"ios"/"web"/"default"，用于取 store_url
#   local_hash_getter : Callable(name: String, kind: String) -> String
#                       返回本地该条目的 sha256（不存在/算不出返回 ""）
# ---------------------------------------------------------------------------
static func plan(
	manifest: Dictionary,
	local_version: String,
	client_api_level: int,
	client_version: String,
	platform_key: String,
	local_hash_getter: Callable
) -> UpdatePlan:
	var p := UpdatePlan.new()

	if manifest == null or manifest.is_empty():
		p.ok = false
		p.reason = "empty_manifest"
		return p
	p.ok = true

	p.remote_version = str(manifest.get("version", ""))

	# ---- 闸一：程序包兼容 ----
	# 优先用数字 API Level 比较；缺省时降级到语义版本比较。
	var min_api := int(manifest.get("min_api_level", 0))
	var compatible := true
	if min_api > 0:
		compatible = client_api_level >= min_api
	else:
		var min_client := str(manifest.get("min_client_version", ""))
		if min_client != "":
			compatible = _semver_ge(client_version, min_client)

	if not compatible:
		p.need_store_upgrade = true
		var store_urls: Dictionary = manifest.get("store_urls", {})
		p.store_url = str(store_urls.get(platform_key, store_urls.get("default", "")))
		p.reason = "need_store_upgrade"
		return p

	# ---- 解析条目：仅认新版 entries[] 格式 ----
	var raw_entries: Array = manifest.get("entries", [])
	var entries: Array[Entry] = []
	for i in range(raw_entries.size()):
		if typeof(raw_entries[i]) == TYPE_DICTIONARY:
			entries.append(Entry.from_dict(raw_entries[i], i))

	# 按 order 升序排序 —— 挂载优先级的唯一真相来源
	entries.sort_custom(func(a: Entry, b: Entry) -> bool: return a.order < b.order)
	p.all_entries = entries

	# ---- 闸二：资源版本未变 ----
	if p.remote_version != "" and p.remote_version == local_version:
		p.up_to_date = true
		p.reason = "up_to_date"
		return p

	if entries.is_empty():
		p.reason = "no_entries"
		return p

	# ---- 逐条目 hash diff：本地已有且一致 → 跳过；否则加入下载队列 ----
	for e in entries:
		if e.sha256 != "":
			var local_hash: String = local_hash_getter.call(e.name, e.kind)
			if local_hash != "" and local_hash == e.sha256:
				p.skipped.append(e)
				continue
		p.to_download.append(e)

	p.reason = "planned"
	return p


# ---------------------------------------------------------------------------
# 语义版本比较：v1 >= v2 ？（"1.2.0" 形式，缺位补 0）
# ---------------------------------------------------------------------------
static func _semver_ge(v1: String, v2: String) -> bool:
	var a := v1.split(".")
	var b := v2.split(".")
	var n: int = max(a.size(), b.size())
	for i in range(n):
		var pa := int(a[i]) if i < a.size() else 0
		var pb := int(b[i]) if i < b.size() else 0
		if pa > pb:
			return true
		if pa < pb:
			return false
	return true   # 相等
