// YooAssetsLuaBridge.cs
// YooAssets Lua 胶水层（方式一：手写 C# Binding）
// 适配 xLua，所有 async 操作均包装为协程 + 回调模式，Lua 侧只需传回调函数。
//
// 依赖：
//   - YooAsset (本工程版本，含 InitializeParameters / UpdatePackageManifestAsync)
//   - CutRope.Framework.RemoteServices (IRemoteServices 实现)
//
// xLua 配置：在 GenConfig 的 [LuaCallCSharp] 列表中加入 typeof(YooAssetsLuaBridge)
//
// Lua 使用示例：
//   local Bridge = CS.YooAssetsLuaBridge
//   Bridge.SetupYooAssets()
//   Bridge.InitializeOffline("DefaultPackage", function(ok, err) ... end)
//   Bridge.RequestVersion("DefaultPackage", function(ok, ver, err) ... end)
//   Bridge.LoadAsset("DefaultPackage", "Assets/Prefabs/Hero.prefab",
//       function(ok, asset, err) ... end)

using System;
using System.Collections;
using System.Collections.Generic;
using System.IO;
using UnityEngine;
using UnityEngine.SceneManagement;
using XLua;
using YooAsset;

namespace UGFramework.Runtime
{
    [LuaCallCSharp]
    public static class YooAssetsLuaBridge
    {
        // ─────────────────────────────────────────────────────────────────────────
        // 0. Lua 文件索引（require 路径 → YooAsset AssetPath）
        //    由 LoadAllLuaFiles 填充，供 Lua loader 同步查询
        // ─────────────────────────────────────────────────────────────────────────
        private static readonly Dictionary<string, string> _luaIndex
            = new Dictionary<string, string>(StringComparer.OrdinalIgnoreCase);

        private static readonly Dictionary<string, AssetHandle> _luaBundle
            = new Dictionary<string, AssetHandle>(StringComparer.OrdinalIgnoreCase);

        /// <summary>同步查询 lua 文件是否存在（require 名，如 "main"、"game/util"）</summary>
        public static bool HasLuaFile(string requireName)
        {
            if (string.IsNullOrEmpty(requireName)) return false;
            return _luaIndex.ContainsKey(NormalizeRequireName(requireName));
        }

        public static AssetHandle GetLuaAssetHandle(string requireName)
        {
            if (string.IsNullOrEmpty(requireName)) return null;
            _luaBundle.TryGetValue(NormalizeRequireName(requireName), out var outHandle);
            return outHandle;
        }

        public static void ReleaseLuaAssetHandle(string requireName)
        {
            if (string.IsNullOrEmpty(requireName)) return;
            string key = NormalizeRequireName(requireName);
            if (_luaBundle.TryGetValue(key, out var handle))
            {
                handle.Release();
                _luaBundle.Remove(key);
                _luaIndex.Remove(key);
            }
        }

        /// <summary>查询 lua 文件对应的 YooAsset AssetPath，不存在返回 null</summary>
        public static string GetLuaAssetPath(string requireName)
        {
            if (string.IsNullOrEmpty(requireName)) return null;
            _luaIndex.TryGetValue(NormalizeRequireName(requireName), out var path);
            return path;
        }

        /// <summary>已索引的 lua 文件总数</summary>
        public static int LuaFileCount => _luaIndex.Count;

        /// <summary>返回所有已索引的 require 名（数组），供 Lua 遍历用</summary>
        public static string[] GetAllLuaNames()
        {
            var arr = new string[_luaIndex.Count];
            int i = 0;
            foreach (var k in _luaIndex.Keys) arr[i++] = k;
            return arr;
        }

        /// <summary>清空索引（重新初始化资源系统时用）</summary>
        public static void ClearLuaIndex()
        {
            _luaIndex.Clear();
        }

        // require 名规范化：统一小写、把 . 和 \ 都转成 /
        private static string NormalizeRequireName(string name)
        {
            return name.Replace('.', '/').Replace('\\', '/').ToLowerInvariant();
        }

        // ─────────────────────────────────────────────────────────────────────────
        // 1. 全局初始化
        // ─────────────────────────────────────────────────────────────────────────

        /// <summary>初始化 YooAssets 框架（全局，只调一次）</summary>
        public static void SetupYooAssets()
        {
            YooAssets.Initialize();
        }

