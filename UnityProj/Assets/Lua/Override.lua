local _behavior = nil

local function getBehavior()
    if _behavior ~= nil then return _behavior end

    -- 从场景中查找挂载了 FTimerListBehavior 的 GameObject
    -- GameLauncher 上已 RequireComponent，直接找 GameLauncher 对象即可
    local go = CS.UnityEngine.GameObject.Find("GameLauncher")
    if go == nil then
        error("[timer] GameLauncher GameObject not found in scene")
    end
    _behavior = go:GetComponent(typeof(CS.UGFramework.Runtime.FTimerListBehavior))
    if _behavior == nil then
        error("[timer] FTimerListBehavior not found on GameLauncher")
    end
    return _behavior
end

do --System.Object
    print("Override System.Object")
	local __lua_userdata = {}
	setmetatable(__lua_userdata, {__mode = "k"})
	local mt = GameUtil.GetMetaTable("System.Object")
	function mt:SetLuaUserData(key, value)
		local t = rawget(__lua_userdata, self)
		if not t then
			t = {}
			rawset(__lua_userdata, self, t)
		end
		t[key] = value
	end
	function mt:GetLuaUserData(key)
		local t = rawget(__lua_userdata, self)
		if not t then
			return nil
		end
		return t[key]
	end

    function mt:IsExtend(t)
        if type(t) == "string" then
            return GameUtil.IsExtendByName(self, t)
        end
        return GameUtil.IsExtend(self, t)
    end
end

do --UnityEngine.GameObject
    print("Override UnityEngine.GameObject")
	local mt = GameUtil.GetMetaTable("UnityEngine.GameObject")
	function mt:FindDirect(name)
		if name == "." or name == "" then
			return self
		end
		name = name:replace('.', '/', {plain=true})
		local sb = name:split('/')

		local child = self.transform
		for _, n in ipairs(sb) do
			child = child:Find(n)
			if not child then
				return nil
			end
		end
		return child.gameObject
	end
	function mt:GetVisible()
		return self.activeSelf
	end

	local mt2 = GameUtil.GetMetaTable("UnityEngine.UIElements.VisualElement")
	if mt2 then
		print("Override UnityEngine.UIElements.VisualElement")
		function mt2:FindDirect(name)
			if name == "." or name == "" then
				return self
			end
			name = name:replace('.', '/', {plain=true})
			return self:Q(name)
		end
		function mt2:GetVisible()
			return self.style.display.value ~= 0
		end
		function mt2:SetVisible(visible)
			self.style.display.value = visible and CS.UnityEngine.UIElements.DisplayStyle.Flex or CS.UnityEngine.UIElements.DisplayStyle.None
		end
	end

	local mt3 = GameUtil.GetMetaTable("UGFramework.Runtime.UGUIPanelBackend")
	if mt3 then
		print("Override UGFramework.Runtime.UGUIPanelBackend", mt3, getmetatable(CS.UGFramework.Runtime.UGUIPanelBackend(nil)))
		function mt3:FindDirect(name)
			---@type GameObject
			local go = self.RootGameObject
			if go then
				return go:FindDirect(name)
			end
		end
	end
	local mt4 = GameUtil.GetMetaTable("UGFramework.Runtime.UITKPanelBackend")
	if mt4 then
		print("Override UGFramework.Runtime.UITKPanelBackend")
		function mt4:FindDirect(name)
			---@type VisualElement
			local vt = self.RootVisualElement
			if vt then
				return vt:FindDirect(name)
			end
		end
	end

	local mt5 = GameUtil.GetMetaTable("FairyGUI.GComponent")
	if mt5 then
		print("Override FairyGUI.GComponent")
		function mt5:FindDirect(name)
			--TODO: fairygui
		end
		function mt5:GetVisible()
			--TODO: fairygui
		end
	end
end

do--UnityEngine.UI.Slider
    print("Override UnityEngine.UI.Slider")
	local mt = GameUtil.GetMetaTable("UnityEngine.UI.Slider")
	function mt:AutoProgress(time, start, to, onfinish)
		local pretimer = self:GetLuaUserData("AutoProgress")
		if pretimer then
			GameUtil.RemoveObjectTimer(self.gameObject, pretimer)
			self:SetLuaUserData("AutoProgress", nil)
		end

		local minValue = self.minValue
		local maxValue = self.maxValue
		to = math.min(to or 0, 100)
		start = math.min(start or 0, 100)
		start = math.max(self.normalizedValue*100, start)
		print("AutoProgress", time, start, to)
		if not time or time == 0 then time = 0.1 end 
		local last = UnityEngine.Time.realtimeSinceStartup
		local speed = (to - start) / time
		local target = 0
		local idTimer = 0
		local currentProgress = start
		local UPDATE_INTERVAL = 0.2
		self.normalizedValue = currentProgress/100
		--idTimer = GameUtil.AddGlobalTimer(0.1, false, function()
		idTimer = GameUtil.AddObjectTimer(self.gameObject, 0.1, false, function()
			if not IsValidObject(self) then
				return
			end
			--print("tick pro:", currentProgress, to)
			if currentProgress >= to then
				GameUtil.RemoveObjectTimer(self.gameObject, idTimer)
				self:SetLuaUserData("AutoProgress", nil)
				idTimer = 0
				if onfinish then onfinish() end
				return
			end
			local newProgress = math.min(currentProgress + speed * (UnityEngine.Time.realtimeSinceStartup - last), to)
			self.normalizedValue = newProgress/100
			currentProgress = newProgress
			last = UnityEngine.Time.realtimeSinceStartup
		end)

		self:SetLuaUserData("AutoProgress", idTimer)
	end
end