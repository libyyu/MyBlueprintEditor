// ChatAppController.cs
// ─────────────────────────────────────────────────────────────────────────────
// 2D 聊天 App 风格小游戏 — 像微信/QQ 界面
//
// 特点：
//   - 无 3D 世界，全 UGUI
//   - 左侧联系人列表（每个 NPC 一项，显示最后一条消息预览、未读红点、好感度 ❤）
//   - 右侧聊天窗（玩家/NPC 气泡流）
//   - 顶部标题栏（当前 NPC 名字 + 好感度图标）
//   - 底部输入框 + 发送按钮
//
// 适合的游戏类型：
//   - AI 恋爱模拟
//   - 剧本杀（玩家同时和多个"嫌疑人"聊天收集线索）
//   - 赛博朋克黑客（AI 扮演 NPC，玩家假扮其他人套取信息）
//   - 心理咨询/倾诉游戏
//
// 用法：
//   - 一个 Canvas 挂本脚本，连好几个 UI 引用
//   - 场景里放 N 个 NPC GameObject（挂 Controller + NpcMemory），不需要 3D 模型
//   - 玩家选择联系人 → 进入聊天 → 发消息 → NPC 回复
// ─────────────────────────────────────────────────────────────────────────────

using System.Collections.Generic;
using UnityEngine;
using UnityEngine.UI;
using TMPro;
using BlueprintRuntime.Samples.AINpc;

namespace BlueprintRuntime.Samples.MiniGame.ChatApp
{
    public class ChatAppController : MonoBehaviour
    {
        [Header("联系人列表")]
        [Tooltip("场景中所有 NPC 的 Controller（按显示顺序）。留空则自动从 FindObjectsOfType 获取")]
        [SerializeField] private List<MonoBehaviour> npcControllers = new List<MonoBehaviour>();

        [SerializeField] private Transform     contactListContent;   // ScrollView/Viewport/Content
        [SerializeField] private GameObject    contactItemPrefab;     // 含 Button + TMP_Text（名字/预览/❤）

        [Header("聊天窗")]
        [SerializeField] private GameObject    chatPanel;
        [SerializeField] private TMP_Text      chatHeaderText;        // 顶部 NPC 名 + 好感度
        [SerializeField] private ScrollRect    chatScroll;
        [SerializeField] private Transform     chatBubbleContent;     // VerticalLayoutGroup
        [SerializeField] private GameObject    playerBubblePrefab;
        [SerializeField] private GameObject    npcBubblePrefab;
        [SerializeField] private GameObject    thinkingBubblePrefab;  // "..." 气泡
        [SerializeField] private TMP_InputField inputField;
        [SerializeField] private Button        sendButton;
        [SerializeField] private Button        backButton;             // 移动端"返回联系人列表"

        // ── 运行时 ──────────────────────────────────────────────
        private readonly Dictionary<MonoBehaviour, ContactItem> _items = new Dictionary<MonoBehaviour, ContactItem>();
        private MonoBehaviour _currentNpc;
        private GameObject    _currentThinkingBubble;
        private TMP_Text      _currentStreamingText;

        // 每个 NPC 的未读计数（切离该 NPC 后他说话就记未读）
        private readonly Dictionary<MonoBehaviour, int> _unreadCount = new Dictionary<MonoBehaviour, int>();

        // 每个 NPC 最后一条消息（用于联系人列表预览）
        private readonly Dictionary<MonoBehaviour, string> _lastMessage = new Dictionary<MonoBehaviour, string>();

        private class ContactItem
        {
            public GameObject root;
            public TMP_Text   nameText;
            public TMP_Text   previewText;
            public TMP_Text   affinityText;
            public GameObject unreadBadge;
        }

        void Start()
        {
            if (npcControllers.Count == 0) AutoCollectNpcs();
            BuildContactList();
            if (chatPanel != null) chatPanel.SetActive(false);
            sendButton?.onClick.AddListener(OnSendClicked);
            inputField?.onSubmit.AddListener(_ => OnSendClicked());
            backButton?.onClick.AddListener(BackToContactList);
        }

        void AutoCollectNpcs()
        {
            foreach (var c in FindObjectsOfType<AINpcStreamingController>())          npcControllers.Add(c);
            foreach (var c in FindObjectsOfType<AINpcController>())                   if (!npcControllers.Contains(c)) npcControllers.Add(c);
            foreach (var c in FindObjectsOfType<EmotionalNpcController>())            if (!npcControllers.Contains(c)) npcControllers.Add(c);
        }

        void BuildContactList()
        {
            if (contactListContent == null || contactItemPrefab == null) return;
            foreach (Transform c in contactListContent) Destroy(c.gameObject);
            _items.Clear();

            foreach (var npc in npcControllers)
            {
                if (npc == null) continue;
                var go = Instantiate(contactItemPrefab, contactListContent);
                var item = new ContactItem {
                    root         = go,
                    nameText     = FindChild<TMP_Text>(go, "Name"),
                    previewText  = FindChild<TMP_Text>(go, "Preview"),
                    affinityText = FindChild<TMP_Text>(go, "Affinity"),
                    unreadBadge  = FindChildObj(go, "Unread"),
                };
                _items[npc] = item;

                SubscribeNpcEvents(npc);
                RefreshContact(npc);

                var btn = go.GetComponentInChildren<Button>();
                if (btn != null)
                {
                    var captured = npc;
                    btn.onClick.AddListener(() => OpenChatWith(captured));
                }
            }
        }

