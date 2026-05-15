-- UpdateLogic.lua
-- 更新阶段入口（Phase 3）
-- 由 LuaManager.RunUpdateLuaVM("UpdateLogic") 加载
--
-- 职责：
--   1. 检查是否需要热更新资源（后续接 YooAsset 远端 API）
--   2. 下载更新包（如有）
--   3. 设置 UpdateLogicDone = true 表示更新阶段完成
--
-- 当前版本：跳过更新直接完成（离线/本地开发模式）

print("[UpdateLogic] *** Update phase start ***")

-- 当前为离线模式：无远端检查，直接通过
-- TODO: 后期接入热更新时，在这里：
--   1. 调用 YooAssetsLuaBridge.CheckUpdate(...)
--   2. 下载差异包（显示进度 UI）
--   3. 完成后设置 UpdateLogicDone = true

UpdateLogicDone   = true
UpdateLogicResult = true   -- true = 更新成功 / 无需更新

print("[UpdateLogic] *** Update phase done (offline mode) ***")
