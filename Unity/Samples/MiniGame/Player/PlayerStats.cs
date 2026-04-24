// PlayerStats.cs
// ─────────────────────────────────────────────────────────────────────────────
// 玩家属性 — 金币、等级、经验、装备、偏好
//
// 设计原则：
//   1. 单例，跨场景存在（DontDestroyOnLoad）
//   2. 跨会话持久化（BlueprintStorage）
//   3. 自动注入到所有 NPC runner 变量：
//        PlayerGold       = "100"
//        PlayerLevel      = "5"
//        PlayerEquipment  = "木剑; 布甲"
//        PlayerName       = "玩家昵称"
//      → NPC 对话时能"看到"玩家状态
//        例："你拿这把破木剑去打怪肯定不行吧喵~ 要不要买我的好剑？"
//
// 扩展：
//   - 如果你的游戏有更复杂属性（HP/MP/技能树），加字段到 PlayerStatsData 即可
//   - 加字段后记得在 InjectInto() 里注入相应变量
// ─────────────────────────────────────────────────────────────────────────────

using System;
using System.Collections.Generic;
using UnityEngine;

namespace BlueprintRuntime.Samples.MiniGame
{
    [Serializable]
    public class PlayerStatsData
    {
        public string        playerName  = "旅者";
        public int           gold        = 0;
        public int           level       = 1;
        public int           exp         = 0;
        public List<string>  equipment   = new List<string>();   // 已装备物品名
        public List<string>  inventory   = new List<string>();   // 背包物品名
        public long          playTime    = 0;   // 秒
        public long          firstLoginAt = 0;
    }

    [DefaultExecutionOrder(-45)]
    public class PlayerStats : MonoBehaviour
    {
        public static PlayerStats Instance { get; private set; }

        [SerializeField] private string storageKey = "Player.stats";

        [Header("升级公式（可选）")]
        [Tooltip("升级所需经验：base + level * step")]
        [SerializeField] private int expBase = 100;
        [SerializeField] private int expStep = 50;

        public PlayerStatsData Data { get; private set; } = new PlayerStatsData();

        public event Action          OnStatsChanged;
        public event Action<int>     OnGoldChanged;
        public event Action<int>     OnLevelUp;
        public event Action<string>  OnItemGained;
        public event Action<string>  OnItemEquipped;

        void Awake()
        {
            if (Instance != null && Instance != this) { Destroy(gameObject); return; }
            Instance = this;
            DontDestroyOnLoad(gameObject);

            Load();
            if (Data.firstLoginAt == 0) Data.firstLoginAt = DateTimeOffset.UtcNow.ToUnixTimeSeconds();
            Save();
        }

        void Update()
        {
            // 累计游戏时间（秒）
            Data.playTime += (long)(Time.unscaledDeltaTime * 1000) / 1000;
            // 为避免每帧写盘，每 30 秒保存一次
            if (Time.frameCount % 1800 == 0) Save();
        }

        // ── 金币 ──────────────────────────────────────────────────
        public void AddGold(int delta)
        {
            if (delta == 0) return;
            Data.gold = Mathf.Max(0, Data.gold + delta);
            OnGoldChanged?.Invoke(Data.gold);
            Changed();
        }

        public void SetGold(int value)
        {
            Data.gold = Mathf.Max(0, value);
            OnGoldChanged?.Invoke(Data.gold);
            Changed();
        }

        public bool TrySpendGold(int cost)
        {
            if (cost < 0) return false;
            if (Data.gold < cost) return false;
            AddGold(-cost);
            return true;
        }

        // ── 经验 / 升级 ───────────────────────────────────────────
        public void SetLevel(int value)
        {
            Data.level = Mathf.Max(1, value);
            Changed();
        }

        public void SetName(string name)
        {
            if (!string.IsNullOrEmpty(name))
            {
                Data.playerName = name;
                Changed();
            }
        }

        public void AddExp(int amount)
        {
            if (amount <= 0) return;
            Data.exp += amount;
            while (Data.exp >= RequiredExpForNextLevel())
            {
                Data.exp -= RequiredExpForNextLevel();
                Data.level++;
                OnLevelUp?.Invoke(Data.level);
                WorldState.Instance?.AddEvent($"玩家升到了 {Data.level} 级");
            }
            Changed();
        }

        public int RequiredExpForNextLevel() => expBase + Data.level * expStep;

        // ── 背包 / 装备 ───────────────────────────────────────────
        public void GainItem(string itemName)
        {
            if (string.IsNullOrEmpty(itemName)) return;
            Data.inventory.Add(itemName);
            OnItemGained?.Invoke(itemName);
            Changed();
        }

        public bool ConsumeItem(string itemName)
        {
            if (Data.inventory.Remove(itemName)) { Changed(); return true; }
            return false;
        }

        public void Equip(string itemName)
        {
            if (!Data.inventory.Contains(itemName) && !Data.equipment.Contains(itemName))
                return;
            if (Data.inventory.Remove(itemName))
                Data.equipment.Add(itemName);
            OnItemEquipped?.Invoke(itemName);
            Changed();
        }

        public void Unequip(string itemName)
        {
            if (Data.equipment.Remove(itemName))
            {
                Data.inventory.Add(itemName);
                Changed();
            }
        }

        public void SetPlayerName(string name)
        {
            if (string.IsNullOrWhiteSpace(name)) return;
            Data.playerName = name.Trim();
            Changed();
        }

        // ── 注入到 NPC runner ──────────────────────────────────────
        public void InjectInto(BPRunner runner)
        {
            if (runner == null) return;
            runner.SetVariable("PlayerName",      Data.playerName);
            runner.SetVariable("PlayerGold",      Data.gold);
            runner.SetVariable("PlayerLevel",     Data.level);
            runner.SetVariable("PlayerEquipment", Data.equipment.Count > 0 ? string.Join("；", Data.equipment) : "无");
            runner.SetVariable("PlayerInventory", Data.inventory.Count > 0 ? string.Join("；", Data.inventory) : "空");
        }

        // ── 持久化 ────────────────────────────────────────────────
        private void Load()
        {
            Data = BlueprintStorage.GetJson<PlayerStatsData>(storageKey) ?? new PlayerStatsData();
        }

        public void Save() => BlueprintStorage.SetJson(storageKey, Data);

        public void Wipe()
        {
            Data = new PlayerStatsData();
            BlueprintStorage.Remove(storageKey);
            Changed();
        }

        private void Changed()
        {
            Save();
            OnStatsChanged?.Invoke();
        }
    }
}
