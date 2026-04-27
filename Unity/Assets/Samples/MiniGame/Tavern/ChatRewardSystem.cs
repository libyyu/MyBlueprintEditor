// ChatRewardSystem.cs — 聊天奖励系统
// 每次 NPC 回复完成后自动奖励：金币 + 声望 + 好感度
// 第一次对话奖励翻倍，每日有上限

using System.Collections.Generic;
using UnityEngine;
using BlueprintRuntime.Samples.AINpc;

namespace BlueprintRuntime.Samples.MiniGame.Tavern
{
    public class ChatRewardSystem : MonoBehaviour
    {
        [Header("基础奖励")]
        [SerializeField] private int goldPerChat = 5;
        [SerializeField] private int repPerChat  = 1;
        [SerializeField] private int affinityPerChat = 2;

        [Header("首次对话奖励倍数")]
        [SerializeField] private int firstChatMultiplier = 3;

        [Header("每日上限")]
        [SerializeField] private int maxDailyChats = 20;

        // 今日已奖励次数
        private int _todayRewardCount;
        private string _lastRewardDate;
        private HashSet<string> _todayFirstChat = new HashSet<string>();

        public event System.Action<int, int, int> OnReward; // gold, rep, affinity

        void Start()
        {
            // 订阅所有 NPC 的 OnReplyDone
            foreach (var ctrl in FindObjectsOfType<AINpcStreamingController>())
            {
                var c = ctrl;
                ctrl.OnReplyDone += _ => OnNpcReplied(c);
            }
        }

        void OnNpcReplied(AINpcStreamingController ctrl)
        {
            // 每日重置
            string today = System.DateTime.Now.ToString("yyyy-MM-dd");
            if (_lastRewardDate != today)
            {
                _lastRewardDate = today;
                _todayRewardCount = 0;
                _todayFirstChat.Clear();
            }

            // 每日上限
            if (_todayRewardCount >= maxDailyChats) return;
            _todayRewardCount++;

            string npcId = ctrl.gameObject.name;
            int mul = 1;
            if (!_todayFirstChat.Contains(npcId))
            {
                _todayFirstChat.Add(npcId);
                mul = firstChatMultiplier;
            }

            int gold = goldPerChat * mul;
            int rep  = repPerChat * mul;
            int aff  = affinityPerChat;

            // 发奖
            PlayerStats.Instance?.AddGold(gold);
            TavernManager.Instance?.GuestSatisfied(0, rep); // 声望
            var memory = ctrl.GetComponent<Storage.NpcMemory>();
            memory?.AddAffinity(aff);

            OnReward?.Invoke(gold, rep, aff);
        }
    }
}
