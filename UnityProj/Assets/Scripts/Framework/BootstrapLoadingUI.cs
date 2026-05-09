// BootstrapLoadingUI.cs
// 启动阶段极简 Loading UI — 纯代码生成，零资源依赖
//
// 为什么要纯代码生成？
//   WebGL 下 YooAsset 初始化之前无法加载任何 AssetBundle，
//   但玩家需要看到进度反馈。用代码直接创建 UGUI 控件，
//   不依赖任何 prefab / texture，全平台通用。
//
// 生命周期：
//   GameLauncher.Start() 调用 Show()
//   LoadingUIPackage 就绪、真正的 LoadingUI prefab 加载完毕后调用 Hide()

using UnityEngine;
using UnityEngine.UI;

namespace CutRope.Framework
{
    public class BootstrapLoadingUI : MonoBehaviour, ILoadingUI
    {
        // ── 运行时引用（由 CreateUI 填充） ───────────────────────────
        private Canvas    _canvas;
        private Image     _barBg;
        private Image     _barFill;
        private Text      _label;
        private Text      _errorText;
        private Button    _retryBtn;
        private GameObject _errorPanel;

        // ── 公共 API ─────────────────────────────────────────────────

        /// <summary>创建并显示极简 Loading UI</summary>
        public void Show()
        {
            if (_canvas != null) return;
            CreateUI();
            gameObject.SetActive(true);
        }

        /// <summary>更新进度（0~1）</summary>
        public void SetProgress(float t, string label = null)
        {
            if (_barFill) _barFill.fillAmount = Mathf.Clamp01(t);
            if (_label)   _label.text         = label ?? $"{t * 100:F0}%";
        }

        /// <summary>显示错误面板</summary>
        public void ShowError(string message, System.Action onRetry)
        {
            if (_errorText)  _errorText.text = message;
            if (_errorPanel) _errorPanel.SetActive(true);
            if (_retryBtn)
            {
                _retryBtn.onClick.RemoveAllListeners();
                _retryBtn.onClick.AddListener(() =>
                {
                    _errorPanel.SetActive(false);
                    SetProgress(0f, "重试中...");
                    onRetry?.Invoke();
                });
            }
        }

        /// <summary>隐藏并销毁（真正的 LoadingUI 接管后调用）</summary>
        public void Hide()
        {
            Destroy(gameObject);
        }

        // ── 内部 UI 构建 ─────────────────────────────────────────────

        private void CreateUI()
        {
            // Canvas（覆盖全屏）
            _canvas = gameObject.AddComponent<Canvas>();
            _canvas.renderMode  = RenderMode.ScreenSpaceOverlay;
            _canvas.sortingOrder = 9999;
            gameObject.AddComponent<CanvasScaler>().uiScaleMode =
                CanvasScaler.ScaleMode.ScaleWithScreenSize;
            gameObject.AddComponent<GraphicRaycaster>();

            // 黑色背景
            var bg = MakeRect("BG", _canvas.transform);
            bg.anchorMin = Vector2.zero;
            bg.anchorMax = Vector2.one;
            bg.sizeDelta = Vector2.zero;
            var bgImg = bg.gameObject.AddComponent<Image>();
            bgImg.color = new Color(0.08f, 0.08f, 0.08f, 1f);

            // 进度条容器（屏幕下方 1/3 处）
            var barContainer = MakeRect("BarContainer", bg);
            barContainer.anchorMin = new Vector2(0.1f, 0.38f);
            barContainer.anchorMax = new Vector2(0.9f, 0.42f);
            barContainer.sizeDelta = Vector2.zero;

            // 进度条背景
            _barBg = barContainer.gameObject.AddComponent<Image>();
            _barBg.color = new Color(0.2f, 0.2f, 0.2f, 1f);

            // 进度条填充
            var fillRect = MakeRect("Fill", barContainer);
            fillRect.anchorMin = Vector2.zero;
            fillRect.anchorMax = new Vector2(0f, 1f);
            fillRect.sizeDelta = Vector2.zero;
            fillRect.pivot     = new Vector2(0f, 0.5f);
            _barFill = fillRect.gameObject.AddComponent<Image>();
            _barFill.color      = new Color(0.2f, 0.8f, 0.4f, 1f);
            _barFill.fillMethod = Image.FillMethod.Horizontal;
            _barFill.type       = Image.Type.Filled;
            _barFill.fillAmount = 0f;

            // 百分比文字
            var labelRect = MakeRect("Label", bg);
            labelRect.anchorMin = new Vector2(0.1f, 0.43f);
            labelRect.anchorMax = new Vector2(0.9f, 0.47f);
            labelRect.sizeDelta = Vector2.zero;
            _label = labelRect.gameObject.AddComponent<Text>();
            _label.text      = "0%";
            _label.fontSize  = 24;
            _label.color     = Color.white;
            _label.alignment = TextAnchor.MiddleCenter;
            _label.font      = Resources.GetBuiltinResource<Font>("LegacyRuntime.ttf");

            // 错误面板（默认隐藏）
            _errorPanel = new GameObject("ErrorPanel");
            var epRect  = _errorPanel.AddComponent<RectTransform>();
            epRect.SetParent(bg, false);
            epRect.anchorMin = new Vector2(0.1f, 0.3f);
            epRect.anchorMax = new Vector2(0.9f, 0.55f);
            epRect.sizeDelta = Vector2.zero;
            _errorPanel.AddComponent<Image>().color = new Color(0.15f, 0.05f, 0.05f, 0.95f);
            _errorPanel.SetActive(false);

            // 错误文字
            var errLabelRect = MakeRect("ErrText", epRect);
            errLabelRect.anchorMin = new Vector2(0.05f, 0.5f);
            errLabelRect.anchorMax = new Vector2(0.95f, 0.95f);
            errLabelRect.sizeDelta = Vector2.zero;
            _errorText = errLabelRect.gameObject.AddComponent<Text>();
            _errorText.text      = "";
            _errorText.fontSize  = 20;
            _errorText.color     = new Color(1f, 0.4f, 0.4f);
            _errorText.alignment = TextAnchor.MiddleCenter;
            _errorText.font      = Resources.GetBuiltinResource<Font>("LegacyRuntime.ttf");

            // 重试按钮
            var retryRect = MakeRect("RetryBtn", epRect);
            retryRect.anchorMin = new Vector2(0.3f, 0.05f);
            retryRect.anchorMax = new Vector2(0.7f, 0.4f);
            retryRect.sizeDelta = Vector2.zero;
            _retryBtn = retryRect.gameObject.AddComponent<Button>();
            var retryImg = retryRect.gameObject.AddComponent<Image>();
            retryImg.color = new Color(0.8f, 0.2f, 0.2f);
            _retryBtn.targetGraphic = retryImg;
            var retryLabel = MakeRect("Text", retryRect);
            retryLabel.anchorMin = Vector2.zero;
            retryLabel.anchorMax = Vector2.one;
            retryLabel.sizeDelta = Vector2.zero;
            var retryText = retryLabel.gameObject.AddComponent<Text>();
            retryText.text      = "重试";
            retryText.fontSize  = 22;
            retryText.color     = Color.white;
            retryText.alignment = TextAnchor.MiddleCenter;
            retryText.font      = Resources.GetBuiltinResource<Font>("LegacyRuntime.ttf");
        }

        private static RectTransform MakeRect(string name, Transform parent)
        {
            var go = new GameObject(name);
            var rt = go.AddComponent<RectTransform>();
            rt.SetParent(parent, false);
            return rt;
        }
    }
}
