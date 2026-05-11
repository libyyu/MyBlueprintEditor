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
        [DllImport(DLL)] static extern IntPtr BP_GetLuaState(IntPtr runner);
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
        /// 初始化 Blueprint Runtime，将外部 xLua LuaEnv 的 lua_State 共享给 Runtime。
        /// 调用方式：
        ///   1. LuaManager 先 new LuaEnv() 建立 VM
        ///   2. 调用 BlueprintRuntime.Init(luaEnv) 把 lua_State 注入给 Runtime
        ///   3. Runtime 的 LuaScriptEngine 用 InitializeWithExternalState 注册 Blueprint.* 绑定
        /// </summary>
        public bool Init(LuaEnv luaEnv)
        {
            try
            {
                // 1. 创建 Runner
                _runner = BP_CreateRunner();
                if (_runner == IntPtr.Zero)
                {
                    Debug.LogError("[BlueprintRuntime] BP_CreateRunner failed");
                    return false;
                }

                // 2. 将 xLua lua_State 注入给 Runtime
                //    Runtime 的 LuaScriptEngine 用 InitializeWithExternalState 注册 Blueprint.* 绑定
                _luaState = luaEnv.rawL;
                BP_SetExternalLuaState(_runner, _luaState);

                Debug.Log($"[BlueprintRuntime] Ready. lua_State=0x{_luaState.ToInt64():X}");
                return true;
            }
            catch (Exception e)
            {
                Debug.LogError($"[BlueprintRuntime] Init failed: {e.Message}\n" +
                               "Make sure BlueprintRuntime.dll is in Assets/Plugins/");
                return false;
            }
        }

        /// <summary>
        /// Runtime 内部的 lua_State（传给 new LuaEnv(externalL) 使用）
        /// </summary>
        public IntPtr LuaState => _luaState;
        private IntPtr _luaState;

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
