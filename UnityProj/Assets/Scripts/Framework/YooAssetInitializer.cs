// YooAssetInitializer.cs
// 负责初始化 YooAsset 主包（DefaultPackage）和 DLC 分包（DlcChapterXX）
//
// 初始化分两阶段，支持"先显示 Loading UI 再检查更新"的流程：
//
//   Phase 1 — InitLocal()  【零网络依赖】
//     原生平台（方案 D）：
//       ① OfflinePlayMode 初始化（只含 BuildinFileSystem）
//       ② RequestPackageVersionAsync → BuildinFS → StreamingAssets 版本文件（纯本地）
//       ③ UpdatePackageManifestAsync → BuildinFS → StreamingAssets manifest（纯本地）
//       Phase1 结束，Package 保持 OfflineMode，资源正常加载（走 BuildinFS）
//     触发事件：OnLocalReady / OnInitFailed
//
//   Phase 2 — CheckAndDownload()  【弱联网】
//     原生平台：
//       ① 将 Package 从 OfflineMode 切换为 HostPlayMode（RemovePackage + 重建）
//       ② RequestPackageVersionAsync → CDN
//          · 成功 → UpdatePackageManifestAsync + 下载增量 bundle
//          · 失败 → 读本地 .version 文件 → UpdatePackageManifestAsync(内置版本)
//                   → CacheFS 无缓存时会去 CDN，若仍不通则 Warning 继续
//                   → 游戏用 BuildinFS 资源正常运行
//     触发事件：OnDefaultPackageReady
//
// 平台分流：
//   UNITY_EDITOR              → EditorSimulate（跳过网络，两阶段均立即完成）
//   UNITY_WEBGL && !UNITY_EDITOR → WebPlay（纯远端，Phase 1 初始化远端 FS）
//   其他（iOS / Android / PC）  → OfflinePlay(Phase1) + HostPlay(Phase2)

using System;
using System.Collections;
using System.IO;
using UnityEngine;
using Cysharp.Threading.Tasks;
using YooAsset;

namespace CutRope.Framework
{
    public class YooAssetInitializer : MonoBehaviour
    {
        // ── Inspector 配置 ────────────────────────────────────────────
        [Header("CDN 地址（真机/发布用）")]
        [Tooltip("主 CDN，如 https://cdn.example.com/res")]
        public string cdnBaseUrl     = "https://cdn.example.com/res";
        [Tooltip("备用 CDN，留空则同主 CDN")]
        public string cdnFallbackUrl = "";

        [Header("主包配置")]
        public string defaultPackageName = "DefaultPackage";

        // ── 事件 ─────────────────────────────────────────────────────
        /// <summary>Phase 1 完成：内置资源可访问（零网络依赖）</summary>
        public event Action OnLocalReady;
        /// <summary>Phase 2 完成：主包就绪（CDN 可达则含最新资源，否则用内置）</summary>
        public event Action OnDefaultPackageReady;
        /// <summary>初始化失败（仅在真正不可恢复时触发）</summary>
        public event Action<string> OnInitFailed;
        /// <summary>Phase 2 下载进度 0~1</summary>
        public event Action<float> OnDownloadProgress;

        // ── 公共状态 ─────────────────────────────────────────────────
        public bool IsLocalReady          { get; private set; }
        public bool IsDefaultPackageReady { get; private set; }

        // ── 生命周期 ─────────────────────────────────────────────────
        private void Awake()
        {
            YooAssets.Initialize();
        }

        // ── 公共 API ─────────────────────────────────────────────────

        /// <summary>Phase 1：纯本地初始化，零网络依赖。</summary>
        public void InitLocal()
        {
            StartCoroutine(InitLocalCoroutine(defaultPackageName));
        }

        /// <summary>Phase 2：弱联网检查更新。必须在 OnLocalReady 后调用。</summary>
        public void CheckAndDownload()
        {
            var package = YooAssets.GetPackage(defaultPackageName);
            if (package == null)
            {
                OnInitFailed?.Invoke("[YooAsset] CheckAndDownload: DefaultPackage not initialized");
                return;
            }
            StartCoroutine(CheckAndDownloadCoroutine(package, defaultPackageName, isDefault: true));
        }

