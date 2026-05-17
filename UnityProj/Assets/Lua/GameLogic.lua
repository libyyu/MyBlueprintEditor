-- GameLogic.lua
-- 游戏主逻辑入口（Phase 4）
-- 由 LuaManager.RunGameLuaVM("GameLogic") 加载
--
-- 职责：
--   1. 注册蓝图节点（BlueprintEntry）
--   2. 注入关卡事件钩子（level_manager）
--   3. 跳转主菜单，开始游戏循环
--   4. 暴露 onAppTick / onAppDestroy 供 LuaManager 驱动

print("[GameLogic] *** Game phase start ***")
require "preload"


-- ── 1. 注册蓝图节点 ──────────────────────────────────────────────────────
local ok, err = pcall(require, 'blueprints.BlueprintEntry')
if not ok then
    print('[GameLogic] Warning: BlueprintEntry load failed: ' .. tostring(err))
end

-- ── 2. 引入核心模块 ──────────────────────────────────────────────────────
local SM = require 'game.scene_manager'
local LM = require 'game.level_manager'

-- ── 3. 注入关卡生命周期钩子 ──────────────────────────────────────────────

LM.on_level_loaded = function(levelId)
    print('[GameLogic] on_level_loaded: ' .. levelId)
    local ok2, err2 = pcall(function()
        local LC = require 'game/level_controller'
        LC.init(levelId)
    end)
    if not ok2 then
        print('[GameLogic] level_controller init error: ' .. tostring(err2))
    end
end

LM.on_level_complete = function(levelId, stars, score)
    print(string.format('[GameLogic] on_level_complete: %s  stars=%d  score=%d', levelId, stars, score))
    local ok2, err2 = pcall(function()
        local UI = require 'ui/ui_manager'
        UI.open('UI/LevelComplete')
    end)
    if not ok2 then
        print('[GameLogic] open LevelComplete UI error: ' .. tostring(err2))
    end
end

LM.on_level_failed = function(levelId)
    print('[GameLogic] on_level_failed: ' .. levelId)
    local ok2, err2 = pcall(function()
        local UI = require 'ui/ui_manager'
        UI.open('UI/LevelFailed')
    end)
    if not ok2 then
        print('[GameLogic] open LevelFailed UI error: ' .. tostring(err2))
    end
end

-- ── 4. 跳转主菜单，启动游戏 ──────────────────────────────────────────────
print('[GameLogic] current scene: ' .. SM.current())
SM.goto_main_menu()

-- ── 5. 全局生命周期钩子（LuaManager 回调）───────────────────────────────

--- 每帧驱动（由 LuaManager.Update 调用）
function onAppTick(dt)
    -- 目前帧逻辑全部由 C# MonoBehaviour 驱动
    -- 如果后续需要 Lua 侧帧更新（Tween、AI Tick 等），在这里加
end

--- 销毁时清理
function onAppDestroy()
    print('[GameLogic] onAppDestroy')
    -- LM / SM / UI 的清理由各自模块的 OnDestroy 或 GC 处理
end

print("[GameLogic] *** Game phase ready ***")
