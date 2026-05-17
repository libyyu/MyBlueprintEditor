-- ui/ui_manager.lua
-- Lua 侧 UI 管理器
-- 封装 C# UIManager 静态绑定，并在打开面板后自动调用对应 Lua 脚本的 open() 初始化逻辑
--
-- 面板地址 → Lua 脚本映射表（新增面板时在此注册）：
--   "UI/MainMenu"       → ui/main_menu
--   "UI/HUD"            → ui/hud
--   "UI/LevelComplete"  → ui/level_complete
--   "UI/LevelFailed"    → ui/level_failed
--   "UI/LevelSelect"    → ui/level_select
--   "UI/Pause"          → ui/pause

local CS_UIManager = CS.CutRope.Framework.UIManager

-- 面板地址 → Lua 模块路径
local LUA_MODULES = {
    ["UI/MainMenu"]      = "ui/main_menu",
    ["UI/HUD"]           = "ui/hud",
    ["UI/LevelComplete"] = "ui/level_complete",
    ["UI/LevelFailed"]   = "ui/level_failed",
    ["UI/LevelSelect"]   = "ui/level_select",
    ["UI/Pause"]         = "ui/pause",
}

-- 当前打开的面板 Lua 对象缓存（address → panel 对象）
local _panels = {}

local UI = {}

--- 打开面板
-- @param address  YooAsset 地址，如 "UI/MainMenu"
-- @param ...      透传给 Lua open() 函数的额外参数（如 stars, score）
function UI.open(address, ...)
    local args = {...}
    CS_UIManager.LuaOpen(address, nil, function(ctrl)
        local modPath = LUA_MODULES[address]
        if modPath then
            local ok, mod = pcall(require, modPath)
            if ok and mod and mod.open then
                -- 把 UIController（ctrl.ViewObject 上的组件）取出来
                local go = ctrl:get_view_object and ctrl:get_view_object() or ctrl.ViewObject
                local uiCtrl = go:GetComponent(typeof(CS.CutRope.Game.UI.UIController))
                if uiCtrl then
                    local panel = mod.open(uiCtrl, table.unpack(args))
                    _panels[address] = panel
                else
                    print("[UI] UIController not found on: " .. address)
                end
            else
                print("[UI] No Lua module for: " .. address)
            end
        end
    end)
end

--- 隐藏面板（不销毁）
function UI.close(address)
    CS_UIManager.LuaClose(address)
end

--- 销毁面板
function UI.dispose(address)
    _panels[address] = nil
    CS_UIManager.LuaDispose(address)
end

--- 隐藏所有面板
function UI.close_all()
    CS_UIManager.LuaCloseAll()
end

--- 获取已打开的面板 Lua 对象
function UI.get(address)
    return _panels[address]
end

return UI
