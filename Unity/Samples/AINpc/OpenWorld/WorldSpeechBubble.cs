// WorldSpeechBubble.cs
// ─────────────────────────────────────────────────────────────────────────────
// 开放世界风 3D 气泡 — 挂在 NPC 头顶的 World Space Canvas
//
// 特性：
//   ✅ 跟随 NPC 移动（每帧更新位置）
//   ✅ 始终面对相机（Billboard 模式，避免侧面看扁）
//   ✅ 距离自动缩放（远处小、近处大，保持视觉一致性）
//   ✅ 远离相机自动淡出隐藏
//   ✅ 打字机 + 光标闪烁
//   ✅ 自动收起：静默 N 秒后淡出
//   ✅ 与 StreamingController 无缝对接（token 实时追加）
//
// 使用方式：
//   1. 在 NPC GameObject 下新建子物体 SpeechBubble
//   2. 添加 Canvas 组件，Render Mode = World Space
//   3. Canvas 内放 Image（背景）+ TMP_Text（文字）+ Image（尾巴三角）
//   4. 把本脚本挂到 SpeechBubble
//   5. 在 Inspector 里绑定 targetNpc / bubbleText / canvasGroup
// ─────────────────────────────────────────────────────────────────────────────

using System.Collections;
using System.Text;
using UnityEngine;
using TMPro;

namespace BlueprintRuntime.Samples.AINpc.OpenWorld
{
    [RequireComponent(typeof(CanvasGroup))]
    public class WorldSpeechBubble : MonoBehaviour
    {
        [Header("数据源（任选其一）")]
        [Tooltip("基础版 NPC（整段回复）")]
        [SerializeField] private AINpcController basicNpc;
        [Tooltip("流式 NPC（token 实时追加）")]
        [SerializeField] private AINpcStreamingController streamNpc;
        [Tooltip("情绪 NPC")]
        [SerializeField] private EmotionalNpcController emotionalNpc;

        [Header("跟随目标")]
        [Tooltip("气泡跟随的骨骼/头顶空点；为空则用 NPC 根节点")]
        [SerializeField] private Transform followTarget;
        [SerializeField] private Vector3   worldOffset = new Vector3(0, 2.0f, 0);

        [Header("Billboard 与相机")]
        [SerializeField] private Camera targetCamera;
        [Tooltip("始终面向相机（常开；如果 NPC 有固定朝向设定可关闭）")]
        [SerializeField] private bool billboard = true;
        [Tooltip("相机距离对缩放的补偿（1=线性缩放，0=不缩放）")]
        [Range(0f, 1f)]
        [SerializeField] private float perspectiveCompensation = 0.5f;
        [SerializeField] private float baseScale = 0.01f;

        [Header("可视范围（单位：米）")]
        [Tooltip("超过此距离完全不显示")]
        [SerializeField] private float maxVisibleDistance = 25f;
        [Tooltip("开始淡出的距离")]
        [SerializeField] private float fadeStartDistance = 18f;

        [Header("自动收起")]
        [Tooltip("NPC 停止说话后经过多少秒自动淡出气泡；≤0 不自动收起")]
        [SerializeField] private float autoHideDelay = 6f;
        [SerializeField] private float fadeDuration  = 0.35f;

        [Header("UI 引用")]
        [SerializeField] private CanvasGroup canvasGroup;
        [SerializeField] private TMP_Text    bubbleText;

        [Header("打字机（桌面流式无效；WebGL/整段模式生效）")]
        [Range(10, 120)]
        [SerializeField] private float typewriterSpeed = 45f;
        [SerializeField] private string cursor = "|";

        // ── 状态 ─────────────────────────────────────────────────────
        private readonly StringBuilder _buf = new StringBuilder(512);
        private Coroutine _typewriterCo;
        private float     _lastTextUpdateTime;
        private bool      _cursorOn;
        private bool      _isStreamingActive;

        void Reset()
        {
            canvasGroup = GetComponent<CanvasGroup>();
        }

        void Awake()
        {
            if (canvasGroup == null) canvasGroup = GetComponent<CanvasGroup>();
            if (followTarget == null)
            {
                // 尝试从数据源上找 Transform
                if      (basicNpc     != null) followTarget = basicNpc.transform;
                else if (streamNpc    != null) followTarget = streamNpc.transform;
                else if (emotionalNpc != null) followTarget = emotionalNpc.transform;
                else                           followTarget = transform.parent != null ? transform.parent : transform;
            }
            if (targetCamera == null) targetCamera = Camera.main;

            canvasGroup.alpha = 0f;
            if (bubbleText != null) bubbleText.text = "";
        }

