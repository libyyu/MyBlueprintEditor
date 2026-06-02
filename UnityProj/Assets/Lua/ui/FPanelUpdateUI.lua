-- ui/FPanelUpdateUI.lua
-- 更新阶段进度条面板
-- Prefab 结构（UILoading.prefab）：
--   UILoading
--     └── bg
--     └── Slider          ← UnityEngine.UI.Slider
--
-- 使用：
--   local UpdateUI = require "ui.FPanelUpdateUI"
--   UpdateUI.Instance():ShowPanel(true)
--   UpdateUI.Instance():SetProgress(0.6)   -- 0.0 ~ 1.0
--   UpdateUI.Instance():SetStatus("正在下载资源...")
--   UpdateUI.Instance():DestroyPanel()

local FPanelBaseUI = require "ui.FPanelBaseUI"

local PREFAB_PATH = "Assets/DefaultPackage/UI/Prefab/UILoading.prefab"

local l_instance = nil
---@class FPanelUpdateUI : FPanelBaseUI
local FPanelUpdateUI = FLua.Class(FPanelBaseUI, "FPanelUpdateUI")
do
    function FPanelUpdateUI.Instance()
        if not l_instance then
            l_instance = FPanelUpdateUI()
        end
        return l_instance
    end

    function FPanelUpdateUI:__constructor()
		self.m_slider = nil
	end

    function FPanelUpdateUI:GetResPath()
        return PREFAB_PATH
    end

    function FPanelUpdateUI:OnCreate()
        -- 找 Slider 组件（路径：UILoading/Slider）
        local sliderTr = self:RequireFind("bg/Slider")
        self.m_slider = sliderTr and sliderTr:GetComponent(typeof(UnityEngine.UI.Slider)) or nil
        if not self.m_slider then
            print("[FPanelUpdateUI] Warning: Slider not found")
        end
        self:SetProgress(0)
    end

    --- 设置进度，0.0 ~ 1.0
    function FPanelUpdateUI:SetProgress(value)
        if self.m_slider then
            self.m_slider.value = math.max(0, math.min(1, value))
        end
    end

    --- 销毁时清单例
    function FPanelUpdateUI:OnDestroy()
        l_instance = nil
    end
end

return FPanelUpdateUI
