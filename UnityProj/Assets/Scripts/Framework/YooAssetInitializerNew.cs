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

namespace CutRope.Framework
{
    public class YooAssetInitializerNew : MonoBehaviour
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
                Debug.LogError($"[YooAssetNew] Phase1 Editor version failed: {verOp.Error}");
                return false;
            }

            if (!await Op(pkg.UpdatePackageManifestAsync(verOp.PackageVersion),
                          "Phase1 Editor manifest")) return false;

            Debug.Log($"[YooAssetNew] '{packageName}' ready (Editor Simulate)");
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
                Debug.LogError($"[YooAssetNew] Phase1 WebGL version failed: {verOp.Error}");
                return false;
            }

            if (!await Op(pkg.UpdatePackageManifestAsync(verOp.PackageVersion),
                          "Phase1 WebGL manifest")) return false;

            Debug.Log($"[YooAssetNew] '{packageName}' ready (WebGL)");
            IsLocalReady = true;
            return true;

#else
            // ── 真机：OfflinePlayMode（只读 BuildinFS，零网络）────────
            var offlinePkg = YooAssets.CreatePackage(packageName);
            YooAssets.SetDefaultPackage(offlinePkg);

            var offlineParam = new OfflinePlayModeParameters
            {
                BuildinFileSystemParameters =
                    FileSystemParameters.CreateDefaultBuildinFileSystemParameters()
            };
            if (!await Op(offlinePkg.InitializeAsync(offlineParam), "Phase1 Offline init")) return false;

            var buildinVerOp = offlinePkg.RequestPackageVersionAsync();
            await buildinVerOp;
            if (buildinVerOp.Status != EOperationStatus.Succeed)
            {
                Debug.LogError($"[YooAssetNew] Phase1 buildin version failed: {buildinVerOp.Error}");
                return false;
            }
            string buildinVersion = buildinVerOp.PackageVersion;
            Debug.Log($"[YooAssetNew] Phase1: '{packageName}' buildinVersion = {buildinVersion}");

            if (!await Op(offlinePkg.UpdatePackageManifestAsync(buildinVersion),
                          "Phase1 buildin manifest")) return false;

            Debug.Log($"[YooAssetNew] '{packageName}' local ready (OfflineMode, version={buildinVersion})");
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
                Debug.LogError($"[YooAssetNew] {tag} failed: {op.Error}");
                return false;
            }
            return true;
        }
    }
}
