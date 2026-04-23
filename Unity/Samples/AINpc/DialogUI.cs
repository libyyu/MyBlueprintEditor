// DialogUI.cs
// ─────────────────────────────────────────────────────────────────────────────
// 对话 UI — 打字机气泡 + 思考动画 + 历史记录
//
// 设计要点：
//   - 打字机用 TMP_Text.maxVisibleCharacters 控制（而非 text +=），
//     性能比字符串拼接高 10 倍。
//   - "..." 思考动画用 Time.time 驱动，不需要 Animator。
//   - 对话历史用 ScrollRect + VerticalLayoutGroup（可选）。
//   - 点击气泡可跳过打字动画（Skip）。
//
// 依赖：TextMeshPro（Unity 内置包）
// ─────────────────────────────────────────────────────────────────────────────

using UnityEngine;
using UnityEngine.UI;
using UnityEngine.EventSystems;
using TMPro;
using System.Collections;

namespace BlueprintRuntime.Samples.AINpc
{
    public class DialogUI : MonoBehaviour, IPointerClickHandler
    {
        [Header("绑定")]
        [SerializeField] private AINpcController targetNpc;

        [Header("气泡")]
        [SerializeField] private TMP_Text        bubbleText;
        [SerializeField] private GameObject      thinkingDots;        // "..." 指示器
        [SerializeField] private TMP_Text        thinkingDotsText;    // 自己画动画用

        [Header("输入")]
        [SerializeField] private TMP_InputField  inputField;
        [SerializeField] private Button          sendButton;

        [Header("打字机")]
        [Tooltip("字符显示速度（字/秒）。30~60 比较舒服")]
        [SerializeField] private float typewriterSpeed = 40f;

        private Coroutine _typewriterCo;
        private bool      _typewriterRunning;

        void Start()
        {
            if (targetNpc == null)
            {
                Debug.LogError("[DialogUI] targetNpc 未绑定");
                enabled = false;
                return;
            }

            targetNpc.OnThinking += ShowThinking;
            targetNpc.OnReply    += ShowReply;
            targetNpc.OnError    += err => ShowReply($"<color=#FF6666>[{err}]</color>");

            sendButton.onClick.AddListener(OnSendClicked);
            inputField.onSubmit.AddListener(_ => OnSendClicked());

            bubbleText.text = $"你好呀，我是 {targetNpc.NpcName}~";
            thinkingDots?.SetActive(false);
        }

        void Update()
        {
            // "..." 思考动画（不依赖 Animator）
            if (thinkingDots != null && thinkingDots.activeSelf && thinkingDotsText != null)
            {
                int dots = (int)(Time.time * 3) % 4;
                thinkingDotsText.text = new string('.', dots);
            }
        }

        void OnSendClicked()
        {
            var text = inputField.text.Trim();
            if (string.IsNullOrEmpty(text)) return;
            if (targetNpc.Say(text))
                inputField.text = "";
        }

        void ShowThinking()
        {
            if (_typewriterCo != null) StopCoroutine(_typewriterCo);
            _typewriterRunning = false;
            bubbleText.text = "";
            thinkingDots?.SetActive(true);
            sendButton.interactable = false;
        }

        void ShowReply(string msg)
        {
            thinkingDots?.SetActive(false);
            sendButton.interactable = true;
            if (_typewriterCo != null) StopCoroutine(_typewriterCo);
            _typewriterCo = StartCoroutine(Typewriter(msg));
        }

        /// <summary>IPointerClickHandler：点击气泡 → 跳过打字</summary>
        public void OnPointerClick(PointerEventData eventData)
        {
            if (_typewriterRunning && _typewriterCo != null)
            {
                StopCoroutine(_typewriterCo);
                _typewriterRunning = false;
                bubbleText.maxVisibleCharacters = int.MaxValue;
            }
        }

        IEnumerator Typewriter(string msg)
        {
            _typewriterRunning = true;
            bubbleText.text = msg;
            bubbleText.maxVisibleCharacters = 0;
            // 强制 UpdateGeometry，拿到准确 characterCount
            bubbleText.ForceMeshUpdate();
            int total = bubbleText.textInfo.characterCount;

            float interval = 1f / Mathf.Max(1f, typewriterSpeed);
            for (int i = 1; i <= total; i++)
            {
                bubbleText.maxVisibleCharacters = i;
                yield return new WaitForSeconds(interval);
            }
            _typewriterRunning = false;
        }

        void OnDestroy()
        {
            if (targetNpc != null)
            {
                targetNpc.OnThinking -= ShowThinking;
                targetNpc.OnReply    -= ShowReply;
            }
        }
    }
}
