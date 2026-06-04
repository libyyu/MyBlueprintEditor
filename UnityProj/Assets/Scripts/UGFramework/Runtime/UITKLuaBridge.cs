// UITKLuaBridge.cs
// UI Toolkit 版事件桥接器，等价于 UGUI 的 UILuaBehaviour
//
// 功能：
//   1. TouchAllButtons(msgHandler) —— 扫描所有 Button 元素，统一路由到 OnClick(name)
//      完全等价于 UILuaBehaviour.TouchButton()，Lua 侧 OnClick 参数从 GameObject 变为 name string
//   2. RegisterClick(name, func)   —— 精确绑定单个元素点击
//   3. UnregisterClick(name)       —— 取消单个元素的精确绑定
//   4. Q(name)                     —— 查找 VisualElement，供 Lua 直接操作
//   5. SetText(name, text)         —— 快捷设置 Label/Button 文本
//   6. SetDisplay(name, visible)   —— 快捷控制元素显隐
//   7. ClearGlobalScan()           —— 仅清理 TouchAllButtons 注册的全局扫描
//   8. ClearAllListeners()         —— 清理全部事件（面板销毁时自动调用）
//   9. TouchAllInputs(msgHandler)  —— 扫描所有 TextField，路由到 OnSubmit(name) / OnChange(name,val)
//
// 设计：两种模式可共存（对齐 UGUI 的 UILuaBehaviour.AddClick 语义）
//   - 全局扫描（TouchAllButtons）和精确绑定（RegisterClick）使用独立的 listener 列表
//   - 同一按钮同时有全局 + 精确回调时，两者都会触发
//   - 同一按钮多次 RegisterClick → 全部累加，按注册顺序依次触发（不覆盖）
//     （等价于 UGUI 的 AddClick：多次调用即多次绑定）
//   - 重复 TouchAllButtons → 仅替换全局扫描，保留所有精确绑定

using System;
using System.Collections.Generic;
using UnityEngine;
using UnityEngine.UIElements;
using XLua;

namespace UGFramework.Runtime
{
    [LuaCallCSharp]
    public class UITKLuaBridge : MonoBehaviour, IUIPanelBridge
    {
        private LuaTable msgHandle = null;
        protected bool initialize = false;
        private bool __visible = false;
        // ── 内部结构 ─────────────────────────────────────────────────────

        /// <summary>全局扫描模式的 listener（每个 Button 一条）</summary>
        private struct GlobalEntry
        {
            public VisualElement element;
            public EventCallback<ClickEvent> callback;
        }

        /// <summary>精确绑定模式的 listener（同一 name 可有多条，全部累加）</summary>
        private struct ExplicitEntry
        {
            public string name;               // 元素名，UnregisterClick 时按此匹配
            public VisualElement element;
            public EventCallback<ClickEvent> callback;
            public LuaFunction luaCallback;   // 持有 Lua 闭包，Dispose 时释放
        }

        private UIDocument _doc;

        // 全局扫描：TouchAllButtons 注册的所有 Button 回调
        private readonly List<GlobalEntry> _globalEntries = new List<GlobalEntry>();
        private LuaFunction _globalOnClick;

        // 精确绑定：RegisterClick 注册的单元素回调（同 name 可累加多条，对齐 UGUI AddClick）
        // 与全局扫描完全独立——同一按钮可同时有全局和精确两类回调，全部依次触发
        private readonly List<ExplicitEntry> _explicitEntries = new List<ExplicitEntry>();

        // 通用事件 entry（Submit/TextChange/ValueChange 共用）
        // detach 闭包封装具体 UnregisterCallback 调用，免去保存 EventCallback<T> 类型的复杂性。
        private struct EventEntry
        {
            public string name;
            public LuaFunction lua;
            public Action detach;
        }
        private readonly List<EventEntry> _explicitSubmits = new List<EventEntry>();
        private readonly List<EventEntry> _explicitChanges = new List<EventEntry>();
        private readonly List<EventEntry> _explicitValues  = new List<EventEntry>();

        // name -> element 缓存（避免每次全树 Q）
        private readonly Dictionary<string, VisualElement> _qCache = new Dictionary<string, VisualElement>();

