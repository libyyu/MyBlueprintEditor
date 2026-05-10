-- main.lua
-- 游戏 Lua 入口，由 LuaManager 从 YooAsset 加载后执行

print("main.lua loaded!")

local SM = require 'game/scene_manager'

-- ── 全局钩子（供 LuaManager 调用）────────────────────────────────────

--- 每帧调用（由 LuaManager.Update 驱动）
function update()
    -- 后续游戏逻辑挂在这里
end

--- 销毁时调用
function on_destroy()
    print("on_destroy called")
end

-- ── 启动：跳转主菜单 ────────────────────────────────────────────────
print('[main] current scene: ' .. SM.current())
SM.goto_main_menu()  -- 加载主菜单场景，场景就绪后由 ui/main_menu.lua 打开面板
