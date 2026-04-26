// StreamingDialogUI.cs
// ─────────────────────────────────────────────────────────────────────────────
// 流式对话 UI — token 实时追加、光标闪烁、自动滚动
//
// 视觉效果：
//   玩家按发送 → [...] 思考
//   第一个 token 到达 → 清空气泡，开始追加文字
//   每个 token → 立即出现（真实感 > 打字机模拟）
//   完成 → 光标消失，UI 允许下一次输入
//
// WebGL 下：StreamAsync 降级为整体返回，相当于"一次性出现全文"——
//   这时用"打字机模拟"包装一下，让体验一致：
//     if 回复很长(>50字) → 模拟打字机逐字显示
//     if 回复很短       → 直接显示
// ─────────────────────────────────────────────────────────────────────────────

using UnityEngine;
using UnityEngine.UI;
using TMPro;
using System.Collections;
using System.Text;

namespace BlueprintRuntime.Samples.AINpc
{
    public class StreamingDialogUI : MonoBehaviour
    {
        [Header("绑定")]
        [SerializeField] private AINpcStreamingController targetNpc;

        [Header("UI")]
        [SerializeField] private TMP_Text       bubbleText;
        [SerializeField] private GameObject     thinkingDots;
        [SerializeField] private TMP_Text       thinkingDotsText;
        [SerializeField] private TMP_InputField inputField;
        [SerializeField] private Button         sendButton;

        [Header("视觉")]
        [Tooltip("光标闪烁字符，为空关闭")]
        [SerializeField] private string cursor = "|";

        [Tooltip("WebGL 降级模式下，每字等待（秒）。设 0 = 直接显示全文")]
        [SerializeField] private float webglTypewriterInterval = 0.02f;

        private readonly StringBuilder _buf = new StringBuilder(512);
        private Coroutine _simulateCo;
        private bool      _cursorOn;

        void Start()
        {
            if (targetNpc == null)
            {
                Debug.LogError("[StreamingDialogUI] targetNpc 未绑定");
                enabled = false; return;
            }

            targetNpc.OnThinking   += ShowThinking;
            targetNpc.OnReplyStart += ShowReplyStart;
            targetNpc.OnChunk      += OnChunk;
            targetNpc.OnReplyDone  += OnReplyDone;
            targetNpc.OnError      += err => {
                StopSimulate();
                bubbleText.text = $"<color=#FF6666>[{err}]</color>";
                SetSending(false);
            };

            sendButton.onClick.AddListener(OnSendClicked);
            inputField.onSubmit.AddListener(_ => OnSendClicked());

            bubbleText.text = $"你好呀，我是 {targetNpc.NpcName}~";
            thinkingDots?.SetActive(false);
        }

        void Update()
        {
            // 思考点动画
            if (thinkingDots != null && thinkingDots.activeSelf && thinkingDotsText != null)
            {
                int dots = (int)(Time.time * 3) % 4;
                thinkingDotsText.text = new string('.', dots);
            }

            // 光标闪烁（仅在流式中）
            if (targetNpc != null && targetNpc.IsStreaming && !string.IsNullOrEmpty(cursor))
            {
                bool on = ((int)(Time.time * 2)) % 2 == 0;
                if (on != _cursorOn)
                {
                    _cursorOn = on;
                    RenderBuffer();
                }
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
            StopSimulate();
            _buf.Clear();
            bubbleText.text = "";
            thinkingDots?.SetActive(true);
            SetSending(false);
        }

        void ShowReplyStart()
        {
            thinkingDots?.SetActive(false);
            _buf.Clear();
            RenderBuffer();
        }

        void OnChunk(string token)
        {
            // WebGL 降级：一个 chunk 就是完整回复，用打字机模拟真·流式
#if UNITY_WEBGL && !UNITY_EDITOR
            if (webglTypewriterInterval > 0 && token.Length > 20)
            {
                StopSimulate();
                _simulateCo = StartCoroutine(SimulateStreaming(token, webglTypewriterInterval));
                return;
            }
#endif
            _buf.Append(token);
            RenderBuffer();
        }

        IEnumerator SimulateStreaming(string fullText, float interval)
        {
            _buf.Clear();
            for (int i = 0; i < fullText.Length; i++)
            {
                _buf.Append(fullText[i]);
                RenderBuffer();
                yield return new WaitForSeconds(interval);
            }
            _simulateCo = null;
        }

        void OnReplyDone(string fullText)
        {
            // 如果正在模拟打字，等它结束再标完成
            if (_simulateCo != null) return;
            FinalizeReply(fullText);
        }

        void FinalizeReply(string fullText)
        {
            // 以 DONE 的 fullText 为准（防止 chunk 拼接异常）
            _buf.Clear();
            _buf.Append(fullText);
            _cursorOn = false;
            RenderBuffer();
            SetSending(true);
        }

        void RenderBuffer()
        {
            if (targetNpc != null && targetNpc.IsStreaming && _cursorOn && !string.IsNullOrEmpty(cursor))
                bubbleText.text = _buf.ToString() + cursor;
            else
                bubbleText.text = _buf.ToString();
        }

        void SetSending(bool canSend)
        {
            sendButton.interactable = canSend;
            if (canSend) inputField.ActivateInputField();
        }

        void StopSimulate()
        {
            if (_simulateCo != null)
            {
                StopCoroutine(_simulateCo);
                _simulateCo = null;
            }
        }

        void OnDestroy()
        {
            if (targetNpc != null)
            {
                targetNpc.OnThinking   -= ShowThinking;
                targetNpc.OnReplyStart -= ShowReplyStart;
                targetNpc.OnChunk      -= OnChunk;
                targetNpc.OnReplyDone  -= OnReplyDone;
            }
        }
    }
}
