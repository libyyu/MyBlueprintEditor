// BlueprintLoader.cs
// ─────────────────────────────────────────────────────────────────────────────
// 动态蓝图加载器 — 从 CDN 下载 .bjson，支持缓存 + 热更新 + A/B 测试
//
// 为什么需要这个？
//   - 小游戏包体 30MB 限制，蓝图不要打进包
//   - 策划改 NPC 对白不用重新打包上线（审核 3~7 天）
//   - A/B 测试不同人设，看玩家反馈
//   - 服务端可以按账号分发不同剧情
//
// 工作流程：
//   1. 配置 BaseURL（如 https://cdn.your-game.com/blueprints/）
//   2. LoadAsync("NPC_MeowMeow_v2.bjson") → 下载 → 缓存到本地存储 → 返回文本
//   3. 下次启动先读缓存，同时后台请求 ETag 判断是否更新
//   4. 有新版本则下载并替换缓存
//
// 缓存策略：
//   - key 前缀：blueprint_cache_
//   - value：{"etag":"...","content":"..."}
//   - Unity Editor / Standalone：PlayerPrefs
//   - WebGL / 小游戏：localStorage / wx.storage（通过 BlueprintStorage 统一接口）
// ─────────────────────────────────────────────────────────────────────────────

using System;
using System.Collections;
using UnityEngine;
using UnityEngine.Networking;

namespace BlueprintRuntime.Samples.MiniGame
{
    public class BlueprintLoader : MonoBehaviour
    {
        public static BlueprintLoader Instance { get; private set; }

        [Header("CDN 配置")]
        [Tooltip("蓝图 CDN 基址。末尾必须带 /")]
        [SerializeField] private string baseUrl = "https://cdn.your-game.com/blueprints/";

        [Tooltip("请求超时（秒）")]
        [SerializeField] private int timeoutSeconds = 15;

        [Tooltip("启用内存缓存（同一会话内重复加载不重新下载）")]
        [SerializeField] private bool memoryCache = true;

        [Tooltip("启用持久化缓存（跨会话）")]
        [SerializeField] private bool persistentCache = true;

        // 内存缓存（Dict<path, content>）
        private readonly System.Collections.Generic.Dictionary<string, string> _memCache
            = new System.Collections.Generic.Dictionary<string, string>();

        void Awake()
        {
            if (Instance != null && Instance != this) { Destroy(gameObject); return; }
            Instance = this;
            DontDestroyOnLoad(gameObject);
        }

        public string BaseUrl
        {
            get => baseUrl;
            set => baseUrl = value ?? "";
        }

        // ── 主 API ─────────────────────────────────────────────────────
        /// <summary>
        /// 加载一个蓝图。优先顺序：内存缓存 → 持久化缓存 + 后台校验 → 网络下载。
        /// </summary>
        /// <param name="relativePath">CDN 下的相对路径，例如 "NPC_MeowMeow.bjson"</param>
        /// <param name="onSuccess">成功回调（参数：bjson 文本）</param>
        /// <param name="onError">失败回调（参数：错误信息）</param>
        /// <param name="forceRefresh">忽略所有缓存强制下载</param>
        public void LoadAsync(string relativePath,
                              Action<string> onSuccess,
                              Action<string> onError = null,
                              bool forceRefresh = false)
        {
            StartCoroutine(LoadCoroutine(relativePath, onSuccess, onError, forceRefresh));
        }

        private IEnumerator LoadCoroutine(string relativePath,
                                          Action<string> onSuccess,
                                          Action<string> onError,
                                          bool forceRefresh)
        {
            if (string.IsNullOrEmpty(relativePath))
            {
                onError?.Invoke("relativePath is empty");
                yield break;
            }

            // 1. 内存缓存
            if (memoryCache && !forceRefresh && _memCache.TryGetValue(relativePath, out var mem))
            {
                onSuccess?.Invoke(mem);
                // 后台仍然尝试校验更新（不阻塞）
                StartCoroutine(RevalidateInBackground(relativePath));
                yield break;
            }

            // 2. 持久化缓存
            string cachedContent = null;
            string cachedEtag    = null;
            if (persistentCache && !forceRefresh)
            {
                cachedContent = BlueprintStorage.GetString(CacheKey(relativePath));
                cachedEtag    = BlueprintStorage.GetString(EtagKey(relativePath));
                if (!string.IsNullOrEmpty(cachedContent))
                {
                    if (memoryCache) _memCache[relativePath] = cachedContent;
                    onSuccess?.Invoke(cachedContent);
                    // 异步检查更新，有新版本就下载并更新缓存（不再触发 onSuccess）
                    StartCoroutine(RevalidateInBackground(relativePath, cachedEtag));
                    yield break;
                }
            }

            // 3. 网络下载
            string url = CombineUrl(baseUrl, relativePath);
            using (var req = UnityWebRequest.Get(url))
            {
                req.timeout = timeoutSeconds;
                yield return req.SendWebRequest();

#if UNITY_2020_2_OR_NEWER
                if (req.result != UnityWebRequest.Result.Success)
#else
                if (req.isNetworkError || req.isHttpError)
#endif
                {
                    onError?.Invoke($"{req.error} ({url})");
                    yield break;
                }

                var content = req.downloadHandler.text;
                var etag    = req.GetResponseHeader("ETag") ?? "";

                if (persistentCache)
                {
                    BlueprintStorage.SetString(CacheKey(relativePath), content);
                    BlueprintStorage.SetString(EtagKey(relativePath), etag);
                }
                if (memoryCache) _memCache[relativePath] = content;

                onSuccess?.Invoke(content);
            }
        }