        public static bool HasPackage(string packageName)
        {
            return YooAssets.ContainsPackage(packageName);
        }

        public static bool RemovePackage(string packageName)
        {
            if (YooAssets.ContainsPackage(packageName))
                return YooAssets.RemovePackage(packageName);
            return false;
        }

        public static bool CreatePackage(string packageName)
        {
            if (YooAssets.ContainsPackage(packageName))
            {
                Debug.LogError($"Package {packageName} already exists!");
                return false;
            }

            YooAssets.CreatePackage(packageName);
            return true;
        }

        public static void SetDefaultPackage(string packageName)
        {
            YooAssets.SetDefaultPackage(YooAssets.GetPackage(packageName));
        }

        [CSharpCallLua]
        public delegate void LuaBoolStringCallback(bool success, string err);

        [CSharpCallLua]
        public delegate void LuaBoolStringStringCallback(bool success, string value, string err);

        // ─────────────────────────────────────────────────────────────────────────
        // 2. 包裹初始化
        // ─────────────────────────────────────────────────────────────────────────
        /// <summary>
        /// 离线模式初始化（仅内置资源，零网络依赖）
        /// callback(bool ok, string error)
        /// </summary>
        public static void InitializeOffline(string packageName, LuaBoolStringCallback callback)
        {
            CoroutineRunner.Instance.StartCoroutine(Co_InitOffline(packageName, callback));
        }

        private static IEnumerator Co_InitOffline(string packageName, LuaBoolStringCallback callback)
        {
            if (YooAssets.ContainsPackage(packageName))
                YooAssets.RemovePackage(packageName);

            var package = YooAssets.CreatePackage(packageName);
            var param = new OfflinePlayModeParameters
            {
                BuildinFileSystemParameters =
                    FileSystemParameters.CreateDefaultBuildinFileSystemParameters()
            };

            var initOp = package.InitializeAsync(param);
            yield return initOp;

            if (initOp.Status != EOperationStatus.Succeed)
            {
                callback?.Invoke(false, initOp.Error);
                yield break;
            }

            callback?.Invoke(true, null);
        }

        /// <summary>
        /// 联机热更模式初始化（BuildinFS + CacheFS，支持 CDN 增量下载）
        /// callback(bool ok, string error)
        /// </summary>
        public static void InitializeHostPlay(
            string packageName,
            string mainCdnUrl,
            string fallbackCdnUrl,
            LuaBoolStringCallback callback)
        {
            CoroutineRunner.Instance.StartCoroutine(
                Co_InitHostPlay(packageName, mainCdnUrl, fallbackCdnUrl, callback));
        }

        private static IEnumerator Co_InitHostPlay(
            string packageName,
            string mainCdnUrl,
            string fallbackCdnUrl,
            LuaBoolStringCallback callback)
        {
            if (YooAssets.ContainsPackage(packageName))
                YooAssets.RemovePackage(packageName);

            var package = YooAssets.CreatePackage(packageName);
            var remote = new RemoteServices(mainCdnUrl, fallbackCdnUrl);
            var param = new HostPlayModeParameters
            {
                BuildinFileSystemParameters =
                    FileSystemParameters.CreateDefaultBuildinFileSystemParameters(),
                CacheFileSystemParameters =
                    FileSystemParameters.CreateDefaultCacheFileSystemParameters(remote)
            };

            var initOp = package.InitializeAsync(param);
            yield return initOp;

            if (initOp.Status != EOperationStatus.Succeed)
            {
                callback?.Invoke(false, initOp.Error);
                yield break;
            }

            callback?.Invoke(true, null);
        }

        // ─────────────────────────────────────────────────────────────────────────
        // 3. 版本 & 清单
        // ─────────────────────────────────────────────────────────────────────────
        /// <summary>
        /// 请求最新包裹版本（需联网）
        /// callback(bool ok, string version, string error)
        /// </summary>
        public static void RequestVersion(string packageName, LuaBoolStringStringCallback callback)
        {
            CoroutineRunner.Instance.StartCoroutine(Co_RequestVersion(packageName, callback));
        }