        void OnEnable()
        {
            if (basicNpc != null)
            {
                basicNpc.OnThinking += OnThinking;
                basicNpc.OnReply    += OnFullReply;
                basicNpc.OnError    += OnErrorMsg;
            }
            if (streamNpc != null)
            {
                streamNpc.OnThinking   += OnThinking;
                streamNpc.OnReplyStart += OnReplyStart;
                streamNpc.OnChunk      += OnChunk;
                streamNpc.OnReplyDone  += OnStreamDone;
                streamNpc.OnError      += OnErrorMsg;
            }
            if (emotionalNpc != null)
            {
                emotionalNpc.OnThinking += OnThinking;
                emotionalNpc.OnReply    += OnFullReply;
                emotionalNpc.OnError    += OnErrorMsg;
            }
        }

        void OnDisable()
        {
            if (basicNpc != null)
            {
                basicNpc.OnThinking -= OnThinking;
                basicNpc.OnReply    -= OnFullReply;
                basicNpc.OnError    -= OnErrorMsg;
            }
            if (streamNpc != null)
            {
                streamNpc.OnThinking   -= OnThinking;
                streamNpc.OnReplyStart -= OnReplyStart;
                streamNpc.OnChunk      -= OnChunk;
                streamNpc.OnReplyDone  -= OnStreamDone;
                streamNpc.OnError      -= OnErrorMsg;
            }
            if (emotionalNpc != null)
            {
                emotionalNpc.OnThinking -= OnThinking;
                emotionalNpc.OnReply    -= OnFullReply;
                emotionalNpc.OnError    -= OnErrorMsg;
            }
        }

        // ── 每帧位置/朝向/缩放/可见性 ─────────────────────────────────
        void LateUpdate()
        {
            if (followTarget == null || targetCamera == null) return;

            // 1. 跟随位置
            transform.position = followTarget.position + worldOffset;

            // 2. Billboard
            if (billboard)
            {
                Vector3 dirToCam = targetCamera.transform.position - transform.position;
                dirToCam.y = 0f;
                if (dirToCam.sqrMagnitude > 0.0001f)
                    transform.rotation = Quaternion.LookRotation(-dirToCam);
            }

            // 3. 距离缩放 + 淡出
            float distance = Vector3.Distance(targetCamera.transform.position, transform.position);

            // 距离淡出（只影响"已有内容的气泡"）
            if (_buf.Length > 0 || _isStreamingActive)
            {
                float distAlpha = 1f;
                if (distance >= maxVisibleDistance) distAlpha = 0f;
                else if (distance > fadeStartDistance)
                    distAlpha = 1f - (distance - fadeStartDistance) / (maxVisibleDistance - fadeStartDistance);

                // 气泡内部的 alpha 是"文本 alpha" 和 "距离 alpha" 的乘积
                // 自动收起协程在操作 canvasGroup.alpha，这里取 min 避免相互覆盖
                canvasGroup.alpha = Mathf.Min(canvasGroup.alpha, distAlpha);
            }

            // 距离透视补偿缩放
            float scale = baseScale * (1f + distance * perspectiveCompensation * 0.05f);
            transform.localScale = Vector3.one * scale;

            // 光标闪烁
            if (_isStreamingActive && !string.IsNullOrEmpty(cursor) && bubbleText != null)
            {
                bool on = ((int)(Time.time * 2)) % 2 == 0;
                if (on != _cursorOn)
                {
                    _cursorOn = on;
                    RenderBuffer();
                }
            }
        }

        // ── 事件处理 ─────────────────────────────────────────────────
        private void OnThinking()
        {
            StopTypewriter();
            _buf.Clear();
            _isStreamingActive = true;
            if (bubbleText != null) bubbleText.text = "<color=#BBBBBB><i>...</i></color>";
            FadeTo(1f);
        }

        private void OnReplyStart()
        {
            _buf.Clear();
            _lastTextUpdateTime = Time.time;
            RenderBuffer();
        }