        /// <summary>
        /// 把 UGUI 风格的路径式 name（如 "TopBar/lbl_score"）转为 UITK 扁平 name。
        /// UITK 没有"父子路径寻址"概念，取最后一段作为 element.name 查询。
        /// </summary>
        private static string NormalizeName(string name)
        {
            if (string.IsNullOrEmpty(name)) return name;
            int slash = name.LastIndexOf('/');
            return slash < 0 ? name : name.Substring(slash + 1);
        }

        /// <summary>带缓存的元素查找。</summary>
        private VisualElement Lookup(string name)
        {
            if (string.IsNullOrEmpty(name) || _doc == null) return null;
            var key = name;
            if (_qCache.TryGetValue(key, out var cached) && cached != null) return cached;

            var root = _doc.rootVisualElement;
            if (root == null) return null;

            var n = NormalizeName(name);
            var el = root.Q(n);
            if (el != null) _qCache[key] = el;
            return el;
        }

        // ── 初始化 ───────────────────────────────────────────────────────

        /// <summary>由 UITKPanelBackend 在创建时调用</summary>
        public void Init(UIDocument doc)
        {
            _doc = doc;
        }

        public void TouchGUIMsg(LuaTable luaMsgHandler)
        {
            msgHandle = luaMsgHandler;
            TouchAllButtons(luaMsgHandler);
            TouchAllInputs(luaMsgHandler);
        }

        // ── 模式一：全局扫描（等价 UILuaBehaviour.TouchButton + TouchGUIMsg）────

        /// <summary>
        /// 扫描所有 Button 元素，统一路由到 msgHandler.onClick(name)。
        /// 与 UILuaBehaviour.TouchGUIMsg 行为对齐，但参数改为 element.name (string)。
        ///
        /// 与 RegisterClick 完全独立：重复调用本方法只会替换全局扫描的回调，
        /// 不会影响已注册的精确绑定（RegisterClick）。
        ///
        /// Lua 使用示例：
        ///   -- FPanelBaseUI 在检测到 UITK 后端时自动调用
        ///   function FPanelXxx:OnClick(name)
        ///       if name == "btn_start" then self:OnClickStart() end
        ///   end
        /// </summary>
        public void TouchAllButtons(LuaTable msgHandler)
        {
            // 仅清理已有的全局扫描，保留所有精确绑定
            ClearGlobalScan();

            _globalOnClick = msgHandler?.Get<LuaFunction>("onClick");
            if (_globalOnClick == null || _doc == null || _doc.rootVisualElement == null) return;

            var buttons = _doc.rootVisualElement.Query<Button>().ToList();
            foreach (var btn in buttons)
            {
                EventCallback<ClickEvent> cb = _ =>
                {
                    if (_globalOnClick != null && _globalOnClick.IsValid())
                        _globalOnClick.Call(btn);
                };
                btn.RegisterCallback(cb);
                _globalEntries.Add(new GlobalEntry { element = btn, callback = cb });
            }
        }

        // ── 模式一-B：全局 TextField 扫描（onSubmit / onTextChange）────────────

        /// <summary>全局 TextField 扫描的 listener</summary>
        private struct InputEntry
        {
            public TextField element;
            public EventCallback<ChangeEvent<string>> cbChange;
            public EventCallback<NavigationSubmitEvent> cbSubmit;
        }
        private readonly List<InputEntry> _inputEntries = new List<InputEntry>();
        private LuaFunction _globalOnChange;
        private LuaFunction _globalOnSubmit;

