-- ui/base_panel.lua
-- 所有 UI 面板 Lua 脚本的基类
-- 封装 UIController C# 对象，提供链式调用风格的 Lua 接口
--
-- 使用方式：
--   local BasePanel = require 'ui/base_panel'
--   local MyPanel = BasePanel.new(ctrl)   -- ctrl 是 UIController C# 对象
--   MyPanel:on_show(function() ... end)
--   MyPanel:on_click("btn_start", function() ... end)
--   MyPanel:set_text("lbl_title", "Hello")

local M = {}
M.__index = M

--- 创建面板包装器
-- @param ctrl  UIController C# 对象（由 UIManager 传入）
function M.new(ctrl)
    local self = setmetatable({}, M)
    self._ctrl = ctrl
    self._on_show    = nil
    self._on_hide    = nil
    self._on_dispose = nil
    return self
end

-- ── 生命周期回调 ─────────────────────────────────────────────────────

function M:on_show(fn)
    self._on_show = fn
    self:_flush_callbacks()
    return self
end

function M:on_hide(fn)
    self._on_hide = fn
    self:_flush_callbacks()
    return self
end

function M:on_dispose(fn)
    self._on_dispose = fn
    self:_flush_callbacks()
    return self
end

function M:_flush_callbacks()
    self._ctrl:SetCallbacks(self._on_show, self._on_hide, self._on_dispose)
end

-- ── 组件操作 ─────────────────────────────────────────────────────────

function M:on_click(name, fn)
    self._ctrl:RegisterOnClick(name, fn)
    return self
end

function M:off_click(name)
    self._ctrl:UnregisterOnClick(name)
    return self
end

function M:set_text(name, text)
    self._ctrl:SetText(name, tostring(text))
    return self
end

function M:get_text(name)
    return self._ctrl:GetText(name)
end

function M:set_active(name, active)
    self._ctrl:SetActive(name, active)
    return self
end

function M:set_fill(name, val)
    self._ctrl:SetFill(name, val)
    return self
end

function M:set_slider(name, val)
    self._ctrl:SetSlider(name, val)
    return self
end

function M:set_interactable(name, v)
    self._ctrl:SetInteractable(name, v)
    return self
end

function M:set_color(name, r, g, b, a)
    self._ctrl:SetColor(name, r, g, b, a or 1)
    return self
end

-- ── 面板整体控制 ─────────────────────────────────────────────────────

function M:show()
    self._ctrl:Show()
    return self
end

function M:hide()
    self._ctrl:Hide()
    return self
end

function M:dispose()
    self._ctrl:Dispose()
end

return M
