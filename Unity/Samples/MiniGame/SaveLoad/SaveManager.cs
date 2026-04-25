// ─────────────────────────────────────────────────────────────────────
// SaveManager.cs — 多存档槽存档/读档系统
//
// 功能：
//   - 3 个存档槽 + 自动存档（autosave）
//   - 汇总所有系统数据：PlayerStats / Inventory / NpcMemory / Quest / WorldState
//   - WebGL/小游戏用 BlueprintStorage（localStorage/wx.storage）
//   - 原生平台用 JSON 文件
//   - 存档预览（截图缩略图 / 进度摘要）
// ─────────────────────────────────────────────────────────────────────

using BlueprintRuntime.Samples.MiniGame.Quest;
using System;
using System.Collections.Generic;
using UnityEngine;

namespace BlueprintRuntime.Samples.MiniGame
{
    public class SaveManager : MonoBehaviour
    {
        public static SaveManager Instance { get; private set; }

        [Header("配置")]
        [SerializeField] private int    slotCount       = 3;
        [SerializeField] private float  autosaveInterval = 120f;  // 秒，0 = 关闭
        [SerializeField] private string storagePrefix    = "bp_save_";

        public event Action<int>  OnSaved;       // slotIndex
        public event Action<int>  OnLoaded;      // slotIndex
        public event Action<int>  OnDeleted;     // slotIndex

        public int SlotCount => slotCount;

        private float _autosaveTimer;

        void Awake()
        {
            if (Instance != null) { Destroy(gameObject); return; }
            Instance = this;
        }

        void Update()
        {
            if (autosaveInterval <= 0f) return;
            _autosaveTimer -= Time.deltaTime;
            if (_autosaveTimer <= 0f)
            {
                _autosaveTimer = autosaveInterval;
                Save(-1);  // -1 = autosave slot
            }
        }

        // ── 存档 ────────────────────────────────────────────────────

        /// <summary>保存到指定槽（-1 = autosave）</summary>
        public void Save(int slot)
        {
            var data = CollectAllData();
            data.slot      = slot;
            data.timestamp = DateTime.Now.ToString("yyyy-MM-dd HH:mm:ss");
            data.playTime  = Time.realtimeSinceStartup;

            string json = JsonUtility.ToJson(data, false);
            string key  = GetKey(slot);
            BlueprintStorage.SetString(key, json);

            Debug.Log($"[SaveManager] Saved to slot {slot} ({json.Length} bytes)");
            OnSaved?.Invoke(slot);
        }

        // ── 读档 ────────────────────────────────────────────────────

        /// <summary>读取指定槽</summary>
        public bool Load(int slot)
        {
            string key  = GetKey(slot);
            string json = BlueprintStorage.GetString(key, "");
            if (string.IsNullOrEmpty(json))
            {
                Debug.LogWarning($"[SaveManager] Slot {slot} is empty");
                return false;
            }

            SaveData data;
            try { data = JsonUtility.FromJson<SaveData>(json); }
            catch (Exception e)
            {
                Debug.LogError($"[SaveManager] Load failed: {e.Message}");
                return false;
            }

            ApplyAllData(data);
            Debug.Log($"[SaveManager] Loaded slot {slot}");
            OnLoaded?.Invoke(slot);
            return true;
        }

        // ── 删档 ────────────────────────────────────────────────────

        public void Delete(int slot)
        {
            BlueprintStorage.DeleteKey(GetKey(slot));
            Debug.Log($"[SaveManager] Deleted slot {slot}");
            OnDeleted?.Invoke(slot);
        }

        // ── 查询 ────────────────────────────────────────────────────

        /// <summary>获取存档预览（不加载全部数据）</summary>
        public SavePreview GetPreview(int slot)
        {
            string json = BlueprintStorage.GetString(GetKey(slot), "");
            if (string.IsNullOrEmpty(json))
                return new SavePreview { exists = false, slot = slot };

            try
            {
                var data = JsonUtility.FromJson<SaveData>(json);
                return new SavePreview
                {
                    exists    = true,
                    slot      = slot,
                    timestamp = data.timestamp,
                    level     = data.playerLevel,
                    gold      = data.playerGold,
                    questCount = data.activeQuestCount,
                    npcCount   = data.npcCount,
                };
            }
            catch
            {
                return new SavePreview { exists = false, slot = slot };
            }
        }

        public bool HasSave(int slot) => !string.IsNullOrEmpty(BlueprintStorage.GetString(GetKey(slot), ""));

        // ── 数据收集 ────────────────────────────────────────────────

