// TavernManager.cs — 酒馆经营核心
// ─────────────────────────────────────────────────────────────────────────────
// 核心循环：招待客人 → 赚金币 → 升级酒馆 → 解锁新 NPC/新区域 → 新剧情
//
// 酒馆等级决定：
//   Lv.1 破旧小酒馆 → 1 个 NPC（喵喵店员）
//   Lv.2 温馨酒馆   → 2 个 NPC + 解锁铁锤
//   Lv.3 人气酒馆   → 3 个 NPC + 解锁月灵 + 每日随机事件
//   Lv.4 传奇酒馆   → 4 个 NPC + 解锁皮皮 + 主线剧情
//   Lv.5 魔法酒馆   → 全 NPC + 特殊客人 + 结局
// ─────────────────────────────────────────────────────────────────────────────

using System;
using System.Collections.Generic;
using UnityEngine;

namespace BlueprintRuntime.Samples.MiniGame.Tavern
{
    [Serializable]
    public class TavernSaveData
    {
        public int    level       = 1;
        public int    reputation  = 0;     // 声望（客人满意度累积）
        public int    totalGuests = 0;     // 总接待客人数
        public int    todayGuests = 0;
        public int    todayIncome = 0;
        public string lastPlayDate = "";
        public List<string> unlockedNpcs    = new List<string> { "NPC_MeowMeow" };
        public List<string> unlockedAreas   = new List<string> { "Bar" };
        public List<string> completedEvents = new List<string>();
        public int    dayCount = 1;
    }

    [DefaultExecutionOrder(-50)]
    public class TavernManager : MonoBehaviour
    {
        public static TavernManager Instance { get; private set; }

        [SerializeField] private string storageKey = "Tavern.save";

        public TavernSaveData Data { get; private set; } = new TavernSaveData();

        // ── 升级配置 ────────────────────────────────────────────────
        [Serializable]
        public class LevelConfig
        {
            public int    level;
            public string name;           // "破旧小酒馆"
            public int    upgradeCost;     // 升级所需金币
            public int    reputationReq;   // 升级所需声望
            public string[] unlockNpcs;    // 升级后解锁的 NPC
            public string[] unlockAreas;   // 升级后解锁的区域
            public string description;     // 升级描述
        }

        public static readonly LevelConfig[] Levels = {
            new LevelConfig { level = 1, name = "破旧小酒馆", upgradeCost = 0,   reputationReq = 0,
                unlockNpcs = new[]{"NPC_MeowMeow"}, unlockAreas = new[]{"Bar"},
                description = "一间勉强能坐下三个人的小酒馆。" },
            new LevelConfig { level = 2, name = "温馨酒馆",   upgradeCost = 100, reputationReq = 10,
                unlockNpcs = new[]{"NPC_IronHammer"}, unlockAreas = new[]{"Forge"},
                description = "铁锤大叔被你的热情打动，决定在这里安个炉子。" },
            new LevelConfig { level = 3, name = "人气酒馆",   upgradeCost = 300, reputationReq = 30,
                unlockNpcs = new[]{"NPC_Luna"}, unlockAreas = new[]{"Library"},
                description = "月灵术士说这里的星光不错，她想在角落放一个书架。" },
            new LevelConfig { level = 4, name = "传奇酒馆",   upgradeCost = 600, reputationReq = 60,
                unlockNpcs = new[]{"NPC_Pippin"}, unlockAreas = new[]{"SecretRoom"},
                description = "皮皮鼠从地下室冒出来——原来这下面还有密室？！" },
            new LevelConfig { level = 5, name = "魔法酒馆",   upgradeCost = 0,   reputationReq = 100,
                unlockNpcs = new string[0], unlockAreas = new[]{"Rooftop"},
                description = "酒馆屋顶长出了一棵发光的树。整个村庄都在谈论你的酒馆。" },
        };

        // ── 事件 ────────────────────────────────────────────────────
        public event Action OnDataChanged;
        public event Action<int> OnLevelUp;              // newLevel
        public event Action<string> OnNpcUnlocked;        // npcId
        public event Action<string> OnDailyEvent;         // eventText