        // ── 同步 API（从内置 Resources/StreamingAssets 加载，用于首包蓝图） ────
        /// <summary>
        /// 从 Resources 同步加载（首包内置蓝图）。
        /// WebGL / 小游戏均支持 Resources.Load<TextAsset>。
        /// </summary>
        public static string LoadFromResources(string resourceName)
        {
            var asset = Resources.Load<TextAsset>(resourceName);
            if (asset == null)
            {
                Debug.LogError($"[BlueprintLoader] Resources/{resourceName} not found");
                return null;
            }
            return asset.text;
        }

        // ── 缓存管理 ───────────────────────────────────────────────────
        public void ClearCache(string relativePath)
        {
            _memCache.Remove(relativePath);
            BlueprintStorage.Remove(CacheKey(relativePath));
            BlueprintStorage.Remove(EtagKey(relativePath));
        }

        public void ClearAllCache()
        {
            _memCache.Clear();
            // 注意：BlueprintStorage.Clear 会清全部蓝图前缀 key（包括 NPC 记忆）
            // 如果只想清蓝图缓存，必须手动遍历；此处保持保守不实现
            Debug.LogWarning("[BlueprintLoader] ClearAllCache 仅清内存缓存，持久化缓存请用 ClearCache(path) 逐个清");
        }

        // ── 内部 ────────────────────────────────────────────────────────
        private IEnumerator RevalidateInBackground(string relativePath, string cachedEtag = null)
        {
            if (string.IsNullOrEmpty(baseUrl)) yield break;

            string url = CombineUrl(baseUrl, relativePath);
            using (var req = UnityWebRequest.Head(url))
            {
                req.timeout = timeoutSeconds;
                yield return req.SendWebRequest();

#if UNITY_2020_2_OR_NEWER
                if (req.result != UnityWebRequest.Result.Success) yield break;
#else
                if (req.isNetworkError || req.isHttpError) yield break;
#endif

                var remoteEtag = req.GetResponseHeader("ETag") ?? "";
                if (string.IsNullOrEmpty(remoteEtag) || remoteEtag == cachedEtag) yield break;

                // 有新版本，下载并更新缓存（不触发 onSuccess，下次 Load 自动拿到新版）
                using (var get = UnityWebRequest.Get(url))
                {
                    get.timeout = timeoutSeconds;
                    yield return get.SendWebRequest();
#if UNITY_2020_2_OR_NEWER
                    if (get.result != UnityWebRequest.Result.Success) yield break;
#else
                    if (get.isNetworkError || get.isHttpError) yield break;
#endif
                    var newContent = get.downloadHandler.text;
                    if (persistentCache)
                    {
                        BlueprintStorage.SetString(CacheKey(relativePath), newContent);
                        BlueprintStorage.SetString(EtagKey(relativePath), remoteEtag);
                    }
                    if (memoryCache) _memCache[relativePath] = newContent;
                    Debug.Log($"[BlueprintLoader] {relativePath} updated in background (etag changed)");
                }
            }
        }

        private static string CacheKey(string path) => $"bp_cache.{path}";
        private static string EtagKey (string path) => $"bp_etag.{path}";

        private static string CombineUrl(string baseUrl, string path)
        {
            if (string.IsNullOrEmpty(baseUrl)) return path;
            if (baseUrl.EndsWith("/")) return baseUrl + path.TrimStart('/');
            return baseUrl + "/" + path.TrimStart('/');
        }
    }
}