        private static IEnumerator Co_RequestVersion(string packageName, LuaBoolStringStringCallback callback)
        {
            var package = YooAssets.GetPackage(packageName);
            var op = package.RequestPackageVersionAsync();
            yield return op;

            if (op.Status == EOperationStatus.Succeed)
                callback?.Invoke(true, op.PackageVersion, null);
            else
                callback?.Invoke(false, null, op.Error);
        }

        /// <summary>
        /// 获取当前已激活的版本号（同步）
        /// </summary>
        public static string GetPackageVersion(string packageName)
        {
            var package = YooAssets.TryGetPackage(packageName);
            if (package == null || !package.PackageValid) return "";
            return package.GetPackageVersion();
        }

        /// <summary>
        /// 更新并加载指定版本的资源清单
        /// callback(bool ok, string error)
        /// </summary>
        public static void UpdateManifest(string packageName, string version, LuaBoolStringCallback callback)
        {
            CoroutineRunner.Instance.StartCoroutine(Co_UpdateManifest(packageName, version, callback));
        }

        private static IEnumerator Co_UpdateManifest(string packageName, string version, LuaBoolStringCallback callback)
        {
            var package = YooAssets.GetPackage(packageName);
            var op = package.UpdatePackageManifestAsync(version);
            yield return op;

            if (op.Status == EOperationStatus.Succeed)
                callback?.Invoke(true, null);
            else
                callback?.Invoke(false, op.Error);
        }

        // ─────────────────────────────────────────────────────────────────────────
        // 4. 下载
        // ─────────────────────────────────────────────────────────────────────────

        /// <summary>
        /// 获取待下载文件数量（同步，需在 UpdateManifest 后调用）
        /// </summary>
        public static int GetDownloadCount(string packageName, int maxConcurrent = 10, int retryCount = 3)
        {
            var package = YooAssets.GetPackage(packageName);
            var downloader = package.CreateResourceDownloader(maxConcurrent, retryCount);
            return downloader.TotalDownloadCount;
        }

        [CSharpCallLua]
        public delegate void LuaResourceDownloadProgressCallback(int TotalDownloadCount, int CurrentDownloadCount,
            long TotalDownloadBytes, long CurrentDownloadBytes);

        /// <summary>
        /// 开始下载缺失资源
        /// onProgress(int totalCount, int downloadedCount, long totalBytes, long downloadedBytes)
        /// onComplete(bool ok, string error)
        /// </summary>
        public static void StartDownload(
            string packageName,
            int maxConcurrent,
            int failedRetryCount,
            LuaResourceDownloadProgressCallback onProgress,
            LuaBoolStringCallback onComplete)
        {
            CoroutineRunner.Instance.StartCoroutine(
                Co_Download(packageName, maxConcurrent, failedRetryCount, onProgress, onComplete));
        }

        private static IEnumerator Co_Download(
            string packageName,
            int maxConcurrent,
            int failedRetryCount,
            LuaResourceDownloadProgressCallback onProgress,
            LuaBoolStringCallback onComplete)
        {
            var package = YooAssets.GetPackage(packageName);
            var downloader = package.CreateResourceDownloader(maxConcurrent, failedRetryCount);

            if (downloader.TotalDownloadCount == 0)
            {
                onComplete?.Invoke(true, null);
                yield break;
            }

            // DownloadUpdateCallback 对应本工程的 DownloadUpdateData struct
            downloader.DownloadUpdateCallback = data =>
            {
                onProgress?.Invoke(
                    data.TotalDownloadCount,
                    data.CurrentDownloadCount,
                    data.TotalDownloadBytes,
                    data.CurrentDownloadBytes);
            };

            downloader.BeginDownload();
            yield return downloader;

            if (downloader.Status == EOperationStatus.Succeed)
                onComplete?.Invoke(true, null);
            else
                onComplete?.Invoke(false, downloader.Error);
        }

        // ─────────────────────────────────────────────────────────────────────────
        // 5. 资源加载
        // ─────────────────────────────────────────────────────────────────────────
        [CSharpCallLua]
        public delegate void LuaLoadAssetCallback(bool success, UnityEngine.Object obj, string err);

        /// <summary>
        /// 异步加载 UnityEngine.Object 资源
        /// callback(bool ok, UnityEngine.Object asset, string error)
        /// 注意：asset 不再使用时请调用 TryUnloadUnusedAsset(packageName, location)
        /// </summary>
        public static void LoadAsset(string packageName, string location, LuaLoadAssetCallback callback)
        {
            CoroutineRunner.Instance.StartCoroutine(Co_LoadAsset(packageName, location, callback));
        }

