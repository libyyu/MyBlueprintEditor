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
-- ═══════════════════════════════════════════════════════════════════════════════

Blueprint.RegisterNodeDef({
    id          = "UI.Open",
    name        = "UI Open Panel",
    category    = "Game/UI",
    color       = "5A3A9A",
    description = "异步打开 UI 面板（从 YooAsset 加载 Prefab）",
    inputs  = {
        { name = "In",      type = "Flow"   },
        { name = "Address", type = "String" },  -- YooAsset address，如 "UI/MainMenu"
        { name = "Param",   type = "String" },  -- 传给 IView.Show 的参数，可为空
    },
    outputs = {
        { name = "Out",  type = "Flow" },  -- 面板打开后
    },
})
Blueprint.RegisterHandler("UI.Open", function(ctx)
    local address = ctx:GetInput("Address"):asString()
    local param   = ctx:GetInput("Param"):asString()
    if param == "" then param = nil end
    CS.CutRope.Framework.UIManager.LuaOpen(address, param, nil)
    return true
end)

-- ───────────────────────────────────────────────────────────────────────────────

Blueprint.RegisterNodeDef({
    id          = "UI.Close",
    name        = "UI Close Panel",
    category    = "Game/UI",
    color       = "5A3A9A",
    description = "隐藏 UI 面板（不销毁，保留状态）",
    inputs  = {
        { name = "In",      type = "Flow"   },
        { name = "Address", type = "String" },
    },
    outputs = {
        { name = "Out", type = "Flow" },
    },
})
Blueprint.RegisterHandler("UI.Close", function(ctx)
    local address = ctx:GetInput("Address"):asString()
    CS.CutRope.Framework.UIManager.LuaClose(address)
    return true
end)

-- ───────────────────────────────────────────────────────────────────────────────

Blueprint.RegisterNodeDef({
    id          = "UI.Dispose",
    name        = "UI Dispose Panel",
    category    = "Game/UI",
    color       = "5A3A9A",
    description = "销毁 UI 面板并释放资源",
    inputs  = {
        { name = "In",      type = "Flow"   },
        { name = "Address", type = "String" },
    },
    outputs = {
        { name = "Out", type = "Flow" },
    },
})
Blueprint.RegisterHandler("UI.Dispose", function(ctx)
    local address = ctx:GetInput("Address"):asString()
    CS.CutRope.Framework.UIManager.LuaDispose(address)
    return true
end)

-- ───────────────────────────────────────────────────────────────────────────────

Blueprint.RegisterNodeDef({
    id          = "UI.CloseAll",
    name        = "UI Close All",
    category    = "Game/UI",
    color       = "5A3A9A",
    description = "隐藏所有 UI 面板",
    inputs  = { { name = "In", type = "Flow" } },
    outputs = { { name = "Out", type = "Flow" } },
})
Blueprint.RegisterHandler("UI.CloseAll", function(ctx)
    CS.CutRope.Framework.UIManager.LuaCloseAll()
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
        { name = "In",    type = "Flow"   },
        { name = "Scene", type = "String" },  -- YooAsset address 或 Build Settings 场景名
    },
    outputs = {
        { name = "Out", type = "Flow" },
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
        { name = "In",    type = "Flow"   },
        { name = "Scene", type = "String" },
    },
    outputs = { { name = "Out", type = "Flow" } },
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
        { name = "In",    type = "Flow"   },
        { name = "Scene", type = "String" },
    },
    outputs = { { name = "Out", type = "Flow" } },
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
    inputs  = { { name = "In", type = "Flow" } },
    outputs = {
        { name = "Out",  type = "Flow"   },
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
        { name = "In",    type = "Flow"    },
        { name = "Stars", type = "Integer" },  -- 1~3
        { name = "Score", type = "Integer" },
    },
    outputs = { { name = "Out", type = "Flow" } },
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
    inputs  = { { name = "In", type = "Flow" } },
    outputs = { { name = "Out", type = "Flow" } },
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
    inputs  = { { name = "In", type = "Flow" } },
    outputs = { { name = "Out", type = "Flow" } },
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
    inputs  = { { name = "In", type = "Flow" } },
    outputs = { { name = "Out", type = "Flow" } },
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
    inputs  = { { name = "In", type = "Flow" } },
    outputs = {
        { name = "Out",         type = "Flow"    },
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
    inputs  = { { name = "In", type = "Flow" } },
    outputs = { { name = "Out", type = "Flow" } },
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
        { name = "In",        type = "Flow"    },
        { name = "RopeIndex", type = "Integer" },  -- 第几条绳子
        { name = "NodeIndex", type = "Integer" },  -- 第几个节点处切断
    },
    outputs = { { name = "Out", type = "Flow" } },
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
    inputs  = { { name = "In", type = "Flow" } },
    outputs = {
        { name = "Out",   type = "Flow"    },
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
    inputs  = { { name = "In", type = "Flow" } },
    outputs = {
        { name = "Out",      type = "Flow"    },
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
    inputs  = { { name = "In", type = "Flow" } },
    outputs = { { name = "Out", type = "Flow" } },
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
    inputs  = { { name = "In", type = "Flow" } },
    outputs = { { name = "Out", type = "Flow" } },
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
        { name = "In",      type = "Flow"    },
        { name = "MaxCuts", type = "Integer" },
    },
    outputs = {
        { name = "Out",   type = "Flow"    },
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
    inputs  = { { name = "In", type = "Flow" } },
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
    inputs  = { { name = "In", type = "Flow" } },
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
    inputs  = { { name = "In", type = "Flow" } },
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
        { name = "In",     type = "Flow"  },
        { name = "Clip",   type = "String" },
        { name = "Volume", type = "Float"  },
    },
    outputs = { { name = "Out", type = "Flow" } },
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
        { name = "In",       type = "Flow"   },
        { name = "Clip",     type = "String" },
        { name = "FadeTime", type = "Float"  },
    },
    outputs = { { name = "Out", type = "Flow" } },
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
        { name = "In",       type = "Flow"  },
        { name = "FadeTime", type = "Float" },
    },
    outputs = { { name = "Out", type = "Flow" } },
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
        { name = "In",        type = "Flow"   },
        { name = "TimerId",   type = "String" },
        { name = "Duration",  type = "Float"  },
        { name = "DeltaTime", type = "Float"  },
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
    local dur  = ctx:GetInput("Duration"):asFloat()
    local dt   = ctx:GetInput("DeltaTime"):asFloat()
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
        { name = "In",      type = "Flow"   },
        { name = "TimerId", type = "String" },
    },
    outputs = { { name = "Out", type = "Flow" } },
})
Blueprint.RegisterHandler("Timer.Reset", function(ctx)
    local id = ctx:GetInput("TimerId"):asString()
    if id == "" then id = "default_timer" end
    Blueprint.SetVariable("__timer_" .. id, 0)
    return true
end)

print("[game_nodes] Registered: UI(4)+Scene(4)+Level(9)+Rope(2)+Candy(1)+GameEvent.On*(3)+Audio(3)+Timer(2) = 28 nodes")