        /// <summary>按需初始化 DLC 分包，内部完成 Phase 1 + Phase 2。</summary>
        public void InitDlcPackage(string packageName, Action onReady, Action<string> onFailed = null)
        {
            StartCoroutine(InitDlcCoroutine(packageName, onReady, onFailed));
        }

        // ════════════════════════════════════════════════════════════
        // Phase 1：本地初始化（零网络）
        // ════════════════════════════════════════════════════════════
        private IEnumerator InitLocalCoroutine(string packageName)
        {
            Debug.Log($"[YooAsset] Phase1 begin: '{packageName}'");

            // 先清理可能残留的旧 Package（重试场景）
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
            var initOp = pkg.InitializeAsync(editorParam);
            yield return initOp;
            if (initOp.Status != EOperationStatus.Succeed)
            {
                string e = $"[YooAsset] Phase1 Editor init failed: {initOp.Error}";
                Debug.LogError(e); OnInitFailed?.Invoke(e); yield break;
            }

            var verOp = pkg.RequestPackageVersionAsync();
            yield return verOp;
            if (verOp.Status != EOperationStatus.Succeed)
            {
                string e = $"[YooAsset] Phase1 Editor version failed: {verOp.Error}";
                Debug.LogError(e); OnInitFailed?.Invoke(e); yield break;
            }

            var mfOp = pkg.UpdatePackageManifestAsync(verOp.PackageVersion);
            yield return mfOp;
            if (mfOp.Status != EOperationStatus.Succeed)
            {
                string e = $"[YooAsset] Phase1 Editor manifest failed: {mfOp.Error}";
                Debug.LogError(e); OnInitFailed?.Invoke(e); yield break;
            }

            Debug.Log($"[YooAsset] '{packageName}' ready (Editor Simulate)");
            IsLocalReady = IsDefaultPackageReady = true;
            OnLocalReady?.Invoke();
            OnDefaultPackageReady?.Invoke();

#elif UNITY_WEBGL
            // ── WebGL：WebPlay ───────────────────────────────────────
            var pkg = YooAssets.CreatePackage(packageName);
            YooAssets.SetDefaultPackage(pkg);

            var webRemote = new RemoteServices(cdnBaseUrl, cdnFallbackUrl);
            var webParam  = new WebPlayModeParameters
            {
                WebFileSystemParameters =
                    FileSystemParameters.CreateDefaultWebFileSystemParameters(webRemote)
            };
            var initOp = pkg.InitializeAsync(webParam);
            yield return initOp;
            if (initOp.Status != EOperationStatus.Succeed)
            {
                string e = $"[YooAsset] Phase1 WebGL init failed: {initOp.Error}";
                Debug.LogError(e); OnInitFailed?.Invoke(e); yield break;
            }

            var verOp = pkg.RequestPackageVersionAsync();
            yield return verOp;
            if (verOp.Status != EOperationStatus.Succeed)
            {
                string e = $"[YooAsset] Phase1 WebGL version failed: {verOp.Error}";
                Debug.LogError(e); OnInitFailed?.Invoke(e); yield break;
            }

            var mfOp = pkg.UpdatePackageManifestAsync(verOp.PackageVersion);
            yield return mfOp;
            if (mfOp.Status != EOperationStatus.Succeed)
            {
                string e = $"[YooAsset] Phase1 WebGL manifest failed: {mfOp.Error}";
                Debug.LogError(e); OnInitFailed?.Invoke(e); yield break;
            }

            Debug.Log($"[YooAsset] '{packageName}' local ready (WebGL)");
            IsLocalReady = true;
            OnLocalReady?.Invoke();

#else
            // ── 原生平台：OfflinePlayMode（只含 BuildinFileSystem，零网络）────
            // UpdatePackageManifestAsync 在 OfflineMode 下走 BuildinFS，
            // BuildinFS 只读 StreamingAssets，完全不访问 CDN。
            var offlinePkg = YooAssets.CreatePackage(packageName);
            YooAssets.SetDefaultPackage(offlinePkg);

            var offlineParam = new OfflinePlayModeParameters
            {
                BuildinFileSystemParameters =
                    FileSystemParameters.CreateDefaultBuildinFileSystemParameters()
            };
            var offlineInitOp = offlinePkg.InitializeAsync(offlineParam);
            yield return offlineInitOp;
            if (offlineInitOp.Status != EOperationStatus.Succeed)
            {
                string e = $"[YooAsset] Phase1 Offline init failed: {offlineInitOp.Error}";
                Debug.LogError(e); OnInitFailed?.Invoke(e); yield break;
            }

            // RequestPackageVersionAsync → BuildinFS → StreamingAssets（纯本地）
            var buildinVerOp = offlinePkg.RequestPackageVersionAsync();
            yield return buildinVerOp;
            if (buildinVerOp.Status != EOperationStatus.Succeed)
            {
                string e = $"[YooAsset] Phase1 buildin version failed: {buildinVerOp.Error}";
                Debug.LogError(e); OnInitFailed?.Invoke(e); yield break;
            }
            string buildinVersion = buildinVerOp.PackageVersion;
            Debug.Log($"[YooAsset] Phase1: '{packageName}' buildinVersion = {buildinVersion}");

            // UpdatePackageManifestAsync → BuildinFS → StreamingAssets（纯本地）
            var buildinMfOp = offlinePkg.UpdatePackageManifestAsync(buildinVersion);
            yield return buildinMfOp;
            if (buildinMfOp.Status != EOperationStatus.Succeed)
            {
                string e = $"[YooAsset] Phase1 buildin manifest failed: {buildinMfOp.Error}";
                Debug.LogError(e); OnInitFailed?.Invoke(e); yield break;
            }

            // Phase1 完成：Package 保持 OfflineMode，ActiveManifest 已激活，资源可正常加载。
            // Phase2 会负责切换到 HostMode 并处理弱联网更新。
            Debug.Log($"[YooAsset] '{packageName}' local ready (OfflineMode, version={buildinVersion})");
            IsLocalReady = true;
            OnLocalReady?.Invoke();
#endif
        }

