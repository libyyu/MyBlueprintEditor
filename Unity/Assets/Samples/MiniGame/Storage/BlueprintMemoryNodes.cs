// ─────────────────────────────────────────────────────────────────────
// BlueprintMemoryNodes.cs — NPC 记忆蓝图节点
//
// 节点：Memory.AddAffinity / Memory.RememberFact / Memory.GetAffinity / Memory.IsFirstMeet
// 让蓝图直接操作 NPC 的好感度和事实库
// ─────────────────────────────────────────────────────────────────────

using UnityEngine;
using BlueprintRuntime;

namespace BlueprintRuntime.Samples.MiniGame
{
    public static class BlueprintMemoryNodes
    {
        public static void RegisterAll(BPRunner runner)
        {
            // ── Memory.AddAffinity ──────────────────────────────────
            runner.RegisterHandler("Memory.AddAffinity", (ctx) =>
            {
                string npcId = ctx.GetInputString("NpcId");
                int delta = (int)ctx.GetInputInt("Delta");

                var mem = FindMemory(npcId);
                if (mem == null)
                {
                    ctx.LogError($"Memory.AddAffinity: NpcMemory '{npcId}' not found");
                    return false;
                }

                mem.AddAffinity(delta);
                ctx.SetOutputInt("NewAffinity", mem.Data.affinity);

                if (delta > 0)
                    ctx.Print($"好感度 +{delta} → {mem.Data.affinity}");
                else if (delta < 0)
                    ctx.Print($"好感度 {delta} → {mem.Data.affinity}");

                ctx.ActivateOutputFlow("Out");
                return true;
            });

            // ── Memory.RememberFact ─────────────────────────────────
            runner.RegisterHandler("Memory.RememberFact", (ctx) =>
            {
                string npcId = ctx.GetInputString("NpcId");
                string fact = ctx.GetInputString("Fact");

                var mem = FindMemory(npcId);
                if (mem == null)
                {
                    ctx.LogError($"Memory.RememberFact: NpcMemory '{npcId}' not found");
                    return false;
                }

                mem.RememberFact(fact);
                ctx.ActivateOutputFlow("Out");
                return true;
            });

            // ── Memory.GetAffinity ──────────────────────────────────
            runner.RegisterHandler("Memory.GetAffinity", (ctx) =>
            {
                string npcId = ctx.GetInputString("NpcId");
                var mem = FindMemory(npcId);

                ctx.SetOutputInt("Affinity", mem?.Data.affinity ?? 0);
                ctx.SetOutputInt("MeetCount", mem?.Data.meetCount ?? 0);
                ctx.SetOutputBool("IsFirstMeet", mem != null && mem.Data.meetCount <= 1);
                return true;
            });

            // ── Memory.ClearHistory ─────────────────────────────────
            runner.RegisterHandler("Memory.ClearHistory", (ctx) =>
            {
                string npcId = ctx.GetInputString("NpcId");
                var mem = FindMemory(npcId);
                if (mem != null)
                {
                    mem.Data.history.Clear();
                    mem.Save();
                    ctx.Print("对话历史已清除");
                }
                ctx.ActivateOutputFlow("Out");
                return true;
            });

            Debug.Log("[BlueprintMemoryNodes] Registered 4 nodes: Memory.AddAffinity/RememberFact/GetAffinity/ClearHistory");
        }

        private static NpcMemory FindMemory(string npcId)
        {
            if (string.IsNullOrEmpty(npcId))
                return Object.FindObjectOfType<NpcMemory>();

            foreach (var mem in Object.FindObjectsOfType<NpcMemory>())
                if (mem.NpcId == npcId) return mem;
            return null;
        }
    }
}
