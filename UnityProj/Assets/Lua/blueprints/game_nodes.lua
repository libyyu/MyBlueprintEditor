-- data/game_nodes.lua
-- 游戏专用蓝图节点扩展
--
-- 包含以下节点分类：
--   UI.*      — 面板显示/隐藏/销毁
--   Scene.*   — 场景加载/切换/查询
--   Level.*   — 关卡状态读取、通关/失败/重试
--   Rope.*    — 绳子操控
--   Candy.*   — 糖果状态查询
--
-- 用法：在 UnityProj/Assets/Lua/ 下的 BlueprintEntry.lua 中 dofile 本文件
-- 所有节点均通过 CS.CutRope.Framework / CS.CutRope.Game 调用 C# 引擎接口

-- ═══════════════════════════════════════════════════════════════════════════════
-- UI 节点
-- UIManager 已废弃，全部走 Lua Panel 映射表
-- ═══════════════════════════════════════════════════════════════════════════════

-- address → Lua 模块路径映射
-- 蓝图里填 "UI/HUD"，这里翻译成 require 路径
local _uiModuleMap = {
    ["UI/HUD"]           = "ui.FPanelHUD",
    ["UI/Pause"]         = "ui.FPanelPause",
    ["UI/LevelComplete"] = "ui.FPanelLevelComplete",
    ["UI/LevelFailed"]   = "ui.FPanelLevelFailed",
    ["UI/MainMenu"]      = "ui.FPanelMainMenu",
    ["UI/LevelSelect"]   = "ui.FPanelLevelSelect",
}

local function _getPanel(address)
    local modPath = _uiModuleMap[address]
    if not modPath then
        print('[UI] Unknown address: ' .. tostring(address))
        return nil
    end
    local ok, mod = pcall(require, modPath)
    if not ok or not mod then
        print('[UI] require failed: ' .. tostring(modPath) .. ' | ' .. tostring(mod))
        return nil
    end
    if type(mod.Instance) ~= 'function' then
        print('[UI] No Instance() on: ' .. modPath)
        return nil
    end
    return mod.Instance()
end

-- ───────────────────────────────────────────────────────────────────────────────

Blueprint.RegisterNodeDef({
    id          = "UI.Open",
    name        = "UI Open Panel",
    category    = "Game/UI",
    color       = "5A3A9A",
    description = "打开 Lua UI 面板（FPanelXxx.Instance():ShowPanel(true)）",
    inputs  = {
        { type = "Flow" },
        { name = "Address", type = "String" },
        { name = "Param",   type = "String" },
    },
    outputs = {
        { type = "Flow" },
    },
})
Blueprint.RegisterHandler("UI.Open", function(ctx)
    local address = ctx:GetInput("Address"):asString()
    local panel = _getPanel(address)
    if panel then panel:ShowPanel(true) end
    return true
end)

-- ───────────────────────────────────────────────────────────────────────────────

Blueprint.RegisterNodeDef({
    id          = "UI.Close",
    name        = "UI Close Panel",
    category    = "Game/UI",
    color       = "5A3A9A",
    description = "隐藏 Lua UI 面板（ShowPanel(false)）",
    inputs  = {
        { type = "Flow" },
        { name = "Address", type = "String" },
    },
    outputs = {
        { type = "Flow" },
    },
})
Blueprint.RegisterHandler("UI.Close", function(ctx)
    local address = ctx:GetInput("Address"):asString()
    local panel = _getPanel(address)
    if panel then panel:ShowPanel(false) end
    return true
end)

-- ───────────────────────────────────────────────────────────────────────────────

Blueprint.RegisterNodeDef({
    id          = "UI.Dispose",
    name        = "UI Dispose Panel",
    category    = "Game/UI",
    color       = "5A3A9A",
    description = "销毁 Lua UI 面板（DestroyPanel）",
    inputs  = {
        { type = "Flow" },
        { name = "Address", type = "String" },
    },
    outputs = {
        { type = "Flow" },
    },
})
Blueprint.RegisterHandler("UI.Dispose", function(ctx)
    local address = ctx:GetInput("Address"):asString()
    local panel = _getPanel(address)
    if panel then panel:DestroyPanel() end
    return true
end)

-- ───────────────────────────────────────────────────────────────────────────────

Blueprint.RegisterNodeDef({
    id          = "UI.CloseAll",
    name        = "UI Close All",
    category    = "Game/UI",
    color       = "5A3A9A",
    description = "隐藏所有已打开的 Lua UI 面板",
    inputs  = { { type = "Flow" } },
    outputs = { { type = "Flow" } },
})
Blueprint.RegisterHandler("UI.CloseAll", function(ctx)
    for address, modPath in pairs(_uiModuleMap) do
        local ok, mod = pcall(require, modPath)
        if ok and mod and type(mod.Instance) == 'function' then
            local inst = mod.Instance()
            if inst then inst:ShowPanel(false) end
        end
    end
    return true
end)

-- ═══════════════════════════════════════════════════════════════════════════════
-- Scene 节点
-- ═══════════════════════════════════════════════════════════════════════════════

Blueprint.RegisterNodeDef({
    id          = "Scene.Load",
    name        = "Scene Load",
    category    = "Game/Scene",
    color       = "3A6A9A",
    description = "异步加载场景（Single 模式，替换当前场景）",
    inputs  = {
        { type = "Flow" },
        { name = "Scene", type = "String" },  -- YooAsset address 或 Build Settings 场景名
    },
    outputs = {
        { type = "Flow" },
    },
})
Blueprint.RegisterHandler("Scene.Load", function(ctx)
    local scene = ctx:GetInput("Scene"):asString()
    CS.CutRope.Framework.SceneLoader.LuaLoadScene(scene, nil, nil)
    return true
end)

