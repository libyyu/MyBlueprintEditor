-- data/game_extensions.lua
-- 游戏逻辑 Lua 扩展节点
--
-- 通过 BlueprintEntry.lua 加载后，可在蓝图中使用以下节点：
--   GameEvent.Emit     — 发出游戏事件（写入全局事件队列）
--   GameEvent.Poll     — 轮询并消费事件队列中的事件
--   GameEvent.Clear    — 清空指定频道的事件队列
--   Dialogue.Start     — 启动对话序列（顺序播放多行文本）
--   Dialogue.Next      — 推进到下一句对话
--   Dialogue.GetLine   — 获取当前对话内容
--   Dialogue.IsEnd     — 是否已到达对话末尾
--   Tween.Start        — 启动数值 Tween（使用 Easing.Lerp，每 tick 自动推进）
--   Tween.Update       — 每帧更新，输出当前值和进度
--   Tween.IsFinished   — 是否 Tween 完成

-- ─────────────────────────────────────────────────────────────────────────────
-- 游戏事件队列系统
-- 事件存储在蓝图变量 __gevt_<channel> (Array) 中
-- ─────────────────────────────────────────────────────────────────────────────

Blueprint.RegisterNodeDef({
    id          = "GameEvent.Emit",
    name        = "Game Event Emit",
    category    = "Game/Event",
    color       = "2A7A5A",
    description = "向指定频道发出游戏事件（写入全局事件队列）",
    inputs  = {
        { name = "In",      type = "Flow" },
        { name = "Channel", type = "String" },
        { name = "EventId", type = "String" },
        { name = "Payload", type = "String" },  -- JSON 字符串，可选
    },
    outputs = {
        { name = "Out", type = "Flow" },
    },
})
Blueprint.RegisterHandler("GameEvent.Emit", function(ctx)
    local channel = ctx:GetInput("Channel"):asString()
    local eventId = ctx:GetInput("EventId"):asString()
    local payload = ctx:GetInput("Payload"):asString()
    if channel == "" then channel = "default" end

    local key = "__gevt_" .. channel
    local queue = Blueprint.GetVariable(key)
    if type(queue) ~= "table" then queue = {} end

    table.insert(queue, {
        id      = eventId,
        payload = payload,
        time    = os.time(),
    })
    Blueprint.SetVariable(key, queue)
    return true
end)

-- ─────────────────────────────────────────────────────────────────────────────

