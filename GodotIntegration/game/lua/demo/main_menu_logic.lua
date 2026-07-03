-- lua/demo/main_menu_logic.lua
-- 纯 Lua 编写的 UI 逻辑，验证 Lua 侧能：打开面板 / 设文本 / 订阅按钮点击。
-- Scene.* / UI.* 全局表由项目层(GDExtension godot_lua_bindings)注册，转发到 Godot
-- 的 SceneService / UiService。引擎(BlueprintRuntime)本身不含任何 Godot 逻辑。

local M = {}

function M.open()
    print("[lua] main_menu_logic.open()")

    -- 打开主菜单面板（Godot UiService 实例化 res://ui/MainMenuPanel.tscn）
    local ok = UI.open_panel("MainMenuPanel")
    print("[lua] UI.open_panel(MainMenuPanel) ->", ok)

    -- 改标题文字（证明 Lua 能操作 Godot 控件）
    UI.set_text("MainMenuPanel", "Center/VBox/Title", "主菜单 (Lua 驱动)")
    UI.set_text("MainMenuPanel", "Center/VBox/Hint", "本文字由 Lua 设置")

    -- 订阅按钮点击 —— 点击时回调这个 Lua 函数
    UI.on_click("MainMenuPanel", "Center/VBox/StartButton", function()
        print("[lua] StartButton clicked! (Lua callback)")
        UI.set_text("MainMenuPanel", "Center/VBox/Hint", "已点击开始 (来自 Lua)")
        -- 也可以在这里 Scene.change("res://scenes/xxx.tscn") 切场景
    end)

    -- 订阅输入框文本变化 —— 每次输入回调，实时把内容显示到 Hint
    UI.on_text_changed("MainMenuPanel", "Center/VBox/NameInput", function(text)
        print("[lua] NameInput text_changed:", text)
        UI.set_text("MainMenuPanel", "Center/VBox/Hint", "你好, " .. text)
    end)
end

-- 演示 Lua 侧订阅输入 action（键鼠/触摸/小游戏宿主统一映射）
function M.setup_input()
    print("[lua] main_menu_logic.setup_input()")

    -- 事件订阅：ui_confirm 按下时触发（回调带 action 名）
    Input.on_action("ui_confirm", function(action)
        print("[lua] Input.on_action fired:", action)
        UI.set_text("MainMenuPanel", "Center/VBox/Hint", "输入触发: " .. action)
    end)

    -- 也可轮询查询（供每帧调用的逻辑用）
    -- if Input.is_pressed("move_left") then ... end
    -- local ax = Input.get_axis("move_left", "move_right")  -- [-1,1]
end

-- 演示 Lua 侧 Timer + Log（对齐 Unity timer.lua / FileLoggger）
M.tick_count = 0
function M.setup_timer_log()
    Log.info("[lua] setup_timer_log 开始安排计时器")

    -- 一次性：0.1 秒后触发
    Timer.after(0.1, function()
        M.after_fired = true
        Log.info("[lua] Timer.after 触发（一次性）")
    end)

    -- 循环：每 0.05 秒，跑 3 次后自己取消
    local loop_id
    loop_id = Timer.every(0.05, function()
        M.tick_count = M.tick_count + 1
        Log.debug("[lua] Timer.every tick #" .. M.tick_count)
        if M.tick_count >= 3 then
            Timer.cancel(loop_id)
            Log.warn("[lua] Timer.every 已循环3次，自行取消")
        end
    end)
end

-- 异步打开面板（后台线程加载，不卡主线程）。加载完成后回调 on_ready(ok)。
function M.open_async()
    print("[lua] main_menu_logic.open_async()")
    UI.open_panel_async("MainMenuPanel", function(ok)
        print("[lua] open_panel_async done, ok =", ok)
        if ok then
            UI.set_text("MainMenuPanel", "Center/VBox/Title", "主菜单 (Lua 异步)")
        end
    end)
end

return M
