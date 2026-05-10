-- ui/level_failed.lua
-- 失败面板（重玩/回菜单）

local BasePanel = require 'ui/base_panel'
local LM        = require 'game/level_manager'

local M = {}

function M.open(ctrl)
    local panel = BasePanel.new(ctrl)

    panel:on_click("btn_retry", function()
        local UI = require 'ui/ui_manager'
        UI.dispose("UI/LevelFailed")
        LM.retry()
    end)

    panel:on_click("btn_menu", function()
        local UI = require 'ui/ui_manager'
        UI.close_all()
        LM.back_to_menu()
    end)

    return panel
end

return M
