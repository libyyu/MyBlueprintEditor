-- game/level_data.lua
-- 关卡数据定义 — 所有章节与关卡的静态配置
--
-- 结构：
--   chapters[i] = {
--     id      = 'chapter_1',
--     name    = '第一章',
--     levels  = {
--       { id='1_1', scene='Level_1_1', name='关卡1', unlock=true },
--       ...
--     }
--   }
--
-- 说明：
--   - scene 对应 YooAsset address 或 Build Settings 场景名
--   - unlock=true 表示默认解锁（第一关），其余通过存档解锁
--   - 星级/分数运行时从 SaveManager 读取，不在此定义

local M = {}

M.chapters = {
    {
        id   = 'chapter_1',
        name = '第一章：初识绳索',
        icon = 'UI/Chapter/chapter_1_icon',  -- YooAsset address
        levels = {
            { id = '1_1', scene = 'Level_1_1', name = '第1关', unlock = true  },
            { id = '1_2', scene = 'Level_1_2', name = '第2关', unlock = false },
            { id = '1_3', scene = 'Level_1_3', name = '第3关', unlock = false },
            { id = '1_4', scene = 'Level_1_4', name = '第4关', unlock = false },
            { id = '1_5', scene = 'Level_1_5', name = '第5关', unlock = false },
        },
    },
    {
        id   = 'chapter_2',
        name = '第二章：重力挑战',
        icon = 'UI/Chapter/chapter_2_icon',
        levels = {
            { id = '2_1', scene = 'Level_2_1', name = '第1关', unlock = false },
            { id = '2_2', scene = 'Level_2_2', name = '第2关', unlock = false },
            { id = '2_3', scene = 'Level_2_3', name = '第3关', unlock = false },
            { id = '2_4', scene = 'Level_2_4', name = '第4关', unlock = false },
            { id = '2_5', scene = 'Level_2_5', name = '第5关', unlock = false },
        },
    },
    -- TODO: 后续添加更多章节
}

-- ── 索引表（快速查找）────────────────────────────────────────────────────

-- levelId → level config
local _levelIndex = {}
-- levelId → chapter config
local _chapterIndex = {}

for _, chapter in ipairs(M.chapters) do
    for _, level in ipairs(chapter.levels) do
        _levelIndex[level.id]   = level
        _chapterIndex[level.id] = chapter
    end
end

--- 根据 levelId 获取关卡配置
function M.get_level(levelId)
    return _levelIndex[levelId]
end

--- 根据 levelId 获取所属章节配置
function M.get_chapter_by_level(levelId)
    return _chapterIndex[levelId]
end

--- 获取某章节中 levelId 的下一关（同章节内）
function M.next_level(levelId)
    local chapter = _chapterIndex[levelId]
    if not chapter then return nil end
    for i, lv in ipairs(chapter.levels) do
        if lv.id == levelId and chapter.levels[i + 1] then
            return chapter.levels[i + 1]
        end
    end
    return nil  -- 已是本章最后一关
end

return M
