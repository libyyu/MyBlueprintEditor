// GameLauncher.cs
// 游戏启动总入口（挂到 Bootstrap 场景的第一个 GameObject）
//
// 启动流程：
//
//   [原生平台 iOS / Android / PC]
//   Step 1  YooAsset Phase 1：初始化本地文件系统（内置包 or 缓存，不访问网络）
//   Step 2  从本地加载正式 LoadingUI prefab 并显示（内置版 or 上次缓存的最新版）
//   Step 3  YooAsset Phase 2：请求版本号 → 更新 Manifest → 下载缺失 bundle（进度显示）
//   Step 4  xLua 启动，执行 main.lua
//   Step 5  跳转主场景
//
//   [WebGL / 微信小游戏]
//   Step 1  极简 BootstrapLoadingUI 立即显示（纯代码生成，零资源依赖，秒显示）
//   Step 2  YooAsset Phase 1：初始化远端文件系统
//   Step 3  从 CDN 加载正式 LoadingUI bundle，替换极简 UI（WaitForEndOfFrame 避免闪烁）
//   Step 4  YooAsset Phase 2：下载缺失 bundle（进度显示）
//   Step 5  xLua 启动，执行 main.lua
//   Step 6  跳转主场景
//
//   再次启动（所有平台）：
//     Phase 1 读缓存/内置，Phase 2 只下本次差量
//
// 挂载要求：
//   - 同一 GameObject 上需有 YooAssetInitializer、LuaManager
//   - Execution Order 设为 -100（早于一切游戏逻辑）

using System.Collections;
using UnityEngine;
using UnityEngine.SceneManagement;
using YooAsset;

namespace CutRope.Framework
{
    [DefaultExecutionOrder(-100)]
    [RequireComponent(typeof(FileLoggger))]
    [RequireComponent(typeof(YooAssetInitializer))]
    [RequireComponent(typeof(LuaManager))]
    public class GameLauncher : MonoBehaviour
    {
        // ── Inspector 配置 ────────────────────────────────────────────
        [Header("Loading UI")]
        [Tooltip("LoadingUI prefab 在主包中的地址（YooAsset address）")]
        public string loadingUiAddress = "Assets/UI/LoadingUI.prefab";

        [Header("场景")]
        [Tooltip("所有资源就绪后跳转的游戏场景")]
        public string mainSceneName = "Main";

        [Header("调试")]
        [Tooltip("勾选后跳过场景跳转，方便在 Bootstrap 场景直接测试")]
        public bool skipSceneLoad = false;

        // ── 私有引用 ─────────────────────────────────────────────────
        private YooAssetInitializer _yooInit;
        private LuaManager          _luaMgr;

        // WebGL 专用极简兜底 UI（原生平台不创建）
        private BootstrapLoadingUI _bootstrapUI;

        // 正式 LoadingUI（原生平台从 Step 1 后立即加载，WebGL 在 Step 3 替换极简 UI）
        private ILoadingUI _loadingUI;

        // 当前活跃 UI
        private ILoadingUI ActiveUI => (_loadingUI != null)
            ? _loadingUI
            : (_bootstrapUI as ILoadingUI);

        // ── 生命周期 ─────────────────────────────────────────────────
        private void Awake()
        {
            _yooInit = GetComponent<YooAssetInitializer>();
            _luaMgr  = GetComponent<LuaManager>();
        }

        private void Start()
        {
            Debug.Log("[GameLauncher] Starting...");
#if UNITY_WEBGL && !UNITY_EDITOR
            StartCoroutine(LaunchWebGL());
#else
            StartCoroutine(LaunchNative());
#endif
        }

        // ════════════════════════════════════════════════════════════
        // 原生平台启动流程（iOS / Android / PC / Editor）
        // ════════════════════════════════════════════════════════════
        private IEnumerator LaunchNative()
        {
            // Step 1：本地初始化（内置包 or 缓存，不访问网络）
            yield return WaitForLocalReady();
            if (_yooInit.IsLocalReady == false) yield break; // 失败由回调处理

            // Step 2：从本地加载正式 LoadingUI 并显示
            yield return LoadAndShowFormalUI(initialProgress: 0.1f, label: "加载界面...");

            if (_loadingUI == null)
            {
                // 本地加载失败（资源配置错误），用极简 UI 兜底继续
                Debug.LogWarning("[GameLauncher] Native: LoadingUI load failed, fallback to bootstrap UI");
                ShowBootstrapUI(0.1f, "加载界面失败，继续...");
            }

            // Step 3：检查更新 + 下载差量
            yield return WaitForDownloadDone();
            if (!_yooInit.IsDefaultPackageReady) yield break;

            // Step 4 & 5：Lua + 场景
            yield return FinishLaunch();
        }

        // ════════════════════════════════════════════════════════════
        // WebGL 启动流程
        // ════════════════════════════════════════════════════════════
        private IEnumerator LaunchWebGL()
        {
            // Step 1：极简 UI 立即显示（零资源依赖）
            ShowBootstrapUI(0f, "初始化...");

            // Step 2：初始化远端文件系统（不下载资源）
            yield return WaitForLocalReady();
            if (_yooInit.IsLocalReady == false) yield break;

            _bootstrapUI?.SetProgress(0.08f, "加载界面...");

            // Step 3：从 CDN 下载并加载正式 LoadingUI bundle，替换极简 UI
            yield return LoadAndShowFormalUI(initialProgress: 0.1f, label: "加载界面...");

            if (_loadingUI != null)
            {
                // 正式 UI 已渲染一帧，安全销毁极简 UI
                _bootstrapUI?.Hide();
                _bootstrapUI = null;
            }
            // 若正式 UI 加载失败，继续用极简 UI（网络差时的兜底）

            // Step 4：检查更新 + 下载差量
            yield return WaitForDownloadDone();
            if (!_yooInit.IsDefaultPackageReady) yield break;

            // Step 5 & 6：Lua + 场景
            yield return FinishLaunch();
        }

