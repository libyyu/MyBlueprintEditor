// BlueprintRuntime.cs
// Blueprint C Runtime 的 C# P/Invoke 封装
//
// 核心思路：
//   xLua 的 LuaEnv.rawL (IntPtr) 就是 lua_State*
//   通过 BP_SetExternalLuaState 把它共享给 Blueprint Runner
//   这样 Blueprint Runtime 里注入的 Blueprint 全局对象就和 xLua 在同一个 VM 里
//   game_extensions.lua / game_nodes.lua 里的 Blueprint.RegisterNodeDef 就能正常工作
//
// 初始化顺序（必须严格遵守）：
//   1. LuaManager.Awake() → LuaEnv 创建完成
//   2. BlueprintRuntime.Init(luaEnv) → 创建 Runner + BP_SetExternalLuaState
//   3. LuaManager.StartLuaAsync() → 预加载 Lua → 执行 main.lua
//      此时 Blueprint 全局对象已由 Runtime 注入，BlueprintEntry.lua 可以正常注册节点

using System;
using System.Runtime.InteropServices;
using UnityEngine;
using XLua;

namespace CutRope.Framework
{
    public class BlueprintRuntime : MonoBehaviour
    {
        // ── 单例 ─────────────────────────────────────────────────────
        public static BlueprintRuntime Instance { get; private set; }

        // Runner 句柄
        private IntPtr _runner = IntPtr.Zero;
        public  IntPtr Runner  => _runner;
        public  bool   IsValid => _runner != IntPtr.Zero;

#if UNITY_EDITOR || UNITY_STANDALONE_WIN
        private const string DLL = "BlueprintRuntime";
#elif UNITY_IOS
        private const string DLL = "__Internal";
#elif UNITY_ANDROID
        private const string DLL = "BlueprintRuntime";
#else
        private const string DLL = "BlueprintRuntime";
#endif

        // ── P/Invoke 声明 ─────────────────────────────────────────────

        [DllImport(DLL)] static extern IntPtr BP_CreateRunner();
        [DllImport(DLL)] static extern void   BP_DestroyRunner(IntPtr runner);
        [DllImport(DLL)] static extern void   BP_SetExternalLuaState(IntPtr runner, IntPtr L);
        [DllImport(DLL)] static extern int    BP_LoadGlobalLuaEntry(IntPtr runner);
        [DllImport(DLL)] static extern int    BP_LoadFromJson(IntPtr runner, string json);
        [DllImport(DLL)] static extern void   BP_Tick(IntPtr runner, float deltaTime);
        [DllImport(DLL)] static extern int    BP_Execute(IntPtr runner);
        [DllImport(DLL)] static extern int    BP_DispatchEvent(IntPtr runner, string eventId);
        [DllImport(DLL)] static extern void   BP_SetBasePath(IntPtr runner, string basePath);
        [DllImport(DLL)] static extern int    BP_HasPendingWork(IntPtr runner);

        // ── 生命周期 ─────────────────────────────────────────────────

        private void Awake()
        {
            if (Instance != null && Instance != this) { Destroy(gameObject); return; }
            Instance = this;
            DontDestroyOnLoad(gameObject);
        }

        private void OnDestroy()
        {
            Shutdown();
            if (Instance == this) Instance = null;
        }

        // ── 公共 API ─────────────────────────────────────────────────

        /// <summary>
        /// 初始化 Blueprint Runtime，共享 xLua 的 lua_State。
        /// 必须在 LuaEnv 创建后、Lua 脚本执行前调用。
        /// </summary>
        public bool Init(LuaEnv luaEnv)
        {
            if (luaEnv == null)
            {
                Debug.LogError("[BlueprintRuntime] LuaEnv is null");
                return false;
            }

            try
            {
                _runner = BP_CreateRunner();
                if (_runner == IntPtr.Zero)
                {
                    Debug.LogError("[BlueprintRuntime] BP_CreateRunner failed");
                    return false;
                }

                // 关键：共享 xLua 的 lua_State，让 Blueprint 在同一 VM 里注入全局对象
                var L = luaEnv.rawL;
                BP_SetExternalLuaState(_runner, L);
                Debug.Log($"[BlueprintRuntime] Shared lua_State: 0x{L.ToInt64():X}");

                // 设置 Lua 入口文件搜索路径（StreamingAssets 或 Application.dataPath）
#if UNITY_EDITOR
                var basePath = System.IO.Path.Combine(Application.dataPath, "Lua");
                BP_SetBasePath(_runner, basePath);
#endif
                // 加载全局 BlueprintEntry（注入 Blueprint 全局对象 + 注册内置节点）
                int ret = BP_LoadGlobalLuaEntry(_runner);
                if (ret != 0)
                {
                    // 非致命：可能 BlueprintEntry.lua 不在搜索路径，由 LuaManager 显式 require
                    Debug.LogWarning($"[BlueprintRuntime] BP_LoadGlobalLuaEntry returned {ret} (will load via LuaManager)");
                }

                Debug.Log("[BlueprintRuntime] Initialized. Blueprint global injected into xLua VM.");
                return true;
            }
            catch (Exception e)
            {
                Debug.LogError($"[BlueprintRuntime] Init failed: {e.Message}\n" +
                               "Make sure BlueprintRuntime.dll is in Assets/Plugins/");
                return false;
            }
        }

        /// <summary>从 JSON 字符串加载并执行蓝图</summary>
        public bool LoadFromJson(string json)
        {
            if (!IsValid) return false;
            return BP_LoadFromJson(_runner, json) == 0;
        }

        /// <summary>派发事件（驱动蓝图中的 GameEvent.Poll 节点）</summary>
        public void DispatchEvent(string eventId)
        {
            if (IsValid) BP_DispatchEvent(_runner, eventId);
        }

        /// <summary>每帧 Tick（驱动 Timer / Tween 等时间节点）</summary>
        private void Update()
        {
            if (IsValid) BP_Tick(_runner, Time.deltaTime);
        }

        public void Shutdown()
        {
            if (_runner != IntPtr.Zero)
            {
                BP_DestroyRunner(_runner);
                _runner = IntPtr.Zero;
                Debug.Log("[BlueprintRuntime] Shutdown.");
            }
        }
    }
}
