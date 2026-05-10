-- game/save_manager.lua
-- 存档管理器 — 基于 Unity PlayerPrefs 的轻量存档
--
-- 存储内容：
--   - 每关解锁状态
--   - 每关最高星级（0~3）
--   - 每关最高分
--   - 最后游玩的关卡
--
-- 用法：
--   local Save = require 'game/save_manager'
--   Save.unlock('1_2')
--   Save.set_stars('1_1', 3)
--   local stars = Save.get_stars('1_1')  -- 0~3
--   Save.flush()  -- 强制写盘（默认自动）

local M = {}

local PlayerPrefs = CS.UnityEngine.PlayerPrefs
local JsonUtility = CS.UnityEngine.JsonUtility

local KEY_PREFIX_UNLOCK = 'unlock_'
local KEY_PREFIX_STARS  = 'stars_'
local KEY_PREFIX_SCORE  = 'score_'
local KEY_LAST_LEVEL    = 'last_level'

-- ── 解锁 ──────────────────────────────────────────────────────────────────

--- 解锁关卡
function M.unlock(levelId)
    PlayerPrefs.SetInt(KEY_PREFIX_UNLOCK .. levelId, 1)
end

--- 是否已解锁（静态数据中 unlock=true 的也算）
function M.is_unlocked(levelId)
    local LevelData = require 'game/level_data'
    local cfg = LevelData.get_level(levelId)
    if cfg and cfg.unlock then return true end
    return PlayerPrefs.GetInt(KEY_PREFIX_UNLOCK .. levelId, 0) == 1
end

-- ── 星级 ──────────────────────────────────────────────────────────────────

--- 设置星级（只保存更高值）
function M.set_stars(levelId, stars)
    stars = math.max(0, math.min(3, stars))
    local cur = M.get_stars(levelId)
    if stars > cur then
        PlayerPrefs.SetInt(KEY_PREFIX_STARS .. levelId, stars)
    end
end

--- 获取星级（0~3）
function M.get_stars(levelId)
    return PlayerPrefs.GetInt(KEY_PREFIX_STARS .. levelId, 0)
end

-- ── 分数 ──────────────────────────────────────────────────────────────────

--- 设置最高分（只保存更高值）
function M.set_score(levelId, score)
    local cur = M.get_score(levelId)
    if score > cur then
        PlayerPrefs.SetInt(KEY_PREFIX_SCORE .. levelId, score)
    end
end

--- 获取最高分
function M.get_score(levelId)
    return PlayerPrefs.GetInt(KEY_PREFIX_SCORE .. levelId, 0)
end

-- ── 最后关卡 ──────────────────────────────────────────────────────────────

function M.set_last_level(levelId)
    PlayerPrefs.SetString(KEY_LAST_LEVEL, levelId)
end

function M.get_last_level()
    return PlayerPrefs.GetString(KEY_LAST_LEVEL, '1_1')
end

-- ── 工具 ──────────────────────────────────────────────────────────────────

--- 强制写盘
function M.flush()
    PlayerPrefs.Save()
end

--- 清除所有存档（开发调试用）
function M.clear_all()
    PlayerPrefs.DeleteAll()
    print('[save_manager] All saves cleared!')
end

return M
