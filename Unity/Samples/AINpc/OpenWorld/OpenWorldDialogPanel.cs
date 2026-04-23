// OpenWorldDialogPanel.cs
// ─────────────────────────────────────────────────────────────────────────────
// 开放世界风 "锁定对话 Panel" — 玩家按 E 后弹出
//
// 设计原则：
//   - 默认隐藏；由 InteractionPrompt.onInteract 触发显示
//   - 显示时锁定鼠标 / 暂停某些游戏系统
//   - 输入框 + 历史记录（ScrollView）
//   - 按 Esc 或点击关闭按钮退出对话
//   - 支持"切换目标 NPC"：绑到任意 Controller 动态切
//
// 与 WorldSpeechBubble 共存：
//   - 远处看 NPC 头顶气泡（轻度沉浸）
//   - 走近按 E 打开本 Panel（深度对话 + 历史记录）
//   - Panel 打开时自动隐藏 WorldSpeechBubble（避免重复显示）
// ─────────────────────────────────────────────────────────────────────────────

using System.Collections.Generic;
using UnityEngine;
using UnityEngine.UI;
using TMPro;

namespace BlueprintRuntime.Samples.AINpc.OpenWorld
{
    public class OpenWorldDialogPanel : MonoBehaviour
    {
        [Header("UI")]
        [SerializeField] private GameObject     panelRoot;
        [SerializeField] private TMP_Text       npcNameText;
        [SerializeField] private TMP_InputField inputField;
        [SerializeField] private Button         sendButton;
        [SerializeField] private Button         closeButton;

        [Header("历史记录")]
        [SerializeField] private ScrollRect     historyScroll;
        [SerializeField] private Transform      historyContent;    // VerticalLayoutGroup
        [SerializeField] private GameObject     playerBubblePrefab; // 预制体：含 TMP_Text
        [SerializeField] private GameObject     npcBubblePrefab;

        [Header("体验")]
        [SerializeField] private KeyCode closeKey = KeyCode.Escape;
        [Tooltip("打开时切换鼠标可见性")]
        [SerializeField] private bool toggleCursor = true;

        // ── 状态 ─────────────────────────────────────────────────────
        private AINpcController           _basic;
        private AINpcStreamingController  _stream;
        private EmotionalNpcController    _emotional;
        private WorldSpeechBubble         _bubble;   // 当前 NPC 的头顶气泡（打开时临时隐藏）
        private TMP_Text                  _currentNpcBubbleText;  // 最后一个 NPC bubble 的文本（用于流式追加）

        void Awake()
        {
            if (panelRoot != null) panelRoot.SetActive(false);
            if (sendButton  != null) sendButton.onClick.AddListener(OnSend);
            if (closeButton != null) closeButton.onClick.AddListener(Close);
            if (inputField  != null) inputField.onSubmit.AddListener(_ => OnSend());
        }

        void Update()
        {
            if (panelRoot != null && panelRoot.activeSelf && Input.GetKeyDown(closeKey))
                Close();
        }

        /// <summary>由 InteractionPrompt.onInteract 绑定调用</summary>
        public void OpenForTrigger(NpcProximityTrigger trigger)
        {
            if (trigger == null) return;

            _basic     = trigger.GetComponent<AINpcController>();
            _stream    = trigger.GetComponent<AINpcStreamingController>();
            _emotional = trigger.GetComponent<EmotionalNpcController>();
            _bubble    = trigger.GetComponentInChildren<WorldSpeechBubble>();

            string npcName = _basic     != null ? _basic.NpcName
                           : _stream    != null ? _stream.NpcName
                           : _emotional != null ? _emotional.NpcName
                           : trigger.gameObject.name;

            if (npcNameText != null) npcNameText.text = npcName;

            SubscribeEvents();

            if (panelRoot != null) panelRoot.SetActive(true);
            if (inputField != null) inputField.ActivateInputField();

            if (_bubble != null) _bubble.HideImmediate();   // 打开 Panel 时清掉头顶气泡

            if (toggleCursor)
            {
                Cursor.lockState = CursorLockMode.None;
                Cursor.visible   = true;
            }
        }

        public void Close()
        {
            UnsubscribeEvents();
            if (panelRoot != null) panelRoot.SetActive(false);

            if (toggleCursor)
            {
                Cursor.lockState = CursorLockMode.Locked;
                Cursor.visible   = false;
            }

            _basic = null; _stream = null; _emotional = null; _bubble = null;
            _currentNpcBubbleText = null;
        }

