-- ui/level_complete.lua
-- 通关面板（星级、得分、下一关/重玩/回菜单）

local BasePanel = require 'ui/base_panel'
local LM        = require 'game/level_manager'

local M = {}

--- @param ctrl   UIController
--- @param stars  number  1~3
--- @param score  number
function M.open(ctrl, stars, score)
    local panel = BasePanel.new(ctrl)
    stars = stars or 1
    score = score or 0

    panel:on_show(function()
        -- 星星显示（假设有 star_1 / star_2 / star_3 子节点）
        for i = 1, 3 do
            panel:set_active("star_" .. i, i <= stars)
        end
        panel:set_text("lbl_score", tostring(score))
        -- 判断是否有下一关
        local has_next = LM.has_next()
        panel:set_active("btn_next", has_next)
    end)

    panel:on_click("btn_next", function()
        local UI = require 'ui/ui_manager'
        UI.dispose("UI/LevelComplete")
        LM.load_next()
    end)

    panel:on_click("btn_retry", function()
        local UI = require 'ui/ui_manager'
        UI.dispose("UI/LevelComplete")
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
