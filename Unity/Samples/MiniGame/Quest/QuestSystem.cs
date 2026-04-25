// QuestSystem.cs
// ─────────────────────────────────────────────────────────────────────────────
// 任务系统 — 轻量级、可序列化、AI 友好
//
// 设计原则：
//   1. 任务定义用 ScriptableObject（策划可视化配置），不用蓝图
//      → 蓝图负责"对话"，ScriptableObject 负责"任务蓝本"
//   2. 任务进度跨会话持久化（BlueprintStorage）
//   3. 任务状态自动注入所有 NPC 的 runner 变量：
//        ActiveQuests   = "寻找丢失的猫咪; 给铁匠送 3 个蘑菇"
//        CompletedQuests = "..."
//      → NPC 能根据玩家当前任务说话
//   4. 任务触发条件：
//        a) 玩家和特定 NPC 说指定话（LLM 分析玩家输入是否匹配）
//        b) 好感度达到 N 时自动给任务
//        c) 到达某地点触发
//        d) 其他任务完成后触发（任务链）
//
// 与 AI 的结合点：
//   - AcceptQuest(id)     玩家接受任务（C#/UI/蓝图都能调）
//   - CompleteObjective(id, objId)  完成一个子目标
//   - NPC 在对话中"知道"当前任务进度
// ─────────────────────────────────────────────────────────────────────────────

using System;
using System.Collections.Generic;
using UnityEngine;

namespace BlueprintRuntime.Samples.MiniGame.Quest
{
    // ── 数据结构 ──────────────────────────────────────────────────────

    public enum QuestState { NotStarted, Active, Completed, Failed }

    [Serializable]
    public class QuestObjective
    {
        public string id;                 // 在本任务内唯一
        public string description;        // 显示文本，如"收集 3 朵蘑菇"
        public int    targetCount = 1;
        [NonSerialized] public int currentCount;     // 运行时字段（不写磁盘）
    }

    [Serializable]
    public class QuestProgress
    {
        public string      questId;
        public QuestState  state = QuestState.NotStarted;
        public long        startedAt;
        public long        completedAt;
        public List<int>   objectiveCounts = new List<int>();   // 与 ScriptableObject 中 objectives 同序
    }

    [Serializable]
    public class QuestSaveData
    {
        public List<QuestProgress> progresses = new List<QuestProgress>();
    }

    // ── 任务定义（ScriptableObject）──────────────────────────────────
    [CreateAssetMenu(fileName = "Quest_", menuName = "Blueprint/Quest Definition", order = 10)]
    public class Quest.QuestSystem.QuestData : ScriptableObject
    {
        [Tooltip("全局唯一 ID，如 'find_lost_cat'")]
        public string questId = "quest_id";

        [Tooltip("任务标题")]
        public string title = "任务标题";

        [TextArea(3, 6)]
        [Tooltip("任务描述，NPC 对话时会提到")]
        public string description = "帮我找回走丢的小猫...";

        [Tooltip("发任务的 NPC（可选，留空=系统任务）")]
        public string giverNpcId = "";

        [Tooltip("完成后交任务的 NPC（可选）")]
        public string turnInNpcId = "";

        public List<QuestObjective> objectives = new List<QuestObjective>();

        [Header("前置条件（可选）")]
        public List<string> requiredCompletedQuests = new List<string>();
        public int          requiredAffinityWith_giver = 0;

        [Header("奖励")]
        public int    rewardGold = 0;
        public int    rewardAffinity = 10;   // 完成后给 giverNpcId 加的好感
        public string rewardUnlockNpc = "";  // 完成后解锁的 NPC 名
    }

    // ── 核心 Manager ─────────────────────────────────────────────────
    [DefaultExecutionOrder(-35)]
    public class QuestSystem : MonoBehaviour
    {
        public static QuestSystem Instance { get; private set; }

        [Header("所有可用任务定义（拖 Quest.QuestSystem.QuestData 资源进来）")]
        [SerializeField] private List<Quest.QuestSystem.QuestData> allQuests = new List<Quest.QuestSystem.QuestData>();

