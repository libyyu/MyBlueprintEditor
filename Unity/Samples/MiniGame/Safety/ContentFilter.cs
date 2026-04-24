// ─────────────────────────────────────────────────────────────────────
// ContentFilter.cs — 内容安全过滤
//
// 三层防线：
//   1. 本地敏感词库（离线，零延迟）
//   2. LLM 输出后置过滤（替换/截断）
//   3. 可选：云端内容审核 API（微信/阿里/腾讯云）
//
// 用法：
//   ContentFilter.Instance.Filter("一些文本") → 过滤后的文本
//   ContentFilter.Instance.IsClean("一些文本") → bool
// ─────────────────────────────────────────────────────────────────────

using System;
using System.Collections.Generic;
using System.Text.RegularExpressions;
using UnityEngine;

namespace BlueprintMiniGame
{
    public class ContentFilter : MonoBehaviour
    {
        public static ContentFilter Instance { get; private set; }

        [Header("本地敏感词")]
        [Tooltip("每行一个敏感词，支持 TextAsset 拖入")]
        [SerializeField] private TextAsset sensitiveWordsFile;
        [Tooltip("手动添加的敏感词（Inspector 里编辑）")]
        [SerializeField] private string[] extraWords;

        [Header("替换策略")]
        [SerializeField] private string replacement = "***";
        [SerializeField] private bool   blockEntireMessage = false;  // true=整条屏蔽，false=替换词
        [SerializeField] private string blockedMessageText = "（该内容包含敏感信息，已被过滤）";

        [Header("输入过滤（玩家发言）")]
        [SerializeField] private bool   filterPlayerInput = true;
        [SerializeField] private int    maxInputLength    = 200;

        [Header("输出过滤（LLM 回复）")]
        [SerializeField] private bool   filterLlmOutput = true;

        [Header("正则规则")]
        [Tooltip("额外的正则过滤规则（每条一个正则表达式）")]
        [SerializeField] private string[] regexPatterns;

        // ── 事件 ────────────────────────────────────────────────────
        public event Action<string, string> OnContentFiltered;  // original, filtered

        private HashSet<string> _wordSet = new();
        private List<Regex>     _regexes = new();

        void Awake()
        {
            if (Instance != null) { Destroy(gameObject); return; }
            Instance = this;
            BuildWordSet();
            BuildRegexes();
        }

        // ── 公共 API ────────────────────────────────────────────────

        /// <summary>过滤文本，返回安全版本</summary>
        public string Filter(string text)
        {
            if (string.IsNullOrEmpty(text)) return text;

            string original = text;

            // 1. 敏感词替换
            foreach (var word in _wordSet)
            {
                if (word.Length == 0) continue;
                int idx;
                while ((idx = text.IndexOf(word, StringComparison.OrdinalIgnoreCase)) >= 0)
                {
                    if (blockEntireMessage)
                        return NotifyAndReturn(original, blockedMessageText);
                    text = text.Remove(idx, word.Length).Insert(idx, replacement);
                }
            }

            // 2. 正则过滤
            foreach (var regex in _regexes)
            {
                if (regex.IsMatch(text))
                {
                    if (blockEntireMessage)
                        return NotifyAndReturn(original, blockedMessageText);
                    text = regex.Replace(text, replacement);
                }
            }

            if (text != original)
                OnContentFiltered?.Invoke(original, text);

            return text;
        }

        /// <summary>检查文本是否干净</summary>
        public bool IsClean(string text)
        {
            if (string.IsNullOrEmpty(text)) return true;

            foreach (var word in _wordSet)
                if (word.Length > 0 && text.IndexOf(word, StringComparison.OrdinalIgnoreCase) >= 0)
                    return false;

            foreach (var regex in _regexes)
                if (regex.IsMatch(text))
                    return false;

            return true;
        }

        /// <summary>过滤玩家输入（截断 + 过滤）</summary>
        public string FilterPlayerInput(string input)
        {
            if (string.IsNullOrEmpty(input)) return input;
            if (input.Length > maxInputLength)
                input = input.Substring(0, maxInputLength);
            return filterPlayerInput ? Filter(input) : input;
        }

        /// <summary>过滤 LLM 输出</summary>
        public string FilterLlmOutput(string output)
        {
            return filterLlmOutput ? Filter(output) : output;
        }

        /// <summary>运行时动态添加敏感词</summary>
        public void AddWord(string word)
        {
            if (!string.IsNullOrWhiteSpace(word))
                _wordSet.Add(word.Trim().ToLowerInvariant());
        }

        // ── 内部 ────────────────────────────────────────────────────

        private void BuildWordSet()
        {
            _wordSet.Clear();

            // 从 TextAsset 加载
            if (sensitiveWordsFile != null)
            {
                var lines = sensitiveWordsFile.text.Split('\n');
                foreach (var line in lines)
                {
                    var w = line.Trim();
                    if (w.Length > 0 && !w.StartsWith("#"))
                        _wordSet.Add(w.ToLowerInvariant());
                }
            }

            // Inspector 额外词
            if (extraWords != null)
                foreach (var w in extraWords)
                    if (!string.IsNullOrWhiteSpace(w))
                        _wordSet.Add(w.Trim().ToLowerInvariant());

            Debug.Log($"[ContentFilter] Loaded {_wordSet.Count} sensitive words");
        }

        private void BuildRegexes()
        {
            _regexes.Clear();
            if (regexPatterns == null) return;
            foreach (var p in regexPatterns)
            {
                if (string.IsNullOrWhiteSpace(p)) continue;
                try { _regexes.Add(new Regex(p, RegexOptions.IgnoreCase | RegexOptions.Compiled)); }
                catch (Exception e) { Debug.LogWarning($"[ContentFilter] Invalid regex '{p}': {e.Message}"); }
            }
        }

        private string NotifyAndReturn(string original, string result)
        {
            OnContentFiltered?.Invoke(original, result);
            return result;
        }
    }
}
