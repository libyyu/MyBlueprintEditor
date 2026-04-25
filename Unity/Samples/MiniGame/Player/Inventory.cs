// ─────────────────────────────────────────────────────────────────────
// Inventory.cs — 玩家背包/物品系统
//
// 功能：
//   - 物品定义（ItemDef）：id / 名称 / 描述 / 图标 / 类型 / 堆叠上限 / 价格
//   - 背包容量限制
//   - 增删查改 + 堆叠
//   - 自动持久化（BlueprintStorage）
//   - 注入蓝图变量（NPC 知道你身上有什么）
//   - 事件通知 UI
// ─────────────────────────────────────────────────────────────────────

using System;
using System.Collections.Generic;
using UnityEngine;
using BlueprintRuntime;

namespace BlueprintRuntime.Samples.MiniGame
{
    // ── 物品定义 ────────────────────────────────────────────────────
    public enum ItemType { Consumable, Equipment, Quest, Gift, Material, Key }

    [Serializable]
    public class ItemDef
    {
        public string   id;
        public string   displayName;
        [TextArea(1, 3)]
        public string   description;
        public Sprite   icon;
        public ItemType type       = ItemType.Consumable;
        public int      maxStack   = 99;
        public int      buyPrice   = 10;
        public int      sellPrice  = 5;
        public bool     tradeable  = true;
    }

    // ── 背包槽 ──────────────────────────────────────────────────────
    [Serializable]
    public class InventorySlot
    {
        public string itemId;
        public int    count;
    }

    // ── 背包主体 ────────────────────────────────────────────────────
    public class Inventory : MonoBehaviour
    {
        public static Inventory Instance { get; private set; }

        [Header("配置")]
        [SerializeField] private int maxSlots = 30;
        [SerializeField] private ItemDef[] itemDatabase;

        [Header("持久化")]
        [SerializeField] private string storageKey = "bp_inventory";

        // ── 事件 ────────────────────────────────────────────────────
        public event Action<string, int> OnItemAdded;      // itemId, count
        public event Action<string, int> OnItemRemoved;    // itemId, count
        public event Action              OnInventoryChanged;

        private List<InventorySlot> _slots = new();
        private Dictionary<string, ItemDef> _db = new();

        void Awake()
        {
            if (Instance != null) { Destroy(gameObject); return; }
            Instance = this;

            // 建索引
            if (itemDatabase != null)
                foreach (var def in itemDatabase)
                    if (def != null && !string.IsNullOrEmpty(def.id))
                        _db[def.id] = def;

            Load();
        }

        // ── 查询 ────────────────────────────────────────────────────

        public ItemDef GetItemDef(string itemId)
        {
            _db.TryGetValue(itemId, out var def);
            return def;
        }

        public int GetItemCount(string itemId)
        {
            int total = 0;
            foreach (var s in _slots)
                if (s.itemId == itemId) total += s.count;
            return total;
        }

        public bool HasItem(string itemId, int count = 1) => GetItemCount(itemId) >= count;

        public IReadOnlyList<InventorySlot> Slots => _slots;
        public int UsedSlots => _slots.Count;
        public int MaxSlots => maxSlots;

        // ── 增加 ────────────────────────────────────────────────────

        /// <summary>添加物品。返回实际添加的数量（可能因为背包满而少于 count）</summary>
        public int AddItem(string itemId, int count = 1)
        {
            if (count <= 0 || string.IsNullOrEmpty(itemId)) return 0;
            var def = GetItemDef(itemId);
            int maxStack = def?.maxStack ?? 99;
            int added = 0;

            // 先填已有槽
            foreach (var s in _slots)
            {
                if (s.itemId != itemId) continue;
                int space = maxStack - s.count;
                if (space <= 0) continue;
                int fill = Mathf.Min(space, count - added);
                s.count += fill;
                added += fill;
                if (added >= count) break;
            }

            // 再开新槽
            while (added < count && _slots.Count < maxSlots)
            {
                int fill = Mathf.Min(maxStack, count - added);
                _slots.Add(new InventorySlot { itemId = itemId, count = fill });
                added += fill;
            }

            if (added > 0)
            {
                Save();
                OnItemAdded?.Invoke(itemId, added);
                OnInventoryChanged?.Invoke();
            }
            return added;
        }

        // ── 减少 ────────────────────────────────────────────────────

        /// <summary>移除物品。返回实际移除数量</summary>
        public int RemoveItem(string itemId, int count = 1)
        {
            if (count <= 0) return 0;
            int removed = 0;

            for (int i = _slots.Count - 1; i >= 0 && removed < count; i--)
            {
                if (_slots[i].itemId != itemId) continue;
                int take = Mathf.Min(_slots[i].count, count - removed);
                _slots[i].count -= take;
                removed += take;
                if (_slots[i].count <= 0) _slots.RemoveAt(i);
            }

            if (removed > 0)
            {
                Save();
                OnItemRemoved?.Invoke(itemId, removed);
                OnInventoryChanged?.Invoke();
            }
            return removed;
        }

        // ── 蓝图注入 ────────────────────────────────────────────────

        /// <summary>注入背包摘要到蓝图变量 InventoryItems</summary>
        public void InjectInto(BPRunner runner)
        {
            if (runner == null) return;
            var sb = new System.Text.StringBuilder();
            foreach (var s in _slots)
            {
                var def = GetItemDef(s.itemId);
                string name = def?.displayName ?? s.itemId;
                sb.Append($"{name}x{s.count}; ");
            }
            runner.SetVariable("InventoryItems", sb.ToString().TrimEnd());
        }

        // ── 持久化 ──────────────────────────────────────────────────

        private void Save()
        {
            var data = new SlotListWrapper { slots = _slots };
            string json = JsonUtility.ToJson(data);
            BlueprintStorage.SetString(storageKey, json);
        }

        private void Load()
        {
            string json = BlueprintStorage.GetString(storageKey, "");
            if (!string.IsNullOrEmpty(json))
            {
                try
                {
                    var data = JsonUtility.FromJson<SlotListWrapper>(json);
                    if (data?.slots != null) _slots = data.slots;
                }
                catch { _slots = new List<InventorySlot>(); }
            }
        }

        [Serializable]
        private class SlotListWrapper
        {
            public List<InventorySlot> slots;
        }
    }
}
