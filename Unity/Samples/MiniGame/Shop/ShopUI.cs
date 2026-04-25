// ─────────────────────────────────────────────────────────────────────
// ShopUI.cs — 商店买卖界面
//
// 左侧商品列表，右侧详情 + 买入/卖出按钮。
// 显示折扣信息、库存、玩家金币余额。
// ─────────────────────────────────────────────────────────────────────

using System.Collections.Generic;
using UnityEngine;
using UnityEngine.UI;
using TMPro;

namespace BlueprintRuntime.Samples.MiniGame
{
    public class ShopUI : MonoBehaviour
    {
        [Header("UI 引用")]
        [SerializeField] private GameObject    panel;
        [SerializeField] private TMP_Text      shopNameText;
        [SerializeField] private TMP_Text      goldText;
        [SerializeField] private RectTransform  listParent;
        [SerializeField] private GameObject    itemPrefab;
        [SerializeField] private TMP_Text      detailName;
        [SerializeField] private TMP_Text      detailDesc;
        [SerializeField] private TMP_Text      detailPrice;
        [SerializeField] private Image         detailIcon;
        [SerializeField] private Button        buyButton;
        [SerializeField] private Button        sellButton;
        [SerializeField] private Button        closeButton;
        [SerializeField] private TMP_Text      discountText;

        private NpcShop _currentShop;
        private int _selectedIndex = -1;
        private readonly List<ShopItemView> _views = new();

        private struct ShopItemView
        {
            public GameObject go;
            public Image      icon;
            public TMP_Text   nameText;
            public TMP_Text   priceText;
            public TMP_Text   stockText;
            public Button     button;
        }

        void Start()
        {
            if (panel != null) panel.SetActive(false);
            if (closeButton != null) closeButton.onClick.AddListener(Close);
            if (buyButton   != null) buyButton.onClick.AddListener(OnBuy);
            if (sellButton  != null) sellButton.onClick.AddListener(OnSell);
        }

        public void Open(NpcShop shop)
        {
            if (shop == null || panel == null) return;
            _currentShop = shop;
            _selectedIndex = -1;
            panel.SetActive(true);

            if (shopNameText != null) shopNameText.text = shop.ShopName;

            Refresh();
            shop.OnShopOpened?.Invoke();
        }

        public void Close()
        {
            if (panel != null) panel.SetActive(false);
            _currentShop = null;
        }

        private void Refresh()
        {
            if (_currentShop == null) return;

            var items = _currentShop.Items;
            while (_views.Count < items.Count) _views.Add(CreateItemView());

            for (int i = 0; i < _views.Count; i++)
            {
                if (i < items.Count)
                {
                    var item = items[i];
                    var def = Inventory.Instance?.GetItemDef(item.itemId);
                    var v = _views[i];
                    v.go.SetActive(true);

                    if (v.icon != null) { v.icon.sprite = def?.icon; v.icon.enabled = def?.icon != null; }
                    if (v.nameText != null)  v.nameText.text  = def?.displayName ?? item.itemId;
                    if (v.priceText != null) v.priceText.text = $"{_currentShop.GetBuyPrice(item)}G";
                    if (v.stockText != null)
                    {
                        v.stockText.text = (item.stock >= 0) ? $"库存:{item.stock}" : "";
                    }
                }
                else
                {
                    _views[i].go.SetActive(false);
                }
            }

            // 金币
            if (goldText != null && PlayerStats.Instance != null)
                goldText.text = $"金币: {PlayerStats.Instance.Gold}";

            UpdateDetail();
        }

        private void UpdateDetail()
        {
            var items = _currentShop?.Items;
            if (items == null || _selectedIndex < 0 || _selectedIndex >= items.Count)
            {
                if (detailName  != null) detailName.text  = "";
                if (detailDesc  != null) detailDesc.text  = "选择商品查看详情";
                if (detailPrice != null) detailPrice.text = "";
                if (detailIcon  != null) detailIcon.enabled = false;
                if (buyButton   != null) buyButton.interactable = false;
                if (sellButton  != null) sellButton.interactable = false;
                if (discountText != null) discountText.text = "";
                return;
            }

            var item = items[_selectedIndex];
            var def = Inventory.Instance?.GetItemDef(item.itemId);
            int price = _currentShop.GetBuyPrice(item);

            if (detailName  != null) detailName.text  = def?.displayName ?? item.itemId;
            if (detailDesc  != null) detailDesc.text  = def?.description ?? "";
            if (detailPrice != null) detailPrice.text = $"买入: {price}G | 卖出: {_currentShop.GetSellPrice(item.itemId)}G";
            if (detailIcon  != null) { detailIcon.sprite = def?.icon; detailIcon.enabled = def?.icon != null; }

            bool canBuy = PlayerStats.Instance != null && PlayerStats.Instance.Gold >= price;
            if (buyButton != null) buyButton.interactable = canBuy;

            bool canSell = Inventory.Instance != null && Inventory.Instance.HasItem(item.itemId);
            if (sellButton != null) sellButton.interactable = canSell;

            // 折扣提示
            if (discountText != null)
            {
                int basePrice = (item.priceOverride > 0 ? item.priceOverride : (def?.buyPrice ?? 10));
                if (price < basePrice)
                    discountText.text = $"<color=#FF6B6B>好感折扣 {Mathf.RoundToInt((float)price / basePrice * 100)}%</color>";
                else
                    discountText.text = "";
            }
        }

        private void OnBuy()
        {
            var items = _currentShop?.Items;
            if (items == null || _selectedIndex < 0 || _selectedIndex >= items.Count) return;
            _currentShop.Buy(items[_selectedIndex]);
            Refresh();
        }

        private void OnSell()
        {
            var items = _currentShop?.Items;
            if (items == null || _selectedIndex < 0 || _selectedIndex >= items.Count) return;
            _currentShop.Sell(items[_selectedIndex].itemId);
            Refresh();
        }

        private ShopItemView CreateItemView()
        {
            var go = Instantiate(itemPrefab, listParent);
            int idx = _views.Count;
            var v = new ShopItemView
            {
                go        = go,
                icon      = go.transform.Find("Icon")?.GetComponent<Image>(),
                nameText  = go.transform.Find("Name")?.GetComponent<TMP_Text>(),
                priceText = go.transform.Find("Price")?.GetComponent<TMP_Text>(),
                stockText = go.transform.Find("Stock")?.GetComponent<TMP_Text>(),
                button    = go.GetComponent<Button>() ?? go.AddComponent<Button>(),
            };
            v.button.onClick.AddListener(() => { _selectedIndex = idx; UpdateDetail(); });
            return v;
        }
    }
}
