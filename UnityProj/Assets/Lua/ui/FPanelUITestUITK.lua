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

    -- ── 列表 item 钩子（基类 SetupLists 调用，后端相关）─────────────────

    --- 创建一个列表 item 节点（VisualElement，这里用 Label 直接承载文本）
    ---@return VisualElement
    function FPanelUITestUITK:MakeItemTemplate()
        local lbl = CS.UnityEngine.UIElements.Label()
        lbl.style.fontSize = 22
        lbl.style.color = CS.UnityEngine.Color(1, 1, 1, 1)
        lbl.style.unityTextAlign = CS.UnityEngine.TextAnchor.MiddleLeft
        lbl.style.paddingLeft = 12
        lbl.style.height = 56
        return lbl
    end

    --- 把文本写到 item 节点（VisualElement 是 Label / 含 Label）
    function FPanelUITestUITK:SetItemText(itemObj, text)
        if itemObj == nil then return end
        if itemObj.text ~= nil then          -- Label/Button 直接有 text
            itemObj.text = text
            return
        end
        local lbl = itemObj:Q(nil, "unity-label") -- 兜底：找子 Label
        if lbl then lbl.text = text end
    end

    function FPanelUITestUITK:OnDestroy()
        FPanelUITestBase.OnDestroy(self)
        l_instance = nil
    end
end

return FPanelUITestUITK
