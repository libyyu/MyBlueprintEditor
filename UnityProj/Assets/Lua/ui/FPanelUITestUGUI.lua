-- ui/FPanelUITestUGUI.lua
-- UI 框架测试面板 —— UGUI 后端
-- 资产：Assets/DefaultPackage/UI/Prefab/UITest.prefab（最小根节点，内容运行时建树）
--
-- 全部测试逻辑在 FPanelUITestBase 中（后端无关，走 self.m_bridge）。
-- 本类只负责 BuildContent()：在面板根节点下用代码构建 label + button，
-- 子节点 name 与 UITK 的 UXML 元素 name 对齐，于是同一套 bridge 调用两端通用。
--
-- 运行时构建：label / button / 输入框 / 滑块 / 开关 / 列表(ScrollRect) 容器，
-- 子节点 name 与 UITK 的 UXML 元素 name 对齐，于是同一套 bridge 调用两端通用：
--   input_name(TMP_InputField) / sld_volume(Slider) / tog_enable(Toggle)
--   list_scroll(虚拟滚动列表容器) / list_fixed(固定列表容器)

local FPanelUITestBase = require "ui.FPanelUITestBase"

local PREFAB_PATH = "Assets/DefaultPackage/UI/Prefab/UITest.prefab"

local Color         = CS.UnityEngine.Color
local RectTransform = CS.UnityEngine.RectTransform
local Image         = CS.UnityEngine.UI.Image
local Button        = CS.UnityEngine.UI.Button
local Slider        = CS.UnityEngine.UI.Slider
local Toggle        = CS.UnityEngine.UI.Toggle
local ScrollRect    = CS.UnityEngine.UI.ScrollRect
local Mask          = CS.UnityEngine.UI.Mask
local InputField    = CS.TMPro.TMP_InputField
local TMP           = CS.TMPro.TextMeshProUGUI
local TMPAlign      = CS.TMPro.TextAlignmentOptions

