-- main.lua
-- 游戏 Lua 入口，由 LuaManager 从 YooAsset 加载后执行

print("main.lua loaded!")

local SM = require 'game/scene_manager'
local LM = require 'game/level_manager'

-- ── 全局钩子（供 LuaManager 调用）────────────────────────────────────

--- 每帧调用（由 LuaManager.Update 驱动）
function update()
    -- 后续游戏逻辑挂在这里
end

--- 销毁时调用
function on_destroy()
    print("on_destroy called")
end

-- ── 注入关卡事件钩子 ──────────────────────────────────────────────────
LM.on_level_loaded = function(levelId)
    print('[main] level loaded: ' .. levelId)
    local LC = require 'game/level_controller'
    LC.init()  -- 绑定 C# LevelController，注入通关/失败/切割钩子
end

LM.on_level_complete = function(levelId, stars, score)
    print(string.format('[main] level complete: %s  stars=%d  score=%d', levelId, stars, score))
    -- TODO: 显示通关面板
end

LM.on_level_failed = function(levelId)
    print('[main] level failed: ' .. levelId)
    -- TODO: 显示失败面板
end

-- ── 启动：跳转主菜单 ──────────────────────────────────────────────────
print('[main] current scene: ' .. SM.current())
SM.goto_main_menu()  -- 加载主菜单场景，场景就绪后由 ui/main_menu.lua 打开面板
