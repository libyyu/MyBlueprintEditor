-- ui/FPanelMainMenu.lua
-- 主菜单面板
-- Prefab: UIMainMenu.prefab
--   btn_start / btn_continue / btn_level_select / lbl_progress

local FPanelBaseUI = require "ui.FPanelBaseUI"

local PREFAB_PATH = "Assets/DefaultPackage/UI/Prefab/UIMainMenu.prefab"

local l_instance = nil
---@class FPanelMainMenu : FPanelBaseUI
local FPanelMainMenu = FLua.Class(FPanelBaseUI, "FPanelMainMenu")
do
    function FPanelMainMenu.Instance()
        if not l_instance then l_instance = FPanelMainMenu() end
        return l_instance
    end

    function FPanelMainMenu:GetResPath() return PREFAB_PATH end

    function FPanelMainMenu:OnCreate()
        local root = self.m_panel.transform

        -- btn_start
        local btnStart = root:Find("btn_start")
        if btnStart then
            btnStart:GetComponent(typeof(UnityEngine.UI.Button)).onClick:AddListener(function()
                self:OnClickStart()
            end)
        end

        -- btn_continue
        local btnContinue = root:Find("btn_continue")
        if btnContinue then
            self.m_btnContinue = btnContinue.gameObject
            btnContinue:GetComponent(typeof(UnityEngine.UI.Button)).onClick:AddListener(function()
                self:OnClickContinue()
            end)
        end

        -- btn_level_select
        local btnLevels = root:Find("btn_level_select")
        if btnLevels then
            btnLevels:GetComponent(typeof(UnityEngine.UI.Button)).onClick:AddListener(function()
                self:OnClickLevelSelect()
            end)
        end

        -- lbl_progress
        local lblTr = root:Find("lbl_progress")
        self.m_lblProgress = lblTr and lblTr:GetComponent(typeof(TMPro.TMP_Text)) or nil
    end

    function FPanelMainMenu:OnShow(bShow)
        if not bShow then return end
        -- 检查存档，决定是否显示 CONTINUE
        local SaveMgr = require "game.save_manager"
        local last = SaveMgr and SaveMgr.get_last_level and SaveMgr.get_last_level()
        if self.m_btnContinue then
            self.m_btnContinue:SetActive(last ~= nil)
        end
        if self.m_lblProgress then
            self.m_lblProgress.text = last and ("Last: " .. last) or ""
        end
    end

    function FPanelMainMenu:OnClickStart()
        print("[MainMenu] btn_start")
        local LM = require "game.level_manager"
        LM.load_level("1_1")
        self:DestroyPanel()
    end

    function FPanelMainMenu:OnClickContinue()
        print("[MainMenu] btn_continue")
        local SaveMgr = require "game.save_manager"
        local last = SaveMgr.get_last_level()
        if last then
            local LM = require "game.level_manager"
            LM.load_level(last)
            self:DestroyPanel()
        end
    end

    function FPanelMainMenu:OnClickLevelSelect()
        print("[MainMenu] btn_level_select")
        require "ui.FPanelLevelSelect".Instance():ShowPanel(true)
    end

    function FPanelMainMenu:OnDestroy()
        l_instance = nil
    end
end

return FPanelMainMenu