        private void OnChunk(string token)
        {
            // WebGL 降级：一个 chunk 就是整段文本，改走打字机
#if UNITY_WEBGL && !UNITY_EDITOR
            if (token.Length > 20 && typewriterSpeed > 0)
            {
                StartTypewriter(token);
                return;
            }
#endif
            _buf.Append(token);
            _lastTextUpdateTime = Time.time;
            RenderBuffer();
        }

        private void OnStreamDone(string fullText)
        {
            // 打字机还在跑：让它跑完再收尾
            if (_typewriterCo != null) return;
            FinalizeReply(fullText);
        }

        private void OnFullReply(string fullText)
        {
            // 基础/情绪 NPC 一次性收到整段
            _isStreamingActive = false;
            FadeTo(1f);
            StartTypewriter(fullText);
        }

        private void OnErrorMsg(string err)
        {
            StopTypewriter();
            _isStreamingActive = false;
            _buf.Clear();
            _buf.Append($"<color=#FF6666>[出错: {err}]</color>");
            RenderBuffer();
            FadeTo(1f);
            ScheduleAutoHide();
        }

        // ── 内部：打字机 ─────────────────────────────────────────────
        private void StartTypewriter(string text)
        {
            StopTypewriter();
            _typewriterCo = StartCoroutine(TypewriterCo(text));
        }

        private void StopTypewriter()
        {
            if (_typewriterCo != null)
            {
                StopCoroutine(_typewriterCo);
                _typewriterCo = null;
            }
        }

        private IEnumerator TypewriterCo(string text)
        {
            _buf.Clear();
            float interval = 1f / Mathf.Max(1f, typewriterSpeed);
            for (int i = 0; i < text.Length; i++)
            {
                _buf.Append(text[i]);
                _lastTextUpdateTime = Time.time;
                RenderBuffer();
                yield return new WaitForSeconds(interval);
            }
            _typewriterCo = null;
            FinalizeReply(_buf.ToString());
        }

        private void FinalizeReply(string fullText)
        {
            _isStreamingActive = false;
            _cursorOn = false;
            if (_buf.Length == 0) { _buf.Append(fullText); }
            RenderBuffer();
            ScheduleAutoHide();
        }

        private void RenderBuffer()
        {
            if (bubbleText == null) return;
            if (_isStreamingActive && _cursorOn && !string.IsNullOrEmpty(cursor))
                bubbleText.text = _buf.ToString() + cursor;
            else
                bubbleText.text = _buf.ToString();
        }

        // ── 内部：淡入/淡出 ───────────────────────────────────────────
        private Coroutine _fadeCo;
        private void FadeTo(float targetAlpha)
        {
            if (_fadeCo != null) StopCoroutine(_fadeCo);
            _fadeCo = StartCoroutine(FadeCo(targetAlpha));
        }

        private IEnumerator FadeCo(float targetAlpha)
        {
            float start = canvasGroup.alpha;
            float elapsed = 0f;
            while (elapsed < fadeDuration)
            {
                elapsed += Time.deltaTime;
                canvasGroup.alpha = Mathf.Lerp(start, targetAlpha, elapsed / fadeDuration);
                yield return null;
            }
            canvasGroup.alpha = targetAlpha;
            if (Mathf.Approximately(targetAlpha, 0f))
            {
                _buf.Clear();
                if (bubbleText != null) bubbleText.text = "";
            }
            _fadeCo = null;
        }

        // ── 内部：自动收起 ─────────────────────────────────────────────
        private Coroutine _autoHideCo;
        private void ScheduleAutoHide()
        {
            if (autoHideDelay <= 0f) return;
            if (_autoHideCo != null) StopCoroutine(_autoHideCo);
            _autoHideCo = StartCoroutine(AutoHideCo());
        }

        private IEnumerator AutoHideCo()
        {
            float deadline = _lastTextUpdateTime + autoHideDelay;
            while (Time.time < deadline)
            {
                // 如果期间 NPC 又说话了，重置倒计时
                if (_lastTextUpdateTime + autoHideDelay > deadline)
                    deadline = _lastTextUpdateTime + autoHideDelay;
                yield return null;
            }
            FadeTo(0f);
            _autoHideCo = null;
        }

        // ── 公开 API（场景代码可主动触发） ─────────────────────────────
        /// <summary>强制隐藏气泡（例如玩家离开）</summary>
        public void HideImmediate()
        {
            StopTypewriter();
            _buf.Clear();
            _isStreamingActive = false;
            if (bubbleText != null) bubbleText.text = "";
            canvasGroup.alpha = 0f;
        }
    }
}
