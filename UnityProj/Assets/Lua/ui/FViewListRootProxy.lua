---ECViewList的根代理，用于设置数量，获取数量等, ECViewListRootProxy为对外的接口类，ECViewListRootProxyImplXXX为实现

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

--***********UIToolkit ScrollView 代理实现********************--
-- 以 ScrollView.contentContainer 为容器，通过动态 Add/Remove VisualElement 子节点实现列表。
-- 使用方式：
--   UXML 里放一个 name="list_container" 的 ScrollView（或任意 VisualElement 容器）
--   在 FPanelXxx:OnCreate 里：
--     local listRoot = FViewListRootProxy.CreateProxyUITK(bridge, "list_container", templateCreatorFn)
--   templateCreatorFn() 返回一个新建的 VisualElement 作为列表项模板

local FViewListRootProxyImplForUITKScrollView = FLua.Class(FViewListRootProxyImplBase, "FViewListRootProxyImplForUITKScrollView")
do
    -- m_RootWidget  : ScrollView (VisualElement)
    -- m_templateFn  : function() -> VisualElement  —— 创建一个空白 item
    -- m_items       : {VisualElement, ...}  —— 已创建的子节点列表
    -- m_count       : 当前逻辑项数

    function FViewListRootProxyImplForUITKScrollView:__constructor()
        self.m_templateFn = nil
        self.m_items = {}
        self.m_count = 0
        self.m_container = nil
    end

    ---@param templateFn fun():VisualElement
    function FViewListRootProxyImplForUITKScrollView:SetTemplateFn(templateFn)
        self.m_templateFn = templateFn
    end

    function FViewListRootProxyImplForUITKScrollView:OnCreate()
        -- 兼容 ScrollView（取 contentContainer）和普通 VisualElement
        -- ScrollView.contentContainer 是真正放子节点的区域
        local ve = self.m_RootWidget
        if ve.contentContainer then
            self.m_container = ve.contentContainer
        else
            self.m_container = ve
        end
    end

    function FViewListRootProxyImplForUITKScrollView:OnDestroy()
        self:_ClearItems()
        self.m_RootWidget = nil
        self.m_container  = nil
    end

    function FViewListRootProxyImplForUITKScrollView:SetCount(count)
        if not self:IsValid() then return end
        local old = self.m_count
        self.m_count = count

        if count > old then
            -- 补充不够的节点
            for i = old + 1, count do
                self:_GetOrCreateItem(i)
            end
        elseif count < old then
            -- 隐藏多余节点（保留 DOM，只 display:none），避免频繁创建
            for i = count + 1, old do
                local item = self.m_items[i]
                if item then
                    item.style.display = CS.UnityEngine.UIElements.DisplayStyle.None
                end
            end
        end
        -- 对所有可见节点调用 itemUpdateFunc 刷新内容
        if self.m_ItemUpdateFunc then
            for i = 1, count do
                local item = self.m_items[i]
                if item then
                    self.m_ItemUpdateFunc(item, i)
                end
            end
        end
    end

    function FViewListRootProxyImplForUITKScrollView:GetCount()
        return self.m_count
    end

    function FViewListRootProxyImplForUITKScrollView:GetItemByIndex(index)
        return self.m_items[index]
    end

    function FViewListRootProxyImplForUITKScrollView:IsValid()
        return self.m_RootWidget ~= nil
    end

    -- 内部：获取第 i 个 item（不存在则新建并 Add 到容器）
    function FViewListRootProxyImplForUITKScrollView:_GetOrCreateItem(i)
        local item = self.m_items[i]
        if item then
            item.style.display = CS.UnityEngine.UIElements.DisplayStyle.Flex
            return item
        end
        -- 通过 templateFn 创建新节点；若未设置则创建空 VisualElement
        if self.m_templateFn then
            item = self.m_templateFn()
        else
            item = CS.UnityEngine.UIElements.VisualElement()
        end
        self.m_container:Add(item)
        self.m_items[i] = item
        return item
    end

    -- 内部：清空所有子节点
    function FViewListRootProxyImplForUITKScrollView:_ClearItems()
        if self.m_container then
            self.m_container:Clear()
        end
        self.m_items = {}
        self.m_count = 0
    end
end
--------------------------------------------------------------

--******************对外接口类********************************--

local FViewListRootProxy = FLua.Class("ECViewListRootProxy")
function FViewListRootProxy:__constructor()
    self.m_ProxyImpl = nil
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

function FViewListRootProxy:SetItemUpdateFunc(itemUpdateFunc)
    self.m_ProxyImpl:SetItemUpdateFunc(itemUpdateFunc)
end
function FViewListRootProxy:GetCount()
    return self.m_ProxyImpl:GetCount()
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