        // ════════════════════════════════════════════════════════════
        // Phase 2：弱联网检查更新 + 下载
        // ════════════════════════════════════════════════════════════
        private IEnumerator CheckAndDownloadCoroutine(
            ResourcePackage package,
            string packageName,
            bool   isDefault,
            Action onReady          = null,
            Action<string> onFailed = null)
        {
#if !UNITY_WEBGL && !UNITY_EDITOR
            // 原生平台：Phase1 用的是 OfflineMode，先切换到 HostMode
            // HostMode 含 CacheFileSystem，支持增量下载和缓存
            Debug.Log($"[YooAsset] Phase2: '{packageName}' OfflineMode → HostMode");
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
            if (isDefault) YooAssets.SetDefaultPackage(package);

            var hostInitOp = package.InitializeAsync(hostParam);
            yield return hostInitOp;
            if (hostInitOp.Status != EOperationStatus.Succeed)
            {
                string e = $"[YooAsset] Phase2 Host init failed: {hostInitOp.Error}";
                Debug.LogError(e);
                if (isDefault) OnInitFailed?.Invoke(e);
                onFailed?.Invoke(e);
                yield break;
            }
#endif

            bool networkAvailable = true;

            // 1. 请求远端版本
            var versionOp = package.RequestPackageVersionAsync();
            yield return versionOp;

            if (versionOp.Status != EOperationStatus.Succeed)
            {
#if UNITY_WEBGL
                string verErr = $"[YooAsset] Phase2 RequestVersion failed: {versionOp.Error}";
                Debug.LogError(verErr);
                if (isDefault) OnInitFailed?.Invoke(verErr);
                onFailed?.Invoke(verErr);
                yield break;
#else
                // CDN 不通：读本地内置版本，激活 HostMode 的 Manifest（用于资源路由）
                networkAvailable = false;
                Debug.LogWarning($"[YooAsset] Phase2: CDN 不可达，尝试内置版本。Error: {versionOp.Error}");

                string buildinVer = ReadBuildinVersion(packageName);
                if (string.IsNullOrEmpty(buildinVer))
                {
                    string e = $"[YooAsset] Phase2: CDN 不可达且无内置版本文件，无法启动";
                    Debug.LogError(e);
                    if (isDefault) OnInitFailed?.Invoke(e);
                    onFailed?.Invoke(e);
                    yield break;
                }

                // 尝试激活 HostMode Manifest（CacheFS 无缓存时去 CDN，CDN 不通则失败）
                // 失败时 Warning 继续——bundle 加载会路由到 BuildinFS，资源仍可用
                var fallbackMfOp = package.UpdatePackageManifestAsync(buildinVer);
                yield return fallbackMfOp;
                if (fallbackMfOp.Status != EOperationStatus.Succeed)
                    Debug.LogWarning($"[YooAsset] Phase2: 内置 Manifest 激活失败（CDN 不通且无缓存），" +
                                     $"资源加载将尝试走 BuildinFS。Error: {fallbackMfOp.Error}");

                goto PackageReady;
#endif
            }

            // 2. 更新 Manifest（有新版本）
            string packageVersion = versionOp.PackageVersion;

            var unloadOp = package.UnloadAllAssetsAsync();
            yield return unloadOp;

            var manifestOp = package.UpdatePackageManifestAsync(packageVersion);
            yield return manifestOp;
            if (manifestOp.Status != EOperationStatus.Succeed)
            {
                networkAvailable = false;
                Debug.LogWarning($"[YooAsset] Phase2: UpdateManifest 失败，使用本地 Manifest。Error: {manifestOp.Error}");
            }

            // 3. 下载缺失 bundle（弱联网：失败只 Warning）
            if (networkAvailable)
                yield return DownloadMissingBundles(package, packageName);
            else
                Debug.LogWarning($"[YooAsset] Phase2: 跳过下载，使用内置/缓存资源");

            PackageReady:
            Debug.Log($"[YooAsset] '{packageName}' Phase2 complete (network={networkAvailable})");
            if (isDefault)
            {
                IsDefaultPackageReady = true;
                OnDefaultPackageReady?.Invoke();
            }
            onReady?.Invoke();
        }

