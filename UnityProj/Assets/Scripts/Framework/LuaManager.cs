// LuaManager.cs
// xLua 生命周期管理器
//   - 创建/销毁 LuaEnv
//   - 每帧 Tick（GC 间隔可配置）
//   - Lua 文件从 YooAsset 包中加载（地址格式：Lua/xxx）
//   - 提供全局 Lua 调用入口

using System;
using System.Collections;
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

        [Tooltip("Lua 入口文件在 YooAsset 中的地址，如 Lua/main")]
        public string mainLuaAddress = "Lua/main";

        // ── 公共访问 ─────────────────────────────────────────────────
        public static LuaManager Instance { get; private set; }
        public LuaEnv LuaEnv { get; private set; }

        // ── 私有 ─────────────────────────────────────────────────────
        private float  _gcTimer;
        private Action _luaUpdate;
        private Action _luaOnDestroy;

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

            LuaEnv = new LuaEnv();

            // 自定义 Loader：从 YooAsset 同步加载 Lua 文件
            // xLua require "game/util" → 加载 YooAsset 地址 "Lua/game/util"
            LuaEnv.AddLoader(YooAssetLuaLoader);
        }

        /// <summary>
        /// 异步启动 Lua：加载 main.lua 并执行。
        /// 由 GameLauncher 在 YooAsset 就绪后调用。
        /// </summary>
        public IEnumerator StartLuaAsync()
        {
            // 加载入口文件
            var handle = YooAssets.LoadAssetAsync<TextAsset>(mainLuaAddress);
            yield return handle;

            if (handle.Status != EOperationStatus.Succeed)
            {
                Debug.LogError($"[LuaManager] Failed to load '{mainLuaAddress}': {handle.LastError}");
                handle.Release();
                yield break;
            }

            var ta = handle.AssetObject as TextAsset;
            if (ta == null)
            {
                Debug.LogError($"[LuaManager] '{mainLuaAddress}' is not a TextAsset");
                handle.Release();
                yield break;
            }

            // 执行 main.lua
            LuaEnv.DoString(ta.text, mainLuaAddress);
            handle.Release();

            // 取出 Lua 导出的钩子（可选）
            _luaUpdate    = LuaEnv.Global.Get<Action>("update");
            _luaOnDestroy = LuaEnv.Global.Get<Action>("on_destroy");

            Debug.Log("[LuaManager] Lua started.");
        }

        private void Update()
        {
            _luaUpdate?.Invoke();

            _gcTimer += Time.deltaTime;
            if (_gcTimer >= gcInterval)
            {
                _gcTimer = 0f;
                LuaEnv.Tick();
            }
        }

        private void OnDestroy()
        {
            _luaOnDestroy?.Invoke();
            LuaEnv?.Dispose();
            LuaEnv = null;
            if (Instance == this) Instance = null;
        }

        // ── YooAsset Lua Loader ───────────────────────────────────────
        // xLua require "game/util" → YooAsset 地址 "Lua/game/util"
        private static byte[] YooAssetLuaLoader(ref string luaPath)
        {
            // luaPath 形如 "main" 或 "game/util"
            string address = $"Lua/{luaPath}";

            // 同步加载（Loader 必须同步返回）
            var handle = YooAssets.LoadAssetSync<TextAsset>(address);
            if (handle.Status != EOperationStatus.Succeed)
            {
                Debug.LogWarning($"[LuaManager] Lua file not found in YooAsset: {address}");
                handle.Release();
                return null;
            }

            var ta = handle.AssetObject as TextAsset;
            if (ta == null)
            {
                handle.Release();
                return null;
            }

            byte[] bytes = ta.bytes;
            handle.Release();
            return bytes;
        }
    }
}
