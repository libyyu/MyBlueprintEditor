local FViewBaseUI = require "ui.FViewBaseUI"
local FPanelLoader = require "ui.FPanelLoader"
local Callbacks = require "utility.Callbacks"
local FGUIMan = require "ui.FGUIMan"

---@class PanelInVisibleMask
_G.PanelInVisibleMask =
{
	None = 0,
	Logical 	=	0x00000001, --逻辑控制隐藏
	ParentPanel =	0x00000002,	--因父界面隐藏而隐藏
	Debug 		=	0x00000004,	--调试功能隐藏
}
---@class GUIDEPTH
_G.GUIDEPTH = Enum.make
{
	'AUTO', '=', 0,
	'BOTTOM',
	'NORMALLESS',
	'NORMAL',
	'TOP',
	'TOPMOST',
}
---@class FPanelBaseUI : FPanelLoader
local FPanelBaseUI = FLua.Class(FPanelLoader, "FPanelBaseUI")
local depthLayers = {
	{ depth = 10    , lastPanel = nil, layerPanels = {} }
, { depth = 5000  , lastPanel = nil, layerPanels = {} }
, { depth = 10000 , lastPanel = nil, layerPanels = {} }
, { depth = 20000 , lastPanel = nil, layerPanels = {} }
, { depth = 30000 , lastPanel = nil, layerPanels = {} }
, { depth = 60000 , lastPanel = nil, layerPanels = {} }
, { depth = 90000 , lastPanel = nil, layerPanels = {} }
, { depth = 100000, lastPanel = nil, layerPanels = {} }
, { depth = 120000, lastPanel = nil, layerPanels = {} }
, { depth = 130000, lastPanel = nil, layerPanels = {} }
, { depth = 140000, lastPanel = nil, layerPanels = {} }
, { depth = 150000, lastPanel = nil, layerPanels = {} }
, { depth = 160000, lastPanel = nil, layerPanels = {} }
, { depth = 180000, lastPanel = nil, layerPanels = {} }
, { depth = 190000, lastPanel = nil, layerPanels = {} }
}

local function GetLayerNextTopDepth(layer_id)
	if not depthLayers[layer_id] then return 0 end

	local final_depth = depthLayers[layer_id].depth
	for _, panel_depth in pairs(depthLayers[layer_id].layerPanels) do
		final_depth = math.max(final_depth, panel_depth)
	end
	return final_depth + 1
end
local function GetLayerNextBottomDepth(layer_id)
	if not depthLayers[layer_id] then return 0 end
	local final_depth = depthLayers[layer_id].depth
	for _, panel_depth in pairs(depthLayers[layer_id].layerPanels) do
		final_depth = math.min(final_depth, panel_depth)
	end
	if final_depth > depthLayers[layer_id].depth then
		return final_depth - 1
	end
	return depthLayers[layer_id].depth
end

--[[
	Private Methods
	默认构造方法，外部禁止显示调用
]]
function FPanelBaseUI:__constructor()
	---@type number
	self.m_depthLayer = GUIDEPTH.AUTO

	---@type userdata
	self.m_msgHandler = nil

	--- 事件桥接器：UGUI = UILuaBehaviour, UITK = UITKLuaBridge
	--- Lua 面板通过 self.m_bridge 访问，无需关心后端类型
	self.m_bridge = nil

	---@type number
	self.m_invisibleFlag = PanelInVisibleMask.None

	---@type boolean
	self.m_created = false	--OnCreate 是否已调用

	self.m_opaque = false


	self.m_destroyByNewCreate = false

	self.m_dontDestroyOnLoad = false

	self:SetViewRoot(self)
end

function FPanelBaseUI:SetVisible(b)
	if self:IsSubView() then --有可能由于优化，将从PanelBase继承的页面作为一个View存在
		FViewBaseUI.SetVisible(self, b)
	else
		self:SetInvisibleFlagValid(_G.PanelInVisibleMask.Logical, not b)
	end
end

