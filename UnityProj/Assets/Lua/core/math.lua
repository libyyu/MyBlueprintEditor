local math = math or {}

function math.HexToRGBA(v)
	if #v <6 then
		return nil
	end
	if v:sub(1,1)== '#' then
		v = v:sub(2, -1)
	end

	if not tonumber(v,16) then
		return nil
	end

	local hex = string.format("%02X",tonumber(v,16))

	local b = string.sub(hex,-2,-1)
	local g = string.sub(hex,-4,-3) == "" and "00" or string.sub(hex,-4,-3)
	local r = string.sub(hex,-6,-5) == "" and "00" or string.sub(hex,-6,-5)
	local a = string.sub(hex,-8,-7) == "" and "FF" or string.sub(hex,-8,-7)

	return tonumber('0x'..r) or 255, tonumber('0x'..g) or 255, tonumber('0x'..b) or 255, tonumber('0x'..a) or 255
end

function math.round(v)
	v = tonumber(v)
	if v >= 0 then
		return math.floor(v + 0.5)
	else
		return math.ceil(v - 0.5)
	end
end

function math.isNan(x)
	if x ~= x then
		return true
	end

	if type(x) ~= "number" then
		return false
	end

	if tostring(x) == tostring((-1)^0.5) then
		return true
	end

	return false
end

function math.clamp(value, range1, range2)
	local min = math.min(range1, range2)
	local max = math.max(range1, range2)
	if value < min then return min end
	if value > max then return max end
	return value
end

function math.inbetween(value, range1, range2)
	local min = math.min(range1, range2)
	local max = math.max(range1, range2)
	return value >= min and value <= max
end

-- 定义一个函数，将度数转换为弧度
function math.degreestoradians(degrees)
    return degrees * (math.pi / 180)
end

function math.radianstodegrees(radians)
    return radians*180/math.pi
end
