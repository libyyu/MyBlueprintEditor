-- ui/main_menu.lua
-- 主菜单逻辑（Lua 侧）
--
-- 用法（在 main.lua 中）：
--   local MainMenu = require 'ui/main_menu'
--   MainMenu.open()

local UI = require 'ui/ui_manager'
local SM = require 'game/scene_manager'

local M = {}

local ADDRESS = 'UI/MainMenu'  -- YooAsset address，与 Prefab 收集器配置一致

local _controller = nil  -- MainMenuController C# 实例

--- 打开主菜单
function M.open()
    UI.open(ADDRESS, nil, function(view)
        if view == nil then
            print('[main_menu] ERROR: view is nil, check prefab address: ' .. ADDRESS)
            return
        end

        _controller = view

        -- 注入按钮回调
        _controller.OnStart = function()
            M.on_start_clicked()
        end

        _controller.OnSettings = function()
            M.on_settings_clicked()
        end

        _controller.OnQuit = function()
            M.on_quit_clicked()
        end

        print('[main_menu] opened')
    end)
end

--- 关闭主菜单（不销毁）
function M.close()
    UI.close(ADDRESS)
end

--- 销毁主菜单
function M.dispose()
    UI.dispose(ADDRESS)
    _controller = nil
end

-- ── 按钮响应 ──────────────────────────────────────────────────────────────

function M.on_start_clicked()
    print('[main_menu] Start clicked -> loading Level_1_1')
    M.close()
    -- TODO: 替换为真实关卡场景名
    SM.load('Level_1_1', function(p)
        -- 可在此驱动加载进度条
    end, function()
        print('[main_menu] Level_1_1 loaded')
    end)
end

function M.on_settings_clicked()
    print('[main_menu] Settings clicked')
    -- TODO: 打开设置面板
    -- local Settings = require 'ui/settings'
    -- Settings.open()
end

function M.on_quit_clicked()
    print('[main_menu] Quit clicked')
    CS.UnityEngine.Application.Quit()
end

return M
