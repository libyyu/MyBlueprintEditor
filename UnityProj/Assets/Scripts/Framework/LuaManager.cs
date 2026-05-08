// LuaManager.cs
// xLua 生命周期管理器
//   - 创建/销毁 LuaEnv
//   - 每帧 Tick（GC 间隔可配置）
//   - 从 Resources 加载并执行 main.lua
//   - 提供全局 Lua 调用入口

using System;
using System.IO;
using UnityEngine;
using XLua;

namespace CutRope.Framework
{
    public class LuaManager : MonoBehaviour
    {
        [Header("Lua 配置")]
        [Tooltip("GC 间隔（秒），建议 1~3")]
        public float gcInterval = 1f;

        // ── 公共访问 ─────────────────────────────────────────────────
        public static LuaManager Instance { get; private set; }
        public LuaEnv LuaEnv { get; private set; }

        // ── 私有 ─────────────────────────────────────────────────────
        private float _gcTimer;
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

            // 自定义 Loader：从 Resources 加载 .lua.txt 文件
            LuaEnv.AddLoader(ResourcesLoader);
        }

        /// <summary>
        /// 执行 Lua 入口。由 GameLauncher 在 YooAsset 就绪后调用。
        /// </summary>
        public void StartLua()
        {
            // 执行 main.lua（Resources/Lua/main.lua.txt）
            LuaEnv.DoString("require 'main'");

            // 取出 Lua 导出的 update / on_destroy 钩子（可选）
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

        // ── Loader：Resources/Lua/{name}.lua.txt ─────────────────────
        private static byte[] ResourcesLoader(ref string filepath)
        {
            // xLua 传入的 filepath 形如 "main" 或 "framework/util"
            // 映射到 Resources/Lua/{filepath}.lua.txt
            string resPath = $"Lua/{filepath.Replace('.', '/')}";
            var ta = Resources.Load<TextAsset>(resPath);
            if (ta == null)
            {
                Debug.LogWarning($"[LuaManager] Lua file not found: Resources/{resPath}.lua.txt");
                return null;
            }
            return ta.bytes;
        }
    }
}
