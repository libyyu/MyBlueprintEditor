// GameLauncher.cs
// 启动总入口（xLua 主 VM 架构）。
//
// 【数据驱动 / 无模式分支】
//   GameLauncher 只负责编排不变的启动骨架（Phase 1→4），所有随形态变化的差异
//   （是否更新 / 入口脚本 / PanelSettings 等）全部外置到 LaunchProfile(ScriptableObject)。
//   游戏场景指向 Game.asset，桌宠场景指向 Pet.asset —— 流程中没有任何 if(isPetMode)。
//
//   Phase 1：YooAssetInitializer.LaunchInitUpdateStage()  本地资源库初始化
//   Phase 2：LuaManager.PreloadAllScript()                预加载所有 Lua
//   Phase 3：LuaManager.RunUpdateLuaVM(updateLuaEntry)    更新逻辑（skipUpdate 时跳过）
//   Phase 4：LuaManager.RunGameLuaVM(gameLuaEntry)        xLua 主 VM 跑主逻辑
//
// VM 主从关系：xLua 是主，BlueprintRuntime 共享 xLua 的 lua_State（不拥有）。

using Cysharp.Threading.Tasks;
using UnityEngine;
using UnityEngine.UIElements;
using Debug = UnityEngine.Debug;

namespace UGFramework.Runtime
{
    [DefaultExecutionOrder(-100)]
    [RequireComponent(typeof(YooAssetInitializer))]
    [RequireComponent(typeof(LuaManager))]
    [RequireComponent(typeof(SceneLoader))]
    [RequireComponent(typeof(FTimerListBehavior))]
    public class GameLauncher : MonoBehaviour
    {
        [Header("启动配置（推荐：指定 Launch Profile）")]
        [Tooltip("数据驱动的启动配置。游戏=Game.asset，桌宠=Pet.asset。")]
        public LaunchProfile profile;

        [Header("（兼容）未指定 Profile 时使用以下内联配置")]
        public string packageName    = "DefaultPackage";
        public bool   skipUpdate     = false;
        public string updateLuaEntry = "UpdateLogic";
        public string gameLuaEntry   = "GameLogic";
        public PanelSettings uitkGameSettings;
        public PanelSettings uitkOverlaySettings;

        /// <summary>静态单例，供 Lua / UITKPanelBackend 访问。</summary>
        public static GameLauncher Instance { get; private set; }

        // ── 生效配置（Profile 优先；否则由内联字段构建，保证旧场景不破坏）──
        private LaunchProfile _cfg;
        private LaunchProfile Config
        {
            get
            {
                if (_cfg != null) return _cfg;
                if (profile != null) { _cfg = profile; return _cfg; }

                _cfg = ScriptableObject.CreateInstance<LaunchProfile>();
                _cfg.packageName       = packageName;
                _cfg.skipUpdate        = skipUpdate;
                _cfg.updateLuaEntry    = updateLuaEntry;
                _cfg.gameLuaEntry      = gameLuaEntry;
                _cfg.uitkGameSettings  = uitkGameSettings;
                _cfg.uitkOverlaySettings = uitkOverlaySettings;
                return _cfg;
            }
        }

        /// <summary>按 sortingOrder 返回对应 PanelSettings（委托给生效配置）。</summary>
        public PanelSettings GetPanelSettingsForOrder(int sortingOrder)
            => Config.GetPanelSettingsForOrder(sortingOrder);

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
            var cfg = Config;

            // AudioManager
            if (AudioManager.Instance == null)
                gameObject.AddComponent<AudioManager>();

            // ── Phase 1：本地资源库初始化 ─────────────────────────────
            Debug.Log("[GameLauncher] === Phase 1: local init ===");
            if (!await _yooInit.LaunchInitUpdateStage(cfg.packageName))
            {
                Debug.LogError("[GameLauncher] Phase 1 failed, abort.");
                return;
            }

            // ── Phase 2：预加载 Lua 文件 ──────────────────────────────
            Debug.Log("[GameLauncher] === Phase 2: preload lua ===");
            if (!await _lua.PreloadAllScript(cfg.packageName))
            {
                Debug.LogError("[GameLauncher] Phase 2 failed, abort.");
                return;
            }

            // ── Phase 3：UpdateLogic.lua 启动（skipUpdate=true 时跳过）──
            bool runUpdate = !cfg.skipUpdate;
            Debug.Log($"[GameLauncher] === Phase 3: UpdateLogic VM (run={runUpdate}) ===");
            if (runUpdate)
            {
                if (!await _lua.RunUpdateLuaVM(cfg.updateLuaEntry))
                {
                    Debug.LogError("[GameLauncher] Phase 3 failed, abort.");
                    return;
                }
            }

            // ── Phase 4：主逻辑 VM 启动（xLua 为主 VM）─────────────────
            Debug.Log($"[GameLauncher] === Phase 4: Game VM (xLua master, entry={cfg.gameLuaEntry}) ===");
            if (!await _lua.RunGameLuaVM(cfg.gameLuaEntry))
            {
                Debug.LogError("[GameLauncher] Phase 4 failed, abort.");
                return;
            }

            Debug.Log("[GameLauncher] All phases complete ✓");
        }
    }
}
