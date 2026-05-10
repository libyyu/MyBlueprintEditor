// MainMenuController.cs
// 主菜单面板 Controller — 挂到 MainMenu.prefab 根节点
//
// 实现 IView 接口，供 UIManager 管理生命周期。
// 按钮事件通过 LuaFunction 回调暴露给 Lua，UI 逻辑全部在 Lua 侧实现。
//
// Inspector 连线：
//   startButton   → "开始游戏" 按钮
//   settingsButton → "设置" 按钮
//   quitButton    → "退出" 按钮（移动端隐藏）
//   versionLabel  → 版本号文本

using System;
using UnityEngine;
using UnityEngine.UI;
using TMPro;
using XLua;

namespace CutRope.Game.UI
{
    [LuaCallCSharp]
    public class MainMenuController : MonoBehaviour, CutRope.Framework.IView
    {
        [Header("按钮")]
        [SerializeField] private Button startButton;
        [SerializeField] private Button settingsButton;
        [SerializeField] private Button quitButton;

        [Header("文本")]
        [SerializeField] private TMP_Text versionLabel;

        [Header("动画（可选）")]
        [SerializeField] private Animator animator;
        [SerializeField] private string showTrigger = "Show";
        [SerializeField] private string hideTrigger = "Hide";

        // ── Lua 回调 ─────────────────────────────────────────────────
        // 由 Lua 侧通过 controller.OnStart = function() ... end 注入
        public Action OnStart    { get; set; }
        public Action OnSettings { get; set; }
        public Action OnQuit     { get; set; }

        // ── IView ────────────────────────────────────────────────────
        public bool       IsVisible  => gameObject.activeSelf;
        public GameObject ViewObject => gameObject;

        public void Show(string param = null)
        {
            gameObject.SetActive(true);
            if (versionLabel != null)
                versionLabel.text = $"v{Application.version}";
            if (animator && !string.IsNullOrEmpty(showTrigger))
                animator.SetTrigger(showTrigger);
        }

        public void Hide()
        {
            if (animator && !string.IsNullOrEmpty(hideTrigger))
                animator.SetTrigger(hideTrigger);
            else
                gameObject.SetActive(false);
        }

        public void Dispose()
        {
            OnStart    = null;
            OnSettings = null;
            OnQuit     = null;
            Destroy(gameObject);
        }

        // ── Unity 生命周期 ────────────────────────────────────────────
        private void Awake()
        {
            startButton?.onClick.AddListener(HandleStart);
            settingsButton?.onClick.AddListener(HandleSettings);

#if UNITY_EDITOR || UNITY_STANDALONE
            if (quitButton) quitButton.gameObject.SetActive(true);
            quitButton?.onClick.AddListener(HandleQuit);
#else
            // 移动端 / WebGL 隐藏退出按钮
            if (quitButton) quitButton.gameObject.SetActive(false);
#endif
        }

        private void OnDestroy()
        {
            OnStart    = null;
            OnSettings = null;
            OnQuit     = null;
        }

        // ── 按钮处理 ──────────────────────────────────────────────────
        private void HandleStart()    => OnStart?.Invoke();
        private void HandleSettings() => OnSettings?.Invoke();
        private void HandleQuit()     => OnQuit?.Invoke();
    }
}
