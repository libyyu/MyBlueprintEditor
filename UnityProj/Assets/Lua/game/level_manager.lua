-- game/level_manager.lua
-- 关卡管理器 — 控制关卡加载、通关、重试、下一关
--
-- 用法：
--   local LM = require 'game/level_manager'
--   LM.load_level('1_1')        -- 加载关卡
--   LM.complete(stars, score)   -- 通关（关卡场景内调用）
--   LM.retry()                  -- 重试当前关卡
--   LM.next()                   -- 进入下一关
--   LM.back_to_menu()           -- 返回主菜单
--
-- 事件钩子（外部注入）：
--   LM.on_level_loaded   = function(levelId) ... end
--   LM.on_level_complete = function(levelId, stars, score) ... end
--   LM.on_level_failed   = function(levelId) ... end

local SM        = require 'game/scene_manager'
local LevelData = require 'game/level_data'
local Save      = require 'game/save_manager'

local M = {}

-- ── 状态 ─────────────────────────────────────────────────────────────────

M.current_level_id = nil   -- 当前关卡 id，如 '1_1'

-- ── 事件钩子（外部可覆盖）────────────────────────────────────────────────

M.on_level_loaded   = nil  -- function(levelId)
M.on_level_complete = nil  -- function(levelId, stars, score)
M.on_level_failed   = nil  -- function(levelId)

-- ── 核心 API ─────────────────────────────────────────────────────────────

--- 加载指定关卡
-- @param levelId  string  如 '1_1'
-- @param onProgress function(float) 可为 nil
--- 加载指定关卡
-- @param levelId 关卡ID，需在 LevelData 中有对应配置
-- @param onProgress 加载进度回调函数
-- @return 无返回值；若关卡不存在或未解锁则提前返回
function M.load_level(levelId, onProgress)
    local cfg = LevelData.get_level(levelId)
    if not cfg then
        print('[level_manager] ERROR: unknown level: ' .. levelId)
        return
    end

    if not Save.is_unlocked(levelId) then
        print('[level_manager] Level locked: ' .. levelId)
        return
    end

    M.current_level_id = levelId
    Save.set_last_level(levelId)

    print('[level_manager] Loading level: ' .. levelId .. ' scene: ' .. cfg.scene)

    SM.load( "Assets/DefaultPackage/Scenes/" .. cfg.scene .. ".unity", onProgress, function()
        print('[level_manager] Level loaded: ' .. levelId)
        if M.on_level_loaded then M.on_level_loaded(levelId) end
    end)
end

--- 通关：保存星级/分数，解锁下一关，触发回调
-- @param stars  int    0~3
-- @param score  int    本次得分
function M.complete(stars, score)
    local levelId = M.current_level_id
    if not levelId then return end

    stars = stars or 0
    score = score or 0

    -- 保存成绩
    Save.set_stars(levelId, stars)
    Save.set_score(levelId, score)
    Save.flush()

    -- 解锁下一关
    local next = LevelData.next_level(levelId)
    if next then
        Save.unlock(next.id)
        print('[level_manager] Unlocked next level: ' .. next.id)
    else
        print('[level_manager] Chapter complete! No next level in this chapter.')
    end

    print(string.format('[level_manager] Complete: %s  stars=%d  score=%d', levelId, stars, score))

    if M.on_level_complete then M.on_level_complete(levelId, stars, score) end
end

--- 关卡失败
function M.fail()
    local levelId = M.current_level_id
    if not levelId then return end
    print('[level_manager] Failed: ' .. levelId)
    if M.on_level_failed then M.on_level_failed(levelId) end
end

--- 重试当前关卡
function M.retry()
    local levelId = M.current_level_id
    if not levelId then return end
    print('[level_manager] Retry: ' .. levelId)
    M.load_level(levelId)
end

--- 进入下一关
function M.next()
    local levelId = M.current_level_id
    if not levelId then return end
    local next = LevelData.next_level(levelId)
    if next then
        M.load_level(next.id)
    else
        print('[level_manager] No next level, back to menu')
        M.back_to_menu()
    end
end

--- 是否有下一关
function M.has_next()
    if not M.current_level_id then return false end
    return LevelData.next_level(M.current_level_id) ~= nil
end

--- 加载下一关（同 next，供 UI 直接调用）
function M.load_next()
    M.next()
end

--- 返回主菜单
function M.back_to_menu()
    M.current_level_id = nil
    SM.goto_main_menu()
end

--- 获取当前关卡配置
function M.current_config()
    if not M.current_level_id then return nil end
    return LevelData.get_level(M.current_level_id)
end

--- 获取所有章节（供关卡选择 UI 使用）
function M.get_chapters()
    return LevelData.chapters
end

return M
