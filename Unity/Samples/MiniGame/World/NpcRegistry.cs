// NpcRegistry.cs
// ─────────────────────────────────────────────────────────────────────────────
// NPC 注册中心 — 让 NPC 能"知道"彼此
//
// 功能：
//   1. 场景内所有 NPC 启动时自动注册（如果挂了 NpcMemory 或任一 Controller）
//   2. 每个 NPC 对话时，自动把"附近 NPC 列表"注入 Runner 变量：
//        NearbyNpcs = "铁匠老王(25米)；诗人莉莉(38米)"
//   3. 支持定向 NPC.Say（向指定 NPC 说话）
//
// 蓝图里用法：
//   SystemPrompt 里可以拼：
//     "你附近有这些 NPC：{NearbyNpcs}。如果玩家问起他们，请用知情但不越界的语气回答。"
//
// 让 NPC 互相对话（进阶）：
//   用 BroadcastTopic(topic, text) 广播一条"话题"，任何 NPC 都可以通过
//   监听 OnTopicPosted 事件 + LLM 决策是否接话。
// ─────────────────────────────────────────────────────────────────────────────

using System;
using System.Collections.Generic;
using UnityEngine;

namespace BlueprintRuntime.Samples.MiniGame
{
    /// <summary>NPC 对外可见的身份信息（供其他 NPC 感知）</summary>
    public class NpcIdentity
    {
        public string        id;            // 如 "NPC_MeowMeow"
        public string        displayName;   // 如 "喵喵店长"
        public Transform     transform;
        public MonoBehaviour controller;    // AINpcController / Streaming / Emotional

        /// <summary>调 NPC.Say（不管它是哪种 Controller）</summary>
        public bool Say(string text)
        {
            if (controller == null) return false;
            switch (controller)
            {
                case AINpcController basic:         return basic.Say(text);
                case AINpcStreamingController s:    return s.Say(text);
                case EmotionalNpcController e:      return e.Say(text);
            }
            return false;
        }
    }

    [DefaultExecutionOrder(-40)]
    public class NpcRegistry : MonoBehaviour
    {
        public static NpcRegistry Instance { get; private set; }

        [Header("世界感知半径（米）")]
        [Tooltip("仅计入此半径内的 NPC 到 NearbyNpcs 变量")]
        [SerializeField] private float awarenessRadius = 50f;

        [Tooltip("每多少秒刷新一次 NearbyNpcs（0=不自动刷新，只在 NPC 对话时刷新）")]
        [SerializeField] private float refreshInterval = 0f;

        private readonly Dictionary<string, NpcIdentity> _npcs = new Dictionary<string, NpcIdentity>();

        /// <summary>任何 NPC 发起话题时触发（其他 NPC 可选择跟话）</summary>
        public event Action<string /*fromNpcId*/, string /*topic*/, string /*text*/> OnTopicPosted;

        void Awake()
        {
            if (Instance != null && Instance != this) { Destroy(gameObject); return; }
            Instance = this;
            DontDestroyOnLoad(gameObject);
        }

        void Start()
        {
            if (refreshInterval > 0) InvokeRepeating(nameof(RefreshAll), refreshInterval, refreshInterval);
        }

        // ── 注册 / 注销 ──────────────────────────────────────────────
        public void Register(NpcIdentity npc)
        {
            if (npc == null || string.IsNullOrEmpty(npc.id)) return;
            _npcs[npc.id] = npc;
        }

        public void Unregister(string id)
        {
            if (!string.IsNullOrEmpty(id)) _npcs.Remove(id);
        }

        public bool TryGet(string id, out NpcIdentity npc) => _npcs.TryGetValue(id, out npc);

        public IEnumerable<NpcIdentity> All => _npcs.Values;

        // ── 查询附近 NPC（不含自己） ─────────────────────────────────
        public List<NpcIdentity> GetNearby(NpcIdentity self)
        {
            var list = new List<NpcIdentity>();
            if (self == null || self.transform == null) return list;
            foreach (var n in _npcs.Values)
            {
                if (n == null || n == self || n.transform == null) continue;
                if (Vector3.Distance(n.transform.position, self.transform.position) <= awarenessRadius)
                    list.Add(n);
            }
            return list;
        }

        public string BuildNearbyText(NpcIdentity self)
        {
            var nearby = GetNearby(self);
            if (nearby.Count == 0) return "附近空无一人";
            var sb = new System.Text.StringBuilder();
            for (int i = 0; i < nearby.Count; i++)
            {
                if (i > 0) sb.Append("；");
                var n = nearby[i];
                float d = self != null && self.transform != null && n.transform != null
                    ? Vector3.Distance(n.transform.position, self.transform.position) : 0;
                sb.Append($"{n.displayName}({d:F0}米)");
            }
            return sb.ToString();
        }

        // ── 注入到 Runner（对话前调用） ─────────────────────────────
        public void InjectNearbyInto(NpcIdentity self, BPRunner runner)
        {
            if (runner == null) return;
            runner.SetVariable("NearbyNpcs", BuildNearbyText(self));
        }

        private void RefreshAll()
        {
            // 定时刷新所有 NPC 的 NearbyNpcs 变量
            foreach (var n in _npcs.Values)
            {
                var runner = RunnerAccessor.Get(n.controller);
                if (runner != null) InjectNearbyInto(n, runner);
            }
        }

