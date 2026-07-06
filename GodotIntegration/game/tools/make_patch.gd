extends SceneTree
## 假 CDN 补丁打包工具（发版侧）。用法：
##   godot --headless --path <game_dir> --script res://tools/make_patch.gd
##
## 做三件事：
##   1) 用 PCKPacker 把 patch_src/** 下的文件打成一个补丁 pck，
##      pck 内路径映射为 res://...（挂载时 replace=true 覆盖原版）
##   2) 算该 pck 的 sha256
##   3) 生成 version.json（新版 entries[] 格式，见 docs/08）
##      每个 entry: { name, kind, url, sha256, size, order }
##      —— url 用 res:// 相对假 CDN，Demo 无需真服务器
##
## 产出目录：res://fake_cdn/  (打进主包，运行时 UpdateService 从这里“拉取”)

const PATCH_VERSION := "1.0.1"
const CDN_DIR := "res://fake_cdn"
const PACK_NAME := "game.pck"

## 补丁内容清单：{ pck内的res路径 : 源文件(patch_src下) }
const FILES := {
	"res://lua/demo/main_menu_logic.lua": "res://patch_src/lua/demo/main_menu_logic.lua",
}

## 与运行时兼容闸联动的最低版本要求（客户端 API Level / 语义版本）。
## planner 优先看 min_api_level；两者都写上更保险。
const MIN_API_LEVEL := 0             # 0 = 不设 API Level 门槛
const MIN_CLIENT_VERSION := ""       # "" = 不设语义版本门槛

func _init() -> void:
	var cdn_os := ProjectSettings.globalize_path(CDN_DIR)
	DirAccess.make_dir_recursive_absolute(cdn_os)
	var pack_os_dir := cdn_os.path_join(PATCH_VERSION)
	DirAccess.make_dir_recursive_absolute(pack_os_dir)
	var pack_os_path := pack_os_dir.path_join(PACK_NAME)

	# 1) 打 pck
	var packer := PCKPacker.new()
	var err := packer.pck_start(pack_os_path)
	if err != OK:
		push_error("[make_patch] pck_start failed: %d" % err); quit(1); return
	for res_path in FILES.keys():
		var src: String = FILES[res_path]
		if not FileAccess.file_exists(src):
			push_error("[make_patch] source missing: " + src); quit(1); return
		packer.add_file(res_path, ProjectSettings.globalize_path(src))
		print("[make_patch] + %s  <=  %s" % [res_path, src])
	err = packer.flush(true)
	if err != OK:
		push_error("[make_patch] flush failed: %d" % err); quit(1); return

	# 2) sha256 + size
	var sha := FileAccess.get_sha256(pack_os_path)
	var size := 0
	var f := FileAccess.open(pack_os_path, FileAccess.READ)
	if f: size = f.get_length(); f.close()

	# 3) version.json —— 新版 entries[] 格式（docs/08）
	var manifest := {
		"version": PATCH_VERSION,
		"min_engine": "4.5",
		"min_api_level": MIN_API_LEVEL,
		"min_client_version": MIN_CLIENT_VERSION,
		"store_urls": {},
		"entries": [
			{
				"name": PACK_NAME.get_basename(),   # 条目名（不带扩展）
				"kind": "pck",
				# 相对假 CDN 的 res:// 路径；真 CDN 换成 https://... 即可
				"url": "%s/%s/%s" % [CDN_DIR, PATCH_VERSION, PACK_NAME],
				"sha256": sha,
				"size": size,
				"order": 0,               # 越大越晚挂、优先级越高；本 Demo 单包，0 即可
			}
		]
	}
	var mf := FileAccess.open(CDN_DIR.path_join("version.json"), FileAccess.WRITE)
	mf.store_string(JSON.stringify(manifest, "  "))
	mf.close()

	print("[make_patch] pack:   %s (%d bytes)" % [pack_os_path, size])
	print("[make_patch] sha256: %s" % sha)
	print("[make_patch] manifest: %s/version.json" % CDN_DIR)
	print("[make_patch] DONE version=%s" % PATCH_VERSION)
	quit(0)
