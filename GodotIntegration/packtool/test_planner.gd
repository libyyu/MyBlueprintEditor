extends SceneTree
## UpdatePlanner 独立单测（在 packtool 纯净空项目跑，避开 game 的 GDExtension 崩溃）。
## 用法：
##   godot --headless --path <GodotIntegration/packtool> --script res://test_planner.gd
##
## 本文件把 game/framework/update/UpdatePlanner.gd 的核心逻辑内联进来做纯逻辑验证，
## 保证与 game 侧实现一致（改 planner 时两边同步）。

var _pass := 0
var _fail := 0

# ============================================================
# --- 被测逻辑（内联复制自 UpdatePlanner，保持同步）---
# ============================================================
class Entry:
	var name: String = ""
	var kind: String = "pck"
	var url: String = ""
	var sha256: String = ""
	var size: int = 0
	var order: int = 0
	static func from_dict(d: Dictionary, index: int) -> Entry:
		var e := Entry.new()
		e.name = str(d.get("name", "entry_%d" % index))
		e.kind = str(d.get("kind", "pck"))
		e.url = str(d.get("url", ""))
		e.sha256 = str(d.get("sha256", ""))
		e.size = int(d.get("size", 0))
		e.order = int(d.get("order", index))
		return e

class UpdatePlan:
	var ok: bool = false
	var need_store_upgrade: bool = false
	var store_url: String = ""
	var up_to_date: bool = false
	var remote_version: String = ""
	var to_download: Array[Entry] = []
	var skipped: Array[Entry] = []
	var all_entries: Array[Entry] = []
	var reason: String = ""

static func plan(
	manifest: Dictionary, local_version: String, client_api_level: int,
	client_version: String, platform_key: String, local_hash_getter: Callable
) -> UpdatePlan:
	var p := UpdatePlan.new()
	if manifest == null or manifest.is_empty():
		p.ok = false; p.reason = "empty_manifest"; return p
	p.ok = true
	p.remote_version = str(manifest.get("version", ""))
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
		p.reason = "need_store_upgrade"; return p
	var raw_entries: Array = manifest.get("entries", [])
	var entries: Array[Entry] = []
	for i in range(raw_entries.size()):
		if typeof(raw_entries[i]) == TYPE_DICTIONARY:
			entries.append(Entry.from_dict(raw_entries[i], i))
	entries.sort_custom(func(a: Entry, b: Entry) -> bool: return a.order < b.order)
	p.all_entries = entries
	if p.remote_version != "" and p.remote_version == local_version:
		p.up_to_date = true; p.reason = "up_to_date"; return p
	if entries.is_empty():
		p.reason = "no_entries"; return p
	for e in entries:
		if e.sha256 != "":
			var local_hash: String = local_hash_getter.call(e.name, e.kind)
			if local_hash != "" and local_hash == e.sha256:
				p.skipped.append(e); continue
		p.to_download.append(e)
	p.reason = "planned"; return p

static func _semver_ge(v1: String, v2: String) -> bool:
	var a := v1.split(".")
	var b := v2.split(".")
	var n: int = max(a.size(), b.size())
	for i in range(n):
		var pa := int(a[i]) if i < a.size() else 0
		var pb := int(b[i]) if i < b.size() else 0
		if pa > pb: return true
		if pa < pb: return false
	return true

# ============================================================
# --- 测试 ---
# ============================================================
func _init() -> void:
	print("========== UpdatePlanner tests ==========")
	_t1_empty_manifest()
	_t2_min_api_gate()
	_t3_min_semver_gate()
	_t4_up_to_date()
	_t5_hash_diff()
	_t6_order_sort()
	print("========== %d passed / %d failed ==========" % [_pass, _fail])
	quit(1 if _fail > 0 else 0)

func _assert(cond: bool, label: String) -> void:
	if cond:
		_pass += 1; print("  [OK] " + label)
	else:
		_fail += 1; printerr("  [FAIL] " + label)

func _t1_empty_manifest() -> void:
	print("-- t1 empty_manifest")
	var p := plan({}, "1.0.0", 100, "1.0.0", "default", _hash_none)
	_assert(not p.ok, "empty -> ok=false")
	_assert(p.reason == "empty_manifest", "reason=empty_manifest")

func _t2_min_api_gate() -> void:
	print("-- t2 min_api_gate")
	var m := { "version": "2.0.0", "min_api_level": 200,
		"store_urls": { "default": "https://store/x" }, "entries": [] }
	var p := plan(m, "1.0.0", 100, "1.0.0", "default", _hash_none)
	_assert(p.need_store_upgrade, "api 100 < 200 -> need_store_upgrade")
	_assert(p.store_url == "https://store/x", "store_url picked")

func _t3_min_semver_gate() -> void:
	print("-- t3 min_semver_gate")
	var m := { "version": "2.0.0", "min_client_version": "1.5.0", "entries": [] }
	var p := plan(m, "1.0.0", 0, "1.2.0", "default", _hash_none)
	_assert(p.need_store_upgrade, "1.2.0 < 1.5.0 -> need_store_upgrade")
	var p2 := plan(m, "1.0.0", 0, "1.5.0", "default", _hash_none)
	_assert(not p2.need_store_upgrade, "1.5.0 == 1.5.0 -> pass")

func _t4_up_to_date() -> void:
	print("-- t4 up_to_date")
	var m := { "version": "1.0.1", "entries": [
		{ "name": "base", "kind": "pck", "url": "u1", "sha256": "aaa", "order": 0 } ] }
	var p := plan(m, "1.0.1", 100, "1.0.0", "default", _hash_none)
	_assert(p.up_to_date, "remote == local -> up_to_date")
	_assert(p.all_entries.size() == 1, "all_entries populated for remount")

func _t5_hash_diff() -> void:
	print("-- t5 hash_diff")
	var m := { "version": "1.0.2", "entries": [
		{ "name": "base", "kind": "pck", "url": "u1", "sha256": "hash_a", "order": 0 },
		{ "name": "ui",   "kind": "pck", "url": "u2", "sha256": "hash_b", "order": 1 },
		{ "name": "lua",  "kind": "pck", "url": "u3", "sha256": "hash_c", "order": 2 } ] }
	var getter := func(name: String, _kind: String) -> String:
		match name:
			"base": return "hash_a"
			"ui":   return "hash_stale"
			_:      return ""
	var p := plan(m, "1.0.1", 100, "1.0.0", "default", getter)
	_assert(p.to_download.size() == 2, "to_download = 2 (ui + lua)")
	_assert(p.skipped.size() == 1, "skipped = 1 (base)")
	_assert(p.skipped[0].name == "base", "base skipped")
	var names := []
	for e in p.to_download: names.append(e.name)
	_assert("ui" in names and "lua" in names, "to_download names correct")

func _t6_order_sort() -> void:
	print("-- t6 order_sort")
	var m := { "version": "1.0.3", "entries": [
		{ "name": "c", "kind": "pck", "url": "u", "order": 5 },
		{ "name": "a", "kind": "pck", "url": "u", "order": 1 },
		{ "name": "b", "kind": "pck", "url": "u", "order": 3 } ] }
	var p := plan(m, "1.0.0", 100, "1.0.0", "default", _hash_none)
	var order_names := []
	for e in p.all_entries: order_names.append(e.name)
	_assert(order_names == ["a", "b", "c"], "sorted by order asc: %s" % [order_names])

func _hash_none(_n: String, _k: String) -> String:
	return ""