        // ── 订阅 NPC 事件（更新联系人列表预览、未读红点、流式气泡）──────
        void SubscribeNpcEvents(MonoBehaviour npc)
        {
            if (npc is AINpcStreamingController s)
            {
                s.OnThinking   += () => OnNpcThinking(s);
                s.OnReplyStart += () => OnNpcReplyStart(s);
                s.OnChunk      += token => OnNpcChunk(s, token);
                s.OnReplyDone  += full  => OnNpcReplyDone(s, full);
                s.OnError      += err   => OnNpcError(s, err);
            }
            else if (npc is AINpcController b)
            {
                b.OnThinking   += () => OnNpcThinking(b);
                b.OnReply      += msg  => OnNpcFullReply(b, msg);
                b.OnError      += err  => OnNpcError(b, err);
            }
            else if (npc is EmotionalNpcController e)
            {
                e.OnThinking   += () => OnNpcThinking(e);
                e.OnReply      += msg  => OnNpcFullReply(e, msg);
                e.OnError      += err  => OnNpcError(e, err);
            }
        }

        // ── 联系人操作 ────────────────────────────────────────────
        public void OpenChatWith(MonoBehaviour npc)
        {
            _currentNpc = npc;
            _unreadCount[npc] = 0;
            RefreshContact(npc);

            if (chatPanel != null) chatPanel.SetActive(true);
            if (chatBubbleContent != null)
                foreach (Transform c in chatBubbleContent) Destroy(c.gameObject);

            if (chatHeaderText != null)
                chatHeaderText.text = $"{GetNpcName(npc)}  {AffinityHeartsFor(npc)}";

            // 加载历史消息
            var mem = (npc as Component)?.GetComponent<NpcMemory>();
            if (mem != null)
            {
                foreach (var turn in mem.Data.history)
                {
                    var prefab = turn.role == "user" ? playerBubblePrefab : npcBubblePrefab;
                    SpawnBubble(prefab, turn.content);
                }
            }

            ScrollToBottom();
            inputField?.ActivateInputField();
        }

        void BackToContactList()
        {
            _currentNpc = null;
            if (chatPanel != null) chatPanel.SetActive(false);
        }

        void RefreshContact(MonoBehaviour npc)
        {
            if (!_items.TryGetValue(npc, out var item)) return;
            if (item.nameText  != null) item.nameText.text  = GetNpcName(npc);
            if (item.previewText != null) item.previewText.text = _lastMessage.TryGetValue(npc, out var m) ? m : "（点击开始聊天）";
            if (item.affinityText != null) item.affinityText.text = AffinityHeartsFor(npc);
            if (item.unreadBadge != null)
            {
                int u = _unreadCount.TryGetValue(npc, out var n) ? n : 0;
                item.unreadBadge.SetActive(u > 0);
                var t = item.unreadBadge.GetComponentInChildren<TMP_Text>();
                if (t != null) t.text = u > 99 ? "99+" : u.ToString();
            }
        }

        // ── 发送消息 ──────────────────────────────────────────────
        void OnSendClicked()
        {
            if (_currentNpc == null) return;
            var text = inputField?.text?.Trim();
            if (string.IsNullOrEmpty(text)) return;

            bool ok = TrySay(_currentNpc, text);
            if (!ok) return;

            SpawnBubble(playerBubblePrefab, text);
            _lastMessage[_currentNpc] = "我：" + text;
            RefreshContact(_currentNpc);
            inputField.text = "";
            inputField.ActivateInputField();
            ScrollToBottom();

            // 同步到 NpcMemory（让它"记得"玩家说了什么）
            var mem = (_currentNpc as Component)?.GetComponent<NpcMemory>();
            mem?.NotifyPlayerSaid(text);
        }

        // ── NPC 事件响应 ─────────────────────────────────────────
        void OnNpcThinking(MonoBehaviour npc)
        {
            if (npc != _currentNpc)
            {
                _unreadCount.TryGetValue(npc, out var u);
                _unreadCount[npc] = u + 1;
                RefreshContact(npc);
                return;
            }
            _currentThinkingBubble = SpawnBubble(thinkingBubblePrefab != null ? thinkingBubblePrefab : npcBubblePrefab, "…");
            ScrollToBottom();
        }

        void OnNpcReplyStart(MonoBehaviour npc)
        {
            if (npc != _currentNpc) return;
            // 把思考气泡变成 NPC 流式气泡
            if (_currentThinkingBubble != null)
            {
                _currentStreamingText = _currentThinkingBubble.GetComponentInChildren<TMP_Text>();
                if (_currentStreamingText != null) _currentStreamingText.text = "";
                _currentThinkingBubble = null;
            }
            else
            {
                var go = SpawnBubble(npcBubblePrefab, "");
                _currentStreamingText = go?.GetComponentInChildren<TMP_Text>();
            }
        }

