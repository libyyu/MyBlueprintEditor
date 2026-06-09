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

using Cysharp.Threading.Tasks;
using System;
using System.IO;
using UnityEngine;
using XLua;
using YooAsset;

namespace UGFramework.Runtime
{
    public class LuaManager : MonoBehaviour
    {
        [Header("Lua 配置")]
        [Tooltip("GC 间隔（秒）")]
        public float gcInterval = 1f;

        [Tooltip("Lua 文件在 YooAsset 中的公共前缀，用于 require 路径解析")]
        public string luaAddressPrefix = "Assets/Lua/";

        // ── 公共访问 ─────────────────────────────────────────────────
        public static LuaManager Instance { get; private set; }

        /// <summary>当前活跃的 LuaEnv（更新阶段或游戏阶段）。同一时刻只有一个。</summary>
        public LuaEnv ActiveLuaEnv { get; private set; }

        public static bool IsLuaValid 
        { 
            get
            {
                if (Instance == null) { return false; }
                return Instance.ActiveLuaEnv != null;
            }
        }

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

            StaticLuaCallbacks.lua_Print = s => Debug.Log($"[{Time.frameCount}][Lua]" + s);
            StaticLuaCallbacks.lua_Warning = s => Debug.LogWarning($"[{Time.frameCount}][Lua]" + s);
            StaticLuaCallbacks.lua_Error = s => Debug.LogError($"[{Time.frameCount}][Lua]" + s);
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
            Debug.Log($"[LuaManager] === Update VM start: {entryLua} ===");

            DisposeActiveVM();
            var env = CreateLuaEnv("update", IntPtr.Zero);
            ActiveLuaEnv = env;

