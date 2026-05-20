-- game/levels/1_1.lua
-- 第一章第一关 — 场景级钩子
--
-- 关卡设计：
--   - 2 根绳子（左右各一），需要找到正确顺序切
--   - 3 颗星星散布在绳子路径上，经过时收集
--   - 1 个移动障碍物（蓝图启动后开始移动）
--   - 星级 = 收集到的星星数（0~3星）

local M = {}

-- 安全读取 XLua 可能未暴露的 C# 属性
local function safeGet(fn, default)
    local ok, v = pcall(fn)
    return (ok and v ~= nil) and v or (default or 0)
end

function M.on_ready(ctrl)
    local ropeCount = ctrl:GetRopeCount()
    local starCount = safeGet(function() return ctrl.StarCount end, 0)
    print('[1_1] Ready — ropes=' .. ropeCount .. ' stars=' .. starCount)
end

function M.on_tick(ctrl, dt)
    -- 第一关无倒计时
end

--- 通关星级 = 收集到的星星数（至少1星）
function M.calc_stars(ctrl)
    local collected = safeGet(function() return ctrl.StarsCollected end, 0)
    return math.max(1, collected)
end

function M.calc_score(ctrl)
    local collected = safeGet(function() return ctrl.StarsCollected end, 0)
    local cuts      = safeGet(function() return ctrl.CutCount end, 0)
    return math.max(0, 100 + collected * 50 - cuts * 10)
end

function M.on_win(ctrl, stars, score)
    print(string.format('[1_1] WIN! stars=%d score=%d', stars or 0, score or 0))
end

function M.on_fail(ctrl)
    print('[1_1] FAIL — candy missed')
end

return M
