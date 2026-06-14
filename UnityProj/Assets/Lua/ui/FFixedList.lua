--[[
	View 列表，适配 AzureScrollList、AzureFixedItemList
	内含多个元素，元素数量可以动态变化
	会构建与元素数量相当的 View 实例。元素可见时，对应 View 会 Attach 到元素上；元素不可见时，会 Detach

	与 ECScrollList、ECViewCloner 的区别：
		使用 OnEvent 处理事件，正确处理元素重用
		保持功能最小化，不添加 view 容器之外的方法
]]

local FViewBaseUI = require "ui.FViewBaseUI"
local FViewListRootProxy = require "ui.FViewListRootProxy"
local assert = assert

local function logError (str, errLevel)
	printError(debug.traceback(str, (errLevel or 0)+1))
end

local SimpleItemView = FLua.Class(FViewBaseUI)
do
	function SimpleItemView:__constructor()
		self.m_onCreateFunc = nil
		self.m_index = 0
	end
	function SimpleItemView.new(onCreateFunc, index)
		local obj = SimpleItemView()
		obj.m_onCreateFunc = onCreateFunc
		obj.m_index = index
		return obj
	end

	function SimpleItemView:OnCreate()
		self.m_onCreateFunc(self, self.m_index)
	end
end
---@class FFixedList:FViewBaseUI
local FFixedList = FLua.Class(FViewBaseUI, "FFixedList")
do
	function FFixedList:__constructor()
		--- 后端无关的列表代理（FairyGUI / UIToolkit / UGUI），在 viewObj 就绪后懒创建
		self.m_RootProxy = nil

		self.m_itemViewConstructor = nil

		self.m_itemViews = nil

		--- UGUI / UIToolkit 后端创建 item 节点所需的选项 {templateFn=, itemHeight=}
		--- FairyGUI 后端忽略（item 由 GList 池自动管理）
		self.m_opts = nil
	end
	
	--[[
		创建一个实例，元素的 View 以 itemViewConstructor 构建
		param itemViewConstructor: 创建元素 View 实例
			function itemViewConstructor (itemIndex)
				return itemView
		param opts: 可选，UGUI/UIToolkit 后端必填 templateFn
			{
				templateFn = function() return GameObject|VisualElement end,  -- item 节点模板
				itemHeight = number,  -- 可选，固定行高(px)；不填则尝试自动测量
			}
			FairyGUI 后端可不传（item 由 GList 自动管理）
	]]
	function FFixedList.Create(itemViewConstructor, opts)
		assert(itemViewConstructor, "itemViewConstructor is nil")
		local obj = FFixedList()
		obj.m_itemViewConstructor = itemViewConstructor
		obj.m_opts = opts
		return obj
	end

	--[[
		简易地创建一个 FFixedList 实例，元素 View 的 OnCreate 调用 itemOnCreate
		param itemOnCreate: 元素 View 的 OnCreate 调用此函数
			function itemOnCreate (itemView, itemIndex)
		param opts: 见 FFixedList.Create
	]]
	function FFixedList.CreateSimple(itemOnCreate, opts)
		return FFixedList.Create(function (index)
			return FFixedList.CreateItemSimple(itemOnCreate, index)
		end, opts)
	end

	--[[
		简易地创建一个元素 View，可用于实现 Create 的回调函数
		param itemOnCreate: 见 CreateSimple
	]]
	function FFixedList.CreateItemSimple(itemOnCreate, index)
		return SimpleItemView.new(itemOnCreate, index)
	end

	--- 在 viewObj 就绪后懒创建后端无关的列表代理
	---@return FViewListRootProxy|nil
	function FFixedList:_EnsureProxy()
		if self.m_RootProxy then
			return self.m_RootProxy
		end
		if not self.m_viewObj then
			return nil
		end
		self.m_RootProxy = FViewListRootProxy.CreateProxyList(self.m_viewObj, self.m_opts)
		return self.m_RootProxy
	end

	function FFixedList:GetItemCount()
		self:_InitIfNeeded(true)		--初始化，以便在 OnCreate 前即可调用
		return self.m_itemViews and #self.m_itemViews or 0
	end
	
	function FFixedList:SetItemCount(count)
		self:_InitIfNeeded(false)		--初始化，以便在 OnCreate 前即可调用

		local proxy = self:_EnsureProxy()
		if not proxy then
			warn("FFixedList:SetItemCount called before viewObj ready")
			return
		end

		local oldCount = self:GetItemCount()
		if count > oldCount then
			proxy:SetCount(count)
			local currentCount = #self.m_itemViews	--数量可能已经发生变化
			for i = currentCount + 1, count do
				self:_AddView(i)
			end
		elseif count < oldCount then
			for i = oldCount, count + 1, -1 do
				self:_RemoveView(i)
			end
			proxy:SetCount(count)
		else
			proxy:SetCount(count)
		end
		
		assert(self:GetItemCount() == count)
	end

	function FFixedList:SetDataList(data_list)
		self:SetItemCount(#data_list)
		for i = 1, self:GetItemCount() do
			local view = self:GetItem(i)
			if view and view:tryget("SetDataByIndex") then
				view:SetDataByIndex(data_list[i], i)
			elseif view and view:tryget("SetData") then
				view:SetData(data_list[i])
			else
				logError(("[%s]"):format(tostring(view)) .. "must implement method <SetData> for your view before calling <SetDataList>.")
			end
		end
	end

	function FFixedList:GetItem(index)
		self:_InitIfNeeded(true)		--初始化，以便在 OnCreate 前即可调用
		
		local item = self.m_itemViews[index]
		if item == nil then
			logError(("Item index too large, max: %d, got: %d"):format(#self.m_itemViews, index), 2)
		end

		return item
	end

	--[[
		对所有 Item View 调用函数
		
		param functionName: Item View 上的函数名
		param varlist: 调用时传递的参数
	]]
	function FFixedList:InvokeItemsFunction(functionName, ...)
		for i =  1, self:GetItemCount() do
			local item = self:GetItem(i)
			local f = item[functionName]
			f(item, ...)
		end
	end

	function FFixedList:InvokeItemsFunctionEx(functionName, getter)
		for i =  1, self:GetItemCount() do
			local item = self:GetItem(i)
			local f = item[functionName]
			f(item, getter(i))
		end
	end

	------------------------------
	-- End of public
	------------------------------

	--[[
		初始化
		param bSyncFromResource: 是否从资源Item数量同步
	]]
	function FFixedList:_InitIfNeeded(bSyncFromResource)
		if not self.m_viewObj then
			warn("view scroll list init when viewObj is nil")
			return
		end

		if self.m_itemViews == nil then
			self.m_itemViews = {}

			if bSyncFromResource then
				local proxy = self:_EnsureProxy()
				local itemCount = proxy and proxy:GetCount() or 0
				for i = 1, itemCount do
					self:_AddView(i)
				end
			end
		end
	end

	function FFixedList:UpdateSubViewObj()
		--尽可能早设置viewObj
		if self.m_viewObj then
			local proxy = self:_EnsureProxy()
			if proxy then proxy:SetRootWidget(self.m_viewObj) end
		end
	    FViewBaseUI.UpdateSubViewObj(self)
	end

	function FFixedList:OnCreateInternal()
		local proxy = self:_EnsureProxy()
		if proxy then
			proxy:SetRootWidget(self.m_viewObj)
			proxy:SetItemUpdateFunc(function (itemWidget, index)
				self:_OnListItemUpdate(itemWidget, index)
			end)
			proxy:OnCreate()
		end
		self:_InitIfNeeded(true)
		--SetItemCount 时会调用 Item 的 OnCreate，因此此处不能调用，否则会重复调用
		self:OnCreate()
	end

	function FFixedList:_OnListItemUpdate(itemWidget, index)
		if not self.m_itemViews then
			local info = debug.getinfo(self.m_itemViewConstructor, "Sl")
			warn("view list error:not init", tostring(self.m_viewOwner), self:IsValid(), info and info.source)
			return
		end
		if index > #self.m_itemViews then
			for i = #self.m_itemViews + 1, index do
				self:_AddView(i)
			end
		end

		local itemView = self.m_itemViews[index]
		assert(itemView, string.format("itemView must valid. %d", index))

		--有 item 更新意味着，可能有 item 不再可见。目前“变得不可见”无通知，故遍历找出
		for iItem, oneItemView in pairs(self.m_itemViews) do
			if oneItemView.m_viewObj then
				if not self.m_RootProxy:GetItemByIndex(iItem) then
					self:DetachSubView(oneItemView, true)
				end
			end
		end
		--[[ 上方 self.m_RootProxy 此时必非 nil：_OnListItemUpdate 仅由代理回调触发 ]]

		--重新绑定 (即使 m_viewObj 未变，其内部 SWidget 也可能已变化，必须重新绑定)
		self:DetachSubView(itemView, true)
		self:_AttachSubView(itemWidget, itemView, true)
	end

	function FFixedList:OnDestroyInternal()
		if self.m_itemViews then
			for index = self:GetItemCount(), 1, -1 do
				self:_RemoveView(index)
			end
			self.m_itemViews = nil
		end
		if self.m_RootProxy then
			self.m_RootProxy:OnDestroy()
			self.m_RootProxy = nil
		end
		FViewBaseUI.OnDestroyInternal(self)
	end
	
	function FFixedList:_CreateItemView(index)
		local itemView = self.m_itemViewConstructor(index)
		if not FLua.is(itemView, FViewBaseUI) then
			error(("invalid result from item view constructor (FViewBaseUI expected, got %s), on: %s"):format(
				--tostring(ECDebugHelper.FormatGameObjectPath(self.m_viewObj)),
				self.m_viewObj:GetName(),
				tostring(itemView)), 3)
		end
		return itemView
	end
	
	--只能在数组尾操作
	function FFixedList:_AddView(index)
		assert(index == #self.m_itemViews + 1)
		local itemView = self:_CreateItemView(index)
		self.m_itemViews[index] = itemView
		local proxy = self:_EnsureProxy()
		local itemObj = proxy and proxy:GetItemByIndex(index)
		if itemObj then
			self:_AttachSubView(itemObj, itemView, true)
		end
	end

	function FFixedList:_AttachSubView(obj, subView, bInvokeOnCreate)
		self:AttachSubView(obj, subView, true)
	end
	
	--只能在数组尾操作
	function FFixedList:_RemoveView(index)
		assert(index == #self.m_itemViews)
		
		local itemView = self.m_itemViews[index]
		if itemView.m_viewObj then
			self:DetachSubView(itemView, true)
		end
		self.m_itemViews[index] = nil
	end
end

return FFixedList
