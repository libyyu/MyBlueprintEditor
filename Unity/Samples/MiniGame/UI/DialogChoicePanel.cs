// ─────────────────────────────────────────────────────────────────────
// DialogChoicePanel.cs — 对话选项面板
//
// NPC 回复时，LLM 可以同时返回 2~4 个后续选项。
// 本组件渲染选项按钮，玩家点击后自动作为下一轮输入。
//
// 用法：
//   1. 蓝图里 LLM 回复格式约定：
//      回复正文
//      ---OPTIONS---
//      选项1文本
//      选项2文本
//      选项3文本
//   2. C# 侧解析 "---OPTIONS---" 分隔符，提取选项
//   3. 玩家点击选项 → 自动调 Controller.Say(选项文本)
//
// 如果 LLM 没返回选项，面板隐藏，回退到自由输入。
// ─────────────────────────────────────────────────────────────────────

using System;
using System.Collections.Generic;
using UnityEngine;
using UnityEngine.UI;
using TMPro;
using BlueprintRuntime.Samples.AINpc;

namespace BlueprintRuntime.Samples.MiniGame
{
    public class DialogChoicePanel : MonoBehaviour
    {
        [Header("UI 引用")]
        [SerializeField] private GameObject   panel;
        [SerializeField] private RectTransform buttonParent;
        [SerializeField] private GameObject   buttonPrefab;     // 含 TMP_Text
        [SerializeField] private TMP_InputField freeInputField; // 自由输入框（选项和自由输入可并存）
        [SerializeField] private Button       freeInputSend;

        [Header("解析")]
        [SerializeField] private string optionSeparator = "---OPTIONS---";
        [SerializeField] private int    maxOptions = 4;

        [Header("自动连接")]
        [Tooltip("留空则需手动调 SetTarget")]
        [SerializeField] private MonoBehaviour targetController;

        // ── 事件 ────────────────────────────────────────────────────
        /// <summary>玩家选择了一个选项</summary>
        public event Action<string> OnOptionSelected;
        /// <summary>玩家输入了自由文本</summary>
        public event Action<string> OnFreeInput;

        private readonly List<GameObject> _buttons = new();
        private MonoBehaviour _target;

        void Start()
        {
            if (panel != null) panel.SetActive(false);
            _target = targetController;

            if (freeInputSend != null)
                freeInputSend.onClick.AddListener(OnSendFreeInput);
            if (freeInputField != null)
                freeInputField.onSubmit.AddListener(_ => OnSendFreeInput());

            // 自动订阅 Controller 的 OnReply
            AutoSubscribe();
        }

        // ── 公共 API ────────────────────────────────────────────────

        /// <summary>设置目标 NPC Controller</summary>
        public void SetTarget(MonoBehaviour controller)
        {
            _target = controller;
        }

        /// <summary>解析 LLM 回复，提取选项并显示。返回去掉选项后的纯回复文本。</summary>
        public string ParseAndShow(string fullReply)
        {
            if (string.IsNullOrEmpty(fullReply))
            {
                Hide();
                return fullReply;
            }

            int sepIdx = fullReply.IndexOf(optionSeparator, StringComparison.OrdinalIgnoreCase);
            if (sepIdx < 0)
            {
                Hide();
                return fullReply;
            }

            string replyText = fullReply.Substring(0, sepIdx).TrimEnd();
            string optionBlock = fullReply.Substring(sepIdx + optionSeparator.Length).Trim();

            var options = new List<string>();
            foreach (var line in optionBlock.Split('\n'))
            {
                var trimmed = line.Trim().TrimStart('-', '·', '•', '*', ' ');
                if (trimmed.Length > 0 && options.Count < maxOptions)
                    options.Add(trimmed);
            }

            if (options.Count > 0)
                ShowOptions(options);
            else
                Hide();

            return replyText;
        }

        public void ShowOptions(List<string> options)
        {
            ClearButtons();

            if (panel != null) panel.SetActive(true);

            for (int i = 0; i < options.Count && i < maxOptions; i++)
            {
                string opt = options[i];
                var go = Instantiate(buttonPrefab, buttonParent);
                go.SetActive(true);

                var text = go.GetComponentInChildren<TMP_Text>();
                if (text != null) text.text = opt;

                var btn = go.GetComponent<Button>();
                if (btn != null)
                    btn.onClick.AddListener(() => SelectOption(opt));

                _buttons.Add(go);
            }
        }

        public void Hide()
        {
            ClearButtons();
            if (panel != null) panel.SetActive(false);
        }

        // ── 内部 ────────────────────────────────────────────────────

        private void SelectOption(string option)
        {
            Hide();
            OnOptionSelected?.Invoke(option);
            SayToTarget(option);
        }

        private void OnSendFreeInput()
        {
            if (freeInputField == null) return;
            string text = freeInputField.text.Trim();
            if (string.IsNullOrEmpty(text)) return;
            freeInputField.text = "";
            Hide();
            OnFreeInput?.Invoke(text);
            SayToTarget(text);
        }

        private void SayToTarget(string text)
        {
            if (_target == null) return;

            // 内容安全过滤
            if (ContentFilter.Instance != null)
                text = ContentFilter.Instance.FilterPlayerInput(text);

            // 限流检查
            if (RateLimiter.Instance != null && !RateLimiter.Instance.TryConsume())
            {
                Debug.Log("[DialogChoice] Rate limited");
                return;
            }

            // 通知 NpcMemory
            var mem = _target.GetComponent<NpcMemory>();
            mem?.NotifyPlayerSaid(text);

            // 调 Say
            var basic = _target as AINpcController;
            var stream = _target as AINpcStreamingController;
            if (basic != null) basic.Say(text);
            else if (stream != null) stream.Say(text);
        }

        private void AutoSubscribe()
        {
            if (_target == null) return;

            var basic = _target as AINpcController;
            var stream = _target as AINpcStreamingController;

            if (basic != null)
                basic.OnReply += reply => ParseAndShow(reply);
            if (stream != null)
                stream.OnReplyDone += reply => ParseAndShow(reply);
        }

        private void ClearButtons()
        {
            foreach (var go in _buttons)
                if (go != null) Destroy(go);
            _buttons.Clear();
        }
    }
}