-- ───────────────────────────────────────────────────────────────────────────────

Blueprint.RegisterNodeDef({
    id          = "Scene.LoadAdditive",
    name        = "Scene Load Additive",
    category    = "Game/Scene",
    color       = "3A6A9A",
    description = "叠加加载场景（Additive 模式）",
    inputs  = {
        { type = "Flow" },
        { name = "Scene", type = "String" },
    },
    outputs = { { type = "Flow" } },
})
Blueprint.RegisterHandler("Scene.LoadAdditive", function(ctx)
    local scene = ctx:GetInput("Scene"):asString()
    CS.CutRope.Framework.SceneLoader.LuaLoadSceneAdditive(scene, nil, nil)
    return true
end)

-- ───────────────────────────────────────────────────────────────────────────────

Blueprint.RegisterNodeDef({
    id          = "Scene.Unload",
    name        = "Scene Unload",
    category    = "Game/Scene",
    color       = "3A6A9A",
    description = "卸载叠加场景",
    inputs  = {
        { type = "Flow" },
        { name = "Scene", type = "String" },
    },
    outputs = { { type = "Flow" } },
})
Blueprint.RegisterHandler("Scene.Unload", function(ctx)
    local scene = ctx:GetInput("Scene"):asString()
    CS.CutRope.Framework.SceneLoader.LuaUnloadScene(scene, nil)
    return true
end)

-- ───────────────────────────────────────────────────────────────────────────────

Blueprint.RegisterNodeDef({
    id          = "Scene.GetCurrent",
    name        = "Scene Get Current",
    category    = "Game/Scene",
    color       = "3A6A9A",
    description = "获取当前激活的场景名",
    inputs  = { { type = "Flow" } },
    outputs = {
        { type = "Flow" },
        { name = "Name", type = "String" },
    },
})
Blueprint.RegisterHandler("Scene.GetCurrent", function(ctx)
    local name = CS.UnityEngine.SceneManagement.SceneManager.GetActiveScene().name
    ctx:SetOutput("Name", name)
    return true
end)

-- ═══════════════════════════════════════════════════════════════════════════════
-- Level 节点
-- ═══════════════════════════════════════════════════════════════════════════════

Blueprint.RegisterNodeDef({
    id          = "Level.Complete",
    name        = "Level Complete",
    category    = "Game/Level",
    color       = "9A6A1A",
    description = "通关：通知 LevelManager 保存成绩并解锁下一关",
    inputs  = {
        { type = "Flow" },
        { name = "Stars", type = "Integer" },  -- 1~3
        { name = "Score", type = "Integer" },
    },
    outputs = { { type = "Flow" } },
})
Blueprint.RegisterHandler("Level.Complete", function(ctx)
    local stars = ctx:GetInput("Stars"):asInt()
    local score = ctx:GetInput("Score"):asInt()
    -- 通过 LevelManager Lua 模块完成存档逻辑
    local LM = require 'game/level_manager'
    LM.complete(stars, score)
    return true
end)

-- ───────────────────────────────────────────────────────────────────────────────

Blueprint.RegisterNodeDef({
    id          = "Level.Fail",
    name        = "Level Fail",
    category    = "Game/Level",
    color       = "9A1A1A",
    description = "关卡失败：通知 LevelManager",
    inputs  = { { type = "Flow" } },
    outputs = { { type = "Flow" } },
})
Blueprint.RegisterHandler("Level.Fail", function(ctx)
    local LM = require 'game/level_manager'
    LM.fail()
    return true
end)

-- ───────────────────────────────────────────────────────────────────────────────

Blueprint.RegisterNodeDef({
    id          = "Level.Retry",
    name        = "Level Retry",
    category    = "Game/Level",
    color       = "9A6A1A",
    description = "重试当前关卡",
    inputs  = { { type = "Flow" } },
    outputs = { { type = "Flow" } },
})
Blueprint.RegisterHandler("Level.Retry", function(ctx)
    local LM = require 'game/level_manager'
    LM.retry()
    return true
end)

-- ───────────────────────────────────────────────────────────────────────────────

Blueprint.RegisterNodeDef({
    id          = "Level.BackToMenu",
    name        = "Level Back To Menu",
    category    = "Game/Level",
    color       = "9A6A1A",
    description = "返回主菜单",
    inputs  = { { type = "Flow" } },
    outputs = { { type = "Flow" } },
})
Blueprint.RegisterHandler("Level.BackToMenu", function(ctx)
    local LM = require 'game/level_manager'
    LM.back_to_menu()
    return true
end)

-- ───────────────────────────────────────────────────────────────────────────────

Blueprint.RegisterNodeDef({
    id          = "Level.GetStats",
    name        = "Level Get Stats",
    category    = "Game/Level",
    color       = "9A6A1A",
    description = "读取当前关卡运行时数据（刀数、用时）",
    inputs  = { { type = "Flow" } },
    outputs = {
        { type = "Flow" },
        { name = "CutCount",    type = "Integer" },
        { name = "ElapsedTime", type = "Float"   },
        { name = "IsRunning",   type = "Boolean" },
    },
})
Blueprint.RegisterHandler("Level.GetStats", function(ctx)
    local ctrl = CS.CutRope.Game.LevelController.Current
    if ctrl then
        ctx:SetOutput("CutCount",    ctrl.CutCount)
        ctx:SetOutput("ElapsedTime", ctrl.ElapsedTime)
        ctx:SetOutput("IsRunning",   ctrl.IsRunning)
    else
        ctx:SetOutput("CutCount",    0)
        ctx:SetOutput("ElapsedTime", 0.0)
        ctx:SetOutput("IsRunning",   false)
    end
    return true
end)