            try
            {
                env.DoString($"require '{entryLua}'");
                // 取出生命周期钩子供 Update / OnDestroy 调用
                _luaUpdate = env.Global.Get<LuaFunction>("onAppTick");
                _luaOnDestroy = env.Global.Get<LuaFunction>("onAppDestroy");
                if(!env.Global.ContainsKey("UpdateLogicDone"))
                {
                    env.Global.Set("UpdateLogicDone", false);
                }
                if (!env.Global.ContainsKey("UpdateLogicResult"))
                {
                    env.Global.Set("UpdateLogicResult", false);
                }
                var begin = Time.realtimeSinceStartup;
                while (!env.Global.Get<bool>("UpdateLogicDone")) await UniTask.Yield();
                var UpdateLogicResult = env.Global.Get<bool>("UpdateLogicResult");
                var cost = Time.realtimeSinceStartup - begin;
                Debug.Log($"[LuaManager] Update VM done, result={UpdateLogicResult} (t={cost:F1}s)");
                return true;
            }
            catch (Exception e)
            {
                Debug.LogError($"[LuaManager] Update VM exception: {e}");
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
        /// <summary>
        /// 起正式游戏 LuaVM（xLua 为主 VM，自建 lua_State）。
        /// 建完后把 rawL 暴露给 BlueprintRunner 注入，共享同一个 VM。
        /// </summary>
        public async UniTask<bool> RunGameLuaVM(string entryLua = "GameLogic")
        {
            Debug.Log($"[LuaManager] === Game VM start (xLua master): {entryLua} ===");

            DisposeActiveVM();
            // xLua 主 VM：自建 lua_State（不传入外部 L）
            var env = CreateLuaEnv("game", IntPtr.Zero);
            ActiveLuaEnv = env;

            // 关键：在 GameLogic.lua 运行之前，先把 Blueprint.* 绑定注入到 xLua VM。
            // 否则 GameLogic.lua 里 require 'BlueprintEntry' 时
            // Blueprint.RegisterHandler / RegisterNodeDef 还不存在，注册会静默失败。
            BlueprintRuntime.BlueprintService.Instance.InjectLuaBindings(env.L);

            try
            {
                env.DoString($"require '{entryLua}'");

                // 取出生命周期钩子供 Update / OnDestroy 调用
                _luaUpdate    = env.Global.Get<LuaFunction>("onAppTick");
                _luaOnDestroy = env.Global.Get<LuaFunction>("onAppDestroy");

                Debug.Log("[LuaManager] Game VM running");
                await UniTask.Yield();   // 让控制权交还给调用者
                return true;
            }
            catch (Exception e)
            {
                Debug.LogError($"[LuaManager] Game VM exception: {e}");
                return false;
            }
        }

        // ════════════════════════════════════════════════════════════
        // 内部：创建/销毁 VM
        // ════════════════════════════════════════════════════════════

        private LuaEnv CreateLuaEnv(string tag, IntPtr L)
        {
            // 每个 VM 都尝试与 BlueprintRuntime 共享 lua_State（如果存在），
            // 否则起一个独立 VM。两个阶段的 VM 是完全独立的实例。
            LuaEnv env;
            if (L != IntPtr.Zero)
            {
                env = new LuaEnv(L);
                Debug.Log($"[LuaManager:{tag}] LuaEnv created (outer VM)");
            }
            else
            {
                env = new LuaEnv();
                Debug.Log($"[LuaManager:{tag}] LuaEnv created (standalone)");
            }

            env.AddLoader(YooAssetLuaLoader);

            return env;
        }

        private void DisposeActiveVM()
        {
            if (ActiveLuaEnv == null) return;
            Debug.Log("[LuaManager] LuaEnv begin dispose");
            try { _luaOnDestroy?.Call(); } catch (Exception e) { Debug.LogWarning($"[LuaManager] onAppDestroy: {e.Message}"); }

            _luaOnDestroy?.Dispose(); _luaOnDestroy = null;
            _luaUpdate?.Dispose();    _luaUpdate    = null;

            // xLua 主 VM 模式下：Dispose 前先通知 BlueprintRuntime 清空 lua_State 指针。
            // 避免 LuaEnv.Dispose 后 Tick 还在访问已释放的 VM 导致 SIGSEGV。
            //
            // Fix: 直接通过 L 调 BP_NotifyLuaStateClosing，不依赖 _runners 列表。
            // 当 BlueprintBehaviour.OnDestroy 先于 LuaManager.OnDestroy 执行时，
            // _runners 已被 ReleaseRunner 逐个移除清空，导致 ReleaseAllRunner 遍历为空，
            // NotifyLuaStateClosing 没有被调用，Engine.m_L 未置 null，lua_close 后成野指针，
            // 下一帧 Tick 访问野指针触发 SIGSEGV。
            try
            {
                var rawL = ActiveLuaEnv.L;
                if (rawL != IntPtr.Zero)
                    BlueprintRuntime.Native.BP_NotifyLuaStateClosing(rawL);
            }
            catch (Exception e) { Debug.LogWarning($"[LuaManager] BP_NotifyLuaStateClosing: {e.Message}"); }
            try { BlueprintRuntime.BlueprintService.Instance?.ReleaseAllRunner(); }
            catch(Exception e) { Debug.LogWarning($"[LuaManager] ReleaseAllRunner: {e.Message}"); }

            try { ActiveLuaEnv.Dispose(); }
            catch (Exception e) { Debug.LogWarning($"[LuaManager] Dispose: {e.Message}"); }

            ActiveLuaEnv = null;
            _gcTimer = 0f;
            Debug.Log("[LuaManager] LuaEnv disposed");
        }

        // ════════════════════════════════════════════════════════════
        // YooAsset Loader（同步，WebGL 安全）
        // ════════════════════════════════════════════════════════════

        // require "main"      → Assets/Lua/main.lua
        // require "game.util" → Assets/Lua/game/util.lua
        private byte[] YooAssetLuaLoader(ref string luaPath)
        {
#if !UNITY_EDITOR && !UNITY_WEBGL
            string pckPath = GameUtil.PckPath;
            string fixluaPath = luaAddressPrefix + luaPath.Replace('.', '/') + ".lua";
            var pckFullPath = Path.Combine(pckPath, fixluaPath);
            if (File.Exists(pckFullPath))
            {
                luaPath = pckFullPath;
                return File.ReadAllBytes(pckFullPath);
            }
#endif

#if UNITY_EDITOR || !UNITY_WEBGL
            // Editor 和非 WebGL 平台直接从 StreamingAssets 同步加载（不走 YooAsset 包管理，方便开发）
            string relativePath = luaAddressPrefix + luaPath.Replace('.', '/') + ".lua";

            var package = YooAssets.GetPackage("DefaultPackage");
            if (null == package) return null;

            var handle = package.LoadAssetSync<TextAsset>(relativePath);
            if (null == handle) return null;

            var textAsset = handle.AssetObject as TextAsset;
            if (null == textAsset) return null;
            handle.Release();
            return textAsset.bytes;
#else
            if (!YooAssetsLuaBridge.HasLuaFile(luaPath))
            {
                return null;
            }

            var handle = YooAssetsLuaBridge.GetLuaAssetHandle(luaPath);
            if (handle == null)
            {
                Debug.LogWarning($"[LuaManager] Lua bundle not init; {luaPath}");
                return null;
            }

            string assetPath  = YooAssetsLuaBridge.GetLuaAssetPath(luaPath);
            if(string.IsNullOrEmpty(assetPath))
            {
                Debug.LogWarning($"[LuaManager] Lua not found: '{luaPath}'");
                return null;
            }

            var textAsset = handle.AssetObject as TextAsset;
            var bytes = textAsset?.bytes;
            if (bytes == null)
            {
                Debug.LogWarning($"[LuaManager] Lua not found: '{assetPath}' (require '{luaPath}')");
                return null;
            }

            YooAssetsLuaBridge.ReleaseLuaAssetHandle(luaPath);

            // 把实际加载到的路径回写给 xLua（用于错误堆栈）
            luaPath = assetPath;
            return bytes;
#endif
        }

        public async UniTask<bool> PreloadAllScript(string packageName)
        {
            var result = new ReturnTuple<bool, bool>();
            YooAssetsLuaBridge.LoadAllLuaFiles(packageName, luaAddressPrefix, null, (bool bSuccessed, string[] f, bool[] s, string e) =>
            {
                result.value_0 = true;
                result.value_1 = bSuccessed;
            });

            while (!result.value_0) await UniTask.Yield();
            if (!result.value_1)
            {
                Debug.LogError("[GameLauncher] Failed to load Lua files, abort.");
                return false;
            }

            Debug.Log("[GameLauncher] All Lua files loaded ✓");
            return true;
        }
    }
}
