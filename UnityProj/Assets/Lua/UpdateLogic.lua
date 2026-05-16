-- UpdateLogic.lua
-- 更新阶段入口（Phase 3）
-- 由 LuaManager.RunUpdateLuaVM("UpdateLogic") 加载
--
-- 流程：
--   1. 尝试联网拉最新版本号（超时 5s 视为无网络）
--   2. 若版本号与当前内置版本相同 → 直接完成
--   3. 若不同 → UpdateManifest + 统计待下载量
--      · 无需下载 → 直接完成
--      · 需要下载 → 下载（带进度）→ 完成
--   4. 任何网络失败 → 回退到内置版本继续（弱联网容错）
--
-- 关键约定：
--   UpdateLogicDone   = true  表示此 VM 可以退出
--   UpdateLogicResult = true  表示更新成功（或无需更新）
--
-- C# YooAssetsLuaBridge API 映射：
--   Bridge.RequestVersion(pkg, cb(ok, ver, err))
--   Bridge.UpdateManifest(pkg, ver, cb(ok, err))
--   Bridge.GetDownloadCount(pkg, concurrent, retry)  → int
--   Bridge.StartDownload(pkg, concurrent, retry, onProgress, onComplete)

print("[UpdateLogic] *** Update phase start ***")

local PACKAGE       = "DefaultPackage"
local VERSION_TIMEOUT = 5   -- 拉版本号超时秒数
local Bridge = CS.YooAssetsLuaBridge

-- ── 工具：等待指定秒数（轮询计时，依赖 onAppTick 驱动）──────────────────
-- UpdateLogic VM 的 onAppTick 由 LuaManager.Update 驱动
local _wait_until = 0
local _wait_done  = false
local function wait_seconds(sec, cb)
    -- 用协程替代：通过 _wait_elapsed 自增
    -- 注意：这里我们用简单的时间戳轮询，不需要真协程
    _wait_done = false
    local deadline = os.clock() + sec
    local function poll()
        if os.clock() >= deadline then
            _wait_done = true
            cb()
        end
    end
    return poll
end

-- ── 主更新逻辑 ──────────────────────────────────────────────────────────

local function finish(success, reason)
    print(string.format("[UpdateLogic] done: success=%s reason=%s",
          tostring(success), tostring(reason or "")))
    UpdateLogicDone   = true
    UpdateLogicResult = success
end

-- Step 3：下载
local function do_download(version)
    local count = Bridge.GetDownloadCount(PACKAGE, 10, 3)
    if count == 0 then
        print("[UpdateLogic] No files to download, up to date: " .. version)
        return finish(true, "no_download")
    end

    print(string.format("[UpdateLogic] Downloading %d files for version %s ...", count, version))
    -- TODO: 这里可以打开 UI 进度条：CS.CutRope.Framework.UIManager.ShowUpdateProgress()

    Bridge.StartDownload(PACKAGE, 10, 3,
        function(total, downloaded, totalBytes, downloadedBytes)
            local pct = total > 0 and math.floor(downloaded / total * 100) or 0
            print(string.format("[UpdateLogic] Download %d/%d (%d%%)", downloaded, total, pct))
            -- TODO: 更新进度条 UI
        end,
        function(ok, err)
            if ok then
                print("[UpdateLogic] Download complete: " .. version)
                finish(true, "downloaded")
            else
                print("[UpdateLogic] Download failed: " .. tostring(err) .. " — using buildin")
                finish(true, "download_failed_fallback")
            end
        end
    )
end

-- Step 2：拉到新版本号后更新 Manifest
local function do_update_manifest(remoteVersion)
    print("[UpdateLogic] Remote version: " .. remoteVersion .. " — updating manifest...")
    Bridge.UpdateManifest(PACKAGE, remoteVersion,
        function(ok, err)
            if ok then
                print("[UpdateLogic] Manifest updated to: " .. remoteVersion)
                do_download(remoteVersion)
            else
                -- Manifest 拉取失败（CDN 有问题），回退到内置
                print("[UpdateLogic] Manifest update failed: " .. tostring(err) .. " — using buildin")
                finish(true, "manifest_failed_fallback")
            end
        end
    )
end

-- Step 1：尝试拉远端版本号（弱联网）
local function check_remote_version()
    local responded = false

    Bridge.RequestVersion(PACKAGE,
        function(ok, remoteVersion, err)
            if responded then return end   -- 超时已触发，忽略迟到回调
            responded = true

            if not ok then
                print("[UpdateLogic] RequestVersion failed: " .. tostring(err) .. " — using buildin")
                return finish(true, "offline_fallback")
            end

            local localVersion = Bridge.GetPackageVersion(PACKAGE)
            print(string.format("[UpdateLogic] local=%s  remote=%s",
                  tostring(localVersion), tostring(remoteVersion)))

            if remoteVersion == localVersion then
                print("[UpdateLogic] Already up to date: " .. localVersion)
                return finish(true, "up_to_date")
            end

            do_update_manifest(remoteVersion)
        end
    )

    -- 超时保护：VERSION_TIMEOUT 秒后如果还没回调，直接跳过
    local deadline = os.clock() + VERSION_TIMEOUT
    local function timeout_poll()
        if responded then return end
        if os.clock() >= deadline then
            responded = true
            print("[UpdateLogic] RequestVersion timeout — using buildin")
            finish(true, "timeout_fallback")
        end
    end

    -- 挂载到 onAppTick 里轮询超时
    local _prev_tick = onAppTick
    function onAppTick(dt)
        if _prev_tick then _prev_tick(dt) end
        timeout_poll()
    end
end

-- 没有配置 CDN 地址时跳过网络检查
local cdnUrl = CS.CutRope.Framework.YooAssetInitializer and "" or ""
-- 从 C# Inspector 拿 cdnBaseUrl（通过单例）
local initializer = CS.UnityEngine.Object.FindObjectOfType(
    typeof(CS.CutRope.Framework.YooAssetInitializer))
if initializer then
    cdnUrl = initializer.cdnBaseUrl or ""
end

if cdnUrl == "" or cdnUrl == "https://cdn.example.com/res" then
    -- 未配置 CDN，跳过网络检查
    print("[UpdateLogic] No CDN configured — skip update check")
    finish(true, "no_cdn")
else
    check_remote_version()
end
