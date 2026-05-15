// GameLauncherNew.cs
// 游戏启动总入口（双 LuaVM 架构）
//
//   Phase 1：YooAssetInitializer.LaunchInitUpdateStage()  — 本地资源初始化
//   Phase 2：YooAssetInitializer.LaunchUpdateStage()      — 弱联网更新
//   Phase 3：LuaManager.RunUpdateLuaVM("UpdateLogic")     — 临时 VM 跑更新逻辑
//   Phase 4：LuaManager.RunGameLuaVM("main")              — 正式 VM 跑游戏

using Cysharp.Threading.Tasks;
using UnityEngine;

namespace CutRope.Framework
{
    [DefaultExecutionOrder(-100)]
    [RequireComponent(typeof(YooAssetInitializerNew))]
    [RequireComponent(typeof(LuaManagerNew))]
    public class GameLauncherNew : MonoBehaviour
    {
        [Header("配置")]
        public string packageName     = "DefaultPackage";

        [Tooltip("更新阶段 Lua 入口（require 路径，无 .lua 后缀）")]
        public string updateLuaEntry  = "UpdateLogic";

        [Tooltip("游戏阶段 Lua 入口（require 路径，无 .lua 后缀）")]
        public string gameLuaEntry    = "main";

        // ── 私有引用 ─────────────────────────────────────────────────
        private YooAssetInitializerNew _yooInit;
        private LuaManagerNew          _lua;

        private void Awake()
        {
            _yooInit = GetComponent<YooAssetInitializerNew>();
            _lua     = GetComponent<LuaManagerNew>();

            _yooInit.OnDownloadProgress += OnDownloadProgress;
        }

        private void OnDestroy()
        {
            if (_yooInit != null)
                _yooInit.OnDownloadProgress -= OnDownloadProgress;
        }

        private async void Start()
        {
            Debug.Log("[GameLauncher] === Phase 1: local init ===");
            if (!await _yooInit.LaunchInitUpdateStage(packageName))
            {
                Debug.LogError("[GameLauncher] Phase 1 failed, abort.");
                return;
            }

            Debug.Log("[GameLauncher] === Phase 2: weak-network update ===");
            if (!await _yooInit.LaunchUpdateStage(packageName))
            {
                Debug.LogError("[GameLauncher] Phase 2 failed, abort.");
                return;
            }

            Debug.Log("[GameLauncher] === Phase 3: update Lua VM ===");
            if (!await _lua.RunUpdateLuaVM(updateLuaEntry))
            {
                Debug.LogError("[GameLauncher] Phase 3 failed, abort.");
                return;
            }

            Debug.Log("[GameLauncher] === Phase 4: game Lua VM ===");
            if (!await _lua.RunGameLuaVM(gameLuaEntry))
            {
                Debug.LogError("[GameLauncher] Phase 4 failed, abort.");
                return;
            }

            Debug.Log("[GameLauncher] All phases complete ✓");
        }

        // ── 下载进度回调（对接 LoadingUI）────────────────────────────
        private void OnDownloadProgress(float p)
        {
            Debug.Log($"[GameLauncher] download: {p:P1}");
        }
    }
}
