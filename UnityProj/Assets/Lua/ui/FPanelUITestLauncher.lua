-- ui/FPanelUITestLauncher.lua
-- UI 框架测试入口（UI Toolkit 后端）
-- 资产：Assets/DefaultPackage/UI/UITK/UITestLauncher.uxml
--
-- 两个按钮分别打开两种后端的测试面板：
--   btn_open_uitk → FPanelUITestUITK
--   btn_open_ugui → FPanelUITestUGUI
-- 全部交互走 self.m_bridge（IUIPanelBridge），后端无关。

local FPanelBaseUI = require "ui.FPanelBaseUI"

local UXML_PATH = "Assets/DefaultPackage/UI/UITK/UITestLauncher.uxml"

---@type FPanelUITestLauncher
local l_instance = nil
---@class FPanelUITestLauncher : FPanelBaseUI
local FPanelUITestLauncher = FLua.Class(FPanelBaseUI, "FPanelUITestLauncher")
do
    ---@return FPanelUITestLauncher
    function FPanelUITestLauncher.Instance()
        if not l_instance then l_instance = FPanelUITestLauncher() end
        return l_instance
    end

    function FPanelUITestLauncher:GetResPath() return UXML_PATH end

    function FPanelUITestLauncher:OnCreate()
        local bridge = self.m_bridge
        if not bridge then
            printError("[FPanelUITestLauncher] m_bridge is nil, backend not ready")
            return
        end

        bridge:RegisterClick("btn_open_uitk", function()
            bridge:SetText("lbl_status", "打开 UI Toolkit 测试面板...")
            self:SetVisible(false)
            require "ui.FPanelUITestUITK".Instance():ShowPanel(true)
        end)

        bridge:RegisterClick("btn_open_ugui", function()
            bridge:SetText("lbl_status", "打开 UGUI 测试面板...")
            self:SetVisible(false)
            require "ui.FPanelUITestUGUI".Instance():ShowPanel(true)
        end)

        bridge:RegisterClick("btn_close", function()
            self:DestroyPanel()
        end)

        print("[FPanelUITestLauncher] OnCreate complete")
    end

    function FPanelUITestLauncher:OnDestroy()
        print("[FPanelUITestLauncher] OnDestroy")
        l_instance = nil
    end
end

return FPanelUITestLauncher
