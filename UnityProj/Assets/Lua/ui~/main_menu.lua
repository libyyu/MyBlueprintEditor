-- ui/main_menu.lua
-- 主菜单面板逻辑（全在 Lua，C# UIController 只提供组件接口）

local BasePanel = require 'ui/base_panel'
local SM        = require 'game/scene_manager'
local LM        = require 'game/level_manager'
local SaveMgr   = require 'game/save_manager'

local M = {}

--- 打开主菜单（由 UIManager 回调传入 ctrl）
function M.open(ctrl)
    local panel = BasePanel.new(ctrl)

    panel:on_show(function()
        -- 显示上次游戏进度
        local last = SaveMgr.get_last_level()
        if last then
            panel:set_text("lbl_progress", "Last: " .. last)
            panel:set_active("btn_continue", true)
        else
            panel:set_active("btn_continue", false)
        end
    end)

    panel:on_click("btn_start", function()
        -- 从第一关开始
        panel:set_interactable("btn_start", false)
        LM.load_level("1_1")
    end)

    panel:on_click("btn_continue", function()
        local last = SaveMgr.get_last_level()
        if last then
            panel:set_interactable("btn_continue", false)
            LM.load_level(last)
        end
    end)

    panel:on_click("btn_level_select", function()
        local UI = require 'ui/ui_manager'
        UI.open("UI/LevelSelect")
    end)

    panel:on_click("btn_settings", function()
        local UI = require 'ui/ui_manager'
        UI.open("UI/Settings")
    end)

    return panel
end

return M