-- ───────────────────────────────────────────────────────────────────────────────

Blueprint.RegisterNodeDef({
    id          = "Level.Stop",
    name        = "Level Stop",
    category    = "Game/Level",
    color       = "9A1A1A",
    description = "停止关卡 tick（通关/失败后调用，防止重复触发）",
    inputs  = { { type = "Flow" } },
    outputs = { { type = "Flow" } },
})
Blueprint.RegisterHandler("Level.Stop", function(ctx)
    local ctrl = CS.CutRope.Game.LevelController.Current
    if ctrl then ctrl:StopLevel() end
    return true
end)

-- ═══════════════════════════════════════════════════════════════════════════════
-- Rope 节点
-- ═══════════════════════════════════════════════════════════════════════════════

Blueprint.RegisterNodeDef({
    id          = "Rope.Cut",
    name        = "Rope Cut",
    category    = "Game/Rope",
    color       = "6A3A1A",
    description = "切断指定索引的绳子的第 N 个节点",
    inputs  = {
        { type = "Flow" },
        { name = "RopeIndex", type = "Integer" },  -- 第几条绳子
        { name = "NodeIndex", type = "Integer" },  -- 第几个节点处切断
    },
    outputs = { { type = "Flow" } },
})
Blueprint.RegisterHandler("Rope.Cut", function(ctx)
    local ctrl      = CS.CutRope.Game.LevelController.Current
    local ropeIdx   = ctx:GetInput("RopeIndex"):asInt()
    local nodeIdx   = ctx:GetInput("NodeIndex"):asInt()
    if ctrl then
        local rope = ctrl:GetRope(ropeIdx)
        if rope then rope:Cut(nodeIdx) end
    end
    return true
end)

-- ───────────────────────────────────────────────────────────────────────────────

Blueprint.RegisterNodeDef({
    id          = "Rope.GetCount",
    name        = "Rope Get Count",
    category    = "Game/Rope",
    color       = "6A3A1A",
    description = "获取当前关卡绳子数量",
    inputs  = { { type = "Flow" } },
    outputs = {
        { type = "Flow" },
        { name = "Count", type = "Integer" },
    },
})
Blueprint.RegisterHandler("Rope.GetCount", function(ctx)
    local ctrl = CS.CutRope.Game.LevelController.Current
    local count = ctrl and ctrl:GetRopeCount() or 0
    ctx:SetOutput("Count", count)
    return true
end)

-- ═══════════════════════════════════════════════════════════════════════════════
-- Candy 节点
-- ═══════════════════════════════════════════════════════════════════════════════

Blueprint.RegisterNodeDef({
    id          = "Candy.GetState",
    name        = "Candy Get State",
    category    = "Game/Candy",
    color       = "9A3A5A",
    description = "查询糖果当前状态（是否被吃/是否失败）",
    inputs  = { { type = "Flow" } },
    outputs = {
        { type = "Flow" },
        { name = "IsEaten",  type = "Boolean" },
        { name = "IsFailed", type = "Boolean" },
    },
})
Blueprint.RegisterHandler("Candy.GetState", function(ctx)
    local ctrl = CS.CutRope.Game.LevelController.Current
    local eaten, failed = false, false
    if ctrl and ctrl.candy then
        eaten  = ctrl.candy.IsEaten
        failed = ctrl.candy.IsFailed
    end
    ctx:SetOutput("IsEaten",  eaten)
    ctx:SetOutput("IsFailed", failed)
    return true
end)

-- ───────────────────────────────────────────────────────────────────────────────

-- ═══════════════════════════════════════════════════════════════════════════════
-- Level 扩展：Pause / Resume / CalcStars
-- ═══════════════════════════════════════════════════════════════════════════════

Blueprint.RegisterNodeDef({
    id          = "Level.Pause",
    name        = "Level Pause",
    category    = "Game/Level",
    color       = "9A6A1A",
    description = "暂停关卡（停止 Tick，打开暂停面板）",
    inputs  = { { type = "Flow" } },
    outputs = { { type = "Flow" } },
})
Blueprint.RegisterHandler("Level.Pause", function(ctx)
    local ctrl = CS.CutRope.Game.LevelController.Current
    if ctrl then ctrl:PauseLevel() end
    CS.CutRope.Framework.UIManager.LuaOpen("UI/Pause", nil, nil)
    return true
end)

-- ───────────────────────────────────────────────────────────────────────────────

Blueprint.RegisterNodeDef({
    id          = "Level.Resume",
    name        = "Level Resume",
    category    = "Game/Level",
    color       = "9A6A1A",
    description = "恢复关卡（继续 Tick，关闭暂停面板）",
    inputs  = { { type = "Flow" } },
    outputs = { { type = "Flow" } },
})
Blueprint.RegisterHandler("Level.Resume", function(ctx)
    local ctrl = CS.CutRope.Game.LevelController.Current
    if ctrl then ctrl:ResumeLevel() end
    CS.CutRope.Framework.UIManager.LuaClose("UI/Pause")
    return true
end)

-- ───────────────────────────────────────────────────────────────────────────────

