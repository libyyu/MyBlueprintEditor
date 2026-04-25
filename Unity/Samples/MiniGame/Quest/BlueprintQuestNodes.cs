// ─────────────────────────────────────────────────────────────────────
// BlueprintQuestNodes.cs — 把 QuestSystem 暴露为蓝图自定义节点
//
// 节点清单：
//   Quest.Give     — NPC 给玩家发任务（输入：id/title/desc/reward/target）
//   Quest.Complete — 完成一个任务（输入：questId）
//   Quest.Progress — 推进任务进度（输入：questId, amount）
//   Quest.Check    — 检查任务状态（输入：questId → 输出 isActive/isComplete）
//
// 注册时机：MiniGameBootstrap.Start() 或任意启动脚本
// ─────────────────────────────────────────────────────────────────────

using UnityEngine;
using BlueprintRuntime;
using BlueprintRuntime.Samples.AINpc;

namespace BlueprintRuntime.Samples.MiniGame.Quest
{
    public static class BlueprintQuestNodes
    {
        /// <summary>注册所有 Quest 蓝图节点（在 BlueprintService 初始化后调用一次）</summary>
        public static void RegisterAll(BPRunner runner)
        {
            // ── Quest.Give ──────────────────────────────────────────
            runner.RegisterHandler("Quest.Give", (ctx) =>
            {
                if (QuestSystem.Instance == null)
                {
                    ctx.LogError("QuestSystem not found");
                    ctx.ActivateOutputFlow("onError");
                    return false;
                }

                string id    = ctx.GetInputString("QuestId");
                string title = ctx.GetInputString("Title");
                string desc  = ctx.GetInputString("Description");
                string reward = ctx.GetInputString("RewardDescription");
                int target   = (int)ctx.GetInputInt("TargetCount");

                if (string.IsNullOrEmpty(id))   id = $"quest_{Time.frameCount}";
                if (string.IsNullOrEmpty(title)) title = "新任务";
                if (target <= 0) target = 1;

                string npcId = ctx.GetInputString("NpcId");

                bool ok = QuestSystem.Instance.GiveQuest(
                    id, title, desc,
                    npcId ?? "",
                    reward ?? $"+50 金币",
                    target,
                    50  // 默认奖励 50 金币
                );

                if (ok)
                {
                    ctx.Print($"📋 新任务：{title}");
                    ctx.ActivateOutputFlow("onSuccess");
                }
                else
                {
                    ctx.ActivateOutputFlow("onDuplicate");
                }
                return true;
            });

            // ── Quest.Complete ──────────────────────────────────────
            runner.RegisterHandler("Quest.Complete", (ctx) =>
            {
                if (QuestSystem.Instance == null)
                {
                    ctx.LogError("QuestSystem not found");
                    return false;
                }

                string id = ctx.GetInputString("QuestId");
                if (string.IsNullOrEmpty(id))
                {
                    ctx.LogError("Quest.Complete: QuestId is empty");
                    return false;
                }

                bool ok = QuestSystem.Instance.CompleteQuest(id);
                if (ok)
                {
                    ctx.Print($"✅ 任务完成：{id}");
                    ctx.ActivateOutputFlow("onSuccess");
                }
                else
                {
                    ctx.ActivateOutputFlow("onNotFound");
                }
                return true;
            });

            // ── Quest.Progress ──────────────────────────────────────
            runner.RegisterHandler("Quest.Progress", (ctx) =>
            {
                if (QuestSystem.Instance == null) return false;

                string id = ctx.GetInputString("QuestId");
                int amount = (int)ctx.GetInputInt("Amount");
                if (amount <= 0) amount = 1;

                QuestSystem.Instance.UpdateProgress(id, amount);
                ctx.ActivateOutputFlow("Out");
                return true;
            });

            // ── Quest.Check ─────────────────────────────────────────
            runner.RegisterHandler("Quest.Check", (ctx) =>
            {
                if (QuestSystem.Instance == null)
                {
                    ctx.SetOutputBool("IsActive", false);
                    ctx.SetOutputBool("IsComplete", false);
                    return false;
                }

                string id = ctx.GetInputString("QuestId");
                var quest = QuestSystem.Instance.GetQuest(id);

                ctx.SetOutputBool("IsActive",
                    quest != null && quest.status == QuestSystem.QuestStatus.Active);
                ctx.SetOutputBool("IsComplete",
                    quest != null && quest.status == QuestSystem.QuestStatus.Completed);
                ctx.SetOutputString("Title",
                    quest?.title ?? "");
                ctx.SetOutputInt("Progress",
                    quest?.currentCount ?? 0);
                ctx.SetOutputInt("Target",
                    quest?.targetCount ?? 0);
                return true;
            });

            Debug.Log("[BlueprintQuestNodes] Registered 4 nodes: Quest.Give/Complete/Progress/Check");
        }
    }
}
