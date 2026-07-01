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