Blueprint.RegisterNodeDef({
    id          = "Level.CalcStars",
    name        = "Level Calc Stars",
    category    = "Game/Level",
    color       = "9A6A1A",
    description = "根据刀数计算星级（≤MaxCuts = 3星；≤MaxCuts*2 = 2星；其余 = 1星）",
    inputs  = {
        { type = "Flow" },
        { name = "MaxCuts", type = "Integer" },
    },
    outputs = {
        { type = "Flow" },
        { name = "Stars", type = "Integer" },
    },
})
Blueprint.RegisterHandler("Level.CalcStars", function(ctx)
    local maxCuts = ctx:GetInput("MaxCuts"):asInt()
    local ctrl    = CS.CutRope.Game.LevelController.Current
    local cuts    = ctrl and ctrl.CutCount or 0
    local stars   = cuts <= maxCuts and 3 or (cuts <= maxCuts * 2 and 2 or 1)
    ctx:SetOutput("Stars", stars)
    return true
end)

-- ═══════════════════════════════════════════════════════════════════════════════
-- GameEvent.On* — 关卡内事件监听
-- 蓝图里用这些节点替代 level_controller 的 Lua 回调
-- 事件由 level_controller 写入全局队列 _G._PushLevelEvent，蓝图 OnTick 轮询消费
-- ═══════════════════════════════════════════════════════════════════════════════

local _levelEvtQueue = {}

--- C# 钩子通过 level_controller 调用此函数写入事件队列
function _PushLevelEvent(evtId, payload)
    table.insert(_levelEvtQueue, { id = evtId, payload = payload or "" })
end
_G._PushLevelEvent = _PushLevelEvent

-- ───────────────────────────────────────────────────────────────────────────────

Blueprint.RegisterNodeDef({
    id          = "GameEvent.OnCut",
    name        = "On Cut",
    category    = "Game/Event",
    color       = "2A7A5A",
    description = "每次玩家切绳时触发（在 OnTick 里轮询）",
    inputs  = { { type = "Flow" } },
    outputs = {
        { name = "onCut",    type = "Flow"    },
        { name = "onEmpty",  type = "Flow"    },
        { name = "CutCount", type = "Integer" },
        { name = "X",        type = "Float"   },
        { name = "Y",        type = "Float"   },
    },
})
Blueprint.RegisterHandler("GameEvent.OnCut", function(ctx)
    for i, evt in ipairs(_levelEvtQueue) do
        if evt.id == "cut" then
            table.remove(_levelEvtQueue, i)
            local parts = {}
            for v in (evt.payload .. ","):gmatch("([^,]*),") do
                table.insert(parts, v)
            end
            ctx:SetOutput("CutCount", tonumber(parts[1]) or 0)
            ctx:SetOutput("X",        tonumber(parts[2]) or 0)
            ctx:SetOutput("Y",        tonumber(parts[3]) or 0)
            ctx:ActivateOutputFlow("onCut")
            return true
        end
    end
    ctx:ActivateOutputFlow("onEmpty")
    return true
end)

-- ───────────────────────────────────────────────────────────────────────────────

Blueprint.RegisterNodeDef({
    id          = "GameEvent.OnCandyEaten",
    name        = "On Candy Eaten",
    category    = "Game/Event",
    color       = "2A7A5A",
    description = "糖果被怪兽吃掉时触发",
    inputs  = { { type = "Flow" } },
    outputs = {
        { name = "onEaten", type = "Flow" },
        { name = "onEmpty", type = "Flow" },
    },
})
Blueprint.RegisterHandler("GameEvent.OnCandyEaten", function(ctx)
    for i, evt in ipairs(_levelEvtQueue) do
        if evt.id == "candy_eaten" then
            table.remove(_levelEvtQueue, i)
            ctx:ActivateOutputFlow("onEaten")
            return true
        end
    end
    ctx:ActivateOutputFlow("onEmpty")
    return true
end)

-- ───────────────────────────────────────────────────────────────────────────────

Blueprint.RegisterNodeDef({
    id          = "GameEvent.OnCandyFailed",
    name        = "On Candy Failed",
    category    = "Game/Event",
    color       = "2A7A5A",
    description = "糖果掉落/出界失败时触发",
    inputs  = { { type = "Flow" } },
    outputs = {
        { name = "onFailed", type = "Flow" },
        { name = "onEmpty",  type = "Flow" },
    },
})
Blueprint.RegisterHandler("GameEvent.OnCandyFailed", function(ctx)
    for i, evt in ipairs(_levelEvtQueue) do
        if evt.id == "candy_failed" then
            table.remove(_levelEvtQueue, i)
            ctx:ActivateOutputFlow("onFailed")
            return true
        end
    end
    ctx:ActivateOutputFlow("onEmpty")
    return true
end)

-- ═══════════════════════════════════════════════════════════════════════════════
-- Audio 节点
-- ═══════════════════════════════════════════════════════════════════════════════

Blueprint.RegisterNodeDef({
    id          = "Audio.Play",
    name        = "Audio Play",
    category    = "Game/Audio",
    color       = "3A7A3A",
    description = "播放音效（YooAsset address）",
    inputs  = {
        { type = "Flow" },
        { name = "Clip",   type = "String" },
        { name = "Volume", type = "Float"  },
    },
    outputs = { { type = "Flow" } },
})
Blueprint.RegisterHandler("Audio.Play", function(ctx)
    local clip   = ctx:GetInput("Clip"):asString()
    local volume = ctx:GetInput("Volume"):asFloat()
    if volume <= 0 then volume = 1 end
    local ok, AudioMgr = pcall(function() return CS.CutRope.Framework.AudioManager.Instance end)
    if ok and AudioMgr then
        AudioMgr:PlaySFX(clip, volume)
    else
        print('[Audio.Play] AudioManager not found, clip=' .. clip)
    end
    return true
end)

-- ───────────────────────────────────────────────────────────────────────────────