function FPanelBaseUI:IsVisible()
	if self:IsSubView() then --有可能由于优化，将从PanelBase继承的页面作为一个View存在
		return FViewBaseUI.IsVisible(self)
	end
	-- UITK：不依赖 m_panel（RootGameObject）判断，直接问 backend
	-- 注意：必须在 IsValidObject(m_panel) 检查之前，否则 DestroyPanelRaw 后 m_backend 已 nil
	if self:IsUIToolkit() then
		if self.m_backend and self.m_backend.IsValid then
			return self.m_backend:GetVisible()
		end
		return false
	end
	if not IsValidObject(self.m_panel) then
		return false
	end
	if self:IsFairyGui() then
		if self.m_panel.displayObject then
			return not not self.m_panel.displayObject.visible
		elseif self.m_panel.gameObject then
			return self.m_panel.gameObject.activeSelf
		else
			printError("self.m_panel not valid >>>>>>>>>>>>>", self.m_panel)
		end
	else
		return self.m_panel.activeSelf
	end
end

--指定界面的资源
---@return string 资源路径
function FPanelBaseUI:GetResPath()
	error("You Must Override GetResPath To Specify ResPath: " .. tostring(self), 2)
end

---@param callback fun(bSucceeded: boolean)
function FPanelBaseUI:CreatePanelNew(callback)
	local resName = self:GetResPath()
	self:CreatePanelInternal(resName, callback)
end
---@param show boolean
---@param params any
---@param callback fun(bSucceeded: boolean)
function FPanelBaseUI:ShowPanel(show, params, callback)
	--self.m_paramExt = params
	if show then
		self:CreatePanelNew(callback)
	else
		self:DestroyPanel()
	end
end
---@param resName string
function FPanelBaseUI:CreatePanel(resName)
	self:CreatePanelInternal(resName, nil)
end

-- def.method("userdata","string", "userdata").CreateFromGameObject = function(self, go, panelName, goParent)
-- 	self:LoadPanelFromGameObject(go, panelName, goParent, function (bFinished, bSucceeded)
-- 		if not bFinished or not bSucceeded then
-- 			return
-- 		end

-- 		self:OnLoadPanel()
-- 	end)
-- end

function FPanelBaseUI:DestroyPanel()
	return self:_DestroyPanelEx(false)
end

function FPanelBaseUI:DestroyPanelImmediate()
	return self:_DestroyPanelEx(true) -- 立即关闭
end

--[[
	param bImmediate: 是否立即关闭。立即关闭时不播放关闭动画
		退出游戏等情况时应立即关闭，否则关闭中点击 UI 可能触发错误访问
]]
---@param bImmediate boolean
function FPanelBaseUI:_DestroyPanelEx(bImmediate)
	local firstDestroy = self.m_createRequested
	self.m_createRequested = false
	
	if not self:IsPanelRegistered() then	--fast path
		return
	end

	if not IsValidObject(self.m_panel) then
		self:AfterDestroyInternal()
		self:UnRegisterPanel()
		return
	end
	
	if firstDestroy then
		
	end

	-- if not bImmediate and not self.m_disappearing and not globalGame.m_isReleasing then  --退出游戏时需立即调用，不然来不及调到
	-- 	self.m_disappearing = true
	-- 	self:PlayDisappearAnimOnDestroy(function()
	-- 		if self.m_disappearing then
	-- 			self:DestroyPanelRaw()
	-- 		end
	-- 	end)
	-- else
		self:DestroyPanelRaw()
	-- end
end

---@return boolean
function FPanelBaseUI:IsRootPanel()
	--return self.m_DependsPanel == nil and not self:IsSubView()
end

function FPanelBaseUI:BringTop()

end
---@return integer
function FPanelBaseUI:GetDepthLayer()
	--local DepthCfg = ui_depth_cfg.Config
	if self.m_depthLayer == GUIDEPTH.AUTO then
		-- if self.m_panelName ~= "" then
		-- 	local depthByConfig = DepthCfg[self.m_panelName]
		-- 	if depthByConfig then
		-- 		return depthByConfig
		-- 	end
		-- end
		return GUIDEPTH.NORMAL
	else
		return self.m_depthLayer
	end
