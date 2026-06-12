---ECViewList的根代理，用于设置数量，获取数量等, ECViewListRootProxy为对外的接口类，ECViewListRootProxyImplXXX为实现
---
--- 后端无关的列表抽象层。三套实现：
---   FViewListRootProxyImplForFairyGUIList     —— FairyGUI GList（虚拟列表，item 由 GList 池管理）
---   FViewListRootProxyImplForUITKScrollView   —— UI Toolkit ScrollView（虚拟列表，templateFn 建 VisualElement）
---   FViewListRootProxyImplForUGUIScrollView   —— UGUI ScrollRect（虚拟列表，templateFn 建 GameObject）
---
--- 统一接口：SetRootWidget -> SetItemUpdateFunc -> OnCreate -> SetCount / GetItemByIndex / Refresh -> OnDestroy
---
--- FFixedList / FScrollList 通过 FViewListRootProxy.CreateByBackend(panel:GetBackendType(), opts)
--- 创建对应后端的代理，自身不再关心后端差异。
---
--- opts（UGUI / UIToolkit 后端用，FairyGUI 忽略）：
---   { templateFn = function() return GameObject|VisualElement end,  -- item 节点模板（必填）
---     itemHeight = number }                                          -- 行高(px)，可选；不填则尝试自动测量

--********************代理实现基类***************--

local FViewListRootProxyImplBase = FLua.Abstract("FViewListRootProxyImplBase")
do
    function FViewListRootProxyImplBase:__constructor()
        self.m_RootWidget = nil
        self.m_ItemUpdateFunc = nil
    end

    function FViewListRootProxyImplBase:OnCreate()
    end

    function FViewListRootProxyImplBase:OnDestroy()
    end

    function FViewListRootProxyImplBase:SetCount(count)
    end

    function FViewListRootProxyImplBase:SetItemUpdateFunc(itemUpdateFunc)
        self.m_ItemUpdateFunc = itemUpdateFunc
    end

    function FViewListRootProxyImplBase:GetCount()
        return 0
    end

    --- 数据变化但数量不变时，强制刷新可见 item 内容（默认空实现）
    function FViewListRootProxyImplBase:Refresh()
    end

    function FViewListRootProxyImplBase:GetItemByIndex(index)
        return nil
    end
    function FViewListRootProxyImplBase:SetRootWidget(widget)
        self.m_RootWidget = widget
    end

    function FViewListRootProxyImplBase:GetRootWidget()
        return self.m_RootWidget
    end

    function FViewListRootProxyImplBase:IsValid()
        return IsValidObject(self.m_RootWidget)
    end
end
-------------------------------------------------------------





--************默认代理实现, 使用List控件创建的ViewList**********--

local FViewListRootProxyImplForFairyGUIList = FLua.Class(FViewListRootProxyImplBase, "FViewListRootProxyImplForFairyGUIList")
do
    function FViewListRootProxyImplForFairyGUIList:OnCreate()
        assert(self.m_RootWidget:IsExtend("FairyGUI.GList"), "FViewListRootProxyImplForFairyGUIList requires a GList as root widget")
        -- 设为虚拟列表（GList 会按需创建/复用 item，配合 itemRenderer 回填内容）
        self.m_RootWidget:SetVirtual()
        self.m_RootWidget.itemRenderer = function(index, go)
            if self.m_ItemUpdateFunc then
                self.m_ItemUpdateFunc(go, index+1)
            end
        end
    end

    function FViewListRootProxyImplForFairyGUIList:OnDestroy()
        if self.m_RootWidget then
            self.m_RootWidget.itemRenderer = nil
        end

        self.m_RootWidget = nil
    end

    --- 数据变化但数量不变时，强制刷新可见 item 内容
    function FViewListRootProxyImplForFairyGUIList:Refresh()
        if not self:IsValid() then return end
        self.m_RootWidget:RefreshVirtualList()
    end

    function FViewListRootProxyImplForFairyGUIList:SetCount(count)
        if not self:IsValid() then
            return
        end
        self.m_RootWidget.numItems = count
    end

    function FViewListRootProxyImplForFairyGUIList:GetCount()
        if not self:IsValid() then
            return 0
        end
        return self.m_RootWidget.numItems
    end


    function FViewListRootProxyImplForFairyGUIList:GetItemByIndex(index)
        if not self:IsValid() then
            return nil
        end
        local childIndex = self.m_RootWidget:ItemIndexToChildIndex(index - 1)
        return self.m_RootWidget:GetChildAt(childIndex)
    end
