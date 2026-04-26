// ─────────────────────────────────────────────────────────────────────
// ContactListUI.cs — NPC 联系人列表（微信通讯录风格）
//
// 显示所有已解锁的 NPC，点击进入聊天界面。
// 每个联系人显示：头像 / 名字 / 最后一句对话 / 未读标记。
// ─────────────────────────────────────────────────────────────────────

using System;
using System.Collections.Generic;
using UnityEngine;
using UnityEngine.UI;
using TMPro;

namespace BlueprintRuntime.Samples.MiniGame.ChatApp
{
    public class ContactListUI : MonoBehaviour
    {
        [Header("UI 引用")]
        [SerializeField] private RectTransform  listParent;
        [SerializeField] private GameObject     contactPrefab;   // 含 Avatar/Name/LastMsg/Badge
        [SerializeField] private GameObject     chatPanel;       // ChatAppController 所在面板
        [SerializeField] private GameObject     contactPanel;    // 自身面板

        [Header("NPC 配置")]
        [SerializeField] private NpcContactInfo[] contacts;

        // ── 事件 ────────────────────────────────────────────────────
        /// <summary>选中某个 NPC 时触发（参数：npcId）</summary>
        public event Action<string> OnContactSelected;

        private readonly Dictionary<string, ContactItemView> _views = new();

        [Serializable]
        public class NpcContactInfo
        {
            public string npcId;
            public string displayName;
            public Sprite avatar;
            [Tooltip("需要解锁条件？(空 = 默认解锁)")]
            public string unlockCondition;
            [HideInInspector] public bool unlocked = true;
            [HideInInspector] public string lastMessage = "";
            [HideInInspector] public int unreadCount;
        }

        private class ContactItemView
        {
            public GameObject  go;
            public Image       avatar;
            public TMP_Text    nameText;
            public TMP_Text    lastMsgText;
            public GameObject  badge;
            public TMP_Text    badgeText;
            public Button      button;
        }

        void Start()
        {
            BuildList();
        }

        private void BuildList()
        {
            if (contacts == null || contactPrefab == null) return;

            foreach (var info in contacts)
            {
                if (!info.unlocked) continue;

                var go = Instantiate(contactPrefab, listParent);
                var view = new ContactItemView
                {
                    go          = go,
                    avatar      = go.transform.Find("Avatar")?.GetComponent<Image>(),
                    nameText    = go.transform.Find("Name")?.GetComponent<TMP_Text>(),
                    lastMsgText = go.transform.Find("LastMsg")?.GetComponent<TMP_Text>(),
                    badge       = go.transform.Find("Badge")?.gameObject,
                    badgeText   = go.transform.Find("Badge/Count")?.GetComponent<TMP_Text>(),
                    button      = go.GetComponent<Button>() ?? go.AddComponent<Button>(),
                };

                if (view.avatar != null && info.avatar != null)
                    view.avatar.sprite = info.avatar;

                if (view.nameText != null)
                    view.nameText.text = info.displayName;

                if (view.lastMsgText != null)
                    view.lastMsgText.text = info.lastMessage;

                if (view.badge != null)
                    view.badge.SetActive(info.unreadCount > 0);

                string npcId = info.npcId;  // capture for closure
                view.button.onClick.AddListener(() => SelectContact(npcId));

                _views[info.npcId] = view;
            }
        }

        private void SelectContact(string npcId)
        {
            // 清除未读
            var info = GetInfo(npcId);
            if (info != null)
            {
                info.unreadCount = 0;
                if (_views.TryGetValue(npcId, out var v) && v.badge != null)
                    v.badge.SetActive(false);
            }

            // 切换面板
            if (contactPanel != null) contactPanel.SetActive(false);
            if (chatPanel    != null) chatPanel.SetActive(true);

            OnContactSelected?.Invoke(npcId);
        }

        /// <summary>从聊天面板返回联系人列表</summary>
        public void BackToContactList()
        {
            if (chatPanel    != null) chatPanel.SetActive(false);
            if (contactPanel != null) contactPanel.SetActive(true);
        }

        /// <summary>更新某 NPC 的最后消息（ChatAppController 收到回复时调用）</summary>
        public void UpdateLastMessage(string npcId, string message)
        {
            var info = GetInfo(npcId);
            if (info == null) return;
            info.lastMessage = message.Length > 20 ? message.Substring(0, 20) + "..." : message;

            if (_views.TryGetValue(npcId, out var v) && v.lastMsgText != null)
                v.lastMsgText.text = info.lastMessage;
        }

        /// <summary>增加未读消息数</summary>
        public void IncrementUnread(string npcId)
        {
            var info = GetInfo(npcId);
            if (info == null) return;
            info.unreadCount++;

            if (_views.TryGetValue(npcId, out var v))
            {
                if (v.badge != null) v.badge.SetActive(true);
                if (v.badgeText != null) v.badgeText.text = info.unreadCount.ToString();
            }
        }

        /// <summary>解锁一个 NPC（例如分享奖励解锁）</summary>
        public void UnlockContact(string npcId)
        {
            var info = GetInfo(npcId);
            if (info == null) return;
            info.unlocked = true;
            // 重建列表
            foreach (Transform child in listParent) Destroy(child.gameObject);
            _views.Clear();
            BuildList();
        }

        private NpcContactInfo GetInfo(string npcId)
        {
            if (contacts == null) return null;
            foreach (var c in contacts)
                if (c.npcId == npcId) return c;
            return null;
        }
    }
}
