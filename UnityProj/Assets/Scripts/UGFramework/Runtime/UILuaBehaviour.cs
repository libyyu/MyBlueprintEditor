using UnityEngine;
using System.Collections;
using System.Collections.Generic;
using System;
using UnityEngine.UI;
using XLua;
using TMPro;
using UGFramework.Runtime.UGUIEvent;

namespace UGFramework.Runtime
{
    [LuaCallCSharp]
    public class UILuaBehaviour : MonoBehaviour, IUIPanelBridge
    {
        private List<LuaFunction> buttons = new List<LuaFunction>();
        private LuaTable msgHandle = null;
        protected bool initialize = false;

        // ─── IUIPanelBridge 实现所需的状态 ────────────────────────────────
        // name -> GameObject 缓存（避免每次 transform.Find）
        private readonly Dictionary<string, GameObject> _qCache = new Dictionary<string, GameObject>();

        // RegisterClick 累加列表（与 buttons 字段独立，buttons 只服务于 AddClick）
        private struct ClickEntry
        {
            public string name;
            public Button button;
            public UnityEngine.Events.UnityAction action;
            public LuaFunction lua;
        }

        private readonly List<ClickEntry> _explicitClicks = new List<ClickEntry>();

        // 通用事件 entry（Submit/TextChange/ValueChange 共用）
        // detach 闭包封装具体的 RemoveListener 逻辑，免去保存原 UnityAction 类型的复杂性。
        private struct EventEntry
        {
            public string name;
            public LuaFunction lua;
            public Action detach;
        }
        private readonly List<EventEntry> _explicitSubmits  = new List<EventEntry>();
        private readonly List<EventEntry> _explicitChanges  = new List<EventEntry>();
        private readonly List<EventEntry> _explicitValues   = new List<EventEntry>();


        private bool __visible = false;

        protected void Awake()
        {
            initialize = true;
            CallMethod("onAwake", gameObject);
        }

        protected void Start()
        {
            CallMethod("onStart");

            if (!__visible && gameObject.activeSelf)
            {
                OnBecameVisible();
            }
        }

        protected void OnDestroy()
        {
            CallMethod("onDestroy");
            UnTouchGUIMsg();

            // 清理 IUIPanelBridge 累加的所有精确绑定（与 buttons 字段独立）
            for (int i = 0; i < _explicitClicks.Count; ++i)
            {
                var e = _explicitClicks[i];
                if (e.button != null) e.button.onClick.RemoveListener(e.action);
                e.lua?.Dispose();
            }
            _explicitClicks.Clear();
            ClearEntries(_explicitSubmits);
            ClearEntries(_explicitChanges);
            ClearEntries(_explicitValues);
            _qCache.Clear();

            initialize = false;
        }

        protected void OnBecameVisible()
        {
            __visible = true;
            CallMethod("onBecameVisible");
        }

        protected void OnBecameInvisible()
        {
            __visible = false;
            CallMethod("onBecameInvisible");
        }

        /// <summary>
        /// 添加单击事件
        /// </summary>
        public void AddClick(GameObject go, LuaFunction luafunc)
        {
            if (go == null) return;
            buttons.Add(luafunc);
            go.GetComponent<Button>().onClick.AddListener(
                delegate() { luafunc.Call(go); }
            );
        }

        /// <summary>
        /// 清除单击事件
        /// </summary>
        public void ClearClick()
        {
            for (int i = 0; i < buttons.Count; i++)
            {
                if (buttons[i] != null)
                {
                    buttons[i].Dispose();
                    buttons[i] = null;
                }
            }
        }

        public void UnTouchGUIMsg()
        {
            ClearClick();
            DetachEventHandle();
            msgHandle = null;
        }

        public void TouchGUIMsg(LuaTable luaMsgHandler)
        {
            UnTouchGUIMsg();
            msgHandle = luaMsgHandler;
            if (msgHandle != null)
            {
                if (HasMethod("onClick"))           TouchButton();
                if (HasMethod("onSubmit"))          TouchInputField();
                else if (HasMethod("onTextChange")) TouchInputField();
                if (HasMethod("onStepTweenFinish")) TouchTweener();
                if (HasMethod("onScroll"))          TouchScroll();
            }
        }

