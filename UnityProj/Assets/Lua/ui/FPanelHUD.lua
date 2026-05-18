-- ui/FPanelHUD.lua
-- 游戏内 HUD
-- Prefab: UIHUD.prefab
--   TopBar/lbl_score / TopBar/btn_pause

local FPanelBaseUI = require "ui.FPanelBaseUI"

local PREFAB_PATH = "Assets/DefaultPackage/UI/Prefab/UIHUD.prefab"

local l_instance = nil
---@class FPanelHUD : FPanelBaseUI
local FPanelHUD = FLua.Class(FPanelBaseUI, "FPanelHUD")
do
    function FPanelHUD.Instance()
        if not l_instance then l_instance = FPanelHUD() end
        return l_instance
    end

    function FPanelHUD:GetResPath() return PREFAB_PATH end

    function FPanelHUD:OnCreate()
        local root = self.m_panel.transform

        -- lbl_score
        local lblTr = root:Find("TopBar/lbl_score")
        self.m_lblScore = lblTr and lblTr:GetComponent(typeof(TMPro.TMP_Text)) or nil

        -- btn_pause
        local btnPause = root:Find("TopBar/btn_pause")
        if btnPause then
            btnPause:GetComponent(typeof(UnityEngine.UI.Button)).onClick:AddListener(function()
                self:OnClickPause()
            end)
        end

        self:SetScore(0)
    end

    --- 更新分数显示
    function FPanelHUD:SetScore(score)
        if self.m_lblScore then
            self.m_lblScore.text = tostring(score)
        end
    end

    function FPanelHUD:OnClickPause()
        print("[HUD] pause (TODO: FPanelPause not implemented yet)")
        -- TODO: require 'ui.FPanelPause'.Instance():ShowPanel(true)
    end

    function FPanelHUD:OnDestroy()
        l_instance = nil
    end
end

return FPanelHUD
