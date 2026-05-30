// UIController.cs
// 通用 UI 面板基类 —— C# 只负责组件查找与接口暴露，逻辑全在 Lua 里
//
// 用法（Lua 侧）：
//   local ctrl = UIManager.Open("UI/MainMenu")
//   ctrl:register_on_click("btn_start", function() ... end)
//   ctrl:set_text("lbl_title", "Cut the Rope")
//   ctrl:show()

using System.Collections.Generic;
using UnityEngine;
using UnityEngine.UI;
using TMPro;
using XLua;

namespace CutRope.Game.UI
{
    [LuaCallCSharp]
    public class UIController : MonoBehaviour, UGFramework.Runtime.IView
    {
        // ── 缓存 ──────────────────────────────────────────────────────
        private readonly Dictionary<string, GameObject>  _goCache     = new();
        private readonly Dictionary<string, Button>      _btnCache    = new();
        private readonly Dictionary<string, TMP_Text>    _tmpCache    = new();
        private readonly Dictionary<string, Text>        _textCache   = new();
        private readonly Dictionary<string, Image>       _imageCache  = new();
        private readonly Dictionary<string, Slider>      _sliderCache = new();
        private readonly List<(Button btn, LuaFunction fn)> _listeners = new();

        // 每个面板打开时由 UIManager 注入、关闭时设为 null 的 Lua open 函数
        private LuaFunction _onShow;
        private LuaFunction _onHide;
        private LuaFunction _onDispose;

        // ── IView ─────────────────────────────────────────────────────
        public bool       IsVisible  => gameObject.activeSelf;
        public GameObject ViewObject => gameObject;

        /// <summary>供 Lua 通过 ctrl:GetViewObject() 获取 GameObject</summary>
        public GameObject GetViewObject() => gameObject;

        public void Show()
        {
            gameObject.SetActive(true);
            _onShow?.Call();
        }

        public void Hide()
        {
            gameObject.SetActive(false);
            _onHide?.Call();
        }

        public void Dispose()
        {
            _onDispose?.Call();
            ClearListeners();
            _onShow?.Dispose();   _onShow    = null;
            _onHide?.Dispose();   _onHide    = null;
            _onDispose?.Dispose(); _onDispose = null;
            Destroy(gameObject);
        }

        // ── Lua 可调用 API ─────────────────────────────────────────────

        /// <summary>注册面板生命周期回调</summary>
        public void SetCallbacks(LuaFunction onShow, LuaFunction onHide, LuaFunction onDispose)
        {
            _onShow?.Dispose();    _onShow    = onShow;
            _onHide?.Dispose();    _onHide    = onHide;
            _onDispose?.Dispose(); _onDispose = onDispose;
        }

        /// <summary>给子节点按钮注册点击回调（Lua function）</summary>
        public void RegisterOnClick(string childName, LuaFunction fn)
        {
            var btn = GetBtn(childName);
            if (btn == null) { Debug.LogWarning($"[UIController] Button not found: {childName}"); return; }
            System.Action cb = () => fn.Call();
            btn.onClick.AddListener(new UnityEngine.Events.UnityAction(cb));
            _listeners.Add((btn, fn));
        }

        /// <summary>移除某按钮所有 Lua 监听</summary>
        public void UnregisterOnClick(string childName)
        {
            var btn = GetBtn(childName);
            if (btn == null) return;
            btn.onClick.RemoveAllListeners();
            _listeners.RemoveAll(pair => {
                if (pair.btn == btn) { pair.fn?.Dispose(); return true; }
                return false;
            });
        }

        /// <summary>设置 TMP_Text 或 legacy Text 的文字</summary>
        public void SetText(string childName, string text)
        {
            if (GetTMP(childName) is TMP_Text tmp) { tmp.text = text; return; }
            if (GetLegacyText(childName) is Text t) { t.text   = text; return; }
            Debug.LogWarning($"[UIController] Text not found: {childName}");
        }

