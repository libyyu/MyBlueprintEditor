-- timer.lua
-- 计时器封装模块
--
-- 依赖：挂载在场景根 GameObject 上的 FTimerListBehavior（C#）
-- 通过 GameLauncher 的 [RequireComponent(typeof(FTimerListBehavior))] 自动挂载
--
-- 使用示例：
--   local timer = require "timer"
--
--   -- 3 秒后执行一次
--   local id = timer.after(3.0, function() print("hello") end)
--
--   -- 每 1 秒循环执行，透传参数表
--   local id2 = timer.every(1.0, function(p) print(p.msg) end, { msg = "tick" })
--
--   -- 取消
--   timer.cancel(id2)
--
--   -- 重置（重新计时）
--   timer.reset(id)
--
-- 说明：
--   - callback 签名：function(cbparam)，cbparam 为传入的 table，未传则为 nil
--   - bLateUpdate 默认 false（在 Update 阶段触发）

local M = {}

-- 缓存 FTimerListBehavior 组件（懒初始化）
local _behavior = nil

local function getBehavior()
    if _behavior ~= nil then return _behavior end

    -- 从场景中查找挂载了 FTimerListBehavior 的 GameObject
    -- GameLauncher 上已 RequireComponent，直接找 GameLauncher 对象即可
    local go = CS.UnityEngine.GameObject.Find("GameLauncher")
    if go == nil then
        error("[timer] GameLauncher GameObject not found in scene")
    end
    _behavior = go:GetComponent(typeof(CS.FTimerListBehavior))
    if _behavior == nil then
        error("[timer] FTimerListBehavior not found on GameLauncher")
    end
    return _behavior
end

--- 延迟 ttl 秒后触发一次回调
---@param ttl number 秒数
---@param callback function 回调函数 function(cbparam)
---@param cbparam table|nil 透传给回调的参数表
---@param bLateUpdate boolean|nil 是否在 LateUpdate 触发，默认 false
---@return number timer_id
function M.after(ttl, callback, cbparam, bLateUpdate)
    assert(type(callback) == "function", "[timer.after] callback must be a function")
    local b = getBehavior()
    return b:AddTimer(ttl, true, callback, cbparam or nil, bLateUpdate or false)
end

--- 每隔 ttl 秒循环触发回调
---@param ttl number 秒数
---@param callback function 回调函数 function(cbparam)
---@param cbparam table|nil 透传给回调的参数表
---@param bLateUpdate boolean|nil 是否在 LateUpdate 触发，默认 false
---@return number timer_id
function M.every(ttl, callback, cbparam, bLateUpdate)
    assert(type(callback) == "function", "[timer.every] callback must be a function")
    local b = getBehavior()
    return b:AddTimer(ttl, false, callback, cbparam or nil, bLateUpdate or false)
end

--- 取消计时器
---@param id number AddTimer 返回的 ID
function M.cancel(id)
    if id == nil then return end
    local b = getBehavior()
    b:RemoveTimer(id)
end

--- 重置计时器（重新开始倒计时）
---@param id number AddTimer 返回的 ID
function M.reset(id)
    if id == nil then return end
    local b = getBehavior()
    b:ResetTimer(id)
end

return M
