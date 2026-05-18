-- game/levels/1_1.lua
-- 第一章第一关 — 场景级钩子
--
-- 职责：
--   只处理"这个场景独有、不适合蓝图表达"的内容
--   星级计算 / 通关失败 UI 编排 全由 blueprint_1_1.bjson 驱动
--
-- 蓝图节点对照：
--   on_win  → Level.CalcStars(MaxCuts=1) → Level.Complete → Timer.Wait → UI.Open(LevelComplete)
--   on_fail → UI.CloseAll → UI.Open(LevelFailed)
--   on_pause → Level.Pause

local M = {}

function M.on_ready(ctrl)
    -- 场景就绪：可在这里做关卡专属初始化
    -- 例如：设置物理参数、生成特殊道具、播放入场动画
    print('[1_1] Ready')
end

function M.on_tick(ctrl, dt)
    -- 第一关无时间限制，无需处理
    -- 有限时关卡在这里做倒计时 → 超时调 _G._PushLevelEvent('candy_failed')
end

return M
