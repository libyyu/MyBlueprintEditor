// ─────────────────────────────────────────────────────────────────────
// NpcSchedule.cs — NPC 日程系统
//
// 根据 WorldState 的时间段（清晨/上午/中午/下午/傍晚/夜晚/深夜）
// 自动切换 NPC 的位置、行为、可交互性。
//
// 用法：挂到 NPC 上，在 Inspector 配置每个时段的目标位置。
// ─────────────────────────────────────────────────────────────────────

using BlueprintRuntime.Samples.AINpc;
using BlueprintRuntime.Samples.AINpc.OpenWorld;
using System;
using UnityEngine;

namespace BlueprintRuntime.Samples.MiniGame
{
    public class NpcSchedule : MonoBehaviour
    {
        [Serializable]
        public class ScheduleEntry
        {
            [Tooltip("时段名称（需与 WorldState.GetTimeText 输出一致）")]
            public string timePhase = "白天";
            [Tooltip("该时段 NPC 的目标位置")]
            public Transform targetPosition;
            [Tooltip("该时段 NPC 是否可交互")]
            public bool interactable = true;
            [Tooltip("该时段 NPC 的状态描述（注入到蓝图 NpcActivity 变量）")]
            public string activity = "在店里忙碌";
            [Tooltip("移动速度")]
            public float moveSpeed = 3f;
        }

        [Header("日程表")]
        [SerializeField] private ScheduleEntry[] schedule;

        [Header("行为")]
        [SerializeField] private float checkInterval = 5f;
        [SerializeField] private float arriveThreshold = 0.5f;

        [Header("不可交互时")]
        [Tooltip("NPC 不可交互时的提示（如\"店长已经休息了\"）")]
        [SerializeField] private string unavailableMessage = "（NPC 正在休息，无法对话）";

        // ── 状态 ────────────────────────────────────────────────────
        public string     CurrentActivity  { get; private set; } = "";
        public bool       IsInteractable   { get; private set; } = true;
        public event Action<string> OnActivityChanged;

        private float   _timer;
        private string  _lastPhase = "";
        private NpcProximityTrigger _trigger;
        private Animator _animator;

        void Start()
        {
            _trigger = GetComponent<NpcProximityTrigger>();
            _animator = GetComponent<Animator>();
            Check();
        }

        void Update()
        {
            _timer -= Time.deltaTime;
            if (_timer <= 0f)
            {
                _timer = checkInterval;
                Check();
            }

            // 向目标移动
            MoveToTarget();
        }

        private void Check()
        {
            if (WorldState.Instance == null || schedule == null) return;

            string phase = WorldState.Instance.GetTimeText();
            if (phase == _lastPhase) return;
            _lastPhase = phase;

            // 查找匹配的日程
            ScheduleEntry entry = null;
            foreach (var e in schedule)
            {
                if (string.Equals(e.timePhase, phase, StringComparison.OrdinalIgnoreCase))
                {
                    entry = e;
                    break;
                }
            }

            if (entry == null)
            {
                // 没有匹配的日程，默认可交互
                IsInteractable = true;
                CurrentActivity = "";
                return;
            }

            IsInteractable = entry.interactable;
            CurrentActivity = entry.activity;

            // 控制 ProximityTrigger
            if (_trigger != null)
                _trigger.enabled = entry.interactable;

            // 通知
            OnActivityChanged?.Invoke(CurrentActivity);

            // 注入蓝图
            InjectActivity();
        }

        private void MoveToTarget()
        {
            if (schedule == null) return;

            ScheduleEntry entry = null;
            foreach (var e in schedule)
                if (string.Equals(e.timePhase, _lastPhase, StringComparison.OrdinalIgnoreCase))
                { entry = e; break; }

            if (entry?.targetPosition == null) return;

            Vector3 target = entry.targetPosition.position;
            Vector3 current = transform.position;
            float dist = Vector3.Distance(current, target);

            if (dist > arriveThreshold)
            {
                Vector3 dir = (target - current).normalized;
                transform.position += dir * entry.moveSpeed * Time.deltaTime;

                // 面向移动方向
                if (dir.sqrMagnitude > 0.01f)
                {
                    dir.y = 0;
                    transform.rotation = Quaternion.Slerp(
                        transform.rotation, Quaternion.LookRotation(dir), Time.deltaTime * 5f);
                }

                // 动画：走路
                if (_animator != null)
                    _animator.SetBool("IsWalking", true);
            }
            else
            {
                if (_animator != null)
                    _animator.SetBool("IsWalking", false);
            }
        }

        private void InjectActivity()
        {
            // 通过反射拿 runner 注入 NpcActivity 变量
            var mem = GetComponent<NpcMemory>();
            if (mem == null) return;

            // 借用 NpcRegistrant 的 InjectNow 机制——在下次对话前自动注入
            // 这里直接设变量
            var runner = NpcRegistry.RunnerAccessor.Get(
                (MonoBehaviour)GetComponent<AINpcStreamingController>()
                ?? (MonoBehaviour)GetComponent<AINpcController>()
                ?? GetComponent<EmotionalNpcController>());

            if (runner != null)
                runner.SetVariable("NpcActivity", CurrentActivity);
        }

        /// <summary>外部查询：NPC 不可交互时的提示语</summary>
        public string GetUnavailableMessage() => unavailableMessage;
    }
}
