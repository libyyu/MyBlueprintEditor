-- game/levels/default.lua
-- 默认关卡规则 — 找不到专属脚本时使用
--
-- 通关条件：糖果被怪兽吃到（由 C# Candy 检测，Lua 不参与）
-- 星级：1刀=3星，2刀=2星，其余1星
-- 分数：基础1000，每多1刀-200，每秒-10

local M = {}

function M.on_ready(ctrl)
    print('[default] Level ready. Cuts to 3-star: 1')
end

function M.on_tick(ctrl, dt)
    -- 默认规则无限时，不需要处理
end

function M.on_cut(ctrl, cutCount, x, y)
    print(string.format('[default] Cut #%d at (%.2f, %.2f)', cutCount, x, y))
end

function M.calc_stars(ctrl)
    local cuts = ctrl.CutCount
    if cuts <= 1 then return 3 end
    if cuts <= 2 then return 2 end
    return 1
end

function M.calc_score(ctrl)
    local base  = 1000
    local cuts  = math.max(0, ctrl.CutCount - 1) * 200
    local time  = math.floor(ctrl.ElapsedTime * 10)
    return math.max(0, base - cuts - time)
end

function M.on_win(ctrl, stars, score)
    print(string.format('[default] Win! stars=%d score=%d', stars, score))
end

function M.on_fail(ctrl)
    print('[default] Fail!')
end

return M
