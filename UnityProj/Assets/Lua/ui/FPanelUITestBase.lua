-- ui/FPanelUITestBase.lua
-- UI 框架测试面板【共享逻辑基类】—— 后端无关
--
-- 设计：所有交互只通过 self.m_bridge（IUIPanelBridge）调用，UGUI / UIToolkit 两端
--       实现完全相同。子类只负责：
--         1) GetResPath() 返回各自资产（.uxml / .prefab）
--         2) BuildContent() 可选——UGUI 子类在此运行时建树（UITK 由 UXML 提供）
--
-- 测试覆盖：
--   基础（两端都有）：RegisterClick / SetText / GetText / SetDisplay / SetColor
--   P3 表单事件（元素存在才测）：
--       input_name : SetInputText/GetInputText + RegisterSubmit + RegisterTextChange
--       sld_volume : RegisterValueChange + Set/GetSliderValue
--       tog_enable : RegisterValueChange + Set/GetToggleValue
--
-- 元素 name 约定（UXML 元素 name 与 UGUI 子节点 name 必须一致）：
--   lbl_title / lbl_status / lbl_toggle_target
--   btn_test_text / btn_test_display / btn_test_gettext / btn_test_color / btn_close
--   input_name / sld_volume / tog_enable   （可选）

local FPanelBaseUI = require "ui.FPanelBaseUI"
local FViewBaseUI  = require "ui.FViewBaseUI"
local FScrollList  = require "ui.FScrollList"
local FFixedList   = require "ui.FFixedList"

-- ── 列表 item 的 View（后端无关）────────────────────────────────────────
-- item 容器对象（GameObject / VisualElement）由列表代理供给，
-- 这里只负责把数据写进去——通过子类提供的 SetItemText 适配两端文本组件。
---@class FUITestItemView : FViewBaseUI
local FUITestItemView = FLua.Class(FViewBaseUI, "FUITestItemView")
do
    function FUITestItemView:__constructor()
        self.m_setItemText = nil   -- fun(itemObj, text)：由 panel 注入，适配后端
    end
    function FUITestItemView.new(setItemText)
        local o = FUITestItemView()
        o.m_setItemText = setItemText
        return o
    end
    -- 列表把数据刷到本项时调用（FScrollList:SetData / FFixedList:SetData）
    function FUITestItemView:SetData(data)
        if self.m_setItemText and self.m_viewObj then
            self.m_setItemText(self.m_viewObj, tostring(data))
        end
    end
end

