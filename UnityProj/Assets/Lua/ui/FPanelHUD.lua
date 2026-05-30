-- ui/FPanelHUD.lua
-- 游戏内 HUD — 改善版
-- Prefab: UIHUD.prefab
--   TopBar/lbl_score / TopBar/btn_pause
--   TopBar/lbl_stars / TopBar/lbl_cuts (动态创建)

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

    function FPanelHUD:__constructor()
        self.m_lblScore = nil
        self.m_lblStars = nil
        self.m_lblCuts = nil
    end

    function FPanelHUD:GetResPath() return PREFAB_PATH end

    function FPanelHUD:OnCreate()
        local root = self.m_panel.transform

        -- lbl_score（分数，居中）
        local lblTr = root:Find("TopBar/lbl_score")
        self.m_lblScore = lblTr and lblTr:GetComponent(typeof(CS.TMPro.TMP_Text)) or nil

        -- lbl_stars（星星，左侧）
        local starTr = root:Find("TopBar/lbl_stars")
        self.m_lblStars = starTr and starTr:GetComponent(typeof(CS.TMPro.TMP_Text)) or nil

        -- lbl_cuts（刀数，分数右侧）
        local cutTr = root:Find("TopBar/lbl_cuts")
        self.m_lblCuts = cutTr and cutTr:GetComponent(typeof(CS.TMPro.TMP_Text)) or nil

        -- btn_pause
        local btnPause = root:Find("TopBar/btn_pause")
        if btnPause then
            btnPause:GetComponent(typeof(CS.UnityEngine.UI.Button)).onClick:AddListener(function()
                self:OnClickPause()
            end)
        end

        -- 初始化数值
        self:SetScore(0)
        self:SetStars(0, 3)
        self:SetCuts(0)
    end

    --- 更新分数显示
    function FPanelHUD:SetScore(score)
        if self.m_lblScore then
            self.m_lblScore.text = tostring(score)
        end
    end

    --- 更新星星显示 (collected/total)
    function FPanelHUD:SetStars(collected, total)
        if not self.m_lblStars then return end
        total = total or 3
        local stars = ""
        for i = 1, total do
            if i <= collected then
                stars = stars .. "★"
            else
                stars = stars .. "☆"
            end
        end
        self.m_lblStars.text = stars
        -- 收集满了变金色
        if collected >= total then
            self.m_lblStars.color = CS.UnityEngine.Color(1.0, 0.85, 0.1, 1.0)
        elseif collected > 0 then
            self.m_lblStars.color = CS.UnityEngine.Color(1.0, 0.85, 0.1, 1.0)
        else
            self.m_lblStars.color = CS.UnityEngine.Color(0.7, 0.7, 0.7, 1.0)
        end
    end

    --- 更新刀数显示
    function FPanelHUD:SetCuts(cuts)
        if self.m_lblCuts then
            self.m_lblCuts.text = "✂ " .. tostring(cuts)
        end
    end

    function FPanelHUD:OnClickPause()
        -- 优先通过蓝图事件 on_pause 驱动（蓝图可选择打开哪个面板）
        local BPR = CS.UGFramework.Runtime.BlueprintRuntime
        if BPR and BPR.Instance and BPR.Instance.IsValid then
            BPR.Instance:DispatchEvent('on_pause')
        else
            -- 蓝图不存在时直接开 Pause 面板
            local ctrl = CS.CutRope.Game.LevelController.Current
            if ctrl then ctrl:PauseLevel() end
            require 'ui.FPanelPause'.Instance():ShowPanel(true)
        end
    end

    function FPanelHUD:OnDestroy()
        l_instance = nil
    end
end

return FPanelHUD