        /// <summary>
        /// 扫描所有 TextField，统一路由：
        ///   msgHandler.onTextChange(name, value)  —— 值变化时触发
        ///   msgHandler.onSubmit(name)             —— 提交（回车/确认）时触发
        /// 与 UGUI 的 UILuaBehaviour.TouchGUIMsg onTextChange/onSubmit 对齐。
        /// </summary>
        public void TouchAllInputs(LuaTable msgHandler)
        {
            ClearInputScan();
            _globalOnChange = msgHandler?.Get<LuaFunction>("onTextChange");
            _globalOnSubmit = msgHandler?.Get<LuaFunction>("onSubmit");
            if (_doc == null || _doc.rootVisualElement == null) return;

            var fields = _doc.rootVisualElement.Query<TextField>().ToList();
            foreach (var tf in fields)
            {
                EventCallback<ChangeEvent<string>> cbChange = null;
                EventCallback<NavigationSubmitEvent> cbSubmit = null;

                if (_globalOnChange != null && _globalOnChange.IsValid())
                {
                    cbChange = evt =>
                    {
                        if (_globalOnChange != null && _globalOnChange.IsValid())
                            _globalOnChange.Call(tf, evt.newValue);
                    };
                    tf.RegisterCallback(cbChange);
                }
                if (_globalOnSubmit != null && _globalOnSubmit.IsValid())
                {
                    cbSubmit = _ =>
                    {
                        if (_globalOnSubmit != null && _globalOnSubmit.IsValid())
                            _globalOnSubmit.Call(tf);
                    };
                    tf.RegisterCallback(cbSubmit);
                }
                _inputEntries.Add(new InputEntry { element = tf, cbChange = cbChange, cbSubmit = cbSubmit });
            }
        }

        /// <summary>仅清理 TouchAllInputs 注册的 TextField 扫描回调。</summary>
        public void ClearInputScan()
        {
            foreach (var entry in _inputEntries)
            {
                if (entry.element == null) continue;
                if (entry.cbChange != null) entry.element.UnregisterCallback(entry.cbChange);
                if (entry.cbSubmit != null) entry.element.UnregisterCallback(entry.cbSubmit);
            }
            _inputEntries.Clear();
            if (_globalOnChange != null && _globalOnChange.IsValid()) _globalOnChange.Dispose();
            if (_globalOnSubmit != null && _globalOnSubmit.IsValid()) _globalOnSubmit.Dispose();
            _globalOnChange = null;
            _globalOnSubmit = null;
        }

        /// <summary>仅清理 TouchAllButtons 注册的全局扫描回调，保留所有精确绑定。</summary>
        public void ClearGlobalScan()
        {
            foreach (var entry in _globalEntries)
            {
                if (entry.element != null)
                    entry.element.UnregisterCallback(entry.callback);
            }
            _globalEntries.Clear();

            if (_globalOnClick != null && _globalOnClick.IsValid())
                _globalOnClick.Dispose();
            _globalOnClick = null;
        }

        // ── 模式二：精确绑定（等价 btn.onClick.AddListener）──────────────

        /// <summary>
        /// 精确绑定单个元素的点击事件（语义对齐 UGUI 的 UILuaBehaviour.AddClick）。
        ///
        /// - 与 TouchAllButtons 完全独立：本方法不会清除全局扫描；
        /// - 同一 name 多次调用 → 全部累加，按注册顺序依次触发（不覆盖旧绑定）；
        /// - 需要取消某个 name 的所有绑定时，调用 UnregisterClick(name)。
        ///
        /// Lua 使用示例：
        ///   self.m_bridge:RegisterClick("btn_start", function(name) self:OnClickStart() end)
        ///   -- 同一按钮可继续追加：
        ///   self.m_bridge:RegisterClick("btn_start", function(name) self:Analytics(name) end)
        /// </summary>
        public void RegisterClick(string name, LuaFunction callback)
        {
            if (_doc == null || callback == null || string.IsNullOrEmpty(name)) return;
            var el = Lookup(name);
            if (el == null)
            {
                Debug.LogWarning($"[UITKLuaBridge] RegisterClick: element '{name}' not found");
                return;
            }

            // 累加（不覆盖）：同 name 可注册多个回调，全部触发
            var captured = callback;
            var n = NormalizeName(name);   // 回调参数用扁平 name，与 TouchAllButtons 对齐
            EventCallback<ClickEvent> cb = _ =>
            {
                if(captured.IsValid())
                    captured.Call(el);
            };
            el.RegisterCallback(cb);
            _explicitEntries.Add(new ExplicitEntry
            {
                name = n,
                element = el,
                callback = cb,
                luaCallback = captured,
            });
        }

