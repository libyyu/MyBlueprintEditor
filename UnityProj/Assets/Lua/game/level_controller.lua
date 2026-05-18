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
        -- 生成所有绳子（连接到 Candy）
        local ropeCount = _ctrl:GetRopeCount()
        for i = 0, ropeCount - 1 do
            local rope = _ctrl:GetRope(i)
            if rope then
                rope:Spawn(_ctrl.candy)
                print(string.format('[level_controller] Rope[%d] spawned', i))
            end
        end
        -- 通知关卡脚本就绪
        if _levelMod.on_ready then _levelMod.on_ready(_ctrl) end
    end

    _ctrl.OnTick = function(dt)
        if _levelMod.on_tick then _levelMod.on_tick(_ctrl, dt) end
    end

    _ctrl.OnCut = function(cutCount, x, y)
        -- 写入蓝图事件队列（蓝图 OnTick 里的 GameEvent.OnCut 节点消费）
        if _G._PushLevelEvent then
            _G._PushLevelEvent('cut', cutCount .. ',' .. x .. ',' .. y)
        end
        if _levelMod.on_cut then _levelMod.on_cut(_ctrl, cutCount, x, y) end
    end

    _ctrl.OnCandyEaten = function()
        if _G._PushLevelEvent then _G._PushLevelEvent('candy_eaten') end
        M._handle_win()
    end

    _ctrl.OnCandyFailed = function()
        if _G._PushLevelEvent then _G._PushLevelEvent('candy_failed') end
        M._handle_fail()
    end

    -- ── 加载关卡蓝图（负责 UI 编排：通关/失败面板等）────────────────────
    -- bjson 在 YooAsset DefaultPackage 里，路径 Assets/Lua/levels/blueprint_<id>.bjson
    -- 用 pcall 保护，找不到蓝图不影响关卡基本逻辑
    local bjsonAssetPath = 'Assets/Lua/levels/blueprint_' .. _levelId .. '.bjson'
    local ok, bjsonText = pcall(function()
        local package = CS.YooAsset.YooAssets.GetPackage('DefaultPackage')
        if not package then return nil end
        local handle = package:LoadAssetSync(bjsonAssetPath, typeof(CS.UnityEngine.TextAsset))
        if not handle or handle.Status ~= CS.YooAsset.EOperationStatus.Succeed then return nil end
        local ta = handle.AssetObject
        local text = ta and ta.text or nil
        handle:Release()
        return text
    end)
    if ok and bjsonText then
        if BPR and BPR.Instance then
            BPR.Instance:LoadFromJson(bjsonText)
            print('[level_controller] Blueprint loaded: ' .. bjsonAssetPath)
        end
    else
        print('[level_controller] No blueprint for ' .. _levelId .. ' (optional, skipped)')
    end

    print('[level_controller] Initialized: ' .. _levelId)
end

-- ── 内部：通关 ────────────────────────────────────────────────────────────

function M._handle_win()
    _ctrl:StopLevel()
    print('[level_controller] candy eaten → dispatch on_win to blueprint')

    -- 存档、星级计算、面板全部由蓝图节点接管：
    --   Level.CalcStars → Level.Complete(存档) → UI.CloseAll → UI.Open(LevelComplete)
    -- Lua 侧只需派发事件，不做任何 UI 操作
    if _G._PushLevelEvent then _G._PushLevelEvent('candy_eaten') end
    if BPR and BPR.Instance then
        BPR.Instance:DispatchEvent('on_win')
    else
        -- 蓝图不存在时降级处理：用 default.lua 算星级存档
        local stars = _levelMod.calc_stars and _levelMod.calc_stars(_ctrl) or 1
        local score = _levelMod.calc_score and _levelMod.calc_score(_ctrl) or 0
        LM.complete(stars, score)
        if _levelMod.on_win then _levelMod.on_win(_ctrl, stars, score) end
    end
end

-- ── 内部：失败 ────────────────────────────────────────────────────────────

function M._handle_fail()
    _ctrl:StopLevel()
    print('[level_controller] candy failed → dispatch on_fail to blueprint')

    if _G._PushLevelEvent then _G._PushLevelEvent('candy_failed') end
    if BPR and BPR.Instance then
        BPR.Instance:DispatchEvent('on_fail')
    else
        LM.fail()
        if _levelMod.on_fail then _levelMod.on_fail(_ctrl) end
    end
end

--- 获取 C# LevelController（供其他模块使用）
function M.get_ctrl() return _ctrl end

return M