end


function FPanelBaseUI:RegisterPanel()
	FGUIMan.Instance():RegisterPanel(self)
end

function FPanelBaseUI:UnRegisterPanel()
	FGUIMan.Instance():UnRegisterPanel(self)
end
---@return boolean
function FPanelBaseUI:IsPanelRegistered()
	return FGUIMan.Instance():IsPanelRegistered(self)
end

-------------------------------------------------------------
-----End Public
-------------------------------------------------------------
---@param resName string
---@param callback fun(bSucceeded: boolean)
function FPanelBaseUI:CreatePanelInternal(resName, callback)
	if self:IsResourceReady() then
		if callback then callback(true) end
		return
	end

	if self.m_disappearing then	--如果是正处于消失中的panel，直接销毁，重新加载
		self.m_destroyByNewCreate = true
		self:DestroyPanelRaw()
	else
		self.m_destroyByNewCreate = false
	end

	--注册顶级 Panel
	self:RegisterPanel()

	local parent = nil
	-- if self.m_dontDestroyOnLoad then
	-- 	parent = FairyGUI.GRoot.inst.gameObject
	-- end
	self:LoadPanel(resName, parent, function (bSucceeded)
		if not bSucceeded then
			if callback then callback(false) end
			self:AfterDestroyInternal()
			return
		elseif not self:IsPanelRegistered() then --某些原因被取消了
			warn("load is cancled because panel is unregistered!!!")
			self:DestroyPanel()
			if callback then callback(false) end
		end

		self:OnLoadPanel()
		if callback then callback(true) end
	end)
end

function FPanelBaseUI:DestroyPanelRaw()	
	if self.m_destroying then	--防重入 (可能造成 OnDestroy 多次调用)
		return
	end

	self.m_destroying = true
	self.m_invisibleFlag = _G.PanelInVisibleMask.None ---退出需要清状态
	self:OnShowInternal(false)
	self:OnDestroyInternal()
	self.m_created = false
	self.m_destroyByNewCreate = false
	self:UnRegisterPanel()
	self:_RemovePanelFromLayer()
	
	--卸载界面
	FGUIMan.Instance():UnRegisterPanelObj(self.m_panel)
	self:UnloadPanel()			
	
	--清除事件桥接器（IUIPanelBridge.ClearAllListeners 统一接口，两端实现相同语义）
	--   UGUI: 清旧 buttons + msgHandle + 新的 _explicitClicks 列表
	--   UITK: 清全局扫描 + 精确绑定 + 缓存
	if self.m_bridge and self.m_bridge.ClearAllListeners then
		self.m_bridge:ClearAllListeners()
	end
	self.m_msgHandler = nil
	self.m_bridge = nil
	self.m_backend = nil
	
	self.m_disappearing = false
	self.m_destroying = false

	self:AfterDestroyInternal()
end

function FPanelBaseUI:OnBeforeLoadPanel()
end

function FPanelBaseUI:OnLoadPanel()
	-- show
	if not IsValidObject(self.m_panel) then
		warn("load panel over but m_panel is nil, panelName:".. self.m_panelName)
		return
	end

	FGUIMan.Instance():RegisterPanelObj(self.m_panel, self)

	self:_BringTopReal()

	--附加 msgHandler
	self:TouchMsgHandler()

	-- 触发OnCreate
	self:OnCreateInternal()
	self.m_created = true

	if self:IsFairyGui() then
		self.m_panel.opaque = self.m_opaque
	end

	local real_active = self:IsVisible()
	self:OnShowInternal(real_active)

	-- 置顶UI
	if self:IsRootPanel() then	--TODO LoadPanel 时已做，此处应该不需要了
		self:_BringTopInner()
	end

	-- after create
	self:AfterCreateInternal()

	self:UpdateVisibleByFlag()	--TODO LoadPanel 时已做，此处应该不需要了
end

