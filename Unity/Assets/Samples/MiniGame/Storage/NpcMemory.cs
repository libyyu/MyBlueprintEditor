// NpcMemory.cs
// ─────────────────────────────────────────────────────────────────────────────
// NPC 跨会话记忆 — 挂到 NPC GameObject 上自动生效
//
// 记忆内容：
//   1. 好感度（0-100 int）
//   2. 最近 N 轮对话（玩家 + NPC）
//   3. NPC 已知事实列表（由 LLM 自动提取）
//   4. 最后见面时间
//
// 自动集成：
//   - 每次 NPC.Say → 自动追加到历史
//   - 每次 NPC.OnReply → 自动追加到历史
//   - Play 时自动加载上次记忆
//   - 每次对话结束自动保存
//
// 注入到蓝图：
//   Start() 时把 History/Affinity/KnownFacts 序列化后 SetVariable，
//   让蓝图的 SystemPrompt 能拼接这些上下文。
// ─────────────────────────────────────────────────────────────────────────────

using BlueprintRuntime.Samples.AINpc;
using System;
using System.Collections.Generic;
using UnityEngine;
using BlueprintRuntime.Samples.AINpc.OpenWorld;

namespace BlueprintRuntime.Samples.MiniGame
{
    // ── 可序列化数据结构（Unity JsonUtility 友好） ──────────────────────
    [Serializable]
    public class ChatTurn
    {
        public string role;        // "user" | "assistant"
        public string content;
        public long   timestamp;   // Unix 秒
    }

    [Serializable]
    public class NpcMemoryData
    {
        public int                 affinity        = 50;   // 0 讨厌 / 50 中立 / 100 崇拜
        public List<ChatTurn>      history         = new List<ChatTurn>();
        public List<string>        knownFacts      = new List<string>();
        public long                lastMeetTime    = 0;
        public int                 meetCount       = 0;
    }

    [DisallowMultipleComponent]
    public class NpcMemory : MonoBehaviour
    {
        [Header("身份")]
        [Tooltip("存档 key 前缀，每个 NPC 必须唯一")]
        [SerializeField] private string npcId = "NPC_MeowMeow";

        [Header("行为")]
        [Tooltip("保留最近多少轮对话。过长会烧 token，过短会失忆")]
        [SerializeField] private int maxHistoryTurns = 10;

        [Tooltip("玩家每进入 1 次 greet 范围，meetCount+1")]
        [SerializeField] private bool countMeetsAutomatically = true;

        // ── 数据与事件 ──────────────────────────────────────────────
        public NpcMemoryData Data { get; private set; } = new NpcMemoryData();
        public string NpcId => npcId;

        /// <summary>记忆被修改时触发（外部保存/反序列化/好感度变化）</summary>
        public event Action OnChanged;

        // 自动发现依赖
        private AINpcController          _basic;
        private AINpcStreamingController _stream;
        private EmotionalNpcController   _emotional;

        void Awake()
        {
            _basic     = GetComponent<AINpcController>();
            _stream    = GetComponent<AINpcStreamingController>();
            _emotional = GetComponent<EmotionalNpcController>();
        }

        void Start()
        {
            Load();

            // 记录一次见面
            Data.meetCount   += 1;
            Data.lastMeetTime = DateTimeOffset.UtcNow.ToUnixTimeSeconds();
            Save();

            // 订阅事件，自动记录对话
            SubscribeAutoRecord();

            // 把记忆注入蓝图变量（供 SystemPrompt 使用）
            InjectIntoRunner();
        }

        void OnDestroy()
        {
            UnsubscribeAutoRecord();
            Save();
        }

        // ── 持久化 ───────────────────────────────────────────────────
        private string StorageKey => $"{npcId}.memory";

        public void Load()
        {
            Data = BlueprintStorage.GetJson<NpcMemoryData>(StorageKey) ?? new NpcMemoryData();
            OnChanged?.Invoke();
        }

        public void Save() => BlueprintStorage.SetJson(StorageKey, Data);

        public void Wipe()
        {
            Data = new NpcMemoryData();
            BlueprintStorage.Remove(StorageKey);
            OnChanged?.Invoke();
        }

        // ── 高层 API（供游戏逻辑/蓝图 Handler 调用） ─────────────────
        public void AppendPlayerTurn(string content)   => AppendTurn("user",      content);
        public void AppendNpcTurn(string content)      => AppendTurn("assistant", content);

        private void AppendTurn(string role, string content)
        {
            if (string.IsNullOrEmpty(content)) return;
            Data.history.Add(new ChatTurn {
                role = role,
                content = content,
                timestamp = DateTimeOffset.UtcNow.ToUnixTimeSeconds(),
            });
            while (Data.history.Count > maxHistoryTurns * 2)
                Data.history.RemoveAt(0);
            Save();
            OnChanged?.Invoke();
        }

