// ShareViralSystem.cs
// ─────────────────────────────────────────────────────────────────────────────
// 分享裂变系统 — 让玩家主动分享游戏换取奖励
//
// 核心玩法（三种常见模式，本类都支持）：
//   1. 单次分享奖励：分享 1 次 → 加 Token 额度 / NPC 解锁
//   2. 累计分享：分享 3/5/10 次达成进度 → 解锁成就
//   3. 每日分享：每天首次分享 → 每日奖励
//
// 微信小游戏分享机制要点：
//   - wx.shareAppMessage 只是"调起分享面板"，玩家是否真的点确认分享，
//     微信不会告诉你（防止刷）
//   - 折中方案：假定点了"分享"按钮就算一次；用延迟 + focus 回归做二次校验
//   - 被分享的链接用 query 参数带 inviterOpenid；新玩家进入时解析 query
//     并调 wx.getLaunchOptionsSync → 反馈给邀请人（要后端配合）
//
// 本类提供的能力：
//   - 记录玩家总分享次数、今日分享次数、最后分享时间（跨会话）
//   - 事件：OnShareTriggered / OnRewardUnlocked
//   - 一键调 ShareForReward(奖励描述, onGranted) 触发分享+奖励流程
//
// 存档 key 前缀："ShareStats"
// ─────────────────────────────────────────────────────────────────────────────

using System;
using UnityEngine;

namespace BlueprintRuntime.Samples.MiniGame.WeChat
{
    [Serializable]
    public class ShareStatsData
    {
        public int  totalShares       = 0;
        public int  todayShares       = 0;
        public long lastShareTime     = 0;   // Unix 秒
        public long lastShareDayKey   = 0;   // YYYYMMDD
        public int  dailyBonusStreak  = 0;   // 连续每日分享天数
    }

    public class ShareViralSystem : MonoBehaviour
    {
        public static ShareViralSystem Instance { get; private set; }

        [Header("分享素材（微信分享卡片）")]
        [SerializeField] private string shareTitle    = "我在 AI 酒馆和猫咪店长聊了半小时！你也来试试喵~";
        [SerializeField] private string shareImageUrl = "";   // 建议 500x400

        [Header("奖励冷却")]
        [Tooltip("两次\"有效分享\"的最小间隔（秒），防刷")]
        [SerializeField] private int cooldownSeconds = 30;

        [Header("debug / editor 模拟")]
        [Tooltip("在 Editor 下每次 ShareForReward 立即给奖励（实际小游戏里靠 wx.shareAppMessage）")]
        [SerializeField] private bool mockShareInEditor = true;

        public ShareStatsData Stats { get; private set; } = new ShareStatsData();

        /// <summary>玩家触发一次分享（不保证成功）</summary>
        public event Action OnShareTriggered;
        /// <summary>分享完成、奖励已发放（参数：奖励描述）</summary>
        public event Action<string> OnRewardUnlocked;

        private const string StorageKey = "ShareStats";

        void Awake()
        {
            if (Instance != null && Instance != this) { Destroy(gameObject); return; }
            Instance = this;
            DontDestroyOnLoad(gameObject);
            Load();
            RolloverDayIfNeeded();
        }

