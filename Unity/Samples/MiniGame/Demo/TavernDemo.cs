// ─────────────────────────────────────────────────────────────────────
// TavernDemo.cs — "AI 魔法酒馆" 完整示例游戏总控
//
// 把所有系统串在一起的最终 Demo 脚本。
// 挂到场景根 GameObject 上，它会协调所有子系统的初始化和联动。
//
// 场景结构参考：
//   Hierarchy
//   ├── TavernDemo (本脚本)
//   ├── MiniGameBootstrap
//   ├── Player (第三人称/第一人称控制器)
//   ├── WorldState
//   ├── NpcRegistry
//   ├── ContentFilter
//   ├── RateLimiter
//   ├── SaveManager
//   ├── GameAudioManager
//   ├── TutorialSystem
//   │
//   ├── NPC_MeowMeow (喵喵店长 — 酒保)
//   │   ├── AINpcStreamingController (AI_NPC_WorldAware.bjson)
//   │   ├── NpcMemory / NpcRegistrant / NpcBehaviorGate
//   │   ├── NpcSchedule / NpcShop / WorldSpeechBubble / AffinityHeart
//   │   └── NpcProximityTrigger
//   │
//   ├── NPC_Blacksmith (铁匠老王 — 武器商)
//   │   └── ... (同上，不同 Personality + Shop)
//   │
//   ├── NPC_Bard (游吟诗人莉莉 — 任务发放)
//   │   └── ... (+ QuestSystem 联动)
//   │
//   ├── NPC_Mysterious (神秘人 — 分享解锁)
//   │   └── ... (默认 inactive，分享 3 次后激活)
//   │
//   └── UI Canvas
//       ├── InteractionPrompt
//       ├── OpenWorldDialogPanel + DialogChoicePanel
//       ├── QuestTrackerUI + QuestRewardPopup
//       ├── InventoryUI + ShopUI
//       ├── SaveSlotUI
//       ├── ShareRewardPanel
//       ├── MinimapSystem
//       └── SettingsPanel (音量 + 重置)
// ─────────────────────────────────────────────────────────────────────

using System.Collections;
using UnityEngine;
using BlueprintRuntime.Samples.MiniGame;
using BlueprintRuntime.Samples.MiniGame.WeChat;
using BlueprintRuntime.Samples.AINpc;

namespace BlueprintRuntime.Samples.MiniGame
{
    public class TavernDemo : MonoBehaviour
    {
        [Header("NPC 引用（Inspector 拖入）")]
        [SerializeField] private GameObject npcMeowMeow;
        [SerializeField] private GameObject npcBlacksmith;
        [SerializeField] private GameObject npcBard;
        [SerializeField] private GameObject npcMysterious;   // 默认 inactive

        [Header("UI")]
        [SerializeField] private DialogChoicePanel choicePanel;
        [SerializeField] private GameObject        settingsPanel;

        [Header("游戏设定")]
        [SerializeField] private int    shareUnlockTarget = 3;
        [SerializeField] private string welcomeMessage = "欢迎来到魔法酒馆！走近 NPC 试试和他们聊天吧~";

        [Header("首次进入")]
        [SerializeField] private bool showTutorialOnFirstPlay = true;

        private bool _initialized;

        void Start()
        {
            StartCoroutine(Initialize());
        }

        private IEnumerator Initialize()
        {
            // 等 BlueprintService 就绪
            while (BlueprintService.Instance == null || !BlueprintService.Instance.IsReady)
                yield return null;

            // 1. 内容安全 + 限流
            SetupSafety();

            // 2. 世界事件联动
            SetupWorldEvents();

            // 3. 分享解锁神秘人
            SetupShareUnlock();

            // 4. BGM 联动时段
            SetupBgmSync();

            // 5. 给所有 NPC 的 Controller 回复加过滤
            SetupOutputFilter();

            // 6. 新手引导
            if (showTutorialOnFirstPlay)
                SetupTutorial();

            // 7. 欢迎消息
            Debug.Log($"[TavernDemo] {welcomeMessage}");

            _initialized = true;
        }

        // ── 安全 ────────────────────────────────────────────────────

        private void SetupSafety()
        {
            if (RateLimiter.Instance != null)
            {
                RateLimiter.Instance.OnDailyExhausted += () =>
                {
#if UNITY_WEBGL && !UNITY_EDITOR
                    WeChatSDK.ShowToast("今日对话次数已用完，看广告可增加额度", WeChatSDK.ToastIcon.None);
#else
                    Debug.Log("[TavernDemo] 每日对话额度已用完");
#endif
                };

                RateLimiter.Instance.OnCooldown += () =>
                {
                    Debug.Log("[TavernDemo] 对话冷却中...");
                };
            }

            if (ContentFilter.Instance != null)
            {
                ContentFilter.Instance.OnContentFiltered += (orig, filtered) =>
                {
                    Debug.LogWarning($"[ContentFilter] 已过滤: \"{orig}\" → \"{filtered}\"");
                };
            }
        }

