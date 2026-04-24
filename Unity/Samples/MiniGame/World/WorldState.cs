// WorldState.cs
// ─────────────────────────────────────────────────────────────────────────────
// 全局世界状态 — 多 NPC 共享的游戏世界数据
//
// 用途：
//   - 时间系统（时段、游戏内日期、天气）
//   - 全局事件（玩家完成的任务、解锁的地点）
//   - NPC 间共享的"公共记忆"（村里发生的大事）
//
// 自动注入到所有 NPC 蓝图：
//   每次 NPC.Say 之前，本类把世界状态写入 Runner 变量：
//     WorldTime      → "清晨" / "中午" / "傍晚" / "深夜"
//     WorldWeather   → "晴" / "雨" / "雪" / "雾"
//     WorldDay       → 游戏内第几天
//     RecentEvents   → 最近 3 条全局事件（NPC 会"听说"这些事）
//
// 这样蓝图里的 SystemPrompt 自动拼上这些：
//   "现在是{WorldTime}，天气{WorldWeather}。最近村里发生了：{RecentEvents}"
//
// 跨会话持久化：每次 SetTime/SetWeather/AddEvent 都自动 Save。
// ─────────────────────────────────────────────────────────────────────────────

using System;
using System.Collections.Generic;
using UnityEngine;

namespace BlueprintRuntime.Samples.MiniGame
{
    public enum WorldTime    { Dawn, Noon, Dusk, Night }
    public enum WorldWeather { Sunny, Rainy, Snowy, Foggy, Cloudy }

    [Serializable]
    public class WorldStateData
    {
        public int          day          = 1;
        public WorldTime    time         = WorldTime.Dawn;
        public WorldWeather weather      = WorldWeather.Sunny;
        public List<string> recentEvents = new List<string>();   // 最多 20 条
        public long         lastUpdated  = 0;
    }

    [DefaultExecutionOrder(-50)]   // 早于 NpcMemory（-30 隐式）和 Controller
    public class WorldState : MonoBehaviour
    {
        public static WorldState Instance { get; private set; }

        [Header("存档 Key")]
        [SerializeField] private string storageKey = "World.state";

        [Header("初始状态")]
        [SerializeField] private WorldTime    initialTime    = WorldTime.Dawn;
        [SerializeField] private WorldWeather initialWeather = WorldWeather.Sunny;

        [Header("自动时段推进")]
        [Tooltip("每多少秒现实时间推进一个时段（Dawn→Noon→Dusk→Night→Dawn）。0 = 不自动推进")]
        [SerializeField] private float secondsPerPhase = 0f;

        public WorldStateData Data { get; private set; } = new WorldStateData();

        /// <summary>世界状态变化时触发（供 UI / NPC 订阅）</summary>
        public event Action OnChanged;

        private float _phaseTimer;

        void Awake()
        {
            if (Instance != null && Instance != this) { Destroy(gameObject); return; }
            Instance = this;
            DontDestroyOnLoad(gameObject);

            Load();
            if (Data.lastUpdated == 0)
            {
                Data.time    = initialTime;
                Data.weather = initialWeather;
                Save();
            }
        }

        void Update()
        {
            if (secondsPerPhase <= 0) return;
            _phaseTimer += Time.unscaledDeltaTime;
            if (_phaseTimer >= secondsPerPhase)
            {
                _phaseTimer = 0;
                AdvancePhase();
            }
        }

        // ── 公共 API ────────────────────────────────────────────────
        public void SetTime(WorldTime t)       { Data.time = t; Persist(); }
        public void SetWeather(WorldWeather w) { Data.weather = w; Persist(); }

        public void AdvancePhase()
        {
            Data.time = (WorldTime)(((int)Data.time + 1) % 4);
            if (Data.time == WorldTime.Dawn) Data.day += 1;
            Persist();
        }

        /// <summary>添加一个全局事件（所有 NPC 将"听说"此事）</summary>
        public void AddEvent(string evt)
        {
            if (string.IsNullOrWhiteSpace(evt)) return;
            Data.recentEvents.Add(evt);
            while (Data.recentEvents.Count > 20) Data.recentEvents.RemoveAt(0);
            Persist();
        }

        /// <summary>读取最近 N 条事件（用分号连接）。N 太大会烧 token</summary>
        public string GetRecentEventsText(int count = 3)
        {
            int n = Data.recentEvents.Count;
            if (n == 0) return "无";
            int start = Mathf.Max(0, n - count);
            var slice = Data.recentEvents.GetRange(start, n - start);
            return string.Join("；", slice);
        }

        public string GetTimeText() => Data.time switch
        {
            WorldTime.Dawn  => "清晨",
            WorldTime.Noon  => "中午",
            WorldTime.Dusk  => "傍晚",
            _               => "深夜",
        };

        public string GetWeatherText() => Data.weather switch
        {
            WorldWeather.Sunny   => "晴",
            WorldWeather.Rainy   => "雨",
            WorldWeather.Snowy   => "雪",
            WorldWeather.Foggy   => "雾",
            _                    => "多云",
        };

        // ── 把世界状态注入到任意 Runner ─────────────────────────────
        public void InjectInto(BPRunner runner)
        {
            if (runner == null) return;
            runner.SetVariable("WorldTime",    GetTimeText());
            runner.SetVariable("WorldWeather", GetWeatherText());
            runner.SetVariable("WorldDay",     Data.day);
            runner.SetVariable("RecentEvents", GetRecentEventsText(3));
        }

        // ── 持久化 ───────────────────────────────────────────────────
        private void Load()
        {
            Data = BlueprintStorage.GetJson<WorldStateData>(storageKey) ?? new WorldStateData();
        }

        private void Save()
        {
            Data.lastUpdated = DateTimeOffset.UtcNow.ToUnixTimeSeconds();
            BlueprintStorage.SetJson(storageKey, Data);
        }

        private void Persist()
        {
            Save();
            OnChanged?.Invoke();
        }

        public void Wipe()
        {
            Data = new WorldStateData { time = initialTime, weather = initialWeather };
            Save();
            OnChanged?.Invoke();
        }
    }
}
