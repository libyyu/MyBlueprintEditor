-- ui/FPanelPause.lua
-- 暂停面板
-- Prefab: UIPause.prefab
--   btn_resume  ← 继续游戏
--   btn_retry   ← 重试
--   btn_home    ← 回主菜单

local FPanelBaseUI = require "ui.FPanelBaseUI"

local PREFAB_PATH = "Assets/DefaultPackage/UI/Prefab/UIPause.prefab"

local l_instance = nil
---@class FPanelPause : FPanelBaseUI
local FPanelPause = FLua.Class(FPanelBaseUI, "FPanelPause")
do
    function FPanelPause.Instance()
        if not l_instance then l_instance = FPanelPause() end
        return l_instance
    end

    function FPanelPause:GetResPath() return PREFAB_PATH end

    function FPanelPause:OnCreate()
        local root = self.m_panel.transform

        local function bindBtn(path, fn)
            local go = root:Find(path)
            if go then
                local btn = go:GetComponent(typeof(UnityEngine.UI.Button))
                if btn then btn.onClick:AddListener(fn) end
            end
        end

        -- 继续游戏：通过蓝图 Level.Resume 节点处理（这里提供直接 Lua 路径备用）
        bindBtn("btn_resume", function()
            local ctrl = CS.CutRope.Game.LevelController.Current
            if ctrl then ctrl:ResumeLevel() end
            self:DestroyPanel()
        end)

        -- 重试
        bindBtn("btn_retry", function()
            local ctrl = CS.CutRope.Game.LevelController.Current
            if ctrl then ctrl:ResumeLevel() end   -- 先恢复时间流动
            self:DestroyPanel()
            require "game.level_manager".retry()
        end)

        -- 回主菜单
        bindBtn("btn_home", function()
            local ctrl = CS.CutRope.Game.LevelController.Current
            if ctrl then ctrl:ResumeLevel() end
            self:DestroyPanel()
            require "game.level_manager".back_to_menu()
        end)
    end

    function FPanelPause:OnDestroy()
        l_instance = nil
    end
end

return FPanelPause