        // ── 话题广播（NPC 间对话基础设施） ─────────────────────────
        /// <summary>
        /// NPC 发起一个话题。其他监听者（可以是 NPC 或 UI）按需响应。
        /// 典型用途：NPC A 看到玩家进 NPC B 的店 → 广播 "Player visits 铁匠"
        /// </summary>
        public void BroadcastTopic(string fromNpcId, string topic, string text)
        {
            OnTopicPosted?.Invoke(fromNpcId ?? "", topic ?? "", text ?? "");
        }

        // 共享反射工具
        internal static class RunnerAccessor
        {
            public static BPRunner Get(MonoBehaviour c)
            {
                if (c == null) return null;
                switch (c)
                {
                    case AINpcController basic:      return GetField<AINpcController>().GetValue(c) as BPRunner;
                    case AINpcStreamingController s: return GetField<AINpcStreamingController>().GetValue(c) as BPRunner;
                    case EmotionalNpcController e:   return GetField<EmotionalNpcController>().GetValue(c) as BPRunner;
                }
                return null;
            }
            private static System.Reflection.FieldInfo GetField<T>() => typeof(T).GetField("_runner",
                System.Reflection.BindingFlags.NonPublic | System.Reflection.BindingFlags.Instance);
        }
    }

    // ─────────────────────────────────────────────────────────────────
    // 辅助组件：挂到 NPC 上，自动 Register/Unregister
    // ─────────────────────────────────────────────────────────────────
    [DisallowMultipleComponent]
    public class NpcRegistrant : MonoBehaviour
    {
        [SerializeField] private string npcId = "";
        [SerializeField] private string displayName = "";

        [Header("对话前自动注入世界/附近信息")]
        [Tooltip("开启后：每次 NPC 开始思考（OnThinking）时自动注入全部世界/玩家/任务变量")]
        [SerializeField] private bool injectBeforeEveryChat = true;
        [SerializeField] private bool injectWorldState      = true;
        [SerializeField] private bool injectNearbyNpcs      = true;
        [SerializeField] private bool injectPlayerStats     = true;
        [SerializeField] private bool injectQuests          = true;

        private NpcIdentity _identity;
        private AINpcController          _basic;
        private AINpcStreamingController _stream;
        private EmotionalNpcController   _emotional;

        void Start()
        {
            var mem = GetComponent<NpcMemory>();
            var id  = !string.IsNullOrEmpty(npcId) ? npcId : mem != null ? mem.NpcId : gameObject.name;
            var name = !string.IsNullOrEmpty(displayName) ? displayName : GetControllerName() ?? gameObject.name;

            _basic     = GetComponent<AINpcController>();
            _stream    = GetComponent<AINpcStreamingController>();
            _emotional = GetComponent<EmotionalNpcController>();

            _identity = new NpcIdentity {
                id          = id,
                displayName = name,
                transform   = transform,
                controller  = (MonoBehaviour)_stream ?? (MonoBehaviour)_basic ?? (MonoBehaviour)_emotional,
            };

            if (NpcRegistry.Instance == null)
            {
                var go = new GameObject("NpcRegistry");
                go.AddComponent<NpcRegistry>();
            }
            NpcRegistry.Instance.Register(_identity);

            if (injectBeforeEveryChat)
            {
                // 订阅 OnThinking —— 在 SetVariable(PlayerInput) 之前触发，
                // 我们这时注入的 WorldTime/NearbyNpcs 会在同一次 Execute 中生效
                if (_basic     != null) _basic.OnThinking     += InjectNow;
                if (_stream    != null) _stream.OnThinking    += InjectNow;
                if (_emotional != null) _emotional.OnThinking += InjectNow;
            }
            else
            {
                InjectNow();   // 仅启动时注入一次
            }
        }

        void OnDestroy()
        {
            if (_basic     != null) _basic.OnThinking     -= InjectNow;
            if (_stream    != null) _stream.OnThinking    -= InjectNow;
            if (_emotional != null) _emotional.OnThinking -= InjectNow;

            if (_identity != null && NpcRegistry.Instance != null)
                NpcRegistry.Instance.Unregister(_identity.id);
        }

        /// <summary>把世界状态 + 附近 NPC + 玩家属性 + 任务状态注入到本 NPC 的 runner</summary>
        public void InjectNow()
        {
            var runner = NpcRegistry.RunnerAccessor.Get(_identity?.controller);
            if (runner == null) return;
            if (injectWorldState && WorldState.Instance != null)
                WorldState.Instance.InjectInto(runner);
            if (injectNearbyNpcs && NpcRegistry.Instance != null)
                NpcRegistry.Instance.InjectNearbyInto(_identity, runner);
            if (injectPlayerStats && PlayerStats.Instance != null)
                PlayerStats.Instance.InjectInto(runner);
            if (injectQuests && Quest.QuestSystem.Instance != null)
                Quest.QuestSystem.Instance.InjectInto(runner);
        }

        private string GetControllerName()
        {
            var b = GetComponent<AINpcController>();          if (b != null) return b.NpcName;
            var s = GetComponent<AINpcStreamingController>(); if (s != null) return s.NpcName;
            var e = GetComponent<EmotionalNpcController>();   if (e != null) return e.NpcName;
            return null;
        }
    }
}
