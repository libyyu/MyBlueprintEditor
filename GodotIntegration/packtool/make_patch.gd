extends SceneTree
## 独立补丁打包工具（不加载 game 工程的 GDExtension，避免其在 --script 模式下崩溃）。
## 用法：
##   godot --headless --path <GodotIntegration/packtool> --script res://make_patch.gd
##
## 所有路径用【绝对 OS 路径】，因为本项目是纯净空项目，不含 game 的 res:// 资源。
## 打出的 pck 内路径仍映射为 res://...（挂载到 game 时 replace=true 覆盖原版）。

const PATCH_VERSION := "1.0.1"
const PACK_NAME := "game.pck"

# game 工程根（OS 绝对路径）
const GAME_DIR := "C:/Users/maxweili/MyProjects/MyBlueprintEditor/GodotIntegration/game"

# 补丁内容清单：{ pck内的res路径 : game 工程内源文件相对路径(相对 GAME_DIR) }
const FILES := {
	"res://lua/demo/main_menu_logic.lua": "patch_src/lua/demo/main_menu_logic.lua",
}

func _init() -> void:
	var cdn_dir := GAME_DIR.path_join("fake_cdn")
	var pack_dir := cdn_dir.path_join(PATCH_VERSION)
	DirAccess.make_dir_recursive_absolute(pack_dir)
	var pack_path := pack_dir.path_join(PACK_NAME)

	# 1) 打 pck
	var packer := PCKPacker.new()
	var err := packer.pck_start(pack_path)
	if err != OK:
		push_error("[make_patch] pck_start failed: %d" % err); quit(1); return
	for res_path in FILES.keys():
		var src: String = GAME_DIR.path_join(FILES[res_path])
		if not FileAccess.file_exists(src):
			push_error("[make_patch] source missing: " + src); quit(1); return
		packer.add_file(res_path, src)
		print("[make_patch] + %s  <=  %s" % [res_path, src])
	err = packer.flush(true)
	if err != OK:
		push_error("[make_patch] flush failed: %d" % err); quit(1); return

	# 2) sha256 + size
	var sha := FileAccess.get_sha256(pack_path)
	var size := 0
	var f := FileAccess.open(pack_path, FileAccess.READ)
	if f: size = f.get_length(); f.close()

	# 3) version.json
	var manifest := {
		"version": PATCH_VERSION,
		"min_engine": "4.5",
		"packs": [
			{
				"name": PACK_NAME,
				"url": "res://fake_cdn/%s/%s" % [PATCH_VERSION, PACK_NAME],
				"sha256": sha,
				"size": size,
			}
		]
	}
	var mf := FileAccess.open(cdn_dir.path_join("version.json"), FileAccess.WRITE)
	mf.store_string(JSON.stringify(manifest, "  "))
	mf.close()

	print("[make_patch] pack:   %s (%d bytes)" % [pack_path, size])
	print("[make_patch] sha256: %s" % sha)
	print("[make_patch] DONE version=%s" % PATCH_VERSION)
	quit(0)
