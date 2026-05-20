-- game/levels/1_1.lua
-- 第一章第一关 — 场景级钩子
--
-- 关卡设计：
--   - 2 根绳子（左右各一），需要找到正确顺序切
--   - 3 颗星星散布在绳子路径上，经过时收集
--   - 1 个移动障碍物（蓝图启动后开始移动）
--   - 星级 = 收集到的星星数（0~3星）

local M = {}

function M.on_ready(ctrl)
    print('[1_1] Ready — ropes=' .. ctrl:GetRopeCount() .. ' stars=' .. ctrl.StarCount)
end

function M.on_tick(ctrl, dt)
    -- 第一关无倒计时
end

--- 通关星级 = 收集到的星星数
function M.calc_stars(ctrl)
    return math.max(1, ctrl.StarsCollected)  -- 至少 1 星（能喂到就给1星）
end

function M.calc_score(ctrl)
    -- 基础分 100，每颗星 +50，每刀 -10
    return math.max(0, 100 + ctrl.StarsCollected * 50 - ctrl.CutCount * 10)
end

function M.on_win(ctrl, stars, score)
    print(string.format('[1_1] WIN! stars=%d score=%d cuts=%d', stars, score, ctrl.CutCount))
end

function M.on_fail(ctrl)
    print('[1_1] FAIL — candy missed')
end

return M
