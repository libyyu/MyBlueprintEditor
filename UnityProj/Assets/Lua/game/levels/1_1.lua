-- game/levels/1_1.lua
-- 第一章第一关 — 新手引导关
--
-- 规则：
--   - 只有1根绳子，1刀即可通关
--   - 无时间限制
--   - 3星：1刀，2星：不可能（1刀=3星），1星：其他
--   - 显示新手提示文字

local M = {}

function M.on_ready(ctrl)
    print('[1_1] 新手关就绪，提示玩家切割绳子')
    -- TODO: 显示新手引导 UI
    -- CS.CutRope.Framework.UIManager.LuaOpen('UI/TutorialHint', '切断绳子喂给怪兽！', nil)
end

function M.on_tick(ctrl, dt)
    -- 无时间限制，不做任何处理
end

function M.on_cut(ctrl, cutCount, x, y)
    if cutCount == 1 then
        print('[1_1] 绳子切断，糖果自由落体中...')
        -- TODO: 收起引导 UI
    end
end

function M.calc_stars(ctrl)
    -- 第一关只有1根绳子，切1刀就是满星
    return 3
end

function M.calc_score(ctrl)
    -- 新手关固定高分，鼓励玩家
    local timeBonus = math.max(0, 500 - math.floor(ctrl.ElapsedTime * 20))
    return 1000 + timeBonus
end

function M.on_win(ctrl, stars, score)
    print(string.format('[1_1] 通关！score=%d 恭喜完成第一关！', score))
    -- TODO: 触发特效（糖果飞入怪兽嘴、怪兽开心动画）
    -- CS.CutRope.Game.GameEvent.Emit('candy_eaten')
end

function M.on_fail(ctrl)
    print('[1_1] 失败，糖果掉落')
end

return M
