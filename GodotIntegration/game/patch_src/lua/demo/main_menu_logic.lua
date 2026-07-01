-- lua/demo/main_menu_logic.lua  【补丁版 v1.0.1】
-- 这是热更补丁包里的“新版”脚本。打进 patch pck 挂载后会覆盖 res:// 里的原版。
-- 与原版的唯一区别：标题带上 (热更 v1.0.1)，用来肉眼/自动验证补丁确实生效。

local M = {}

function M.open()
    print("[lua] main_menu_logic.open() [PATCHED v1.0.1]")

    local ok = UI.open_panel("MainMenuPanel")
    print("[lua] UI.open_panel(MainMenuPanel) ->", ok)

    -- 标题带热更标记（原版是 "主菜单 (Lua 驱动)"）
    UI.set_text("MainMenuPanel", "Center/VBox/Title", "主菜单 (热更 v1.0.1)")
    UI.set_text("MainMenuPanel", "Center/VBox/Hint", "本文字来自热更补丁包")

    UI.on_click("MainMenuPanel", "Center/VBox/StartButton", function()
        print("[lua] StartButton clicked! (patched)")
        UI.set_text("MainMenuPanel", "Center/VBox/Hint", "已点击开始 (热更版)")
    end)
end

function M.open_async()
    UI.open_panel_async("MainMenuPanel", function(ok)
        if ok then
            UI.set_text("MainMenuPanel", "Center/VBox/Title", "主菜单 (热更 v1.0.1 异步)")
        end
    end)
end

return M
