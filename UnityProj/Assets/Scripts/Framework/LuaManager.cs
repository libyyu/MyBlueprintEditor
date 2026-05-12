// LuaManager.cs
// xLua 生命周期管理器
//
// WebGL 兼容方案：
//   - 启动时异步批量预加载所有 Lua 文件到内存字典
//   - xLua Loader 从字典同步读取（无 IO，WebGL 安全）
//
// Lua 文件地址约定（YooAsset）：
//   Assets/Lua/main.lua          → 字典 key: "main"
//   Assets/Lua/game/util.lua     → 字典 key: "game/util"
//   xLua require "game/util"     → 查字典 key "game/util"

using System;
using System.Collections;
using System.Collections.Generic;
using System.IO;
using UnityEngine;
using XLua;
using YooAsset;

namespace CutRope.Framework
{
    public class LuaManager : MonoBehaviour
    {
        [Header("Lua 配置")]
        [Tooltip("GC 间隔（秒），建议 1~3")]
        public float gcInterval = 1f;

        [Tooltip("Lua 入口文件的 YooAsset 地址（不含 Lua/ 前缀），如 main")]
        public string mainLuaAddress = "main";

        [Tooltip("Lua 文件在 YooAsset 中的公共前缀，用于批量加载")]
        public string luaAddressPrefix = "Lua/";

        // ── 公共访问 ─────────────────────────────────────────────────
        public static LuaManager Instance { get; private set; }
        public LuaEnv LuaEnv { get; private set; }

        // ── 私有 ─────────────────────────────────────────────────────
        private float  _gcTimer;
        private LuaFunction _luaUpdate;
        private LuaFunction _luaOnDestroy;

        // 预加载缓存：key = lua模块路径（如 "main", "game/util"）
        private readonly Dictionary<string, string> _luaCache = new Dictionary<string, string>();

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

            // 1. 初始化 Blueprint Runtime（内部立即创建 Lua VM）
            var bpRuntime = BlueprintRuntime.Instance
                         ?? gameObject.AddComponent<BlueprintRuntime>();
            bpRuntime.Init();

            // 2. 用 Runtime 的 lua_State 创建 LuaEnv（共享同一 VM）
            //    Blueprint 全局对象已在 VM 里，BlueprintEntry.lua 的注册能正常工作
            if (bpRuntime.LuaState != System.IntPtr.Zero)
            {
                LuaEnv = new LuaEnv(bpRuntime.LuaState);
                Debug.Log("[LuaManager] Using Blueprint Runtime lua_State");
            }
            else
            {
                Debug.LogWarning("[LuaManager] Blueprint Runtime unavailable, using standalone LuaEnv");
                LuaEnv = new LuaEnv();
            }

            // Loader：从内存缓存同步返回（WebGL 安全）
            LuaEnv.AddLoader(CachedLuaLoader);

            StaticLuaCallbacks.lua_Print   = s => Debug.Log("[Lua]" + s);
            StaticLuaCallbacks.lua_Warning = s => Debug.LogWarning("[Lua]" + s);
            StaticLuaCallbacks.lua_Error   = s => Debug.LogError("[Lua]" + s);
        }

        /// <summary>
        /// 异步启动 Lua：
        ///   1. 批量预加载所有 Lua 文件到缓存
        ///   2. 执行 main.lua
        /// 由 GameLauncher 在 YooAsset 就绪后调用。
        /// </summary>
        public IEnumerator StartLuaAsync()
        {
            // 1. 批量预加载所有 Lua 资源
            yield return PreloadAllLuaFiles();

            // 2. 执行入口
            if (!_luaCache.ContainsKey(mainLuaAddress))
            {
                Debug.LogError($"[LuaManager] Entry lua not found in cache: '{mainLuaAddress}'");
                yield break;
            }

            LuaEnv.DoString($"require '{mainLuaAddress}'");

            // 3. 取出 Lua 钩子
            _luaUpdate    = LuaEnv.Global.Get<LuaFunction>("update");
            _luaOnDestroy = LuaEnv.Global.Get<LuaFunction>("on_destroy");

            Debug.Log($"[LuaManager] Lua started. {_luaCache.Count} files cached.");
        }