        /// <summary>
        /// 取消该 name 的所有精确绑定（不影响全局扫描）。
        /// 等价于 UGUI 的 ClearClick，但仅作用于指定 name。
        /// </summary>
        public void UnregisterClick(string name)
        {
            if (string.IsNullOrEmpty(name) || _explicitEntries.Count == 0) return;
            var n = NormalizeName(name);

            // 倒序遍历，安全地从列表中移除多条同 name 记录
            for (int i = _explicitEntries.Count - 1; i >= 0; --i)
            {
                var entry = _explicitEntries[i];
                if (entry.name != n) continue;

                if (entry.element != null)
                    entry.element.UnregisterCallback(entry.callback);
                if(entry.luaCallback != null && entry.luaCallback.IsValid())
                    entry.luaCallback?.Dispose();
                _explicitEntries.RemoveAt(i);
            }
        }

        // ── 输入框值读写 / Submit / TextChange / ValueChange ──────────────

        public void SetInputText(string name, string text)
        {
            var el = Lookup(name);
            if (el is TextField tf) tf.value = text;
            else if (el == null) Debug.LogWarning($"[UITKLuaBridge] SetInputText: '{name}' not found");
            else Debug.LogWarning($"[UITKLuaBridge] SetInputText: '{name}' is not a TextField");
        }

        public string GetInputText(string name)
        {
            var el = Lookup(name);
            return el is TextField tf ? tf.value : "";
        }

        /// <summary>
        /// 输入框提交回调。回调签名：function(name, text) end
        /// UITK 用 NavigationSubmitEvent（回车/确认）。
        /// </summary>
        public void RegisterSubmit(string name, LuaFunction callback)
        {
            if (callback == null || string.IsNullOrEmpty(name)) return;
            var el = Lookup(name);
            if (el == null) { Debug.LogWarning($"[UITKLuaBridge] RegisterSubmit: '{name}' not found"); return; }
            if (!(el is TextField tf)) { Debug.LogWarning($"[UITKLuaBridge] RegisterSubmit: '{name}' is not a TextField"); return; }

            var captured = callback;
            var n = NormalizeName(name);
            EventCallback<NavigationSubmitEvent> cb = _ =>
            {
                if (captured.IsValid()) captured.Call(tf, tf.value);
            };
            tf.RegisterCallback(cb);
            Action detach = () => { if (tf != null) tf.UnregisterCallback(cb); };
            _explicitSubmits.Add(new EventEntry { name = n, lua = captured, detach = detach });
        }

        public void UnregisterSubmit(string name) => UnregisterEntry(_explicitSubmits, name);

        /// <summary>
        /// 输入框文本变化回调。回调签名：function(name, newText) end
        /// UITK 用 ChangeEvent&lt;string&gt;。
        /// </summary>
        public void RegisterTextChange(string name, LuaFunction callback)
        {
            if (callback == null || string.IsNullOrEmpty(name)) return;
            var el = Lookup(name);
            if (el == null) { Debug.LogWarning($"[UITKLuaBridge] RegisterTextChange: '{name}' not found"); return; }
            if (!(el is TextField tf)) { Debug.LogWarning($"[UITKLuaBridge] RegisterTextChange: '{name}' is not a TextField"); return; }

            var captured = callback;
            var n = NormalizeName(name);
            EventCallback<ChangeEvent<string>> cb = evt =>
            {
                if (captured.IsValid()) captured.Call(el, evt.newValue);
            };
            tf.RegisterCallback(cb);
            Action detach = () => { if (tf != null) tf.UnregisterCallback(cb); };
            _explicitChanges.Add(new EventEntry { name = n, lua = captured, detach = detach });
        }

        public void UnregisterTextChange(string name) => UnregisterEntry(_explicitChanges, name);

        /// <summary>
        /// Slider/Toggle 值变化回调。回调签名：function(name, value) end
        /// value 为 float（Slider）或 bool（Toggle）。
        /// </summary>
        public void RegisterValueChange(string name, LuaFunction callback)
        {
            if (callback == null || string.IsNullOrEmpty(name)) return;
            var el = Lookup(name);
            if (el == null) { Debug.LogWarning($"[UITKLuaBridge] RegisterValueChange: '{name}' not found"); return; }

            var captured = callback;
            var n = NormalizeName(name);
            Action detach = null;

            if (el is Slider sl)
            {
                EventCallback<ChangeEvent<float>> cb = evt =>
                {
                    if (captured.IsValid()) captured.Call(el, evt.newValue);
                };
                sl.RegisterCallback(cb);
                detach = () => { if (sl != null) sl.UnregisterCallback(cb); };
            }
            else if (el is Toggle tg)
            {
                EventCallback<ChangeEvent<bool>> cb = evt =>
                {
                    if (captured.IsValid()) captured.Call(el, evt.newValue);
                };
                tg.RegisterCallback(cb);
                detach = () => { if (tg != null) tg.UnregisterCallback(cb); };
            }
            else
            {
                Debug.LogWarning($"[UITKLuaBridge] RegisterValueChange: '{name}' is not a Slider/Toggle");
                return;
            }
            _explicitValues.Add(new EventEntry { name = n, lua = captured, detach = detach });
        }

