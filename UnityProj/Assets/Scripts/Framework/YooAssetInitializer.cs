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
using UnityEngine.Networking;
using Cysharp.Threading.Tasks;
using YooAsset;

namespace CutRope.Framework
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
            // ── 真机：HostPlayMode（BuildinFS + CacheFS，支持弱联网热更）────
            // Phase1 只做「零网络」内置包激活，UpdateLogic.lua 再做联网检查 + 下载
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

            // 用内置版本先激活，让游戏能跑起来；UpdateLogic.lua 会尝试换到更新版本
            // 直接读 StreamingAssets 内置版本文件（零网络，不走 MainFileSystem API）
            string buildinVersion = await ReadBuildinPackageVersion(packageName);
            if (string.IsNullOrEmpty(buildinVersion))
            {
                Debug.LogError($"[YooAssetNew] Phase1: cannot read buildin version for '{packageName}'");
                return false;
            }
            Debug.Log($"[YooAssetNew] Phase1: '{packageName}' buildinVersion = {buildinVersion}");

            if (!await Op(hostPkg.UpdatePackageManifestAsync(buildinVersion),
                          "Phase1 buildin manifest")) return false;

            Debug.Log($"[YooAssetNew] '{packageName}' local ready (HostMode, buildin={buildinVersion})");
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

        /// <summary>
        /// 直接读 StreamingAssets 内置版本文件，零网络，无需 YooAsset API。
        /// 路径：{StreamingAssets}/yoo/{packageName}/{packageName}.version
        /// </summary>
        private static async UniTask<string> ReadBuildinPackageVersion(string packageName)
        {
            // YooAsset 默认 folder = "yoo"，版本文件名 = "{packageName}.version"
            string yooFolder    = YooAsset.YooAssetSettingsData.GetDefaultYooFolderName();
            string versionFile  = $"{packageName}.version";
            string streamingDir = string.IsNullOrEmpty(yooFolder)
                ? Application.streamingAssetsPath
                : System.IO.Path.Combine(Application.streamingAssetsPath, yooFolder);
            string fullPath = System.IO.Path.Combine(streamingDir, packageName, versionFile);

#if UNITY_ANDROID && !UNITY_EDITOR
            // Android 的 StreamingAssets 在 APK 内，需要 UnityWebRequest 读取
            using var req = UnityEngine.Networking.UnityWebRequest.Get(fullPath);
            var op = req.SendWebRequest();
            while (!op.isDone) await UniTask.Yield();
            if (req.result != UnityEngine.Networking.UnityWebRequest.Result.Success)
            {
                Debug.LogError($"[YooAssetNew] ReadBuildinVersion failed: {req.error} path={fullPath}");
                return null;
            }
            return req.downloadHandler.text.Trim();
#else
            if (!System.IO.File.Exists(fullPath))
            {
                Debug.LogError($"[YooAssetNew] ReadBuildinVersion: file not found: {fullPath}");
                return null;
            }
            await UniTask.SwitchToThreadPool();
            var text = System.IO.File.ReadAllText(fullPath).Trim();
            await UniTask.SwitchToMainThread();
            return text;
#endif
        }
    }
}