---@type FPanelUITestUGUI
local l_instance = nil
---@class FPanelUITestUGUI : FPanelUITestBase
local FPanelUITestUGUI = FLua.Class(FPanelUITestBase, "FPanelUITestUGUI")
do
    ---@return FPanelUITestUGUI
    function FPanelUITestUGUI.Instance()
        if not l_instance then l_instance = FPanelUITestUGUI() end
        return l_instance
    end

    function FPanelUITestUGUI:GetResPath() return PREFAB_PATH end

    -- ── 运行时建树工具 ─────────────────────────────────────────────────

    --- 新建一个带 RectTransform 的 UI 节点（AddComponent<Image> 会自动带上 RectTransform）
    ---@return GameObject, RectTransform
    local function newNode(name, parent)
        local go = NewGameObject(name)
        go.layer = UnityEngine.LayerMask.NameToLayer("UI")
        local rt = go:AddComponent(typeof(RectTransform))
        rt:SetParent(parent, false)
        rt.localScale = Vector3(1, 1, 1)
        return go, rt
    end

    --- 锚到顶部居中，自上而下用 y(负值) 布局
    local function anchorTopCenter(rt, w, h, y)
        rt.anchorMin = Vector2(0.5, 1)
        rt.anchorMax = Vector2(0.5, 1)
        rt.pivot = Vector2(0.5, 1)
        rt.sizeDelta = Vector2(w, h)
        rt.anchoredPosition = Vector2(0, y)
    end

    local function mkLabel(parent, name, text, y, fontSize, col)
        local go, rt = newNode(name, parent)
        anchorTopCenter(rt, 600, 60, y)
        local tmp = go:AddComponent(typeof(TMP))
        tmp.text = text
        tmp.fontSize = fontSize or 28
        tmp.alignment = TMPAlign.Center
        tmp.color = col or Color(1, 1, 1, 1)
        return go
    end

    local function mkButton(parent, name, text, y)
        local go, rt = newNode(name, parent)
        anchorTopCenter(rt, 560, 80, y)
        local img = go:AddComponent(typeof(Image))
        img.color = Color(0.22, 0.28, 0.42, 1)
        go:AddComponent(typeof(Button))
        -- 文本子节点（bridge:SetText 会用 GetComponentInChildren<TMP_Text> 找到它）
        local lblGo, lblRt = newNode("label", go.transform)
        lblRt.anchorMin = Vector2(0, 0)
        lblRt.anchorMax = Vector2(1, 1)
        lblRt.offsetMin = Vector2(0, 0)
        lblRt.offsetMax = Vector2(0, 0)
        local tmp = lblGo:AddComponent(typeof(TMP))
        tmp.text = text
        tmp.fontSize = 28
        tmp.alignment = TMPAlign.Center
        tmp.color = Color(1, 1, 1, 1)
        return go
    end

    -- 子节点：相对父节点拉伸或定位的小块
    local function mkChild(parent, name, anchorMin, anchorMax, offMin, offMax)
        local go, rt = newNode(name, parent)
        rt.anchorMin = anchorMin
        rt.anchorMax = anchorMax
        rt.offsetMin = offMin
        rt.offsetMax = offMax
        return go, rt
    end

    --- TMP_InputField（含 Text Area / Text / Placeholder）
    local function mkInputField(parent, name, y)
        local go, rt = newNode(name, parent)
        anchorTopCenter(rt, 560, 56, y)
        local img = go:AddComponent(typeof(Image))
        img.color = Color(0.95, 0.95, 0.97, 1)
        local field = go:AddComponent(typeof(InputField))

        -- Text Area（viewport，带 RectMask2D 由 InputField 自动处理裁剪）
        local areaGo, _ = mkChild(go.transform, "TextArea",
            Vector2(0, 0), Vector2(1, 1), Vector2(10, 6), Vector2(-10, -6))
        areaGo:AddComponent(typeof(CS.UnityEngine.UI.RectMask2D))

        -- Placeholder
        local phGo = mkChild(areaGo.transform, "Placeholder",
            Vector2(0, 0), Vector2(1, 1), Vector2(0, 0), Vector2(0, 0))
        local ph = phGo:AddComponent(typeof(TMP))
        ph.text = "请输入..."
        ph.fontSize = 24
        ph.color = Color(0.5, 0.5, 0.5, 1)
        ph.alignment = TMPAlign.MidlineLeft

        -- Text
        local txtGo = mkChild(areaGo.transform, "Text",
            Vector2(0, 0), Vector2(1, 1), Vector2(0, 0), Vector2(0, 0))
        local txt = txtGo:AddComponent(typeof(TMP))
        txt.fontSize = 24
        txt.color = Color(0.1, 0.1, 0.1, 1)
        txt.alignment = TMPAlign.MidlineLeft

        -- 接线（TMP_InputField 需要 textViewport / textComponent / placeholder）
        field.textViewport = areaGo:GetComponent(typeof(RectTransform))
        field.textComponent = txt
        field.placeholder = ph
        field.targetGraphic = img
        return go
    end

    --- Slider（Background / Fill Area / Fill / Handle Slide Area / Handle）
    local function mkSlider(parent, name, y)
        local go, rt = newNode(name, parent)
        anchorTopCenter(rt, 560, 36, y)
        local slider = go:AddComponent(typeof(Slider))

        -- Background
        local bgGo = mkChild(go.transform, "Background",
            Vector2(0, 0.25), Vector2(1, 0.75), Vector2(0, 0), Vector2(0, 0))
        local bgImg = bgGo:AddComponent(typeof(Image))
        bgImg.color = Color(0.3, 0.3, 0.34, 1)

        -- Fill Area / Fill
        local fillAreaGo = mkChild(go.transform, "Fill Area",
            Vector2(0, 0.25), Vector2(1, 0.75), Vector2(8, 0), Vector2(-8, 0))
        local fillGo, fillRt = mkChild(fillAreaGo.transform, "Fill",
            Vector2(0, 0), Vector2(0, 1), Vector2(0, 0), Vector2(10, 0))
        local fillImg = fillGo:AddComponent(typeof(Image))
        fillImg.color = Color(0.4, 0.7, 1, 1)

        -- Handle Slide Area / Handle
        local handleAreaGo = mkChild(go.transform, "Handle Slide Area",
            Vector2(0, 0), Vector2(1, 1), Vector2(8, 0), Vector2(-8, 0))
        local handleGo, handleRt = mkChild(handleAreaGo.transform, "Handle",
            Vector2(0, 0), Vector2(0, 1), Vector2(-10, 0), Vector2(10, 0))
        local handleImg = handleGo:AddComponent(typeof(Image))
        handleImg.color = Color(1, 1, 1, 1)

        slider.fillRect = fillRt
        slider.handleRect = handleRt
        slider.targetGraphic = handleImg
        slider.direction = CS.UnityEngine.UI.Slider.Direction.LeftToRight
        slider.minValue = 0
        slider.maxValue = 1
        return go
    end

    --- Toggle（Background / Checkmark + 文本）
    local function mkToggle(parent, name, label, y)
        local go, rt = newNode(name, parent)
        anchorTopCenter(rt, 560, 44, y)
        local toggle = go:AddComponent(typeof(Toggle))

        -- Background（左侧方块）
        local bgGo, bgRt = mkChild(go.transform, "Background",
            Vector2(0, 0.5), Vector2(0, 0.5), Vector2(0, 0), Vector2(0, 0))
        bgRt.sizeDelta = Vector2(36, 36)
        bgRt.anchoredPosition = Vector2(18, 0)
        local bgImg = bgGo:AddComponent(typeof(Image))
        bgImg.color = Color(0.85, 0.85, 0.9, 1)

        -- Checkmark
        local ckGo = mkChild(bgGo.transform, "Checkmark",
            Vector2(0, 0), Vector2(1, 1), Vector2(4, 4), Vector2(-4, -4))
        local ckImg = ckGo:AddComponent(typeof(Image))
        ckImg.color = Color(0.2, 0.6, 1, 1)

        -- Label
        local lblGo = mkChild(go.transform, "Label",
            Vector2(0, 0), Vector2(1, 1), Vector2(48, 0), Vector2(0, 0))
        local lblTmp = lblGo:AddComponent(typeof(TMP))
        lblTmp.text = label or "开关"
        lblTmp.fontSize = 24
        lblTmp.alignment = TMPAlign.MidlineLeft
        lblTmp.color = Color(1, 1, 1, 1)

        toggle.targetGraphic = bgImg
        toggle.graphic = ckImg
        return go
    end

    --- ScrollRect 列表容器（带 Viewport + Content）。返回容器 GameObject。
    --- FScrollList/FFixedList 内部按 GetBackendType()=UGUI 走 ScrollRect 虚拟列表代理。
    local function mkScrollContainer(parent, name, y, h)
        local go, rt = newNode(name, parent)
        anchorTopCenter(rt, 560, h, y)
        local outBg = go:AddComponent(typeof(Image))
        outBg.color = Color(0.05, 0.05, 0.08, 0.6)
        local sr = go:AddComponent(typeof(ScrollRect))
        sr.horizontal = false
        sr.vertical = true

        -- Viewport（裁剪）
        local vpGo, vpRt = mkChild(go.transform, "Viewport",
            Vector2(0, 0), Vector2(1, 1), Vector2(2, 2), Vector2(-2, -2))
        vpGo:AddComponent(typeof(Image)).color = Color(1, 1, 1, 0.02)
        local mask = vpGo:AddComponent(typeof(Mask))
        mask.showMaskGraphic = false

        -- Content（FViewListRootProxy(UGUI) 会把它当滚动内容，按需建 item）
        local _, contentRt = mkChild(vpGo.transform, "Content",
            Vector2(0, 1), Vector2(1, 1), Vector2(0, 0), Vector2(0, 0))
        contentRt.pivot = Vector2(0.5, 1)
        contentRt.sizeDelta = Vector2(0, 0)

        sr.viewport = vpRt
        sr.content = contentRt
        return go
    end

    -- ── 运行时建树（覆盖基类钩子）──────────────────────────────────────
    function FPanelUITestUGUI:BuildContent()
        local rootGo = self.m_panel and self.m_panel.RootGameObject
        if not IsValidObject(rootGo) then
            printError("[FPanelUITestUGUI] RootGameObject invalid, cannot build content")
            return
        end
        local root = rootGo.transform

        -- 背景面板（居中，左右两栏）
        local panelGo, panelRt = newNode("panel_bg", root)
        panelRt.anchorMin = Vector2(0.5, 0.5)
        panelRt.anchorMax = Vector2(0.5, 0.5)
        panelRt.pivot = Vector2(0.5, 0.5)
        panelRt.sizeDelta = Vector2(1240, 940)
        panelRt.anchoredPosition = Vector2(0, 0)
        local bg = panelGo:AddComponent(typeof(Image))
        bg.color = Color(0.08, 0.08, 0.12, 0.92)
        local panel = panelGo.transform

        -- 标题 / 状态（顶部通栏）
        mkLabel(panel, "lbl_title",  "UGUI Test Panel", -24, 30, Color(1, 1, 1, 1))
        mkLabel(panel, "lbl_status", "status: ready",   -78, 20, Color(0.7, 0.86, 0.7, 1))

        -- ── 左栏：按钮 + 表单 ──────────────────────────────────────────
        local leftGo, leftRt = newNode("col_left", panel)
        leftRt.anchorMin = Vector2(0, 1); leftRt.anchorMax = Vector2(0, 1)
        leftRt.pivot = Vector2(0, 1)
        leftRt.sizeDelta = Vector2(600, 820)
        leftRt.anchoredPosition = Vector2(30, -120)
        local left = leftGo.transform

        mkButton(left, "btn_test_text",    "① SetText 测试",   -10)
        mkButton(left, "btn_test_display", "② SetDisplay 切换", -90)
        mkButton(left, "btn_test_gettext", "③ GetText 读取",    -170)
        mkButton(left, "btn_test_color",   "④ SetColor 测试",   -250)

        mkInputField(left, "input_name", -330)
        mkSlider(left, "sld_volume", -400)
        mkToggle(left, "tog_enable", "启用开关", -450)

        mkLabel(left, "lbl_toggle_target", "★ 这个文字可被隐藏 ★", -510, 24, Color(1, 1, 0.2, 1))

        local closeGo = mkButton(left, "btn_close", "关闭", -580)
        closeGo:GetComponent(typeof(Image)).color = Color(0.7, 0.24, 0.24, 1)

        -- ── 右栏：两个列表 ─────────────────────────────────────────────
        local rightGo, rightRt = newNode("col_right", panel)
        rightRt.anchorMin = Vector2(1, 1); rightRt.anchorMax = Vector2(1, 1)
        rightRt.pivot = Vector2(1, 1)
        rightRt.sizeDelta = Vector2(580, 820)
        rightRt.anchoredPosition = Vector2(-30, -120)
        local right = rightGo.transform

        mkLabel(right, "lbl_scroll_title", "FScrollList (虚拟滚动)", -10, 22, Color(0.8, 0.9, 1, 1))
        mkScrollContainer(right, "list_scroll", -50, 340)

        mkLabel(right, "lbl_fixed_title", "FFixedList (固定列表)", -410, 22, Color(0.8, 1, 0.85, 1))
        mkScrollContainer(right, "list_fixed", -450, 340)
    end

    -- ── 列表 item 钩子（基类 SetupLists 调用，后端相关）─────────────────

    --- 创建一个列表 item 节点（GameObject）。代理会自行设置 RectTransform 定位/高度。
    ---@return GameObject
    function FPanelUITestUGUI:MakeItemTemplate()
        local go = NewGameObject("item")
        go.layer = UnityEngine.LayerMask.NameToLayer("UI")
        go:AddComponent(typeof(RectTransform))
        local img = go:AddComponent(typeof(Image))
        img.color = Color(0.16, 0.18, 0.26, 1)
        -- 文本子节点
        local lblGo, lblRt = newNode("item_label", go.transform)
        lblRt.anchorMin = Vector2(0, 0)
        lblRt.anchorMax = Vector2(1, 1)
        lblRt.offsetMin = Vector2(12, 0)
        lblRt.offsetMax = Vector2(-12, 0)
        local tmp = lblGo:AddComponent(typeof(TMP))
        tmp.text = ""
        tmp.fontSize = 22
        tmp.alignment = TMPAlign.MidlineLeft
        tmp.color = Color(1, 1, 1, 1)
        return go
    end

    --- 把文本写到 item 节点（找子节点上的 TMP）
    function FPanelUITestUGUI:SetItemText(itemObj, text)
        if not IsValidObject(itemObj) then return end
        local tmp = itemObj:GetComponentInChildren(typeof(TMP))
        if tmp then tmp.text = text end
    end

    function FPanelUITestUGUI:OnDestroy()
        FPanelUITestBase.OnDestroy(self)
        l_instance = nil
    end
end

return FPanelUITestUGUI
