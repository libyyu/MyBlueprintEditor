// YooAssetInitializerNew.cs
// 精简版资源系统启动器，对外只暴露两个 async 方法：
//
//   Phase 1 — LaunchInitUpdateStage()：纯本地初始化，零网络依赖
//             · Editor    → EditorSimulate
//             · WebGL     → WebPlay（远端，Phase 1 即拉远端 Manifest）
//             · 真机      → OfflinePlayMode（仅 BuildinFileSystem）
//
//   Phase 2 — LaunchUpdateStage()：弱联网检查更新 + 下载差量
//             · Editor    → no-op，直接 true
//             · WebGL     → no-op（Phase 1 已走远端，无差量概念）
//             · 真机      → 切到 HostPlayMode + RequestVersion + 下载差量
//                          失败回退本地 Manifest，不阻塞启动

using System;
using System.IO;
using UnityEngine;
using Cysharp.Threading.Tasks;
using YooAsset;

namespace CutRope.Framework
{
    public class YooAssetInitializerNew : MonoBehaviour
    {
        // ── Inspector 配置 ────────────────────────────────────────────
        [Header("CDN 地址（真机/发布用）")]
        [Tooltip("主 CDN，如 https://cdn.example.com/res")]
        public string cdnBaseUrl     = "https://cdn.example.com/res";
        [Tooltip("备用 CDN，留空则同主 CDN")]
        public string cdnFallbackUrl = "";

        [Header("主包配置")]
        public string defaultPackageName = "DefaultPackage";

        // ── 下载进度（可选订阅）─────────────────────────────────────
        public event Action<float> OnDownloadProgress;

        // ── 公共状态 ─────────────────────────────────────────────────
        public bool IsLocalReady          { get; private set; }
        public bool IsDefaultPackageReady { get; private set; }

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
            IsLocalReady = IsDefaultPackageReady = true;
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
            IsLocalReady = IsDefaultPackageReady = true;
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
        // Phase 2：弱联网检查更新 + 下载差量
        // ════════════════════════════════════════════════════════════
        public async UniTask<bool> LaunchUpdateStage(string packageName = null)
        {
            packageName = packageName ?? defaultPackageName;

#if UNITY_EDITOR || UNITY_WEBGL
            // Editor / WebGL：Phase 1 已经走远端或 SimulateBuild，无差量更新
            Debug.Log($"[YooAssetNew] Phase2 skipped (Editor/WebGL): '{packageName}'");
            IsDefaultPackageReady = true;
            await UniTask.Yield();
            return true;
#else
            var package = YooAssets.GetPackage(packageName);
            if (package == null)
            {
                Debug.LogError($"[YooAssetNew] Phase2: package '{packageName}' not initialized (call Phase1 first)");
                return false;
            }

            // OfflineMode → HostMode：CacheFS 支持增量下载和缓存
            Debug.Log($"[YooAssetNew] Phase2: '{packageName}' OfflineMode → HostMode");
            YooAssets.RemovePackage(package);

            var hostRemote = new RemoteServices(cdnBaseUrl, cdnFallbackUrl);
            var hostParam  = new HostPlayModeParameters
            {
                BuildinFileSystemParameters =
                    FileSystemParameters.CreateDefaultBuildinFileSystemParameters(),
                CacheFileSystemParameters   =
                    FileSystemParameters.CreateDefaultCacheFileSystemParameters(hostRemote)
            };
            package = YooAssets.CreatePackage(packageName);
            YooAssets.SetDefaultPackage(package);
            if (!await Op(package.InitializeAsync(hostParam), "Phase2 Host init")) return false;

            // 1. 请求远端版本
            var versionOp = package.RequestPackageVersionAsync();
            await versionOp;

            if (versionOp.Status != EOperationStatus.Succeed)
            {
                // CDN 不通：读本地内置版本，激活 HostMode Manifest（用于资源路由）
                Debug.LogWarning($"[YooAssetNew] Phase2: CDN 不可达，回退内置版本。Error: {versionOp.Error}");

                string buildinVer = ReadBuildinVersion(packageName);
                if (string.IsNullOrEmpty(buildinVer))
                {
                    Debug.LogError("[YooAssetNew] Phase2: CDN 不可达且无内置版本文件");
                    return false;
                }

                // 失败时 Warning 继续——bundle 加载会路由到 BuildinFS
                var fallbackMfOp = package.UpdatePackageManifestAsync(buildinVer);
                await fallbackMfOp;
                if (fallbackMfOp.Status != EOperationStatus.Succeed)
                    Debug.LogWarning($"[YooAssetNew] Phase2: 内置 Manifest 激活失败，BuildinFS 资源仍可用。Error: {fallbackMfOp.Error}");

                IsDefaultPackageReady = true;
                return true;
            }

            // 2. 更新 Manifest
            string packageVersion = versionOp.PackageVersion;
            await package.UnloadAllAssetsAsync();

            var manifestOp = package.UpdatePackageManifestAsync(packageVersion);
            await manifestOp;
            if (manifestOp.Status != EOperationStatus.Succeed)
            {
                Debug.LogWarning($"[YooAssetNew] Phase2: UpdateManifest 失败，使用本地 Manifest。Error: {manifestOp.Error}");
                IsDefaultPackageReady = true;
                return true;
            }

            // 3. 下载缺失 bundle（弱联网：失败只 Warning）
            await DownloadMissingBundles(package, packageName);

            Debug.Log($"[YooAssetNew] '{packageName}' Phase2 complete");
            IsDefaultPackageReady = true;
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

        private async UniTask DownloadMissingBundles(ResourcePackage package, string packageName)
        {
            var downloader = package.CreateResourceDownloader(
                downloadingMaxNumber: 10, failedTryAgain: 1);  // 弱联网只重试 1 次

            if (downloader.TotalDownloadCount == 0)
            {
                Debug.Log($"[YooAssetNew] '{packageName}': 无需下载");
                return;
            }

            Debug.Log($"[YooAssetNew] '{packageName}': 下载 {downloader.TotalDownloadCount} 个 bundle " +
                      $"({downloader.TotalDownloadBytes / 1024f / 1024f:F1} MB)");

            downloader.DownloadUpdateCallback = data => OnDownloadProgress?.Invoke(data.Progress);
            downloader.BeginDownload();
            await downloader;

            if (downloader.Status != EOperationStatus.Succeed)
                Debug.LogWarning($"[YooAssetNew] 部分资源下载失败，将使用内置/缓存资源。Error: {downloader.Error}");
        }

        // 直接读 StreamingAssets 内置版本号（不依赖网络）
        private static string ReadBuildinVersion(string packageName)
        {
            string path = Path.Combine(Application.streamingAssetsPath,
                                       "yoo", packageName, $"{packageName}.version");
            if (!File.Exists(path))
            {
                Debug.LogWarning($"[YooAssetNew] 内置版本文件不存在: {path}");
                return null;
            }
            try   { return File.ReadAllText(path).Trim(); }
            catch (Exception ex)
            {
                Debug.LogWarning($"[YooAssetNew] 读取内置版本文件失败: {ex.Message}");
                return null;
            }
        }
    }
}
