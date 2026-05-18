-- ui/FPanelLevelSelect.lua
-- 关卡选择面板
-- Prefab: UILevelSelect.prefab
--   Header/btn_back
--   Content/level_item_0  ← 模板，运行时克隆出所有关卡按钮
--     └── Label           ← 关卡名
--
-- 逻辑：
--   - 以 level_item_0 为模板，动态克隆生成每个关卡按钮
--   - 已解锁：可点击，显示关卡名 + 星级
--   - coming_soon：灰色 + "Coming" 文字，不可点击
--   - 未解锁：灰色 + 🔒，不可点击

local FPanelBaseUI = require "ui.FPanelBaseUI"
local LevelData    = require "game.level_data"
local SaveMgr      = require "game.save_manager"

local PREFAB_PATH = "Assets/DefaultPackage/UI/Prefab/UILevelSelect.prefab"

-- ── 布局常量 ─────────────────────────────────────────────────────────────
local COLS        = 3        -- 每行几个按钮
local ITEM_W      = 160      -- 按钮宽（像素）
local ITEM_H      = 160      -- 按钮高
local PAD_X       = 20       -- 水平间距
local PAD_Y       = 20       -- 垂直间距
local START_X     = -160     -- Content 左上角 x 偏移
local START_Y     = 0        -- Content 左上角 y 偏移（从上往下）

local l_instance = nil
---@class FPanelLevelSelect : FPanelBaseUI
local FPanelLevelSelect = FLua.Class(FPanelBaseUI, "FPanelLevelSelect")
do
    function FPanelLevelSelect.Instance()
        if not l_instance then l_instance = FPanelLevelSelect() end
        return l_instance
    end

    function FPanelLevelSelect:GetResPath() return PREFAB_PATH end

    function FPanelLevelSelect:OnCreate()
        local root    = self.m_panel.transform
        self.m_items  = {}

        -- btn_back
        local btnBack = root:Find("Header/btn_back")
        if btnBack then
            btnBack:GetComponent(typeof(UnityEngine.UI.Button)).onClick:AddListener(function()
                self:DestroyPanel()
            end)
        end

        -- 模板（level_item_0）
        local content  = root:Find("Content")
        local template = content and content:Find("level_item_0")
        if not template then
            print("[LevelSelect] ERROR: level_item_0 template not found")
            return
        end
        -- 隐藏模板（我们自己克隆）
        template.gameObject:SetActive(false)

        -- 动态生成所有关卡按钮
        local levels = LevelData.all_levels()
        for i, lv in ipairs(levels) do
            local col = (i - 1) % COLS
            local row = math.floor((i - 1) / COLS)

            -- 克隆模板
            local go = UnityEngine.Object.Instantiate(
                template.gameObject, content)
            go.name = "level_item_" .. lv.id
            go:SetActive(true)

            -- 定位
            local rt = go:GetComponent(typeof(UnityEngine.RectTransform))
            if rt then
                rt.anchoredPosition = UnityEngine.Vector2(
                    START_X + col * (ITEM_W + PAD_X),
                    START_Y - row * (ITEM_H + PAD_Y))
            end

            -- Label 文字
            local lbl = go.transform:Find("Label")
            if lbl then
                local tmp = lbl:GetComponent(typeof(TMPro.TMP_Text))
                if tmp then
                    if lv.coming_soon then
                        tmp.text = lv.name .. "\n<size=60%>Coming</size>"
                    elseif not SaveMgr.is_unlocked(lv.id) then
                        local stars = SaveMgr.get_stars(lv.id)
                        local starsStr = stars > 0 and string.rep("★", stars) .. string.rep("☆", 3 - stars) or "🔒"
                        tmp.text = lv.name .. "\n<size=60%>" .. starsStr .. "</size>"
                    else
                        tmp.text = lv.name .. "\n<size=60%>☆☆☆</size>"
                    end
                end
            end

            -- 按钮颜色和点击
            local btn = go:GetComponent(typeof(UnityEngine.UI.Button))
            if btn then
                local canPlay = not lv.coming_soon and SaveMgr.is_unlocked(lv.id)
                -- 已解锁：正常颜色；否则半透明灰色
                local img = go:GetComponent(typeof(UnityEngine.UI.Image))
                if img then
                    img.color = canPlay
                        and UnityEngine.Color(1, 1, 1, 1)
                        or  UnityEngine.Color(0.5, 0.5, 0.5, 0.7)
                end

                if canPlay then
                    local levelId = lv.id
                    btn.onClick:AddListener(function()
                        self:OnClickLevel(levelId)
                    end)
                else
                    -- 禁用按钮交互
                    btn.interactable = false
                end
            end

            table.insert(self.m_items, go)
        end

        -- 调整 Content 高度适配按钮数
        if content then
            local rows   = math.ceil(#levels / COLS)
            local totalH = rows * (ITEM_H + PAD_Y) + PAD_Y
            local rt = content:GetComponent(typeof(UnityEngine.RectTransform))
            if rt then
                rt.sizeDelta = UnityEngine.Vector2(rt.sizeDelta.x, totalH)
            end
        end
    end

    function FPanelLevelSelect:OnClickLevel(levelId)
        print("[LevelSelect] level: " .. levelId)
        self:DestroyPanel()
        require "ui.FPanelMainMenu".Instance():DestroyPanel()
        require "game.level_manager".load_level(levelId)
    end

    function FPanelLevelSelect:OnDestroy()
        self.m_items = nil
        l_instance = nil
    end
end

return FPanelLevelSelect
