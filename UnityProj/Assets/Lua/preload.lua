---@diagnostic disable: 131

UnityEngine = CS.UnityEngine
GameObject = UnityEngine.GameObject
EventSystems = UnityEngine.EventSystems
Vector3 = UnityEngine.Vector3
Vector2 = UnityEngine.Vector2
GameUtil = CS.CutRope.Framework.GameUtil

function NewGameObject(name, parent)
	if type(name) == "string" then
		local go = GameObject(name)
		if parent then
			go.transform:SetParent(parent.transform)
		end
		return go
	elseif not name then
		local go = GameObject()
		if parent then
			go.transform:SetParent(parent.transform)
		end
		return go
	else
		error("constructor GameObject param 1 is expected nil or string")
	end
end

function Instantiate(go, name, parentTr)
	local obj = parentTr and UnityEngine.Object.Instantiate(go, parentTr) or UnityEngine.Object.Instantiate(go)
	if type(name) == "string" then
		obj.name = name
	end
	return obj
end

function DestroyObject(go)
	UnityEngine.Object.Destroy(go)
end

function DontDestroyOnLoad(go)
	UnityEngine.Object.DontDestroyOnLoad(go)
end

---@return boolean
function IsValidObject(obj)
	return obj ~= nil and obj:Equals(nil) == false
end

---@return boolean
function IsWebGLRuntime()
	return not GameUtil.IsEditorEnv() and (UnityEngine.RuntimePlatform.WebGLPlayer == UnityEngine.Application.platform or GameUtil.IsWebGLEnv())
end

function IsWXRuntime()
	return IsWebGLRuntime() and GameUtil.IsWXEnv()
end

function IsStandaloneRuntime()
	return UnityEngine.RuntimePlatform.WindowsPlayer == UnityEngine.Application.platform 
		or UnityEngine.RuntimePlatform.OSXPlayer == UnityEngine.Application.platform 
		or UnityEngine.RuntimePlatform.LinuxPlayer == UnityEngine.Application.platform
end

---加载资源
---@param assetName string
---@param cb fun(obj:any)
function AsyncLoad(assetName, cb)
	if type(assetName) ~= "string" then
		error(("argument #%d expected string, but got %s"):format(1, type(assetName)))
	end
	local FAssetBundleUtil = require "utility.FAssetBundleUtil"
    FAssetBundleUtil.Instance():AsyncLoadAsset(assetName, cb)
end

function AsyncLoadArray(assetNames, cb)
	if type(assetNames) ~= "table" then
		error(("argument #%d expected table, but got %s"):format(2, type(assetNames)))
	end

	local results = {}
	local count = #assetNames
	local finishnum = 0
	for i=1, #assetNames do
		AsyncLoad(assetNames[i], function(obj)
			results[i] = obj
			finishnum = finishnum + 1

			if finishnum == count then
				cb(results)
			end
		end)
	end
end

function HasLuaScript(luaRequiredPath)
    return CS.YooAssetsLuaBridge.HasLuaFile(luaRequiredPath)
end

DefaultPackageSeparator = ":"
DefaultFGUISeparator = "@"
DefaultPackageName = "DefaultPackage"
PlatformSuffix = IsWebGLRuntime() and "WebGL" or "App"
print("PlatformSuffix", PlatformSuffix, "IsWebGLRuntime", IsWebGLRuntime(), "IsWXRuntime", IsWXRuntime())

require "coro"
require "core.preload"
require "FLua"
require "utility.Enum"
