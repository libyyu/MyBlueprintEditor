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
        -- 优先通过蓝图事件 on_pause 驱动（蓝图可选择打开哪个面板）
        local BPR = CS.CutRope.Framework.BlueprintRuntime
        if BPR and BPR.Instance and BPR.Instance.IsValid then
            BPR.Instance:DispatchEvent('on_pause')
        else
            -- 蓝图不存在时直接开 Pause 面板
            local ctrl = CS.CutRope.Game.LevelController.Current
            if ctrl then ctrl:PauseLevel() end
            require 'ui.FPanelPause'.Instance():ShowPanel(true)
        end
    end
    end

    function FPanelHUD:OnDestroy()
        l_instance = nil
    end
end

return FPanelHUD
