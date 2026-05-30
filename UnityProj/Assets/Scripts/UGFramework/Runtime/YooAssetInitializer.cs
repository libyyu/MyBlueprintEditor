// YooAssetInitializerNew.cs
// 精简版资源系统启动器，对外只暴露一个 async 方法：
//
//   LaunchInitUpdateStage()：本地资源库文件初始化
//             · Editor    → EditorSimulate
//             · WebGL     → WebPlay（远端，Phase 1 即拉远端 Manifest）
//             · 真机      → OfflinePlayMode（仅 BuildinFileSystem）
//
// 后续的"检查更新 / 下载差量"逻辑由 Lua 侧 UpdateLogic.lua 自行处理。

using System;
using UnityEngine;
using Cysharp.Threading.Tasks;
using YooAsset;

namespace UGFramework.Runtime
{
    public class YooAssetInitializer : MonoBehaviour
    {
        // ── Inspector 配置 ────────────────────────────────────────────
        [Header("CDN 地址（真机/发布用，Lua 侧检查更新会用到）")]
        [Tooltip("主 CDN，如 https://cdn.example.com/res")]
        public string cdnBaseUrl     = "https://cdn.example.com/res";
        [Tooltip("备用 CDN，留空则同主 CDN")]
        public string cdnFallbackUrl = "";

        [Header("主包配置")]
        public string defaultPackageName = "DefaultPackage";

        // ── 公共状态 ─────────────────────────────────────────────────
        public bool IsLocalReady { get; private set; }

        // ── 生命周期 ─────────────────────────────────────────────────
        private void Awake()
        {
            YooAssets.Initialize();
        }

        // ════════════════════════════════════════════════════════════
        // Phase 1：本地初始化（零网络）
        // ════════════════════════════════════════════════════════════
        public async UniTask<bool> LaunchInitUpdateStage(string packageName = null)
        {
            packageName = packageName ?? defaultPackageName;

            if (YooAssets.ContainsPackage(packageName))
                YooAssets.RemovePackage(packageName);

#if UNITY_EDITOR
            // ── Editor：EditorSimulate ───────────────────────────────
            var pkg = YooAssets.CreatePackage(packageName);
            YooAssets.SetDefaultPackage(pkg);

            var buildResult = EditorSimulateModeHelper.SimulateBuild(packageName);
            var editorParam = new EditorSimulateModeParameters
            {
                AutoUnloadBundleWhenUnused = true,
                EditorFileSystemParameters =
                    FileSystemParameters.CreateDefaultEditorFileSystemParameters(buildResult.PackageRootDirectory)
            };
            if (!await Op(pkg.InitializeAsync(editorParam), "Phase1 Editor init")) return false;

            var verOp = pkg.RequestPackageVersionAsync();
            await verOp;
            if (verOp.Status != EOperationStatus.Succeed)
            {
                Debug.LogError($"[YooAsset] Phase1 Editor version failed: {verOp.Error}");
                return false;
            }

            if (!await Op(pkg.UpdatePackageManifestAsync(verOp.PackageVersion),
                          "Phase1 Editor manifest")) return false;

            Debug.Log($"[YooAsset] '{packageName}' ready (Editor Simulate)");
            IsLocalReady = true;
            return true;

#elif UNITY_WEBGL
            // ── WebGL：WebPlay（直接远端）─────────────────────────────
            var pkg = YooAssets.CreatePackage(packageName);
            YooAssets.SetDefaultPackage(pkg);

            var webRemote = new RemoteServices(cdnBaseUrl, cdnFallbackUrl);
            var webParam  = new WebPlayModeParameters
            {
                WebRemoteFileSystemParameters =
                    FileSystemParameters.CreateDefaultWebRemoteFileSystemParameters(webRemote)
            };
            if (!await Op(pkg.InitializeAsync(webParam), "Phase1 WebGL init")) return false;

            var verOp = pkg.RequestPackageVersionAsync();
            await verOp;
            if (verOp.Status != EOperationStatus.Succeed)
            {
                Debug.LogError($"[YooAsset] Phase1 WebGL version failed: {verOp.Error}");
                return false;
            }

            if (!await Op(pkg.UpdatePackageManifestAsync(verOp.PackageVersion),
                          "Phase1 WebGL manifest")) return false;

            Debug.Log($"[YooAsset] '{packageName}' ready (WebGL)");
            IsLocalReady = true;
            return true;

#else
            // ── 真机：HostPlayMode（BuildinFS + CacheFS，支持弱联网热更）────
            // Phase1 使用改造后的 RequestPackageVersionAsync：
            //   → 先尝试 CacheFS 联网拉版本号（有网且 CDN 可达时生效）
            //   → 失败则自动回退 BuildinFS 读内置版本（零网络，必定成功）
            // 这样 Phase1 永远单步就能激活一个有效 Manifest，UpdateLogic.lua 再做增量下载。
            var hostPkg = YooAssets.CreatePackage(packageName);
            YooAssets.SetDefaultPackage(hostPkg);

            var remote    = new RemoteServices(cdnBaseUrl,
                                string.IsNullOrEmpty(cdnFallbackUrl) ? cdnBaseUrl : cdnFallbackUrl);
            var hostParam = new HostPlayModeParameters
            {
                BuildinFileSystemParameters =
                    FileSystemParameters.CreateDefaultBuildinFileSystemParameters(),
                CacheFileSystemParameters =
                    FileSystemParameters.CreateDefaultCacheFileSystemParameters(remote)
            };
            if (!await Op(hostPkg.InitializeAsync(hostParam), "Phase1 Host init")) return false;

            // RequestPackageVersionAsync 已支持 fallback：
            //   有网 → 拉到远端最新版本（Phase1 + UpdateLogic.lua 共用最新版本）
            //   无网 → 自动读内置版本（零网络，必定成功）
            var verOp = hostPkg.RequestPackageVersionAsync(appendTimeTicks: true, timeout: 5);
            await verOp;
            if (verOp.Status != EOperationStatus.Succeed)
            {
                // 正常不应到这里（fallback 也失败，说明内置包完整性问题）
                Debug.LogError($"[YooAsset] Phase1 version failed: {verOp.Error}");
                return false;
            }
            string activeVersion = verOp.PackageVersion;
            Debug.Log($"[YooAsset] Phase1: '{packageName}' activeVersion = {activeVersion}");

            if (!await Op(hostPkg.UpdatePackageManifestAsync(activeVersion),
                          "Phase1 manifest")) return false;

            Debug.Log($"[YooAsset] '{packageName}' local ready (HostMode, version={activeVersion})");
            IsLocalReady = true;
            return true;
#endif
        }

        // ════════════════════════════════════════════════════════════
        // 工具方法
        // ════════════════════════════════════════════════════════════

        // 包装 YooAsset Operation：失败时打 LogError 并返回 false
        private static async UniTask<bool> Op(AsyncOperationBase op, string tag)
        {
            await op;
            if (op.Status != EOperationStatus.Succeed)
            {
                Debug.LogError($"[YooAsset] {tag} failed: {op.Error}");
                return false;
            }
            return true;
        }
    }
}
