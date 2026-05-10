-- main.lua
-- 游戏 Lua 入口，由 LuaManager 从 YooAsset 加载后执行

print("main.lua loaded!")

-- ── 全局钩子（供 LuaManager 调用）────────────────────────────────────

--- 每帧调用（由 LuaManager.Update 驱动）
function update()
    -- 后续游戏逻辑挂在这里
end

--- 销毁时调用
function on_destroy()
    print("on_destroy called")
end

-- ── 后续扩展区 ────────────────────────────────────────────────────────
-- require 'game/scene_manager'
-- require 'game/level_manager'
-- require 'ui/main_menu'
