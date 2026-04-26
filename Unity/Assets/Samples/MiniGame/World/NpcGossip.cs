// NpcGossip.cs
// ─────────────────────────────────────────────────────────────────────────────
// NPC 间互相对话系统（"村庄八卦"）
//
// 触发方式：
//   1. 定时八卦：每 N 秒随机选两个距离 ≤ gossipRadius 的 NPC，让 A 评论 B 最近的事
//   2. 事件八卦：WorldState.AddEvent / QuestSystem.OnQuestCompleted 时，
//      附近的 NPC 自动"听说"并发表评论
//   3. 定向八卦：代码主动调 TriggerGossip(aId, bId, topic) 指定两个 NPC 对话
//
// 对话是"A 说一句 → B 回一句"，不是完整对话回合（避免无限循环）。
//
// 配合蓝图：所有 NPC 用同一个 AI_NPC_WorldAware.bjson，
// NPC A 被 "Say" 时会把八卦内容作为 PlayerInput 传入，
// 但 B 会通过蓝图的 PrintString 以 "GOSSIP:" 前缀输出（避免被玩家看到？ 或者让玩家看到！）
//
// 注意：八卦频率要控制好，token 开销 = 2x 玩家对话，可以：
//   - 只在玩家附近触发（避免无观众的 NPC 说废话烧钱）
//   - gossipCooldownPerNpc 保底每个 NPC 至少 N 秒一次
//   - 关闭开关时完全不发请求
// ─────────────────────────────────────────────────────────────────────────────

using System;
using System.Collections.Generic;
using UnityEngine;
using BlueprintRuntime.Samples.MiniGame.Quest;

namespace BlueprintRuntime.Samples.MiniGame
{
    public class NpcGossip : MonoBehaviour
    {
        public static NpcGossip Instance { get; private set; }

        [Header("总开关")]
        [SerializeField] private bool enabled_ = true;

        [Header("定时八卦")]
        [Tooltip("每多少秒随机触发一次八卦。0=关闭定时八卦")]
        [SerializeField] private float autoIntervalSeconds = 60f;

        [Tooltip("同一个 NPC 最短多少秒才会再被选中八卦（防刷 token）")]
        [SerializeField] private float perNpcCooldownSeconds = 90f;

        [Header("事件八卦")]
        [Tooltip("WorldState.AddEvent 时是否自动触发附近 NPC 评论")]
        [SerializeField] private bool gossipOnWorldEvent = true;

        [Header("玩家在场限制")]
        [Tooltip("仅在玩家附近 N 米范围内的 NPC 才互相八卦（省 token）。<=0 = 不限制")]
        [SerializeField] private float onlyNearPlayerRadius = 30f;

        [Header("对话长度限制")]
        [Tooltip("LLM 输出字数上限（防止 NPC 八卦说太多）")]
        [SerializeField] private int maxGossipTokens = 60;

        private Transform _player;
        private readonly Dictionary<string, float> _lastGossipTime = new Dictionary<string, float>();

        void Awake()
        {
            if (Instance != null && Instance != this) { Destroy(gameObject); return; }
            Instance = this;
            DontDestroyOnLoad(gameObject);
        }

        void Start()
        {
            var pg = GameObject.FindGameObjectWithTag("Player");
            if (pg != null) _player = pg.transform;

            if (autoIntervalSeconds > 0)
                InvokeRepeating(nameof(TryAutoGossip), autoIntervalSeconds, autoIntervalSeconds);

            if (gossipOnWorldEvent && WorldState.Instance != null)
                WorldState.Instance.OnChanged += OnWorldChanged;
        }

        void OnDestroy()
        {
            if (WorldState.Instance != null)
                WorldState.Instance.OnChanged -= OnWorldChanged;
        }

