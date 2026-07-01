-- PetLogic.lua
-- 桌宠主逻辑入口（由 GameLauncher 通过 Pet.asset 的 gameLuaEntry="pet.PetLogic" 加载）
-- 对应游戏侧的 GameLogic.lua，但面向桌宠形态。
--
-- 此时 xLua 主 VM 已就绪，Blueprint.* 全局表已由 BlueprintService 注入。

print("[PetLogic] *** Pet phase start ***")

require "preload"

-- ── 1. 注册桌宠蓝图节点 ──────────────────────────────────────────────────
local ok, err = pcall(require, 'blueprints.PetBlueprintEntry')
if not ok then
    print('[PetLogic] Warning: PetBlueprintEntry load failed: ' .. tostring(err))
end

-- ── 2. 应用桌宠窗口（透明 / 置顶）──────────────────────────────────────────
-- PetWindowService.applyOnStart 已在打包后自动应用；这里显式确保一次。
local PetWindow = CS.Pet.Runtime.PetWindowService.Instance
if PetWindow then
    PetWindow:ApplyTransparentWindow()
    print('[PetLogic] window applied, supported=' .. tostring(PetWindow.IsSupported))
else
    print('[PetLogic] Warning: PetWindowService.Instance is nil')
end

-- ── 3. 加载并执行桌宠行为蓝图 ────────────────────────────────────────────
-- 蓝图 = OnBeginPlay → Pet.SetTopmost(true) → Pet.Say("你好…")
local BPService = CS.UGFramework.Runtime.BlueprintRuntime.BlueprintService.Instance
local petRunner = nil

local bjsonText = CS.UGFramework.Runtime.YooAssetsLuaBridge.GetLuaText('Assets/Lua/pet/behavior.bjson')
if bjsonText and #bjsonText > 0 then
    petRunner = BPService:CreateRunner()
    petRunner:LoadFromJson(bjsonText)
    if petRunner.IsLoaded then
        petRunner:Execute()   -- 触发 OnBeginPlay 流：SetTopmost → Say
        print('[PetLogic] behavior blueprint executed')
    else
        print('[PetLogic] Warning: behavior blueprint not loaded')
    end
else
    print('[PetLogic] Warning: behavior.bjson text empty/not found')
end

-- ── 4. 全局生命周期钩子（LuaManager 每帧 / 销毁时回调）────────────────────
function onAppTick(dt)
    if petRunner then petRunner:Tick(dt) end
end

function onAppDestroy()
    print('[PetLogic] onAppDestroy')
    if petRunner then
        BPService:ReleaseRunner(petRunner)
        petRunner = nil
    end
end

print("[PetLogic] *** Pet phase ready ***")
