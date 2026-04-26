// ScreenDialogUI.cs
// ─────────────────────────────────────────────────────────────────────────────
// 2D 屏幕对话 UI — 底部半屏对话面板
// 不用 TMP，直接用 UnityEngine.UI.Text，天然支持中文
//
// 功能：
//   1. 玩家靠近 NPC → 底部弹出 "按 E 对话" 提示
//   2. 按 E → 面板展开，显示 NPC 名字 + 头像颜色 + 对话内容
//   3. 流式追加文字（逐 token 打字机效果）
//   4. 输入框 → 发送 → NPC 回复
//   5. ESC 关闭
// ─────────────────────────────────────────────────────────────────────────────

using System.Text;
using UnityEngine;
using UnityEngine.UI;
using BlueprintRuntime.Samples.AINpc;
using BlueprintRuntime.Samples.AINpc.OpenWorld;

namespace BlueprintRuntime.Samples.MiniGame.Demo
{
    public class ScreenDialogUI : MonoBehaviour
    {
        [Header("UI 引用（自动创建时会填好）")]
        [SerializeField] private GameObject promptPanel;       // "按 E 对话" 提示
        [SerializeField] private Text       promptText;
        [SerializeField] private Button     promptButton;       // 点击提示面板也能开始对话（手机端）
        [SerializeField] private GameObject dialogPanel;       // 对话面板
        [SerializeField] private Text       npcNameText;
        [SerializeField] private Image      npcAvatarImage;
        [SerializeField] private Text       dialogText;        // NPC 说的话
        [SerializeField] private Text       historyText;       // 历史记录
        [SerializeField] private InputField inputField;
        [SerializeField] private Button     sendButton;
        [SerializeField] private Button     closeButton;        // 关闭按钮（右上角 ✕）
        [SerializeField] private ScrollRect historyScroll;

        [Header("设置")]
        [SerializeField] private KeyCode interactKey = KeyCode.E;
        [SerializeField] private int maxHistoryLines = 50;
        [SerializeField] private string thinkingText = "<color=#888>思考中...</color>";

        // ── 状态 ─────────────────────────────────────────────────────
        private NpcProximityTrigger _currentTrigger;
        private AINpcStreamingController _currentNpc;
        private SimplePlayerController _player;
        private readonly StringBuilder _streamBuf = new StringBuilder(512);
        private readonly StringBuilder _historyBuf = new StringBuilder(2048);
        private bool _isDialogOpen;

        void Start()
        {
            _player = FindObjectOfType<SimplePlayerController>();

            if (promptPanel != null) promptPanel.SetActive(false);
            if (dialogPanel != null) dialogPanel.SetActive(false);

            if (sendButton != null)
                sendButton.onClick.AddListener(OnSendClicked);

            if (closeButton != null)
                closeButton.onClick.AddListener(CloseDialog);

            if (promptButton != null)
                promptButton.onClick.AddListener(OpenDialog);

            if (inputField != null)
                inputField.onEndEdit.AddListener(s => { if (Input.GetKeyDown(KeyCode.Return)) OnSendClicked(); });

            // 订阅所有 NPC 的 ProximityTrigger 事件
            foreach (var trigger in FindObjectsOfType<NpcProximityTrigger>())
            {
                var t = trigger;
                trigger.OnPlayerEnterInteract += () => OnNpcNear(t);
                trigger.OnPlayerExitInteract  += () => OnNpcLeave(t);
            }

            // 订阅所有 NPC 的流式事件
            foreach (var ctrl in FindObjectsOfType<AINpcStreamingController>())
            {
                var c = ctrl;
                ctrl.OnThinking   += ()    => OnNpcThinking(c);
                ctrl.OnReplyStart += ()    => OnNpcReplyStart(c);
                ctrl.OnChunk      += (s)   => OnNpcChunk(c, s);
                ctrl.OnReplyDone  += (s)   => OnNpcReplyDone(c, s);
                ctrl.OnError      += (s)   => OnNpcError(c, s);
            }
        }

        void Update()
        {
            // 按 E 开始对话
            if (Input.GetKeyDown(interactKey) && _currentTrigger != null && !_isDialogOpen)
            {
                OpenDialog();
            }

            // ESC 关闭
            if (Input.GetKeyDown(KeyCode.Escape) && _isDialogOpen)
            {
                CloseDialog();
            }
        }

        // ── 接近/离开 NPC ────────────────────────────────────────────
        void OnNpcNear(NpcProximityTrigger trigger)
        {
            _currentTrigger = trigger;
            var ctrl = trigger.GetComponent<AINpcStreamingController>();
            if (ctrl == null) return;

            // 显示提示
            if (promptPanel != null)
            {
                promptPanel.SetActive(true);
                if (promptText != null)
                    promptText.text = $"按 <color=#FFCC00>[E]</color> 与 <b>{ctrl.NpcName}</b> 对话";
            }
        }

        void OnNpcLeave(NpcProximityTrigger trigger)
        {
            if (_currentTrigger == trigger)
            {
                _currentTrigger = null;
                if (promptPanel != null) promptPanel.SetActive(false);
                if (_isDialogOpen) CloseDialog();
            }
        }