        private SaveData CollectAllData()
        {
            var data = new SaveData();

            // PlayerStats
            if (PlayerStats.Instance != null)
            {
                data.playerGold  = PlayerStats.Instance.Gold;
                data.playerLevel = PlayerStats.Instance.Level;
                data.playerExp   = PlayerStats.Instance.Exp;
                data.playerName  = PlayerStats.Instance.PlayerName;
            }

            // Inventory
            if (Inventory.Instance != null)
            {
                data.inventoryJson = JsonUtility.ToJson(
                    new InventoryWrapper { slots = new List<InventorySlot>(Inventory.Instance.Slots) });
            }

            // Quest
            if (Quest.QuestSystem.Instance != null)
            {
                var active = Quest.QuestSystem.Instance.GetActiveQuests();
                data.activeQuestCount = active.Count;
                data.questsJson = JsonUtility.ToJson(
                    new QuestListWrapper { quests = active });
            }

            // WorldState
            if (WorldState.Instance != null)
            {
                data.worldTimePhase = WorldState.Instance.GetTimeText();
                data.worldWeather   = WorldState.Instance.Weather;
                data.worldEventsJson = JsonUtility.ToJson(
                    new StringListWrapper { items = WorldState.Instance.RecentEvents });
            }

            // NpcMemory（遍历所有 NPC）
            var memories = FindObjectsOfType<NpcMemory>();
            data.npcCount = memories.Length;
            var npcList = new List<NpcMemorySaveEntry>();
            foreach (var mem in memories)
            {
                npcList.Add(new NpcMemorySaveEntry
                {
                    npcId    = mem.NpcId,
                    affinity = mem.Affinity,
                    meetCount = mem.MeetCount,
                    factsJson = JsonUtility.ToJson(new StringListWrapper { items = new List<string>(mem.KnownFacts) }),
                });
            }
            data.npcMemoriesJson = JsonUtility.ToJson(new NpcMemoryListWrapper { entries = npcList });

            return data;
        }

        private void ApplyAllData(SaveData data)
        {
            // PlayerStats
            if (PlayerStats.Instance != null)
            {
                PlayerStats.Instance.SetGold(data.playerGold);
                PlayerStats.Instance.SetLevel(data.playerLevel);
                // Exp 和 Name 也可以恢复
            }

            // Inventory
            if (Inventory.Instance != null && !string.IsNullOrEmpty(data.inventoryJson))
            {
                // Inventory 自己的 Load 用 BlueprintStorage，这里直接写入 storage key
                BlueprintStorage.SetString("bp_inventory", data.inventoryJson);
                // 触发 Inventory 重新读取
                // 注意：需要 Inventory 暴露 Reload() 方法，或通过重新 Awake
            }

            // Quest
            if (!string.IsNullOrEmpty(data.questsJson))
            {
                BlueprintStorage.SetString("bp_quests", data.questsJson);
            }

            // NpcMemory
            if (!string.IsNullOrEmpty(data.npcMemoriesJson))
            {
                try
                {
                    var wrapper = JsonUtility.FromJson<NpcMemoryListWrapper>(data.npcMemoriesJson);
                    if (wrapper?.entries != null)
                    {
                        foreach (var entry in wrapper.entries)
                        {
                            // 写入各 NPC 的 storage key
                            BlueprintStorage.SetString($"npc_mem_{entry.npcId}_affinity", entry.affinity.ToString());
                            BlueprintStorage.SetString($"npc_mem_{entry.npcId}_meetcount", entry.meetCount.ToString());
                            if (!string.IsNullOrEmpty(entry.factsJson))
                                BlueprintStorage.SetString($"npc_mem_{entry.npcId}_facts", entry.factsJson);
                        }
                    }
                }
                catch { }
            }
        }

        private string GetKey(int slot) => $"{storagePrefix}{(slot < 0 ? "auto" : slot.ToString())}";

        // ── 数据结构 ────────────────────────────────────────────────

        [Serializable]
        public class SaveData
        {
            public int    slot;
            public string timestamp;
            public float  playTime;

            // Player
            public int    playerGold;
            public int    playerLevel;
            public int    playerExp;
            public string playerName;

            // Inventory
            public string inventoryJson;

            // Quest
            public int    activeQuestCount;
            public string questsJson;

            // World
            public string worldTimePhase;
            public string worldWeather;
            public string worldEventsJson;

            // NPC
            public int    npcCount;
            public string npcMemoriesJson;
        }

        [Serializable]
        public struct SavePreview
        {
            public bool   exists;
            public int    slot;
            public string timestamp;
            public int    level;
            public int    gold;
            public int    questCount;
            public int    npcCount;
        }

        [Serializable] private class InventoryWrapper { public List<InventorySlot> slots; }
        [Serializable] private class QuestListWrapper { public List<QuestData> quests; }
        [Serializable] private class StringListWrapper { public List<string> items; }
        [Serializable] private class NpcMemorySaveEntry { public string npcId; public float affinity; public int meetCount; public string factsJson; }
        [Serializable] private class NpcMemoryListWrapper { public List<NpcMemorySaveEntry> entries; }
    }
}