Blueprint.RegisterNodeDef({
    id          = "GameEvent.Poll",
    name        = "Game Event Poll",
    category    = "Game/Event",
    color       = "2A7A5A",
    description = "消费事件队列中最早的一个事件",
    inputs  = {
        { name = "In",      type = "Flow" },
        { name = "Channel", type = "String" },
    },
    outputs = {
        { name = "onEvent",  type = "Flow" },  -- 有事件
        { name = "onEmpty",  type = "Flow" },  -- 队列空
        { name = "EventId",  type = "String" },
        { name = "Payload",  type = "String" },
        { name = "Remaining",type = "Integer" },
    },
})
Blueprint.RegisterHandler("GameEvent.Poll", function(ctx)
    local channel = ctx:GetInput("Channel"):asString()
    if channel == "" then channel = "default" end

    local key = "__gevt_" .. channel
    local queue = Blueprint.GetVariable(key)
    if type(queue) ~= "table" or #queue == 0 then
        ctx:SetOutput("EventId",   "")
        ctx:SetOutput("Payload",   "")
        ctx:SetOutput("Remaining", 0)
        ctx:ActivateOutputFlow("onEmpty")
        return true
    end

    local evt = table.remove(queue, 1)
    Blueprint.SetVariable(key, queue)

    ctx:SetOutput("EventId",   evt.id or "")
    ctx:SetOutput("Payload",   evt.payload or "")
    ctx:SetOutput("Remaining", #queue)
    ctx:ActivateOutputFlow("onEvent")
    return true
end)

-- ─────────────────────────────────────────────────────────────────────────────

Blueprint.RegisterNodeDef({
    id          = "GameEvent.Clear",
    name        = "Game Event Clear",
    category    = "Game/Event",
    color       = "2A7A5A",
    description = "清空指定频道的事件队列",
    inputs  = {
        { name = "In",      type = "Flow" },
        { name = "Channel", type = "String" },
    },
    outputs = { { name = "Out", type = "Flow" } },
})
Blueprint.RegisterHandler("GameEvent.Clear", function(ctx)
    local channel = ctx:GetInput("Channel"):asString()
    if channel == "" then channel = "default" end
    Blueprint.SetVariable("__gevt_" .. channel, {})
    return true
end)

-- ─────────────────────────────────────────────────────────────────────────────
-- 对话系统
-- 对话序列存储在蓝图变量 __dlg_<id>_lines (Array) 和 __dlg_<id>_pos (Integer)
-- ─────────────────────────────────────────────────────────────────────────────

Blueprint.RegisterNodeDef({
    id          = "Dialogue.Start",
    name        = "Dialogue Start",
    category    = "Game/Dialogue",
    color       = "7A5A2A",
    description = "启动对话序列（Lines 为 Array of String）",
    inputs  = {
        { name = "In",       type = "Flow" },
        { name = "DialogueId", type = "String" },
        { name = "Lines",    type = "Array" },
    },
    outputs = {
        { name = "Out",       type = "Flow" },
        { name = "FirstLine", type = "String" },
    },
})
Blueprint.RegisterHandler("Dialogue.Start", function(ctx)
    local id    = ctx:GetInput("DialogueId"):asString()
    if id == "" then id = "default" end
    local lines = ctx:GetInput("Lines")
    -- lines 是 Variant，转为 Lua table
    local arr = {}
    if lines and lines.arraySize then
        for i = 0, lines:arraySize()-1 do
            table.insert(arr, lines:arrayGet(i):asString())
        end
    end
    Blueprint.SetVariable("__dlg_" .. id .. "_lines", arr)
    Blueprint.SetVariable("__dlg_" .. id .. "_pos",   1)

    local first = #arr > 0 and arr[1] or ""
    ctx:SetOutput("FirstLine", first)
    return true
end)

-- ─────────────────────────────────────────────────────────────────────────────

Blueprint.RegisterNodeDef({
    id          = "Dialogue.Next",
    name        = "Dialogue Next",
    category    = "Game/Dialogue",
    color       = "7A5A2A",
    description = "推进到下一句对话",
    inputs  = {
        { name = "In",         type = "Flow" },
        { name = "DialogueId", type = "String" },
    },
    outputs = {
        { name = "onLine", type = "Flow" },  -- 有下一句
        { name = "onEnd",  type = "Flow" },  -- 对话结束
        { name = "Line",   type = "String" },
        { name = "Index",  type = "Integer" },
    },
})
Blueprint.RegisterHandler("Dialogue.Next", function(ctx)
    local id = ctx:GetInput("DialogueId"):asString()
    if id == "" then id = "default" end

    local lines = Blueprint.GetVariable("__dlg_" .. id .. "_lines")
    local pos   = Blueprint.GetVariable("__dlg_" .. id .. "_pos")
    if type(lines) ~= "table" then lines = {} end
    local p = (type(pos) == "number" and pos or 1) + 1

    if p > #lines then
        ctx:SetOutput("Line",  "")
        ctx:SetOutput("Index", #lines)
        ctx:ActivateOutputFlow("onEnd")
    else
        Blueprint.SetVariable("__dlg_" .. id .. "_pos", p)
        ctx:SetOutput("Line",  lines[p] or "")
        ctx:SetOutput("Index", p)
        ctx:ActivateOutputFlow("onLine")
    end
    return true
end)

-- ─────────────────────────────────────────────────────────────────────────────

Blueprint.RegisterNodeDef({
    id          = "Dialogue.GetLine",
    name        = "Dialogue Get Line",
    category    = "Game/Dialogue",
    color       = "7A5A2A",
    description = "获取对话当前句内容（不推进）",
    inputs  = { { name = "DialogueId", type = "String" } },
    outputs = {
        { name = "Line",    type = "String" },
        { name = "Index",   type = "Integer" },
        { name = "IsEnd",   type = "Boolean" },
    },
})
Blueprint.RegisterHandler("Dialogue.GetLine", function(ctx)
    local id = ctx:GetInput("DialogueId"):asString()
    if id == "" then id = "default" end
    local lines = Blueprint.GetVariable("__dlg_" .. id .. "_lines")
    local pos   = Blueprint.GetVariable("__dlg_" .. id .. "_pos")
    if type(lines) ~= "table" then lines = {} end
    local p = type(pos) == "number" and pos or 1
    ctx:SetOutput("Line",  lines[p] or "")
    ctx:SetOutput("Index", p)
    ctx:SetOutput("IsEnd", p > #lines)
    return true
end)

-- ─────────────────────────────────────────────────────────────────────────────
-- Tween 系统
-- 存储：__tween_<id>_from / _to / _duration / _elapsed / _curve
-- ─────────────────────────────────────────────────────────────────────────────

Blueprint.RegisterNodeDef({
    id          = "Tween.Start",
    name        = "Tween Start",
    category    = "Game/Tween",
    color       = "5A2A7A",
    description = "启动数值 Tween（配合 Tween.Update 每帧调用）",
    inputs  = {
        { name = "In",       type = "Flow" },
        { name = "TweenId",  type = "String" },
        { name = "From",     type = "Float" },
        { name = "To",       type = "Float" },
        { name = "Duration", type = "Float" },
        { name = "Curve",    type = "String" },  -- 缓动曲线名，见 Easing.Apply
    },
    outputs = { { name = "Out", type = "Flow" } },
})
Blueprint.RegisterHandler("Tween.Start", function(ctx)
    local id  = ctx:GetInput("TweenId"):asString()
    if id == "" then id = "default" end
    local from = ctx:GetInput("From"):asFloat()
    local to   = ctx:GetInput("To"):asFloat()
    local dur  = ctx:GetInput("Duration"):asFloat()
    local curve= ctx:GetInput("Curve"):asString()
    if curve == "" then curve = "Linear" end
    Blueprint.SetVariable("__tw_" .. id .. "_from",    from)
    Blueprint.SetVariable("__tw_" .. id .. "_to",      to)
    Blueprint.SetVariable("__tw_" .. id .. "_dur",     dur)
    Blueprint.SetVariable("__tw_" .. id .. "_elapsed", 0)
    Blueprint.SetVariable("__tw_" .. id .. "_curve",   curve)
    return true
end)

-- ─────────────────────────────────────────────────────────────────────────────

Blueprint.RegisterNodeDef({
    id          = "Tween.Update",
    name        = "Tween Update",
    category    = "Game/Tween",
    color       = "5A2A7A",
    description = "每帧推进 Tween（传入 DeltaTime），输出当前插值",
    inputs  = {
        { name = "In",        type = "Flow" },
        { name = "TweenId",   type = "String" },
        { name = "DeltaTime", type = "Float" },
    },
    outputs = {
        { name = "onTick",     type = "Flow" },  -- 正在运行
        { name = "onFinished", type = "Flow" },  -- 完成
        { name = "Value",      type = "Float" },
        { name = "Progress",   type = "Float" },  -- 0~1
    },
})
Blueprint.RegisterHandler("Tween.Update", function(ctx)
    local id  = ctx:GetInput("TweenId"):asString()
    if id == "" then id = "default" end
    local dt  = ctx:GetInput("DeltaTime"):asFloat()

    local from    = Blueprint.GetVariable("__tw_" .. id .. "_from")    or 0
    local to      = Blueprint.GetVariable("__tw_" .. id .. "_to")      or 0
    local dur     = Blueprint.GetVariable("__tw_" .. id .. "_dur")     or 1
    local elapsed = Blueprint.GetVariable("__tw_" .. id .. "_elapsed") or 0
    local curve   = Blueprint.GetVariable("__tw_" .. id .. "_curve")   or "Linear"

    elapsed = math.min(elapsed + dt, dur)
    Blueprint.SetVariable("__tw_" .. id .. "_elapsed", elapsed)

    local t = dur > 0 and (elapsed / dur) or 1
    t = math.max(0, math.min(1, t))

    -- 应用缓动（简单线性，完整缓动需要 C++ Easing.Apply）
    -- 此处内置 Quad 常用曲线，其余走 Linear
    local et = t
    if     curve == "InQuad"      then et = t * t
    elseif curve == "OutQuad"     then et = t * (2 - t)
    elseif curve == "InOutQuad"   then et = t < .5 and 2*t*t or -1+(4-2*t)*t
    elseif curve == "InCubic"     then et = t * t * t
    elseif curve == "OutCubic"    then local u=1-t; et = 1-u*u*u
    elseif curve == "InOutCubic"  then et = t<.5 and 4*t*t*t or (t-1)*(2*t-2)*(2*t-2)+1
    end

    local value = from + (to - from) * et
    ctx:SetOutput("Value",    value)
    ctx:SetOutput("Progress", t)

    if elapsed >= dur then
        ctx:ActivateOutputFlow("onFinished")
    else
        ctx:ActivateOutputFlow("onTick")
    end
    return true
end)

-- ─────────────────────────────────────────────────────────────────────────────

Blueprint.RegisterNodeDef({
    id          = "Tween.IsFinished",
    name        = "Tween Is Finished",
    category    = "Game/Tween",
    color       = "5A2A7A",
    description = "检查 Tween 是否已完成",
    inputs  = { { name = "TweenId", type = "String" } },
    outputs = {
        { name = "Finished", type = "Boolean" },
        { name = "Progress", type = "Float" },
    },
})
Blueprint.RegisterHandler("Tween.IsFinished", function(ctx)
    local id  = ctx:GetInput("TweenId"):asString()
    if id == "" then id = "default" end
    local dur     = Blueprint.GetVariable("__tw_" .. id .. "_dur")     or 1
    local elapsed = Blueprint.GetVariable("__tw_" .. id .. "_elapsed") or 0
    local t = dur > 0 and math.min(elapsed / dur, 1) or 1
    ctx:SetOutput("Finished", elapsed >= dur)
    ctx:SetOutput("Progress", t)
    return true
end)

print("[GameExtensions] Loaded: GameEvent / Dialogue / Tween nodes registered.")
