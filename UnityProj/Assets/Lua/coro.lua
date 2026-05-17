--if _G.MarkNotReloadAfterUpdate then _G.MarkNotReloadAfterUpdate() end

_G.coro = {}
local coroTable = {}
local coroToRun = {}
local stopTag = "stop"

local function newCoro()
	local coro
	coro = coroutine.create(function()
		while true do
			local func = coroToRun[coro]
			if func then func() end
			coroutine.yield(stopTag)
		end
	end)
	return coro
end

function _G.coro.start(func)
	local coroIndexToReplace
	for index, coro in ipairs(coroTable) do
		if coroutine.status(coro) == "dead" then
			coroIndexToReplace = index
			break
		end
		if coroToRun[coro] == nil then
			coroToRun[coro] = func
			return
		end
	end
	
	local coro = newCoro()
	if coroIndexToReplace then
		coroTable[coroIndexToReplace] = coro
	else
		table.insert(coroTable, coro)
	end
	coroToRun[coro] = func
end

function _G.coro.yield()
	return coroutine.yield()
end

function _G.coro.wait(milliseconds)
	local startTime = UnityEngine.Time.realtimeSinceStartup
	while UnityEngine.Time.realtimeSinceStartup - startTime < milliseconds / 1000 do
		coroutine.yield()
	end
end
function _G.coro.waitSeconds(seconds)
	local startTime = UnityEngine.Time.realtimeSinceStartup
	while UnityEngine.Time.realtimeSinceStartup - startTime < seconds do
		coroutine.yield()
	end
end

function _G.coro.clear()
	coroTable = {}
end


--local promises = require('promises')

function _G.TickCoroutine(DeltaTime)
	for coro, func in pairs(coroToRun) do
		local ret1, ret2 = coroutine.resume(coro)
		if (not ret1) or (ret2 == stopTag) then
			coroToRun[coro] = nil
			if not ret1 then
				printError(debug.traceback(coro, ret2, 0))
			end
		end
	end
end