Blueprint.RegisterNodeDef({
    id          = "Audio.PlayBGM",
    name        = "Audio Play BGM",
    category    = "Game/Audio",
    color       = "3A7A3A",
    description = "播放背景音乐（淡入替换当前 BGM）",
    inputs  = {
        { type = "Flow" },
        { name = "Clip",     type = "String" },
        { name = "FadeTime", type = "Float"  },
    },
    outputs = { { type = "Flow" } },
})
Blueprint.RegisterHandler("Audio.PlayBGM", function(ctx)
    local clip     = ctx:GetInput("Clip"):asString()
    local fadeTime = ctx:GetInput("FadeTime"):asFloat()
    if fadeTime <= 0 then fadeTime = 0.5 end
    local ok, AudioMgr = pcall(function() return CS.CutRope.Framework.AudioManager.Instance end)
    if ok and AudioMgr then AudioMgr:PlayBGM(clip, fadeTime) end
    return true
end)

-- ───────────────────────────────────────────────────────────────────────────────

Blueprint.RegisterNodeDef({
    id          = "Audio.Stop",
    name        = "Audio Stop BGM",
    category    = "Game/Audio",
    color       = "3A7A3A",
    description = "停止背景音乐",
    inputs  = {
        { type = "Flow" },
        { name = "FadeTime", type = "Float" },
    },
    outputs = { { type = "Flow" } },
})
Blueprint.RegisterHandler("Audio.Stop", function(ctx)
    local fadeTime = ctx:GetInput("FadeTime"):asFloat()
    if fadeTime <= 0 then fadeTime = 0.5 end
    local ok, AudioMgr = pcall(function() return CS.CutRope.Framework.AudioManager.Instance end)
    if ok and AudioMgr then AudioMgr:StopBGM(fadeTime) end
    return true
end)

-- ═══════════════════════════════════════════════════════════════════════════════
-- Timer 节点
-- ═══════════════════════════════════════════════════════════════════════════════

Blueprint.RegisterNodeDef({
    id          = "Timer.Wait",
    name        = "Timer Wait",
    category    = "Game/Timer",
    color       = "4A4A9A",
    description = "延迟节点：每帧推进，Duration 秒后触发 onDone（在 OnTick 里驱动）",
    inputs  = {
        { type = "Flow" },
        { name = "TimerId",   type = "String"  },
        { name = "Duration",  type = "Float"   },
        { name = "DeltaTime", type = "Float"   },
        { name = "Enabled",   type = "Boolean" },  -- false 时不推进（默认 true）
    },
    outputs = {
        { name = "onTick",    type = "Flow"  },
        { name = "onDone",    type = "Flow"  },
        { name = "Elapsed",   type = "Float" },
        { name = "Progress",  type = "Float" },
    },
})
Blueprint.RegisterHandler("Timer.Wait", function(ctx)
    local id  = ctx:GetInput("TimerId"):asString()
    if id == "" then id = "default_timer" end
    local dur     = ctx:GetInput("Duration"):asFloat()
    local dt      = ctx:GetInput("DeltaTime"):asFloat()
    local enabledV = ctx:GetInput("Enabled")
    local enabled  = (not enabledV:isValid()) or enabledV:asBool()
    if not enabled then dt = 0 end
    local key  = "__timer_" .. id
    local elapsed = math.min((Blueprint.GetVariable(key) or 0) + dt, dur)
    Blueprint.SetVariable(key, elapsed)
    ctx:SetOutput("Elapsed",  elapsed)
    ctx:SetOutput("Progress", dur > 0 and elapsed / dur or 1)
    if elapsed >= dur then
        Blueprint.SetVariable(key, 0)  -- 自动重置方便复用
        ctx:ActivateOutputFlow("onDone")
    else
        ctx:ActivateOutputFlow("onTick")
    end
    return true
end)

-- ───────────────────────────────────────────────────────────────────────────────

Blueprint.RegisterNodeDef({
    id          = "Timer.Reset",
    name        = "Timer Reset",
    category    = "Game/Timer",
    color       = "4A4A9A",
    description = "手动重置指定 Timer 的计时",
    inputs  = {
        { type = "Flow" },
        { name = "TimerId", type = "String" },
    },
    outputs = { { type = "Flow" } },
})
Blueprint.RegisterHandler("Timer.Reset", function(ctx)
    local id = ctx:GetInput("TimerId"):asString()
    if id == "" then id = "default_timer" end
    Blueprint.SetVariable("__timer_" .. id, 0)
    return true
end)

-- ═══════════════════════════════════════════════════════════════════════════════
-- Star 节点
-- ═══════════════════════════════════════════════════════════════════════════════

Blueprint.RegisterNodeDef({
    id          = "Star.GetCount",
    name        = "Star Get Count",
    category    = "Game/Star",
    color       = "9A8A1A",
    description = "获取当前关卡星星总数和已收集数",
    inputs  = { { type = "Flow" } },
    outputs = {
        { type = "Flow" },
        { name = "Total",     type = "Integer" },
        { name = "Collected", type = "Integer" },
    },
})
Blueprint.RegisterHandler("Star.GetCount", function(ctx)
    local ctrl = CS.CutRope.Game.LevelController.Current
    ctx:SetOutput("Total",     ctrl and ctrl.StarCount     or 0)
    ctx:SetOutput("Collected", ctrl and ctrl.StarsCollected or 0)
    return true
end)

-- ───────────────────────────────────────────────────────────────────────────────