        private static IEnumerator Co_LoadAsset(string packageName, string location, LuaLoadAssetCallback callback)
        {
            var package = YooAssets.GetPackage(packageName);
            var handle = package.LoadAssetAsync(location, typeof(UnityEngine.Object));
            yield return handle;

            if (handle.Status == EOperationStatus.Succeed)
            {
                callback?.Invoke(true, handle.AssetObject, null);
                // handle 不在此处 Release，由 YooAsset 引用计数管理；
                // 调用方不再使用时可调 TryUnloadUnusedAsset
            }
            else
            {
                callback?.Invoke(false, null, handle.LastError);
                handle.Release();
            }
        }

        [CSharpCallLua]
        public delegate void LuaInstantiateCallback(bool success, GameObject obj, string err);

        /// <summary>
        /// 异步加载 GameObject 并实例化（handle 自动 Release）
        /// callback(bool ok, GameObject instance, string error)
        /// </summary>
        public static void InstantiateAsync(
            string packageName,
            string location,
            LuaInstantiateCallback callback,
            Transform parent = null)
        {
            CoroutineRunner.Instance.StartCoroutine(Co_Instantiate(packageName, location, parent, callback));
        }

        private static IEnumerator Co_Instantiate(
            string packageName,
            string location,
            Transform parent,
            LuaInstantiateCallback callback)
        {
            var package = YooAssets.GetPackage(packageName);
            var handle = package.LoadAssetAsync<GameObject>(location);
            yield return handle;

            if (handle.Status != EOperationStatus.Succeed)
            {
                callback?.Invoke(false, null, handle.LastError);
                handle.Release();
                yield break;
            }

            var prefab = handle.AssetObject as GameObject;
            var go = parent != null
                ? UnityEngine.Object.Instantiate(prefab, parent)
                : UnityEngine.Object.Instantiate(prefab);

            handle.Release(); // 实例已创建，释放句柄引用计数
            callback?.Invoke(true, go, null);
        }

        /// <summary>
        /// 异步加载场景
        /// callback(bool ok, string error)
        /// </summary>
        public static void LoadScene(
            string packageName,
            string location,
            bool additive,
            LuaBoolStringCallback callback)
        {
            CoroutineRunner.Instance.StartCoroutine(Co_LoadScene(packageName, location, additive, callback));
        }

        private static IEnumerator Co_LoadScene(
            string packageName,
            string location,
            bool additive,
            LuaBoolStringCallback callback)
        {
            var package = YooAssets.GetPackage(packageName);
            var loadMode = additive ? LoadSceneMode.Additive : LoadSceneMode.Single;
            var handle = package.LoadSceneAsync(location, loadMode);
            yield return handle;

            if (handle.Status == EOperationStatus.Succeed)
                callback?.Invoke(true, null);
            else
                callback?.Invoke(false, handle.LastError);
        }

        [CSharpCallLua]
        public delegate void LuaLoadRawFileCallback(bool success, byte[] bytes, string err);

        /// <summary>
        /// 异步加载原生文件（如 JSON/二进制配置）
        /// callback(bool ok, byte[] data, string error)
        /// </summary>
        public static void LoadRawFile(string packageName, string location, LuaLoadRawFileCallback callback)
        {
            CoroutineRunner.Instance.StartCoroutine(Co_LoadRawFile(packageName, location, callback));
        }

        private static IEnumerator Co_LoadRawFile(string packageName, string location, LuaLoadRawFileCallback callback)
        {
            var package = YooAssets.GetPackage(packageName);
            var handle = package.LoadRawFileAsync(location);
            yield return handle;

            if (handle.Status == EOperationStatus.Succeed)
            {
                callback?.Invoke(true, handle.GetRawFileData(), null);
            }
            else
            {
                callback?.Invoke(false, null, handle.LastError);
            }

            handle.Release();
        }

