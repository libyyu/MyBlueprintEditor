// ─────────────────────────────────────────────────────────────────────
// RateLimiter.cs — 对话限流（防刷 LLM 费用）
//
// 三层限制：
//   1. 冷却（每次对话之间最少间隔 N 秒）
//   2. 每日额度（每天最多 N 次对话）
//   3. 每分钟频率（每分钟最多 N 次）
//
// 数据持久化到 BlueprintStorage（跨会话）
// ─────────────────────────────────────────────────────────────────────

using System;
using UnityEngine;

namespace BlueprintMiniGame
{
    public class RateLimiter : MonoBehaviour
    {
        public static RateLimiter Instance { get; private set; }

        [Header("冷却")]
        [Tooltip("两次对话之间最少间隔（秒）")]
        [SerializeField] private float cooldownSeconds = 2f;

        [Header("每日额度")]
        [SerializeField] private int  dailyLimit     = 100;  // 0 = 不限
        [Tooltip("额外额度（看广告/分享奖励）")]
        [SerializeField] private int  bonusDaily      = 0;

        [Header("频率限制")]
        [SerializeField] private int  perMinuteLimit  = 10;   // 0 = 不限

        // ── 事件 ────────────────────────────────────────────────────
        public event Action         OnCooldown;        // 冷却中
        public event Action         OnDailyExhausted;  // 每日额度用完
        public event Action<int>    OnUsageUpdated;    // 剩余次数

        private float _lastRequestTime = -999f;
        private int   _todayCount;
        private int   _minuteCount;
        private float _minuteResetTime;
        private string _todayKey;

        public int RemainingToday => Mathf.Max(0, dailyLimit + bonusDaily - _todayCount);
        public int TodayUsed => _todayCount;
        public float CooldownRemaining => Mathf.Max(0f, cooldownSeconds - (Time.realtimeSinceStartup - _lastRequestTime));
        public bool IsOnCooldown => CooldownRemaining > 0f;

        void Awake()
        {
            if (Instance != null) { Destroy(gameObject); return; }
            Instance = this;
        }

        void Start()
        {
            _todayKey = "rl_" + DateTime.Now.ToString("yyyyMMdd");
            _todayCount = BlueprintStorage.GetInt(_todayKey, 0);
            _minuteResetTime = Time.realtimeSinceStartup + 60f;
        }

        void Update()
        {
            // 每分钟重置计数
            if (Time.realtimeSinceStartup >= _minuteResetTime)
            {
                _minuteCount = 0;
                _minuteResetTime = Time.realtimeSinceStartup + 60f;
            }

            // 日期变更检测
            string newKey = "rl_" + DateTime.Now.ToString("yyyyMMdd");
            if (newKey != _todayKey)
            {
                _todayKey = newKey;
                _todayCount = 0;
                bonusDaily = 0;
            }
        }

        // ── 公共 API ────────────────────────────────────────────────

        /// <summary>请求一次对话权限。返回 true = 允许，false = 被限流</summary>
        public bool TryConsume()
        {
            // 冷却检查
            if (IsOnCooldown)
            {
                OnCooldown?.Invoke();
                return false;
            }

            // 每分钟频率
            if (perMinuteLimit > 0 && _minuteCount >= perMinuteLimit)
            {
                OnCooldown?.Invoke();
                return false;
            }

            // 每日额度
            if (dailyLimit > 0 && _todayCount >= dailyLimit + bonusDaily)
            {
                OnDailyExhausted?.Invoke();
                return false;
            }

            // 通过，记录
            _lastRequestTime = Time.realtimeSinceStartup;
            _todayCount++;
            _minuteCount++;
            BlueprintStorage.SetInt(_todayKey, _todayCount);
            OnUsageUpdated?.Invoke(RemainingToday);
            return true;
        }

        /// <summary>增加额外每日额度（看广告/分享奖励）</summary>
        public void AddBonusQuota(int amount)
        {
            bonusDaily += amount;
            OnUsageUpdated?.Invoke(RemainingToday);
        }

        /// <summary>重置每日计数（管理员/测试用）</summary>
        public void ResetDaily()
        {
            _todayCount = 0;
            bonusDaily = 0;
            BlueprintStorage.SetInt(_todayKey, 0);
        }
    }
}