        private void OnSend()
        {
            if (inputField == null) return;
            var text = inputField.text.Trim();
            if (string.IsNullOrEmpty(text)) return;

            bool ok = false;
            if      (_basic     != null) ok = _basic.Say(text);
            else if (_stream    != null) ok = _stream.Say(text);
            else if (_emotional != null) ok = _emotional.Say(text);
            if (!ok) return;

            AddPlayerBubble(text);
            inputField.text = "";
            inputField.ActivateInputField();
        }

        // ── 事件订阅：多 Controller 复用 ─────────────────────────────
        private void SubscribeEvents()
        {
            if (_basic != null)
            {
                _basic.OnThinking += OnThinking;
                _basic.OnReply    += OnFullReply;
                _basic.OnError    += OnError;
            }
            if (_stream != null)
            {
                _stream.OnThinking   += OnThinking;
                _stream.OnReplyStart += OnReplyStart;
                _stream.OnChunk      += OnChunk;
                _stream.OnReplyDone  += OnStreamDone;
                _stream.OnError      += OnError;
            }
            if (_emotional != null)
            {
                _emotional.OnThinking += OnThinking;
                _emotional.OnReply    += OnFullReply;
                _emotional.OnError    += OnError;
            }
        }

        private void UnsubscribeEvents()
        {
            if (_basic != null)
            {
                _basic.OnThinking -= OnThinking;
                _basic.OnReply    -= OnFullReply;
                _basic.OnError    -= OnError;
            }
            if (_stream != null)
            {
                _stream.OnThinking   -= OnThinking;
                _stream.OnReplyStart -= OnReplyStart;
                _stream.OnChunk      -= OnChunk;
                _stream.OnReplyDone  -= OnStreamDone;
                _stream.OnError      -= OnError;
            }
            if (_emotional != null)
            {
                _emotional.OnThinking -= OnThinking;
                _emotional.OnReply    -= OnFullReply;
                _emotional.OnError    -= OnError;
            }
        }

        // ── 事件 → 气泡记录 ─────────────────────────────────────────
        private void OnThinking()
        {
            _currentNpcBubbleText = AddNpcBubble("<color=#AAAAAA><i>...</i></color>");
        }

        private void OnReplyStart()
        {
            // 流式开始：清空上一个 thinking 气泡的内容，准备 append
            if (_currentNpcBubbleText != null) _currentNpcBubbleText.text = "";
            else _currentNpcBubbleText = AddNpcBubble("");
        }

        private void OnChunk(string token)
        {
            if (_currentNpcBubbleText == null) _currentNpcBubbleText = AddNpcBubble("");
            _currentNpcBubbleText.text += token;
            ScrollToBottom();
        }

        private void OnStreamDone(string fullText)
        {
            if (_currentNpcBubbleText != null && !string.IsNullOrEmpty(fullText))
                _currentNpcBubbleText.text = fullText;   // 以 DONE 为权威
            _currentNpcBubbleText = null;
            ScrollToBottom();
        }

        private void OnFullReply(string text)
        {
            if (_currentNpcBubbleText != null) _currentNpcBubbleText.text = text;
            else AddNpcBubble(text);
            _currentNpcBubbleText = null;
            ScrollToBottom();
        }

        private void OnError(string err)
        {
            if (_currentNpcBubbleText != null)
                _currentNpcBubbleText.text = $"<color=#FF6666>[出错: {err}]</color>";
            else
                AddNpcBubble($"<color=#FF6666>[出错: {err}]</color>");
            _currentNpcBubbleText = null;
            ScrollToBottom();
        }

        // ── UI 操作 ─────────────────────────────────────────────────
        private void AddPlayerBubble(string text)
        {
            SpawnBubble(playerBubblePrefab, text);
            ScrollToBottom();
        }

        private TMP_Text AddNpcBubble(string text)
        {
            return SpawnBubble(npcBubblePrefab, text);
        }

        private TMP_Text SpawnBubble(GameObject prefab, string text)
        {
            if (prefab == null || historyContent == null) return null;
            var go = Instantiate(prefab, historyContent);
            var tmp = go.GetComponentInChildren<TMP_Text>();
            if (tmp != null) tmp.text = text;
            return tmp;
        }

        private void ScrollToBottom()
        {
            if (historyScroll == null) return;
            Canvas.ForceUpdateCanvases();
            historyScroll.verticalNormalizedPosition = 0f;
        }
    }
}