        [ContextMenu("UnTouchEvent")]
        public void DetachEventHandle()
        {
            UnTouchButton();
            UnTouchInputField();
            UnTouchTweener();
            UnTouchScroll();
        }

        [ContextMenu("TouchEvent")]
        public void AttachEventHandle()
        {
            DetachEventHandle();
            TouchButton();
            TouchInputField();
            TouchTweener();
            TouchScroll();
        }

        public void TouchButton()
        {
            var comps = gameObject.GetComponentsInChildren<Button>(true);
            int Length = comps.Length;
            for (int i = 0; i < Length; ++i)
            {
                Button button = comps[i];
                EventClick.Get(button.gameObject).onClick += onClick;
            }
        }

        public void UnTouchButton()
        {
            var comps = gameObject.GetComponentsInChildren<Button>(true);
            int Length = comps.Length;
            for (int i = 0; i < Length; ++i)
            {
                Button button = comps[i];
                EventClick.Get(button.gameObject).onClick -= onClick;
            }
        }

        public void TouchInputField()
        {
            var comps = gameObject.GetComponentsInChildren<InputField>(true);
            int Length = comps.Length;
            for (int i = 0; i < Length; ++i)
            {
                InputField inputfield = comps[i];
                EventEdit.Get(inputfield.gameObject).onSubmit += onSubmit;
                EventEdit.Get(inputfield.gameObject).onTextChange += onTextChange;
            }
        }

        public void UnTouchInputField()
        {
            var comps = gameObject.GetComponentsInChildren<InputField>(true);
            int Length = comps.Length;
            for (int i = 0; i < Length; ++i)
            {
                InputField inputfield = comps[i];
                EventEdit.Get(inputfield.gameObject).onSubmit -= onSubmit;
                EventEdit.Get(inputfield.gameObject).onTextChange -= onTextChange;
            }
        }

        public void TouchTweener()
        {
            //var comps = gameObject.GetComponentsInChildren<UGUI.ITween>(true);
            //int Length = comps.Length;
            //for (int i = 0; i < Length; ++i)
            //{
            //    UGUI.ITween tweener = comps[i];
            //    tweener.onTweenFinish += onTweenFinish;
            //    tweener.onStepTweenFinish += onStepTweenFinish;
            //}
        }

        public void UnTouchTweener()
        {
            //var comps = gameObject.GetComponentsInChildren<UGUI.ITween>(true);
            //int Length = comps.Length;
            //for (int i = 0; i < Length; ++i)
            //{
            //    UGUI.ITween tweener = comps[i];
            //    tweener.onTweenFinish -= onTweenFinish;
            //    tweener.onStepTweenFinish -= onStepTweenFinish;
            //}
        }

        void TouchScroll()
        {
            var comps = gameObject.GetComponentsInChildren<Slider>(true);
            int Length = comps.Length;
            for (int i = 0; i < Length; ++i)
            {
                Slider slider = comps[i];
                EventSlider.Get(slider.gameObject).onScroll += onScroll;
            }
        }

        void UnTouchScroll()
        {
            var comps = gameObject.GetComponentsInChildren<Slider>(true);
            int Length = comps.Length;
            for (int i = 0; i < Length; ++i)
            {
                Slider slider = comps[i];
                EventSlider.Get(slider.gameObject).onScroll -= onScroll;
            }
        }

        void onClick(GameObject go)
        {
            CallMethod("onClick", go);
        }

        void onSubmit(GameObject go, string str)
        {
            CallMethod("onSubmit", go, str);
        }

        void onTextChange(GameObject go, string str)
        {
            CallMethod("onTextChange", go, str);
        }

        void onTweenFinish(GameObject go)
        {
            CallMethod("onTweenFinish", go);
        }

        void onStepTweenFinish(GameObject go)
        {
            CallMethod("onStepTweenFinish", go);
        }

        void onScroll(GameObject go, float value)
        {
            CallMethod("onScroll", go, value);
        }