        // ── 公共 API ────────────────────────────────────────────────
        /// <summary>
        /// 发起"分享换奖励"流程。
        /// </summary>
        /// <param name="rewardDesc">奖励文本（用于通知玩家）</param>
        /// <param name="onGranted">确认发放奖励的回调（由业务侧实际执行奖励逻辑）</param>
        /// <param name="extraQuery">分享链接附带的 query 参数（如 inviterOpenId=xxx）</param>
        public void ShareForReward(string rewardDesc, Action onGranted, string extraQuery = "")
        {
            if (IsOnCooldown())
            {
                Debug.Log($"[Share] 冷却中，请稍后（剩余 {RemainingCooldown()}s）");
                WeChatSDK.ShowToast($"太快啦！{RemainingCooldown()}s 后再来", WeChatSDK.ToastIcon.None, 1500);
                return;
            }

            OnShareTriggered?.Invoke();

            // 调起微信分享面板（小游戏）/ Editor 模拟
#if UNITY_EDITOR || (UNITY_STANDALONE && !UNITY_WEBGL)
            if (mockShareInEditor)
            {
                Debug.Log($"[Share/mock] 立即发放奖励：{rewardDesc}");
                FinalizeShare(rewardDesc, onGranted);
                return;
            }
#endif
            WeChatSDK.Share(shareTitle, shareImageUrl, extraQuery);

            // 微信无法告诉我们分享是否成功，只能启发式：
            //   - 假定分享面板弹出后玩家点了"确认分享"
            //   - 3 秒后发奖励（过短可能是误触取消，过长玩家会抱怨）
            CancelInvoke(nameof(GrantPendingReward));
            _pendingRewardDesc = rewardDesc;
            _pendingOnGranted  = onGranted;
            Invoke(nameof(GrantPendingReward), 3f);
        }

        private string _pendingRewardDesc;
        private Action _pendingOnGranted;

        private void GrantPendingReward()
        {
            if (_pendingOnGranted == null) return;
            FinalizeShare(_pendingRewardDesc, _pendingOnGranted);
            _pendingRewardDesc = null;
            _pendingOnGranted  = null;
        }

        private void FinalizeShare(string rewardDesc, Action onGranted)
        {
            // 更新统计
            RolloverDayIfNeeded();
            Stats.totalShares++;
            Stats.todayShares++;
            Stats.lastShareTime = DateTimeOffset.UtcNow.ToUnixTimeSeconds();
            // 连续每日分享天数（如果今天是第一次分享，判断昨天有没有）
            if (Stats.todayShares == 1)
            {
                long yesterday = TodayKey(-1);
                Stats.dailyBonusStreak = Stats.lastShareDayKey == yesterday ? Stats.dailyBonusStreak + 1 : 1;
            }
            Stats.lastShareDayKey = TodayKey();
            Save();

            onGranted?.Invoke();
            OnRewardUnlocked?.Invoke(rewardDesc ?? "");
            WeChatSDK.ShowToast("分享成功！奖励已发放", WeChatSDK.ToastIcon.Success, 1500);
        }

        /// <summary>纯分享不给奖励（比如玩家主动点分享按钮）</summary>
        public void ShareOnly(string extraQuery = "")
        {
            WeChatSDK.Share(shareTitle, shareImageUrl, extraQuery);
        }

        // ── 成就查询 ────────────────────────────────────────────────
        /// <summary>达成指定累计分享次数（3/5/10 等阶梯）</summary>
        public bool HasReachedTotal(int n) => Stats.totalShares >= n;
        /// <summary>今天是否已分享</summary>
        public bool HasSharedToday() => Stats.todayShares > 0;
        /// <summary>连续每日分享天数</summary>
        public int DailyStreak => Stats.dailyBonusStreak;

        // ── 内部 ───────────────────────────────────────────────────────
        private bool IsOnCooldown() =>
            DateTimeOffset.UtcNow.ToUnixTimeSeconds() - Stats.lastShareTime < cooldownSeconds;

        private int RemainingCooldown() =>
            Math.Max(0, cooldownSeconds - (int)(DateTimeOffset.UtcNow.ToUnixTimeSeconds() - Stats.lastShareTime));

        private void RolloverDayIfNeeded()
        {
            long today = TodayKey();
            if (Stats.lastShareDayKey != today)
                Stats.todayShares = 0;
        }

        private static long TodayKey(int offsetDays = 0)
        {
            var d = DateTime.Now.AddDays(offsetDays);
            return d.Year * 10000L + d.Month * 100L + d.Day;
        }

        private void Load()
        {
            Stats = BlueprintStorage.GetJson<ShareStatsData>(StorageKey) ?? new ShareStatsData();
        }

        private void Save() => BlueprintStorage.SetJson(StorageKey, Stats);
    }
}
