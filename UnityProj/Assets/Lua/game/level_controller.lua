-- game/level_controller.lua
-- 关卡逻辑 Lua 层 — 连接 C# LevelController 与 LevelManager
--
-- 在关卡场景加载完成后，由 main.lua 的 LM.on_level_loaded 回调中 require 并初始化。
--
-- 用法：
--   local LC = require 'game/level_controller'
--   LC.init()   -- 绑定当前场景的 LevelController，注入钩子

local LM   = require 'game/level_manager'
local Save = require 'game/save_manager'

local M = {}

-- C# LevelController 实例（关卡场景内唯一）
local _ctrl = nil

--- 初始化：绑定 C# LevelController，注入所有事件钩子
-- 必须在关卡场景加载完成后调用
function M.init()
    _ctrl = CS.CutRope.Game.LevelController.Current
    if _ctrl == nil then
        print('[level_controller] ERROR: LevelController.Current is nil. Is it in the scene?')
        return
    end

    -- 关卡开始
    _ctrl.OnLevelStart = function()
        print('[level_controller] Level started: ' .. (_ctrl.levelId or '?'))
        M.on_start()
    end

    -- 通关
    _ctrl.OnLevelComplete = function(stars, score)
        print(string.format('[level_controller] Complete! stars=%d score=%d', stars, score))
        M.on_complete(stars, score)
    end

    -- 失败
    _ctrl.OnLevelFailed = function()
        print('[level_controller] Failed!')
        M.on_failed()
    end

    -- 每次切割
    _ctrl.OnCut = function(cutCount)
        print('[level_controller] Cut #' .. cutCount)
        M.on_cut(cutCount)
    end

    print('[level_controller] Initialized for: ' .. (_ctrl.levelId or '?'))
end

-- ── 事件处理（可在外部覆盖）──────────────────────────────────────────────

function M.on_start()
    -- TODO: 显示关卡 HUD（剩余刀数提示等）
end

function M.on_complete(stars, score)
    -- 通知 LevelManager（负责存档、解锁下一关）
    LM.complete(stars, score)

    -- TODO: 显示通关面板（星级动画、分数等）
    -- local UI = require 'ui/ui_manager'
    -- UI.open('UI/LevelComplete', ...)
end

function M.on_failed()
    LM.fail()
    -- TODO: 显示失败面板
    -- local UI = require 'ui/ui_manager'
    -- UI.open('UI/LevelFailed', ...)
end

function M.on_cut(cutCount)
    -- TODO: 播放切割音效/粒子（通过 GameEvent 通知 Blueprint）
end

--- 获取当前 C# LevelController
function M.get_ctrl()
    return _ctrl
end

return M
