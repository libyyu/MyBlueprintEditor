// ─────────────────────────────────────────────────────────────────────
// BlueprintItemNodes.cs — 物品系统蓝图节点
//
// 节点：Item.Give / Item.Remove / Item.Check / Item.Use
// 注册后蓝图里可以直接操作背包。
// ─────────────────────────────────────────────────────────────────────

using UnityEngine;
using BlueprintRuntime;

namespace BlueprintRuntime.Samples.MiniGame
{
    public static class BlueprintItemNodes
    {
        public static void RegisterAll(BPRunner runner)
        {
            // ── Item.Give ───────────────────────────────────────────
            runner.RegisterHandler("Item.Give", (ctx) =>
            {
                if (Inventory.Instance == null) { ctx.LogError("Inventory not found"); return false; }
                string id = ctx.GetInputString("ItemId");
                int count = (int)ctx.GetInputInt("Count");
                if (count <= 0) count = 1;

                int added = Inventory.Instance.AddItem(id, count);
                var def = Inventory.Instance.GetItemDef(id);
                string name = def?.displayName ?? id;

                ctx.SetOutputInt("Added", added);
                ctx.Print($"获得 {name} x{added}");
                ctx.ActivateOutputFlow(added > 0 ? "onSuccess" : "onFull");
                return true;
            });

            // ── Item.Remove ─────────────────────────────────────────
            runner.RegisterHandler("Item.Remove", (ctx) =>
            {
                if (Inventory.Instance == null) { ctx.LogError("Inventory not found"); return false; }
                string id = ctx.GetInputString("ItemId");
                int count = (int)ctx.GetInputInt("Count");
                if (count <= 0) count = 1;

                int removed = Inventory.Instance.RemoveItem(id, count);
                ctx.SetOutputInt("Removed", removed);
                ctx.ActivateOutputFlow(removed >= count ? "onSuccess" : "onInsufficient");
                return true;
            });

            // ── Item.Check ──────────────────────────────────────────
            runner.RegisterHandler("Item.Check", (ctx) =>
            {
                if (Inventory.Instance == null)
                {
                    ctx.SetOutputInt("Count", 0);
                    ctx.SetOutputBool("HasItem", false);
                    return false;
                }
                string id = ctx.GetInputString("ItemId");
                int need = (int)ctx.GetInputInt("RequiredCount");
                if (need <= 0) need = 1;

                int has = Inventory.Instance.GetItemCount(id);
                ctx.SetOutputInt("Count", has);
                ctx.SetOutputBool("HasItem", has >= need);
                return true;
            });

            // ── Item.Use ────────────────────────────────────────────
            runner.RegisterHandler("Item.Use", (ctx) =>
            {
                if (Inventory.Instance == null) { ctx.LogError("Inventory not found"); return false; }
                string id = ctx.GetInputString("ItemId");

                if (!Inventory.Instance.HasItem(id))
                {
                    ctx.ActivateOutputFlow("onNotFound");
                    return true;
                }

                Inventory.Instance.RemoveItem(id, 1);
                var def = Inventory.Instance.GetItemDef(id);
                string name = def?.displayName ?? id;
                ctx.Print($"使用了 {name}");
                ctx.ActivateOutputFlow("onUsed");
                return true;
            });

            Debug.Log("[BlueprintItemNodes] Registered 4 nodes: Item.Give/Remove/Check/Use");
        }
    }
}
