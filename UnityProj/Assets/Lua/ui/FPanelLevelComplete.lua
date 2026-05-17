-- ui/FPanelLevelComplete.lua
-- 关卡完成面板
-- Prefab: UILevelComplete.prefab
--   Panel/lbl_score / Panel/btn_next / Panel/btn_retry

local FPanelBaseUI = require "ui.FPanelBaseUI"

local PREFAB_PATH = "Assets/DefaultPackage/UI/Prefab/UILevelComplete.prefab"

local l_instance = nil
---@class FPanelLevelComplete : FPanelBaseUI
local FPanelLevelComplete = FLua.Class(FPanelBaseUI, "FPanelLevelComplete")
do
    function FPanelLevelComplete.Instance()
        if not l_instance then l_instance = FPanelLevelComplete() end
        return l_instance
    end

    function FPanelLevelComplete:GetResPath() return PREFAB_PATH end

    function FPanelLevelComplete:OnCreate()
        local root = self.m_panel.transform
        local panel = root:Find("Panel")
        if not panel then return end

        -- lbl_score
        local lblTr = panel:Find("lbl_score")
        self.m_lblScore = lblTr and lblTr:GetComponent(typeof(TMPro.TMP_Text)) or nil

        -- btn_next
        local btnNext = panel:Find("btn_next")
        if btnNext then
            btnNext:GetComponent(typeof(UnityEngine.UI.Button)).onClick:AddListener(function()
                self:OnClickNext()
            end)
        end

        -- btn_retry
        local btnRetry = panel:Find("btn_retry")
        if btnRetry then
            btnRetry:GetComponent(typeof(UnityEngine.UI.Button)).onClick:AddListener(function()
                self:OnClickRetry()
            end)
        end
    end

    --- 显示结算数据
    function FPanelLevelComplete:SetResult(levelId, score)
        self.m_levelId = levelId
        if self.m_lblScore then
            self.m_lblScore.text = "Score: " .. tostring(score or 0)
        end
        -- 存档
        local SaveMgr = require "game.save_manager"
        if SaveMgr and SaveMgr.set_last_level then
            SaveMgr.set_last_level(levelId)
        end
    end

    function FPanelLevelComplete:OnClickNext()
        print("[LevelComplete] next")
        -- 简单关卡 ID 递增：1_1 → 1_2
        local nextId = self:_nextLevelId(self.m_levelId)
        self:DestroyPanel()
        require "ui.FPanelHUD".Instance():DestroyPanel()
        if nextId then
            require "game.level_manager".load_level(nextId)
        else
            -- 没有下一关，回主菜单
            require "game.scene_manager".goto_main_menu()
        end
    end

    function FPanelLevelComplete:OnClickRetry()
        print("[LevelComplete] retry")
        local id = self.m_levelId
        self:DestroyPanel()
        require "ui.FPanelHUD".Instance():DestroyPanel()
        require "game.level_manager".load_level(id)
    end

    function FPanelLevelComplete:_nextLevelId(id)
        if not id then return nil end
        -- 格式 "world_level" 如 "1_1"
        local w, l = id:match("^(%d+)_(%d+)$")
        if w and l then
            return w .. "_" .. (tonumber(l) + 1)
        end
        return nil
    end

    function FPanelLevelComplete:OnDestroy()
        l_instance = nil
    end
end

return FPanelLevelComplete
