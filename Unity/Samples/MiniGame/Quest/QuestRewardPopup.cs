// ─────────────────────────────────────────────────────────────────────
// QuestRewardPopup.cs — 任务完成奖励弹窗
//
// QuestSystem.OnQuestCompleted 时弹出，显示奖励内容，3 秒后自动消失。
// ─────────────────────────────────────────────────────────────────────

using System.Collections;
using UnityEngine;
using UnityEngine.UI;
using TMPro;

namespace BlueprintRuntime.Samples.MiniGame.Quest
{
    public class QuestRewardPopup : MonoBehaviour
    {
        [Header("UI 引用")]
        [SerializeField] private GameObject   popupPanel;
        [SerializeField] private TMP_Text     titleText;
        [SerializeField] private TMP_Text     rewardText;
        [SerializeField] private TMP_Text     descText;
        [SerializeField] private Button       closeButton;
        [SerializeField] private CanvasGroup  canvasGroup;

        [Header("动画")]
        [SerializeField] private float showDuration    = 3.5f;
        [SerializeField] private float fadeInDuration   = 0.3f;
        [SerializeField] private float fadeOutDuration  = 0.5f;
        [SerializeField] private AnimationCurve scaleCurve = AnimationCurve.EaseInOut(0, 0.5f, 1, 1f);

        private Coroutine _current;

        void Start()
        {
            if (popupPanel != null)
                popupPanel.SetActive(false);

            if (closeButton != null)
                closeButton.onClick.AddListener(Hide);

            if (QuestSystem.Instance != null)
                QuestSystem.Instance.OnQuestCompleted += OnComplete;
        }

        void OnDestroy()
        {
            if (QuestSystem.Instance != null)
                QuestSystem.Instance.OnQuestCompleted -= OnComplete;
        }

        private void OnComplete(QuestDefinition q)
        {
            Show(q.title, q.rewardDescription, q.description);
        }

        public void Show(string title, string reward, string desc = "")
        {
            if (popupPanel == null) return;

            if (_current != null) StopCoroutine(_current);
            _current = StartCoroutine(ShowRoutine(title, reward, desc));
        }

        public void Hide()
        {
            if (_current != null) StopCoroutine(_current);
            StartCoroutine(FadeOut());
        }

        private IEnumerator ShowRoutine(string title, string reward, string desc)
        {
            if (titleText  != null) titleText.text  = $"✨ 任务完成：{title}";
            if (rewardText != null) rewardText.text = $"奖励：{reward}";
            if (descText   != null) descText.text   = desc;

            popupPanel.SetActive(true);

            // Fade in + scale
            float t = 0f;
            while (t < fadeInDuration)
            {
                t += Time.deltaTime;
                float p = t / fadeInDuration;
                if (canvasGroup != null) canvasGroup.alpha = p;
                popupPanel.transform.localScale = Vector3.one * scaleCurve.Evaluate(p);
                yield return null;
            }

            if (canvasGroup != null) canvasGroup.alpha = 1f;
            popupPanel.transform.localScale = Vector3.one;

            // Wait
            yield return new WaitForSeconds(showDuration);

            // Fade out
            yield return FadeOut();
        }

        private IEnumerator FadeOut()
        {
            float t = 0f;
            float startAlpha = canvasGroup != null ? canvasGroup.alpha : 1f;
            while (t < fadeOutDuration)
            {
                t += Time.deltaTime;
                if (canvasGroup != null)
                    canvasGroup.alpha = Mathf.Lerp(startAlpha, 0f, t / fadeOutDuration);
                yield return null;
            }

            popupPanel.SetActive(false);
        }
    }
}
