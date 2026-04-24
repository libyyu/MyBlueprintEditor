// AffinityHeart.cs
// ─────────────────────────────────────────────────────────────────────────────
// NPC 头顶好感度 UI — "❤️ ❤️ ❤️ ♡ ♡"
//
// 特性：
//   - 跟随 NPC 位置（World Space Canvas）
//   - 好感度变化时动画提示（+1 ❤️ 飘字）
//   - 5 颗心分级：0-20=0颗 / 21-40=1颗 / 41-60=2颗 / 61-80=3颗 / 81-95=4颗 / 96+=5颗
//   - 距离玩家远时自动隐藏
//
// 使用：
//   和 WorldSpeechBubble 挂在同一个子 Canvas 上即可
// ─────────────────────────────────────────────────────────────────────────────

using System.Collections;
using System.Text;
using UnityEngine;
using TMPro;

namespace BlueprintRuntime.Samples.MiniGame
{
    [RequireComponent(typeof(CanvasGroup))]
    public class AffinityHeart : MonoBehaviour
    {
        [Header("数据源")]
        [SerializeField] private NpcMemory memory;

        [Header("UI")]
        [SerializeField] private TMP_Text heartsText;
        [SerializeField] private TMP_Text popupText;    // 用于飘 "+1 ❤️"
        [SerializeField] private CanvasGroup canvasGroup;

        [Header("可见范围")]
        [SerializeField] private Camera  targetCamera;
        [SerializeField] private float   maxVisibleDistance = 15f;
        [SerializeField] private float   fadeStartDistance  = 10f;

        [Header("样式")]
        [Tooltip("满心字符（黑/彩）")]
        [SerializeField] private string heartFull  = "❤";
        [Tooltip("空心字符")]
        [SerializeField] private string heartEmpty = "♡";
        [SerializeField] private Color  heartColor  = new Color(1f, 0.35f, 0.45f);
        [SerializeField] private int    maxHearts   = 5;

        [Header("飘字动画")]
        [SerializeField] private float popupDuration = 1.5f;
        [SerializeField] private float popupRiseHeight = 30f;

        private int _lastLevel = -1;

        void Reset()
        {
            canvasGroup = GetComponent<CanvasGroup>();
        }

        void Awake()
        {
            if (canvasGroup == null) canvasGroup = GetComponent<CanvasGroup>();
            if (targetCamera == null) targetCamera = Camera.main;
            if (memory == null)       memory = GetComponentInParent<NpcMemory>();
            if (popupText  != null)   popupText.text = "";
        }

        void OnEnable()
        {
            if (memory != null) memory.OnChanged += OnMemoryChanged;
            RefreshImmediate();
        }

        void OnDisable()
        {
            if (memory != null) memory.OnChanged -= OnMemoryChanged;
        }

        void LateUpdate()
        {
            // 距离淡入淡出（跟 WorldSpeechBubble 独立）
            if (targetCamera == null) return;
            float dist = Vector3.Distance(targetCamera.transform.position, transform.position);
            float alpha;
            if      (dist >= maxVisibleDistance) alpha = 0f;
            else if (dist >  fadeStartDistance)  alpha = 1f - (dist - fadeStartDistance) / (maxVisibleDistance - fadeStartDistance);
            else                                 alpha = 1f;
            canvasGroup.alpha = alpha;
        }

        private void OnMemoryChanged()
        {
            if (memory == null) return;
            int newLevel = LevelFromAffinity(memory.Data.affinity);
            bool changed = newLevel != _lastLevel;
            RefreshImmediate();
            if (changed && _lastLevel >= 0)
            {
                int delta = newLevel - _lastLevel;
                ShowPopup(delta > 0 ? $"+{delta} ❤" : $"{delta} ❤",
                          delta > 0 ? heartColor : Color.gray);
            }
            _lastLevel = newLevel;
        }

        private void RefreshImmediate()
        {
            if (memory == null || heartsText == null) return;
            int level = LevelFromAffinity(memory.Data.affinity);
            var sb = new StringBuilder(maxHearts * 2);
            for (int i = 0; i < maxHearts; i++)
                sb.Append(i < level ? heartFull : heartEmpty);
            heartsText.text = sb.ToString();
            heartsText.color = heartColor;
            _lastLevel = level;
        }

        private int LevelFromAffinity(int aff)
        {
            if (aff >= 96) return 5;
            if (aff >= 81) return 4;
            if (aff >= 61) return 3;
            if (aff >= 41) return 2;
            if (aff >= 21) return 1;
            return 0;
        }

        private void ShowPopup(string text, Color color)
        {
            if (popupText == null) return;
            StopAllCoroutines();
            StartCoroutine(PopupCo(text, color));
        }

        private IEnumerator PopupCo(string text, Color color)
        {
            popupText.text = text;
            popupText.color = color;
            var rt = popupText.rectTransform;
            var startPos = rt.anchoredPosition;
            float elapsed = 0f;
            while (elapsed < popupDuration)
            {
                elapsed += Time.deltaTime;
                float t = elapsed / popupDuration;
                rt.anchoredPosition = startPos + new Vector2(0, popupRiseHeight * t);
                var c = color;
                c.a = 1f - t;
                popupText.color = c;
                yield return null;
            }
            popupText.text = "";
            rt.anchoredPosition = startPos;
        }
    }
}