        /// <summary>
        /// 执行Lua方法。
        /// 修复：每次 Get LuaFunction 都会创建 wrapper + xLua 注册表条目，必须 Dispose。
        /// 否则像 onScroll/onTextChange/onClick 这类高频回调每帧/每按键都会泄漏一个。
        /// </summary>
        object[] CallMethod(string func, params object[] args)
        {
            if (!initialize || null == msgHandle || !msgHandle.IsValid()) return null;
            var fun = msgHandle.Get<LuaFunction>(func);
            if (null == fun) return null;
            try { return fun.Call(args); } finally { fun.Dispose(); }
        }

        /// <summary>
        /// 仅做存在性判断。Get 必须配 Dispose，否则每次 TouchGUIMsg 都会泄漏多个 LuaFunction。
        /// </summary>
        private bool HasMethod(string name)
        {
            if (msgHandle == null || !msgHandle.IsValid()) return false;
            var fun = msgHandle.Get<LuaFunction>(name);
            if (fun == null) return false;
            fun.Dispose();
            return true;
        }

        // ═══════════════════════════════════════════════════════════════════
        // IUIPanelBridge 实现 —— 后端无关的 UI 操作 API
        // 所有方法签名与 UITKLuaBridge 完全一致，Lua 调用代码两端通用。
        //
        // 寻址：name 直接传给 transform.Find，支持 "TopBar/lbl_score" 路径式
        // 缓存：name -> GameObject 字典，重复访问 O(1)
        // ═══════════════════════════════════════════════════════════════════

        /// <summary>路径式 / 扁平 name 查找子 GameObject（带缓存）</summary>
        private GameObject FindByName(string name)
        {
            if (string.IsNullOrEmpty(name)) return null;
            if (_qCache.TryGetValue(name, out var cached) && cached != null)
                return cached;

            // 优先走 transform.Find（O(深度) 路径式）
            var tr = transform.Find(name);
            if (tr != null)
            {
                _qCache[name] = tr.gameObject;
                return tr.gameObject;
            }

            // Fallback：递归全树搜索（兼容只传末段 name 的情况）
            var found = FindByNameRecursive(transform, name);
            if (found != null) _qCache[name] = found;
            return found;
        }

        private static GameObject FindByNameRecursive(Transform parent, string name)
        {
            for (int i = 0; i < parent.childCount; ++i)
            {
                var c = parent.GetChild(i);
                if (c.name == name) return c.gameObject;
                var r = FindByNameRecursive(c, name);
                if (r != null) return r;
            }

            return null;
        }

        public object Q(string name) => FindByName(name);

        // ─── 文本 ────────────────────────────────────────────────────────
        public void SetText(string name, string text)
        {
            var go = FindByName(name);
            if (go == null)
            {
                Debug.LogWarning($"[UILuaBehaviour] SetText: '{name}' not found");
                return;
            }

            var tmp = go.GetComponent<TMP_Text>();
            if (tmp != null)
            {
                tmp.text = text;
                return;
            }

            var txt = go.GetComponent<UnityEngine.UI.Text>();
            if (txt != null)
            {
                txt.text = text;
                return;
            }

            // Button 的标签可能是子节点上的 Text/TMP_Text
            var btnTmp = go.GetComponentInChildren<TMP_Text>(true);
            if (btnTmp != null)
            {
                btnTmp.text = text;
                return;
            }

            var btnTxt = go.GetComponentInChildren<UnityEngine.UI.Text>(true);
            if (btnTxt != null)
            {
                btnTxt.text = text;
                return;
            }

            Debug.LogWarning($"[UILuaBehaviour] SetText: '{name}' has no Text/TMP_Text component");
        }

        public string GetText(string name)
        {
            var go = FindByName(name);
            if (go == null) return "";

            var tmp = go.GetComponent<TMP_Text>();
            if (tmp != null) return tmp.text;
            var txt = go.GetComponent<UnityEngine.UI.Text>();
            if (txt != null) return txt.text;

            var btnTmp = go.GetComponentInChildren<TMP_Text>(true);
            if (btnTmp != null) return btnTmp.text;
            var btnTxt = go.GetComponentInChildren<UnityEngine.UI.Text>(true);
            if (btnTxt != null) return btnTxt.text;

            return "";
        }

