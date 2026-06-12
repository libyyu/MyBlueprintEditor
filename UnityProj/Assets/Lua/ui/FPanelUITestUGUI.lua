-- ui/FPanelUITestUGUI.lua
-- UI 框架测试面板 —— UGUI 后端
-- 资产：Assets/DefaultPackage/UI/Prefab/UITest.prefab（最小根节点，内容运行时建树）
--
-- 全部测试逻辑在 FPanelUITestBase 中（后端无关，走 self.m_bridge）。
-- 本类只负责 BuildContent()：在面板根节点下用代码构建 label + button，
-- 子节点 name 与 UITK 的 UXML 元素 name 对齐，于是同一套 bridge 调用两端通用。
--
-- 说明：input_name / sld_volume / tog_enable 这类控件运行时构建较繁琐，UGUI 版暂未建，
--       基类会自动跳过对应测试；如需在 UGUI 端测 P3 表单事件，建议改用美术 prefab。

local FPanelUITestBase = require "ui.FPanelUITestBase"

local PREFAB_PATH = "Assets/DefaultPackage/UI/Prefab/UITest.prefab"

local Color         = CS.UnityEngine.Color
local RectTransform = CS.UnityEngine.RectTransform
local Image         = CS.UnityEngine.UI.Image
local Button        = CS.UnityEngine.UI.Button
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

    -- ── 运行时建树（覆盖基类钩子）──────────────────────────────────────
    function FPanelUITestUGUI:BuildContent()
        local rootGo = self.m_panel and self.m_panel.RootGameObject
        if not IsValidObject(rootGo) then
            printError("[FPanelUITestUGUI] RootGameObject invalid, cannot build content")
            return
        end
        local root = rootGo.transform

        -- 背景面板（居中）
        local panelGo, panelRt = newNode("panel_bg", root)
        panelRt.anchorMin = Vector2(0.5, 0.5)
        panelRt.anchorMax = Vector2(0.5, 0.5)
        panelRt.pivot = Vector2(0.5, 0.5)
        panelRt.sizeDelta = Vector2(680, 760)
        panelRt.anchoredPosition = Vector2(0, 0)
        local bg = panelGo:AddComponent(typeof(Image))
        bg.color = Color(0.08, 0.08, 0.12, 0.92)
        local panel = panelGo.transform

        -- 标题 / 状态
        mkLabel(panel, "lbl_title",  "UGUI Test Panel", -40, 32, Color(1, 1, 1, 1))
        mkLabel(panel, "lbl_status", "status: ready",   -110, 22, Color(0.7, 0.86, 0.7, 1))

        -- 按钮组
        mkButton(panel, "btn_test_text",    "① SetText 测试",   -180)
        mkButton(panel, "btn_test_display", "② SetDisplay 切换", -276)
        mkButton(panel, "btn_test_gettext", "③ GetText 读取",    -372)
        mkButton(panel, "btn_test_color",   "④ SetColor 测试",   -468)

        -- 可切换显隐的测试元素
        mkLabel(panel, "lbl_toggle_target", "★ 这个文字可被隐藏 ★", -560, 26, Color(1, 1, 0.2, 1))

        -- 关闭
        local closeGo = mkButton(panel, "btn_close", "关闭", -660)
        closeGo:GetComponent(typeof(Image)).color = Color(0.7, 0.24, 0.24, 1)
    end

    function FPanelUITestUGUI:OnDestroy()
        FPanelUITestBase.OnDestroy(self)
        l_instance = nil
    end
end

return FPanelUITestUGUI
