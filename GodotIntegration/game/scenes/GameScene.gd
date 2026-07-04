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
	#var launcher := get_node_or_null("/root/Launcher")   # autoload，跨场景常驻
	#bp.load_blueprint("res://blueprints/blueprint_1_1.bjson")
	#if launcher and launcher.has_method("bind_lua_api"):
		#launcher.bind_lua_api(bp)   # 注册 Scene.*/UI.* 到该 runner 的 Lua VM（引擎零改动）

	# --- 路径 2：纯 Lua 编写的 UI 逻辑（开面板 / 设文本 / 订阅按钮点击） ---
	#bp.run_lua("local ok, m = pcall(require, 'demo.main_menu_logic'); if ok then m.open() else print('[lua] require failed: '..tostring(m)) end")

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

			# --- 验证 UI.on_text_changed（Lua 订阅输入框文本变化） ---
			var name_input = panel.get_node_or_null("Center/VBox/NameInput")
			if name_input and hint:
				name_input.text_changed.emit("阿伟")   # headless 模拟一次输入
				await get_tree().process_frame
				print("[GameScene] 文本变化后 Lua 回调改写的提示 = '", hint.text, "'")

	# --- 验证 Input 系统（GDScript + Lua 双端） ---
	var input_srv := get_node_or_null("/root/InputService")
	if input_srv:
		# Lua 侧先订阅 ui_confirm
		#bp.run_lua("local ok,m = pcall(require,'demo.main_menu_logic'); if ok then m.setup_input() end")
		await get_tree().process_frame
		# GDScript 侧也订阅同一 action，验证 GDScript 路径
		var gd_fired := {"hit": false}
		input_srv.on_action("ui_confirm", func(): gd_fired["hit"] = true)
		# 模拟 action 触发（headless 无键盘）→ 两端回调都应触发
		input_srv.fire_action("ui_confirm", "pressed")
		await get_tree().process_frame
		print("[GameScene] GDScript Input.on_action 回调触发 = ", gd_fired["hit"])
		if ui:
			var panel2 = ui.get_panel("MainMenuPanel")
			if panel2:
				var hint2 = panel2.get_node_or_null("Center/VBox/Hint")
				if hint2:
					print("[GameScene] Lua Input.on_action 改写的提示 = '", hint2.text, "'")

	# --- 验证 Timer + Log（GDScript + Lua 双端） ---
	var timer_srv := get_node_or_null("/root/TimerService")
	var log_srv := get_node_or_null("/root/LogService")
	if timer_srv and log_srv:
		# GDScript 侧：after 一次性
		var gd_fired := {"hit": 0}
		log_srv.info("[GameScene] GDScript 安排 Timer.after")
		timer_srv.after(0.1, func():
			gd_fired["hit"] += 1
			log_srv.info("[GameScene] GDScript Timer.after 触发"))
		# Lua 侧：after + every（模块里 setup_timer_log）
		#bp.run_lua("local ok,m=pcall(require,'demo.main_menu_logic'); if ok then m.setup_timer_log() end")
		# 等足够帧让计时器到期（0.1s after + 3×0.05s every）
		await get_tree().create_timer(0.4).timeout
		print("[GameScene] GDScript Timer.after 触发次数 = ", gd_fired["hit"])
		# 读回 Lua 模块状态验证 Lua 计时器也跑了
		bp.run_lua("local ok,m=pcall(require,'demo.main_menu_logic'); if ok then print('[GameScene] Lua after_fired='..tostring(m.after_fired)..' every_ticks='..tostring(m.tick_count)) end")

	# --- 异步加载演示（后台线程加载 .tscn，不卡主线程） ---
	if ui:
		# 路径 1（GDScript）：await 拿到实例
		ui.close_panel("UpdatePanel")
		var async_panel = await ui.open_panel_async("UpdatePanel")
		print("[GameScene] GDScript await open_panel_async -> ", async_panel != null)
		# 路径 2（Lua）：回调风格，加载完成后回调 function(ok)
		ui.close_panel("UpdatePanel")
		#bp.run_lua("UI.open_panel_async('UpdatePanel', function(ok) print('[lua] UpdatePanel async loaded, ok=', ok) end)")
		await get_tree().create_timer(0.3).timeout
		print("[GameScene] Lua async 完成后 is_open(UpdatePanel) = ", ui.is_open("UpdatePanel"))
