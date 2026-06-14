---@type FGUIMan
local l_instance = nil
---@class FGUIMan
local FGUIMan = FLua.Class("FGUIMan")
do
	---@return FGUIMan
	function FGUIMan.Instance()
		if not l_instance then
			l_instance = FGUIMan()
		end
		return l_instance
	end
	function FGUIMan:__constructor()
		---@type GameObject|nil
		self.m_UIRoot = nil
		---@type Transform|nil
		self.m_UGUIRoot = nil
		---@type Transform|nil
		self.m_UITKRoot = nil
		---@type GameObject|nil
		self.m_FGUIRoot = nil

		self.m_panelSet = {}
		self.m_ObjToPanel = setmetatable({}, {__mode = "k"})
	end

	---@return Transform|nil
	function FGUIMan:GetUGUIRoot()
		return self.m_UGUIRoot
	end
	---@return Transform|nil
	function FGUIMan:GetUITKRoot()
		return self.m_UITKRoot
	end
	---@return GameObject|nil
	function FGUIMan:GetFGUIRoot()
		return self.m_FGUIRoot
	end

	function FGUIMan:InitUGUIRoot()
		if IsValidObject(self.m_UGUIRoot) then return end
		print("GUIMan:InitUGUIRoot")

		local goRoot = NewGameObject("UIRoot(2D)");
	    goRoot.transform.localPosition = Vector3(0, 0, 0);
	    goRoot.transform.localScale = Vector3(1, 1, 1);
	    goRoot.layer = UnityEngine.LayerMask.NameToLayer("UI");
	    goRoot.transform:SetParent(self.m_UIRoot.transform)

	    local cam = goRoot:AddComponent(typeof(UnityEngine.Camera))
	    cam.clearFlags = UnityEngine.CameraClearFlags.Depth
	    --cam.backgroundColor = Color(128,128,128,255)
	    cam.cullingMask = 1 << 5
	    cam.orthographic = true;
	    cam.orthographicSize = 3.2
	    cam.nearClipPlane = -10
	    cam.farClipPlane = 1000
	    cam.depth = 0

	    goRoot:AddComponent(typeof(UnityEngine.FlareLayer))
        --goRoot:AddComponent(LuaHelper.GetClsType("UnityEngine.GUILayer"));

        local goCanvas = NewGameObject("Canvas")
        goCanvas.layer = UnityEngine.LayerMask.NameToLayer("UI")

        goCanvas.transform:SetParent(goRoot.transform)
	    goCanvas.transform.localPosition = Vector3(0, 0, 0)
	    goCanvas.transform.localScale = Vector3(1, 1, 1)
	    local canvas = goCanvas:AddComponent(typeof(UnityEngine.Canvas))
	    canvas.renderMode = UnityEngine.RenderMode.ScreenSpaceCamera
	    canvas.pixelPerfect = true
	    canvas.worldCamera = cam

	    local canScaler = goCanvas:AddComponent(typeof(UnityEngine.UI.CanvasScaler));
	    canScaler.uiScaleMode = UnityEngine.UI.CanvasScaler.ScaleMode.ScaleWithScreenSize
	    canScaler.screenMatchMode = UnityEngine.UI.CanvasScaler.ScreenMatchMode.MatchWidthOrHeight
	    canScaler.referenceResolution = Vector2(750, 1344)

	    goCanvas:AddComponent(typeof(UnityEngine.UI.GraphicRaycaster))

        --Event Handle
		local goEvent = NewGameObject("EventSystem");
	    goEvent:AddComponent(typeof(EventSystems.EventSystem));
	    goEvent:AddComponent(typeof(EventSystems.StandaloneInputModule));
	    --goEvent:AddComponent(typeof(EventSystems.TouchInputModule));
	    goEvent.transform:SetParent(goRoot.transform)

	    self.m_UGUIRoot = goCanvas.transform
	end

	function FGUIMan:InitUITKRoot()
		if IsValidObject(self.m_UITKRoot) then return end

		local goRoot = NewGameObject("UITKRoot");
	    goRoot.transform.localPosition = Vector3(0, 0, 0);
	    goRoot.transform.localScale = Vector3(1, 1, 1);
	    goRoot.layer = UnityEngine.LayerMask.NameToLayer("UI");
	    goRoot.transform:SetParent(self.m_UIRoot.transform)
		self.m_UITKRoot = goRoot.transform
	end

	function FGUIMan:InitFGUIRoot()
		if IsValidObject(self.m_FGUIRoot) then return end

		CS.FairyGUI.StageCamera.LayerName = "FairyGUI"

		if UnityEngine.GameObject.Find("Stage Camera") then 
		else
			local camearGo = NewGameObject("Stage Camera");
			camearGo.transform.localPosition = Vector3(2.790179, -5, 0);
			camearGo.transform.localScale = Vector3(1, 1, 1);
			camearGo.layer = UnityEngine.LayerMask.NameToLayer("FairyGUI")

			local cam = camearGo:AddComponent(typeof(UnityEngine.Camera))
			cam.clearFlags = UnityEngine.CameraClearFlags.Depth
			--cam.backgroundColor = Color(128,128,128,255)
			cam.cullingMask = 1 << 20
			cam.orthographic = true
			cam.orthographicSize = 5
			cam.nearClipPlane = -30
			cam.farClipPlane = 30
			cam.depth = 1
			camearGo:AddComponent(typeof(CS.FairyGUI.StageCamera))
		end

	    local goRoot = NewGameObject("FGUIRoot(2D)");
	    goRoot.transform.localPosition = Vector3(0, 0, 0);
	    goRoot.transform.localScale = Vector3(1, 1, 1);
	    goRoot.layer = UnityEngine.LayerMask.NameToLayer("FairyGUI")
	    goRoot.transform:SetParent(self.m_UIRoot.transform)
		self.m_FGUIRoot = goRoot

		print("GUIMan:InitFGUIRoot", self.m_FGUIRoot)
		CS.FairyGUI.GRoot.inst:SetContentScaleFactor(750, 1344, CS.FairyGUI.UIContentScaler.ScreenMatchMode.MatchWidthOrHeight)
		CS.FairyGUI.Stage.inst.gameObject:GetComponent(typeof(CS.FairyGUI.UIContentScaler)).ignoreOrientation = true
		print("Stage width:", CS.FairyGUI.Stage.inst.width, CS.FairyGUI.Stage.inst.height)
		print("Screen width:", UnityEngine.Screen.width, UnityEngine.Screen.height)
	end

	function FGUIMan:InitUIRoot()
		if IsValidObject(self.m_UIRoot) then
			return
		end

		local tag = "UIRoot"
		print("GUIMan:InitUIRoot Tag = ", tag)

		local object = GameObject.FindGameObjectWithTag(tag)
		if IsValidObject(object) then
			self.m_UIRoot = object
			self.m_UGUIRoot = self.m_UIRoot.transform:Find("UIRoot(2D)/Canvas").transform
			self.m_UITKRoot = self.m_UIRoot.transform:Find("UITKRoot").transform
			self.m_FGUIRoot = self.m_UIRoot.transform:Find("FGUIRoot(2D)").gameObject
			return
		end
		if not IsValidObject(self.m_UIRoot) then
			self.m_UIRoot = NewGameObject("UIRootContainer")
		    self.m_UIRoot.transform.localPosition = Vector3(0, 0, 0)
		    self.m_UIRoot.transform.localScale = Vector3(1, 1, 1)
			self.m_UIRoot.tag = tag
		end
		self:InitUGUIRoot()
		self:InitUITKRoot()
		self:InitFGUIRoot()
	end

	---@param panel FPanelBaseUI
	function FGUIMan:RegisterPanel(panel)
		self.m_panelSet[panel] = true
	end
	---@param panel FPanelBaseUI
	function FGUIMan:UnRegisterPanel(panel)
		self.m_panelSet[panel] = nil
	end
	---@param panel FPanelBaseUI
	---@return boolean
	function FGUIMan:IsPanelRegistered(panel)
		return self.m_panelSet[panel] == true
	end

	function FGUIMan:EachPanel()
		return pairs(self.m_panelSet)
	end

	---@param obj GameObject
	---@param panel FPanelBaseUI
	function FGUIMan:RegisterPanelObj(obj, panel)
		if obj then self.m_ObjToPanel[obj] = panel end
	end
	---@param obj GameObject
	function FGUIMan:UnRegisterPanelObj(obj)
		if obj then self.m_ObjToPanel[obj] = nil end
	end

	---@param obj GameObject
	---@return FPanelBaseUI?
	function FGUIMan:GetPanelByObj(obj)
		return self.m_ObjToPanel[obj]
	end

	---@param assetName string
	---@param oncreate fun(panel:FPanelBaseUI):void
	---@param onclick fun(panel:FPanelBaseUI, ...):void
	---@return FPanelBaseUI
	function FGUIMan:CreateSimpleUI(assetName, oncreate, onclick)
		local M = FLua.Class(require "ui.FPanelBaseUI")
		do
			function M.OnCreate(panel)
				if oncreate then 
					oncreate(panel)
				end
			end
			if onclick then
				M.OnClick = function(panel, ...)
					onclick(panel, ...)
				end
			end
		end
		local panel = M()
		panel:CreatePanel(assetName)
		return panel
	end
end

return FGUIMan