        /// <summary>
        /// 批量异步加载指定前缀下所有 .lua 文件。
        ///
        /// onProgress(int loadedCount, int totalCount) — 每加载完一个回调一次（可空）
        /// onComplete(bool ok, string[] requirePaths, string[] sources, string error)
        ///   · requirePaths：去掉前缀和 .lua 后缀的路径，斜杠形式（直接可作 require 名）
        ///                   如 "main"、"game/util"
        ///   · sources：对应的 lua 源码字符串（UTF-8 解码后），与 requirePaths 一一对应
        ///   · 失败时 ok=false，其余参数可能为 null
        ///
        /// Lua 用法：
        ///   Bridge.LoadAllLuaFiles("DefaultPackage", "Assets/Lua/", nil,
        ///       function(ok, paths, sources, err)
        ///           if not ok then print(err) return end
        ///           for i = 0, paths.Length - 1 do
        ///               package.preload[paths[i]] = function() return assert(load(sources[i], paths[i]))() end
        ///           end
        ///       end)
        /// </summary>
        /// <summary>

        [CSharpCallLua]
        public delegate void LuaLoadAllLuaFilesProgressCallback(int loaded, int total);

        [CSharpCallLua]
        public delegate void LuaLoadAllLuaFilesCompleteCallback(bool success, string[] requirePaths, bool[] successMap,
            string err);

        public static void LoadAllLuaFiles(
            string packageName,
            string assetPrefix,
            LuaLoadAllLuaFilesProgressCallback onProgress,
            LuaLoadAllLuaFilesCompleteCallback onComplete)
        {
            CoroutineRunner.Instance.StartCoroutine(
                Co_LoadAllLuaFiles(packageName, assetPrefix, onProgress, onComplete));
        }

        private static IEnumerator Co_LoadAllLuaFiles(
            string packageName,
            string assetPrefix,
            LuaLoadAllLuaFilesProgressCallback onProgress,
            LuaLoadAllLuaFilesCompleteCallback onComplete)
        {
            var package = YooAssets.GetPackage(packageName);
            if (package == null)
            {
                onComplete?.Invoke(false, null, null, $"package '{packageName}' not found");
                yield break;
            }

            // 1. 枚举所有 .lua 资源
            var assetInfos = package.GetAssetInfos(assetPrefix);
            if (assetInfos == null || assetInfos.Length == 0)
            {
                Debug.LogWarning($"[YooAssetsLuaBridge] No assets under prefix '{assetPrefix}'");
                onComplete?.Invoke(true, new string[0], new bool[0], null);
                yield break;
            }

            // 2. 过滤出 .lua 文件，预生成 require 路径，同步填充全局索引
            var luaInfos = new System.Collections.Generic.List<AssetInfo>();
            var requirePaths = new System.Collections.Generic.List<string>();

            string normPrefix = assetPrefix.Replace('\\', '/').ToLowerInvariant();
            if (!normPrefix.EndsWith("/")) normPrefix += "/";

            var handles = new List<AssetHandle>();
            foreach (var info in assetInfos)
            {
                string path = info.AssetPath.Replace('\\', '/');
                string lower = path.ToLowerInvariant();

                if (!lower.EndsWith(".lua")) continue;

                string trimmed = lower;
                int prefixIdx = trimmed.IndexOf(normPrefix, StringComparison.Ordinal);
                if (prefixIdx >= 0)
                    trimmed = trimmed.Substring(prefixIdx + normPrefix.Length);
                trimmed = trimmed.Substring(0, trimmed.Length - 4); // 去 .lua

                luaInfos.Add(info);
                requirePaths.Add(trimmed);

                // 立即填充索引：Lua 侧此时已可同步查询是否存在
                _luaIndex[trimmed] = info.AssetPath;
                _luaBundle[trimmed] = package.LoadAssetAsync<TextAsset>(info.AssetPath);
                handles.Add(_luaBundle[trimmed]);
            }

            int total = luaInfos.Count;
            if (total == 0)
            {
                Debug.LogWarning($"[YooAssetsLuaBridge] No .lua files under '{assetPrefix}'");
                onComplete?.Invoke(true, new string[0], new bool[0], null);
                yield break;
            }

            // 3. 并发发起所有加载请求,等待全部完成        
            // 4. 顺序等待 + 进度回调
            int loaded = 0;
            int failed = 0;
            string firstError = null;
            var success = new bool[total];

            for (int i = 0; i < total; i++)
            {
                yield return handles[i];

                if (handles[i].Status == EOperationStatus.Succeed && handles[i].AssetObject is TextAsset ta)
                {
                    success[i] = true;
                }
                else
                {
                    failed++;
                    if (firstError == null)
                        firstError = $"load '{luaInfos[i].AssetPath}' failed: {handles[i].LastError}";
                    Debug.LogWarning($"[YooAssetsLuaBridge] {firstError}");
                }

                loaded++;
                try
                {
                    onProgress?.Invoke(loaded, total);
                }
                catch (Exception e)
                {
                    Debug.LogWarning($"[YooAssetsLuaBridge] onProgress: {e.Message}");
                }
            }

            Debug.Log($"[YooAssetsLuaBridge] LoadAllLuaFiles: {loaded}/{total} files (prefix='{assetPrefix}')");
            onComplete?.Invoke(firstError == null, requirePaths.ToArray(), success, firstError);
        }


