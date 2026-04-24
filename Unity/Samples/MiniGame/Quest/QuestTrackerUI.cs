// ─────────────────────────────────────────────────────────────────────
// QuestTrackerUI.cs — 屏幕角落任务追踪面板
//
// 实时显示所有活跃任务（进度、描述、NPC）。
// 点击任务可展开详情 / 导航到 NPC。
// ─────────────────────────────────────────────────────────────────────

using System.Collections.Generic;
using UnityEngine;
using UnityEngine.UI;
using TMPro;

namespace BlueprintMiniGame.Quest
{
    public class QuestTrackerUI : MonoBehaviour
    {
        [Header("UI 引用")]
        [SerializeField] private RectTransform  listParent;     // 任务条目的容器
        [SerializeField] private GameObject     itemPrefab;     // 预制体：含 TMP_Text title / progress
        [SerializeField] private TMP_Text       headerText;     // "任务 (2/5)"
        [SerializeField] private Button         toggleButton;   // 折叠/展开
        [SerializeField] private CanvasGroup    panelGroup;     // 整体透明度（战斗时可半透明）
        [SerializeField] private GameObject     emptyHint;      // "暂无任务"

        [Header("行为")]
        [SerializeField] private float refreshInterval = 1f;
        [SerializeField] private bool  showCompleted   = false;  // 完成后保留 1 秒再消失
        [SerializeField] private int   maxVisible      = 5;

        // ── 内部 ────────────────────────────────────────────────────
        private readonly List<QuestItemView> _views = new();
        private bool _collapsed;
        private float _refreshTimer;

        private struct QuestItemView
        {
            public GameObject  go;
            public TMP_Text    titleText;
            public TMP_Text    progressText;
            public Image       statusIcon;
            public string      questId;
        }

        void Start()
        {
            if (toggleButton != null)
                toggleButton.onClick.AddListener(ToggleCollapse);

            if (QuestSystem.Instance != null)
            {
                QuestSystem.Instance.OnQuestAccepted  += _ => RefreshNow();
                QuestSystem.Instance.OnQuestCompleted += _ => RefreshNow();
            }

            RefreshNow();
        }

        void Update()
        {
            _refreshTimer -= Time.deltaTime;
            if (_refreshTimer <= 0f)
            {
                _refreshTimer = refreshInterval;
                Refresh();
            }
        }

        public void RefreshNow()
        {
            _refreshTimer = refreshInterval;
            Refresh();
        }

        // ── 刷新 ────────────────────────────────────────────────────
        private void Refresh()
        {
            if (QuestSystem.Instance == null) return;

            var active = QuestSystem.Instance.GetActiveQuests();
            int count = Mathf.Min(active.Count, maxVisible);

            // 确保 view 池足够
            while (_views.Count < count)
                _views.Add(CreateItem());

            // 隐藏多余
            for (int i = count; i < _views.Count; i++)
                _views[i].go.SetActive(false);

            // 更新内容
            for (int i = 0; i < count; i++)
            {
                var q = active[i];
                var v = _views[i];
                v.go.SetActive(!_collapsed);
                v.questId = q.id;

                if (v.titleText != null)
                {
                    string statusEmoji = q.status switch
                    {
                        QuestSystem.QuestStatus.Active    => "📌",
                        QuestSystem.QuestStatus.Completed => "✅",
                        QuestSystem.QuestStatus.Failed    => "❌",
                        _ => "📋"
                    };
                    v.titleText.text = $"{statusEmoji} {q.title}";
                }

                if (v.progressText != null)
                {
                    if (q.targetCount > 1)
                        v.progressText.text = $"{q.currentCount}/{q.targetCount}";
                    else
                        v.progressText.text = q.description;
                }

                if (v.statusIcon != null)
                {
                    v.statusIcon.color = q.status switch
                    {
                        QuestSystem.QuestStatus.Active    => new Color(1f, 0.85f, 0.3f),
                        QuestSystem.QuestStatus.Completed => new Color(0.3f, 0.9f, 0.4f),
                        QuestSystem.QuestStatus.Failed    => new Color(0.9f, 0.3f, 0.3f),
                        _ => Color.white
                    };
                }
            }

            // Header
            if (headerText != null)
            {
                int total = active.Count;
                int done  = 0;
                foreach (var q in active)
                    if (q.status == QuestSystem.QuestStatus.Completed) done++;
                headerText.text = $"任务 ({done}/{total})";
            }

            // 空提示
            if (emptyHint != null)
                emptyHint.SetActive(count == 0 && !_collapsed);
        }

        private QuestItemView CreateItem()
        {
            if (itemPrefab == null)
            {
                Debug.LogError("[QuestTrackerUI] itemPrefab is null");
                return default;
            }

            var go = Instantiate(itemPrefab, listParent);
            return new QuestItemView
            {
                go           = go,
                titleText    = go.transform.Find("Title")?.GetComponent<TMP_Text>(),
                progressText = go.transform.Find("Progress")?.GetComponent<TMP_Text>(),
                statusIcon   = go.transform.Find("Icon")?.GetComponent<Image>(),
                questId      = ""
            };
        }

        private void ToggleCollapse()
        {
            _collapsed = !_collapsed;
            RefreshNow();

            if (toggleButton != null)
            {
                var txt = toggleButton.GetComponentInChildren<TMP_Text>();
                if (txt != null) txt.text = _collapsed ? "▶" : "▼";
            }
        }
    }
}