        public void UnregisterValueChange(string name) => UnregisterEntry(_explicitValues, name);

        // ── 通用 Unregister/Clear 帮助函数 ───────────────────────────────

        private static void UnregisterEntry(List<EventEntry> list, string name)
        {
            if (string.IsNullOrEmpty(name) || list.Count == 0) return;
            var n = NormalizeName(name);
            for (int i = list.Count - 1; i >= 0; --i)
            {
                var e = list[i];
                if (e.name != n) continue;
                e.detach?.Invoke();
                if (e.lua != null && e.lua.IsValid()) e.lua.Dispose();
                list.RemoveAt(i);
            }
        }

        private static void ClearEntries(List<EventEntry> list)
        {
            for (int i = 0; i < list.Count; ++i)
            {
                var e = list[i];
                e.detach?.Invoke();
                if (e.lua != null && e.lua.IsValid()) e.lua.Dispose();
            }
            list.Clear();
        }

        // ═══════════════════════════════════════════════════════════════════
        // IUIPanelBridge 实现 —— 与 UILuaBehaviour 同名同参，Lua 调用代码后端无关
        // 寻址支持路径式 name（如 "TopBar/lbl_score"），自动取末段当 element.name
        // ═══════════════════════════════════════════════════════════════════

        // ── 元素查找 ─────────────────────────────────────────────────────

        /// <summary>
        /// 查找 VisualElement（带缓存）。
        /// 路径式 name（如 "Header/btn_back"）自动取最后一段。
        /// 返回值是 object 类型以满足 IUIPanelBridge 接口（UGUI 端返回 GameObject）。
        /// Lua 端可继续 cast：local ve = self.m_bridge:Q("name")  （UITK 后端时是 VisualElement）
        /// </summary>
        public object Q(string name) => Lookup(name);

        /// <summary>按类型查找，类型名传字符串（"Label"/"Button"/"TextField" 等）。UITK 专属。</summary>
        public VisualElement QByType(string name, string typeName)
        {
            if (_doc == null) return null;
            var root = _doc.rootVisualElement;
            if (root == null) return null;
            var n = NormalizeName(name);
            return typeName switch
            {
                "Label" => root.Q<Label>(n),
                "Button" => root.Q<Button>(n),
                "TextField" => root.Q<TextField>(n),
                "Toggle" => root.Q<Toggle>(n),
                "Slider" => root.Q<Slider>(n),
                _ => root.Q(n),
            };
        }

        // ── 文本 ────────────────────────────────────────────────────────

        public void SetText(string name, string text)
        {
            var el = Lookup(name);
            if (el is Label lbl) lbl.text = text;
            else if (el is Button btn) btn.text = text;
            else if (el is TextField tf) tf.value = text;
            else if (el == null) Debug.LogWarning($"[UITKLuaBridge] SetText: '{name}' not found");
            else Debug.LogWarning($"[UITKLuaBridge] SetText: '{name}' is not a text element");
        }

        public string GetText(string name)
        {
            var el = Lookup(name);
            return el switch
            {
                Label lbl => lbl.text,
                Button btn => btn.text,
                TextField tf => tf.value,
                _ => ""
            };
        }

        // ── 显隐 / 启用 ─────────────────────────────────────────────────

        /// <summary>完全隐藏（不占布局）— display Flex/None</summary>
        public void SetDisplay(string name, bool visible)
        {
            var el = Lookup(name);
            if (el != null)
                el.style.display = visible ? DisplayStyle.Flex : DisplayStyle.None;
            else
                Debug.LogWarning($"[UITKLuaBridge] SetDisplay: '{name}' not found");
        }