        // ── 每日随机事件 ────────────────────────────────────────────
        static readonly string[] DailyEvents = {
            "一位旅行商人带来了远方的消息……",
            "酒馆外下起了大雨，客人们不愿离去。",
            "铁锤打造了一把新剑，引来围观。",
            "月灵的占卜水晶突然亮了起来。",
            "皮皮说他在地下室发现了一扇旧门。",
            "一位神秘的蒙面客人走进了酒馆。",
            "今天的酒特别好喝，客人们赞不绝口。",
            "有人在酒馆门口贴了一张悬赏告示。",
            "村长来访，说有重要的事要商量。",
            "一只受伤的小鸟飞进了酒馆。",
        };

        void Awake()
        {
            if (Instance != null && Instance != this) { Destroy(gameObject); return; }
            Instance = this;
            DontDestroyOnLoad(gameObject);
            Load();
            CheckNewDay();
        }

        // ── 核心操作 ────────────────────────────────────────────────

        /// <summary>客人满意（+声望，+金币）</summary>
        public void GuestSatisfied(int gold, int rep = 1)
        {
            Data.totalGuests++;
            Data.todayGuests++;
            Data.todayIncome += gold;
            Data.reputation += rep;

            PlayerStats.Instance?.AddGold(gold);
            Save();
            OnDataChanged?.Invoke();
        }

        /// <summary>尝试升级酒馆</summary>
        public bool TryUpgrade()
        {
            if (Data.level >= Levels.Length) return false;

            var next = Levels[Data.level]; // Levels[0]=Lv1, Levels[1]=Lv2...
            if (Data.reputation < next.reputationReq) return false;
            if (PlayerStats.Instance == null || PlayerStats.Instance.Gold < next.upgradeCost) return false;

            PlayerStats.Instance.AddGold(-next.upgradeCost);
            Data.level++;

            // 解锁 NPC
            foreach (var npc in next.unlockNpcs)
            {
                if (!Data.unlockedNpcs.Contains(npc))
                {
                    Data.unlockedNpcs.Add(npc);
                    OnNpcUnlocked?.Invoke(npc);
                }
            }

            // 解锁区域
            foreach (var area in next.unlockAreas)
            {
                if (!Data.unlockedAreas.Contains(area))
                    Data.unlockedAreas.Add(area);
            }

            Save();
            OnLevelUp?.Invoke(Data.level);
            OnDataChanged?.Invoke();
            return true;
        }

        /// <summary>检查 NPC 是否已解锁</summary>
        public bool IsNpcUnlocked(string npcId) => Data.unlockedNpcs.Contains(npcId);

        /// <summary>当前等级配置</summary>
        public LevelConfig CurrentLevel => Levels[Mathf.Clamp(Data.level - 1, 0, Levels.Length - 1)];

        /// <summary>下一等级配置（已满级返回 null）</summary>
        public LevelConfig NextLevel => Data.level < Levels.Length ? Levels[Data.level] : null;

        // ── 每日重置 ────────────────────────────────────────────────
        void CheckNewDay()
        {
            string today = DateTime.Now.ToString("yyyy-MM-dd");
            if (Data.lastPlayDate != today)
            {
                Data.lastPlayDate = today;
                Data.todayGuests = 0;
                Data.todayIncome = 0;
                Data.dayCount++;

                // Lv.3+ 每日随机事件
                if (Data.level >= 3)
                {
                    string evt = DailyEvents[UnityEngine.Random.Range(0, DailyEvents.Length)];
                    OnDailyEvent?.Invoke(evt);
                }

                Save();
            }
        }

        // ── 注入到蓝图变量 ──────────────────────────────────────────
        public void InjectInto(BPRunner runner)
        {
            if (runner == null) return;
            runner.SetVariable("TavernLevel",    Data.level.ToString());
            runner.SetVariable("TavernName",     CurrentLevel.name);
            runner.SetVariable("TavernRep",      Data.reputation.ToString());
            runner.SetVariable("TodayGuests",    Data.todayGuests.ToString());
            runner.SetVariable("DayCount",       Data.dayCount.ToString());
        }

        // ── 持久化 ──────────────────────────────────────────────────
        void Load()
        {
            Data = BlueprintStorage.GetJson<TavernSaveData>(storageKey) ?? new TavernSaveData();
        }

        void Save() => BlueprintStorage.SetJson(storageKey, Data);
    }
}