function FPanelBaseUI:OnChangePanelObject()
	self.m_viewObj = self.m_panel

	---初始状态，有可能m_panel创建好，没有调用OnLoadPanel的时候，被隐藏了
	if self:IsValid() then
		--需在任何实际访问调用 (特别的，需在 UpdateSubViewObj 前)，涉及 UWidget::GetUserWidget 判断
		self:TouchMsgHandler()
	end

	self:UpdateSubViewObj()
end

-- if FairyGUI then
-- 	FairyGUI.GLoader.gLoaderFunc = function(url, loader)
-- 		print("loader:", url, loader.name, loader.rootOwner, loader.rootOwner:GetHashCode(), loader.rootOwner.displayObject.gameObject)
-- 		-- local panel = loader.rootOwner:GetLuaUserData("__panel", self)
-- 		-- print("loader:", panel)
-- 		-- if loader.name == 'data' then
-- 		-- 	loader.url = "ui://dzefjlp5aewvc"
-- 		-- end
-- 		return true
-- 	end
-- end

function FPanelBaseUI:TouchMsgHandler()
	local function getFunc(name)
		local func = self:tryget(name)
		if func and type(func) == "function" then
			return func
		end
		return nil
	end

	-- ── UI Toolkit 分支 ───────────────────────────────────────────────────
	if self:IsUIToolkit() then
		-- 从 backend 拿到 UITKLuaBridge
		if not self.m_backend then return end
		local bridge = self.m_backend:GetEventBridge()
		if not bridge then return end
		self.m_bridge = bridge
		self.m_msgHandler = bridge  -- 兼容旧代码对 m_msgHandler 的引用

		-- OnClick：全局扫描所有 Button
		local func = getFunc("OnClick")
		if func then
			local mst = {
				onClick = function(name) self:OnClick(name) end
			}
			bridge:TouchAllButtons(mst)
		end

		-- OnSubmit / OnChange：全局扫描所有 TextField
		local hasSubmit = getFunc("OnSubmit")
		local hasChange = getFunc("OnChange")
		if hasSubmit or hasChange then
			local mst2 = {}
			if hasSubmit then
				mst2.onSubmit = function(name) self:OnSubmit(name) end
			end
			if hasChange then
				mst2.onTextChange = function(name, val) self:OnChange(name, val) end
			end
			bridge:TouchAllInputs(mst2)
		end
		return
	end

	-- ── 现有 UGUI / FairyGUI 分支（不变）─────────────────────────────
	if not self:IsFairyGui() or not self:IsFairyGuiWindow() then
		if IsValidObject(self.m_msgHandler) then
			return
		end

		local obj = self.m_panel
		if self:IsFairyGui() then
			obj = self.m_fguiOwner
		end
		self.m_msgHandler = obj:GetComponent(typeof(CS.UGFramework.Runtime.UILuaBehaviour))
		if not self.m_msgHandler then
			self.m_msgHandler = obj:AddComponent(typeof(CS.UGFramework.Runtime.UILuaBehaviour))
		end
		-- UGUI 后端：m_bridge 指向同一个 UILuaBehaviour
		self.m_bridge = self.m_msgHandler

		local mst = {
			onDestroy = function() self:DestroyPanelRaw() end,
			onBecameVisible = function(...) self:_BecameVisible(...) end,
			onBecameInvisible = function(...) self:_BecameInvisible(...) end,
		}
		local func = getFunc("OnClick")
		if func then
			mst.onClick = function(...) print("ui:click", ...) self:OnClick(...) end
		end 
		func = getFunc("OnSubmit")
		if func then
			mst.onSubmit = function(...) self:OnSubmit(...) end
		end 
		func = getFunc("OnChange")
		if func then
			mst.onTextChange = function(...) self:OnChange(...) end
		end 

		self.m_msgHandler:TouchGUIMsg(mst)

		if self.m_isfgui then
			local ui = self.m_panel
			if mst.onClick then
				ui:AddEventListener("onClick", mst.onClick)
			end
			if mst.onSubmit then
				ui:AddEventListener("onSubmit", mst.onSubmit)
			end
			if mst.onTextChange then
				ui:AddEventListener("onChanged", mst.onTextChange)
			end
		end
	else
		if self.m_msgHandler then
			return
		end

		local mst = {
			OnInit = function(...) print("OnInit", ...) end,
			OnShown = function(...) self:_BecameVisible(...) end,
			OnHide = function(...) self:_BecameInvisible(...) end,
		}
		self.m_msgHandler = mst

		if self.m_fguiOwner then
			print("TouchGUIMsg", self.m_panel, self.m_fguiOwner)
			local func = self:tryget("DoShowAnimation")
			if func and type(func) == "function" then
				mst.DoShowAnimation = function(...) print("DoShowAnimation", ...) end
			end 
			func = self:tryget("DoHideAnimation")
			if func and type(func) == "function" then
				mst.DoHideAnimation = function(...) print("DoHideAnimation", ...) end
			end 
			local ui = self.m_panel
			ui:SetLuaHandler(mst)
			if getFunc("OnClick") then
				ui:AddEventListener("onClick", function(...)
					print("ui:click", ...) self:OnClick(...) 
				end)
			end
			if getFunc("OnSubmit") then
				ui:AddEventListener("onSubmit", function(...)
					print("ui:input-submit", ...) self:OnSubmit(...) 
				end)
			end
			if getFunc("OnChange") then
				ui:AddEventListener("onChanged", function(...)
					print("ui:input-changed", ...) self:OnChange(...) 
				end)
			end
		end
	end
