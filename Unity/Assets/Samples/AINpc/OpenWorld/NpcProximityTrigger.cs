// NpcProximityTrigger.cs
// ─────────────────────────────────────────────────────────────────────────────
// 开放世界风 — 玩家靠近 NPC 时自动"打招呼" + 开启对话交互
//
// 三个触发阶段：
//   GreetRadius（打招呼半径，通常 8~12m）
//     玩家进入 → NPC 自动说一句问候（蓝图驱动，上下文带位置/时间）
//     玩家离开 → 清除气泡
//
//   InteractRadius（交互半径，通常 2~4m）
//     显示 "按 E 对话" 提示
//     玩家按 E → 进入对话模式（通常打开传统 UI 或锁定到 NPC）
//
//   ViewCheck（可选：视锥/遮挡检测）
//     只有玩家"看到"NPC 才触发（开放世界常见做法，避免远处 NPC 吵到玩家）
//
// 蓝图约定：
//   玩家进入 Greet 范围 → 调 Say("[系统] 玩家走近你，请用一句简短的话打招呼，不超过 15 字")
//   进入交互范围     → 不额外触发，只显示 UI
//   离开 Greet 范围    → 气泡自动超时淡出
// ─────────────────────────────────────────────────────────────────────────────

using System;
using UnityEngine;

namespace BlueprintRuntime.Samples.AINpc.OpenWorld
{
    public class NpcProximityTrigger : MonoBehaviour
    {
        [Header("玩家识别")]
        [Tooltip("玩家 Transform。留空则用 tag=\"Player\" 自动查找")]
        [SerializeField] private Transform player;

        [Header("数据源（任选其一）")]
        [SerializeField] private AINpcController basicNpc;
        [SerializeField] private AINpcStreamingController streamNpc;
        [SerializeField] private EmotionalNpcController emotionalNpc;

        [Header("范围设置")]
        [SerializeField] private float greetRadius    = 10f;   // 打招呼半径
        [SerializeField] private float interactRadius = 3f;    // 交互半径
        [SerializeField] private float leaveHysteresis = 1.5f; // 离开滞回（避免边缘抖动）

        [Header("视野检测（可选）")]
        [SerializeField] private bool requireLineOfSight = false;
        [SerializeField] private LayerMask occluderMask  = ~0;
        [SerializeField] private Transform eyeAnchor;          // NPC 眼部锚点（可选）

        [Header("触发冷却")]
        [Tooltip("同一 NPC 两次打招呼最小间隔（秒），避免反复进出刷屏")]
        [SerializeField] private float greetCooldown = 30f;

        [Header("问候语生成 Prompt")]
        [Tooltip("触发时传给 NPC 的玩家输入文本。{time} 会被替换为当前时段。")]
        [TextArea(2, 4)]
        [SerializeField] private string greetPrompt =
            "[系统提示] 玩家刚走到你附近（{time}）。请用符合你人设的一句话主动打招呼，不超过 20 字。";

        // ── 事件（供 UI 订阅） ────────────────────────────────────────
        /// <summary>玩家进入交互范围（显示"按 E 对话"提示）</summary>
        public event Action OnPlayerEnterInteract;
        /// <summary>玩家离开交互范围</summary>
        public event Action OnPlayerExitInteract;

        public bool  IsPlayerInGreet    { get; private set; }
        public bool  IsPlayerInInteract { get; private set; }

        private float _lastGreetTime = -9999f;

        void Start()
        {
            if (player == null)
            {
                var go = GameObject.FindGameObjectWithTag("Player");
                if (go != null) player = go.transform;
            }
        }

        void Update()
        {
            if (player == null) return;

            float dist = Vector3.Distance(transform.position, player.position);

            // ── Greet 层 ─────────────────────────────────────────
            if (!IsPlayerInGreet && dist < greetRadius)
            {
                if (!requireLineOfSight || HasLineOfSight())
                {
                    IsPlayerInGreet = true;
                    TryGreet();
                }
            }
            else if (IsPlayerInGreet && dist > greetRadius + leaveHysteresis)
            {
                IsPlayerInGreet = false;
            }

            // ── Interact 层 ──────────────────────────────────────
            if (!IsPlayerInInteract && dist < interactRadius)
            {
                IsPlayerInInteract = true;
                OnPlayerEnterInteract?.Invoke();
            }
            else if (IsPlayerInInteract && dist > interactRadius + leaveHysteresis * 0.5f)
            {
                IsPlayerInInteract = false;
                OnPlayerExitInteract?.Invoke();
            }
        }

        private bool HasLineOfSight()
        {
            Vector3 from = eyeAnchor != null ? eyeAnchor.position : transform.position + Vector3.up * 1.7f;
            Vector3 to   = player.position + Vector3.up * 1.5f;
            Vector3 dir  = to - from;
            float   dist = dir.magnitude;
            if (dist < 0.1f) return true;

            // 如果中间有遮挡（墙壁/建筑），不触发
            return !Physics.Raycast(from, dir.normalized, dist * 0.95f, occluderMask, QueryTriggerInteraction.Ignore);
        }

        private void TryGreet()
        {
            if (Time.time - _lastGreetTime < greetCooldown) return;
            _lastGreetTime = Time.time;

            string prompt = greetPrompt.Replace("{time}", TimeOfDay());

            if      (basicNpc     != null) basicNpc.Say(prompt);
            else if (streamNpc    != null) streamNpc.Say(prompt);
            else if (emotionalNpc != null) emotionalNpc.Say(prompt);
        }

        private static string TimeOfDay()
        {
            int h = DateTime.Now.Hour;
            if (h >= 5  && h < 11) return "清晨";
            if (h >= 11 && h < 13) return "中午";
            if (h >= 13 && h < 18) return "下午";
            if (h >= 18 && h < 22) return "傍晚";
            return "深夜";
        }

#if UNITY_EDITOR
        void OnDrawGizmosSelected()
        {
            Gizmos.color = new Color(0.2f, 0.6f, 1f, 0.35f);
            Gizmos.DrawWireSphere(transform.position, greetRadius);
            Gizmos.color = new Color(0.2f, 1f, 0.4f, 0.6f);
            Gizmos.DrawWireSphere(transform.position, interactRadius);
        }
#endif
    }
}
