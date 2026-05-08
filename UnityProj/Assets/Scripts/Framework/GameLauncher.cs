// GameLauncher.cs
// 游戏启动总入口（挂到 Bootstrap 场景的第一个 GameObject）
//
// 启动顺序：
//   1. YooAsset 初始化（主包）
//   2. xLua 启动，执行 main.lua
//   3. 加载游戏主场景（Main）
//
// 挂载要求：
//   - 同一 GameObject 上需有 YooAssetInitializer、LuaManager
//   - Execution Order 设为 -100（早于一切游戏逻辑）

using System.Collections;
using UnityEngine;
using UnityEngine.SceneManagement;

namespace CutRope.Framework
{
    [DefaultExecutionOrder(-100)]
    [RequireComponent(typeof(YooAssetInitializer))]
    [RequireComponent(typeof(LuaManager))]
    public class GameLauncher : MonoBehaviour
    {
        [Header("场景名称")]
        [Tooltip("主包就绪后跳转的游戏场景")]
        public string mainSceneName = "Main";

        [Header("调试")]
        [Tooltip("勾选后跳过加载场景，方便直接在 Bootstrap 场景测试")]
        public bool skipSceneLoad = false;

        private YooAssetInitializer _yooInit;
        private LuaManager          _luaMgr;

        private void Awake()
        {
            _yooInit = GetComponent<YooAssetInitializer>();
            _luaMgr  = GetComponent<LuaManager>();

            // 订阅 YooAsset 事件
            _yooInit.OnDefaultPackageReady += OnDefaultPackageReady;
            _yooInit.OnInitFailed          += OnInitFailed;
            _yooInit.OnDownloadProgress    += OnDownloadProgress;
        }

        private void Start()
        {
            Debug.Log("[GameLauncher] Starting...");
            _yooInit.InitDefaultPackage();
        }

        // ── 主包就绪回调 ──────────────────────────────────────────────
        private void OnDefaultPackageReady()
        {
            Debug.Log("[GameLauncher] DefaultPackage ready. Starting Lua...");
            StartCoroutine(StartLuaAndLoadScene());
        }

        private IEnumerator StartLuaAndLoadScene()
        {
            // 异步启动 Lua（从 YooAsset 加载 main.lua）
            yield return _luaMgr.StartLuaAsync();

            // 跳转主场景
            if (!skipSceneLoad && !string.IsNullOrEmpty(mainSceneName))
            {
                Debug.Log($"[GameLauncher] Loading scene: {mainSceneName}");
                SceneManager.LoadScene(mainSceneName);
            }
        }

        // ── 错误处理 ─────────────────────────────────────────────────
        private void OnInitFailed(string error)
        {
            Debug.LogError($"[GameLauncher] Init failed: {error}");
            // TODO: 显示错误 UI，提示玩家重试或联系客服
        }

        // ── 下载进度 ─────────────────────────────────────────────────
        private void OnDownloadProgress(float progress)
        {
            // TODO: 在这里更新 Loading UI 进度条
            Debug.Log($"[GameLauncher] Download progress: {progress * 100:F0}%");
        }
    }
}
