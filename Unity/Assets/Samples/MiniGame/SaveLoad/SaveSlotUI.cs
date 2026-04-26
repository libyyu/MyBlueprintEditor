// ─────────────────────────────────────────────────────────────────────
// SaveSlotUI.cs — 存档/读档槽选择面板
//
// 显示 3 个槽 + 1 个自动存档，每个槽显示预览（时间/等级/金币/任务数）。
// ─────────────────────────────────────────────────────────────────────

using UnityEngine;
using UnityEngine.UI;
using TMPro;
using BlueprintRuntime.Samples.MiniGame.WeChat;

namespace BlueprintRuntime.Samples.MiniGame
{
    public class SaveSlotUI : MonoBehaviour
    {
        public enum Mode { Save, Load }

        [Header("UI 引用")]
        [SerializeField] private GameObject   panel;
        [SerializeField] private TMP_Text     titleText;
        [SerializeField] private RectTransform slotParent;
        [SerializeField] private GameObject   slotPrefab;    // 含 Info/Time/Button/Delete
        [SerializeField] private Button       closeButton;

        [Header("确认")]
        [SerializeField] private GameObject   confirmPanel;
        [SerializeField] private TMP_Text     confirmText;
        [SerializeField] private Button       confirmYes;
        [SerializeField] private Button       confirmNo;

        private Mode _mode;
        private int  _pendingSlot;

        void Start()
        {
            if (panel != null) panel.SetActive(false);
            if (confirmPanel != null) confirmPanel.SetActive(false);
            if (closeButton != null) closeButton.onClick.AddListener(Close);
            if (confirmYes  != null) confirmYes.onClick.AddListener(OnConfirm);
            if (confirmNo   != null) confirmNo.onClick.AddListener(() => confirmPanel?.SetActive(false));
        }

        public void Open(Mode mode)
        {
            _mode = mode;
            if (panel == null) return;
            panel.SetActive(true);
            if (titleText != null) titleText.text = mode == Mode.Save ? "保存进度" : "读取进度";
            Refresh();
        }

        public void Close()
        {
            if (panel != null) panel.SetActive(false);
        }

        private void Refresh()
        {
            if (SaveManager.Instance == null) return;

            // 清旧
            foreach (Transform child in slotParent) Destroy(child.gameObject);

            // 自动存档
            CreateSlotView(-1);

            // 普通槽
            for (int i = 0; i < SaveManager.Instance.SlotCount; i++)
                CreateSlotView(i);
        }

        private void CreateSlotView(int slot)
        {
            if (slotPrefab == null) return;
            var go = Instantiate(slotPrefab, slotParent);

            var preview = SaveManager.Instance.GetPreview(slot);
            string label = slot < 0 ? "自动存档" : $"存档 {slot + 1}";

            var infoText = go.transform.Find("Info")?.GetComponent<TMP_Text>();
            var timeText = go.transform.Find("Time")?.GetComponent<TMP_Text>();
            var btn      = go.transform.Find("Action")?.GetComponent<Button>();
            var delBtn   = go.transform.Find("Delete")?.GetComponent<Button>();
            var btnText  = btn?.GetComponentInChildren<TMP_Text>();

            if (preview.exists)
            {
                if (infoText != null)
                    infoText.text = $"{label}\nLv.{preview.level} | {preview.gold}G | 任务{preview.questCount} | NPC{preview.npcCount}";
                if (timeText != null)
                    timeText.text = preview.timestamp;
            }
            else
            {
                if (infoText != null) infoText.text = $"{label}\n<color=#999>空</color>";
                if (timeText != null) timeText.text = "";
            }

            // 按钮
            if (btn != null)
            {
                if (_mode == Mode.Save)
                {
                    if (btnText != null) btnText.text = "保存";
                    btn.interactable = (slot >= 0);  // 不能手动存到自动槽
                    int s = slot;
                    btn.onClick.AddListener(() => TryAction(s));
                }
                else
                {
                    if (btnText != null) btnText.text = "读取";
                    btn.interactable = preview.exists;
                    int s = slot;
                    btn.onClick.AddListener(() => TryAction(s));
                }
            }

            // 删除
            if (delBtn != null)
            {
                delBtn.gameObject.SetActive(preview.exists && slot >= 0);
                int s = slot;
                delBtn.onClick.AddListener(() =>
                {
                    SaveManager.Instance.Delete(s);
                    Refresh();
                });
            }
        }

        private void TryAction(int slot)
        {
            if (_mode == Mode.Save && SaveManager.Instance.HasSave(slot))
            {
                // 覆盖确认
                _pendingSlot = slot;
                if (confirmPanel != null)
                {
                    confirmPanel.SetActive(true);
                    if (confirmText != null)
                        confirmText.text = $"存档 {slot + 1} 已有数据，确认覆盖？";
                }
            }
            else
            {
                DoAction(slot);
            }
        }

        private void OnConfirm()
        {
            if (confirmPanel != null) confirmPanel.SetActive(false);
            DoAction(_pendingSlot);
        }

        private void DoAction(int slot)
        {
            if (SaveManager.Instance == null) return;

            if (_mode == Mode.Save)
            {
                SaveManager.Instance.Save(slot);
#if UNITY_WEBGL && !UNITY_EDITOR
                WeChatSDK.ShowToast("保存成功", WeChatSDK.ToastIcon.Success);
#endif
            }
            else
            {
                bool ok = SaveManager.Instance.Load(slot);
#if UNITY_WEBGL && !UNITY_EDITOR
                if (ok) WeChatSDK.ShowToast("读取成功", WeChatSDK.ToastIcon.Success);
                else    WeChatSDK.ShowToast("读取失败", WeChatSDK.ToastIcon.Error);
#endif
            }

            Refresh();
        }
    }
}
