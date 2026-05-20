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
local BPR = CS.CutRope.Framework.BlueprintRunner  -- C# BlueprintRunner 单例

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

    -- 星星收集钩子
    _ctrl.OnStarCollected = function(starIndex, totalCollected)
        if _G._PushLevelEvent then
            _G._PushLevelEvent('star_collected', starIndex .. ',' .. totalCollected)
        end
        print(string.format('[level_controller] Star %d collected (%d total)', starIndex, totalCollected))
    end

    print('[level_controller] Initialized: ' .. _levelId)

    -- ── 异步加载关卡蓝图（YooAsset 异步接口）──────────────────────────
    M._load_blueprint_async(_levelId)
end

--- 异步加载蓝图，支持依赖/子蓝图递归加载
-- @param levelId  string  如 '1_1'
-- @param onDone   function() 可选，加载完成回调
function M._load_blueprint_async(levelId, onDone)
    local bjsonPath = 'Assets/Lua/levels/blueprint_' .. levelId .. '.bjson'
    M._load_asset_async(bjsonPath, function(text)
        if not text then
            print('[level_controller] No blueprint for ' .. levelId .. ' (optional, skipped)')
            if onDone then onDone() end
            return
        end
        -- 解析 JSON，检查是否有 subBlueprints 依赖
        local deps = M._parse_blueprint_deps(text)
        if #deps == 0 then
            M._apply_blueprint(text, levelId)
            if onDone then onDone() end
        else
            -- 先加载所有依赖蓝图，再加载主蓝图
            local loaded = 0
            local depTexts = {}
            for i, dep in ipairs(deps) do
                M._load_asset_async('Assets/Lua/levels/' .. dep .. '.bjson', function(depText)
                    depTexts[dep] = depText or ''
                    loaded = loaded + 1
                    if loaded == #deps then
                        -- 依赖全部加载完，注入后加载主蓝图
                        for depId, dt in pairs(depTexts) do
                            if dt ~= '' and BPR and BPR.Instance then
                                BPR.Instance:LoadFromJson(dt)
                                print('[level_controller] Sub-blueprint loaded: ' .. depId)
                            end
                        end
                        M._apply_blueprint(text, levelId)
                        if onDone then onDone() end
                    end
                end)
            end
        end
    end)
end

--- 从 bjson 文本解析依赖的子蓝图 id 列表
function M._parse_blueprint_deps(text)
    local deps = {}
    -- 简单字符串匹配 "subBlueprints": ["1_2", ...]
    local arr = text:match('"subBlueprints"%s*:%s*%[(.-)%]')
    if arr then
        for id in arr:gmatch('"([^"]+)"') do
            table.insert(deps, id)
        end
    end
    return deps
end

--- 将蓝图 JSON 提交给 BlueprintRunner
function M._apply_blueprint(text, levelId)
    if BPR and BPR.Instance then
        BPR.Instance:LoadFromJson(text)
        print('[level_controller] Blueprint loaded: ' .. levelId)
    end
end

--- YooAsset 异步加载 TextAsset，完成后回调 fn(text|nil)
function M._load_asset_async(assetPath, fn)
    local package = CS.YooAsset.YooAssets.GetPackage('DefaultPackage')
    if not package then
        print('[level_controller] YooAsset package not found')
        fn(nil)
        return
    end
    local handle = package:LoadAssetAsync(assetPath, typeof(CS.UnityEngine.TextAsset))
    if not handle then
        fn(nil)
        return
    end
    -- 用 coro 等待异步完成
    local co = coroutine.create(function()
        -- 轮询直到完成
        while not handle.IsDone do
            coroutine.yield()
        end
        if handle.Status == CS.YooAsset.EOperationStatus.Succeed then
            local ta = handle.AssetObject
            local text = ta and ta.text or nil
            handle:Release()
            fn(text)
        else
            print('[level_controller] Load failed: ' .. assetPath)
            handle:Release()
            fn(nil)
        end
    end)
    -- 注册到全局 coro 调度器
    if StartCoroutine then
        StartCoroutine(co)
    else
        -- fallback: 用 timer 每帧推进
        local function tick(dt)
            if coroutine.status(co) ~= 'dead' then
                coroutine.resume(co)
            end
        end
        -- 挂到 LevelController OnTick
        local origTick = _ctrl.OnTick
        local _myTick
        _myTick = function(dt)
            if coroutine.status(co) ~= 'dead' then
                coroutine.resume(co)
            end
            if origTick then origTick(dt) end
            if coroutine.status(co) == 'dead' then
                -- 加载完成，移除临时 tick
                _ctrl.OnTick = origTick
            end
        end
        _ctrl.OnTick = _myTick
        coroutine.resume(co)  -- 启动
    end
end

-- ── 内部：通关 ────────────────────────────────────────────────────────────

function M._handle_win()
    _ctrl:StopLevel()

    -- 星级和分数先用 Lua 计算（保证存档不依赖蓝图连线）
    local stars = _levelMod.calc_stars and _levelMod.calc_stars(_ctrl) or 1
    local score = _levelMod.calc_score and _levelMod.calc_score(_ctrl) or 0
    print(string.format('[level_controller] WIN stars=%d score=%d', stars, score))

    -- 存档、解锁下一关 —— 数据层，不依赖蓝图
    LM.complete(stars, score)
    if _levelMod.on_win then _levelMod.on_win(_ctrl, stars, score) end

    -- 派发到蓝图：蓝图只管 UI 流程（CalcStars 节点读星级显示，不再存档）
    if BPR and BPR.Instance then
        BPR.Instance:DispatchEvent('on_win')
    end
end

-- ── 内部：失败 ────────────────────────────────────────────────────────────

function M._handle_fail()
    _ctrl:StopLevel()
    print('[level_controller] FAIL')

    -- 存档——数据层，不依赖蓝图
    LM.fail()
    if _levelMod.on_fail then _levelMod.on_fail(_ctrl) end

    -- 派发到蓝图：蓝图只管 UI 流程
    if BPR and BPR.Instance then
        BPR.Instance:DispatchEvent('on_fail')
    end
end

--- 获取 C# LevelController（供其他模块使用）
function M.get_ctrl() return _ctrl end

return M
