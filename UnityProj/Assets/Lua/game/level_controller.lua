-- game/level_controller.lua
-- 关卡逻辑调度器（Lua 层）
--
-- 职责：
--   1. 从 C# 取得 LevelController 引用
--   2. 加载对应关卡脚本 levels/<id>.lua，注入钩子
--   3. 连接 LevelManager（通关/失败）
--   4. 加载关卡蓝图 levels/blueprint_<id>.bjson，驱动 UI 编排
--
-- 每个关卡只需实现自己的规则文件 levels/<levelId>.lua，
-- 通用框架逻辑全在此文件，不需要重复写。

local LM  = require 'game/level_manager'
local BPR = CS.CutRope.Framework.BlueprintRuntime  -- C# BlueprintRuntime 单例

local M = {}
local _ctrl    = nil   -- C# LevelController
local _levelId = nil
local _levelMod = nil  -- 当前关卡规则模块

--- 初始化：绑定 C# LevelController，加载关卡规则脚本
-- @param levelId  string  如 '1_1'
function M.init(levelId)
    _ctrl = CS.CutRope.Game.LevelController.Current
    if _ctrl == nil then
        print('[level_controller] ERROR: LevelController.Current is nil')
        return
    end

    _levelId  = levelId or LM.current_level_id or '1_1'

    -- 加载关卡规则模块（levels/1_1.lua 等）
    local ok, mod = pcall(require, 'game/levels/' .. _levelId)
    if ok and mod then
        _levelMod = mod
        print('[level_controller] Loaded rule module: game/levels/' .. _levelId)
    else
        -- 找不到专属脚本，使用默认规则
        print('[level_controller] No rule module for ' .. _levelId .. ', using default rules')
        _levelMod = require 'game/levels/default'
    end

    -- ── 注入 C# 钩子 ────────────────────────────────────────────────

    _ctrl.OnLevelReady = function()
        if _levelMod.on_ready then _levelMod.on_ready(_ctrl) end
    end

    _ctrl.OnTick = function(dt)
        if _levelMod.on_tick then _levelMod.on_tick(_ctrl, dt) end
    end

    _ctrl.OnCut = function(cutCount, x, y)
        if _levelMod.on_cut then _levelMod.on_cut(_ctrl, cutCount, x, y) end
    end

    _ctrl.OnCandyEaten = function()
        M._handle_win()
    end

    _ctrl.OnCandyFailed = function()
        M._handle_fail()
    end

    -- ── 加载关卡蓝图（负责 UI 编排：通关/失败面板等）────────────────────
    local bjsonKey = 'game/levels/blueprint_' .. _levelId
    -- Lua 读取 bjson 文本（YooAsset 已预加载到 LuaManager 缓存；bjson 当 TextAsset 加载）
    -- 用 pcall 保护，找不到蓝图不影响关卡基本逻辑
    local ok, bjsonText = pcall(function()
        local ta = CS.UnityEngine.Resources.Load(bjsonKey)
        return ta and ta.text or nil
    end)
    if ok and bjsonText then
        if BPR and BPR.Instance then
            BPR.Instance:LoadFromJson(bjsonText)
            print('[level_controller] Blueprint loaded: ' .. bjsonKey)
        end
    else
        print('[level_controller] No blueprint for ' .. _levelId .. ' (optional, skipped)')
    end

    print('[level_controller] Initialized: ' .. _levelId)
end

-- ── 内部：通关 ────────────────────────────────────────────────────────────

function M._handle_win()
    _ctrl.StopLevel()

    -- 让关卡脚本计算星级和分数
    local stars = 1
    local score = 0
    if _levelMod.calc_stars then
        stars = _levelMod.calc_stars(_ctrl)
    end
    if _levelMod.calc_score then
        score = _levelMod.calc_score(_ctrl)
    end

    print(string.format('[level_controller] WIN: stars=%d score=%d', stars, score))

    -- 通知 LevelManager（存档、解锁下一关）
    LM.complete(stars, score)

    -- 触发关卡脚本的通关回调（可显示特效等）
    if _levelMod.on_win then _levelMod.on_win(_ctrl, stars, score) end

    -- 触发蓝图自定义事件 on_win → 由蓝图处理打开通关面板
    if BPR and BPR.Instance then
        BPR.Instance:DispatchEvent('on_win')
    end
end

-- ── 内部：失败 ────────────────────────────────────────────────────────────

function M._handle_fail()
    _ctrl.StopLevel()

    print('[level_controller] FAIL')
    LM.fail()

    if _levelMod.on_fail then _levelMod.on_fail(_ctrl) end

    -- 触发蓝图自定义事件 on_fail → 由蓝图处理打开失败面板
    if BPR and BPR.Instance then
        BPR.Instance:DispatchEvent('on_fail')
    end
end

--- 获取 C# LevelController（供其他模块使用）
function M.get_ctrl() return _ctrl end

return M