---@class FPanelUITestBase : FPanelBaseUI
---@field MakeItemTemplate fun(self:FPanelUITestBase):any  子类(后端相关)提供：返回新 item 节点(GameObject/VisualElement)
---@field SetItemText fun(self:FPanelUITestBase, itemObj:any, text:string)  子类(后端相关)提供：把文本写到 item 节点
local FPanelUITestBase = FLua.Class(FPanelBaseUI, "FPanelUITestBase")
do
    function FPanelUITestBase:__constructor()
        self.m_toggleVisible = true   -- lbl_toggle_target 初始可见
        self.m_colorIndex = 0
        self.m_scrollList = nil
        self.m_fixedList = nil
    end

    --- 子类必须覆盖：返回资产路径（.uxml 或 .prefab）
    function FPanelUITestBase:GetResPath()
        error("FPanelUITestBase:GetResPath must be overridden", 2)
    end

    --- 子类可选覆盖：在 bridge 就绪后、绑定事件前构建 UI 内容。
    --- UITK 由 UXML 提供，无需覆盖；UGUI 子类在此运行时建树。
    function FPanelUITestBase:BuildContent()
    end

    -- ──────────────────────────────────────────────────────────────────
    -- OnCreate：面板加载完成，self.m_bridge 已注入（两端均为 IUIPanelBridge）
    -- ──────────────────────────────────────────────────────────────────
    function FPanelUITestBase:OnCreate()
        -- 先建树（UGUI），UITK 为空实现
        self:BuildContent()

        local bridge = self.m_bridge
        if not bridge then
            printError("[FPanelUITest] m_bridge is nil, backend not ready")
            return
        end

        -- ① 按钮精确绑定（两端通用）
        bridge:RegisterClick("btn_test_text",    function() self:TestSetText() end)
        bridge:RegisterClick("btn_test_display", function() self:TestSetDisplay() end)
        bridge:RegisterClick("btn_test_gettext", function() self:TestGetText() end)
        bridge:RegisterClick("btn_test_color",   function() self:TestSetColor() end)
        bridge:RegisterClick("btn_close",        function() self:DestroyPanel() end)

        -- ② 输入框（存在才测）：SetInputText / Submit / TextChange
        if bridge:Q("input_name") then
            bridge:SetInputText("input_name", "")
            bridge:RegisterSubmit("input_name", function(name, text)
                self:SetStatus("Submit[" .. name .. "] = " .. tostring(text))
            end)
            bridge:RegisterTextChange("input_name", function(name, text)
                self:SetStatus("Change[" .. name .. "] = " .. tostring(text))
            end)
        end

        -- ③ Slider（存在才测）：ValueChange
        if bridge:Q("sld_volume") then
            bridge:SetSliderValue("sld_volume", 0.5)
            bridge:RegisterValueChange("sld_volume", function(name, value)
                self:SetStatus(string.format("Slider[%s] = %.2f", name, value))
            end)
        end

        -- ④ Toggle（存在才测）：ValueChange
        if bridge:Q("tog_enable") then
            bridge:SetToggleValue("tog_enable", true)
            bridge:RegisterValueChange("tog_enable", function(name, value)
                self:SetStatus("Toggle[" .. name .. "] = " .. tostring(value))
            end)
        end

        -- ⑤ 列表（容器存在才测）：FScrollList + FFixedList
        self:SetupLists()

        self:SetStatus("OnCreate OK (" .. self:_BackendName() .. ") ✓")
        print("[FPanelUITest] OnCreate complete, backend =", self:_BackendName(), "bridge =", bridge)
    end

    -- ──────────────────────────────────────────────────────────────────
    -- 列表测试（FScrollList 虚拟滚动 + FFixedList 固定列表）
    -- 容器：list_scroll / list_fixed（UITK=ScrollView，UGUI=带 ScrollRect 的 GO）
    -- item 模板与文本写入由子类提供（后端相关）：
    --   self:MakeItemTemplate()        -> 返回新 item 节点（GameObject / VisualElement）
    --   self:SetItemText(itemObj, str) -> 把文本写到 item 节点上
    -- 子类未实现这两个钩子，或容器不存在时，自动跳过。
    -- ──────────────────────────────────────────────────────────────────
    function FPanelUITestBase:SetupLists()
        if not self.MakeItemTemplate or not self.SetItemText then
            return  -- 子类未提供后端相关的模板/写文本钩子
        end
        local bridge = self.m_bridge

        local function setItemText(itemObj, text)
            self:SetItemText(itemObj, text)
        end
        local opts = {
            templateFn = function() return self:MakeItemTemplate() end,
            itemHeight = 56,
        }

        -- FScrollList：虚拟滚动，item View 复用
        local scrollContainer = bridge and bridge:Q("list_scroll")
        if scrollContainer then
            local list = FScrollList.Create(
                function(itemObj, index) return FUITestItemView.new(setItemText) end,
                function(view, index) end,   -- 额外刷新钩子，这里数据走 SetData
                opts)
            self:AttachSubView(scrollContainer, list, true)
            local data = {}
            for i = 1, 200 do data[i] = "Scroll Item #" .. i end
            list:SetDataList(data)
            self.m_scrollList = list
        end

        -- FFixedList：每项一个常驻 View
        local fixedContainer = bridge and bridge:Q("list_fixed")
        if fixedContainer then
            local list = FFixedList.Create(
                function(index) return FUITestItemView.new(setItemText) end,
                opts)
            self:AttachSubView(fixedContainer, list, true)
            local data = {}
            for i = 1, 30 do data[i] = "Fixed Item #" .. i end
            list:SetDataList(data)
            self.m_fixedList = list
        end
    end

    -- ──────────────────────────────────────────────────────────────────
    -- 测试用例（全部走 bridge，后端无关）
    -- ──────────────────────────────────────────────────────────────────

    function FPanelUITestBase:TestSetText()
        local ts = tostring(os.clock()):sub(1, 6)
        self:SetStatus("SetText OK @ " .. ts)
        self.m_bridge:SetText("btn_test_text", "① SetText ✓")
    end

    function FPanelUITestBase:TestSetDisplay()
        self.m_toggleVisible = not self.m_toggleVisible
        self.m_bridge:SetDisplay("lbl_toggle_target", self.m_toggleVisible)
        self:SetStatus("SetDisplay → " .. (self.m_toggleVisible and "显示" or "隐藏"))
    end

    function FPanelUITestBase:TestGetText()
        local t = self.m_bridge:GetText("lbl_title")
        self:SetStatus("GetText(lbl_title) = [" .. tostring(t) .. "]")
    end

    function FPanelUITestBase:TestSetColor()
        local palette = {
            { 1.0, 0.4, 0.4, 1.0 },  -- 红
            { 0.4, 1.0, 0.5, 1.0 },  -- 绿
            { 0.4, 0.7, 1.0, 1.0 },  -- 蓝
            { 1.0, 0.85, 0.2, 1.0 }, -- 金
        }
        self.m_colorIndex = (self.m_colorIndex % #palette) + 1
        local c = palette[self.m_colorIndex]
        self.m_bridge:SetColor("lbl_status", c[1], c[2], c[3], c[4])
        self:SetStatus("SetColor #" .. self.m_colorIndex)
    end

    -- ──────────────────────────────────────────────────────────────────
    function FPanelUITestBase:SetStatus(text)
        if self.m_bridge then
            self.m_bridge:SetText("lbl_status", text)
        end
        print("[FPanelUITest] " .. text)
    end

    ---@return string
    function FPanelUITestBase:_BackendName()
        if self:IsUIToolkit() then return "UIToolkit" end
        if self:IsUGUI() then return "UGUI" end
        if self:IsFairyGui() then return "FairyGUI" end
        return "Unknown"
    end

    function FPanelUITestBase:OnDestroy()
        print("[FPanelUITest] OnDestroy (" .. self:_BackendName() .. ")")
    end
end

return FPanelUITestBase
