using CutRope.Framework;
using System;
using System.Collections;
using System.Collections.Generic;
using System.Text;
using UnityEngine;
using XLua;
#if USE_UNI_LUA
using LuaAPI = UniLua.Lua;
using RealStatePtr = UniLua.ILuaState;
using LuaCSFunction = UniLua.CSharpFunctionDelegate;
#else
using LuaAPI = XLua.LuaDLL.Lua;
using RealStatePtr = System.IntPtr;
using LuaCSFunction = XLua.LuaDLL.lua_CSFunction;
#endif

public class FTimerList
{
    static int _uniqueid = 1;
    public int total_count = 0;

    public struct Timer
    {
        public int id;
        public float ttl;
        public float end_time;
        //public int callback;
        public LuaFunction callback;
        public int cbparam;
        public bool bOnce;
    }

    List<Timer> m_List = new List<Timer>();
    List<Timer> m_TempList = new List<Timer>();
    List<int> m_TempDelList = new List<int>();
    bool m_bTick = false;

    IntPtr getL()
    {
        if (!LuaManager.Instance || null == LuaManager.Instance.ActiveLuaEnv)
            return IntPtr.Zero;
        return LuaManager.Instance.ActiveLuaEnv.L;
    }
    LuaEnv getEnv()
    {
        if (!LuaManager.Instance || null == LuaManager.Instance.ActiveLuaEnv)
            return null;
        return LuaManager.Instance.ActiveLuaEnv;
    }

    public int GetEnableCount() { return m_List.Count + m_TempList.Count - m_TempDelList.Count; }

    public int AddTimer(float ttl, bool bOnce, int cb, int cbparam)
    {
        if (cb < 0)
        {
            throw new Exception("AddTimer, cb is wrong");
        }

        Timer tm;
        tm.id = _uniqueid++;
        tm.ttl = ttl;
        tm.end_time = Time.time + ttl;
        tm.callback = new LuaFunction(cb, getEnv());
        tm.cbparam = cbparam;
        tm.bOnce = bOnce;
        if (m_bTick)
            m_TempList.Add(tm);
        else
        {
            m_List.Add(tm);
            total_count++;
        }
        return tm.id;
    }

    public void RemoveTimer(int id)
    {
        if (m_bTick)
            m_TempDelList.Add(id);
        else
        {
            var env = getEnv();
            if (env == null)
                return;

            for (int i = 0; i < m_List.Count; i++)
            {
                Timer tm = m_List[i];

                if (tm.id == id)
                {
                    if (tm.callback != null)
                    {
                        tm.callback.Dispose();
                        tm.callback = null;
                    }
                    //TODO: 释放tick额外参数
                    //      if (tm.cbparam != LuaRefValue.LUA_NOREF)
                    //          LuaDLL.luaL_unref(L, LuaIndexes.LUA_REGISTRYINDEX, tm.cbparam);
                    m_List.RemoveAt(i);
                    total_count--;
                    return;
                }
            }
        }
    }

    public void ResetTimer(int id)
    {
        for (int i = 0; i < m_List.Count; i++)
        {
            Timer tm = m_List[i];

            if (tm.id == id)
            {
                tm.end_time = Time.time + tm.ttl;
                m_List[i] = tm;
                return;
            }
        }
    }

    public void Tick(float CurTime)
    {
        if (m_List.Count == 0)
            return;

        float cur = CurTime;
        int i = 0;
        m_bTick = true;
        var env = getEnv();
        while (i < m_List.Count)
        {
            Timer tm = m_List[i];

            if (tm.end_time <= cur)
            {
                
                if(env != null)
                {
                    if (tm.callback != null)
                        tm.callback.Call();
                }

                if (tm.bOnce)
                {
                    if (tm.callback != null)
                    {
                        tm.callback.Dispose();
                        tm.callback = null;
                    }
                    //TODO: 释放tick额外参数
                    //     if (tm.cbparam != LuaRefValue.LUA_NOREF)
                    //         LuaDLL.luaL_unref(L, LuaIndexes.LUA_REGISTRYINDEX, tm.cbparam);
                    m_List.RemoveAt(i);
                    total_count--;
                }
                else
                {
                    tm.end_time = cur + tm.ttl;
                    m_List[i] = tm;
                    i++;
                }
            }
            else
                i++;
        }

        m_bTick = false;

        if (m_TempList.Count > 0)
        {
            for (int j = 0; j < m_TempList.Count; j++)
            {
                Timer tm = m_TempList[j];
                m_List.Add(tm);
            }

            total_count += m_TempList.Count;
            m_TempList.Clear();
        }

        if (m_TempDelList.Count > 0)
        {
            for (int j = 0; j < m_TempDelList.Count; j++)
            {
                int id = m_TempDelList[j];
                RemoveTimer(id);
            }

            m_TempDelList.Clear();
        }
    }

