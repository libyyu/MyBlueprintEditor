// TavernUI.cs — 酒馆经营 UI
// 顶部状态栏：酒馆名 | 等级 | 金币 | 声望
// 升级按钮、每日事件弹窗、NPC 解锁提示

using UnityEngine;
using UnityEngine.UI;

namespace BlueprintRuntime.Samples.MiniGame.Tavern
{
    public class TavernUI : MonoBehaviour
    {
        [Header("顶部状态栏")]
        [SerializeField] private Text tavernNameText;
        [SerializeField] private Text levelText;
        [SerializeField] private Text goldText;
        [SerializeField] private Text repText;
        [SerializeField] private Text guestsText;

        [Header("升级面板")]
        [SerializeField] private GameObject upgradePanel;
        [SerializeField] private Button     upgradeButton;
        [SerializeField] private Text       upgradeCostText;
        [SerializeField] private Text       upgradeDescText;

        [Header("通知")]
        [SerializeField] private GameObject notifyPanel;
        [SerializeField] private Text       notifyText;

        void Start()
        {
            if (upgradeButton != null)
                upgradeButton.onClick.AddListener(OnUpgradeClicked);

            if (TavernManager.Instance != null)
            {
                TavernManager.Instance.OnDataChanged += Refresh;
                TavernManager.Instance.OnLevelUp     += OnLevelUp;
                TavernManager.Instance.OnNpcUnlocked += OnNpcUnlocked;
                TavernManager.Instance.OnDailyEvent  += OnDailyEvent;
            }

            if (PlayerStats.Instance != null)
                PlayerStats.Instance.OnGoldChanged += _ => Refresh();

            if (upgradePanel != null) upgradePanel.SetActive(false);
            if (notifyPanel != null)  notifyPanel.SetActive(false);

            Refresh();
        }

        void Refresh()
        {
            var tm = TavernManager.Instance;
            var ps = PlayerStats.Instance;
            if (tm == null) return;

            if (tavernNameText != null) tavernNameText.text = tm.CurrentLevel.name;
            if (levelText != null)     levelText.text = $"Lv.{tm.Data.level}";
            if (goldText != null)      goldText.text = $"💰 {ps?.Gold ?? 0}";
            if (repText != null)       repText.text = $"⭐ {tm.Data.reputation}";
            if (guestsText != null)    guestsText.text = $"👤 今日 {tm.Data.todayGuests}";

            // 升级按钮状态
            var next = tm.NextLevel;
            if (upgradeButton != null)
                upgradeButton.interactable = next != null && ps != null
                    && ps.Gold >= next.upgradeCost
                    && tm.Data.reputation >= next.reputationReq;

            if (upgradeCostText != null && next != null)
                upgradeCostText.text = $"升级到 {next.name}\n需要 {next.upgradeCost} 💰 + {next.reputationReq} ⭐";
        }

        void OnUpgradeClicked()
        {
            if (TavernManager.Instance == null) return;

            if (upgradePanel != null && !upgradePanel.activeSelf)
            {
                upgradePanel.SetActive(true);
                var next = TavernManager.Instance.NextLevel;
                if (upgradeDescText != null && next != null)
                    upgradeDescText.text = next.description;
                return;
            }

            bool ok = TavernManager.Instance.TryUpgrade();
            if (ok)
            {
                if (upgradePanel != null) upgradePanel.SetActive(false);
            }
        }

        void OnLevelUp(int newLevel)
        {
            var cfg = TavernManager.Levels[newLevel - 1];
            ShowNotify($"🎉 酒馆升级！\n<b><size=28>{cfg.name}</size></b>\n{cfg.description}");
        }

        void OnNpcUnlocked(string npcId)
        {
            // 找 NPC 名字
            var ctrl = FindNpcController(npcId);
            string name = ctrl != null ? ctrl.NpcName : npcId;
            ShowNotify($"✨ 新伙伴加入！\n<b>{name}</b> 来到了你的酒馆！");
        }

        void OnDailyEvent(string text)
        {
            ShowNotify($"📜 今日事件\n{text}");
        }

        void ShowNotify(string text)
        {
            if (notifyPanel == null || notifyText == null) return;
            notifyText.text = text;
            notifyPanel.SetActive(true);
            CancelInvoke(nameof(HideNotify));
            Invoke(nameof(HideNotify), 4f);
        }

        void HideNotify()
        {
            if (notifyPanel != null) notifyPanel.SetActive(false);
        }

        static AINpc.AINpcStreamingController FindNpcController(string npcId)
        {
            foreach (var ctrl in FindObjectsOfType<AINpc.AINpcStreamingController>())
            {
                if (ctrl.gameObject.name == npcId) return ctrl;
            }
            return null;
        }
    }
}