        [Header("存档 Key")]
        [SerializeField] private string storageKey = "Quests.save";

        public QuestSaveData Save { get; private set; } = new QuestSaveData();

        public event Action<Quest.QuestSystem.QuestData>                     OnQuestAccepted;
        public event Action<Quest.QuestSystem.QuestData, QuestObjective>     OnObjectiveProgressed;
        public event Action<Quest.QuestSystem.QuestData>                     OnQuestCompleted;

        // ── 查询接口 ───────────────────────────────────────────────
        public Quest.QuestSystem.QuestData Find(string questId)
        {
            if (string.IsNullOrEmpty(questId)) return null;
            return allQuests.Find(q => q != null && q.questId == questId);
        }

        public QuestProgress GetProgress(string questId)
        {
            return Save.progresses.Find(p => p.questId == questId);
        }

        public QuestState GetState(string questId)
        {
            var p = GetProgress(questId);
            return p != null ? p.state : QuestState.NotStarted;
        }

        public List<Quest.QuestSystem.QuestData> GetActiveQuests()
        {
            var list = new List<Quest.QuestSystem.QuestData>();
            foreach (var p in Save.progresses)
            {
                if (p.state != QuestState.Active) continue;
                var def = Find(p.questId);
                if (def != null) list.Add(def);
            }
            return list;
        }

        public List<Quest.QuestSystem.QuestData> GetCompletedQuests()
        {
            var list = new List<Quest.QuestSystem.QuestData>();
            foreach (var p in Save.progresses)
            {
                if (p.state != QuestState.Completed) continue;
                var def = Find(p.questId);
                if (def != null) list.Add(def);
            }
            return list;
        }

        // ── 生命周期 ───────────────────────────────────────────────
        void Awake()
        {
            if (Instance != null && Instance != this) { Destroy(gameObject); return; }
            Instance = this;
            DontDestroyOnLoad(gameObject);
            LoadFromStorage();
        }

        // ── 操作接口 ───────────────────────────────────────────────
        /// <summary>尝试接受任务（前置条件不满足时返回 false）</summary>
        public bool AcceptQuest(string questId)
        {
            var def = Find(questId);
            if (def == null)
            {
                Debug.LogWarning($"[Quest] 未定义：{questId}");
                return false;
            }
            if (GetState(questId) != QuestState.NotStarted) return false;
            if (!CheckPrerequisites(def)) return false;

            var p = new QuestProgress {
                questId   = questId,
                state     = QuestState.Active,
                startedAt = DateTimeOffset.UtcNow.ToUnixTimeSeconds(),
            };
            for (int i = 0; i < def.objectives.Count; i++) p.objectiveCounts.Add(0);
            Save.progresses.Add(p);
            Persist();
            OnQuestAccepted?.Invoke(def);
            InjectAllNpcRunners();
            return true;
        }

        /// <summary>推进一个子目标（通常由游戏事件调用：拾取物品、杀怪等）</summary>
        public void Progress(string questId, string objectiveId, int delta = 1)
        {
            var def = Find(questId);
            if (def == null) return;
            var p = GetProgress(questId);
            if (p == null || p.state != QuestState.Active) return;

            int idx = def.objectives.FindIndex(o => o.id == objectiveId);
            if (idx < 0) return;

            while (p.objectiveCounts.Count <= idx) p.objectiveCounts.Add(0);
            p.objectiveCounts[idx] = Mathf.Min(p.objectiveCounts[idx] + delta, def.objectives[idx].targetCount);

            OnObjectiveProgressed?.Invoke(def, def.objectives[idx]);

            // 检查是否全部完成
            bool allDone = true;
            for (int i = 0; i < def.objectives.Count; i++)
                if (p.objectiveCounts[i] < def.objectives[i].targetCount) { allDone = false; break; }

            if (allDone) InternalComplete(def, p);
            Persist();
        }

