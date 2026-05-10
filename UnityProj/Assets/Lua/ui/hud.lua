-- ui/hud.lua
-- 游戏内 HUD（剪刀数、计时器、暂停按钮）

local BasePanel = require 'ui/base_panel'

local M = {}
local _panel   = nil
local _timer   = 0
local _running = false

function M.open(ctrl)
    _panel   = BasePanel.new(ctrl)
    _timer   = 0
    _running = false

    _panel:on_show(function()
        _running = true
        _panel:set_text("lbl_cuts", "0")
        _panel:set_text("lbl_time", "0.0")
    end)

    _panel:on_hide(function()
        _running = false
    end)

    _panel:on_click("btn_pause", function()
        _running = false
        local UI = require 'ui/ui_manager'
        UI.open("UI/Pause")
    end)

    return _panel
end

--- 每帧由 level_controller.lua 调用
function M.update(dt)
    if not _running or not _panel then return end
    _timer = _timer + dt
    _panel:set_text("lbl_time", string.format("%.1f", _timer))
end

--- 更新剪刀计数显示
function M.set_cuts(n)
    if _panel then _panel:set_text("lbl_cuts", tostring(n)) end
end

--- 获取当前计时（秒）
function M.get_time()
    return _timer
end

function M.stop()
    _running = false
end

return M
