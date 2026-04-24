// ─────────────────────────────────────────────────────────────────────
// BlueprintShopNodes.cs — NPC 商店蓝图节点
//
// 节点：Shop.Buy / Shop.Sell / Shop.GetPrice / Shop.ListItems
// 让蓝图驱动买卖逻辑（NPC 对话中触发交易）
// ─────────────────────────────────────────────────────────────────────

using UnityEngine;
using BlueprintRuntime;

namespace BlueprintMiniGame
{
    public static class BlueprintShopNodes
    {
        public static void RegisterAll(BPRunner runner)
        {
            // ── Shop.Buy ────────────────────────────────────────────
            runner.RegisterHandler("Shop.Buy", (ctx) =>
            {
                string npcId = ctx.GetInputString("NpcId");
                string itemId = ctx.GetInputString("ItemId");
                int count = ctx.GetInputInt("Count");
                if (count <= 0) count = 1;

                var shop = FindShop(npcId);
                if (shop == null)
                {
                    ctx.LogError($"Shop.Buy: no shop found on NPC '{npcId}'");
                    ctx.ActivateOutputFlow("onError");
                    return;
                }

                var shopItem = FindShopItem(shop, itemId);
                if (shopItem == null)
                {
                    ctx.Print($"这个商品不在售卖列表里");
                    ctx.ActivateOutputFlow("onNotFound");
                    return;
                }

                int price = shop.GetBuyPrice(shopItem) * count;
                if (PlayerStats.Instance == null || PlayerStats.Instance.Gold < price)
                {
                    ctx.Print($"金币不足（需要 {price}G）");
                    ctx.SetOutputInt("Cost", price);
                    ctx.ActivateOutputFlow("onPoor");
                    return;
                }

                bool ok = shop.Buy(shopItem, count);
                if (ok)
                {
                    var def = Inventory.Instance?.GetItemDef(itemId);
                    string name = def?.displayName ?? itemId;
                    ctx.Print($"购买了 {name} x{count}，花费 {price}G");
                    ctx.SetOutputInt("Cost", price);
                    ctx.ActivateOutputFlow("onSuccess");
                }
                else
                {
                    ctx.ActivateOutputFlow("onError");
                }
            });

            // ── Shop.Sell ───────────────────────────────────────────
            runner.RegisterHandler("Shop.Sell", (ctx) =>
            {
                string npcId = ctx.GetInputString("NpcId");
                string itemId = ctx.GetInputString("ItemId");
                int count = ctx.GetInputInt("Count");
                if (count <= 0) count = 1;

                var shop = FindShop(npcId);
                if (shop == null)
                {
                    ctx.LogError($"Shop.Sell: no shop found on NPC '{npcId}'");
                    ctx.ActivateOutputFlow("onError");
                    return;
                }

                if (Inventory.Instance == null || !Inventory.Instance.HasItem(itemId, count))
                {
                    ctx.Print("你没有足够的物品可以卖");
                    ctx.ActivateOutputFlow("onInsufficient");
                    return;
                }

                int revenue = shop.GetSellPrice(itemId) * count;
                bool ok = shop.Sell(itemId, count);
                if (ok)
                {
                    var def = Inventory.Instance?.GetItemDef(itemId);
                    string name = def?.displayName ?? itemId;
                    ctx.Print($"卖出了 {name} x{count}，获得 {revenue}G");
                    ctx.SetOutputInt("Revenue", revenue);
                    ctx.ActivateOutputFlow("onSuccess");
                }
                else
                {
                    ctx.ActivateOutputFlow("onError");
                }
            });

            // ── Shop.GetPrice ───────────────────────────────────────
            runner.RegisterHandler("Shop.GetPrice", (ctx) =>
            {
                string npcId = ctx.GetInputString("NpcId");
                string itemId = ctx.GetInputString("ItemId");

                var shop = FindShop(npcId);
                if (shop == null)
                {
                    ctx.SetOutputInt("BuyPrice", 0);
                    ctx.SetOutputInt("SellPrice", 0);
                    return;
                }

                var shopItem = FindShopItem(shop, itemId);
                int buyPrice = shopItem != null ? shop.GetBuyPrice(shopItem) : 0;
                int sellPrice = shop.GetSellPrice(itemId);

                ctx.SetOutputInt("BuyPrice", buyPrice);
                ctx.SetOutputInt("SellPrice", sellPrice);
            });

            // ── Shop.ListItems ──────────────────────────────────────
            runner.RegisterHandler("Shop.ListItems", (ctx) =>
            {
                string npcId = ctx.GetInputString("NpcId");
                var shop = FindShop(npcId);
                if (shop == null)
                {
                    ctx.SetOutputString("ItemList", "");
                    ctx.SetOutputInt("Count", 0);
                    return;
                }

                var sb = new System.Text.StringBuilder();
                int count = 0;
                foreach (var item in shop.Items)
                {
                    var def = Inventory.Instance?.GetItemDef(item.itemId);
                    string name = def?.displayName ?? item.itemId;
                    int price = shop.GetBuyPrice(item);
                    sb.Append($"{name}({price}G); ");
                    count++;
                }

                ctx.SetOutputString("ItemList", sb.ToString().TrimEnd());
                ctx.SetOutputInt("Count", count);
            });

            Debug.Log("[BlueprintShopNodes] Registered 4 nodes: Shop.Buy/Sell/GetPrice/ListItems");
        }

        // ── 辅助 ────────────────────────────────────────────────────

        private static NpcShop FindShop(string npcId)
        {
            if (string.IsNullOrEmpty(npcId))
            {
                // 没指定 NPC，找最近的
                return Object.FindObjectOfType<NpcShop>();
            }

            foreach (var shop in Object.FindObjectsOfType<NpcShop>())
            {
                var mem = shop.GetComponent<NpcMemory>();
                if (mem != null && mem.NpcId == npcId) return shop;
                if (shop.gameObject.name == npcId) return shop;
            }
            return null;
        }

        private static NpcShop.ShopItem FindShopItem(NpcShop shop, string itemId)
        {
            foreach (var item in shop.Items)
                if (item.itemId == itemId) return item;
            return null;
        }
    }
}
