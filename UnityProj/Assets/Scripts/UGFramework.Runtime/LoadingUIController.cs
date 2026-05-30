// LoadingUIController.cs
// 正式 Loading UI 控制器 — 挂到 LoadingUI.prefab 根节点
//
// 此脚本实现 ILoadingUI 接口，供 GameLauncher 统一调用。
// UI 元素在 Inspector 中连线（prefab 由美术/策划设计，支持热更）。
//
// 使用方式：
//   1. 在 Unity 编辑器中设计 LoadingUI.prefab
//   2. 将此脚本挂到 prefab 根节点
//   3. 在 Inspector 中将各 UI 元素拖入对应字段
//   4. 将 prefab 打入 LoadingUIPackage（YooAsset 分包）

using UnityEngine;
using UnityEngine.UI;
using TMPro;

namespace UGFramework.Runtime
{
    public class LoadingUIController : MonoBehaviour, ILoadingUI
    {
        [Header("进度条")]
        [SerializeField] private Slider     progressSlider;
        [SerializeField] private Image      progressFill;   // 可选：自定义颜色动画
        [SerializeField] private TMP_Text   progressLabel;  // 显示 "下载资源 42%"

        [Header("错误面板")]
        [SerializeField] private GameObject errorPanel;
        [SerializeField] private TMP_Text   errorLabel;
        [SerializeField] private Button     retryButton;

        [Header("动画（可选）")]
        [SerializeField] private Animator   logoAnimator;   // 入场动画
        [SerializeField] private string     showTrigger = "Show";

        // ── ILoadingUI 实现 ──────────────────────────────────────────

        public void Show(float initialProgress = 0f, string initialLabel = null)
        {
            gameObject.SetActive(true);
            if (errorPanel) errorPanel.SetActive(false);
            // 先设进度再播动画，确保第一帧就是正确进度状态而非初始帧
            SetProgress(initialProgress, initialLabel);
            if (logoAnimator && !string.IsNullOrEmpty(showTrigger))
                logoAnimator.SetTrigger(showTrigger);
        }

        public void SetProgress(float t, string label = null)
        {
            t = Mathf.Clamp01(t);
            if (progressSlider) progressSlider.value = t;
            if (progressLabel)  progressLabel.text   = label ?? $"{t * 100:F0}%";
        }

        public void ShowError(string message, System.Action onRetry)
        {
            if (errorLabel) errorLabel.text = message;
            if (errorPanel) errorPanel.SetActive(true);
            if (retryButton)
            {
                retryButton.onClick.RemoveAllListeners();
                retryButton.onClick.AddListener(() =>
                {
                    errorPanel.SetActive(false);
                    SetProgress(0f, "重试中...");
                    onRetry?.Invoke();
                });
            }
        }

        public void Hide()
        {
            Destroy(gameObject);
        }
    }
}
