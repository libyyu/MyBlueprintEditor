// GameLauncher.cs
// 游戏启动总入口（xLua 主 VM 架构）
//
//   Phase 1：YooAssetInitializer.LaunchInitUpdateStage()  — 本地资源库初始化
//   Phase 2：LuaManager.PreloadAllScript()               — 预加载所有 Lua 文件
//   Phase 3：LuaManager.RunUpdateLuaVM("UpdateLogic")    — 临时 VM 跑更新逻辑
//   Phase 4：LuaManager.RunGameLuaVM("GameLogic")        — xLua 自建主 VM
//              内部：new LuaEnv() → rawL 注入 BlueprintRunner → require 'GameLogic'
//
// VM 主从关系：xLua 是主，BlueprintRuntime 共享 xLua 的 lua_State（不拥有）。

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
        public string packageName    = "DefaultPackage";

        [Tooltip("更新阶段 Lua 入口（require 路径，无 .lua 后缀）")]
        public string updateLuaEntry = "UpdateLogic";

        [Tooltip("游戏阶段 Lua 入口（require 路径，无 .lua 后缀）")]
        public string gameLuaEntry   = "GameLogic";

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

            // ── Phase 2：预加载 Lua 文件 ──────────────────────────────
            Debug.Log("[GameLauncher] === Phase 2: preload lua ===");
            if (!await _lua.PreloadAllScript(packageName))
            {
                Debug.LogError("[GameLauncher] Phase 2 failed, abort.");
                return;
            }

            // ── Phase 3：UpdateLogic.lua 启动（检查更新）─────────────
            Debug.Log("[GameLauncher] === Phase 3: UpdateLogic VM ===");
            if (!await _lua.RunUpdateLuaVM(updateLuaEntry))
            {
                Debug.LogError("[GameLauncher] Phase 3 failed, abort.");
                return;
            }

            // BR 实例确保存在（Awake 里可能已创建）
            if (BlueprintRunner.Instance == null)
                gameObject.AddComponent<BlueprintRunner>();

            // AudioManager
            if (AudioManager.Instance == null)
                gameObject.AddComponent<AudioManager>();

            // ── Phase 4：GameLogic.lua 启动（xLua 为主 VM）───────────
            // 内部顺序：
            //   1. xLua new LuaEnv()（自建 lua_State，xLua 拥有生命周期）
            //   2. rawL 注入 BlueprintRunner.InitWithExternalLuaState()
            //   3. require 'GameLogic'（此时 Blueprint.* 全局表已就绪）
            Debug.Log("[GameLauncher] === Phase 4: GameLogic VM (xLua master) ===");
            if (!await _lua.RunGameLuaVM(gameLuaEntry))
            {
                Debug.LogError("[GameLauncher] Phase 4 failed, abort.");
                return;
            }

            Debug.Log("[GameLauncher] All phases complete ✓");
        }
    }
}