Blueprint.RegisterNodeDef({
    id          = "GameEvent.OnStarCollected",
    name        = "On Star Collected",
    category    = "Game/Event",
    color       = "2A7A5A",
    description = "收集到星星时触发（OnTick 里轮询）",
    inputs  = { { type = "Flow" } },
    outputs = {
        { name = "onCollected", type = "Flow"    },
        { name = "onEmpty",     type = "Flow"    },
        { name = "StarIndex",   type = "Integer" },
        { name = "Total",       type = "Integer" },
    },
})
Blueprint.RegisterHandler("GameEvent.OnStarCollected", function(ctx)
    for i, evt in ipairs(_levelEvtQueue) do
        if evt.id == "star_collected" then
            table.remove(_levelEvtQueue, i)
            local parts = {}
            for v in (evt.payload .. ","):gmatch("([^,]*),") do
                table.insert(parts, v)
            end
            ctx:SetOutput("StarIndex", tonumber(parts[1]) or 0)
            ctx:SetOutput("Total",     tonumber(parts[2]) or 0)
            ctx:ActivateOutputFlow("onCollected")
            return true
        end
    end
    ctx:ActivateOutputFlow("onEmpty")
    return true
end)

-- ═══════════════════════════════════════════════════════════════════════════════
-- Obstacle 节点
-- ═══════════════════════════════════════════════════════════════════════════════

Blueprint.RegisterNodeDef({
    id          = "Obstacle.SetMoving",
    name        = "Obstacle Set Moving",
    category    = "Game/Obstacle",
    color       = "8A1A1A",
    description = "启动/停止障碍物移动（蓝图驱动障碍物行为）",
    inputs  = {
        { type = "Flow" },
        { name = "Name",      type = "String"  },  -- GameObject 名字
        { name = "Moving",    type = "Boolean" },
        { name = "Speed",     type = "Float"   },
        { name = "Range",     type = "Float"   },
    },
    outputs = { { type = "Flow" } },
})
Blueprint.RegisterHandler("Obstacle.SetMoving", function(ctx)
    local name    = ctx:GetInput("Name"):asString()
    local moving  = ctx:GetInput("Moving"):asBool()
    local speed   = ctx:GetInput("Speed"):asFloat()
    local range   = ctx:GetInput("Range"):asFloat()
    local go = CS.UnityEngine.GameObject.Find(name)
    if go then
        local obs = go:GetComponent(typeof(CS.CutRope.Game.Obstacle))
        if obs then
            if speed > 0 then obs:SetMoveSpeed(speed) end
            if range > 0 then obs:SetMoveRange(range) end
            obs:SetMoving(moving)
        end
    end
    return true
end)

-- ═══════════════════════════════════════════════════════════════════════════════
-- Camera 节点
-- ═══════════════════════════════════════════════════════════════════════════════

Blueprint.RegisterNodeDef({
    id          = "Camera.Shake",
    name        = "Camera Shake",
    category    = "Game/Camera",
    color       = "3A5A8A",
    description = "摄像机震屏（切绳/通关/失败时反馈）",
    inputs  = {
        { type = "Flow" },
        { name = "Intensity", type = "Float" },  -- 震动强度（默认 0.3）
        { name = "Duration",  type = "Float" },  -- 持续时间（默认 0.2）
    },
    outputs = { { type = "Flow" } },
})

local _shakeCoroutine = nil
Blueprint.RegisterHandler("Camera.Shake", function(ctx)
    local intensity = ctx:GetInput("Intensity"):asFloat()
    local duration  = ctx:GetInput("Duration"):asFloat()
    if intensity <= 0 then intensity = 0.3 end
    if duration  <= 0 then duration  = 0.2 end
    -- CameraShaker 是挂在 Main Camera 上的组件，必须先检查存在
    local ok, shaker = pcall(function()
        return CS.CutRope.Game.CameraShaker.Instance
    end)
    if ok and shaker then
        shaker:Shake(intensity, duration)
    else
        print('[Camera.Shake] CameraShaker not ready, skip shake')
    end
    return true
end)

-- ═══════════════════════════════════════════════════════════════════════════════
-- HUD 节点 — 直接更新 FPanelHUD 的显示
-- ═══════════════════════════════════════════════════════════════════════════════

Blueprint.RegisterNodeDef({
    id          = "HUD.SetScore",
    name        = "HUD Set Score",
    category    = "Game/HUD",
    color       = "5A6A9A",
    description = "更新 HUD 分数显示",
    inputs  = {
        { type = "Flow" },
        { name = "Score", type = "Integer" },
    },
    outputs = { { type = "Flow" } },
})
Blueprint.RegisterHandler("HUD.SetScore", function(ctx)
    local score = ctx:GetInput("Score"):asInt()
    local ok, hud = pcall(require, "ui.FPanelHUD")
    if ok and hud and type(hud.Instance) == "function" then
        local inst = hud.Instance()
        if inst and inst.SetScore then inst:SetScore(score) end
    end
    return true
end)

-- ───────────────────────────────────────────────────────────────────────────────

Blueprint.RegisterNodeDef({
    id          = "HUD.SetStars",
    name        = "HUD Set Stars",
    category    = "Game/HUD",
    color       = "5A6A9A",
    description = "更新 HUD 星星显示（★☆☆）",
    inputs  = {
        { type = "Flow" },
        { name = "Collected", type = "Integer" },
        { name = "Total",     type = "Integer" },
    },
    outputs = { { type = "Flow" } },
})
Blueprint.RegisterHandler("HUD.SetStars", function(ctx)
    local collected = ctx:GetInput("Collected"):asInt()
    local total     = ctx:GetInput("Total"):asInt()
    if total <= 0 then total = 3 end
    local ok, hud = pcall(require, "ui.FPanelHUD")
    if ok and hud and type(hud.Instance) == "function" then
        local inst = hud.Instance()
        if inst and inst.SetStars then inst:SetStars(collected, total) end
    end
    return true
end)

