-- ui/FPanelUITest.lua
-- UI Toolkit 框架测试面板
-- UXML: Assets/DefaultPackage/UI/UITK/UITest.uxml
--
-- 测试覆盖：
--   1. RegisterClick 精确绑定
--   2. OnClick 全局扫描模式（注释掉的备选）
--   3. bridge:SetText / GetText
--   4. bridge:SetDisplay 显隐切换
--   5. bridge:AddClass / RemoveClass
--   6. 生命周期：OnCreate / OnDestroy
--
-- 后端无关：
--   self.m_bridge 在 UGUI/UITK 后端均实现 IUIPanelBridge 接口，
--   下方所有 RegisterClick / SetText / SetDisplay / SetColor / SetSprite
--   等调用代码完全相同。仅需把 GetResPath 返回的 .uxml 改为 .prefab
--   即可切换到 UGUI 后端运行（前提：Prefab 中的子节点 name 与 UXML 元素 name 对齐）。

local FPanelBaseUI = require "ui.FPanelBaseUI"

local UXML_PATH = "Assets/DefaultPackage/UI/UITK/UITest.uxml"

---@type FPanelUITest
local l_instance = nil
---@class FPanelUITest : FPanelBaseUI
local FPanelUITest = FLua.Class(FPanelBaseUI, "FPanelUITest")
do
    ---@return FPanelUITest
    function FPanelUITest.Instance()
        if not l_instance then l_instance = FPanelUITest() end
        return l_instance
    end

    function FPanelUITest:GetResPath() return UXML_PATH end

    function FPanelUITest:__constructor()
        self.m_toggleVisible = true  -- lbl_toggle_target 初始可见
    end
    -- ──────────────────────────────────────────────────────────────────
    -- OnCreate：面板加载完成，self.m_bridge 已自动注入
    -- ──────────────────────────────────────────────────────────────────
    function FPanelUITest:OnCreate()
        local bridge = self.m_bridge
        self.m_toggleVisible = true  -- lbl_toggle_target 初始可见

        -- ① 精确绑定：每个按钮独立回调
        bridge:RegisterClick("btn_test_text", function()
            self:TestSetText()
        end)

        bridge:RegisterClick("btn_test_display", function()
            self:TestSetDisplay()
        end)

        bridge:RegisterClick("btn_test_gettext", function()
            self:TestGetText()
        end)

        bridge:RegisterClick("btn_test_class", function()
            self:TestAddClass()
        end)

        bridge:RegisterClick("btn_close", function()
            self:DestroyPanel()
        end)

        -- 初始状态
        bridge:SetText("lbl_status", "status: OnCreate OK ✓")
        print("[FPanelUITest] OnCreate complete, bridge =", bridge)
    end

    --[[────────────────────────────────────────────────────────────────
      备选：全局扫描模式（与上面 RegisterClick 二选一）
      注释掉 OnCreate 里的 RegisterClick，改用下面这个即可
    ────────────────────────────────────────────────────────────────]]
    -- function FPanelUITest:OnClick(name)
    --     if     name == "btn_test_text"    then self:TestSetText()
    --     elseif name == "btn_test_display" then self:TestSetDisplay()
    --     elseif name == "btn_test_gettext" then self:TestGetText()
    --     elseif name == "btn_test_class"   then self:TestAddClass()
    --     elseif name == "btn_close"        then self:DestroyPanel()
    --     end
    -- end

    -- ──────────────────────────────────────────────────────────────────
    -- 测试用例
    -- ──────────────────────────────────────────────────────────────────

    function FPanelUITest:TestSetText()
        local bridge = self.m_bridge
        local ts = tostring(os.clock()):sub(1, 6)
        bridge:SetText("lbl_status", "SetText OK @ " .. ts)
        bridge:SetText("btn_test_text", "① 点击成功 ✓")
    end

    function FPanelUITest:TestSetDisplay()
        local bridge = self.m_bridge
        self.m_toggleVisible = not self.m_toggleVisible
        bridge:SetDisplay("lbl_toggle_target", self.m_toggleVisible)
        local state = self.m_toggleVisible and "显示" or "隐藏"
        bridge:SetText("lbl_status", "SetDisplay → " .. state)
    end

    function FPanelUITest:TestGetText()
        local bridge = self.m_bridge
        local t = bridge:GetText("lbl_title")
        bridge:SetText("lbl_status", "GetText: [" .. (t or "nil") .. "]")
    end

    function FPanelUITest:TestAddClass()
        local bridge = self.m_bridge
        -- USS 类名测试（需要 PanelSettings 里引用对应 USS，否则只是逻辑上挂上去）
        bridge:AddClass("lbl_status", "highlight")
        bridge:SetText("lbl_status", "AddClass 'highlight' 已添加")
    end

    -- ──────────────────────────────────────────────────────────────────
    function FPanelUITest:OnDestroy()
        print("[FPanelUITest] OnDestroy")
        l_instance = nil
    end
end

return FPanelUITest