end
--------------------------------------------------------------

--***********UIToolkit ScrollView 虚拟滚动代理实现**************--
-- 虚拟列表：只创建可见区域 + 缓冲区数量的节点，滚动时复用。
-- item 高度自动从模板节点的 resolvedStyle.height 获取（由设计师在 UXML/USS 中指定）。
-- 使用方式：
--   UXML 里放一个 name="list_container" 的 ScrollView
--   在 FPanelXxx:OnCreate 里：
--     local listRoot = FViewListRootProxy.CreateProxyUITK(bridge, "list_container", templateCreatorFn)
--     listRoot:SetCount(1000)      -- 设置数据总数
--   templateCreatorFn() 返回一个新建的 VisualElement 作为列表项模板
--   item 高度由模板节点样式决定（USS 中定义 height），无需代码指定

local FViewListRootProxyImplForUITKScrollView = FLua.Class(FViewListRootProxyImplBase, "FViewListRootProxyImplForUITKScrollView")
do
    local DisplayStyle = CS.UnityEngine.UIElements.DisplayStyle
    local Position = CS.UnityEngine.UIElements.Position
    local LengthUnit = CS.UnityEngine.UIElements.LengthUnit
    local Length = CS.UnityEngine.UIElements.Length

    -- m_RootWidget   : ScrollView (VisualElement)
    -- m_templateFn   : function() -> VisualElement
    -- m_count        : 逻辑总数据量
    -- m_itemHeight   : 每个 item 的固定高度(px)
    -- m_pool         : {VisualElement, ...} 对象池（已创建的可复用节点）
    -- m_activeItems  : {[poolIndex] = dataIndex, ...} 当前正在显示的映射
    -- m_container    : 实际放置子节点的容器
    -- m_spacer       : 占位元素，撑出总高度
    -- m_viewport     : ScrollView 的 viewport 高度
    -- m_bufferCount  : 上下缓冲区额外节点数
    -- m_visibleStart : 当前可见区域起始数据索引（1-based）
    -- m_visibleEnd   : 当前可见区域结束数据索引（1-based）
    -- m_scrollView   : ScrollView 引用（用于注册滚动事件）

    function FViewListRootProxyImplForUITKScrollView:__constructor()
        self.m_templateFn = nil
        self.m_count = 0
        self.m_itemHeight = 0       -- 从模板节点自动测量
        self.m_itemHeightResolved = false
        self.m_pool = {}
        self.m_activeItems = {}     -- poolIndex -> dataIndex
        self.m_dataToPool = {}      -- dataIndex -> poolIndex
        self.m_container = nil
        self.m_spacer = nil
        self.m_viewport = 0
        self.m_bufferCount = 2
        self.m_visibleStart = 0
        self.m_visibleEnd = 0
        self.m_scrollView = nil
        self.m_scrollCallback = nil
        self.m_contentWrapper = nil
        self.m_measureItem = nil    -- 用于测量高度的隐藏模板节点
    end

    ---@param templateFn fun():VisualElement
    function FViewListRootProxyImplForUITKScrollView:SetTemplateFn(templateFn)
        self.m_templateFn = templateFn
    end

    ---手动覆盖 item 高度（通常不需要，高度自动从模板样式获取）
    ---@param height number
    function FViewListRootProxyImplForUITKScrollView:SetItemHeight(height)
        self.m_itemHeight = height
        self.m_itemHeightResolved = true
    end

    function FViewListRootProxyImplForUITKScrollView:OnCreate()
        local ve = self.m_RootWidget
        self.m_scrollView = ve

        -- 取 contentContainer 作为实际容器
        if ve.contentContainer then
            self.m_container = ve.contentContainer
        else
            self.m_container = ve
        end

        -- 创建内容包装器
        self.m_contentWrapper = CS.UnityEngine.UIElements.VisualElement()
        self.m_contentWrapper.style.position = Position.Relative
        self.m_contentWrapper.style.width = Length(100, LengthUnit.Percent)
        self.m_container:Add(self.m_contentWrapper)

        -- 创建占位 spacer 撑出总高度
        self.m_spacer = CS.UnityEngine.UIElements.VisualElement()
        self.m_spacer.name = "__virtual_list_spacer"
        self.m_spacer.style.width = Length(100, LengthUnit.Percent)
        self.m_spacer.style.height = Length(0, LengthUnit.Pixel)
        self.m_contentWrapper:Add(self.m_spacer)

        -- 自动测量 item 高度：创建一个模板节点加入 DOM，等布局完成后读取高度
        self:_ResolveMeasureHeight()

        -- 监听滚动事件
        self.m_scrollCallback = function(_evt)
            self:_OnScroll()
        end
        if self.m_scrollView.verticalScroller then
            self.m_scrollView.verticalScroller.valueChanged:AddCallback(self.m_scrollCallback)
        end
    end

    --- 自动测量模板节点高度
    function FViewListRootProxyImplForUITKScrollView:_ResolveMeasureHeight()
        if self.m_itemHeightResolved then return end

        -- 创建一个测量用的模板节点
        local measureItem
        if self.m_templateFn then
            measureItem = self.m_templateFn()
        else
            measureItem = CS.UnityEngine.UIElements.VisualElement()
        end
        -- 加入容器让布局系统计算高度，但对用户不可见
        measureItem.style.position = Position.Absolute
        measureItem.style.left = Length(-9999, LengthUnit.Pixel)
        measureItem.style.visibility = CS.UnityEngine.UIElements.Visibility.Hidden
        self.m_contentWrapper:Add(measureItem)
        self.m_measureItem = measureItem

        -- 注册 GeometryChangedEvent 回调，布局完成后读取实际高度
        measureItem:RegisterCallback(
            CS.UnityEngine.UIElements.GeometryChangedEvent,
            function(_geoEvt)
                local h = measureItem.resolvedStyle.height
                if h and h > 0 then
                    self.m_itemHeight = h
                    self.m_itemHeightResolved = true
                else
                    -- fallback: 尝试从样式中读取
                    h = measureItem.style.height.value
                        and measureItem.style.height.value.value or 40
                    self.m_itemHeight = h > 0 and h or 40
                    self.m_itemHeightResolved = true
                end
                -- 移除测量节点
                self.m_contentWrapper:Remove(measureItem)
                self.m_measureItem = nil
                -- 如果已经有数据了，触发重新布局
                if self.m_count > 0 then
                    local totalHeight = self.m_count * self.m_itemHeight
                    self.m_spacer.style.height = Length(totalHeight, LengthUnit.Pixel)
                    self:_OnScroll()
                end
            end
        )
    end

    function FViewListRootProxyImplForUITKScrollView:OnDestroy()
        -- 取消滚动监听
        if self.m_scrollView and self.m_scrollView.verticalScroller and self.m_scrollCallback then
            self.m_scrollView.verticalScroller.valueChanged:RemoveCallback(self.m_scrollCallback)
        end
        self.m_scrollCallback = nil

        -- 清空节点
        if self.m_container then
            self.m_container:Clear()
        end
        self.m_pool = {}
        self.m_activeItems = {}
        self.m_dataToPool = {}
        self.m_count = 0
        self.m_RootWidget = nil
        self.m_container = nil
        self.m_spacer = nil
        self.m_contentWrapper = nil
        self.m_scrollView = nil
    end

    function FViewListRootProxyImplForUITKScrollView:SetCount(count)
        if not self:IsValid() then return end
        self.m_count = count

        -- 如果高度还没测量完成，先记录 count，等 _ResolveMeasureHeight 回调后会自动布局
        if not self.m_itemHeightResolved then
            return
        end

        -- 更新 spacer 总高度
        local totalHeight = count * self.m_itemHeight
        self.m_spacer.style.height = Length(totalHeight, LengthUnit.Pixel)

        -- 重新计算可见区域
        self.m_visibleStart = 0
        self.m_visibleEnd = 0
        self:_OnScroll()
    end

    function FViewListRootProxyImplForUITKScrollView:GetCount()
        return self.m_count
    end

    function FViewListRootProxyImplForUITKScrollView:GetItemByIndex(index)
        local poolIdx = self.m_dataToPool[index]
        if poolIdx then
            return self.m_pool[poolIdx]
        end
        return nil
    end

    function FViewListRootProxyImplForUITKScrollView:IsValid()
        return self.m_RootWidget ~= nil
    end

    --- 滚动回调：计算可见范围，复用节点
    function FViewListRootProxyImplForUITKScrollView:_OnScroll()
        if not self:IsValid() or self.m_count == 0 then
            self:_HideAll()
            return
        end

        -- 获取当前滚动偏移和视口高度
        local scrollOffset = 0
        if self.m_scrollView.verticalScroller then
            scrollOffset = self.m_scrollView.verticalScroller.value or 0
        end

        local viewportHeight = self.m_scrollView.contentViewport
            and self.m_scrollView.contentViewport.resolvedStyle.height
            or self.m_scrollView.resolvedStyle.height
            or 600

        if viewportHeight <= 0 then viewportHeight = 600 end
        self.m_viewport = viewportHeight

        -- 计算可见数据索引范围（1-based）
        local startIdx = math.floor(scrollOffset / self.m_itemHeight) + 1
        local endIdx = math.ceil((scrollOffset + viewportHeight) / self.m_itemHeight)

        -- 加缓冲区
        startIdx = math.max(1, startIdx - self.m_bufferCount)
        endIdx = math.min(self.m_count, endIdx + self.m_bufferCount)

        -- 如果范围没变，跳过
        if startIdx == self.m_visibleStart and endIdx == self.m_visibleEnd then
            return
        end

        self.m_visibleStart = startIdx
        self.m_visibleEnd = endIdx

        -- 标记哪些 dataIndex 需要显示
        local needed = {}
        for i = startIdx, endIdx do
            needed[i] = true
        end

        -- 回收不再可见的节点到空闲池
        local freePool = {}
        for poolIdx, dataIdx in pairs(self.m_activeItems) do
            if not needed[dataIdx] then
                -- 回收：隐藏节点
                local item = self.m_pool[poolIdx]
                if item then
                    item.style.display = DisplayStyle.None
                end
                self.m_dataToPool[dataIdx] = nil
                freePool[#freePool + 1] = poolIdx
            end
        end
        -- 从 activeItems 中移除已回收的
        for _, poolIdx in ipairs(freePool) do
            self.m_activeItems[poolIdx] = nil
        end

        -- 分配节点给新出现的 dataIndex
        local freeIdx = 1
        for i = startIdx, endIdx do
            if not self.m_dataToPool[i] then
                -- 需要分配一个节点
                local poolIdx
                if freeIdx <= #freePool then
                    poolIdx = freePool[freeIdx]
                    freeIdx = freeIdx + 1
                else
                    -- 池不够，新建节点
                    poolIdx = #self.m_pool + 1
                    local newItem
                    if self.m_templateFn then
                        newItem = self.m_templateFn()
                    else
                        newItem = CS.UnityEngine.UIElements.VisualElement()
                    end
                    -- 设置为 absolute 定位以精确控制位置
                    newItem.style.position = Position.Absolute
                    newItem.style.left = Length(0, LengthUnit.Pixel)
                    newItem.style.right = Length(0, LengthUnit.Pixel)
                    newItem.style.height = Length(self.m_itemHeight, LengthUnit.Pixel)
                    self.m_contentWrapper:Add(newItem)
                    self.m_pool[poolIdx] = newItem
                end

                local item = self.m_pool[poolIdx]
                -- 设置位置
                local top = (i - 1) * self.m_itemHeight
                item.style.top = Length(top, LengthUnit.Pixel)
                item.style.height = Length(self.m_itemHeight, LengthUnit.Pixel)
                item.style.display = DisplayStyle.Flex

                self.m_activeItems[poolIdx] = i
                self.m_dataToPool[i] = poolIdx

                -- 刷新内容
                if self.m_ItemUpdateFunc then
                    self.m_ItemUpdateFunc(item, i)
                end
            else
                -- 已有节点，确保位置正确（防止 itemHeight 变化）
                local poolIdx = self.m_dataToPool[i]
                local item = self.m_pool[poolIdx]
                if item then
                    local top = (i - 1) * self.m_itemHeight
                    item.style.top = Length(top, LengthUnit.Pixel)
                    item.style.display = DisplayStyle.Flex
                end
            end
        end
    end

    --- 隐藏所有活跃节点
    function FViewListRootProxyImplForUITKScrollView:_HideAll()
        for poolIdx, _ in pairs(self.m_activeItems) do
            local item = self.m_pool[poolIdx]
            if item then
                item.style.display = DisplayStyle.None
            end
        end
        self.m_activeItems = {}
        self.m_dataToPool = {}
        self.m_visibleStart = 0
        self.m_visibleEnd = 0
    end

    --- 数据变化但数量不变时，重新对所有当前可见 item 调用 ItemUpdateFunc
    function FViewListRootProxyImplForUITKScrollView:Refresh()
        if not self:IsValid() or not self.m_ItemUpdateFunc then return end
        for poolIdx, dataIdx in pairs(self.m_activeItems) do
            local item = self.m_pool[poolIdx]
            if item then
                self.m_ItemUpdateFunc(item, dataIdx)
            end
        end
    end
end
--------------------------------------------------------------

--***********UGUI ScrollRect 虚拟滚动代理实现*****************--
-- 虚拟列表：只创建可见区域 + 缓冲区数量的节点，滚动时复用（与 UITK 实现镜像）。
-- 节点用 templateFn() 创建（返回 GameObject），按 anchoredPosition 绝对定位，
-- 显隐用 SetActive，总高度用 content.sizeDelta.y 撑出。
-- 使用方式：
--   容器 GameObject 上需挂 UnityEngine.UI.ScrollRect（带 content / viewport）
--   content 不要挂 LayoutGroup / ContentSizeFitter（会与手动定位冲突）
--   item 高度优先用 opts.itemHeight；否则从模板节点 RectTransform 测量；测不到则默认 100
local FViewListRootProxyImplForUGUIScrollView = FLua.Class(FViewListRootProxyImplBase, "FViewListRootProxyImplForUGUIScrollView")
do
    local ScrollRect    = CS.UnityEngine.UI.ScrollRect
    local RectTransform = CS.UnityEngine.RectTransform

    -- m_scrollRect  : ScrollRect 组件
    -- m_content     : 滚动内容 RectTransform（放置所有 item）
    -- m_viewportRT  : viewport RectTransform（决定可视高度）
    -- m_templateFn  : function() -> GameObject
    -- m_count       : 逻辑总数据量
    -- m_itemHeight  : 每个 item 固定高度(px)
    -- m_pool        : {GameObject, ...} 对象池
    -- m_activeItems : {[poolIndex] = dataIndex, ...}
    -- m_dataToPool  : {[dataIndex] = poolIndex, ...}
    -- m_bufferCount : 上下缓冲区额外节点数
    -- m_visibleStart/End : 当前可见数据索引范围（1-based）

    function FViewListRootProxyImplForUGUIScrollView:__constructor()
        self.m_scrollRect = nil
        self.m_content = nil
        self.m_viewportRT = nil
        self.m_templateFn = nil
        self.m_count = 0
        self.m_itemHeight = 0
        self.m_itemHeightResolved = false
        self.m_pool = {}
        self.m_activeItems = {}
        self.m_dataToPool = {}
        self.m_bufferCount = 2
        self.m_visibleStart = 0
        self.m_visibleEnd = 0
        self.m_scrollCallback = nil
    end

    ---@param templateFn fun():GameObject
    function FViewListRootProxyImplForUGUIScrollView:SetTemplateFn(templateFn)
        self.m_templateFn = templateFn
    end

    ---手动覆盖 item 高度（推荐：UGUI 同步测量不一定准）
    ---@param height number
    function FViewListRootProxyImplForUGUIScrollView:SetItemHeight(height)
        self.m_itemHeight = height
        self.m_itemHeightResolved = height and height > 0
    end

    function FViewListRootProxyImplForUGUIScrollView:OnCreate()
        local go = self.m_RootWidget
        local sr = go:GetComponent(typeof(ScrollRect))
        if not sr then
            error("[FViewListRootProxyImplForUGUIScrollView] root object <" .. tostring(go) .. "> has no ScrollRect component")
        end
        self.m_scrollRect = sr
        self.m_content    = sr.content
        self.m_viewportRT = sr.viewport or go:GetComponent(typeof(RectTransform))
        assert(self.m_content, "[FViewListRootProxyImplForUGUIScrollView] ScrollRect.content is nil")

        -- content 锚定到顶部，方便自上而下绝对定位
        self.m_content.anchorMin = Vector2(0, 1)
        self.m_content.anchorMax = Vector2(1, 1)
        self.m_content.pivot     = Vector2(0.5, 1)

        -- 测量 item 高度
        self:_ResolveItemHeight()

        -- 监听滚动
        self.m_scrollCallback = function(_v)
            self:_OnScroll()
        end
        sr.onValueChanged:AddListener(self.m_scrollCallback)
    end

    --- 解析 item 高度：opts 优先；否则从模板 RectTransform 同步测量
    function FViewListRootProxyImplForUGUIScrollView:_ResolveItemHeight()
        if self.m_itemHeightResolved then return end
        local h = 0
        if self.m_templateFn then
            local probe = self.m_templateFn()
            if probe then
                probe.transform:SetParent(self.m_content, false)
                local rt = probe:GetComponent(typeof(RectTransform))
                if rt then
                    h = rt.rect.height
                    if not h or h <= 0 then
                        h = rt.sizeDelta.y
                    end
                end
                UnityEngine.Object.Destroy(probe)
            end
        end
        if not h or h <= 0 then h = 100 end
        self.m_itemHeight = h
        self.m_itemHeightResolved = true
    end

    function FViewListRootProxyImplForUGUIScrollView:OnDestroy()
        if self.m_scrollRect and self.m_scrollCallback then
            -- 用 RemoveAllListeners 更稳：xLua 同一 lua 函数二次包装可能不是同一 delegate
            self.m_scrollRect.onValueChanged:RemoveAllListeners()
        end
        self.m_scrollCallback = nil

        for _, item in pairs(self.m_pool) do
            if item then UnityEngine.Object.Destroy(item) end
        end
        self.m_pool = {}
        self.m_activeItems = {}
        self.m_dataToPool = {}
        self.m_count = 0
        self.m_scrollRect = nil
        self.m_content = nil
        self.m_viewportRT = nil
        self.m_RootWidget = nil
    end

    function FViewListRootProxyImplForUGUIScrollView:SetCount(count)
        if not self:IsValid() then return end
        self.m_count = count

        -- 撑出总高度
        if self.m_content then
            local sd = self.m_content.sizeDelta
            self.m_content.sizeDelta = Vector2(sd.x, count * self.m_itemHeight)
        end

        self.m_visibleStart = 0
        self.m_visibleEnd = 0
        self:_OnScroll()
    end

    function FViewListRootProxyImplForUGUIScrollView:GetCount()
        return self.m_count
    end

    function FViewListRootProxyImplForUGUIScrollView:GetItemByIndex(index)
        local poolIdx = self.m_dataToPool[index]
        if poolIdx then
            return self.m_pool[poolIdx]
        end
        return nil
    end

    function FViewListRootProxyImplForUGUIScrollView:IsValid()
        return self.m_RootWidget ~= nil and self.m_content ~= nil
    end

    --- 滚动回调：计算可见范围，复用节点（算法与 UITK 实现一致）
    function FViewListRootProxyImplForUGUIScrollView:_OnScroll()
        if not self:IsValid() or self.m_count == 0 then
            self:_HideAll()
            return
        end

        -- content 顶部锚定：滚动向下时 content 上移，anchoredPosition.y 增大（正值）
        local scrollOffset = self.m_content.anchoredPosition.y or 0
        if scrollOffset < 0 then scrollOffset = 0 end

        local viewportHeight = 0
        if self.m_viewportRT then
            viewportHeight = self.m_viewportRT.rect.height
        end
        if not viewportHeight or viewportHeight <= 0 then viewportHeight = 600 end

        local startIdx = math.floor(scrollOffset / self.m_itemHeight) + 1
        local endIdx   = math.ceil((scrollOffset + viewportHeight) / self.m_itemHeight)

        startIdx = math.max(1, startIdx - self.m_bufferCount)
        endIdx   = math.min(self.m_count, endIdx + self.m_bufferCount)

        if startIdx == self.m_visibleStart and endIdx == self.m_visibleEnd then
            return
        end
        self.m_visibleStart = startIdx
        self.m_visibleEnd = endIdx

        local needed = {}
        for i = startIdx, endIdx do needed[i] = true end

        -- 回收不再可见的节点
        local freePool = {}
        for poolIdx, dataIdx in pairs(self.m_activeItems) do
            if not needed[dataIdx] then
                local item = self.m_pool[poolIdx]
                if item then item:SetActive(false) end
                self.m_dataToPool[dataIdx] = nil
                freePool[#freePool + 1] = poolIdx
            end
        end
        for _, poolIdx in ipairs(freePool) do
            self.m_activeItems[poolIdx] = nil
        end

        -- 给新出现的数据索引分配节点
        local freeIdx = 1
        for i = startIdx, endIdx do
            if not self.m_dataToPool[i] then
                local poolIdx
                if freeIdx <= #freePool then
                    poolIdx = freePool[freeIdx]
                    freeIdx = freeIdx + 1
                else
                    poolIdx = #self.m_pool + 1
                    local newItem
                    if self.m_templateFn then
                        newItem = self.m_templateFn()
                    end
                    if not newItem then
                        error("[FViewListRootProxyImplForUGUIScrollView] templateFn must return a GameObject")
                    end
                    newItem.transform:SetParent(self.m_content, false)
                    local rt = newItem:GetComponent(typeof(RectTransform))
                    if rt then
                        rt.anchorMin = Vector2(0, 1)
                        rt.anchorMax = Vector2(1, 1)
                        rt.pivot     = Vector2(0.5, 1)
                        rt.sizeDelta = Vector2(0, self.m_itemHeight)
                    end
                    self.m_pool[poolIdx] = newItem
                end

                local item = self.m_pool[poolIdx]
                local rt = item:GetComponent(typeof(RectTransform))
                if rt then
                    rt.anchoredPosition = Vector2(0, -(i - 1) * self.m_itemHeight)
                    rt.sizeDelta = Vector2(0, self.m_itemHeight)
                end
                item:SetActive(true)

                self.m_activeItems[poolIdx] = i
                self.m_dataToPool[i] = poolIdx

                if self.m_ItemUpdateFunc then
                    self.m_ItemUpdateFunc(item, i)
                end
            else
                local poolIdx = self.m_dataToPool[i]
                local item = self.m_pool[poolIdx]
                if item then
                    local rt = item:GetComponent(typeof(RectTransform))
                    if rt then
                        rt.anchoredPosition = Vector2(0, -(i - 1) * self.m_itemHeight)
                    end
                    item:SetActive(true)
                end
            end
        end
    end

    function FViewListRootProxyImplForUGUIScrollView:_HideAll()
        for poolIdx, _ in pairs(self.m_activeItems) do
            local item = self.m_pool[poolIdx]
            if item then item:SetActive(false) end
        end
        self.m_activeItems = {}
        self.m_dataToPool = {}
        self.m_visibleStart = 0
        self.m_visibleEnd = 0
    end

    --- 数据变化但数量不变时，重新对所有当前可见 item 调用 ItemUpdateFunc
    function FViewListRootProxyImplForUGUIScrollView:Refresh()
        if not self:IsValid() or not self.m_ItemUpdateFunc then return end
        for poolIdx, dataIdx in pairs(self.m_activeItems) do
            local item = self.m_pool[poolIdx]
            if item then
                self.m_ItemUpdateFunc(item, dataIdx)
            end
        end
    end
end
--------------------------------------------------------------

--******************对外接口类********************************--

local FViewListRootProxy = FLua.Class("ECViewListRootProxy")
function FViewListRootProxy:__constructor()
    self.m_ProxyImpl = nil
end

---按面板后端类型创建空代理（不绑定 widget，也不 OnCreate）。
---调用者负责后续 SetRootWidget(viewObj) -> SetItemUpdateFunc(fn) -> OnCreate()。
---这是 FFixedList / FScrollList 等"在 viewObj 就绪后才知道后端"场景的统一入口。
---@param backend integer PanelType.FairyGUI / PanelType.UIkit / PanelType.UGUI
---@param opts table|nil {templateFn=fun():GameObject|VisualElement, itemHeight=number}
---@return FViewListRootProxy
function FViewListRootProxy.CreateByBackend(backend, opts)
    opts = opts or {}
    local object = FViewListRootProxy()
    local impl
    if backend == PanelType.FairyGUI then
        impl = FViewListRootProxyImplForFairyGUIList()
    elseif backend == PanelType.UIkit then
        impl = FViewListRootProxyImplForUITKScrollView()
    elseif backend == PanelType.UGUI then
        impl = FViewListRootProxyImplForUGUIScrollView()
    else
        error("FViewListRootProxy.CreateByBackend: unsupported backend " .. tostring(backend))
    end
    -- UGUI / UITK 需要 templateFn 创建 item 节点；itemHeight 可选覆盖。FairyGUI 忽略。
    if impl.SetTemplateFn and opts.templateFn then impl:SetTemplateFn(opts.templateFn) end
    if impl.SetItemHeight and opts.itemHeight then impl:SetItemHeight(opts.itemHeight) end
    object.m_ProxyImpl = impl
    return object
end

function FViewListRootProxy.CreateProxyList(RootObject)
    local object = FViewListRootProxy()
    if RootObject:IsExtend("FairyGUI.GList") then
        object.m_ProxyImpl = FViewListRootProxyImplForFairyGUIList()
    --TODO: UGUI List
    else
        error("Unsupported root object type for FViewListRootProxy: " .. tostring(RootObject))
    end
    object:SetRootWidget(RootObject)
    return object
end

---创建 UIToolkit ScrollView / VisualElement 容器代理
---@param bridge UITKLuaBridge  由 self.m_bridge 传入
---@param containerName string  UXML 里 ScrollView（或容器 VisualElement）的 name
---@param templateFn fun():VisualElement|nil  可选，返回一个新建 VisualElement 作为 item 模板；nil 则用空 VisualElement
---@return FViewListRootProxy
function FViewListRootProxy.CreateProxyUITK(bridge, containerName, templateFn)
    assert(bridge, "FViewListRootProxy.CreateProxyUITK: bridge is nil")
    assert(containerName and #containerName > 0, "FViewListRootProxy.CreateProxyUITK: containerName is required")

    -- 通过 bridge 拿到 VisualElement
    local ve = bridge:Q(containerName)
    assert(ve, "FViewListRootProxy.CreateProxyUITK: element '" .. containerName .. "' not found in UIDocument")

    local object = FViewListRootProxy()
    local impl = FViewListRootProxyImplForUITKScrollView()
    if templateFn then
        impl:SetTemplateFn(templateFn)
    end
    object.m_ProxyImpl = impl
    object:SetRootWidget(ve)
    object:OnCreate()   -- 触发 OnCreate 让 impl 初始化 container
    return object
end


function FViewListRootProxy:SetCount(count)
    self.m_ProxyImpl:SetCount(count)
end

---[可选] 手动覆盖 item 高度（像素）。通常不需要调用——高度会自动从模板节点样式中获取。
---仅当模板节点没有在 USS 中指定固定高度时才需要手动设置。
---@param height number
function FViewListRootProxy:SetItemHeight(height)
    if self.m_ProxyImpl.SetItemHeight then
        self.m_ProxyImpl:SetItemHeight(height)
    end
end

function FViewListRootProxy:SetItemUpdateFunc(itemUpdateFunc)
    self.m_ProxyImpl:SetItemUpdateFunc(itemUpdateFunc)
end

function FViewListRootProxy:GetCount()
    return self.m_ProxyImpl:GetCount()
end

---数据变化但数量不变时，强制刷新所有可见 item 内容
function FViewListRootProxy:Refresh()
    if self.m_ProxyImpl.Refresh then
        self.m_ProxyImpl:Refresh()
    end
end

function FViewListRootProxy:GetItemByIndex(index)
    return self.m_ProxyImpl:GetItemByIndex(index)
end

function FViewListRootProxy:SetRootWidget(widget)
    self.m_ProxyImpl:SetRootWidget(widget)
end

function FViewListRootProxy:OnCreate()
    self.m_ProxyImpl:OnCreate()
end
function FViewListRootProxy:OnDestroy()
    self.m_ProxyImpl:OnDestroy()
end
return FViewListRootProxy