-- ───────────────────────────────────────────────────────────────────────────────

Blueprint.RegisterNodeDef({
    id          = "HUD.SetCuts",
    name        = "HUD Set Cuts",
    category    = "Game/HUD",
    color       = "5A6A9A",
    description = "更新 HUD 刀数显示",
    inputs  = {
        { type = "Flow" },
        { name = "Cuts", type = "Integer" },
    },
    outputs = { { type = "Flow" } },
})
Blueprint.RegisterHandler("HUD.SetCuts", function(ctx)
    local cuts = ctx:GetInput("Cuts"):asInt()
    local ok, hud = pcall(require, "ui.FPanelHUD")
    if ok and hud and type(hud.Instance) == "function" then
        local inst = hud.Instance()
        if inst and inst.SetCuts then inst:SetCuts(cuts) end
    end
    return true
end)

-- ───────────────────────────────────────────────────────────────────────────────
-- HUD.Sync — 一次性同步所有 HUD 数据（Score + Stars + Cuts）
-- ───────────────────────────────────────────────────────────────────────────────

Blueprint.RegisterNodeDef({
    id          = "HUD.Sync",
    name        = "HUD Sync",
    category    = "Game/HUD",
    color       = "5A6A9A",
    description = "一次性从 LevelController 读取状态，更新 HUD 分数+星星+刀数",
    inputs  = {
        { type = "Flow" },
        { name = "Score", type = "Integer" },  -- 外部传入分数
    },
    outputs = { { type = "Flow" } },
})
Blueprint.RegisterHandler("HUD.Sync", function(ctx)
    local score = ctx:GetInput("Score"):asInt()
    local ctrl  = CS.CutRope.Game.LevelController.Current
    local cuts      = ctrl and ctrl.CutCount      or 0
    local collected = ctrl and ctrl.StarsCollected or 0
    local total     = ctrl and ctrl.StarCount      or 3

    local ok, hud = pcall(require, "ui.FPanelHUD")
    if ok and hud and type(hud.Instance) == "function" then
        local inst = hud.Instance()
        if inst then
            if inst.SetScore and score > 0 then inst:SetScore(score) end
            if inst.SetStars then inst:SetStars(collected, total) end
            if inst.SetCuts  then inst:SetCuts(cuts)  end
        end
    end
    return true
end)

-- ═══════════════════════════════════════════════════════════════════════════════
-- Rope 补充节点：Spawn（单条） / SpawnAll（全部）
-- ═══════════════════════════════════════════════════════════════════════════════

Blueprint.RegisterNodeDef({
    id          = "Rope.Spawn",
    name        = "Rope Spawn",
    category    = "Game/Rope",
    color       = "6A3A1A",
    description = "生成第 RopeIndex 条绳子，末端连接到 Candy",
    inputs  = {
        { type = "Flow" },
        { name = "RopeIndex", type = "Integer" },  -- 默认 0
    },
    outputs = { { type = "Flow" } },
})
Blueprint.RegisterHandler("Rope.Spawn", function(ctx)
    local ctrl = CS.CutRope.Game.LevelController.Current
    if not ctrl then return true end
    local idx  = ctx:GetInput("RopeIndex"):asInt()
    local rope = ctrl:GetRope(idx)
    if rope then
        rope:Spawn(ctrl.candy)
        print(string.format('[Rope.Spawn] rope[%d] spawned', idx))
    else
        print(string.format('[Rope.Spawn] rope[%d] not found', idx))
    end
    return true
end)

-- ───────────────────────────────────────────────────────────────────────────────

Blueprint.RegisterNodeDef({
    id          = "Rope.SpawnAll",
    name        = "Rope Spawn All",
    category    = "Game/Rope",
    color       = "6A3A1A",
    description = "生成场景中所有绳子，末端全部连接到 Candy",
    inputs  = { { type = "Flow" } },
    outputs = {
        { type = "Flow" },
        { name = "Count", type = "Integer" },  -- 实际生成数量
    },
})
Blueprint.RegisterHandler("Rope.SpawnAll", function(ctx)
    print('[Rope.SpawnAll] start')
    CS.UnityEngine.Debug.Log("[Rope.SpawnAll] start")
    local ctrl = CS.CutRope.Game.LevelController.Current
    if not ctrl then
        print('[Rope.SpawnAll] LevelController not found')
        ctx:SetOutput("Count", 0)
        return true
    end
    local count = ctrl:GetRopeCount()
    for i = 0, count - 1 do
        local rope = ctrl:GetRope(i)
        if rope then rope:Spawn(ctrl.candy) end
    end
    ctx:SetOutput("Count", count)
    print(string.format('[Rope.SpawnAll] spawned %d ropes', count))
    return true
end)

-- ═══════════════════════════════════════════════════════════════════════════════
-- Level 补充：按收集星星数计算星级 / 计算分数
-- ═══════════════════════════════════════════════════════════════════════════════