        // ── 批量预加载 ────────────────────────────────────────────────
        private IEnumerator PreloadAllLuaFiles()
        {
            // 获取 DefaultPackage 中所有以 luaAddressPrefix 开头的资源
            var package = YooAssets.GetPackage("DefaultPackage");
            if (package == null)
            {
                Debug.LogError("[LuaManager] DefaultPackage not found!");
                yield break;
            }

            // 通过 AssetInfo 枚举所有 Lua 资产
            var assetInfos = package.GetAssetInfos(luaAddressPrefix);
            if (assetInfos == null || assetInfos.Length == 0)
            {
                Debug.LogWarning($"[LuaManager] No lua files found with tag/prefix: {luaAddressPrefix}");
                yield break;
            }

            Debug.Log($"[LuaManager] Preloading {assetInfos.Length} lua files...");

            // 并发加载所有 Lua 文件
            var handles = new List<AssetHandle>();
            var prefix = ("assets/" + luaAddressPrefix).ToLower();
            foreach (var info in assetInfos)
            {
                var assetPath = info.AssetPath.ToLower();
                if (!assetPath.EndsWith(".lua"))
                {
                    Debug.LogWarning($"`{assetPath}` is not a valid lua script");
                    continue;
                }
                assetPath = assetPath.Replace(".lua", "").Replace("\\", "/").Replace(prefix, "");
                _luaCache.Add(assetPath, info.AssetPath);
                handles.Add(package.LoadAssetAsync<TextAsset>(info.AssetPath));
            }

            // 等待全部完成
            foreach (var handle in handles)
                yield return handle;

            Debug.Log($"[LuaManager] Lua preload done. {_luaCache.Count} files ready.");
        }

        // ── 内存缓存 Loader（同步，WebGL 安全）──────────────────────
        private byte[] CachedLuaLoader(ref string luaPath)
        {
            var package = YooAssets.GetPackage("DefaultPackage");
            if (package == null) return null;
           
            var matchluaPath = luaPath.Replace(".lua", "").Replace(".", "/").ToLower();
            string packageAssetPath;
            // xLua 传入 "main" 或 "game/util" 或 "game.util"
            if (_luaCache.TryGetValue(matchluaPath, out packageAssetPath) && !string.IsNullOrEmpty(packageAssetPath))
            {
                luaPath = packageAssetPath;
                Debug.Log($"CustomLoader: {packageAssetPath}");
                var handle = package.LoadAssetSync<TextAsset>(packageAssetPath);
                if (null != handle)
                {
                    var textAsset = handle.AssetObject as TextAsset;
                    var result = textAsset.bytes;
                    handle.Release();
                    return result;
                }
                else
                {
                    Debug.LogWarning($"lua script load failed. {packageAssetPath}");
                    return null;
                }
            }

            Debug.LogWarning($"[LuaManager] Lua not in cache: '{luaPath}'. Did you preload?");
            return null;
        }

        // ── Update / Destroy ─────────────────────────────────────────
        private void Update()
        {
            _luaUpdate?.Call();

            _gcTimer += Time.deltaTime;
            if (_gcTimer >= gcInterval)
            {
                _gcTimer = 0f;
                LuaEnv.Tick();
            }
        }

        private void OnDestroy()
        {
            _luaOnDestroy?.Call();
            _luaOnDestroy?.Dispose();
            _luaOnDestroy = null;
            _luaUpdate?.Dispose();
            _luaUpdate = null;
            _luaCache.Clear();
            LuaEnv?.Dispose();
            LuaEnv = null;
            if (Instance == this) Instance = null;
        }
    }
}