        // ════════════════════════════════════════════════════════════
        // DLC 包初始化（Phase1 + Phase2 合并）
        // ════════════════════════════════════════════════════════════
        private IEnumerator InitDlcCoroutine(
            string packageName,
            Action onReady,
            Action<string> onFailed)
        {
            if (YooAssets.ContainsPackage(packageName))
                YooAssets.RemovePackage(packageName);

#if UNITY_EDITOR
            var pkg = YooAssets.CreatePackage(packageName);
            var buildResult = EditorSimulateModeHelper.SimulateBuild(packageName);
            var editorParam = new EditorSimulateModeParameters
            {
                EditorFileSystemParameters =
                    FileSystemParameters.CreateDefaultEditorFileSystemParameters(buildResult.PackageRootDirectory)
            };
            var initOp = pkg.InitializeAsync(editorParam);
            yield return initOp;
            if (initOp.Status != EOperationStatus.Succeed)
            {
                string e = $"[YooAsset] DLC '{packageName}' Editor init failed: {initOp.Error}";
                Debug.LogError(e); onFailed?.Invoke(e); yield break;
            }
            Debug.Log($"[YooAsset] DLC '{packageName}' ready (Editor)");
            onReady?.Invoke();

#elif UNITY_WEBGL
            var pkg = YooAssets.CreatePackage(packageName);
            var webRemote = new RemoteServices(cdnBaseUrl, cdnFallbackUrl);
            var webParam  = new WebPlayModeParameters
            {
                WebFileSystemParameters =
                    FileSystemParameters.CreateDefaultWebFileSystemParameters(webRemote)
            };
            var initOp = pkg.InitializeAsync(webParam);
            yield return initOp;
            if (initOp.Status != EOperationStatus.Succeed)
            {
                string e = $"[YooAsset] DLC '{packageName}' WebGL init failed: {initOp.Error}";
                Debug.LogError(e); onFailed?.Invoke(e); yield break;
            }
            yield return CheckAndDownloadCoroutine(pkg, packageName,
                isDefault: false, onReady: onReady, onFailed: onFailed);

#else
            // 原生平台：同主包，先 OfflineMode 读内置，再由 CheckAndDownload 切 HostMode
            var offlinePkg = YooAssets.CreatePackage(packageName);
            var offlineParam = new OfflinePlayModeParameters
            {
                BuildinFileSystemParameters =
                    FileSystemParameters.CreateDefaultBuildinFileSystemParameters()
            };
            var offlineInitOp = offlinePkg.InitializeAsync(offlineParam);
            yield return offlineInitOp;
            if (offlineInitOp.Status != EOperationStatus.Succeed)
            {
                string e = $"[YooAsset] DLC '{packageName}' Offline init failed: {offlineInitOp.Error}";
                Debug.LogError(e); onFailed?.Invoke(e); yield break;
            }

            var verOp = offlinePkg.RequestPackageVersionAsync();
            yield return verOp;
            if (verOp.Status != EOperationStatus.Succeed)
            {
                string e = $"[YooAsset] DLC '{packageName}' buildin version failed: {verOp.Error}";
                Debug.LogError(e); onFailed?.Invoke(e); yield break;
            }

            var mfOp = offlinePkg.UpdatePackageManifestAsync(verOp.PackageVersion);
            yield return mfOp;
            if (mfOp.Status != EOperationStatus.Succeed)
            {
                string e = $"[YooAsset] DLC '{packageName}' buildin manifest failed: {mfOp.Error}";
                Debug.LogError(e); onFailed?.Invoke(e); yield break;
            }

            // 切 HostMode + Phase2 弱联网更新
            yield return CheckAndDownloadCoroutine(offlinePkg, packageName,
                isDefault: false, onReady: onReady, onFailed: onFailed);
#endif
        }