Blueprint.RegisterNodeDef({
    id          = "Level.CalcStarsByCollected",
    name        = "Level Calc Stars By Collected",
    category    = "Game/Level",
    color       = "9A6A1A",
    description = "按收集到的星星数直接返回星级（收集数即星级，最少1星）",
    inputs  = { { type = "Flow" } },
    outputs = {
        { type = "Flow" },
        { name = "Stars",     type = "Integer" },
        { name = "Collected", type = "Integer" },
        { name = "Total",     type = "Integer" },
    },
})
Blueprint.RegisterHandler("Level.CalcStarsByCollected", function(ctx)
    local ctrl      = CS.CutRope.Game.LevelController.Current
    local collected = ctrl and ctrl.StarsCollected or 0
    local total     = ctrl and ctrl.StarCount      or 3
    local stars     = math.max(1, math.min(3, collected))
    ctx:SetOutput("Stars",     stars)
    ctx:SetOutput("Collected", collected)
    ctx:SetOutput("Total",     total)
    return true
end)

-- ───────────────────────────────────────────────────────────────────────────────

Blueprint.RegisterNodeDef({
    id          = "Score.Calc",
    name        = "Score Calc",
    category    = "Game/Level",
    color       = "9A6A1A",
    description = "计算分数：Base + Collected×StarBonus - Cuts×CutPenalty",
    inputs  = {
        { type = "Flow" },
        { name = "Base",       type = "Integer" },  -- 基础分，默认 100
        { name = "StarBonus",  type = "Integer" },  -- 每颗星加分，默认 50
        { name = "CutPenalty", type = "Integer" },  -- 每刀扣分，默认 10
    },
    outputs = {
        { type = "Flow" },
        { name = "Score", type = "Integer" },
    },
})
Blueprint.RegisterHandler("Score.Calc", function(ctx)
    local base       = ctx:GetInput("Base"):asInt()
    local starBonus  = ctx:GetInput("StarBonus"):asInt()
    local cutPenalty = ctx:GetInput("CutPenalty"):asInt()
    if base       <= 0 then base       = 100 end
    if starBonus  <= 0 then starBonus  = 50  end
    if cutPenalty <= 0 then cutPenalty = 10  end
    local ctrl      = CS.CutRope.Game.LevelController.Current
    local collected = ctrl and ctrl.StarsCollected or 0
    local cuts      = ctrl and ctrl.CutCount       or 0
    local score     = math.max(0, base + collected * starBonus - cuts * cutPenalty)
    ctx:SetOutput("Score", score)
    return true
end)

-- ═══════════════════════════════════════════════════════════════════════════════
-- Level.SetResult — 把最终结果推给通关面板
-- ═══════════════════════════════════════════════════════════════════════════════

Blueprint.RegisterNodeDef({
    id          = "Level.SetResult",
    name        = "Level Set Result",
    category    = "Game/Level",
    color       = "9A6A1A",
    description = "把 Stars / Score 写入通关面板（在 UI.Open LevelComplete 之前调用）",
    inputs  = {
        { type = "Flow" },
        { name = "Stars", type = "Integer" },
        { name = "Score", type = "Integer" },
    },
    outputs = { { type = "Flow" } },
})
Blueprint.RegisterHandler("Level.SetResult", function(ctx)
    local stars = ctx:GetInput("Stars"):asInt()
    local score = ctx:GetInput("Score"):asInt()
    -- 暂存到全局，LevelComplete 面板 OnCreate 时读取
    _G._pendingLevelResult = { stars = stars, score = score }
    -- 如果面板已打开，直接更新
    local ok, mod = pcall(require, "ui.FPanelLevelComplete")
    if ok and mod and type(mod.Instance) == "function" then
        local inst = mod.Instance()
        if inst and inst.SetResult then
            local LM = require 'game.level_manager'
            inst:SetResult(LM.current_level_id or '', score)
        end
    end
    print(string.format('[Level.SetResult] stars=%d score=%d', stars, score))
    return true
end)

-- ═══════════════════════════════════════════════════════════════════════════════
-- Var 节点 — 读写蓝图全局变量（门控、计数器等）
-- ═══════════════════════════════════════════════════════════════════════════════

Blueprint.RegisterNodeDef({
    id          = "Var.Set",
    name        = "Var Set",
    category    = "Game/Var",
    color       = "4A7A4A",
    description = "设置蓝图全局变量（布尔 / 数字 / 字符串）",
    inputs  = {
        { type = "Flow" },
        { name = "Key",   type = "String" },
        { name = "Value", type = "String" },  -- 用字符串传输，支持 '1'/'0'/'true'/'false' 和数字字符串
    },
    outputs = { { type = "Flow" } },
})
Blueprint.RegisterHandler("Var.Set", function(ctx)
    local key = ctx:GetInput("Key"):asString()
    local val = ctx:GetInput("Value"):asString()
    Blueprint.SetVariable(key, val)
    return true
end)

-- ───────────────────────────────────────────────────────────────────────────────

Blueprint.RegisterNodeDef({
    id          = "Var.GetBool",
    name        = "Var Get Bool",
    category    = "Game/Var",
    color       = "4A7A4A",
    description = "读取蓝图变量（返回布尔），用于 Timer.Wait 的 Enabled 引脚等门控场景",
    inputs  = {
        { type = "Flow" },
        { name = "Key", type = "String" },
    },
    outputs = {
        { type = "Flow" },
        { name = "Value", type = "Boolean" },
    },
})
Blueprint.RegisterHandler("Var.GetBool", function(ctx)
    local key = ctx:GetInput("Key"):asString()
    local raw = Blueprint.GetVariable(key)
    local val = raw == "1" or raw == "true" or raw == true
    ctx:SetOutput("Value", val)
    return true
end)

print("[game_nodes] Registered: UI(4)+Scene(4)+Level(11)+Rope(4)+Candy(1)+Star(2)+Obstacle(1)+Camera(1)+GameEvent.On*(4)+Audio(3)+Timer(2)+HUD(4)+Score(1)+Var(2) = 44 nodes")