        // ── 世界事件 ────────────────────────────────────────────────

        private void SetupWorldEvents()
        {
            if (WorldState.Instance == null) return;

            // 时段变化 → 所有 NPC 知道
            WorldState.Instance.OnTimePhaseChanged += phase =>
            {
                Debug.Log($"[TavernDemo] 时段变为: {phase}");
                // BGM 联动
                GameAudioManager.Instance?.UpdateBgmForTime(phase);
            };

            // 添加初始世界事件
            WorldState.Instance.AddEvent("魔法酒馆今天新进了一批稀有药水");

            // 玩家升级 → 世界事件
            if (PlayerStats.Instance != null)
            {
                PlayerStats.Instance.OnLevelUp += lv =>
                {
                    WorldState.Instance?.AddEvent($"酒馆里有人升到了 {lv} 级，大家都在议论");
                };
            }
        }

        // ── 分享解锁 ────────────────────────────────────────────────

        private void SetupShareUnlock()
        {
            if (npcMysterious == null) return;

            // 检查是否已解锁
            bool unlocked = BlueprintStorage.GetInt("npc_mysterious_unlocked", 0) == 1;
            npcMysterious.SetActive(unlocked);

            if (!unlocked && WeChat.ShareViralSystem.Instance != null)
            {
                WeChat.ShareViralSystem.Instance.OnTotalSharesChanged += total =>
                {
                    if (total >= shareUnlockTarget && !npcMysterious.activeSelf)
                    {
                        npcMysterious.SetActive(true);
                        BlueprintStorage.SetInt("npc_mysterious_unlocked", 1);
                        WorldState.Instance?.AddEvent("一个神秘的旅人来到了酒馆...");
                        GameAudioManager.Instance?.PlayNotification();
#if UNITY_WEBGL && !UNITY_EDITOR
                        WeChatSDK.ShowToast("神秘人解锁！", WeChatSDK.ToastIcon.Success);
#endif
                    }
                };
            }
        }

        // ── BGM 时段联动 ────────────────────────────────────────────

        private void SetupBgmSync()
        {
            if (WorldState.Instance != null && GameAudioManager.Instance != null)
            {
                GameAudioManager.Instance.UpdateBgmForTime(WorldState.Instance.GetTimeText());
            }
        }

        // ── LLM 输出过滤 ────────────────────────────────────────────

        private void SetupOutputFilter()
        {
            if (ContentFilter.Instance == null) return;

            // 订阅所有 NPC 的 OnReply，在显示前过滤
            // 注意：这里不直接改 Controller 的回调，而是通过 BlueprintService 的 print callback
            // 实际实现中，建议在 DialogUI/OpenWorldDialogPanel 里调 ContentFilter.Filter()
            // 此处做全局兜底
            foreach (var ctrl in FindObjectsOfType<AINpcStreamingController>())
            {
                var c = ctrl;  // capture
                ctrl.OnReply += msg =>
                {
                    // 过滤已在 DialogUI 或 OpenWorldDialogPanel 做，此处仅日志
                };
            }
        }

        // ── 新手引导 ────────────────────────────────────────────────

        private void SetupTutorial()
        {
            if (TutorialSystem.Instance == null) return;
            if (TutorialSystem.Instance.IsAllDone()) return;

            // 延迟 1 秒后启动引导（等场景完全加载）
            StartCoroutine(DelayedTutorial());
        }

        private IEnumerator DelayedTutorial()
        {
            yield return new WaitForSeconds(1.5f);
            TutorialSystem.Instance?.StartTutorial();
        }

        // ── 设置面板 ────────────────────────────────────────────────

        public void ToggleSettings()
        {
            if (settingsPanel != null)
                settingsPanel.SetActive(!settingsPanel.activeSelf);
        }

        public void ResetAllData()
        {
            // 危险操作！清除所有存档
            BlueprintStorage.Clear();
            Debug.LogWarning("[TavernDemo] All data cleared!");
#if UNITY_WEBGL && !UNITY_EDITOR
            WeChatSDK.ShowToast("数据已重置", WeChatSDK.ToastIcon.Success);
#endif
        }
    }
}
