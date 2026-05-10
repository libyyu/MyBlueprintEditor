-- ui/ui_manager.lua
-- UI 管理器 Lua 封装
--
-- 用法：
--   local UI = require 'ui/ui_manager'
--   UI.open('UI/MainMenu', nil, function(view) ... end)
--   UI.close('UI/MainMenu')
--   UI.dispose('UI/MainMenu')
--   UI.close_all()

local M = {}

local UIManager = CS.CutRope.Framework.UIManager

--- 异步打开面板
-- @param address    string   YooAsset address（如 'UI/MainMenu'）
-- @param param      string   传给 IView.Show 的参数，可为 nil
-- @param onComplete function(view) 完成回调，可为 nil
function M.open(address, param, onComplete)
    assert(type(address) == 'string', 'ui_manager.open: address must be string')
    UIManager.LuaOpen(address, param, onComplete)
end

--- 隐藏面板（不销毁）
function M.close(address)
    UIManager.LuaClose(address)
end

--- 销毁面板（释放资源）
function M.dispose(address)
    UIManager.LuaDispose(address)
end

--- 隐藏所有面板
function M.close_all()
    UIManager.LuaCloseAll()
end

return M