        // ════════════════════════════════════════════════════════════
        // 下载缺失 bundle（弱联网：失败只 Warning，不中断游戏）
        // ════════════════════════════════════════════════════════════
        private IEnumerator DownloadMissingBundles(ResourcePackage package, string packageName)
        {
            var downloader = package.CreateResourceDownloader(
                downloadingMaxNumber: 10, failedTryAgain: 1);  // 弱联网只重试1次

            if (downloader.TotalDownloadCount == 0)
            {
                Debug.Log($"[YooAsset] '{packageName}': 无需下载");
                yield break;
            }

            Debug.Log($"[YooAsset] '{packageName}': 下载 {downloader.TotalDownloadCount} 个 bundle " +
                      $"({downloader.TotalDownloadBytes / 1024f / 1024f:F1} MB)");

            downloader.DownloadUpdateCallback = data => OnDownloadProgress?.Invoke(data.Progress);
            downloader.BeginDownload();
            yield return downloader;

            if (downloader.Status != EOperationStatus.Succeed)
                Debug.LogWarning($"[YooAsset] 部分资源下载失败，将使用内置/缓存资源。Error: {downloader.Error}");
        }

        // ════════════════════════════════════════════════════════════
        // 工具：直接读 StreamingAssets 内置版本号（不依赖网络）
        // ════════════════════════════════════════════════════════════
        private string ReadBuildinVersion(string packageName)
        {
            // 路径：StreamingAssets/yoo/{packageName}/{packageName}.version
            // 对应项目中 YooAssetSettings.DefaultYooFolderName = "yoo"
            string path = Path.Combine(Application.streamingAssetsPath,
                                       "yoo", packageName, $"{packageName}.version");
            if (!File.Exists(path))
            {
                Debug.LogWarning($"[YooAsset] 内置版本文件不存在: {path}");
                return null;
            }
            try   { return File.ReadAllText(path).Trim(); }
            catch (Exception ex)
            {
                Debug.LogWarning($"[YooAsset] 读取内置版本文件失败: {ex.Message}");
                return null;
            }
        }

