// SceneLoader.cs
// 场景加载管理器 — 封装 YooAsset 场景包 + Unity SceneManager
//
// 职责：
//   - 异步加载/卸载场景（支持 YooAsset 打包场景 & Scenes in Build 直接场景）
//   - 提供进度回调（0~1）
//   - 暴露静态 Lua 绑定接口，供 game/scene_manager.lua 调用
//
// 用法（Lua）：
//   local SM = require 'game/scene_manager'
//   SM.load('Main', function(progress) end, function() end)
//   SM.load_additive('HUD', nil, function() end)
//   SM.unload('HUD', function() end)

using System;
using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using UnityEngine.SceneManagement;
using XLua;
using YooAsset;

namespace CutRope.Framework
{
    [LuaCallCSharp]
    public class SceneLoader : MonoBehaviour
    {
        // ── 单例 ─────────────────────────────────────────────────────
        public static SceneLoader Instance { get; private set; }

        // 已通过 YooAsset 加载的场景句柄（用于卸载）
        private readonly Dictionary<string, SceneHandle> _sceneHandles = new Dictionary<string, SceneHandle>();

        // ── 生命周期 ─────────────────────────────────────────────────
        private void Awake()
        {
            if (Instance != null && Instance != this) { Destroy(gameObject); return; }
            Instance = this;
            DontDestroyOnLoad(gameObject);
        }

        private void OnDestroy()
        {
            if (Instance == this) Instance = null;
        }

        // ── 公共 API（C# 侧）────────────────────────────────────────

        /// <summary>
        /// 异步加载场景（单场景模式，替换当前场景）
        /// </summary>
        /// <param name="sceneName">YooAsset 场景 address 或 Scenes in Build 场景名</param>
        /// <param name="onProgress">进度回调 0~1（可为 null）</param>
        /// <param name="onComplete">完成回调（可为 null）</param>
        public void LoadScene(string sceneName, Action<float> onProgress = null, Action onComplete = null)
        {
            StartCoroutine(LoadSceneCoroutine(sceneName, LoadSceneMode.Single, onProgress, onComplete));
        }

        /// <summary>
        /// 异步叠加加载场景（Additive 模式）
        /// </summary>
        public void LoadSceneAdditive(string sceneName, Action<float> onProgress = null, Action onComplete = null)
        {
            StartCoroutine(LoadSceneCoroutine(sceneName, LoadSceneMode.Additive, onProgress, onComplete));
        }

        /// <summary>
        /// 卸载已叠加加载的场景
        /// </summary>
        public void UnloadScene(string sceneName, Action onComplete = null)
        {
            StartCoroutine(UnloadSceneCoroutine(sceneName, onComplete));
        }

        // ── Lua 绑定（静态，方便 Lua 直接调用）──────────────────────

        [CSharpCallLua]
        public delegate void LuaProgressCallback(float progress);
        [CSharpCallLua]
        public delegate void LuaCompleteCallback();

        /// <summary>供 Lua 调用：LoadScene</summary>
        public static void LuaLoadScene(string sceneName, LuaProgressCallback onProgress, LuaCompleteCallback onComplete)
        {
            if (Instance == null) { Debug.LogError("[SceneLoader] Instance is null"); return; }
            Instance.LoadScene(sceneName,
                onProgress != null ? (p) => onProgress(p) : (Action<float>)null,
                onComplete != null ? (Action)(() => onComplete()) : null);
        }

        /// <summary>供 Lua 调用：LoadSceneAdditive</summary>
        public static void LuaLoadSceneAdditive(string sceneName, LuaProgressCallback onProgress, LuaCompleteCallback onComplete)
        {
            if (Instance == null) { Debug.LogError("[SceneLoader] Instance is null"); return; }
            Instance.LoadSceneAdditive(sceneName,
                onProgress != null ? (p) => onProgress(p) : (Action<float>)null,
                onComplete != null ? (Action)(() => onComplete()) : null);
        }

        /// <summary>供 Lua 调用：UnloadScene</summary>
        public static void LuaUnloadScene(string sceneName, LuaCompleteCallback onComplete)
        {
            if (Instance == null) { Debug.LogError("[SceneLoader] Instance is null"); return; }
            Instance.UnloadScene(sceneName,
                onComplete != null ? (Action)(() => onComplete()) : null);
        }

        // ── 协程实现 ─────────────────────────────────────────────────
        private IEnumerator LoadSceneCoroutine(
            string sceneName,
            LoadSceneMode mode,
            Action<float> onProgress,
            Action onComplete)
        {
            Debug.Log($"[SceneLoader] Loading scene: '{sceneName}' ({mode})");

            // 优先尝试 YooAsset 场景包
            var package = YooAssets.GetPackage("DefaultPackage");
            bool useYoo = package != null && IsYooAssetScene(package, sceneName);

            if (useYoo)
            {
                var sceneMode = mode == LoadSceneMode.Single
                    ? UnityEngine.SceneManagement.LoadSceneMode.Single
                    : UnityEngine.SceneManagement.LoadSceneMode.Additive;

                var handle = package.LoadSceneAsync(sceneName, sceneMode);
                _sceneHandles[sceneName] = handle;

                while (!handle.IsDone)
                {
                    onProgress?.Invoke(handle.Progress);
                    yield return null;
                }

                if (handle.Status != EOperationStatus.Succeed)
                {
                    Debug.LogError($"[SceneLoader] YooAsset load scene failed: {handle.LastError}");
                    yield break;
                }
            }
            else
            {
                // 回退到 Unity 直接加载（Scenes in Build）
                var asyncOp = UnityEngine.SceneManagement.SceneManager.LoadSceneAsync(sceneName, mode);
                if (asyncOp == null)
                {
                    Debug.LogError($"[SceneLoader] Scene not found: '{sceneName}'. Add it to Build Settings.");
                    yield break;
                }

                while (!asyncOp.isDone)
                {
                    onProgress?.Invoke(asyncOp.progress);
                    yield return null;
                }
            }

            onProgress?.Invoke(1f);
            Debug.Log($"[SceneLoader] Scene loaded: '{sceneName}'");
            onComplete?.Invoke();
        }

        private IEnumerator UnloadSceneCoroutine(string sceneName, Action onComplete)
        {
            Debug.Log($"[SceneLoader] Unloading scene: '{sceneName}'");

            if (_sceneHandles.TryGetValue(sceneName, out var handle))
            {
                // YooAsset 场景通过 handle.UnloadScene() 卸载
                handle.UnloadScene();
                _sceneHandles.Remove(sceneName);
            }
            else
            {
                // 直接 Unity 卸载
                var asyncOp = UnityEngine.SceneManagement.SceneManager.UnloadSceneAsync(sceneName);
                if (asyncOp != null) yield return asyncOp;
            }

            Debug.Log($"[SceneLoader] Scene unloaded: '{sceneName}'");
            onComplete?.Invoke();
        }

        // ── 工具方法 ─────────────────────────────────────────────────
        private static bool IsYooAssetScene(ResourcePackage package, string sceneName)
        {
            if (package == null) return false;
            var info = package.GetAssetInfo(sceneName);
            return info != null && info.AssetPath.EndsWith(".unity", StringComparison.OrdinalIgnoreCase);
        }
    }
}
