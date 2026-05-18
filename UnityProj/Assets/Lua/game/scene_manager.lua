-- game/scene_manager.lua
-- 场景管理模块（Lua 侧）
--
-- 用法：
--   local SM = require 'game/scene_manager'
--
--   -- 切换场景（单场景模式）
--   SM.load('MainMenu', function(p) print('progress', p) end, function() print('done') end)
--
--   -- 叠加加载（HUD、UI层等）
--   SM.load_additive('HUD', nil, function() print('hud loaded') end)
--
--   -- 卸载叠加场景
--   SM.unload('HUD', function() print('hud unloaded') end)
--
--   -- 当前场景名
--   print(SM.current())

local M = {}

-- C# SceneLoader 静态方法绑定
local SceneLoader = CS.CutRope.Framework.SceneLoader

-- ── 场景加载完成钩子（外部可覆盖）─────────────────────────────────────────
-- 每次任意场景加载完成后都会触发，参数为场景名称字符串
-- 用法：SM.on_scene_loaded = function(sceneName) ... end
M.on_scene_loaded = nil

--- 内部：wrap onComplete，统一触发 on_scene_loaded 钩子
local function wrapComplete(sceneName, onComplete)
    return function()
        if onComplete then onComplete() end
        if M.on_scene_loaded then
            M.on_scene_loaded(sceneName)
        end
    end
end

--- 异步加载场景（Single 模式，替换当前场景）
-- @param sceneName  string   YooAsset address 或 Build Settings 场景名
-- @param onProgress function(float) 进度回调，可为 nil
-- @param onComplete function()      完成回调，可为 nil
function M.load(sceneName, onProgress, onComplete)
    assert(type(sceneName) == 'string', 'scene_manager.load: sceneName must be string')
    SceneLoader.LuaLoadScene(sceneName, onProgress, wrapComplete(sceneName, onComplete))
end

--- 异步叠加加载场景（Additive 模式）
-- @param sceneName  string
-- @param onProgress function(float) 可为 nil
-- @param onComplete function()      可为 nil
function M.load_additive(sceneName, onProgress, onComplete)
    assert(type(sceneName) == 'string', 'scene_manager.load_additive: sceneName must be string')
    SceneLoader.LuaLoadSceneAdditive(sceneName, onProgress, wrapComplete(sceneName, onComplete))
end

--- 卸载叠加场景
-- @param sceneName  string
-- @param onComplete function() 可为 nil
function M.unload(sceneName, onComplete)
    assert(type(sceneName) == 'string', 'scene_manager.unload: sceneName must be string')
    SceneLoader.LuaUnloadScene(sceneName, onComplete)
end

--- 返回当前激活场景名
function M.current()
    return CS.UnityEngine.SceneManagement.SceneManager.GetActiveScene().name
end

--- 便捷：加载主菜单
function M.goto_main_menu()
    M.load('Assets/DefaultPackage/Scenes/MainMenu', nil, nil)
end

--- 便捷：加载游戏场景（附带进度打印）
function M.goto_game(sceneName, onComplete)
    M.load(sceneName, nil, onComplete)
end

return M