        public async UniTask<bool> LaunchInitUpdateStage(string packageName = "DefaultPackage")
        {
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
            var initOp = pkg.InitializeAsync(editorParam);
            await initOp;
            if (initOp.Status != EOperationStatus.Succeed)
            {
                string e = $"[YooAsset] Phase1 Editor init failed: {initOp.Error}";
                Debug.LogError(e); return false;
            }

            var verOp = pkg.RequestPackageVersionAsync();
            await verOp;
            if (verOp.Status != EOperationStatus.Succeed)
            {
                string e = $"[YooAsset] Phase1 Editor version failed: {verOp.Error}";
                Debug.LogError(e); return false;
            }

            var mfOp = pkg.UpdatePackageManifestAsync(verOp.PackageVersion);
            await mfOp;
            if (mfOp.Status != EOperationStatus.Succeed)
            {
                string e = $"[YooAsset] Phase1 Editor manifest failed: {mfOp.Error}";
                Debug.LogError(e); return false;
            }

            Debug.Log($"[YooAsset] '{packageName}' ready (Editor Simulate)");
            return true;
#elif UNITY_WEBGL
            // ── WebGL：WebPlay ───────────────────────────────────────
            var pkg = YooAssets.CreatePackage(packageName);
            YooAssets.SetDefaultPackage(pkg);

            var webRemote = new RemoteServices(cdnBaseUrl, cdnFallbackUrl);
            var webParam = new WebPlayModeParameters
            {
                WebRemoteFileSystemParameters =
                    FileSystemParameters.CreateDefaultWebRemoteFileSystemParameters(webRemote)
            };
            var initOp = pkg.InitializeAsync(webParam);
            await initOp;
            if (initOp.Status != EOperationStatus.Succeed)
            {
                string e = $"[YooAsset] Phase1 WebGL init failed: {initOp.Error}";
                Debug.LogError(e); return false;
            }

            var verOp = pkg.RequestPackageVersionAsync();
            await verOp;
            if (verOp.Status != EOperationStatus.Succeed)
            {
                string e = $"[YooAsset] Phase1 WebGL version failed: {verOp.Error}";
                Debug.LogError(e); return false;
            }

            var mfOp = pkg.UpdatePackageManifestAsync(verOp.PackageVersion);
            await mfOp;
            if (mfOp.Status != EOperationStatus.Succeed)
            {
                string e = $"[YooAsset] Phase1 WebGL manifest failed: {mfOp.Error}";
                Debug.LogError(e); return false;
            }

            Debug.Log($"[YooAsset] '{packageName}' local ready (WebGL)");
            return true;
#else
            var offlinePkg = YooAssets.CreatePackage(packageName);
            YooAssets.SetDefaultPackage(offlinePkg);

            var offlineParam = new OfflinePlayModeParameters
            {
                BuildinFileSystemParameters =
                    FileSystemParameters.CreateDefaultBuildinFileSystemParameters()
            };
            var offlineInitOp = offlinePkg.InitializeAsync(offlineParam);
            await offlineInitOp;
            if (offlineInitOp.Status != EOperationStatus.Succeed)
            {
                string e = $"[YooAsset] Phase1 Offline init failed: {offlineInitOp.Error}";
                Debug.LogError(e); return false;
            }

            // RequestPackageVersionAsync → BuildinFS → StreamingAssets（纯本地）
            var buildinVerOp = offlinePkg.RequestPackageVersionAsync();
            await buildinVerOp;
            if (buildinVerOp.Status != EOperationStatus.Succeed)
            {
                string e = $"[YooAsset] Phase1 buildin version failed: {buildinVerOp.Error}";
                Debug.LogError(e); return false;
            }
            string buildinVersion = buildinVerOp.PackageVersion;
            Debug.Log($"[YooAsset] Phase1: '{packageName}' buildinVersion = {buildinVersion}");

            // UpdatePackageManifestAsync → BuildinFS → StreamingAssets（纯本地）
            var buildinMfOp = offlinePkg.UpdatePackageManifestAsync(buildinVersion);
            await buildinMfOp;
            if (buildinMfOp.Status != EOperationStatus.Succeed)
            {
                string e = $"[YooAsset] Phase1 buildin manifest failed: {buildinMfOp.Error}";
                Debug.LogError(e); return false;
            }

            Debug.Log($"[YooAsset] '{packageName}' local ready (OfflineMode, version={buildinVersion})");
            return true;
#endif

        }
    }
}
