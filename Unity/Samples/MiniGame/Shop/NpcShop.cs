// ─────────────────────────────────────────────────────────────────────
// NpcShop.cs — NPC 商店系统
//
// 挂到 NPC 上。玩家靠近 → 按 E → 打开商店面板。
// 功能：
//   - 买入/卖出
//   - 商店库存（可刷新）
//   - 好感度折扣（NpcMemory.Affinity 驱动）
//   - 蓝图变量注入（NPC 知道你买了什么）
//   - 交易记录持久化
// ─────────────────────────────────────────────────────────────────────

using System;
using System.Collections.Generic;
using UnityEngine;
using BlueprintRuntime;

namespace BlueprintMiniGame
{
    public class NpcShop : MonoBehaviour
    {
        [Header("商店配置")]
        [SerializeField] private string shopId = "";
        [SerializeField] private string shopName = "杂货店";
        [SerializeField] private ShopItem[] shopItems;

        [Header("好感度折扣")]
        [Tooltip("好感度 100 时的折扣率（0.8 = 八折）")]
        [SerializeField] private float maxDiscount = 0.8f;

        [Header("库存刷新")]
        [SerializeField] private bool  hasLimitedStock = false;
        [SerializeField] private float restockInterval = 300f;  // 秒

        // ── 事件 ────────────────────────────────────────────────────
        public event Action<string, int> OnItemBought;   // itemId, count
        public event Action<string, int> OnItemSold;     // itemId, count
        public event Action              OnShopOpened;

        public string ShopName => shopName;

        [Serializable]
        public class ShopItem
        {
            public string itemId;
            [Tooltip("覆盖买入价（0 = 使用 ItemDef.buyPrice）")]
            public int    priceOverride;
            public int    stock = -1;         // -1 = 无限
            [HideInInspector]
            public int    soldCount;
        }

        private float _restockTimer;
        private Dictionary<string, int> _tradeHistory = new();  // itemId → 买入总数

        void Start()
        {
            if (string.IsNullOrEmpty(shopId))
                shopId = gameObject.name + "_shop";
            LoadTradeHistory();
        }

        void Update()
        {
            if (!hasLimitedStock) return;
            _restockTimer -= Time.deltaTime;
            if (_restockTimer <= 0f)
            {
                _restockTimer = restockInterval;
                Restock();
            }
        }

        // ── 公共 API ────────────────────────────────────────────────

        /// <summary>获取折扣后的买入价格</summary>
        public int GetBuyPrice(ShopItem item)
        {
            var def = Inventory.Instance?.GetItemDef(item.itemId);
            int basePrice = item.priceOverride > 0 ? item.priceOverride : (def?.buyPrice ?? 10);
            float discount = GetDiscount();
            return Mathf.Max(1, Mathf.RoundToInt(basePrice * discount));
        }

        /// <summary>获取卖出价格（通常是买价的一半）</summary>
        public int GetSellPrice(string itemId)
        {
            var def = Inventory.Instance?.GetItemDef(itemId);
            return def?.sellPrice ?? 5;
        }

        /// <summary>买入物品</summary>
        public bool Buy(ShopItem item, int count = 1)
        {
            if (Inventory.Instance == null || PlayerStats.Instance == null) return false;

            int price = GetBuyPrice(item) * count;
            if (PlayerStats.Instance.Gold < price) return false;

            // 库存检查
            if (hasLimitedStock && item.stock >= 0 && item.stock < count) return false;

            int added = Inventory.Instance.AddItem(item.itemId, count);
            if (added <= 0) return false;

            int actualCost = GetBuyPrice(item) * added;
            PlayerStats.Instance.AddGold(-actualCost);

            if (hasLimitedStock && item.stock >= 0)
                item.stock -= added;

            // 记录交易
            if (!_tradeHistory.ContainsKey(item.itemId))
                _tradeHistory[item.itemId] = 0;
            _tradeHistory[item.itemId] += added;
            SaveTradeHistory();

            OnItemBought?.Invoke(item.itemId, added);
            return true;
        }

        /// <summary>卖出物品</summary>
        public bool Sell(string itemId, int count = 1)
        {
            if (Inventory.Instance == null || PlayerStats.Instance == null) return false;
            if (!Inventory.Instance.HasItem(itemId, count)) return false;

            var def = Inventory.Instance.GetItemDef(itemId);
            if (def != null && !def.tradeable) return false;

            int removed = Inventory.Instance.RemoveItem(itemId, count);
            if (removed <= 0) return false;

            int revenue = GetSellPrice(itemId) * removed;
            PlayerStats.Instance.AddGold(revenue);

            OnItemSold?.Invoke(itemId, removed);
            return true;
        }

        public IReadOnlyList<ShopItem> Items => shopItems;

        /// <summary>注入商品信息到蓝图（NPC 推荐商品用）</summary>
        public void InjectInto(BPRunner runner)
        {
            if (runner == null || shopItems == null) return;
            var sb = new System.Text.StringBuilder();
            foreach (var item in shopItems)
            {
                var def = Inventory.Instance?.GetItemDef(item.itemId);
                string name = def?.displayName ?? item.itemId;
                int price = GetBuyPrice(item);
                string stock = (hasLimitedStock && item.stock >= 0) ? $"库存{item.stock}" : "充足";
                sb.Append($"{name}({price}金币,{stock}); ");
            }
            runner.SetVariable("ShopItems", sb.ToString().TrimEnd());
        }

        // ── 内部 ────────────────────────────────────────────────────

        private float GetDiscount()
        {
            var mem = GetComponent<NpcMemory>();
            if (mem == null) return 1f;
            float affinity = mem.Affinity;
            // 好感度 0→100 映射到 1.0→maxDiscount
            return Mathf.Lerp(1f, maxDiscount, affinity / 100f);
        }

        private void Restock()
        {
            if (shopItems == null) return;
            foreach (var item in shopItems)
            {
                if (item.stock < 0) continue;
                item.stock = Mathf.Max(item.stock, 3);  // 最少补到 3
            }
        }

        private void SaveTradeHistory()
        {
            var wrapper = new TradeWrapper();
            foreach (var kv in _tradeHistory)
                wrapper.items.Add(new TradeEntry { id = kv.Key, count = kv.Value });
            BlueprintStorage.SetString($"shop_{shopId}_trades", JsonUtility.ToJson(wrapper));
        }

        private void LoadTradeHistory()
        {
            string json = BlueprintStorage.GetString($"shop_{shopId}_trades", "");
            if (string.IsNullOrEmpty(json)) return;
            try
            {
                var wrapper = JsonUtility.FromJson<TradeWrapper>(json);
                if (wrapper?.items != null)
                    foreach (var e in wrapper.items)
                        _tradeHistory[e.id] = e.count;
            }
            catch { }
        }

        [Serializable] private class TradeEntry { public string id; public int count; }
        [Serializable] private class TradeWrapper { public List<TradeEntry> items = new(); }
    }
}
