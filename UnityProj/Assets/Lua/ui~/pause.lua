-- ui/pause.lua
-- 暂停面板

local BasePanel = require 'ui/base_panel'
local LM        = require 'game/level_manager'

local M = {}

function M.open(ctrl)
    local panel = BasePanel.new(ctrl)

    panel:on_click("btn_resume", function()
        local UI = require 'ui/ui_manager'
        UI.dispose("UI/Pause")
        -- 通知 HUD 恢复计时
        local hud = require 'ui/hud'
        hud.resume and hud.resume()
    end)

    panel:on_click("btn_retry", function()
        local UI = require 'ui/ui_manager'
        UI.close_all()
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
