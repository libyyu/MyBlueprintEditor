// NpcBehaviorGate.cs
// ─────────────────────────────────────────────────────────────────────────────
// 好感度驱动的 NPC 行为分支 — 根据 affinity 切换 Personality / 解锁剧情
//
// 核心机制：
//   1. 定义若干"好感度段位"（Stranger/Acquaintance/Friend/CloseFriend/Lover）
//   2. 每个段位有专属 Personality（性格变化）+ 可选专属蓝图
//   3. 额外可指定"解锁事件"：进入此段位时触发 WorldState.AddEvent
//
// 使用：
//   挂到和 AINpcStreamingController 同一 GameObject；配置段位；
//   玩家和 NPC 对话时，游戏侧（比如情绪版 NPC 的 LLM）给 AddAffinity 调整数值；
//   好感度变化跨段位时，本组件自动切换 Personality 注入到 runner。
//
// 何时调 AddAffinity？
//   - 方案 A（手动）：玩家送礼、完成任务 → 显式 AddAffinity(+10)
//   - 方案 B（LLM 自动）：情绪版蓝图让 LLM 输出 {affinity_delta: N}，C# 解析后加
//   - 方案 C（情感识别）：分析玩家输入关键词（夸奖/侮辱）→ ±N
//
// 本文件只负责"段位切换"逻辑，不强制使用哪种 Add 方案。
// ─────────────────────────────────────────────────────────────────────────────

using BlueprintRuntime.Samples.AINpc;
using System;
using System.Collections.Generic;
using UnityEngine;

namespace BlueprintRuntime.Samples.MiniGame
{
    [Serializable]
    public class AffinityStage
    {
        [Tooltip("段位名（仅调试显示）")]
        public string name = "Stranger";

        [Tooltip("进入此段位的最低 affinity（闭区间）")]
        [Range(0, 100)]
        public int minAffinity = 0;

        [TextArea(3, 8)]
        [Tooltip("此段位的 Personality（将 SetVariable 注入蓝图）")]
        public string personality = "你对玩家很陌生，保持礼貌但有距离感。";

        [Tooltip("可选：此段位触发的全局事件（一次性）")]
        public string unlockEvent = "";

        [Tooltip("可选：此段位专用蓝图。留空=继续用默认 Controller 蓝图")]
        public TextAsset overrideBlueprint;
    }

    [DisallowMultipleComponent]
    public class NpcBehaviorGate : MonoBehaviour
    {
        [Header("段位配置（minAffinity 从低到高排列）")]
        [SerializeField] private List<AffinityStage> stages = new List<AffinityStage>
        {
            new AffinityStage { name = "Stranger",    minAffinity = 0,  personality = "你对玩家很陌生，礼貌但疏远，回答简短。" },
            new AffinityStage { name = "Acquaintance",minAffinity = 21, personality = "你已经见过玩家几次，开始好奇 TA 是谁。" },
            new AffinityStage { name = "Friend",      minAffinity = 41, personality = "玩家是你的朋友，你会主动分享小事，偶尔开玩笑。" },
            new AffinityStage { name = "CloseFriend", minAffinity = 61, personality = "玩家是你的挚友，你会说心里话，也会请 TA 帮忙。" },
            new AffinityStage { name = "Admire",      minAffinity = 81, personality = "你对玩家非常信赖甚至崇拜，语气热切。" },
        };

        [Header("依赖")]
        [SerializeField] private NpcMemory memory;
        [SerializeField] private MonoBehaviour controller;   // AINpcController / Streaming / Emotional

        public event Action<AffinityStage> OnStageChanged;

        public AffinityStage CurrentStage { get; private set; }

        void Awake()
        {
            if (memory     == null) memory     = GetComponent<NpcMemory>();
            if (controller == null)
            {
                controller = (MonoBehaviour)GetComponent<AINpcStreamingController>()
                          ?? (MonoBehaviour)GetComponent<AINpcController>()
                          ?? (MonoBehaviour)GetComponent<EmotionalNpcController>();
            }
        }

        void Start()
        {
            if (memory != null) memory.OnChanged += OnMemoryChanged;
            ApplyStageFor(memory != null ? memory.Data.affinity : 50, fireEvents: false);
        }

        void OnDestroy()
        {
            if (memory != null) memory.OnChanged -= OnMemoryChanged;
        }

        private void OnMemoryChanged()
        {
            if (memory != null) ApplyStageFor(memory.Data.affinity, fireEvents: true);
        }

        /// <summary>找到 affinity 对应的最高段位（从大到小找第一个 min &lt;= aff）</summary>
        public AffinityStage FindStage(int affinity)
        {
            AffinityStage found = null;
            for (int i = 0; i < stages.Count; i++)
            {
                if (affinity >= stages[i].minAffinity)
                {
                    if (found == null || stages[i].minAffinity > found.minAffinity)
                        found = stages[i];
                }
            }
            return found ?? (stages.Count > 0 ? stages[0] : null);
        }

        private void ApplyStageFor(int affinity, bool fireEvents)
        {
            var stage = FindStage(affinity);
            if (stage == null) return;
            if (CurrentStage != null && CurrentStage.name == stage.name) return;

            var prev = CurrentStage;
            CurrentStage = stage;

            // 注入 Personality 到 runner（覆盖 Controller 的默认 personality）
            var runner = RunnerAccessor.Get(controller);
            if (runner != null)
            {
                runner.SetVariable("Personality", stage.personality);
                runner.SetVariable("AffinityStage", stage.name);
            }

            // 切换蓝图（如果配置了）
            if (stage.overrideBlueprint != null && runner != null)
            {
                runner.LoadFromJson(stage.overrideBlueprint.text);
            }

            // 触发全局事件
            if (fireEvents)
            {
                if (!string.IsNullOrWhiteSpace(stage.unlockEvent) && WorldState.Instance != null)
                {
                    WorldState.Instance.AddEvent(stage.unlockEvent);
                }
                OnStageChanged?.Invoke(stage);
            }
        }

        // 用 NpcMemory 里的 RunnerAccessor 复用反射
        private static class RunnerAccessor
        {
            public static BPRunner Get(MonoBehaviour c)
            {
                if (c == null) return null;
                switch (c)
                {
                    case AINpcController basic:         return Get(basic);
                    case AINpcStreamingController s:    return Get(s);
                    case EmotionalNpcController e:      return Get(e);
                }
                return null;
            }
            public static BPRunner Get(AINpcController c)          => Field<AINpcController>().GetValue(c) as BPRunner;
            public static BPRunner Get(AINpcStreamingController c) => Field<AINpcStreamingController>().GetValue(c) as BPRunner;
            public static BPRunner Get(EmotionalNpcController c)   => Field<EmotionalNpcController>().GetValue(c) as BPRunner;
            private static System.Reflection.FieldInfo Field<T>() => typeof(T).GetField("_runner",
                System.Reflection.BindingFlags.NonPublic | System.Reflection.BindingFlags.Instance);
        }
    }
}
