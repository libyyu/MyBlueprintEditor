// ─────────────────────────────────────────────────────────────────────
// BlueprintPlayerNodes.cs — 玩家属性蓝图节点
//
// 节点：Player.AddGold / Player.AddExp / Player.GetStats / Player.SetName
// ─────────────────────────────────────────────────────────────────────

using UnityEngine;
using BlueprintRuntime;

namespace BlueprintRuntime.Samples.MiniGame
{
    public static class BlueprintPlayerNodes
    {
        public static void RegisterAll(BPRunner runner)
        {
            // ── Player.AddGold ──────────────────────────────────────
            BPRunner.RegisterHandler("Player.AddGold", (ctx) =>
            {
                if (PlayerStats.Instance == null) { ctx.LogError("PlayerStats not found"); return false; }
                int amount = (int)ctx.GetInputInt("Amount");
                PlayerStats.Instance.AddGold(amount);
                ctx.SetOutputInt("NewGold", PlayerStats.Instance.Data.gold);
                if (amount > 0)
                    ctx.Print($"获得 {amount} 金币（余额: {PlayerStats.Instance.Data.gold}）");
                else if (amount < 0)
                    ctx.Print($"花费 {-amount} 金币（余额: {PlayerStats.Instance.Data.gold}）");
                ctx.ActivateOutputFlow("Out");
                return true;
            });

            // ── Player.AddExp ───────────────────────────────────────
            BPRunner.RegisterHandler("Player.AddExp", (ctx) =>
            {
                if (PlayerStats.Instance == null) { ctx.LogError("PlayerStats not found"); return false; }
                int amount = (int)ctx.GetInputInt("Amount");
                int oldLevel = PlayerStats.Instance.Data.level;
                PlayerStats.Instance.AddExp(amount);
                int newLevel = PlayerStats.Instance.Data.level;

                ctx.SetOutputInt("NewExp", PlayerStats.Instance.Data.exp);
                ctx.SetOutputInt("NewLevel", newLevel);
                ctx.SetOutputBool("LeveledUp", newLevel > oldLevel);

                if (newLevel > oldLevel)
                {
                    ctx.Print($"获得 {amount} 经验，升级到 Lv.{newLevel}！");
                    ctx.ActivateOutputFlow("onLevelUp");
                }
                else
                {
                    ctx.Print($"获得 {amount} 经验");
                    ctx.ActivateOutputFlow("Out");
                }
                return true;
            });

            // ── Player.GetStats ─────────────────────────────────────
            BPRunner.RegisterHandler("Player.GetStats", (ctx) =>
            {
                if (PlayerStats.Instance == null)
                {
                    ctx.SetOutputInt("Gold", 0);
                    ctx.SetOutputInt("Level", 1);
                    ctx.SetOutputInt("Exp", 0);
                    ctx.SetOutputString("Name", "");
                    return false;
                }
                ctx.SetOutputInt("Gold", PlayerStats.Instance.Data.gold);
                ctx.SetOutputInt("Level", PlayerStats.Instance.Data.level);
                ctx.SetOutputInt("Exp", PlayerStats.Instance.Data.exp);
                ctx.SetOutputString("Name", PlayerStats.Instance.Data.playerName);
                return true;
            });

            // ── Player.SetName ──────────────────────────────────────
            BPRunner.RegisterHandler("Player.SetName", (ctx) =>
            {
                if (PlayerStats.Instance == null) { ctx.LogError("PlayerStats not found"); return false; }
                string name = ctx.GetInputString("Name");
                if (!string.IsNullOrEmpty(name))
                    PlayerStats.Instance.SetPlayerName(name);
                ctx.ActivateOutputFlow("Out");
                return true;
            });

            Debug.Log("[BlueprintPlayerNodes] Registered 4 nodes: Player.AddGold/AddExp/GetStats/SetName");
        }
    }
}
