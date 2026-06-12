-- ui/FPanelUITestUITK.lua
-- UI 框架测试面板 —— UI Toolkit 后端
-- 资产：Assets/DefaultPackage/UI/UITK/UITest.uxml
--
-- 全部测试逻辑在 FPanelUITestBase 中（后端无关，走 self.m_bridge）。
-- 本类只指定 UXML 资产；UI 内容由 UXML 提供，无需运行时建树。

local FPanelUITestBase = require "ui.FPanelUITestBase"

local UXML_PATH = "Assets/DefaultPackage/UI/UITK/UITest.uxml"

---@type FPanelUITestUITK
local l_instance = nil
---@class FPanelUITestUITK : FPanelUITestBase
local FPanelUITestUITK = FLua.Class(FPanelUITestBase, "FPanelUITestUITK")
do
    ---@return FPanelUITestUITK
    function FPanelUITestUITK.Instance()
        if not l_instance then l_instance = FPanelUITestUITK() end
        return l_instance
    end

    function FPanelUITestUITK:GetResPath() return UXML_PATH end

    function FPanelUITestUITK:OnDestroy()
        FPanelUITestBase.OnDestroy(self)
        l_instance = nil
    end
end

return FPanelUITestUITK