end

function FPanelBaseUI:_BecameVisible(...)
	print("BecameVisible", self, ...)
	self:OnShowInternal(true)
end

function FPanelBaseUI:_BecameInvisible(...)
	print("BecameInvisible", self, ...)
	self:OnShowInternal(false)
end

-------------------------------------------------------------
-----End Protected
-------------------------------------------------------------

function FPanelBaseUI:_BringTopReal()
	if not IsValidObject(self.m_panel) then return end
	self:_SetPanelToLayerMaxDepth()
end

function FPanelBaseUI:_SetPanelToLayerMaxDepth()
	self:_RemovePanelFromLayer()

	local real_depth = GetLayerNextTopDepth(self:GetDepthLayer())
	if self:IsUIToolkit() then
		-- UI Toolkit：通过 backend 设置 UIDocument.sortingOrder
		if self.m_backend and self.m_backend.IsValid then
			self.m_backend:SetSortingOrder(real_depth)
			self:_AddPanelToLayerDepth(real_depth)
		end
	elseif self:IsFairyGui() then
		if self:IsFairyGuiWindow() then
			self.m_panel:SetSortingOrder(real_depth, true)
			self:_AddPanelToLayerDepth(real_depth)
		else
			local panel = self.m_fguiOwner:GetComponent("UIPanel")
			panel:SetSortingOrder(real_depth, true)
			self:_AddPanelToLayerDepth(real_depth)
		end
	else
		local canvas = self.m_panel:GetComponent(typeof(UnityEngine.Canvas))
		if IsValidObject(canvas) then
			canvas.overrideSorting = true
			canvas.sortingOrder = real_depth
			self:_AddPanelToLayerDepth(real_depth)
		end
	end
end

function FPanelBaseUI:_SetPanelToLayerMinDepth()
	self:_RemovePanelFromLayer()

	local real_depth = GetLayerNextBottomDepth(self:GetDepthLayer())

	if self:IsUIToolkit() then
		if self.m_backend and self.m_backend.IsValid then
			self.m_backend:SetSortingOrder(real_depth)
			self:_AddPanelToLayerDepth(real_depth)
		end
	elseif self:IsFairyGui() then
		if self:IsFairyGuiWindow() then
			self.m_panel:SetSortingOrder(real_depth, true)
			self:_AddPanelToLayerDepth(real_depth)
		else
			local panel = self.m_fguiOwner:GetComponent("UIPanel")
			panel:SetSortingOrder(real_depth)
			self:_AddPanelToLayerDepth(real_depth)
		end
	else
		local canvas = self.m_panel:GetComponent(typeof(UnityEngine.Canvas))
		if IsValidObject(canvas) then
			canvas.overrideSorting = true
			canvas.sortingOrder = real_depth
			self:_AddPanelToLayerDepth(real_depth)
		end
	end
