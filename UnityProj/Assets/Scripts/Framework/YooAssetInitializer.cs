// YooAssetInitializer.cs
// 负责初始化 YooAsset 主包（DefaultPackage）和 DLC 分包（DlcChapterXX）
//
// 初始化分两阶段，支持"先显示 Loading UI 再检查更新"的流程：
//
//   Phase 1 — InitLocalAsync()
//     初始化本地文件系统（内置包 or 缓存），让资源立即可加载。
//     此阶段不访问网络，完成后可加载 LoadingUI prefab 等本地资源。
//     触发事件：OnLocalReady / OnInitFailed
//
//   Phase 2 — CheckAndDownloadAsync()
//     请求远端版本号 → 更新 Manifest → 下载缺失 bundle。
//     完成后触发：OnDefaultPackageReady（主包）或 onReady 回调（DLC 包）
//     下载进度：OnDownloadProgress (0~1)
//
// 平台分流：
//   UNITY_EDITOR              → EditorSimulate（跳过网络，两阶段均立即完成）
//   UNITY_WEBGL && !UNITY_EDITOR → WebPlay（纯远端，Phase 1 仅初始化远端FS，
//                                            Phase 2 拉版本+下载）
//   其他（iOS / Android / PC）  → HostPlay（Phase 1 读内置/缓存，Phase 2 增量更新）
//
// DLC 包（DlcChapterXX）：按需调用 InitDlcPackage，内部同样走两阶段。
// 主包（DefaultPackage）：游戏启动时必须完成，包含 Loading UI、主菜单、第1章前2关

using System;
using System.Collections;
using UnityEngine;
using YooAsset;

namespace CutRope.Framework
{
    public class YooAssetInitializer : MonoBehaviour
    {
        // ── Inspector 配置 ────────────────────────────────────────────
        [Header("CDN 地址（真机/发布用）")]
        [Tooltip("主 CDN，如 https://cdn.example.com/res")]
        public string cdnBaseUrl    = "https://cdn.example.com/res";
        [Tooltip("备用 CDN，留空则同主 CDN")]
        public string cdnFallbackUrl = "";

        [Header("主包配置")]
        public string defaultPackageName = "DefaultPackage";

        // ── 事件 ─────────────────────────────────────────────────────
        /// <summary>Phase 1 完成：本地资源可访问（内置包或缓存已就绪）</summary>
        public event Action OnLocalReady;
        /// <summary>Phase 2 完成：主包所有资源下载完毕</summary>
        public event Action OnDefaultPackageReady;
        /// <summary>初始化失败（Phase 1 或 Phase 2 均可触发）</summary>
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

        /// <summary>
        /// Phase 1：初始化本地文件系统。
        /// 完成后触发 OnLocalReady，此时可加载内置/缓存资源（如 LoadingUI prefab）。
        /// 由 GameLauncher 在 Start 中调用。
        /// </summary>
        public void InitLocal()
        {
            StartCoroutine(InitLocalCoroutine(defaultPackageName));
        }

        /// <summary>
        /// Phase 2：检查远端版本并下载缺失资源。
        /// 必须在 InitLocal 完成（OnLocalReady 触发）之后调用。
        /// 完成后触发 OnDefaultPackageReady。
        /// </summary>
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

        /// <summary>
        /// 按需初始化 DLC 分包（章节资源包），内部完成 Phase 1 + Phase 2。
        /// 例：InitDlcPackage("DlcChapter1", onReady, onFailed)
        /// </summary>
        public void InitDlcPackage(string packageName, Action onReady, Action<string> onFailed = null)
        {
            StartCoroutine(InitDlcCoroutine(packageName, onReady, onFailed));
        }

