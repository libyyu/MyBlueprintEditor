// GameLauncher.cs
// 游戏启动总入口（挂到 Bootstrap 场景的第一个 GameObject）
//
// 两阶段启动流程：
//
//   [Stage 1] 极简内置 Loading UI（纯代码生成，零资源依赖）
//     → YooAsset 初始化 LoadingUIPackage（轻量包，仅含 LoadingUI prefab）
//     → 加载最新 LoadingUI prefab，替换极简 UI
//
//   [Stage 2] 正式 Loading UI 接管
//     → YooAsset 初始化 DefaultPackage（主包，含游戏逻辑资源）
//     → 下载缺失资源（进度实时更新到 Loading UI）
//     → xLua 启动，执行 main.lua
//     → 跳转主场景
//
// 为什么两阶段？
//   WebGL 下无法在 YooAsset 初始化之前加载 AssetBundle，
//   但玩家需要即时看到进度反馈。Stage 1 用代码生成极简 UI 兜底，
//   Stage 2 换成可热更的正式 UI。全平台统一，无特殊分支。
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
        [Header("包名配置")]
        [Tooltip("Loading UI 专用轻量包（仅含 LoadingUI prefab，尽可能小）")]
        public string loadingUiPackageName = "LoadingUIPackage";

        [Tooltip("Loading UI prefab 在 LoadingUIPackage 中的地址")]
        public string loadingUiAddress = "Assets/UI/LoadingUI.prefab";

        [Tooltip("主包就绪后跳转的游戏场景")]
        public string mainSceneName = "Main";

        [Header("调试")]
        [Tooltip("勾选后跳过场景跳转，方便在 Bootstrap 场景直接测试")]
        public bool skipSceneLoad = false;

        // ── 私有引用 ─────────────────────────────────────────────────
        private YooAssetInitializer _yooInit;
        private LuaManager          _luaMgr;

        // Stage 1：极简内置 Loading UI（纯代码生成）
        private BootstrapLoadingUI _bootstrapUI;

        // Stage 2：从 YooAsset 加载的正式 Loading UI
        // 约定接口：挂有实现 ILoadingUI 的组件（或直接用鸭子类型反射）
        private ILoadingUI _loadingUI;

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
            // ── Stage 1：显示极简内置 Loading UI ─────────────────────
            _bootstrapUI = new GameObject("BootstrapLoadingUI").AddComponent<BootstrapLoadingUI>();
            DontDestroyOnLoad(_bootstrapUI.gameObject);
            _bootstrapUI.Show();
            _bootstrapUI.SetProgress(0f, "初始化...");

            // ── Stage 1a：初始化 LoadingUIPackage ────────────────────
            bool loadingUiReady  = false;
            bool loadingUiFailed = false;
            string loadingUiErr  = "";

            _yooInit.InitDlcPackage(
                loadingUiPackageName,
                onReady:  () => loadingUiReady  = true,
                onFailed: e  => { loadingUiFailed = true; loadingUiErr = e; }
            );

            // 等待 LoadingUIPackage 就绪（进度 0→30%）
            float waitTimer = 0f;
            while (!loadingUiReady && !loadingUiFailed)
            {
                waitTimer += Time.deltaTime;
                // 用时间模拟假进度，最多推到 28%，避免卡在 0
                _bootstrapUI.SetProgress(Mathf.Min(waitTimer / 5f * 0.28f, 0.28f), "加载界面资源...");
                yield return null;
            }

            if (loadingUiFailed)
            {
                // LoadingUIPackage 失败：继续用极简 UI 走主包流程
                Debug.LogWarning($"[GameLauncher] LoadingUIPackage failed: {loadingUiErr}, using bootstrap UI");
            }
            else
            {
                // ── Stage 1b：从 LoadingUIPackage 加载正式 Loading UI ──
                _bootstrapUI.SetProgress(0.3f, "加载界面...");
                yield return LoadFormalLoadingUI();
            }

            // ── Stage 2：初始化主包 ───────────────────────────────────
            ActiveLoadingUI.SetProgress(0.3f, "检查更新...");

            bool defaultReady  = false;
            bool defaultFailed = false;
            string defaultErr  = "";

            _yooInit.OnDefaultPackageReady += () => defaultReady  = true;
            _yooInit.OnInitFailed          += e  => { defaultFailed = true; defaultErr = e; };
            _yooInit.OnDownloadProgress    += OnDownloadProgress;

            _yooInit.InitDefaultPackage();

            while (!defaultReady && !defaultFailed)
                yield return null;

            if (defaultFailed)
            {
                ActiveLoadingUI.ShowError(
                    $"资源加载失败\n{defaultErr}",
                    onRetry: () => StartCoroutine(RetryDefaultPackage())
                );
                yield break;
            }

            // ── Stage 3：启动 Lua → 进入主场景 ───────────────────────
            ActiveLoadingUI.SetProgress(0.95f, "启动游戏...");
            yield return _luaMgr.StartLuaAsync();

            ActiveLoadingUI.SetProgress(1f, "完成！");
            yield return new WaitForSeconds(0.2f);   // 短暂停留让玩家看到 100%

            ActiveLoadingUI.Hide();

            if (!skipSceneLoad && !string.IsNullOrEmpty(mainSceneName))
            {
                Debug.Log($"[GameLauncher] Loading scene: {mainSceneName}");
                SceneManager.LoadScene(mainSceneName);
            }
        }

        // ── 加载正式 Loading UI ───────────────────────────────────────
        private IEnumerator LoadFormalLoadingUI()
        {
            var package = YooAssets.GetPackage(loadingUiPackageName);
            if (package == null) yield break;

            var handle = package.LoadAssetAsync<GameObject>(loadingUiAddress);
            yield return handle;

            if (handle.Status != EOperationStatus.Succeed)
            {
                Debug.LogWarning($"[GameLauncher] LoadingUI prefab load failed: {handle.LastError}");
                yield break;
            }

            var go = Instantiate(handle.AssetObject as GameObject);
            DontDestroyOnLoad(go);
            _loadingUI = go.GetComponent<ILoadingUI>();

            if (_loadingUI == null)
            {
                Debug.LogWarning("[GameLauncher] LoadingUI prefab 缺少 ILoadingUI 组件，继续用极简 UI");
                Destroy(go);
                yield break;
            }

            // 正式 UI 就绪，销毁极简 UI
            _bootstrapUI.Hide();
            _bootstrapUI = null;
            _loadingUI.Show();
            _loadingUI.SetProgress(0.3f, "检查更新...");
            Debug.Log("[GameLauncher] Formal LoadingUI activated");
        }

        // ── 下载进度回调（Stage 2 主包下载） ─────────────────────────
        private void OnDownloadProgress(float progress)
        {
            // 主包下载进度映射到 30%~90%
            float mapped = 0.3f + progress * 0.6f;
            ActiveLoadingUI.SetProgress(mapped, $"下载资源 {progress * 100:F0}%");
        }

        // ── 重试主包（用户点击重试后） ────────────────────────────────
        private IEnumerator RetryDefaultPackage()
        {
            bool ready = false, failed = false;
            string err = "";

            _yooInit.OnDefaultPackageReady += () => ready  = true;
            _yooInit.OnInitFailed          += e  => { failed = true; err = e; };

            _yooInit.InitDefaultPackage();
            while (!ready && !failed) yield return null;

            if (failed)
            {
                ActiveLoadingUI.ShowError($"资源加载失败\n{err}", () => StartCoroutine(RetryDefaultPackage()));
                yield break;
            }

            ActiveLoadingUI.SetProgress(0.95f, "启动游戏...");
            yield return _luaMgr.StartLuaAsync();
            ActiveLoadingUI.SetProgress(1f, "完成！");
            yield return new WaitForSeconds(0.2f);
            ActiveLoadingUI.Hide();

            if (!skipSceneLoad && !string.IsNullOrEmpty(mainSceneName))
                SceneManager.LoadScene(mainSceneName);
        }

        // ── 统一访问当前活跃的 Loading UI ────────────────────────────
        private ILoadingUI ActiveLoadingUI =>
            (_loadingUI != null) ? _loadingUI : _bootstrapUI;
    }
}