        // ─── 显隐 / 启用 ─────────────────────────────────────────────────

        /// <summary>完全隐藏（不占布局）— SetActive</summary>
        public void SetDisplay(string name, bool visible)
        {
            var go = FindByName(name);
            if (go != null) go.SetActive(visible);
            else Debug.LogWarning($"[UILuaBehaviour] SetDisplay: '{name}' not found");
        }

        /// <summary>隐藏但保留布局 — 优先 CanvasGroup.alpha，其次 Graphic.enabled</summary>
        public void SetVisibility(string name, bool visible)
        {
            var go = FindByName(name);
            if (go == null) return;

            var cg = go.GetComponent<CanvasGroup>();
            if (cg != null)
            {
                cg.alpha = visible ? 1f : 0f;
                cg.blocksRaycasts = visible;
                return;
            }

            var g = go.GetComponent<Graphic>();
            if (g != null) g.enabled = visible;
        }

        /// <summary>启用/禁用交互 — Selectable.interactable</summary>
        public void SetEnabled(string name, bool enabled)
        {
            var go = FindByName(name);
            if (go == null) return;

            var sel = go.GetComponent<Selectable>(); // Button/Toggle/Slider/InputField 都是 Selectable
            if (sel != null) sel.interactable = enabled;
            else Debug.LogWarning($"[UILuaBehaviour] SetEnabled: '{name}' has no Selectable");
        }

        // ─── 视觉 ────────────────────────────────────────────────────────

        public void SetColor(string name, float r, float g, float b, float a)
        {
            var go = FindByName(name);
            if (go == null) return;

            var color = new Color(r, g, b, a);
            var tmp = go.GetComponent<TMP_Text>();
            if (tmp != null)
            {
                tmp.color = color;
                return;
            }

            var graphic = go.GetComponent<Graphic>(); // Image / RawImage / Text 都是 Graphic
            if (graphic != null)
            {
                graphic.color = color;
                return;
            }

            Debug.LogWarning($"[UILuaBehaviour] SetColor: '{name}' has no Graphic/TMP_Text");
        }

        public void SetSprite(string name, Sprite sprite)
        {
            var go = FindByName(name);
            if (go == null) return;
            var img = go.GetComponent<Image>();
            if (img != null)
            {
                img.sprite = sprite;
                return;
            }

            Debug.LogWarning($"[UILuaBehaviour] SetSprite: '{name}' has no Image");
        }

        // ─── 表单控件 ────────────────────────────────────────────────────

        public void SetSliderValue(string name, float value)
        {
            var go = FindByName(name);
            var s = go?.GetComponent<Slider>();
            if (s != null) s.value = value;
        }

        public float GetSliderValue(string name)
        {
            var go = FindByName(name);
            var s = go?.GetComponent<Slider>();
            return s != null ? s.value : 0f;
        }

        public void SetToggleValue(string name, bool value)
        {
            var go = FindByName(name);
            var t = go?.GetComponent<Toggle>();
            if (t != null) t.isOn = value;
        }

        public bool GetToggleValue(string name)
        {
            var go = FindByName(name);
            var t = go?.GetComponent<Toggle>();
            return t != null && t.isOn;
        }

        // ─── 事件（与 UITK 端语义一致：累加，不覆盖）─────────────────────

        /// <summary>
        /// 注册按钮点击（对齐 UITKLuaBridge.RegisterClick 语义）。
        /// 同 name 多次调用全部累加，按注册顺序触发；UnregisterClick 清掉所有同 name 绑定。
        /// 与 TouchGUIMsg 全局扫描完全独立，可共存。
        /// 回调签名：function(name) end
        /// </summary>
        public void RegisterClick(string name, LuaFunction callback)
        {
            if (callback == null || string.IsNullOrEmpty(name)) return;
            var go = FindByName(name);
            if (go == null)
            {
                Debug.LogWarning($"[UILuaBehaviour] RegisterClick: '{name}' not found");
                return;
            }

            var btn = go.GetComponent<Button>();
            if (btn == null)
            {
                Debug.LogWarning($"[UILuaBehaviour] RegisterClick: '{name}' has no Button");
                return;
            }

            var captured = callback;
            UnityEngine.Events.UnityAction action = () => captured.Call(name);
            btn.onClick.AddListener(action);
            _explicitClicks.Add(new ClickEntry { name = name, button = btn, action = action, lua = captured });
        }