        // ─────────────────────────────────────────────────────────────────────────
        // 6. 内存管理
        // ─────────────────────────────────────────────────────────────────────────

        /// <summary>尝试卸载指定的未使用资源（引用计数为 0 时才真正卸载）</summary>
        public static void TryUnloadUnusedAsset(string packageName, string location)
        {
            var package = YooAssets.TryGetPackage(packageName);
            package?.TryUnloadUnusedAsset(location);
        }

        /// <summary>
        /// 回收不再使用的资源（引用计数 = 0 的全部卸载）
        /// callback(bool ok, string error)
        /// </summary>
        public static void UnloadUnusedAssets(string packageName, LuaBoolStringCallback callback = null)
        {
            CoroutineRunner.Instance.StartCoroutine(Co_UnloadUnused(packageName, callback));
        }

        private static IEnumerator Co_UnloadUnused(string packageName, LuaBoolStringCallback callback)
        {
            var package = YooAssets.GetPackage(packageName);
            var op = package.UnloadUnusedAssetsAsync();
            yield return op;
            if (op.Status == EOperationStatus.Succeed)
                callback?.Invoke(true, null);
            else
                callback?.Invoke(false, op.Error);
        }

        /// <summary>
        /// 强制卸载所有资源（切大版本 / 退出时用）
        /// callback(bool ok, string error)
        /// </summary>
        public static void UnloadAllAssets(string packageName, LuaBoolStringCallback callback = null)
        {
            CoroutineRunner.Instance.StartCoroutine(Co_UnloadAll(packageName, callback));
        }

        private static IEnumerator Co_UnloadAll(string packageName, LuaBoolStringCallback callback)
        {
            var package = YooAssets.GetPackage(packageName);
            var op = package.UnloadAllAssetsAsync();
            yield return op;
            if (op.Status == EOperationStatus.Succeed)
                callback?.Invoke(true, null);
            else
                callback?.Invoke(false, op.Error);
        }

        /// <summary>
        /// 清理磁盘缓存（保留当前版本，清理超量旧文件）
        /// callback(bool ok, string error)
        /// </summary>
        public static void ClearCache(string packageName, LuaBoolStringCallback callback = null)
        {
            CoroutineRunner.Instance.StartCoroutine(Co_ClearCache(packageName, callback));
        }

        private static IEnumerator Co_ClearCache(string packageName, LuaBoolStringCallback callback)
        {
            var package = YooAssets.GetPackage(packageName);
            var op = package.ClearCacheFilesAsync(EFileClearMode.ClearUnusedBundleFiles);
            yield return op;
            if (op.Status == EOperationStatus.Succeed)
                callback?.Invoke(true, null);
            else
                callback?.Invoke(false, op.Error);
        }

        // ─────────────────────────────────────────────────────────────────────────
        // 内部：协程运行器（轻量单例，挂在 DontDestroyOnLoad 对象上）
        // ─────────────────────────────────────────────────────────────────────────

        private class CoroutineRunner : MonoBehaviour
        {
            private static CoroutineRunner _instance;

            public static CoroutineRunner Instance
            {
                get
                {
                    if (_instance == null)
                    {
                        var go = new GameObject("[YooAssetsLuaBridge]");
                        UnityEngine.Object.DontDestroyOnLoad(go);
                        _instance = go.AddComponent<CoroutineRunner>();
                    }

                    return _instance;
                }
            }
        }
    }
}
