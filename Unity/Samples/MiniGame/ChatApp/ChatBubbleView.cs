// ─────────────────────────────────────────────────────────────────────
// ChatBubbleView.cs — 单个聊天气泡（微信风格）
//
// 由 ChatAppController 动态 Instantiate。
// 左侧 = NPC 消息（灰底），右侧 = 玩家消息（绿底）。
// ─────────────────────────────────────────────────────────────────────

using UnityEngine;
using UnityEngine.UI;
using TMPro;

namespace BlueprintRuntime.Samples.MiniGame.ChatApp
{
    public class ChatBubbleView : MonoBehaviour
    {
        [Header("布局")]
        [SerializeField] private HorizontalLayoutGroup layoutGroup;
        [SerializeField] private RectTransform         avatarSlot;     // 头像位置
        [SerializeField] private Image                 avatarImage;
        [SerializeField] private Image                 bubbleBg;
        [SerializeField] private TMP_Text              messageText;
        [SerializeField] private TMP_Text              nameText;       // 可选：NPC 名
        [SerializeField] private TMP_Text              timeText;       // 可选：时间戳

        [Header("颜色")]
        [SerializeField] private Color playerBubbleColor = new Color(0.58f, 0.90f, 0.40f);
        [SerializeField] private Color npcBubbleColor    = new Color(0.95f, 0.95f, 0.95f);
        [SerializeField] private Color playerTextColor   = Color.white;
        [SerializeField] private Color npcTextColor      = new Color(0.1f, 0.1f, 0.1f);

        /// <summary>配置气泡内容和方向</summary>
        public void Setup(string message, string senderName, bool isPlayer, Sprite avatar = null, string time = "")
        {
            if (messageText != null)
            {
                messageText.text = message;
                messageText.color = isPlayer ? playerTextColor : npcTextColor;
            }

            if (bubbleBg != null)
                bubbleBg.color = isPlayer ? playerBubbleColor : npcBubbleColor;

            if (nameText != null)
            {
                nameText.text = senderName;
                nameText.gameObject.SetActive(!isPlayer);  // 玩家不显示名字
            }

            if (timeText != null)
                timeText.text = time;

            if (avatarImage != null && avatar != null)
                avatarImage.sprite = avatar;

            // 布局：玩家右对齐，NPC 左对齐
            if (layoutGroup != null)
            {
                layoutGroup.childAlignment = isPlayer
                    ? TextAnchor.UpperRight
                    : TextAnchor.UpperLeft;
                layoutGroup.reverseArrangement = isPlayer;
            }

            // 如果气泡 bg 有 RectTransform pivot，翻转
            if (bubbleBg != null)
            {
                var rt = bubbleBg.GetComponent<RectTransform>();
                if (rt != null)
                    rt.pivot = isPlayer ? new Vector2(1f, 1f) : new Vector2(0f, 1f);
            }
        }

        /// <summary>追加文字（用于流式模式）</summary>
        public void AppendText(string chunk)
        {
            if (messageText != null)
                messageText.text += chunk;
        }

        /// <summary>更新已有文字</summary>
        public void SetText(string text)
        {
            if (messageText != null)
                messageText.text = text;
        }
    }
}
