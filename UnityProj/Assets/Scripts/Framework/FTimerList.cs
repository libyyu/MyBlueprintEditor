using CutRope.Framework;
using System;
using System.Collections.Generic;
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

/// <summary>
/// 纯逻辑计时器列表，供 FTimerListBehavior 驱动。
/// 支持一次性 / 循环定时，回调为 LuaFunction，可选传入 LuaTable 参数。
/// Tick 期间新增 / 删除操作安全（暂存至 m_TempList / m_TempDelList）。
/// </summary>
public class FTimerList
{
    public delegate void TimerCallback();

    static int _uniqueid = 1;
    public int total_count = 0;

    public struct Timer
    {
        public int id;
        public float ttl;
        public float end_time;
        public TimerCallback callback;
        public bool bOnce;
    }

    List<Timer> m_List        = new List<Timer>();
    List<Timer> m_TempList    = new List<Timer>();
    List<int>   m_TempDelList = new List<int>();
    bool        m_bTick       = false;

    LuaEnv getEnv()
    {
        if (!LuaManager.Instance || LuaManager.Instance.ActiveLuaEnv == null)
            return null;
        return LuaManager.Instance.ActiveLuaEnv;
    }

    public int GetEnableCount() { return m_List.Count + m_TempList.Count - m_TempDelList.Count; }

    // ── 释放单个 Timer 持有的 Lua 引用 ────────────────────────────────────
    static void DisposeTimer(ref Timer tm)
    {
        //if (tm.callback != null) { tm.callback.Dispose(); tm.callback = null; }
    }

    // ════════════════════════════════════════════════════════════════════════
    // 公共 API
    // ════════════════════════════════════════════════════════════════════════

    /// <summary>
    /// 添加计时器。
    /// </summary>
    /// <param name="ttl">间隔秒数</param>
    /// <param name="bOnce">true = 触发一次后自动移除；false = 循环触发</param>
    /// <param name="callback">Lua 回调函数（不能为 null）</param>
    /// <param name="cbparam">透传给回调的 Lua Table 参数，null 表示无参</param>
    /// <returns>计时器 ID，可用于 RemoveTimer / ResetTimer</returns>
    public int AddTimer(float ttl, bool bOnce, TimerCallback callback)
    {
        if (callback == null)
            throw new ArgumentNullException("callback", "AddTimer: callback is null");

        Timer tm;
        tm.id       = _uniqueid++;
        tm.ttl      = ttl;
        tm.end_time = Time.time + ttl;
        tm.callback = callback;
        tm.bOnce    = bOnce;

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
        {
            m_TempDelList.Add(id);
            return;
        }

        for (int i = 0; i < m_List.Count; i++)
        {
            Timer tm = m_List[i];
            if (tm.id == id)
            {
                DisposeTimer(ref tm);
                m_List.RemoveAt(i);
                total_count--;
                return;
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

    public void Tick(float curTime)
    {
        if (m_List.Count == 0)
            return;

        int i = 0;
        m_bTick = true;

        while (i < m_List.Count)
        {
            Timer tm = m_List[i];

            if (tm.end_time <= curTime)
            {
                // 触发回调：有参数则传入 LuaTable，否则无参调用
                if (tm.callback != null)
                {
                    tm.callback?.Invoke();
                }

                if (tm.bOnce)
                {
                    DisposeTimer(ref tm);
                    m_List.RemoveAt(i);
                    total_count--;
                }
                else
                {
                    tm.end_time = curTime + tm.ttl;
                    m_List[i] = tm;
                    i++;
                }
            }
            else
            {
                i++;
            }
        }

        m_bTick = false;

        // 补入 Tick 期间新增的计时器
        if (m_TempList.Count > 0)
        {
            foreach (var tm in m_TempList)
                m_List.Add(tm);
            total_count += m_TempList.Count;
            m_TempList.Clear();
        }

        // 处理 Tick 期间的延迟删除
        if (m_TempDelList.Count > 0)
        {
            foreach (int delId in m_TempDelList)
                RemoveTimer(delId);
            m_TempDelList.Clear();
        }
    }

    public void Clear()
    {
        if (m_List.Count == 0)
            return;

        total_count -= m_List.Count;
        for (int i = 0; i < m_List.Count; i++)
        {
            var tm = m_List[i];
            DisposeTimer(ref tm);
        }
        m_List.Clear();
    }

    // ── 静态注册表（用于调试输出所有活跃计时器）─────────────────────────
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

/// <summary>
/// 挂在 GameObject 上的计时器驱动组件。
/// 管理 Update 和 LateUpdate 两条计时器链。
/// Lua 侧通过 CS.FTimerListBehavior 访问。
/// </summary>
[LuaCallCSharp]
public class FTimerListBehavior : MonoBehaviour
{
    float     CurTime;
    FTimerList m_TimerList     = new FTimerList();
    FTimerList m_LateTimerList = new FTimerList();

#if UNITY_EDITOR
    public int timer_num = 0;
#endif

    public static FTimerListBehavior Instance { get; private set; }

    void Awake()
    {
        if (Instance != null && Instance != this)
        {
            Destroy(gameObject);
            return;
        }
        Instance = this;
        DontDestroyOnLoad(gameObject);
        FTimerList.RegisterTimerList(m_TimerList,     gameObject);
        FTimerList.RegisterTimerList(m_LateTimerList, gameObject);
    }

    void OnDestroy()
    {
        FTimerList.UnregisterTimerList(m_TimerList);
        FTimerList.UnregisterTimerList(m_LateTimerList);

        m_TimerList.Clear();
        m_LateTimerList.Clear();

        if (Instance == this) Instance = null;
    }

    void Update()
    {
        CurTime = Time.time;
        m_TimerList.Tick(CurTime);

#if UNITY_EDITOR
        timer_num = m_TimerList.total_count;
#endif
    }

    void LateUpdate()
    {
        m_LateTimerList.Tick(CurTime);
    }

    // ── Lua 侧调用接口 ────────────────────────────────────────────────────

    /// <summary>
    /// 添加计时器。
    /// </summary>
    /// <param name="ttl">间隔秒数</param>
    /// <param name="bOnce">是否只触发一次</param>
    /// <param name="callback">Lua 回调函数</param>
    /// <param name="cbparam">透传给回调的 Lua Table（可为 nil）</param>
    /// <param name="bLateUpdate">true = 在 LateUpdate 触发；false = 在 Update 触发</param>
    /// <returns>计时器 ID</returns>
    public int AddTimer(float ttl, bool bOnce, FTimerList.TimerCallback callback, bool bLateUpdate)
    {
        if (bLateUpdate)
            return m_LateTimerList.AddTimer(ttl, bOnce, callback);
        else
            return m_TimerList.AddTimer(ttl, bOnce, callback);
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
