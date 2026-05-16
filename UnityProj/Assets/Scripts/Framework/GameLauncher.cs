// GameLauncherNew.cs
// 游戏启动总入口（双 LuaVM 架构）
//
//   Phase 1：YooAssetInitializerNew.LaunchInitUpdateStage()  — 本地资源库初始化
//   Phase 2：LuaManagerNew.RunUpdateLuaVM("UpdateLogic")     — 临时 VM 跑更新逻辑
//                                                            （Lua 内部读内置资源 + 检查更新）
//   Phase 3：LuaManagerNew.RunGameLuaVM("GameLogic")         — 正式 VM 跑游戏逻辑

using Cysharp.Threading.Tasks;
using UnityEngine;

namespace CutRope.Framework
{
    [DefaultExecutionOrder(-100)]
    [RequireComponent(typeof(YooAssetInitializer))]
    [RequireComponent(typeof(LuaManager))]
    [RequireComponent(typeof(SceneLoader))]
    [RequireComponent(typeof(FTimerListBehavior))]
    public class GameLauncher : MonoBehaviour
    {
        [Header("配置")]
        public string packageName     = "DefaultPackage";

        [Tooltip("更新阶段 Lua 入口（require 路径，无 .lua 后缀）")]
        public string updateLuaEntry  = "UpdateLogic";

        [Tooltip("游戏阶段 Lua 入口（require 路径，无 .lua 后缀）")]
        public string gameLuaEntry    = "GameLogic";

        // ── 私有引用 ─────────────────────────────────────────────────
        private YooAssetInitializer _yooInit;
        private LuaManager          _lua;

        private void Awake()
        {
            _yooInit = GetComponent<YooAssetInitializer>();
            _lua     = GetComponent<LuaManager>();
        }

        private async void Start()
        {
            // ── Phase 1：本地资源库初始化 ─────────────────────────────
            Debug.Log("[GameLauncher] === Phase 1: local init ===");
            if (!await _yooInit.LaunchInitUpdateStage(packageName))
            {
                Debug.LogError("[GameLauncher] Phase 1 failed, abort.");
                return;
            }

            Debug.Log("[GameLauncher] === Phase 2: preload lua ===");
#if !UNITY_EDITOR && UNITY_WEBGL
            if (!await _lua.PreloadAllScript(packageName))
            {
                Debug.LogError("[GameLauncher] Phase 2 failed, abort.");
                return;
            }
#endif

            // ── Phase 3：UpdateLogic.lua 启动（Lua 内部检查更新+下载）
            Debug.Log("[GameLauncher] === Phase 3: UpdateLogic VM ===");
            if (!await _lua.RunUpdateLuaVM(updateLuaEntry))
            {
                Debug.LogError("[GameLauncher] Phase 3 failed, abort.");
                return;
            }

            var bpRuntime = BlueprintRuntime.Instance
                         ?? gameObject.AddComponent<BlueprintRuntime>();
            bpRuntime.Init();

            // ── Phase 4：GameLogic.lua 启动（游戏正式开始）─────────
            Debug.Log("[GameLauncher] === Phase 4: GameLogic VM ===");
            if (!await _lua.RunGameLuaVM(gameLuaEntry))
            {
                Debug.LogError("[GameLauncher] Phase 4 failed, abort.");
                return;
            }

            Debug.Log("[GameLauncher] All phases complete ✓");
        }
    }
}
