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
local SM      = require 'game.scene_manager'
local LM      = require 'game.level_manager'
local FGUIMan = require 'ui.FGUIMan'

-- ── 4. 场景加载完成钩子（负责打开对应 UI）────────────────────────────────
-- scene_manager 每次加载完场景都会回调，参数为完整 address 字符串
SM.on_scene_loaded = function(sceneName)
    print('[GameLogic] scene loaded: ' .. sceneName)
    -- ── 3. 初始化 UI Root ────────────────────────────────────────────────────
    FGUIMan.Instance():InitUIRoot(true)
    -- 只取末段名字，避免路径前缀干扰
    local name = sceneName:match("([^/]+)$") or sceneName
    if name == 'MainMenu' or name == 'main_menu' then
        --require 'ui.FPanelMainMenu'.Instance():ShowPanel(true)
        -- UI 框架测试面板：UITK 版（默认）/ UGUI 版（同一套逻辑，换后端）
        require 'ui.FPanelUITestUITK'.Instance():ShowPanel(true)
        --require 'ui.FPanelUITestUGUI'.Instance():ShowPanel(true)
    end
end

-- ── 5. 注入关卡生命周期钩子 ──────────────────────────────────────────────
LM.on_level_loaded = function(levelId)
    print('[GameLogic] on_level_loaded: ' .. levelId)
    -- HUD 由蓝图 OnBeginPlay 节点开启，这里只停关卡 controller
    local ok2, err2 = pcall(function()
        local LC = require 'game.level_controller'
        LC.init(levelId)
    end)
    if not ok2 then
        print('[GameLogic] level_controller init error: ' .. tostring(err2))
    end
end

LM.on_level_complete = function(levelId, stars, score)
    -- UI 由蓝图 on_win 事件处理，这里只记日志
    print(string.format('[GameLogic] on_level_complete: %s  stars=%d  score=%d',
        levelId, stars or 0, score or 0))
end

LM.on_level_failed = function(levelId)
    -- UI 由蓝图 on_fail 事件处理
    print('[GameLogic] on_level_failed: ' .. levelId)
end

-- ── 6. 跳转主菜单，启动游戏 ──────────────────────────────────────────────
print('[GameLogic] goto main menu...')
SM.goto_main_menu()

-- ── 7. 全局生命周期钩子（LuaManager 回调）───────────────────────────────
function onAppTick(dt)
    TickCoroutine(dt)
end

function onAppDestroy()
    print('[GameLogic] onAppDestroy')
    coro.clear()
end

print("[GameLogic] *** Game phase ready ***")