        // ── 定时八卦 ──────────────────────────────────────────────
        private void TryAutoGossip()
        {
            if (!enabled_ || NpcRegistry.Instance == null) return;

            // 候选池：有玩家时只取玩家附近的
            var candidates = new List<NpcIdentity>();
            foreach (var n in NpcRegistry.Instance.All)
            {
                if (n?.transform == null) continue;
                if (onlyNearPlayerRadius > 0 && _player != null &&
                    Vector3.Distance(n.transform.position, _player.position) > onlyNearPlayerRadius)
                    continue;
                if (OnCooldown(n.id)) continue;
                candidates.Add(n);
            }
            if (candidates.Count < 2) return;

            var a = candidates[UnityEngine.Random.Range(0, candidates.Count)];
            // B 从 a 附近选
            var nearby = new List<NpcIdentity>();
            foreach (var c in candidates)
                if (c != a && Vector3.Distance(c.transform.position, a.transform.position) < 15f)
                    nearby.Add(c);
            if (nearby.Count == 0) return;
            var b = nearby[UnityEngine.Random.Range(0, nearby.Count)];

            TriggerGossipBetween(a, b, PickRandomTopic());
        }

        // ── 事件八卦 ──────────────────────────────────────────────
        private int _lastEventCount;
        private void OnWorldChanged()
        {
            if (!enabled_ || !gossipOnWorldEvent) return;
            if (WorldState.Instance == null) return;

            int count = WorldState.Instance.Data.recentEvents.Count;
            if (count <= _lastEventCount) { _lastEventCount = count; return; }
            _lastEventCount = count;

            var latestEvent = WorldState.Instance.Data.recentEvents[count - 1];
            if (string.IsNullOrEmpty(latestEvent)) return;

            // 玩家附近两个空闲 NPC 来评论一下
            if (NpcRegistry.Instance == null) return;
            var candidates = new List<NpcIdentity>();
            foreach (var n in NpcRegistry.Instance.All)
            {
                if (n?.transform == null) continue;
                if (onlyNearPlayerRadius > 0 && _player != null &&
                    Vector3.Distance(n.transform.position, _player.position) > onlyNearPlayerRadius)
                    continue;
                if (OnCooldown(n.id)) continue;
                candidates.Add(n);
            }
            if (candidates.Count == 0) return;

            var commentator = candidates[UnityEngine.Random.Range(0, candidates.Count)];
            TriggerNpcComment(commentator, $"（自言自语）你听说了吗：{latestEvent}");
        }

        // ── 公共 API ──────────────────────────────────────────────
        /// <summary>让 A 对 B 说一句话题，B 通过 NPC 回路回应（回应只展示在 B 的头顶气泡）</summary>
        public void TriggerGossipBetween(NpcIdentity a, NpcIdentity b, string topic)
        {
            if (a == null || b == null || string.IsNullOrEmpty(topic)) return;

            MarkGossiped(a.id);
            MarkGossiped(b.id);

            // A "自言自语"方式说出话题（通过 Say 触发 LLM 回路）
            a.Say($"（对{b.displayName}说）{topic}");

            // 稍后让 B 回应（避免两个 LLM 请求同时发）
            StartCoroutine(DelayedReply(b, a.displayName, topic, 3f));
        }

        public void TriggerNpcComment(NpcIdentity npc, string comment)
        {
            if (npc == null || string.IsNullOrEmpty(comment)) return;
            MarkGossiped(npc.id);
            npc.Say(comment);
        }

        public void TriggerGossipBetween(string aId, string bId, string topic)
        {
            if (NpcRegistry.Instance == null) return;
            if (NpcRegistry.Instance.TryGet(aId, out var a) &&
                NpcRegistry.Instance.TryGet(bId, out var b))
                TriggerGossipBetween(a, b, topic);
        }

        // ── 内部 ──────────────────────────────────────────────────
        private System.Collections.IEnumerator DelayedReply(NpcIdentity b, string aName, string topic, float delay)
        {
            yield return new WaitForSeconds(delay);
            b.Say($"（{aName}刚刚说：{topic}。你简短回一句，最多 15 字）");
        }

        private bool OnCooldown(string npcId)
        {
            if (_lastGossipTime.TryGetValue(npcId, out var t))
                return Time.time - t < perNpcCooldownSeconds;
            return false;
        }

        private void MarkGossiped(string npcId) => _lastGossipTime[npcId] = Time.time;

        private static string PickRandomTopic()
        {
            string[] topics = new []
            {
                "最近玩家在村里做了什么事你听说了吗",
                "这天气你觉得怎么样",
                "你对那个新来的冒险者什么看法",
                "村里最近有什么新鲜事吗",
            };
            return topics[UnityEngine.Random.Range(0, topics.Length)];
        }

        public void SetEnabled(bool v) => enabled_ = v;
    }
}