    public void Clear()
    {
        if (m_List.Count == 0)
            return;

        total_count -= m_List.Count;
        var env = getEnv();
        if (env == null)
        {
            m_List.Clear();
            return;
        }

        for (int i = 0; i < m_List.Count; i++)
        {
            if (m_List[i].callback != null)
            {
                m_List[i].callback.Dispose();
            }
            //TODO: 释放tick额外参数
          //  if (m_List[i].cbparam != LuaRefValue.LUA_NOREF)
          //  {
          //      LuaDLL.luaL_unref(getL(), LuaIndexes.LUA_REGISTRYINDEX, m_List[i].cbparam);
          //  }
        }

        m_List.Clear();
    }

    //public String GetDebugInfo(GameObject obj, Dictionary<string, int> callbackCountMap)
    //{
    //    StringBuilder strBuilder = new StringBuilder();
    //    strBuilder.Append(UnityDebugHelper.FormatGameObjectPath(obj));
    //    strBuilder.AppendLine(": ");
    //    foreach (Timer timer in m_List)
    //    {
    //        String callback = wLua.L.GetRegistryFunctionInfo(timer.callback);
    //        int oldCount;
    //        if (callbackCountMap.TryGetValue(callback, out oldCount))
    //            callbackCountMap[callback] = oldCount + 1;
    //        else
    //            callbackCountMap[callback] = 1;

    //        strBuilder.Append("  ");
    //        var info = String.Format("id: {0}, ttl: {1}, end_time: {2}, once: {3}, cb: {4}", timer.id, timer.ttl, timer.end_time, timer.bOnce, wLua.L.GetRegistryFunctionInfo(timer.callback));
    //        strBuilder.AppendLine(info);
    //    }
    //    return strBuilder.ToString();
    //}

    //public static String GetAllDebugInfo()
    //{
    //    Dictionary<string, int> callbackCountMap = new Dictionary<string, int>();

    //    StringBuilder strBuilder = new StringBuilder();
    //    foreach (var item in s_instanceMap)
    //    {
    //        if (item.Value != null)
    //            strBuilder.Append(item.Key.GetDebugInfo(item.Value, callbackCountMap));
    //    }
    //    strBuilder.AppendLine("------------------------------");
    //    strBuilder.AppendLine("-- duplicated callbacks: ");
    //    foreach (var item in callbackCountMap)
    //    {
    //        if (item.Value >= 2)
    //        {
    //            strBuilder.Append("  ");
    //            strBuilder.Append(item.Key);
    //            strBuilder.Append(" = ");
    //            strBuilder.Append(item.Value);
    //            strBuilder.AppendLine();
    //        }
    //    }

    //    return strBuilder.ToString();
    //}

    static Dictionary<FTimerList, GameObject> s_instanceMap = new Dictionary<FTimerList, GameObject>();

    public static void RegisterTimerList(FTimerList timerList, GameObject obj)
    {
        s_instanceMap[timerList] = obj;
    }

    public static void UnregisterTimerList(FTimerList timerList)
    {
        s_instanceMap.Remove(timerList);
    }
}

[LuaCallCSharp]
public class FTimerListBehavior : MonoBehaviour
{
    float CurTime;
    float DeltaTime;
    FTimerList m_TimerList = new FTimerList();
    FTimerList m_LateTimerList = new FTimerList();
#if UNITY_EDITOR
    public int timer_num = 0;
#endif
    void Awake()
    {
        FTimerList.RegisterTimerList(m_TimerList, gameObject);
        FTimerList.RegisterTimerList(m_LateTimerList, gameObject);
    }

    void OnDestroy()
    {
        FTimerList.UnregisterTimerList(m_TimerList);
        FTimerList.UnregisterTimerList(m_LateTimerList);

        m_TimerList.Clear();
        m_LateTimerList.Clear();
    }

    void Update()
    {
        CurTime = Time.time;
        DeltaTime = Time.deltaTime;

        m_TimerList.Tick(CurTime);

#if UNITY_EDITOR
        timer_num = m_TimerList.total_count;
#endif
    }
    void LateUpdate()
    {
        m_LateTimerList.Tick(CurTime);
    }

    //TODO: int cb 需要改成LuaFunction; int cbparam需要改成任意lua对象
    public int AddTimer(float ttl, bool bOnce, int cb, int cbparam, bool bLateUpdate)
    {
        if (bLateUpdate)
        {
            return m_LateTimerList.AddTimer(ttl, bOnce, cb, cbparam);
        }
        else
        {
            return m_TimerList.AddTimer(ttl, bOnce, cb, cbparam);
        }
    }

    public void RemoveTimer(int id)
    {
        m_TimerList.RemoveTimer(id);
        m_LateTimerList.RemoveTimer(id);
    }

    public void ResetTimer(int id)
    {
        m_TimerList.ResetTimer(id);
        m_LateTimerList.ResetTimer(id);
    }
}