        /// <summary>取消该 name 的所有精确绑定（不影响 TouchGUIMsg/AddClick）。</summary>
        public void UnregisterClick(string name)
        {
            if (string.IsNullOrEmpty(name) || _explicitClicks.Count == 0) return;
            for (int i = _explicitClicks.Count - 1; i >= 0; --i)
            {
                var e = _explicitClicks[i];
                if (e.name != name) continue;
                if (e.button != null) e.button.onClick.RemoveListener(e.action);
                e.lua?.Dispose();
                _explicitClicks.RemoveAt(i);
            }
        }

        // ─── 输入框值读写 ────────────────────────────────────────────────

        public void SetInputText(string name, string text)
        {
            var go = FindByName(name);
            if (go == null) { Debug.LogWarning($"[UILuaBehaviour] SetInputText: '{name}' not found"); return; }
            var f = go.GetComponent<InputField>();
            if (f != null) { f.text = text; return; }
            var tf = go.GetComponent<TMP_InputField>();
            if (tf != null) { tf.text = text; return; }
            Debug.LogWarning($"[UILuaBehaviour] SetInputText: '{name}' is not an InputField/TMP_InputField");
        }

        public string GetInputText(string name)
        {
            var go = FindByName(name);
            if (go == null) return "";
            var f = go.GetComponent<InputField>();
            if (f != null) return f.text;
            var tf = go.GetComponent<TMP_InputField>();
            if (tf != null) return tf.text;
            return "";
        }

        // ─── Submit (输入框提交) ─────────────────────────────────────────

        /// <summary>
        /// 输入框提交（回车）回调。回调签名：function(name, text) end
        /// UGUI 用 onEndEdit（InputField/TMP_InputField 都有）。
        /// 多次注册累加，UnregisterSubmit(name) 清同 name 所有绑定。
        /// </summary>
        public void RegisterSubmit(string name, LuaFunction callback)
        {
            if (callback == null || string.IsNullOrEmpty(name)) return;
            var go = FindByName(name);
            if (go == null) { Debug.LogWarning($"[UILuaBehaviour] RegisterSubmit: '{name}' not found"); return; }

            var captured = callback;
            UnityEngine.Events.UnityAction<string> action = s => captured.Call(name, s);

            Action detach = null;
            var f = go.GetComponent<InputField>();
            if (f != null)
            {
                f.onEndEdit.AddListener(action);
                detach = () => { if (f != null) f.onEndEdit.RemoveListener(action); };
            }
            else
            {
                var tf = go.GetComponent<TMP_InputField>();
                if (tf != null)
                {
                    tf.onEndEdit.AddListener(action);
                    detach = () => { if (tf != null) tf.onEndEdit.RemoveListener(action); };
                }
            }
            if (detach == null)
            {
                Debug.LogWarning($"[UILuaBehaviour] RegisterSubmit: '{name}' has no InputField/TMP_InputField");
                return;
            }
            _explicitSubmits.Add(new EventEntry { name = name, lua = captured, detach = detach });
        }

        public void UnregisterSubmit(string name) => UnregisterEntry(_explicitSubmits, name);

        // ─── TextChange (输入框值变化) ───────────────────────────────────

        /// <summary>
        /// 输入框文本变化回调。回调签名：function(name, newText) end
        /// UGUI 用 onValueChanged。
        /// </summary>
        public void RegisterTextChange(string name, LuaFunction callback)
        {
            if (callback == null || string.IsNullOrEmpty(name)) return;
            var go = FindByName(name);
            if (go == null) { Debug.LogWarning($"[UILuaBehaviour] RegisterTextChange: '{name}' not found"); return; }

            var captured = callback;
            UnityEngine.Events.UnityAction<string> action = s => captured.Call(name, s);

            Action detach = null;
            var f = go.GetComponent<InputField>();
            if (f != null)
            {
                f.onValueChanged.AddListener(action);
                detach = () => { if (f != null) f.onValueChanged.RemoveListener(action); };
            }
            else
            {
                var tf = go.GetComponent<TMP_InputField>();
                if (tf != null)
                {
                    tf.onValueChanged.AddListener(action);
                    detach = () => { if (tf != null) tf.onValueChanged.RemoveListener(action); };
                }
            }
            if (detach == null)
            {
                Debug.LogWarning($"[UILuaBehaviour] RegisterTextChange: '{name}' has no InputField/TMP_InputField");
                return;
            }
            _explicitChanges.Add(new EventEntry { name = name, lua = captured, detach = detach });
        }