        /// <summary>隐藏但保留布局 — visibility Visible/Hidden</summary>
        public void SetVisibility(string name, bool visible)
        {
            var el = Lookup(name);
            if (el != null)
                el.style.visibility = visible ? Visibility.Visible : Visibility.Hidden;
        }

        /// <summary>启用/禁用交互 — VisualElement.SetEnabled</summary>
        public void SetEnabled(string name, bool enabled)
        {
            var el = Lookup(name);
            if (el != null) el.SetEnabled(enabled);
        }

        // ── 视觉 ────────────────────────────────────────────────────────

        /// <summary>
        /// 颜色：Label/Button 改文本色（style.color），其他改背景色（style.backgroundColor）。
        /// </summary>
        public void SetColor(string name, float r, float g, float b, float a)
        {
            var el = Lookup(name);
            if (el == null) return;
            var c = new Color(r, g, b, a);
            if (el is Label || el is Button || el is TextField)
                el.style.color = c;
            else
                el.style.backgroundColor = c;
        }

        /// <summary>设置 Sprite 作为 backgroundImage（UITK 不直接吃 Sprite，需转 StyleBackground）</summary>
        public void SetSprite(string name, Sprite sprite)
        {
            var el = Lookup(name);
            if (el == null) return;
            el.style.backgroundImage = sprite != null
                ? new StyleBackground(sprite)
                : StyleKeyword.None;
        }

        // ── 表单控件 ────────────────────────────────────────────────────

        public void SetSliderValue(string name, float value)
        {
            var el = Lookup(name);
            if (el is Slider s) s.value = value;
        }
        public float GetSliderValue(string name)
        {
            var el = Lookup(name);
            return el is Slider s ? s.value : 0f;
        }

        public void SetToggleValue(string name, bool value)
        {
            var el = Lookup(name);
            if (el is Toggle t) t.value = value;
        }
        public bool GetToggleValue(string name)
        {
            var el = Lookup(name);
            return el is Toggle t && t.value;
        }

        // ── USS 类（UITK 专属）──────────────────────────────────────────

        public void AddClass(string name, string className)
            => Lookup(name)?.AddToClassList(className);

        public void RemoveClass(string name, string className)
            => Lookup(name)?.RemoveFromClassList(className);

        // ── 清理 ─────────────────────────────────────────────────────────

        /// <summary>
        /// 清理全部事件（全局扫描 + 所有精确绑定），释放所有 LuaFunction。
        /// 面板销毁时由 FPanelBaseUI:DestroyPanelRaw 自动调用。
        /// 等价于 UGUI 的 UILuaBehaviour.UnTouchGUIMsg + ClearClick。
        /// </summary>
        public void ClearAllListeners()
        {
            // 全局扫描（按钮 + 输入框）
            ClearGlobalScan();
            ClearInputScan();

            // 精确绑定（点击 + 输入框 Submit/TextChange + Slider/Toggle ValueChange）
            foreach (var entry in _explicitEntries)
            {
                if (entry.element != null)
                    entry.element.UnregisterCallback(entry.callback);
                if(entry.luaCallback != null && entry.luaCallback.IsValid())
                    entry.luaCallback.Dispose();
            }
            _explicitEntries.Clear();
            ClearEntries(_explicitSubmits);
            ClearEntries(_explicitChanges);
            ClearEntries(_explicitValues);

            // 元素缓存（VisualElement 可能已随 UIDocument 销毁）
            _qCache.Clear();
        }

        private void OnDestroy()
        {
            CallMethod("onDestroy");
            ClearAllListeners();
            initialize = false;
        }

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

        // 修复：Get<LuaFunction> 必须 Dispose，否则每次 onAwake/onStart/onBecameVisible/
        // onBecameInvisible 都会泄漏一个 wrapper（可显示/隐藏 Panel 反复触发）。
        void CallMethod(string func, params object[] args)
        {
            if (!initialize || null == msgHandle || !msgHandle.IsValid()) return;
            var fun = msgHandle.Get<LuaFunction>(func);
            if (null == fun) return;
            try { fun.Call(args); } finally { fun.Dispose(); }
        }
    }
}
