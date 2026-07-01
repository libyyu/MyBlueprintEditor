-- pet_nodes.lua
-- 桌宠原子动作蓝图节点。
-- 设计原则：每个节点 = 桌宠的一个原子能力，可在蓝图编辑器里可视化连线编排。
-- handler 调用 C# 薄引擎层（PetWindowService）或通过 FLibEvent 解耦通知表现层。
--
-- 仿照 game_extensions.lua 的注册写法：
--   Blueprint.RegisterNodeDef{ id, name, category, color, pins }
--   Blueprint.RegisterHandler(id, function(ctx) ... end)

local M = {}

local function PetWindow()
    return CS.Pet.Runtime.PetWindowService.Instance
end

-- ════════════════════════════════════════════════════════════════════════
-- Pet.SetTopmost —— 设置窗口置顶
-- ════════════════════════════════════════════════════════════════════════
Blueprint.RegisterNodeDef({
    id       = 'Pet.SetTopmost',
    name     = 'Set Topmost',
    category = 'Pet/Window',
    color    = 'FF9933',
    pins = {
        { name = 'Exec',    dataType = 'Exec', isInput = true,  isExec = true },
        { name = 'Done',    dataType = 'Exec', isInput = false, isExec = true },
        { name = 'Topmost', dataType = 'Bool', isInput = true },
    }
})
Blueprint.RegisterHandler('Pet.SetTopmost', function(ctx)
    local win = PetWindow()
    if win then win:SetTopmost(ctx:GetInputBool('Topmost')) end
    ctx:ActivateOutputFlow('Done')
    return true
end)

-- ════════════════════════════════════════════════════════════════════════
-- Pet.MoveTo —— 移动桌宠窗口到屏幕坐标
-- ════════════════════════════════════════════════════════════════════════
Blueprint.RegisterNodeDef({
    id       = 'Pet.MoveTo',
    name     = 'Move Window To',
    category = 'Pet/Window',
    color    = 'FF9933',
    pins = {
        { name = 'Exec', dataType = 'Exec',  isInput = true,  isExec = true },
        { name = 'Done', dataType = 'Exec',  isInput = false, isExec = true },
        { name = 'X',    dataType = 'Int',   isInput = true },
        { name = 'Y',    dataType = 'Int',   isInput = true },
    }
})
Blueprint.RegisterHandler('Pet.MoveTo', function(ctx)
    local win = PetWindow()
    if win then
        win:MoveWindow(math.floor(ctx:GetInputInt('X')), math.floor(ctx:GetInputInt('Y')))
    end
    ctx:ActivateOutputFlow('Done')
    return true
end)

-- ════════════════════════════════════════════════════════════════════════
-- Pet.Say —— 桌宠说话（气泡）。表现层由 UI 监听事件实现。
-- ════════════════════════════════════════════════════════════════════════
Blueprint.RegisterNodeDef({
    id       = 'Pet.Say',
    name     = 'Pet Say',
    category = 'Pet/Action',
    color    = '3399FF',
    pins = {
        { name = 'Exec',     dataType = 'Exec',   isInput = true,  isExec = true },
        { name = 'Done',     dataType = 'Exec',   isInput = false, isExec = true },
        { name = 'Content',  dataType = 'String', isInput = true },
        { name = 'Duration', dataType = 'Float',  isInput = true },
    }
})
Blueprint.RegisterHandler('Pet.Say', function(ctx)
    local content  = ctx:GetInputString('Content')
    local duration = ctx:GetInputFloat('Duration')
    print(string.format('[Pet.Say] %s (%.1fs)', content, duration))
    if _G.FLibEvent then
        _G.FLibEvent.Emit('pet.say', { content = content, duration = duration })
    end
    ctx:ActivateOutputFlow('Done')
    return true
end)

-- ════════════════════════════════════════════════════════════════════════
-- Pet.PlayAnim —— 播放桌宠动画。表现层由 PetAnimator(C#) 监听事件实现。
-- ════════════════════════════════════════════════════════════════════════
Blueprint.RegisterNodeDef({
    id       = 'Pet.PlayAnim',
    name     = 'Play Animation',
    category = 'Pet/Action',
    color    = '66CC33',
    pins = {
        { name = 'Exec', dataType = 'Exec',   isInput = true,  isExec = true },
        { name = 'Done', dataType = 'Exec',   isInput = false, isExec = true },
        { name = 'Name', dataType = 'String', isInput = true },
    }
})
Blueprint.RegisterHandler('Pet.PlayAnim', function(ctx)
    local name = ctx:GetInputString('Name')
    print('[Pet.PlayAnim] ' .. tostring(name))
    if _G.FLibEvent then
        _G.FLibEvent.Emit('pet.play_anim', { name = name })
    end
    ctx:ActivateOutputFlow('Done')
    return true
end)

print('[pet_nodes] registered: Pet.SetTopmost / Pet.MoveTo / Pet.Say / Pet.PlayAnim')

return M
