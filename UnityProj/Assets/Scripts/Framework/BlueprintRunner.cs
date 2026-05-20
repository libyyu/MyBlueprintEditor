// BlueprintRunner.cs
// Blueprint C Runtime �� C# P/Invoke ��װ
//
// ����˼·��
//   xLua �� LuaEnv.rawL (IntPtr) ���� lua_State*
//   ͨ�� BP_SetExternalLuaState ���������� Blueprint Runner
//   ���� Blueprint Runtime ��ע��� Blueprint ȫ�ֶ���ͺ� xLua ��ͬһ�� VM ��
//   game_extensions.lua / game_nodes.lua ��� Blueprint.RegisterNodeDef ������������
//
// ��ʼ��˳�򣨱����ϸ����أ���
//   1. LuaManager.Awake() �� LuaEnv �������
//   2. BlueprintRunner.Init(luaEnv) �� ���� Runner + BP_SetExternalLuaState
//   3. LuaManager.StartLuaAsync() �� Ԥ���� Lua �� ִ�� main.lua
//      ��ʱ Blueprint ȫ�ֶ������� Runtime ע�룬BlueprintEntry.lua ��������ע��ڵ�

using BlueprintRuntime;
using System;
using UnityEngine;

namespace CutRope.Framework
{
    public class BlueprintRunner : MonoBehaviour
    {
        // ���� ���� ����������������������������������������������������������������������������������������������������������
        public static BlueprintRunner Instance { get; private set; }

        // Runner ���
        private BPRunner _runner = null;
        public BPRunner Runner => _runner;
        public bool IsValid => _runner != null;

        // ���� �������� ��������������������������������������������������������������������������������������������������

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

        // ���� ���� API ��������������������������������������������������������������������������������������������������

        /// <summary>
        /// ��ʼ�� Blueprint Runtime��
        /// BP_CreateRunner �ڲ���������ʼ�� Lua VM��
        /// ��ɺ� C# ����ͨ�� LuaState ȡ�� lua_State*��
        /// ���� new LuaEnv(externalL) ����ͬһ�� VM��
        /// </summary>
        public bool Init()
        {
            try
            {
                // 在初始化前先设好 dump 目录，这样即使 Init 自身崩溃也能抓到
                string dumpDir = System.IO.Path.Combine(
                    UnityEngine.Application.persistentDataPath, "CrashDumps");
                BPRunner.SetCrashDumpDir(dumpDir);
                Debug.Log($"[BlueprintRunner] Crash dump dir: {dumpDir}");

                _runner = new BPRunner();

                _runner.OnPrint += (lv, msg) =>
                {
                    switch (lv)
                    {
                        case BPLogLevel.Warning: Debug.LogWarning($"[BlueprintRunner-InnrPrint] {msg}"); break;
                        case BPLogLevel.Error: Debug.LogError($"[BlueprintRunner-InnrPrint] {msg}"); break;
                        default: Debug.Log($"[BlueprintRunner-InnrPrint] {msg}"); break;
                    }
                };
#if UNITY_EDITOR || DEVELOPMENT_BUILD
                _runner.EnableLogging(true);
                _runner.OnLog += (lv, msg) =>
                {
                    switch(lv)
                    {
                        case BPLogLevel.Warning: Debug.LogWarning($"[BlueprintRunner] {msg}"); break;
                        case BPLogLevel.Error: Debug.LogError($"[BlueprintRunner] {msg}"); break;
                        default: Debug.Log($"[BlueprintRunner-InnrPrint] {msg}"); break;
                    }
                };
#endif

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
        /// Runtime �ڲ��� lua_State������ new LuaEnv(externalL) ʹ�ã�
        /// </summary>
        public IntPtr LuaState => _luaState;
        private IntPtr _luaState;

        /// <summary>�� JSON �ַ������ز�ִ����ͼ</summary>
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

        /// <summary>�ɷ��¼���������ͼ�е� GameEvent.Poll �ڵ㣩</summary>
        public void DispatchEvent(string eventId)
        {
            if (IsValid) _runner.DispatchEvent(eventId);
        }

        /// <summary>ÿ֡ Tick������ Timer / Tween ��ʱ��ڵ㣩</summary>
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