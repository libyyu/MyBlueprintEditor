// LuaManagerNew.cs
// xLua 双 VM 调度器（精简版）：
//
//   RunUpdateLuaVM(entry)：起一个临时 LuaEnv 跑更新逻辑（如 UpdateLogic.lua）
//                          脚本设置 _G.UpdateLogicDone = true 或超时（60s）后销毁 VM
//
//   RunGameLuaVM(entry)：起正式游戏 LuaEnv，跑 main.lua / GameLogic.lua
//                        VM 在 OnDestroy 时销毁，Update() 调 _G.onAppTick(dt)
//
// 两个 VM 完全独立：内存隔离、global 隔离、loader 各自管理。
//
// Lua 文件加载：从 YooAsset DefaultPackage 同步加载 TextAsset，路径形如
//   require "main"        → Assets/Lua/main.lua
//   require "game/util"   → Assets/Lua/game/util.lua

using System;
using System.IO;
using UnityEngine;
using Cysharp.Threading.Tasks;
using XLua;
using YooAsset;

namespace CutRope.Framework
{
    public class LuaManagerNew : MonoBehaviour
    {
        [Header("Lua 配置")]
        [Tooltip("GC 间隔（秒）")]
        public float gcInterval = 1f;

        [Tooltip("Lua 文件在 YooAsset 中的公共前缀，用于 require 路径解析")]
        public string luaAddressPrefix = "Assets/Lua/";

        // ── 公共访问 ─────────────────────────────────────────────────
        public static LuaManagerNew Instance { get; private set; }

        /// <summary>当前活跃的 LuaEnv（更新阶段或游戏阶段）。同一时刻只有一个。</summary>
        public LuaEnv ActiveLuaEnv { get; private set; }

        // ── 私有 ─────────────────────────────────────────────────────
        private float       _gcTimer;
        private LuaFunction _luaUpdate;
        private LuaFunction _luaOnDestroy;

        // ── 生命周期 ─────────────────────────────────────────────────
        private void Awake()
        {
            if (Instance != null && Instance != this)
            {
                Destroy(gameObject);
                return;
            }
            Instance = this;
            DontDestroyOnLoad(gameObject);
        }

        private void Update()
        {
            if (ActiveLuaEnv == null) return;

            _luaUpdate?.Call(Time.deltaTime);

            _gcTimer += Time.deltaTime;
            if (_gcTimer >= gcInterval)
            {
                _gcTimer = 0f;
                ActiveLuaEnv.Tick();
            }
        }

        private void OnDestroy()
        {
            DisposeActiveVM();
            if (Instance == this) Instance = null;
        }

        // ════════════════════════════════════════════════════════════
        // 公共 API
        // ════════════════════════════════════════════════════════════

        /// <summary>
        /// 起一个 LuaVM 跑更新逻辑（如 UpdateLogic.lua）。
        /// 脚本执行完成 / 设置 _G.UpdateLogicDone = true / 异常时返回。
        /// VM 退出后立即销毁，释放内存。
        /// </summary>
        public async UniTask<bool> RunUpdateLuaVM(string entryLua = "UpdateLogic")
        {
            Debug.Log($"[LuaManagerNew] === Update VM start: {entryLua} ===");

            DisposeActiveVM();
            var env = CreateLuaEnv("update");
            ActiveLuaEnv = env;

            try
            {
                env.DoString($"require '{entryLua}'");

                // 等待 Lua 侧标记 _G.UpdateLogicDone = true（最长 60s 兜底）
                float t = 0f;
                while (t < 60f)
                {
                    var done = env.Global.Get<bool>("UpdateLogicDone");
                    if (done) break;
                    await UniTask.Yield();
                    t += Time.deltaTime;
                }

                Debug.Log($"[LuaManagerNew] Update VM done (t={t:F1}s)");
                return true;
            }
            catch (Exception e)
            {
                Debug.LogError($"[LuaManagerNew] Update VM exception: {e}");
                return false;
            }
            finally
            {
                DisposeActiveVM();
            }
        }

