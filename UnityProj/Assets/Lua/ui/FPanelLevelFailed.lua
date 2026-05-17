-- ui/FPanelLevelFailed.lua
-- 关卡失败面板
-- Prefab: UILevelFailed.prefab
--   Panel/btn_retry / Panel/btn_home

local FPanelBaseUI = require "ui.FPanelBaseUI"

local PREFAB_PATH = "Assets/DefaultPackage/UI/Prefab/UILevelFailed.prefab"

local l_instance = nil
---@class FPanelLevelFailed : FPanelBaseUI
local FPanelLevelFailed = FLua.Class(FPanelBaseUI, "FPanelLevelFailed")
do
    function FPanelLevelFailed.Instance()
        if not l_instance then l_instance = FPanelLevelFailed() end
        return l_instance
    end

    function FPanelLevelFailed:GetResPath() return PREFAB_PATH end

    function FPanelLevelFailed:OnCreate()
        local root = self.m_panel.transform
        local panel = root:Find("Panel")
        if not panel then return end

        -- btn_retry
        local btnRetry = panel:Find("btn_retry")
        if btnRetry then
            btnRetry:GetComponent(typeof(UnityEngine.UI.Button)).onClick:AddListener(function()
                self:OnClickRetry()
            end)
        end

        -- btn_home
        local btnHome = panel:Find("btn_home")
        if btnHome then
            btnHome:GetComponent(typeof(UnityEngine.UI.Button)).onClick:AddListener(function()
                self:OnClickHome()
            end)
        end
    end

    function FPanelLevelFailed:SetLevelId(levelId)
        self.m_levelId = levelId
    end

    function FPanelLevelFailed:OnClickRetry()
        print("[LevelFailed] retry")
        local id = self.m_levelId
        self:DestroyPanel()
        require "ui.FPanelHUD".Instance():DestroyPanel()
        require "game.level_manager".load_level(id)
    end

    function FPanelLevelFailed:OnClickHome()
        print("[LevelFailed] home")
        self:DestroyPanel()
        require "ui.FPanelHUD".Instance():DestroyPanel()
        require "game.scene_manager".goto_main_menu()
    end

    function FPanelLevelFailed:OnDestroy()
        l_instance = nil
    end
end

return FPanelLevelFailed
