-- ui/FPanelLevelSelect.lua
-- 关卡选择面板
-- Prefab: UILevelSelect.prefab
--   btn_back / Content/level_item_0 .. level_item_N

local FPanelBaseUI = require "ui.FPanelBaseUI"

local PREFAB_PATH = "Assets/DefaultPackage/UI/Prefab/UILevelSelect.prefab"

local l_instance = nil
---@class FPanelLevelSelect : FPanelBaseUI
local FPanelLevelSelect = FLua.Class(FPanelBaseUI, "FPanelLevelSelect")
do
    function FPanelLevelSelect.Instance()
        if not l_instance then l_instance = FPanelLevelSelect() end
        return l_instance
    end

    function FPanelLevelSelect:GetResPath() return PREFAB_PATH end

    function FPanelLevelSelect:OnCreate()
        local root = self.m_panel.transform

        -- btn_back
        local btnBack = root:Find("Header/btn_back")
        if btnBack then
            btnBack:GetComponent(typeof(UnityEngine.UI.Button)).onClick:AddListener(function()
                self:DestroyPanel()
            end)
        end

        -- 关卡按钮（level_item_0, level_item_1, ...）
        local levels = { "1_1", "1_2", "1_3" }
        local content = root:Find("Content")
        if content then
            for i, levelId in ipairs(levels) do
                local itemName = "level_item_" .. (i - 1)
                local item = content:Find(itemName)
                if item then
                    local btn = item:GetComponent(typeof(UnityEngine.UI.Button))
                    if btn then
                        local id = levelId  -- 闭包捕获
                        btn.onClick:AddListener(function()
                            self:OnClickLevel(id)
                        end)
                    end
                end
            end
        end
    end

    function FPanelLevelSelect:OnClickLevel(levelId)
        print("[LevelSelect] level: " .. levelId)
        local LM = require "game.level_manager"
        LM.load_level(levelId)
        self:DestroyPanel()
        -- 主菜单也关掉
        local mainMenu = require "ui.FPanelMainMenu"
        if l_instance ~= self then return end
        require "ui.FPanelMainMenu".Instance():DestroyPanel()
    end

    function FPanelLevelSelect:OnDestroy()
        l_instance = nil
    end
end

return FPanelLevelSelect
