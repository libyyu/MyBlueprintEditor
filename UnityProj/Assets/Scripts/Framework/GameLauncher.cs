// GameLauncher.cs
// 游戏启动总入口（挂到 Bootstrap 场景的第一个 GameObject）
//
// 启动流程：
//
//   Step 1  显示极简内置 Loading UI（纯代码生成，零资源依赖，秒显示）
//   Step 2  YooAsset Phase 1：初始化本地文件系统（内置包 or 缓存，不访问网络）
//   Step 3  从本地加载正式 LoadingUI prefab 替换极简 UI（内置版 or 上次缓存的最新版）
//   Step 4  YooAsset Phase 2：请求版本号 → 更新 Manifest → 下载缺失 bundle
//             进度实时显示在 Loading UI 上；无需下载时直接跳过
//   Step 5  xLua 启动，执行 main.lua
//   Step 6  跳转主场景
//
//   再次启动：Step 2 读缓存（上次下好的最新版），Step 3 显示最新 LoadingUI，
//             Step 4 只下本次有差量的部分
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

        private BootstrapLoadingUI _bootstrapUI;   // 极简兜底 UI（Step 1）
        private ILoadingUI         _formalUI;      // 正式 LoadingUI（Step 3，可为空）

        // 当前活跃 UI（优先 formalUI，否则 bootstrapUI）
        private ILoadingUI ActiveUI => (_formalUI != null) ? _formalUI : _bootstrapUI;

        // ── 生命周期 ─────────────────────────────────────────────────
        private void Awake()
        {
            _yooInit = GetComponent<YooAssetInitializer>();
            _luaMgr  = GetComponent<LuaManager>();
        }

        private void Start()
        {
            Debug.Log("[GameLauncher] Starting...");
            StartCoroutine(LaunchSequence());
        }

        // ── 主启动协程 ────────────────────────────────────────────────
        private IEnumerator LaunchSequence()
        {
            // ── Step 1：极简 Loading UI 立即显示 ─────────────────────
            var uiGo = new GameObject("BootstrapLoadingUI");
            DontDestroyOnLoad(uiGo);
            _bootstrapUI = uiGo.AddComponent<BootstrapLoadingUI>();
            _bootstrapUI.Show();
            _bootstrapUI.SetProgress(0f, "初始化...");

            // ── Step 2：YooAsset Phase 1（本地，不访问网络） ──────────
            bool localReady  = false;
            bool localFailed = false;
            string localErr  = "";

            _yooInit.OnLocalReady  += () => localReady  = true;
            _yooInit.OnInitFailed  += e  => { localFailed = true; localErr = e; };

            _yooInit.InitLocal();
            while (!localReady && !localFailed) yield return null;

            if (localFailed)
            {
                // 本地初始化失败（极少发生），直接报错等待重试
                _bootstrapUI.ShowError($"初始化失败\n{localErr}",
                    () => StartCoroutine(LaunchSequence()));
                yield break;
            }

            _bootstrapUI.SetProgress(0.1f, "加载界面...");

            // ── Step 3：从本地（内置/缓存）加载正式 LoadingUI ─────────
            // 此时 YooAsset 已可读取本地 bundle，不需要网络
            yield return LoadFormalLoadingUI();
            // 无论成功与否，ActiveUI 都已指向当前最优 UI

            ActiveUI.SetProgress(0.15f, "检查更新...");

            // ── Step 4：YooAsset Phase 2（检查 + 下载差量） ───────────
            bool downloadDone   = false;
            bool downloadFailed = false;
            string downloadErr  = "";

            _yooInit.OnDefaultPackageReady += () => downloadDone   = true;
            // OnInitFailed 已在 Step 2 订阅，追加 download 判断
            _yooInit.OnInitFailed          += e  => { downloadFailed = true; downloadErr = e; };
            _yooInit.OnDownloadProgress    += OnDownloadProgress;

            _yooInit.CheckAndDownload();
            while (!downloadDone && !downloadFailed) yield return null;

            if (downloadFailed)
            {
                ActiveUI.ShowError($"资源下载失败\n{downloadErr}",
                    () => StartCoroutine(RetryDownload()));
                yield break;
            }

            // ── Step 5：启动 Lua ──────────────────────────────────────
            ActiveUI.SetProgress(0.95f, "启动游戏...");
            yield return _luaMgr.StartLuaAsync();

            // ── Step 6：进入主场景 ────────────────────────────────────
            ActiveUI.SetProgress(1f, "完成！");
            yield return new WaitForSeconds(0.2f);
            ActiveUI.Hide();

            if (!skipSceneLoad && !string.IsNullOrEmpty(mainSceneName))
            {
                Debug.Log($"[GameLauncher] Loading scene: {mainSceneName}");
                SceneManager.LoadScene(mainSceneName);
            }
        }

        // ── 加载正式 Loading UI（从本地包，无网络） ───────────────────
        private IEnumerator LoadFormalLoadingUI()
        {
            if (string.IsNullOrEmpty(loadingUiAddress)) yield break;

            var handle = YooAssets.LoadAssetAsync<GameObject>(loadingUiAddress);
            yield return handle;

            if (handle.Status != EOperationStatus.Succeed)
            {
                Debug.LogWarning($"[GameLauncher] LoadingUI load failed: {handle.LastError}，继续用极简 UI");
                yield break;
            }

            var go = Instantiate(handle.AssetObject as GameObject);
            DontDestroyOnLoad(go);
            _formalUI = go.GetComponent<ILoadingUI>();

            if (_formalUI == null)
            {
                Debug.LogWarning("[GameLauncher] LoadingUI prefab 缺少 ILoadingUI 组件，继续用极简 UI");
                Destroy(go);
                yield break;
            }

            // 先把极简 UI 的当前进度同步给正式 UI，再显示正式 UI
            // 注意顺序：Show(含初始进度) → WaitForEndOfFrame → Hide极简UI
            // 确保正式 UI 先渲染一帧再销毁极简 UI，避免闪烁
            float currentProgress = 0.1f;
            _formalUI.Show(currentProgress, "加载界面...");
            yield return new WaitForEndOfFrame();
            _bootstrapUI.Hide();
            _bootstrapUI = null;
            Debug.Log("[GameLauncher] Formal LoadingUI activated");
        }

        // ── 下载进度回调（Phase 2） ───────────────────────────────────
        private void OnDownloadProgress(float progress)
        {
            // 进度映射到 15%~90%（Phase 1 占 0~15%）
            ActiveUI.SetProgress(0.15f + progress * 0.75f,
                progress > 0f ? $"下载资源 {progress * 100:F0}%" : "检查更新...");
        }

        // ── 重试下载 ─────────────────────────────────────────────────
        private IEnumerator RetryDownload()
        {
            ActiveUI.SetProgress(0.15f, "重试中...");

            bool done = false, failed = false;
            string err = "";

            _yooInit.OnDefaultPackageReady += () => done   = true;
            _yooInit.OnInitFailed          += e  => { failed = true; err = e; };

            _yooInit.CheckAndDownload();
            while (!done && !failed) yield return null;

            if (failed)
            {
                ActiveUI.ShowError($"资源下载失败\n{err}",
                    () => StartCoroutine(RetryDownload()));
                yield break;
            }

            ActiveUI.SetProgress(0.95f, "启动游戏...");
            yield return _luaMgr.StartLuaAsync();
            ActiveUI.SetProgress(1f, "完成！");
            yield return new WaitForSeconds(0.2f);
            ActiveUI.Hide();

            if (!skipSceneLoad && !string.IsNullOrEmpty(mainSceneName))
                SceneManager.LoadScene(mainSceneName);
        }
    }
}