        /// <summary>强制完成（不检查进度，用于"系统奖励"或调试）</summary>
        public void ForceComplete(string questId)
        {
            var def = Find(questId);
            var p   = GetProgress(questId);
            if (def == null || p == null) return;
            for (int i = 0; i < def.objectives.Count; i++)
                p.objectiveCounts[i] = def.objectives[i].targetCount;
            InternalComplete(def, p);
            Persist();
        }

        private void InternalComplete(Quest.QuestSystem.QuestData def, QuestProgress p)
        {
            if (p.state == QuestState.Completed) return;
            p.state       = QuestState.Completed;
            p.completedAt = DateTimeOffset.UtcNow.ToUnixTimeSeconds();

            // 发奖励
            if (def.rewardGold > 0)
                PlayerStats.Instance?.AddGold(def.rewardGold);
            if (def.rewardAffinity != 0 && !string.IsNullOrEmpty(def.giverNpcId))
            {
                var mem = FindNpcMemory(def.giverNpcId);
                mem?.AddAffinity(def.rewardAffinity);
            }
            if (WorldState.Instance != null)
                WorldState.Instance.AddEvent($"玩家完成了任务：{def.title}");

            OnQuestCompleted?.Invoke(def);
            InjectAllNpcRunners();
        }

        // ── 前置条件 ───────────────────────────────────────────────
        public bool CheckPrerequisites(Quest.QuestSystem.QuestData def)
        {
            foreach (var req in def.requiredCompletedQuests)
            {
                if (GetState(req) != QuestState.Completed) return false;
            }
            if (def.requiredAffinityWith_giver > 0 && !string.IsNullOrEmpty(def.giverNpcId))
            {
                var mem = FindNpcMemory(def.giverNpcId);
                if (mem == null || mem.Data.affinity < def.requiredAffinityWith_giver) return false;
            }
            return true;
        }

        // ── 注入到 NPC runner ──────────────────────────────────────
        public void InjectInto(BPRunner runner)
        {
            if (runner == null) return;
            runner.SetVariable("ActiveQuests",   BuildQuestText(GetActiveQuests(),   true));
            runner.SetVariable("CompletedQuests", BuildQuestText(GetCompletedQuests(), false));
        }

        private string BuildQuestText(List<Quest.QuestSystem.QuestData> defs, bool includeProgress)
        {
            if (defs.Count == 0) return "无";
            var sb = new System.Text.StringBuilder();
            for (int i = 0; i < defs.Count; i++)
            {
                if (i > 0) sb.Append("；");
                var d = defs[i];
                sb.Append(d.title);
                if (includeProgress)
                {
                    var p = GetProgress(d.questId);
                    int done = 0;
                    for (int k = 0; k < d.objectives.Count; k++)
                        if (p != null && k < p.objectiveCounts.Count &&
                            p.objectiveCounts[k] >= d.objectives[k].targetCount) done++;
                    sb.Append($"({done}/{d.objectives.Count})");
                }
            }
            return sb.ToString();
        }

        /// <summary>刷新所有 NPC runner 的任务变量</summary>
        public void InjectAllNpcRunners()
        {
            if (NpcRegistry.Instance == null) return;
            foreach (var id in NpcRegistry.Instance.All)
            {
                var runner = NpcRegistry.RunnerAccessor.Get(id.controller);
                if (runner != null) InjectInto(runner);
            }
        }

        // ── 持久化 ──────────────────────────────────────────────────
        private void LoadFromStorage()
        {
            Save = BlueprintStorage.GetJson<QuestSaveData>(storageKey) ?? new QuestSaveData();
        }

        private void Persist() => BlueprintStorage.SetJson(storageKey, Save);

        public void WipeAllProgress()
        {
            Save = new QuestSaveData();
            BlueprintStorage.Remove(storageKey);
            InjectAllNpcRunners();
        }

        // 工具
        private static NpcMemory FindNpcMemory(string npcId)
        {
            foreach (var m in FindObjectsOfType<NpcMemory>())
            {
                if (m.NpcId == npcId) return m;
            }
            return null;
        }
    }
}