        // ════════════════════════════════════════════════════════════
        // 共用步骤
        // ════════════════════════════════════════════════════════════

        /// <summary>等待 YooAsset Phase 1 本地初始化完成</summary>
        private IEnumerator WaitForLocalReady()
        {
            bool ready = false, failed = false;
            string err = "";

            _yooInit.OnLocalReady += () => ready = true;
            _yooInit.OnInitFailed += e => { failed = true; err = e; };
            _yooInit.InitLocal();

            while (!ready && !failed) yield return null;

            if (failed)
            {
                var msg = $"初始化失败\n{err}";
                if (ActiveUI != null)
                    ActiveUI.ShowError(msg, () => StartCoroutine(LaunchSequenceRetry()));
                else
                    Debug.LogError($"[GameLauncher] {msg}");
            }
        }

        /// <summary>从 YooAsset 本地包加载正式 LoadingUI，Show 后等一帧确保渲染</summary>
        private IEnumerator LoadAndShowFormalUI(float initialProgress, string label)
        {
            if (string.IsNullOrEmpty(loadingUiAddress)) yield break;

            var handle = YooAssets.LoadAssetAsync<GameObject>(loadingUiAddress);
            yield return handle;

            if (handle.Status != EOperationStatus.Succeed)
            {
                Debug.LogWarning($"[GameLauncher] LoadingUI load failed: {handle.LastError}");
                yield break;
            }

            var go = Instantiate(handle.AssetObject as GameObject);
            DontDestroyOnLoad(go);
            var ui = go.GetComponent<ILoadingUI>();

            if (ui == null)
            {
                Debug.LogWarning("[GameLauncher] LoadingUI prefab 缺少 ILoadingUI 组件");
                Destroy(go);
                yield break;
            }

            // 带初始进度显示，避免第一帧从 0 闪烁
            ui.Show(initialProgress, label);
            _loadingUI = ui;

            // 等一帧确保正式 UI 已渲染，此后销毁极简 UI 不会出现黑帧
            yield return new WaitForEndOfFrame();
        }

        /// <summary>等待 YooAsset Phase 2 下载完成</summary>
        private IEnumerator WaitForDownloadDone()
        {
            bool done = false, failed = false;
            string err = "";

            _yooInit.OnDefaultPackageReady += () => done = true;
            _yooInit.OnInitFailed          += e => { failed = true; err = e; };
            _yooInit.OnDownloadProgress    += OnDownloadProgress;

            ActiveUI?.SetProgress(0.15f, "检查更新...");
            _yooInit.CheckAndDownload();

            while (!done && !failed) yield return null;

            if (failed)
                ActiveUI?.ShowError($"资源下载失败\n{err}",
                    () => StartCoroutine(RetryDownload()));
        }

        /// <summary>Lua 启动 + 进入主场景</summary>
        private IEnumerator FinishLaunch()
        {
            ActiveUI?.SetProgress(0.95f, "启动游戏...");
            yield return _luaMgr.StartLuaAsync();

            ActiveUI?.SetProgress(1f, "完成！");
            yield return new WaitForSeconds(0.2f);
            ActiveUI?.Hide();

            if (!skipSceneLoad && !string.IsNullOrEmpty(mainSceneName))
            {
                Debug.Log($"[GameLauncher] Loading scene: {mainSceneName}");
                SceneManager.LoadScene(mainSceneName);
            }
        }

        // ── 下载进度回调 ──────────────────────────────────────────────
        private void OnDownloadProgress(float progress)
        {
            // 下载进度映射到 15%~90%
            ActiveUI?.SetProgress(0.15f + progress * 0.75f,
                progress > 0f ? $"下载资源 {progress * 100:F0}%" : "检查更新...");
        }

        // ── 极简 UI 工具方法 ──────────────────────────────────────────
        private void ShowBootstrapUI(float initialProgress, string label)
        {
            if (_bootstrapUI != null) return;
            var go = new GameObject("BootstrapLoadingUI");
            DontDestroyOnLoad(go);
            _bootstrapUI = go.AddComponent<BootstrapLoadingUI>();
            _bootstrapUI.Show(initialProgress, label);
        }

        // ── 重试 ─────────────────────────────────────────────────────
        private IEnumerator RetryDownload()
        {
            ActiveUI?.SetProgress(0.15f, "重试中...");

            bool done = false, failed = false;
            string err = "";

            _yooInit.OnDefaultPackageReady += () => done = true;
            _yooInit.OnInitFailed          += e => { failed = true; err = e; };

            _yooInit.CheckAndDownload();
            while (!done && !failed) yield return null;

            if (failed)
            {
                ActiveUI?.ShowError($"资源下载失败\n{err}",
                    () => StartCoroutine(RetryDownload()));
                yield break;
            }
            yield return FinishLaunch();
        }

        private IEnumerator LaunchSequenceRetry()
        {
#if UNITY_WEBGL && !UNITY_EDITOR
            yield return LaunchWebGL();
#else
            yield return LaunchNative();
#endif
        }
    }
}