        void OnNpcChunk(MonoBehaviour npc, string token)
        {
            if (npc != _currentNpc)
            {
                // 离开状态时不更新 UI，只记未读/预览
                var prev = _lastMessage.TryGetValue(npc, out var l) ? l : "";
                _lastMessage[npc] = (prev.StartsWith(GetNpcName(npc) + "：") ? prev : $"{GetNpcName(npc)}：") + token;
                return;
            }
            if (_currentStreamingText != null)
            {
                _currentStreamingText.text += token;
                ScrollToBottom();
            }
        }

        void OnNpcReplyDone(MonoBehaviour npc, string fullText)
        {
            _lastMessage[npc] = $"{GetNpcName(npc)}：{Truncate(fullText, 20)}";
            if (npc != _currentNpc)
            {
                _unreadCount.TryGetValue(npc, out var u);
                _unreadCount[npc] = u + 1;
            }
            else if (_currentStreamingText != null)
            {
                _currentStreamingText.text = fullText;   // DONE 为权威
            }
            RefreshContact(npc);
            _currentStreamingText = null;
            ScrollToBottom();
        }

        void OnNpcFullReply(MonoBehaviour npc, string msg)
        {
            _lastMessage[npc] = $"{GetNpcName(npc)}：{Truncate(msg, 20)}";
            if (npc == _currentNpc)
            {
                // 基础 Controller 没有流式开始事件，气泡直接替换 thinking
                if (_currentThinkingBubble != null)
                {
                    var t = _currentThinkingBubble.GetComponentInChildren<TMP_Text>();
                    if (t != null) t.text = msg;
                    _currentThinkingBubble = null;
                }
                else
                {
                    SpawnBubble(npcBubblePrefab, msg);
                }
            }
            else
            {
                _unreadCount.TryGetValue(npc, out var u);
                _unreadCount[npc] = u + 1;
            }
            RefreshContact(npc);
            ScrollToBottom();
        }

        void OnNpcError(MonoBehaviour npc, string err)
        {
            if (npc != _currentNpc) return;
            if (_currentThinkingBubble != null)
            {
                var t = _currentThinkingBubble.GetComponentInChildren<TMP_Text>();
                if (t != null) t.text = $"<color=#FF6666>[{err}]</color>";
                _currentThinkingBubble = null;
            }
            else
            {
                SpawnBubble(npcBubblePrefab, $"<color=#FF6666>[{err}]</color>");
            }
            ScrollToBottom();
        }

        // ── 工具 ─────────────────────────────────────────────────
        private GameObject SpawnBubble(GameObject prefab, string text)
        {
            if (prefab == null || chatBubbleContent == null) return null;
            var go = Instantiate(prefab, chatBubbleContent);
            var t = go.GetComponentInChildren<TMP_Text>();
            if (t != null) t.text = text;
            return go;
        }

        void ScrollToBottom()
        {
            if (chatScroll == null) return;
            Canvas.ForceUpdateCanvases();
            chatScroll.verticalNormalizedPosition = 0f;
        }

        private static bool TrySay(MonoBehaviour npc, string text)
        {
            switch (npc)
            {
                case AINpcController b:          return b.Say(text);
                case AINpcStreamingController s: return s.Say(text);
                case EmotionalNpcController e:   return e.Say(text);
            }
            return false;
        }

        private static string GetNpcName(MonoBehaviour npc)
        {
            switch (npc)
            {
                case AINpcController b:          return b.NpcName;
                case AINpcStreamingController s: return s.NpcName;
                case EmotionalNpcController e:   return e.NpcName;
            }
            return npc != null ? npc.name : "?";
        }

        private static string AffinityHeartsFor(MonoBehaviour npc)
        {
            var mem = (npc as Component)?.GetComponent<NpcMemory>();
            if (mem == null) return "";
            int level = mem.Data.affinity switch
            {
                >= 96 => 5,
                >= 81 => 4,
                >= 61 => 3,
                >= 41 => 2,
                >= 21 => 1,
                _     => 0,
            };
            var sb = new System.Text.StringBuilder();
            for (int i = 0; i < 5; i++) sb.Append(i < level ? "❤" : "♡");
            return sb.ToString();
        }

        private static string Truncate(string s, int len)
        {
            if (string.IsNullOrEmpty(s)) return "";
            return s.Length <= len ? s : s.Substring(0, len) + "...";
        }

        private static T FindChild<T>(GameObject root, string name) where T : Component
        {
            foreach (var t in root.GetComponentsInChildren<T>(true))
                if (t.name == name) return t;
            // fallback：只有一个同类型时直接返回
            var all = root.GetComponentsInChildren<T>(true);
            return all.Length == 1 ? all[0] : null;
        }

        private static GameObject FindChildObj(GameObject root, string name)
        {
            foreach (var t in root.GetComponentsInChildren<Transform>(true))
                if (t.name == name) return t.gameObject;
            return null;
        }
    }
}
