-- ui/level_select.lua
-- 关卡选择面板

local BasePanel  = require 'ui/base_panel'
local LevelData  = require 'game/level_data'
local SaveMgr    = require 'game/save_manager'
local LM         = require 'game/level_manager'

local M = {}

function M.open(ctrl)
    local panel = BasePanel.new(ctrl)

    panel:on_show(function()
        -- 渲染每个章节关卡按钮（假设 Prefab 里有 btn_1_1 ~ btn_2_5）
        for _, chapter in ipairs(LevelData.chapters) do
            for _, lv in ipairs(chapter.levels) do
                local id  = lv.id                        -- e.g. "1_1"
                local name = "btn_" .. id
                local stars = SaveMgr.get_stars(id) or 0
                local unlocked = SaveMgr.is_unlocked(id)

                panel:set_active(name, true)
                panel:set_interactable(name, unlocked)
                -- 星星数用 lbl_stars_1_1 显示
                panel:set_text("lbl_stars_" .. id, string.rep("★", stars) .. string.rep("☆", 3 - stars))
            end
        end
    end)

    -- 动态为每个关卡按钮注册点击
    for _, chapter in ipairs(LevelData.chapters) do
        for _, lv in ipairs(chapter.levels) do
            local id = lv.id
            panel:on_click("btn_" .. id, function()
                local UI = require 'ui/ui_manager'
                UI.dispose("UI/LevelSelect")
                LM.load_level(id)
            end)
        end
    end

    panel:on_click("btn_back", function()
        local UI = require 'ui/ui_manager'
        UI.dispose("UI/LevelSelect")
    end)

    return panel
end

return M
