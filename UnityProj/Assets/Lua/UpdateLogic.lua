-- UpdateLogic.lua
-- 更新阶段入口（Phase 3）
-- 由 LuaManager.RunUpdateLuaVM("UpdateLogic") 加载
--
-- 流程：
--   1. 显示更新 UI（进度条）
--   2. 检查 CDN 配置，无配置直接完成
--   3. 尝试联网拉最新版本号（超时 5s 视为无网络）
--   4. 若版本号与本地相同 → 直接完成
--   5. 若不同 → UpdateManifest → 统计下载量
--      · 无需下载 → 直接完成
--      · 需要下载 → 下载（进度条同步）→ 完成
--   6. 任何网络失败 → 回退到内置版本继续（弱联网容错）
--
-- 关键约定：
--   UpdateLogicDone   = true  表示此 VM 可以退出
--   UpdateLogicResult = true  表示更新成功（或无需更新）

print("[UpdateLogic] *** Update phase start ***")
require "preload"

local VERSION_TIMEOUT = 5   -- 拉版本号超时秒数
local Bridge = CS.YooAssetsLuaBridge

-- ── 生命周期钩子 ─────────────────────────────────────────────────────────
function onAppTick(deltaTime)
    TickCoroutine(deltaTime)
end

function onAppDestroy()
    print("[UpdateLogic] onAppDestroy")
    coro.clear()
end

-- ── UI 初始化 ─────────────────────────────────────────────────────────────
local FGUIMan = require "ui.FGUIMan"
FGUIMan.Instance():InitUIRoot()

local UpdateUI = require "ui.FPanelUpdateUI"
UpdateUI.Instance():ShowPanel(true)

-- ── 工具函数 ─────────────────────────────────────────────────────────────

local function setProgress(value)
    UpdateUI.Instance():SetProgress(value)
end

local function finish(success, reason)
    print(string.format("[UpdateLogic] done: success=%s reason=%s",
          tostring(success), tostring(reason or "")))
    setProgress(1.0)
    UpdateLogicDone   = true
    UpdateLogicResult = success
end

-- ── Step 3：下载资源 ─────────────────────────────────────────────────────
local function do_download(version)
    local count = Bridge.GetDownloadCount(DefaultPackageName, 10, 3)
    if count == 0 then
        print("[UpdateLogic] No files to download, up to date: " .. version)
        return finish(true, "no_download")
    end

    print(string.format("[UpdateLogic] Downloading %d files for version %s ...", count, version))

    Bridge.StartDownload(DefaultPackageName, 10, 3,
        function(total, downloaded, totalBytes, downloadedBytes)
            local pct = total > 0 and (downloaded / total) or 0
            setProgress(0.3 + pct * 0.7)   -- 进度 30%~100%
            print(string.format("[UpdateLogic] Download %d/%d (%.0f%%)",
                  downloaded, total, pct * 100))
        end,
        function(ok, err)
            if ok then
                print("[UpdateLogic] Download complete: " .. version)
                finish(true, "downloaded")
            else
                print("[UpdateLogic] Download failed: " .. tostring(err) .. " — using builtin")
                finish(true, "download_failed_fallback")
            end
        end
    )
end

-- ── Step 2：更新 Manifest ────────────────────────────────────────────────
local function do_update_manifest(remoteVersion)
    print("[UpdateLogic] Remote version: " .. remoteVersion .. " — updating manifest...")
    setProgress(0.2)
    Bridge.UpdateManifest(DefaultPackageName, remoteVersion,
        function(ok, err)
            if ok then
                print("[UpdateLogic] Manifest updated to: " .. remoteVersion)
                setProgress(0.3)
                do_download(remoteVersion)
            else
                print("[UpdateLogic] Manifest update failed: " .. tostring(err) .. " — using builtin")
                finish(true, "manifest_failed_fallback")
            end
        end
    )
end

-- ── Step 1：拉远端版本号（弱联网，超时容错）─────────────────────────────
local function check_remote_version()
    setProgress(0.05)

    local bTimeout   = false
    local bFinished  = false
    local bResponded = false
    local RemoteVersion = nil

    Bridge.RequestVersion(DefaultPackageName,
        function(ok, remoteVersion, err)
            if bTimeout then return end
            bResponded    = ok
            RemoteVersion = remoteVersion
            bFinished     = true
        end
    )

    local startTime = CS.UnityEngine.Time.realtimeSinceStartup
    while not bFinished do
        if CS.UnityEngine.Time.realtimeSinceStartup - startTime >= VERSION_TIMEOUT then
            bTimeout  = true
            bFinished = true
        end
        coro.yield()
    end

    if bTimeout then
        print("[UpdateLogic] RequestVersion timeout — using builtin")
        finish(true, "timeout_fallback")
        return
    end

    if not bResponded then
        print("[UpdateLogic] RequestVersion failed — using builtin")
        finish(true, "no_response_fallback")
        return
    end

    local localVersion = Bridge.GetPackageVersion(DefaultPackageName)
    print(string.format("[UpdateLogic] local=%s  remote=%s",
          tostring(localVersion), tostring(RemoteVersion)))

    setProgress(0.1)

    if RemoteVersion == localVersion then
        print("[UpdateLogic] Already up to date: " .. localVersion)
        finish(true, "up_to_date")
        return
    end

    do_update_manifest(RemoteVersion)
end

-- ── 入口：检查是否有 CDN 配置 ────────────────────────────────────────────
local cdnUrl = ""
local initializer = CS.UnityEngine.Object.FindObjectOfType(
    typeof(CS.CutRope.Framework.YooAssetInitializer))
if initializer then
    cdnUrl = initializer.cdnBaseUrl or ""
end

if cdnUrl == "" then
    print("[UpdateLogic] No CDN configured — skip update check")
    finish(true, "no_cdn")
else
    coro.start(function()
        check_remote_version()
    end)
end