        public void AddAffinity(int delta)
        {
            Data.affinity = Mathf.Clamp(Data.affinity + delta, 0, 100);
            Save();
            OnChanged?.Invoke();
            InjectIntoRunner();  // 好感度立即反映到下次对话
        }

        public void RememberFact(string fact)
        {
            if (string.IsNullOrWhiteSpace(fact)) return;
            if (Data.knownFacts.Contains(fact)) return;
            Data.knownFacts.Add(fact);
            // 事实库不无限膨胀
            while (Data.knownFacts.Count > 30) Data.knownFacts.RemoveAt(0);
            Save();
        }

        // ── 向蓝图注入上下文 ─────────────────────────────────────────
        /// <summary>
        /// 把记忆序列化后写入蓝图变量，供 SystemPrompt 拼接使用。
        /// 蓝图里的 SystemPrompt 应该是：
        ///   {Personality} + "\n\n" +
        ///   "你对玩家的好感度是 {Affinity}（0=讨厌，100=崇拜）。" +
        ///   "这是你们第 {MeetCount} 次见面。" +
        ///   "你记得的事实：{KnownFacts}。"
        /// </summary>
        public void InjectIntoRunner()
        {
            var r = FindRunner();
            if (r == null) return;

            r.SetVariable("Affinity",    Data.affinity);
            r.SetVariable("MeetCount",   Data.meetCount);
            r.SetVariable("KnownFacts",  string.Join("; ", Data.knownFacts));
            r.SetVariable("HistoryJSON", JsonUtility.ToJson(new SerializableList<ChatTurn>(Data.history)));
            r.SetVariable("IsFirstMeet", Data.meetCount <= 1);
        }

        // 直接从 Controller 里拿 runner（通过反射？不，Controller 里有 BPRunner 字段）
        // 稳妥做法：Controller 没暴露 runner，我们只暴露 SetVariable 接口 —— 加个简单方法
        // 这里用 rich API 直接拿
        private BPRunner FindRunner()
        {
            // 借助 BPRunnerAccessor 扩展（见文件底部）
            if (_stream    != null) return RunnerAccessor.Get(_stream);
            if (_basic     != null) return RunnerAccessor.Get(_basic);
            if (_emotional != null) return RunnerAccessor.Get(_emotional);
            return null;
        }

        // ── 自动记录（订阅 Controller 事件） ─────────────────────────
        private void SubscribeAutoRecord()
        {
            if (_basic != null)
            {
                _basic.OnReply += OnBasicReply;
            }
            if (_stream != null)
            {
                _stream.OnReplyDone += OnStreamDone;
            }
            if (_emotional != null)
            {
                _emotional.OnReply += OnEmotionalReply;
            }
        }

        private void UnsubscribeAutoRecord()
        {
            if (_basic     != null) _basic.OnReply         -= OnBasicReply;
            if (_stream    != null) _stream.OnReplyDone    -= OnStreamDone;
            if (_emotional != null) _emotional.OnReply     -= OnEmotionalReply;
        }

        private void OnBasicReply(string msg)      => AppendNpcTurn(msg);
        private void OnStreamDone(string full)     => AppendNpcTurn(full);
        private void OnEmotionalReply(string msg)  => AppendNpcTurn(msg);

        /// <summary>
        /// 外部在调 NPC.Say(text) 前后，应该手动调 AppendPlayerTurn(text)。
        /// 因为 Controller.Say 返回 bool 无法挂钩；OpenWorldDialogPanel 里已经做了。
        /// </summary>
        public void NotifyPlayerSaid(string text) => AppendPlayerTurn(text);
    }

    // ── 工具：JsonUtility 不支持直接序列化 List<T> ──────────────────
    [Serializable]
    public class SerializableList<T>
    {
        public List<T> items;
        public SerializableList(List<T> src) { items = src; }
    }

    // ── 工具：从 NPC Controller 拿 BPRunner ─────────────────────────
    // 通过反射访问 Controller 的 private _runner 字段，避免改动 Controller 接口
    internal static class RunnerAccessor
    {
        private static readonly System.Reflection.FieldInfo s_basicField     = GetField<AINpcController>();
        private static readonly System.Reflection.FieldInfo s_streamField    = GetField<AINpcStreamingController>();
        private static readonly System.Reflection.FieldInfo s_emotionalField = GetField<EmotionalNpcController>();

        private static System.Reflection.FieldInfo GetField<T>() => typeof(T).GetField("_runner",
            System.Reflection.BindingFlags.NonPublic | System.Reflection.BindingFlags.Instance);

        public static BPRunner Get(AINpcController c)           => s_basicField?.GetValue(c) as BPRunner;
        public static BPRunner Get(AINpcStreamingController c)  => s_streamField?.GetValue(c) as BPRunner;
        public static BPRunner Get(EmotionalNpcController c)    => s_emotionalField?.GetValue(c) as BPRunner;
    }
}