end
---@param real_depth integer
function FPanelBaseUI:_AddPanelToLayerDepth(real_depth)
	depthLayers[self:GetDepthLayer()].layerPanels[self] = real_depth
end

function FPanelBaseUI:_RemovePanelFromLayer()
	for _, layer_info in pairs(depthLayers) do
		layer_info.layerPanels[self] = nil
	end
end


---不要直接调用这个接口
---@param shouldVisible boolean
function FPanelBaseUI:_SetVisibleInner(shouldVisible)
	if self:IsSubView() then --有可能由于优化，将从PanelBase继承的页面作为一个View存在
		FViewBaseUI._SetVisibleInner(self, shouldVisible)
		return
	end
	local oldVisible = self:IsVisible()
	--播放动画中，变为隐藏状态了，直接销毁界面
	if not shouldVisible and self.m_disappearing then
		self:DestroyPanelImmediate()
		return
	end

	if self.m_panel then
		if self:IsUIToolkit() then
			-- UITK：通过 backend 控制，同步 rootVisualElement.display + GO active
			if self.m_backend and self.m_backend.IsValid then
				self.m_backend:SetVisible(shouldVisible)
			end
		elseif self:IsFairyGui() then
			if self.m_panel.displayObject then
				self.m_panel.displayObject.visible = shouldVisible
			elseif self.m_panel.gameObject then
				self.m_panel.gameObject.activeSelf = shouldVisible
			else
				printError("self.m_panel not valid >>>>>>>>>>>>>", self.m_panel)
			end
		else
			self.m_panel:SetActive(shouldVisible)
		end
	end

	local newVisible = self:IsVisible()
	if oldVisible ~= newVisible then
		if self.m_created then	--OnCreate 调用前不应调用 OnShow
			self:OnShowInternal(newVisible)
		end
	end
end

--设置界面不显示标志
---@param flag integer
---@param valid boolean
function FPanelBaseUI:_SetInvisibleFlagValidRaw(flag, valid)
	if valid then
		self.m_invisibleFlag = bit.bor(self.m_invisibleFlag, flag)
	else
		self.m_invisibleFlag = bit.band(bit.bnot(flag), self.m_invisibleFlag)
	end
end
---@param flag integer
---@param valid boolean
function FPanelBaseUI:SetInvisibleFlagValid(flag, valid)
	local oldFlag = self.m_invisibleFlag
	self:_SetInvisibleFlagValidRaw(flag, valid)
	if self.m_invisibleFlag == oldFlag then
		return
	end

	local currentVisible = (self.m_invisibleFlag == _G.PanelInVisibleMask.None)
	if self.m_SubPanelList then
		for _, subPanel in ipairs(self.m_SubPanelList) do
			subPanel:SetInvisibleFlagValid(_G.PanelInVisibleMask.ParentPanel, not currentVisible)
		end
	end

	self:UpdateVisibleByFlag()
end

function FPanelBaseUI:UpdateVisibleByFlag()
	--self:_UpdatePanelForTopFullscreen(self:_ShouldBeVisibleForTopFullscreen())
	
	self:_SetVisibleInner(self.m_invisibleFlag == _G.PanelInVisibleMask.None)
end
---@return boolean
function FPanelBaseUI:HasAnyInvisibleFlag()
	return self.m_invisibleFlag ~= 0
end
---@return boolean
function FPanelBaseUI:HasPrimaryInvisibleFlag()
	local primaryInVisibleMask = bit.band(self.m_invisibleFlag, _G.PanelInVisibleMask.SecondaryStart-1)
	return primaryInVisibleMask ~= 0
end
---@return boolean
function FPanelBaseUI:CheckInvisibleFlag(flag)
	return bit.band(flag, self.m_invisibleFlag) ~= 0
end


return FPanelBaseUI