        // ── Phase 1：本地初始化协程 ───────────────────────────────────
        private IEnumerator InitLocalCoroutine(string packageName)
        {
            // 1. 获取或创建 Package
            ResourcePackage package;
            if (YooAssets.ContainsPackage(packageName))
                package = YooAssets.GetPackage(packageName);
            else
                package = YooAssets.CreatePackage(packageName);

            YooAssets.SetDefaultPackage(package);

            // 2. 初始化参数
#if UNITY_EDITOR
            var buildResult = EditorSimulateModeHelper.SimulateBuild(packageName);
            var editorParam = new EditorSimulateModeParameters
            {
                AutoUnloadBundleWhenUnused = true,
                EditorFileSystemParameters =
                    FileSystemParameters.CreateDefaultEditorFileSystemParameters(buildResult.PackageRootDirectory)
            };
            var initOp = package.InitializeAsync(editorParam);
#elif UNITY_WEBGL
            // WebGL：无本地文件系统，Phase 1 直接初始化远端 FS
            // 首次启动没有本地缓存，资源全部来自 CDN
            var webRemote = new RemoteServices(cdnBaseUrl, cdnFallbackUrl);
            var webParam  = new WebPlayModeParameters
            {
                WebFileSystemParameters =
                    FileSystemParameters.CreateDefaultWebFileSystemParameters(webRemote)
            };
            var initOp = package.InitializeAsync(webParam);
#else
            // 原生平台：内置包 + 缓存，不访问网络
            var hostRemote = new RemoteServices(cdnBaseUrl, cdnFallbackUrl);
            var hostParam  = new HostPlayModeParameters
            {
                BuildinFileSystemParameters =
                    FileSystemParameters.CreateDefaultBuildinFileSystemParameters(),
                CacheFileSystemParameters   =
                    FileSystemParameters.CreateDefaultCacheFileSystemParameters(hostRemote)
            };
            var initOp = package.InitializeAsync(hostParam);
#endif

            yield return initOp;

            if (initOp.Status != EOperationStatus.Succeed)
            {
                string err = $"[YooAsset] Phase1 init failed: {initOp.Error}";
                Debug.LogError(err);
                OnInitFailed?.Invoke(err);
                yield break;
            }

            // 激活本地 Manifest（InitializeAsync 只初始化文件系统，不激活 Manifest）
            // 无论何种平台，必须先 UpdatePackageManifestAsync 才能加载任何资源
            var localVersion = package.GetPackageVersion();
            var localManifestOp = package.UpdatePackageManifestAsync(localVersion);
            yield return localManifestOp;

            if (localManifestOp.Status != EOperationStatus.Succeed)
            {
                string err = $"[YooAsset] Phase1 activate manifest failed: {localManifestOp.Error}";
                Debug.LogError(err);
                OnInitFailed?.Invoke(err);
                yield break;
            }

#if UNITY_EDITOR
            // Editor：跳过网络，Phase 1 = Phase 2，直接全部就绪
            Debug.Log($"[YooAsset] '{packageName}' ready (Editor Simulate)");
            IsLocalReady          = true;
            IsDefaultPackageReady = true;
            OnLocalReady?.Invoke();
            OnDefaultPackageReady?.Invoke();
#else
            Debug.Log($"[YooAsset] '{packageName}' local ready");
            IsLocalReady = true;
            OnLocalReady?.Invoke();
            // Phase 2 由 GameLauncher 在加载完 LoadingUI 后显式调用 CheckAndDownload()
            // Phase 2 的 UpdatePackageManifestAsync 会用远端版本覆盖此处的本地版本
#endif
        }

        // ── Phase 2：检查更新 + 下载协程 ─────────────────────────────
        private IEnumerator CheckAndDownloadCoroutine(
            ResourcePackage package,
            string packageName,
            bool   isDefault,
            Action onReady   = null,
            Action<string> onFailed = null)
        {
            // 3. 请求远端版本
            var versionOp = package.RequestPackageVersionAsync();
            yield return versionOp;

            if (versionOp.Status != EOperationStatus.Succeed)
            {
#if UNITY_WEBGL
                // WebGL 无缓存回退，直接报错
                string verErr = $"[YooAsset] RequestVersion failed: {versionOp.Error}";
                Debug.LogError(verErr);
                onFailed?.Invoke(verErr);
                if (isDefault) OnInitFailed?.Invoke(verErr);
                yield break;
#else
                // 原生平台：离线容错，用本地版本继续
                Debug.LogWarning($"[YooAsset] RequestVersion failed, using cached version. Error: {versionOp.Error}");
#endif
            }

            // 4. 更新 Manifest
            string packageVersion = versionOp.Status == EOperationStatus.Succeed
                ? versionOp.PackageVersion
                : package.GetPackageVersion();

            var manifestOp = package.UpdatePackageManifestAsync(packageVersion);
            yield return manifestOp;

            if (manifestOp.Status != EOperationStatus.Succeed)
            {
                // Manifest 更新失败：继续用旧版本，不中断游戏
                Debug.LogWarning($"[YooAsset] UpdateManifest failed: {manifestOp.Error}");
            }

            // 5. 下载缺失 bundle
            yield return DownloadMissingBundles(package, packageName);

            Debug.Log($"[YooAsset] '{packageName}' download complete");
            if (isDefault)
            {
                IsDefaultPackageReady = true;
                OnDefaultPackageReady?.Invoke();
            }
            onReady?.Invoke();
        }

