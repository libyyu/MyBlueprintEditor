
---@class Utils
local Utils = FLua.StaticClass("Utils")

function Utils.Value2String(sth, inIndent)
	local indent = inIndent
	if type(indent) ~= "number" then
		indent = 0
	elseif indent < 0 then
		indent = 0
	elseif indent > 10 then
		indent = 10
	end
	local startBlank = string.rep(' ', indent)
	if type(sth) ~= "table" then
 		return startBlank .. tostring(sth)
 	else
 		local mt = getmetatable(sth)
		if mt and mt.__tostring then
			return startBlank .. tostring(sth)
		end
 	end
 	local function make_key(k)
		if type(k) == "number" then
			return "[" .. tostring(k) .. "]"
		else
			return tostring(k)
		end
	end
	local function make_value(v)
		if type(v) == "string" then
			return "\"" .. v .. "\""
		else
			return tostring(v)
		end
	end

	local recorded = {}
	local sb = {}
	local blankSpace = inIndent and ' ' or ''
	local space, deep = string.rep(blankSpace, 4), 0
	--space = "\t" .. space
	local function _dump(t, level, prefixKey)
		if recorded[t] then
			if prefixKey then
				sb[#sb+1] = string.rep(space, level+1) .. '"'.. ("@set previous: `%s`"):format(tostring(prefixKey)) .. '"'
			end
			return
		end
		recorded[t] = true

		if level > 20 then
			sb[#sb+1] = string.rep(space, level+1) .. '"'.. "Table Print Stack ToDeep!!!" .. '"'
			return
		end

		local mt = getmetatable(t)
		if mt and mt.__pMessage and mt.__pDescriptor then--是一个pb协议
			local v = tostring(t)
			local arr = v:split("\n")
			for _, v in ipairs(arr) do
				sb[#sb+1] = string.rep(space, level+1) .. tostring(v)
			end
		elseif mt and mt.__tostring then
			sb[#sb+1] = string.rep(space, level+1) .. tostring(t)
		else
			local dumpKeyNum = 0
			for key, v in pairs(t) do
				dumpKeyNum = dumpKeyNum + 1
				if dumpKeyNum ~= 1 then
					sb[#sb] = sb[#sb] .. ","
				end
				if type(v) == "table" then
					sb[#sb+1] = string.format("%s%s%s=%s{", string.rep(space, level+1 ), make_key(key), blankSpace, blankSpace)
					_dump(v, level+1, prefixKey and (prefixKey .. "." .. key) or key)
					sb[#sb+1] = string.format("%s}", string.rep(space, level+1))
				else
					sb[#sb+1] = string.format("%s%s%s=%s%s", string.rep(space, level+1), make_key(key), blankSpace, blankSpace, make_value(v))
				end
			end
		end
	end

	sb[#sb+1] = "{"
	_dump(sth, deep)
	sb[#sb+1] = "}"
	local sepJoin = inIndent and "\n" or ""
 	return table.concat(sb, indent > 0 and ('\n' .. startBlank) or sepJoin)
end

function Utils.printValue(...)
	local sb = {}
	for _,v in ipairs({...}) do
		sb[#sb+1] = Utils.Value2String(v)
	end
	print(table.unpack(sb))
end

return Utils