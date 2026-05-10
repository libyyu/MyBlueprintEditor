-- BlueprintEntry.lua
-- Unity 运行时蓝图节点注册入口
-- 由 LuaManager 在 Lua 环境初始化后执行（通过 YooAsset 加载）
--
-- 负责加载所有自定义蓝图节点扩展：
--   1. game_extensions.lua — 通用游戏事件/Dialogue/Tween 节点
--   2. game_nodes.lua      — UI/Scene/Level/Rope/Candy 游戏专用节点

-- ── 加载通用游戏扩展节点 ─────────────────────────────────────────────────────
local ok, err = pcall(require, 'game_extensions')
if not ok then
    print('[BlueprintEntry] Warning: game_extensions load failed: ' .. tostring(err))
end

-- ── 加载游戏专用节点 ─────────────────────────────────────────────────────────
ok, err = pcall(require, 'game_nodes')
if not ok then
    print('[BlueprintEntry] Warning: game_nodes load failed: ' .. tostring(err))
end

print('[BlueprintEntry] Blueprint nodes registered.')