        /// <summary>
        /// 起正式游戏 LuaVM 跑入口脚本（如 main.lua）。
        /// VM 持续存活直到 GameObject 销毁，Update() 会调用 _G.onAppTick。
        /// </summary>
        public async UniTask<bool> RunGameLuaVM(string entryLua = "main")
        {
            Debug.Log($"[LuaManagerNew] === Game VM start: {entryLua} ===");

            DisposeActiveVM();
            var env = CreateLuaEnv("game");
            ActiveLuaEnv = env;

            try
            {
                env.DoString($"require '{entryLua}'");

                // 取出生命周期钩子供 Update / OnDestroy 调用
                _luaUpdate    = env.Global.Get<LuaFunction>("onAppTick");
                _luaOnDestroy = env.Global.Get<LuaFunction>("onAppDestroy");

                Debug.Log("[LuaManagerNew] Game VM running");
                await UniTask.Yield();   // 让控制权交还给调用者
                return true;
            }
            catch (Exception e)
            {
                Debug.LogError($"[LuaManagerNew] Game VM exception: {e}");
                DisposeActiveVM();
                return false;
            }
        }

        // ════════════════════════════════════════════════════════════
        // 内部：创建/销毁 VM
        // ════════════════════════════════════════════════════════════

        private LuaEnv CreateLuaEnv(string tag)
        {
            // 每个 VM 都尝试与 BlueprintRuntime 共享 lua_State（如果存在），
            // 否则起一个独立 VM。两个阶段的 VM 是完全独立的实例。
            LuaEnv env;
            var bpRuntime = BlueprintRuntime.Instance;
            if (bpRuntime != null && bpRuntime.LuaState != IntPtr.Zero)
            {
                env = new LuaEnv(bpRuntime.LuaState);
                Debug.Log($"[LuaManagerNew:{tag}] LuaEnv created (shared with BlueprintRuntime)");
            }
            else
            {
                env = new LuaEnv();
                Debug.Log($"[LuaManagerNew:{tag}] LuaEnv created (standalone)");
            }

            env.AddLoader(YooAssetLuaLoader);

            StaticLuaCallbacks.lua_Print   = s => Debug.Log("[Lua]" + s);
            StaticLuaCallbacks.lua_Warning = s => Debug.LogWarning("[Lua]" + s);
            StaticLuaCallbacks.lua_Error   = s => Debug.LogError("[Lua]" + s);

            return env;
        }

        private void DisposeActiveVM()
        {
            if (ActiveLuaEnv == null) return;

            try { _luaOnDestroy?.Call(); } catch (Exception e) { Debug.LogWarning($"[LuaManagerNew] onAppDestroy: {e.Message}"); }

            _luaOnDestroy?.Dispose(); _luaOnDestroy = null;
            _luaUpdate?.Dispose();    _luaUpdate    = null;

            try { ActiveLuaEnv.Dispose(); }
            catch (Exception e) { Debug.LogWarning($"[LuaManagerNew] Dispose: {e.Message}"); }

            ActiveLuaEnv = null;
            _gcTimer = 0f;
            Debug.Log("[LuaManagerNew] LuaEnv disposed");
        }

        // ════════════════════════════════════════════════════════════
        // YooAsset Loader（同步，WebGL 安全）
        // ════════════════════════════════════════════════════════════

        // require "main"      → Assets/Lua/main.lua
        // require "game.util" → Assets/Lua/game/util.lua
        private byte[] YooAssetLuaLoader(ref string luaPath)
        {
            var package = YooAssets.GetPackage("DefaultPackage");
            if (package == null)
            {
                Debug.LogError("[LuaManagerNew] DefaultPackage not initialized");
                return null;
            }

            // 规范化路径：把 . 转 /，添加前缀和扩展名
            string normalized = luaPath.Replace('.', '/');
            string assetPath  = Path.Combine(luaAddressPrefix, normalized + ".lua")
                                    .Replace('\\', '/');

            var handle = package.LoadAssetSync<TextAsset>(assetPath);
            if (handle == null || handle.AssetObject == null)
            {
                Debug.LogWarning($"[LuaManagerNew] Lua not found: '{assetPath}' (require '{luaPath}')");
                return null;
            }

            var bytes = (handle.AssetObject as TextAsset)?.bytes;
            handle.Release();

            // 把实际加载到的路径回写给 xLua（用于错误堆栈）
            luaPath = assetPath;
            return bytes;
        }
    }
}
