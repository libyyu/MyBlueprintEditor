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

using BlueprintRuntime;
using Cysharp.Threading.Tasks;
using UnityEngine;
using UnityEngine.UIElements;

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

        // ── UI Toolkit PanelSettings ─────────────────────────────────
        [Header("UI Toolkit PanelSettings")]
        [Tooltip("游戏内常规面板（HUD / 主菜单 / 关卡选择等），sortingOrder 10~50000")]
        public PanelSettings uitkGameSettings;

        [Tooltip("全局遮罩 / 系统弹窗（加载、错误提示等），sortingOrder 90000+")]
        public PanelSettings uitkOverlaySettings;

        /// <summary>
        /// 静态单例，供 Lua 和 UITKPanelBackend 通过
        /// CS.CutRope.Framework.GameLauncher.Instance 访问 PanelSettings。
        /// </summary>
        public static GameLauncher Instance { get; private set; }

        /// <summary>
        /// 根据 sortingOrder 自动返回对应的 PanelSettings。
        /// sortingOrder >= 90000 → uitkOverlaySettings；其他 → uitkGameSettings。
        /// 任一为 null 时降级使用另一个（均为 null 则返回 null 让 Unity 用全局默认）。
        /// </summary>
        public PanelSettings GetPanelSettingsForOrder(int sortingOrder)
        {
            var target = sortingOrder >= 90000 ? uitkOverlaySettings : uitkGameSettings;
            return target != null ? target
                 : (uitkGameSettings ?? uitkOverlaySettings);
        }

        // ── 私有引用 ─────────────────────────────────────────────────
        private YooAssetInitializer _yooInit;
        private LuaManager          _lua;

        private void Awake()
        {
            Instance = this;
            _yooInit = GetComponent<YooAssetInitializer>();
            _lua     = GetComponent<LuaManager>();
        }

        private void OnDestroy()
        {
            if (Instance == this) Instance = null;
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