        // ── DLC 包（两阶段合并） ──────────────────────────────────────
        private IEnumerator InitDlcCoroutine(
            string packageName,
            Action onReady,
            Action<string> onFailed)
        {
            ResourcePackage package;
            if (YooAssets.ContainsPackage(packageName))
                package = YooAssets.GetPackage(packageName);
            else
                package = YooAssets.CreatePackage(packageName);

#if UNITY_EDITOR
            var buildResult = EditorSimulateModeHelper.SimulateBuild(packageName);
            var editorParam = new EditorSimulateModeParameters
            {
                EditorFileSystemParameters =
                    FileSystemParameters.CreateDefaultEditorFileSystemParameters(buildResult.PackageRootDirectory)
            };
            var initOp = package.InitializeAsync(editorParam);
#elif UNITY_WEBGL
            var webRemote = new RemoteServices(cdnBaseUrl, cdnFallbackUrl);
            var webParam  = new WebPlayModeParameters
            {
                WebFileSystemParameters =
                    FileSystemParameters.CreateDefaultWebFileSystemParameters(webRemote)
            };
            var initOp = package.InitializeAsync(webParam);
#else
            var hostRemote = new RemoteServices(cdnBaseUrl, cdnFallbackUrl);
            var hostParam  = new HostPlayModeParameters
            {
                BuildinFileSystemParameters =
                    FileSystemParameters.CreateDefaultBuildinFileSystemParameters(),
                CacheFileSystemParameters   =
                    FileSystemParameters.CreateDefaultCacheFileSystemParameters(hostRemote)
            };
            var initOp = package.InitializeAsync(hostParam);
#endif

            yield return initOp;

            if (initOp.Status != EOperationStatus.Succeed)
            {
                string err = $"[YooAsset] DLC '{packageName}' init failed: {initOp.Error}";
                Debug.LogError(err);
                onFailed?.Invoke(err);
                yield break;
            }

            // 激活本地 Manifest（同主包逻辑）
            var dlcLocalVersion = package.GetPackageVersion();
            var dlcLocalManifestOp = package.UpdatePackageManifestAsync(dlcLocalVersion);
            yield return dlcLocalManifestOp;

            if (dlcLocalManifestOp.Status != EOperationStatus.Succeed)
            {
                string err = $"[YooAsset] DLC '{packageName}' activate manifest failed: {dlcLocalManifestOp.Error}";
                Debug.LogError(err);
                onFailed?.Invoke(err);
                yield break;
            }

#if UNITY_EDITOR
            Debug.Log($"[YooAsset] DLC '{packageName}' ready (Editor Simulate)");
            onReady?.Invoke();
#else
            yield return CheckAndDownloadCoroutine(package, packageName,
                isDefault: false, onReady: onReady, onFailed: onFailed);
#endif
        }

        // ── 下载缺失 bundle ───────────────────────────────────────────
        private IEnumerator DownloadMissingBundles(ResourcePackage package, string packageName)
        {
            var downloader = package.CreateResourceDownloader(
                downloadingMaxNumber: 10, failedTryAgain: 3);

            if (downloader.TotalDownloadCount == 0)
            {
                Debug.Log($"[YooAsset] '{packageName}': no bundles to download");
                yield break;
            }

            Debug.Log($"[YooAsset] '{packageName}': {downloader.TotalDownloadCount} bundles " +
                      $"({downloader.TotalDownloadBytes / 1024f / 1024f:F1} MB)");

            downloader.DownloadUpdateCallback = (data) =>
            {
                OnDownloadProgress?.Invoke(data.Progress);
            };

            downloader.BeginDownload();
            yield return downloader;

            if (downloader.Status != EOperationStatus.Succeed)
                Debug.LogError($"[YooAsset] Download failed for '{packageName}': {downloader.Error}");
        }
    }
}
