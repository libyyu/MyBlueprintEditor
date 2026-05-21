-- game/level_controller.lua
-- 关卡 Lua 调度层（精简版）— 只做引擎绑定，不含任何游戏逻辑
--
-- 职责：
--   1. 获取 C# LevelController 引用
--   2. 把 C# 钩子全部转发到蓝图事件队列 / DispatchEvent
--   3. 异步加载关卡蓝图 bjson
--
-- 所有游戏逻辑（星级计算、分数、通关条件）全部在 bjson 里用节点实现。
-- 本文件不再依赖 levels/<id>.lua，levels/ 目录可保留但不会被 require。

local LM  = require 'game/level_manager'
local BPR = CS.CutRope.Framework.BlueprintRunner

local M = {}
local _ctrl    = nil
local _levelId = nil

--- 初始化：绑定 C# LevelController，加载蓝图
function M.init(levelId)
    _ctrl = CS.CutRope.Game.LevelController.Current
    if _ctrl == nil then
        print('[level_controller] ERROR: LevelController.Current is nil')
        return
    end
    _levelId = levelId or LM.current_level_id or '1_1'

    -- ── C# 钩子：全部转为蓝图事件，不含逻辑 ────────────────────────

    -- 场景就绪（OnBeginPlay 等价）→ 蓝图自己用 Rope.SpawnAll 生成绳子
    _ctrl.OnLevelReady = function()
        print('[level_controller] OnLevelReady → dispatching on_ready')
        _push('level_ready')
        if BPR and BPR.Instance then
            BPR.Instance:DispatchEvent('on_ready')
        end
    end

    -- 每帧 → 蓝图 OnTick 驱动
    _ctrl.OnTick = function(dt)
        -- OnTick 由蓝图引擎内部驱动，不需要额外 dispatch
    end

    -- 切绳 → 事件队列（蓝图 GameEvent.OnCut 消费）
    _ctrl.OnCut = function(cutCount, x, y)
        _push('cut', cutCount .. ',' .. x .. ',' .. y)
    end

    -- 糖果吃掉 → 事件队列 + 蓝图 on_candy_eaten
    _ctrl.OnCandyEaten = function()
        _push('candy_eaten')
        _ctrl:StopLevel()
        if BPR and BPR.Instance then
            BPR.Instance:DispatchEvent('on_candy_eaten')
        end
    end

    -- 糖果失败 → 事件队列 + 蓝图 on_candy_failed
    _ctrl.OnCandyFailed = function()
        _push('candy_failed')
        _ctrl:StopLevel()
        if BPR and BPR.Instance then
            BPR.Instance:DispatchEvent('on_candy_failed')
        end
    end

    -- 星星收集 → 事件队列（蓝图 GameEvent.OnStarCollected 消费）
    _ctrl.OnStarCollected = function(starIndex, totalCollected)
        _push('star_collected', starIndex .. ',' .. totalCollected)
    end

    print('[level_controller] Bound to LevelController: ' .. _levelId)

    -- ── 异步加载蓝图 ──────────────────────────────────────────────────
    --M._load_blueprint_async(_levelId)
end

-- 写入蓝图事件队列（game_nodes.lua 里的 GameEvent.On* 节点消费）
function _push(evtId, payload)
    if _G._PushLevelEvent then
        _G._PushLevelEvent(evtId, payload or '')
    end
end

-- 异步加载蓝图（支持 subBlueprints 依赖）
function M._load_blueprint_async(levelId, onDone)
    local bjsonPath = 'Assets/Lua/levels/blueprint_' .. levelId .. '.bjson'
    local Bridge = CS.YooAssetsLuaBridge
    Bridge.LoadAsset('DefaultPackage', bjsonPath, function(ok, asset, err)
        if not ok or not asset then
            print('[level_controller] No blueprint for ' .. levelId .. ': ' .. tostring(err))
            if onDone then onDone() end
            return
        end
        local text = asset.text
        local deps = M._parse_blueprint_deps(text)
        if #deps == 0 then
            M._apply_blueprint(text, levelId)
            if onDone then onDone() end
        else
            local loaded = 0
            for _, depId in ipairs(deps) do
                Bridge.LoadAsset('DefaultPackage',
                    'Assets/Lua/levels/' .. depId .. '.bjson',
                    function(dok, dasset, derr)
                        if dok and dasset then
                            M._apply_blueprint(dasset.text, depId)
                        end
                        loaded = loaded + 1
                        if loaded == #deps then
                            M._apply_blueprint(text, levelId)
                            if onDone then onDone() end
                        end
                    end)
            end
        end
    end)
end

function M._parse_blueprint_deps(text)
    local deps = {}
    local arr = text:match('"subBlueprints"%s*:%s*%[(.-)%]')
    if arr then
        for id in arr:gmatch('"([^"]+)"') do
            table.insert(deps, id)
        end
    end
    return deps
end

function M._apply_blueprint(text, levelId)
    if BPR and BPR.Instance then
        BPR.Instance:LoadFromJson(text)
        print('[level_controller] Blueprint applied: ' .. levelId)
    end
end

function M.get_ctrl() return _ctrl end

return M