        public void UnregisterTextChange(string name) => UnregisterEntry(_explicitChanges, name);

        // ─── ValueChange (Slider/Toggle 值变化) ──────────────────────────

        /// <summary>
        /// Slider/Toggle 值变化回调。回调签名：function(name, value) end
        /// value 为 float（Slider）或 bool（Toggle）。
        /// </summary>
        public void RegisterValueChange(string name, LuaFunction callback)
        {
            if (callback == null || string.IsNullOrEmpty(name)) return;
            var go = FindByName(name);
            if (go == null) { Debug.LogWarning($"[UILuaBehaviour] RegisterValueChange: '{name}' not found"); return; }

            var captured = callback;
            Action detach = null;

            var sl = go.GetComponent<Slider>();
            if (sl != null)
            {
                UnityEngine.Events.UnityAction<float> a = v => captured.Call(name, v);
                sl.onValueChanged.AddListener(a);
                detach = () => { if (sl != null) sl.onValueChanged.RemoveListener(a); };
            }
            else
            {
                var tg = go.GetComponent<Toggle>();
                if (tg != null)
                {
                    UnityEngine.Events.UnityAction<bool> a = v => captured.Call(name, v);
                    tg.onValueChanged.AddListener(a);
                    detach = () => { if (tg != null) tg.onValueChanged.RemoveListener(a); };
                }
            }
            if (detach == null)
            {
                Debug.LogWarning($"[UILuaBehaviour] RegisterValueChange: '{name}' has no Slider/Toggle");
                return;
            }
            _explicitValues.Add(new EventEntry { name = name, lua = captured, detach = detach });
        }

        public void UnregisterValueChange(string name) => UnregisterEntry(_explicitValues, name);

        // ─── 通用 Unregister/Clear 帮助函数 ──────────────────────────────

        private static void UnregisterEntry(List<EventEntry> list, string name)
        {
            if (string.IsNullOrEmpty(name) || list.Count == 0) return;
            for (int i = list.Count - 1; i >= 0; --i)
            {
                var e = list[i];
                if (e.name != name) continue;
                e.detach?.Invoke();
                e.lua?.Dispose();
                list.RemoveAt(i);
            }
        }

        private static void ClearEntries(List<EventEntry> list)
        {
            for (int i = 0; i < list.Count; ++i)
            {
                var e = list[i];
                e.detach?.Invoke();
                e.lua?.Dispose();
            }
            list.Clear();
        }

        /// <summary>
        /// 清理全部 listener（对齐 UITKLuaBridge.ClearAllListeners 语义）：
        ///   - 全局扫描的 onClick / onSubmit / onTextChange (DetachEventHandle)
        ///   - AddClick 累加的 buttons 列表 (ClearClick)
        ///   - 本桥接的 RegisterClick 精确绑定
        /// 由 FPanelBaseUI:DestroyPanelRaw 调用。
        /// </summary>
        public void ClearAllListeners()
        {
            // 旧路径：DetachEventHandle + ClearClick
            DetachEventHandle();
            ClearClick();

            // 新路径：精确绑定
            for (int i = 0; i < _explicitClicks.Count; ++i)
            {
                var e = _explicitClicks[i];
                if (e.button != null) e.button.onClick.RemoveListener(e.action);
                e.lua?.Dispose();
            }

            _explicitClicks.Clear();
            ClearEntries(_explicitSubmits);
            ClearEntries(_explicitChanges);
            ClearEntries(_explicitValues);

            // 缓存清空（GameObject 可能已被 destroy）
            _qCache.Clear();
        }

        //-----------------------------------------------------------------
    }
}
