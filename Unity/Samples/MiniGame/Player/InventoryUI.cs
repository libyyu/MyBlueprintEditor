// ─────────────────────────────────────────────────────────────────────
// InventoryUI.cs — 背包 UI 面板
//
// 网格布局显示所有物品槽，点击选中查看详情。
// 支持：拖拽排序（可选）、使用/丢弃按钮、数量显示。
// ─────────────────────────────────────────────────────────────────────

using System.Collections.Generic;
using UnityEngine;
using UnityEngine.UI;
using TMPro;

namespace BlueprintMiniGame
{
    public class InventoryUI : MonoBehaviour
    {
        [Header("UI 引用")]
        [SerializeField] private GameObject    panel;
        [SerializeField] private RectTransform gridParent;
        [SerializeField] private GameObject    slotPrefab;    // 含 Icon(Image) + Count(TMP_Text)
        [SerializeField] private TMP_Text      detailName;
        [SerializeField] private TMP_Text      detailDesc;
        [SerializeField] private Image         detailIcon;
        [SerializeField] private Button        useButton;
        [SerializeField] private Button        discardButton;
        [SerializeField] private TMP_Text      capacityText;  // "12/30"

        [Header("操作")]
        [SerializeField] private KeyCode toggleKey = KeyCode.I;

        private readonly List<SlotView> _views = new();
        private int _selectedIndex = -1;

        private struct SlotView
        {
            public GameObject go;
            public Image      icon;
            public TMP_Text   countText;
            public Image      highlight;
            public Button     button;
        }

        void Start()
        {
            if (panel != null) panel.SetActive(false);

            if (Inventory.Instance != null)
                Inventory.Instance.OnInventoryChanged += Refresh;

            if (useButton != null)
                useButton.onClick.AddListener(OnUse);
            if (discardButton != null)
                discardButton.onClick.AddListener(OnDiscard);
        }

        void Update()
        {
            if (Input.GetKeyDown(toggleKey))
                Toggle();
        }

        public void Toggle()
        {
            if (panel == null) return;
            bool show = !panel.activeSelf;
            panel.SetActive(show);
            if (show) Refresh();
        }

        public void Show()  { if (panel != null) { panel.SetActive(true);  Refresh(); } }
        public void Hide()  { if (panel != null) panel.SetActive(false); }

        private void Refresh()
        {
            if (Inventory.Instance == null) return;
            var slots = Inventory.Instance.Slots;

            // 确保 view 池足够
            while (_views.Count < Inventory.Instance.MaxSlots)
                _views.Add(CreateSlotView());

            for (int i = 0; i < _views.Count; i++)
            {
                var v = _views[i];
                if (i < slots.Count && slots[i].count > 0)
                {
                    v.go.SetActive(true);
                    var def = Inventory.Instance.GetItemDef(slots[i].itemId);
                    if (v.icon != null)
                    {
                        v.icon.sprite = def?.icon;
                        v.icon.enabled = def?.icon != null;
                    }
                    if (v.countText != null)
                        v.countText.text = slots[i].count > 1 ? slots[i].count.ToString() : "";
                    if (v.highlight != null)
                        v.highlight.enabled = (i == _selectedIndex);
                }
                else
                {
                    // 空槽
                    v.go.SetActive(i < Inventory.Instance.MaxSlots);
                    if (v.icon != null) v.icon.enabled = false;
                    if (v.countText != null) v.countText.text = "";
                    if (v.highlight != null) v.highlight.enabled = false;
                }
            }

            // 容量
            if (capacityText != null)
                capacityText.text = $"{Inventory.Instance.UsedSlots}/{Inventory.Instance.MaxSlots}";

            // 详情
            UpdateDetail();
        }

        private void UpdateDetail()
        {
            var slots = Inventory.Instance?.Slots;
            if (slots == null || _selectedIndex < 0 || _selectedIndex >= slots.Count)
            {
                if (detailName != null) detailName.text = "";
                if (detailDesc != null) detailDesc.text = "选择一个物品查看详情";
                if (detailIcon != null) detailIcon.enabled = false;
                if (useButton != null) useButton.interactable = false;
                if (discardButton != null) discardButton.interactable = false;
                return;
            }

            var slot = slots[_selectedIndex];
            var def = Inventory.Instance.GetItemDef(slot.itemId);

            if (detailName != null) detailName.text = def?.displayName ?? slot.itemId;
            if (detailDesc != null) detailDesc.text = def?.description ?? "";
            if (detailIcon != null) { detailIcon.sprite = def?.icon; detailIcon.enabled = def?.icon != null; }
            if (useButton != null) useButton.interactable = (def?.type == ItemType.Consumable || def?.type == ItemType.Gift);
            if (discardButton != null) discardButton.interactable = true;
        }

        private void SelectSlot(int index)
        {
            _selectedIndex = index;
            Refresh();
        }

        private void OnUse()
        {
            var slots = Inventory.Instance?.Slots;
            if (slots == null || _selectedIndex < 0 || _selectedIndex >= slots.Count) return;
            string id = slots[_selectedIndex].itemId;
            Inventory.Instance.RemoveItem(id, 1);
            // TODO: 触发物品效果（恢复 HP / 送礼等）
            if (_selectedIndex >= slots.Count) _selectedIndex = slots.Count - 1;
            Refresh();
        }

        private void OnDiscard()
        {
            var slots = Inventory.Instance?.Slots;
            if (slots == null || _selectedIndex < 0 || _selectedIndex >= slots.Count) return;
            string id = slots[_selectedIndex].itemId;
            Inventory.Instance.RemoveItem(id, 1);
            if (_selectedIndex >= slots.Count) _selectedIndex = slots.Count - 1;
            Refresh();
        }

        private SlotView CreateSlotView()
        {
            var go = Instantiate(slotPrefab, gridParent);
            int idx = _views.Count;
            var v = new SlotView
            {
                go        = go,
                icon      = go.transform.Find("Icon")?.GetComponent<Image>(),
                countText = go.transform.Find("Count")?.GetComponent<TMP_Text>(),
                highlight = go.transform.Find("Highlight")?.GetComponent<Image>(),
                button    = go.GetComponent<Button>() ?? go.AddComponent<Button>(),
            };
            v.button.onClick.AddListener(() => SelectSlot(idx));
            return v;
        }
    }
}
