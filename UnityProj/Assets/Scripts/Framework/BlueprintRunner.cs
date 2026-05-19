// BlueprintRunner.cs
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
//   2. BlueprintRunner.Init(luaEnv) → 创建 Runner + BP_SetExternalLuaState
//   3. LuaManager.StartLuaAsync() → 预加载 Lua → 执行 main.lua
//      此时 Blueprint 全局对象已由 Runtime 注入，BlueprintEntry.lua 可以正常注册节点

using BlueprintRuntime;
using System;
using UnityEngine;

namespace CutRope.Framework
{
    public class BlueprintRunner : MonoBehaviour
    {
        // ── 单例 ─────────────────────────────────────────────────────
        public static BlueprintRunner Instance { get; private set; }

        // Runner 句柄
        private BPRunner _runner = null;
        public BPRunner Runner => _runner;
        public bool IsValid => _runner != null;

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
        /// 初始化 Blueprint Runtime。
        /// BP_CreateRunner 内部会立即初始化 Lua VM，
        /// 完成后 C# 就能通过 LuaState 取得 lua_State*，
        /// 传给 new LuaEnv(externalL) 共享同一个 VM。
        /// </summary>
        public bool Init()
        {
            try
            {
                _runner = new BPRunner();

                _runner.OnPrint += (lv, msg) =>
                {
                    switch (lv)
                    {
                        case BPLogLevel.Warning: Debug.LogWarning($"[BlueprintRunner] {msg}"); break;
                        case BPLogLevel.Error: Debug.LogError($"[BlueprintRunner] {msg}"); break;
                        default: Debug.Log($"[BlueprintRunner] {msg}"); break;
                    }
                };

                _luaState = _runner.GetLuaState();
                if (_luaState == IntPtr.Zero)
                {
                    Debug.LogError("[BlueprintRunner] GetLuaState returned null");
                    return false;
                }

                Debug.Log($"[BlueprintRunner] Ready. lua_State=0x{_luaState.ToInt64():X}");
                return true;
            }
            catch (Exception e)
            {
                Debug.LogError($"[BlueprintRunner] Init failed: {e.Message}\n" +
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
            try
            {
                _runner.LoadFromJson(json);
                return true;
            }
            catch(Exception e)
            {
                Debug.LogError($"[BlueprintRunner] LoadFromJson failed: {e.Message}");
                return false;
            }
        }

        /// <summary>派发事件（驱动蓝图中的 GameEvent.Poll 节点）</summary>
        public void DispatchEvent(string eventId)
        {
            if (IsValid) _runner.DispatchEvent(eventId);
        }

        /// <summary>每帧 Tick（驱动 Timer / Tween 等时间节点）</summary>
        private void Update()
        {
            if (IsValid) _runner.Tick(Time.deltaTime);
        }

        public void Shutdown()
        {
            if (_runner != null)
            {
                _runner.Dispose();
                _runner = null;
                Debug.Log("[BlueprintRunner] Shutdown.");
            }
        }
    }
}