-- game/level_data.lua
-- 关卡数据定义 — 所有章节与关卡的静态配置
--
-- 结构：
--   chapters[i] = {
--     id    = "1",
--     name  = "第一章",
--     levels = {
--       { id="1_1", name="1-1", scene="Level_1_1", coming_soon=false },
--       ...
--     }
--   }
--
-- coming_soon=true 的关卡在选关界面显示为灰色锁定，不可点击，
-- 等场景制作完成后去掉该字段即可上线。

local M = {}

M.chapters = {
    {
        id   = "1",
        name = "第一章",
        levels = {
            { id = "1_1", name = "1-1", scene = "Level_1_1" },
            { id = "1_2", name = "1-2", scene = "Level_1_2", coming_soon = true },
            { id = "1_3", name = "1-3", scene = "Level_1_3", coming_soon = true },
            { id = "1_4", name = "1-4", scene = "Level_1_4", coming_soon = true },
            { id = "1_5", name = "1-5", scene = "Level_1_5", coming_soon = true },
        }
    },
    {
        id   = "2",
        name = "第二章",
        levels = {
            { id = "2_1", name = "2-1", scene = "Level_2_1", coming_soon = true },
            { id = "2_2", name = "2-2", scene = "Level_2_2", coming_soon = true },
            { id = "2_3", name = "2-3", scene = "Level_2_3", coming_soon = true },
            { id = "2_4", name = "2-4", scene = "Level_2_4", coming_soon = true },
            { id = "2_5", name = "2-5", scene = "Level_2_5", coming_soon = true },
        }
    },
}

-- ── 快速索引 ──────────────────────────────────────────────────────────────
-- 所有关卡展开为线性列表，方便 next_level / get_level 查询
M._flat = {}
M._map  = {}

for _, ch in ipairs(M.chapters) do
    for _, lv in ipairs(ch.levels) do
        lv.chapter_id = ch.id
        lv.chapter_name = ch.name
        table.insert(M._flat, lv)
        M._map[lv.id] = lv
    end
end

--- 根据 id 获取关卡配置
function M.get_level(id)
    return M._map[id]
end

--- 获取下一关配置（跨章节）
function M.next_level(id)
    for i, lv in ipairs(M._flat) do
        if lv.id == id then
            return M._flat[i + 1]
        end
    end
    return nil
end

--- 获取所有关卡（线性顺序）
function M.all_levels()
    return M._flat
end

return M
