// YooAssetInitializer.cs
// 负责初始化 YooAsset 主包（DefaultPackage）和 DLC 分包（DlcChapterXX）
//
// 初始化流程（每个 Package）：
//   1. CreatePackage / 获取已有包
//   2. 初始化（Editor 模式用 EditorSimulate，真机用 HostPlay）
//   3. 请求最新版本号
//   4. 更新 Manifest
//   5. 创建下载器下载缺失资源
//
// 主包（DefaultPackage）：游戏启动时必须完成，包含 Loading UI、主菜单、第1章前2关
// DLC 包（DlcChapterXX）：按需初始化，玩家进入对应章节时调用 InitDlcPackageAsync

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
        public string cdnBaseUrl   = "https://cdn.example.com/res";
        [Tooltip("备用 CDN，留空则同主 CDN")]
        public string cdnFallbackUrl = "";

        [Header("主包配置")]
        public string defaultPackageName = "DefaultPackage";

        // ── 事件 ─────────────────────────────────────────────────────
        /// <summary>主包初始化成功</summary>
        public event Action OnDefaultPackageReady;
        /// <summary>初始化失败</summary>
        public event Action<string> OnInitFailed;
        /// <summary>下载进度 0~1</summary>
        public event Action<float> OnDownloadProgress;

        // ── 公共状态 ─────────────────────────────────────────────────
        public bool IsDefaultPackageReady { get; private set; }

        // ── 生命周期 ─────────────────────────────────────────────────
        private void Awake()
        {
            YooAssets.Initialize();
        }

        /// <summary>
        /// 启动主包初始化。由 GameLauncher 在 Start 中调用。
        /// </summary>
        public void InitDefaultPackage()
        {
            StartCoroutine(InitPackageCoroutine(defaultPackageName, isDefault: true));
        }

        /// <summary>
        /// 按需初始化 DLC 分包（章节资源包）。
        /// 例：InitDlcPackageAsync("DlcChapter1", callback)
        /// </summary>
        public void InitDlcPackage(string packageName, Action onReady, Action<string> onFailed = null)
        {
            StartCoroutine(InitPackageCoroutine(packageName, isDefault: false, onReady, onFailed));
        }

        // ── 核心初始化协程 ────────────────────────────────────────────
        private IEnumerator InitPackageCoroutine(
            string packageName,
            bool   isDefault,
            Action onReady   = null,
            Action<string> onFailed = null)
        {
            // 1. 获取或创建 Package
            ResourcePackage package;
            if (YooAssets.ContainsPackage(packageName))
                package = YooAssets.GetPackage(packageName);
            else
                package = YooAssets.CreatePackage(packageName);

            if (isDefault)
                YooAssets.SetDefaultPackage(package);

            // 2. 初始化参数（Editor 模拟 / 真机分流）
            InitializationOperationBase initOp;
#if UNITY_EDITOR
            // Editor 下使用模拟模式，不需要实际打包
            var buildResult = EditorSimulateModeHelper.SimulateBuild(EDefaultBuildPipeline.BuiltinBuildPipeline, packageName);
            var editorParam = new EditorSimulateModeParameters
            {
                EditorFileSystemParameters = FileSystemParameters.CreateDefaultEditorFileSystemParameters(buildResult)
            };
            initOp = package.InitializeAsync(editorParam);
#else
            // 真机 HostPlay 模式：内置资源 + CDN 更新
            var remoteServices = new RemoteServices(cdnBaseUrl, cdnFallbackUrl);
            var hostParam = new HostPlayModeParameters
            {
                BuildinFileSystemParameters = FileSystemParameters.CreateDefaultBuildinFileSystemParameters(),
                CacheFileSystemParameters   = FileSystemParameters.CreateDefaultCacheFileSystemParameters(remoteServices)
            };
            initOp = package.InitializeAsync(hostParam);
#endif

            yield return initOp;

            if (initOp.Status != EOperationStatus.Succeed)
            {
                string err = $"[YooAsset] Init package '{packageName}' failed: {initOp.Error}";
                Debug.LogError(err);
                onFailed?.Invoke(err);
                if (isDefault) OnInitFailed?.Invoke(err);
                yield break;
            }

#if UNITY_EDITOR
            // Editor 模式跳过版本/Manifest 更新，直接就绪
            Debug.Log($"[YooAsset] Package '{packageName}' ready (Editor Simulate)");
            SetPackageReady(packageName, isDefault, onReady);
            yield break;
#else
            // 3. 请求远端版本
            var versionOp = package.RequestPackageVersionAsync();
            yield return versionOp;

            if (versionOp.Status != EOperationStatus.Succeed)
            {
                // 版本请求失败时使用本地缓存版本继续（离线容错）
                Debug.LogWarning($"[YooAsset] RequestVersion failed for '{packageName}', using cached version. Error: {versionOp.Error}");
            }

            // 4. 更新 Manifest
            string packageVersion = versionOp.Status == EOperationStatus.Succeed
                ? versionOp.PackageVersion
                : package.GetPackageVersion();

            var manifestOp = package.UpdatePackageManifestAsync(packageVersion);
            yield return manifestOp;

            if (manifestOp.Status != EOperationStatus.Succeed)
            {
                Debug.LogWarning($"[YooAsset] UpdateManifest failed for '{packageName}': {manifestOp.Error}");
                // 使用旧 Manifest 继续，不中断游戏
            }

            // 5. 检查并下载缺失资源
            yield return DownloadMissingBundles(package, packageName);

            Debug.Log($"[YooAsset] Package '{packageName}' ready (HostPlay)");
            SetPackageReady(packageName, isDefault, onReady);
#endif
        }

        private IEnumerator DownloadMissingBundles(ResourcePackage package, string packageName)
        {
            var downloader = package.CreateResourceDownloader(downloadingMaxNumber: 10, failedTryAgain: 3);
            if (downloader.TotalDownloadCount == 0)
                yield break;

            Debug.Log($"[YooAsset] '{packageName}': {downloader.TotalDownloadCount} bundles to download ({downloader.TotalDownloadBytes / 1024 / 1024f:F1} MB)");

            downloader.OnDownloadProgressCallback = (total, done, totalBytes, doneBytes) =>
            {
                float progress = total > 0 ? (float)done / total : 0f;
                OnDownloadProgress?.Invoke(progress);
            };

            downloader.BeginDownload();
            yield return downloader;

            if (downloader.Status != EOperationStatus.Succeed)
            {
                Debug.LogError($"[YooAsset] Download failed for '{packageName}': {downloader.Error}");
            }
        }

        private void SetPackageReady(string packageName, bool isDefault, Action onReady)
        {
            if (isDefault)
            {
                IsDefaultPackageReady = true;
                OnDefaultPackageReady?.Invoke();
            }
            onReady?.Invoke();
        }
    }
}