        // ── 打开/关闭对话 ────────────────────────────────────────────
        void OpenDialog()
        {
            var ctrl = _currentTrigger?.GetComponent<AINpcStreamingController>();
            if (ctrl == null) return;

            _currentNpc = ctrl;
            _isDialogOpen = true;
            _historyBuf.Clear();

            if (promptPanel != null) promptPanel.SetActive(false);
            if (dialogPanel != null) dialogPanel.SetActive(true);
            if (npcNameText != null) npcNameText.text = ctrl.NpcName;

            // NPC 颜色作为头像
            if (npcAvatarImage != null)
            {
                var renderer = ctrl.GetComponent<Renderer>();
                if (renderer != null)
                    npcAvatarImage.color = renderer.material.color;
            }

            if (dialogText != null) dialogText.text = "";
            if (historyText != null) historyText.text = "";
            if (inputField != null) { inputField.text = ""; inputField.ActivateInputField(); }

            if (_player != null) _player.EnterDialog();
        }

        void CloseDialog()
        {
            _isDialogOpen = false;
            _currentNpc = null;

            if (dialogPanel != null) dialogPanel.SetActive(false);
            if (_player != null) _player.ExitDialog();
        }

        // ── 发送 ────────────────────────────────────────────────────
        void OnSendClicked()
        {
            if (_currentNpc == null || inputField == null) return;
            string text = inputField.text.Trim();
            if (string.IsNullOrEmpty(text)) return;

            inputField.text = "";
            inputField.ActivateInputField();

            // 显示玩家消息
            AppendHistory($"<color=#6CB4EE>你：{text}</color>");

            // 发送给 NPC
            _currentNpc.Say(text);
        }

        // ── NPC 事件回调 ────────────────────────────────────────────
        void OnNpcThinking(AINpcStreamingController ctrl)
        {
            if (ctrl != _currentNpc && !IsGreeting(ctrl)) return;
            if (dialogText != null) dialogText.text = thinkingText;
        }

        void OnNpcReplyStart(AINpcStreamingController ctrl)
        {
            if (ctrl != _currentNpc && !IsGreeting(ctrl)) return;
            _streamBuf.Clear();
            if (dialogText != null) dialogText.text = "";
        }

        void OnNpcChunk(AINpcStreamingController ctrl, string chunk)
        {
            if (ctrl != _currentNpc && !IsGreeting(ctrl)) return;
            _streamBuf.Append(chunk);
            if (dialogText != null)
                dialogText.text = _streamBuf.ToString() + "<color=#888>|</color>";
        }

        void OnNpcReplyDone(AINpcStreamingController ctrl, string fullText)
        {
            // 如果是路过打招呼（不在对话中），显示为浮动提示
            if (!_isDialogOpen || ctrl != _currentNpc)
            {
                ShowGreetBubble(ctrl, fullText);
                return;
            }

            if (dialogText != null)
                dialogText.text = fullText;

            AppendHistory($"<color=#FFD700>{ctrl.NpcName}：</color>{fullText}");
        }

        void OnNpcError(AINpcStreamingController ctrl, string error)
        {
            if (ctrl != _currentNpc) return;
            if (dialogText != null)
                dialogText.text = $"<color=#FF6666>[出错了] {error}</color>";
        }

        // ── 打招呼浮动提示（不在对话时，NPC 主动说话） ────────────────
        void ShowGreetBubble(AINpcStreamingController ctrl, string text)
        {
            // 临时显示在 dialogText（3 秒后清除）
            if (dialogText != null && !_isDialogOpen)
            {
                if (dialogPanel != null) dialogPanel.SetActive(true);
                if (npcNameText != null) npcNameText.text = ctrl.NpcName;
                if (npcAvatarImage != null)
                {
                    var r = ctrl.GetComponent<Renderer>();
                    if (r != null) npcAvatarImage.color = r.material.color;
                }
                dialogText.text = text;

                // 隐藏输入行（不在对话模式）
                if (inputField != null) inputField.transform.parent.gameObject.SetActive(false);

                CancelInvoke(nameof(HideGreet));
                Invoke(nameof(HideGreet), 5f);
            }
        }

        void HideGreet()
        {
            if (!_isDialogOpen)
            {
                if (dialogPanel != null) dialogPanel.SetActive(false);
                if (inputField != null) inputField.transform.parent.gameObject.SetActive(true);
            }
        }

        bool IsGreeting(AINpcStreamingController ctrl)
        {
            // NPC 主动打招呼时，不在对话中但需要接收消息
            return !_isDialogOpen && ctrl.IsStreaming;
        }

        // ── 历史记录 ────────────────────────────────────────────────
        void AppendHistory(string line)
        {
            _historyBuf.AppendLine(line);

            // 限制行数
            string full = _historyBuf.ToString();
            string[] lines = full.Split('\n');
            if (lines.Length > maxHistoryLines)
            {
                _historyBuf.Clear();
                for (int i = lines.Length - maxHistoryLines; i < lines.Length; i++)
                    _historyBuf.AppendLine(lines[i]);
            }

            if (historyText != null)
                historyText.text = _historyBuf.ToString();

            // 滚动到底部
            if (historyScroll != null)
                Canvas.ForceUpdateCanvases();
            if (historyScroll != null)
                historyScroll.verticalNormalizedPosition = 0f;
        }
    }
}
