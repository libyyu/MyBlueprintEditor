extends Node2D
## 主游戏场景：演示 Scene/UI 框架的两条使用路径（同一套框架，两端都能用）。
##   路径 1（GDScript 直接用）：直接调 autoload UiService/SceneService。
##   路径 2（Lua 驱动）：BlueprintNode 的 Lua VM 里 require 一个 Lua 模块，
##       通过项目层注册的 Scene.*/UI.* 全局表操作 Godot（开面板 / 设文本 / 按钮点击回调）。
## 引擎（BlueprintRuntime）零改动：Lua 绑定用 BP_GetLuaState + dll 导出的 lua 符号，
## 全部在 GDExtension（业务层）实现。不含 CutRope 玩法。

@onready var bp: BlueprintNode = $BlueprintNode

func _ready() -> void:
	print("[GameScene] loaded via SceneService.change_scene")
	call_deferred("_demo")

func _demo() -> void:
	# --- 准备：加载一个占位蓝图（有 runner 才有 Lua VM），并把 Scene/UI 绑进该 VM ---
	var launcher := get_node_or_null("/root/Launcher")   # autoload，跨场景常驻
	bp.load_blueprint("res://blueprints/blueprint_1_1.bjson")
	if launcher and launcher.has_method("bind_lua_api"):
		launcher.bind_lua_api(bp)   # 注册 Scene.*/UI.* 到该 runner 的 Lua VM（引擎零改动）

	# --- 路径 2：纯 Lua 编写的 UI 逻辑（开面板 / 设文本 / 订阅按钮点击） ---
	bp.run_lua("local ok, m = pcall(require, 'demo.main_menu_logic'); if ok then m.open() else print('[lua] require failed: '..tostring(m)) end")

	# --- 验证：Lua 确实驱动了 Godot 侧（不依赖 Lua print 通道） ---
	var ui := get_node_or_null("/root/UiService")
	if ui:
		var panel = ui.get_panel("MainMenuPanel")
		print("[GameScene] Lua 打开面板 MainMenuPanel = ", panel != null)
		if panel:
			var title = panel.get_node_or_null("Center/VBox/Title")
			if title:
				print("[GameScene] Lua 设置的标题 = '", title.text, "'")
			# 模拟按钮点击（headless 无鼠标），验证点击事件回流到 Lua 回调
			var btn = panel.get_node_or_null("Center/VBox/StartButton")
			var hint = panel.get_node_or_null("Center/VBox/Hint")
			if btn and hint:
				btn.pressed.emit()
				await get_tree().process_frame
				print("[GameScene] 点击后 Lua 回调改写的提示 = '", hint.text, "'")

	# --- 异步加载演示（后台线程加载 .tscn，不卡主线程） ---
	if ui:
		# 路径 1（GDScript）：await 拿到实例
		ui.close_panel("UpdatePanel")
		var async_panel = await ui.open_panel_async("UpdatePanel")
		print("[GameScene] GDScript await open_panel_async -> ", async_panel != null)
		# 路径 2（Lua）：回调风格，加载完成后回调 function(ok)
		ui.close_panel("UpdatePanel")
		bp.run_lua("UI.open_panel_async('UpdatePanel', function(ok) print('[lua] UpdatePanel async loaded, ok=', ok) end)")
		await get_tree().create_timer(0.3).timeout
		print("[GameScene] Lua async 完成后 is_open(UpdatePanel) = ", ui.is_open("UpdatePanel"))
