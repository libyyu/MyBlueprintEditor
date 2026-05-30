
local YooAssetsLuaBridge = CS.UGFramework.Runtime.YooAssetsLuaBridge


local l_instance = nil
---@class FAssetBundleUtil
local FAssetBundleUtil = FLua.Class("FAssetBundleUtil")
do
	function FAssetBundleUtil:__constructor()
	end
	function FAssetBundleUtil.Instance()
		if not l_instance then
			l_instance = FAssetBundleUtil()
		end
		return l_instance
	end

	local function splitPackageAndAssetName(fullName, rep)
		-- TODO: 实现分割包名和资产名的逻辑
		rep = rep or DefaultPackageSeparator
		if fullName:find(rep) then
			local arr = fullName:split(rep)
			if #arr == 2 then
				return arr[1], arr[2]
			end
		end
		return DefaultPackageName, fullName
	end

	function FAssetBundleUtil:AsyncLoadAsset(inAssetName, cb)
		print("AsyncLoad:", inAssetName)
		local packageName, assetName = splitPackageAndAssetName(inAssetName)
		YooAssetsLuaBridge.LoadAsset(packageName, assetName, function(success, obj, err)
			if not success then
				warn("Failed to load asset:", inAssetName, "Error:", err)
				if cb then cb(nil) end
				return
			end
			if cb then cb(obj) end
		end)
	end
end

return FAssetBundleUtil