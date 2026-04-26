// ShareRewardPanel.cs
// ─────────────────────────────────────────────────────────────────────────────
// 分享奖励 UI Panel — 典型用例合集
//
// 支持三种常见模式：
//   1. 单次分享解锁：配置一个目标（如"分享 1 次解锁 NPC 铁匠"）
//   2. 累计进度：分享 N 次达成目标（3/5/10 阶梯奖励）
//   3. 每日分享：每天首次分享给额度
//
// 使用：把本脚本挂到 UI Panel（含 TMP_Text "进度条" + Button "去分享"）
// ─────────────────────────────────────────────────────────────────────────────

using UnityEngine;
using UnityEngine.UI;
using UnityEngine.Events;
using TMPro;

namespace BlueprintRuntime.Samples.MiniGame.WeChat
{
    public class ShareRewardPanel : MonoBehaviour
    {
        public enum RewardMode { OneShot, Cumulative, Daily }

        [Header("模式")]
        [SerializeField] private RewardMode mode = RewardMode.Cumulative;

        [Header("目标（累计模式下是次数目标）")]
        [SerializeField] private int targetShares = 3;

        [Header("奖励描述")]
        [SerializeField] private string rewardDesc = "解锁新 NPC：铁匠老王";

        [Header("UI 绑定")]
        [SerializeField] private TMP_Text titleText;
        [SerializeField] private TMP_Text progressText;
        [SerializeField] private Button   shareButton;
        [SerializeField] private Slider   progressBar;

        [Header("状态存档 key（用来记录是否已领过）")]
        [SerializeField] private string rewardClaimedKey = "Reward.unlock_blacksmith";

        [Header("发奖励回调")]
        [Tooltip("分享成功时触发。在这里写给玩家什么：如解锁 NPC、加金币、加 Token 等")]
        public UnityEvent onRewardClaimed;

        private ShareViralSystem Svc => ShareViralSystem.Instance;

        void Start()
        {
            if (shareButton != null)
                shareButton.onClick.AddListener(OnShareClicked);

            if (Svc != null)
            {
                Svc.OnRewardUnlocked += _ => Refresh();
            }

            Refresh();
        }

        void OnEnable()
        {
            Refresh();
        }

        void OnShareClicked()
        {
            if (IsAlreadyClaimed())
            {
                WeChatSDK.ShowToast("已领取过啦~", WeChatSDK.ToastIcon.None);
                return;
            }
            // 触发分享 + 奖励流程
            if (Svc == null)
            {
                Debug.LogError("[ShareRewardPanel] ShareViralSystem not found in scene");
                return;
            }
            Svc.ShareForReward(rewardDesc, TryGrantReward);
        }

        void TryGrantReward()
        {
            // 分享成功后，按模式判断是否满足解锁条件
            bool satisfied = mode switch
            {
                RewardMode.OneShot    => Svc.Stats.totalShares >= 1,
                RewardMode.Cumulative => Svc.Stats.totalShares >= targetShares,
                RewardMode.Daily      => Svc.HasSharedToday(),
                _ => false,
            };

            if (!satisfied)
            {
                Refresh();
                return;
            }

            if (IsAlreadyClaimed() && mode != RewardMode.Daily)
            {
                Refresh();
                return;
            }

            // 发奖励
            onRewardClaimed?.Invoke();
            MarkClaimed();
            Refresh();

            WeChatSDK.ShowToast("🎉 " + rewardDesc, WeChatSDK.ToastIcon.Success, 2500);
        }

        void Refresh()
        {
            if (Svc == null) return;

            int total = Svc.Stats.totalShares;
            bool claimed = IsAlreadyClaimed();

            if (titleText != null)
                titleText.text = rewardDesc;

            if (progressText != null)
            {
                progressText.text = mode switch
                {
                    RewardMode.OneShot    => claimed ? "已领取 ✓" : "分享 1 次即可领取",
                    RewardMode.Cumulative => claimed ? "已领取 ✓" : $"{Mathf.Min(total, targetShares)}/{targetShares}",
                    RewardMode.Daily      => Svc.HasSharedToday() ? $"今日已分享 · 连续 {Svc.DailyStreak} 天"
                                                                   : "今天还没分享哦",
                    _ => "",
                };
            }

            if (progressBar != null && mode == RewardMode.Cumulative)
            {
                progressBar.maxValue = targetShares;
                progressBar.value    = Mathf.Min(total, targetShares);
            }

            if (shareButton != null)
            {
                shareButton.interactable = !claimed || mode == RewardMode.Daily;
            }
        }

        bool IsAlreadyClaimed()
        {
            if (mode == RewardMode.Daily) return false;   // Daily 每天可领
            return BlueprintStorage.GetBool(rewardClaimedKey);
        }

        void MarkClaimed()
        {
            if (mode == RewardMode.Daily)
            {
                // Daily 用日期 key
                var day = System.DateTime.Now.ToString("yyyyMMdd");
                BlueprintStorage.SetString(rewardClaimedKey + ".day", day);
            }
            else
            {
                BlueprintStorage.SetBool(rewardClaimedKey, true);
            }
        }
    }
}