        /// <summary>获取 TMP_Text 或 legacy Text 的文字</summary>
        public string GetTextValue(string childName)
        {
            if (GetTMP(childName) is TMP_Text tmp) return tmp.text;
            if (GetLegacyText(childName) is Text t) return t.text;
            return "";
        }

        /// <summary>设置子节点显示/隐藏</summary>
        public void SetActive(string childName, bool active)
        {
            var go = GetGo(childName);
            if (go != null) go.SetActive(active);
            else Debug.LogWarning($"[UIController] GameObject not found: {childName}");
        }

        /// <summary>设置 Image sprite（通过 YooAsset 外部加载后传入）</summary>
        public void SetSprite(string childName, Sprite sp)
        {
            var img = GetImage(childName);
            if (img != null) img.sprite = sp;
            else Debug.LogWarning($"[UIController] Image not found: {childName}");
        }

        /// <summary>设置 Image fillAmount（0~1，用于进度条/星级等）</summary>
        public void SetFill(string childName, float val)
        {
            var img = GetImage(childName);
            if (img != null) img.fillAmount = Mathf.Clamp01(val);
        }

        /// <summary>设置 Slider value（0~1）</summary>
        public void SetSlider(string childName, float val)
        {
            var slider = GetSlider(childName);
            if (slider != null) slider.value = Mathf.Clamp01(val);
        }

        /// <summary>设置按钮可否交互</summary>
        public void SetInteractable(string childName, bool interactable)
        {
            var btn = GetBtn(childName);
            if (btn != null) btn.interactable = interactable;
        }

        /// <summary>设置 Image color（r,g,b,a 均 0~1）</summary>
        public void SetColor(string childName, float r, float g, float b, float a = 1f)
        {
            var img = GetImage(childName);
            if (img != null) img.color = new Color(r, g, b, a);
        }

        // ── 内部查找（带缓存）──────────────────────────────────────────

        private GameObject GetGo(string name)
        {
            if (_goCache.TryGetValue(name, out var go)) return go;
            var t = transform.Find(name) ?? FindDeep(transform, name);
            go = t?.gameObject;
            if (go != null) _goCache[name] = go;
            return go;
        }

        private Button GetBtn(string name)
        {
            if (_btnCache.TryGetValue(name, out var c)) return c;
            var go = GetGo(name);
            c = go?.GetComponent<Button>();
            if (c != null) _btnCache[name] = c;
            return c;
        }

        private TMP_Text GetTMP(string name)
        {
            if (_tmpCache.TryGetValue(name, out var c)) return c;
            var go = GetGo(name);
            c = go?.GetComponent<TMP_Text>();
            if (c != null) _tmpCache[name] = c;
            return c;
        }

        private Text GetLegacyText(string name)
        {
            if (_textCache.TryGetValue(name, out var c)) return c;
            var go = GetGo(name);
            c = go?.GetComponent<Text>();
            if (c != null) _textCache[name] = c;
            return c;
        }

        private Image GetImage(string name)
        {
            if (_imageCache.TryGetValue(name, out var c)) return c;
            var go = GetGo(name);
            c = go?.GetComponent<Image>();
            if (c != null) _imageCache[name] = c;
            return c;
        }

        private Slider GetSlider(string name)
        {
            if (_sliderCache.TryGetValue(name, out var c)) return c;
            var go = GetGo(name);
            c = go?.GetComponent<Slider>();
            if (c != null) _sliderCache[name] = c;
            return c;
        }

        /// <summary>深度查找（transform.Find 不找隐藏节点时的补救）</summary>
        private static Transform FindDeep(Transform root, string name)
        {
            for (int i = 0; i < root.childCount; i++)
            {
                var child = root.GetChild(i);
                if (child.name == name) return child;
                var found = FindDeep(child, name);
                if (found != null) return found;
            }
            return null;
        }

        private void ClearListeners()
        {
            foreach (var (btn, fn) in _listeners)
            {
                btn?.onClick.RemoveAllListeners();
                fn?.Dispose();
            }
            _listeners.Clear();
        }

        private void OnDestroy() => ClearListeners();
    }
}
