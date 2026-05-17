-- GameLogic.lua
-- 游戏主逻辑入口（Phase 4）
-- 由 LuaManager.RunGameLuaVM("GameLogic") 加载

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
local FGUIMan = require 'ui.FGUIMan'

-- ── 3. 初始化 UI Root ────────────────────────────────────────────────────
FGUIMan.Instance():InitUIRoot()

-- ── 4. 注入关卡生命周期钩子 ──────────────────────────────────────────────

LM.on_level_loaded = function(levelId)
    print('[GameLogic] on_level_loaded: ' .. levelId)
    -- 显示 HUD
    require 'ui.FPanelHUD'.Instance():ShowPanel(true)
    -- 初始化关卡逻辑
    local ok2, err2 = pcall(function()
        local LC = require 'game.level_controller'
        LC.init(levelId)
    end)
    if not ok2 then
        print('[GameLogic] level_controller init error: ' .. tostring(err2))
    end
end

LM.on_level_complete = function(levelId, stars, score)
    print(string.format('[GameLogic] on_level_complete: %s  stars=%d  score=%d', levelId, stars or 0, score or 0))
    local panel = require 'ui.FPanelLevelComplete'.Instance()
    panel:ShowPanel(true)
    panel:SetResult(levelId, score)
end

LM.on_level_failed = function(levelId)
    print('[GameLogic] on_level_failed: ' .. levelId)
    local panel = require 'ui.FPanelLevelFailed'.Instance()
    panel:ShowPanel(true)
    panel:SetLevelId(levelId)
end

-- ── 5. 跳转主菜单，启动游戏 ──────────────────────────────────────────────
-- 场景加载完后由 scene_manager 回调，在 MainMenu 场景里打开主菜单面板
SM.on_scene_loaded = function(sceneName)
    print('[GameLogic] scene loaded: ' .. sceneName)
    if sceneName == 'MainMenu' or sceneName == 'main_menu' then
        require 'ui.FPanelMainMenu'.Instance():ShowPanel(true)
    end
end

print('[GameLogic] goto main menu...')
SM.goto_main_menu()

-- ── 6. 全局生命周期钩子（LuaManager 回调）───────────────────────────────

function onAppTick(dt)
    TickCoroutine(dt)
end

function onAppDestroy()
    print('[GameLogic] onAppDestroy')
    coro.clear()
end

print("[GameLogic] *** Game phase ready ***")
