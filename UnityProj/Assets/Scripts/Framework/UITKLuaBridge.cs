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

namespace CutRope.Framework
{
    [LuaCallCSharp]
    public class UITKLuaBridge : MonoBehaviour
    {
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

        // ── 初始化 ───────────────────────────────────────────────────────

        /// <summary>由 UITKPanelBackend 在创建时调用</summary>
        public void Init(UIDocument doc)
        {
            _doc = doc;
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
                var capturedName = btn.name;
                EventCallback<ClickEvent> cb = _ =>
                {
                    if (_globalOnClick != null)
                        _globalOnClick.Call(capturedName);
                };
                btn.RegisterCallback(cb);
                _globalEntries.Add(new GlobalEntry { element = btn, callback = cb });
            }
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

            _globalOnClick?.Dispose();
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
            if (_doc.rootVisualElement == null)
            {
                Debug.LogWarning($"[UITKLuaBridge] RegisterClick('{name}'): rootVisualElement not ready");
                return;
            }
            var el = _doc.rootVisualElement.Q(name);
            if (el == null)
            {
                Debug.LogWarning($"[UITKLuaBridge] RegisterClick: element '{name}' not found");
                return;
            }

            // 累加（不覆盖）：同 name 可注册多个回调，全部触发
            var captured = callback;
            EventCallback<ClickEvent> cb = _ => captured.Call(name);
            el.RegisterCallback(cb);
            _explicitEntries.Add(new ExplicitEntry
            {
                name        = name,
                element     = el,
                callback    = cb,
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

            // 倒序遍历，安全地从列表中移除多条同 name 记录
            for (int i = _explicitEntries.Count - 1; i >= 0; --i)
            {
                var entry = _explicitEntries[i];
                if (entry.name != name) continue;

                if (entry.element != null)
                    entry.element.UnregisterCallback(entry.callback);
                entry.luaCallback?.Dispose();
                _explicitEntries.RemoveAt(i);
            }
        }

        // ── 元素查找 ─────────────────────────────────────────────────────

        /// <summary>
        /// 查找 VisualElement，供 Lua 直接操作。
        /// 返回 VisualElement（xLua 绑定后 Lua 可访问其属性）。
        /// 
        /// Lua 使用示例：
        ///   local el = self.m_bridge:Q("lbl_score")
        /// </summary>
        public VisualElement Q(string name)
        {
            return _doc?.rootVisualElement.Q(name);
        }

        /// <summary>按类型查找，类型名传字符串（"Label"/"Button"/"TextField" 等）</summary>
        public VisualElement QByType(string name, string typeName)
        {
            if (_doc == null) return null;
            return typeName switch
            {
                "Label"     => _doc.rootVisualElement.Q<Label>(name),
                "Button"    => _doc.rootVisualElement.Q<Button>(name),
                "TextField" => _doc.rootVisualElement.Q<TextField>(name),
                "Toggle"    => _doc.rootVisualElement.Q<Toggle>(name),
                "Slider"    => _doc.rootVisualElement.Q<Slider>(name),
                _           => _doc.rootVisualElement.Q(name),
            };
        }

        // ── 快捷属性方法（避免 Lua 直接操作 VisualElement 属性绑定问题）───

        /// <summary>设置 Label 或 Button 文本</summary>
        public void SetText(string name, string text)
        {
            if (_doc == null) return;
            var el = _doc.rootVisualElement.Q(name);
            if      (el is Label     lbl) lbl.text = text;
            else if (el is Button    btn) btn.text = text;
            else if (el is TextField tf)  tf.value = text;
            else Debug.LogWarning($"[UITKLuaBridge] SetText: '{name}' is not a text element");
        }

        /// <summary>获取 Label 或 TextField 文本</summary>
        public string GetText(string name)
        {
            if (_doc == null) return "";
            var el = _doc.rootVisualElement.Q(name);
            return el switch
            {
                Label     lbl => lbl.text,
                Button    btn => btn.text,
                TextField tf  => tf.value,
                _             => ""
            };
        }

        /// <summary>控制元素显隐（Flex / None）</summary>
        public void SetDisplay(string name, bool visible)
        {
            if (_doc == null) return;
            var el = _doc.rootVisualElement.Q(name);
            if (el != null)
                el.style.display = visible ? DisplayStyle.Flex : DisplayStyle.None;
            else
                Debug.LogWarning($"[UITKLuaBridge] SetDisplay: element '{name}' not found");
        }

        /// <summary>控制元素可见性（Visible / Hidden，保留布局空间）</summary>
        public void SetVisibility(string name, bool visible)
        {
            if (_doc == null) return;
            var el = _doc.rootVisualElement.Q(name);
            if (el != null)
                el.style.visibility = visible ? Visibility.Visible : Visibility.Hidden;
        }

        /// <summary>添加/移除 USS 类名</summary>
        public void AddClass(string name, string className)
        {
            _doc?.rootVisualElement.Q(name)?.AddToClassList(className);
        }

        public void RemoveClass(string name, string className)
        {
            _doc?.rootVisualElement.Q(name)?.RemoveFromClassList(className);
        }

        // ── 清理 ─────────────────────────────────────────────────────────

        /// <summary>
        /// 清理全部事件（全局扫描 + 所有精确绑定），释放所有 LuaFunction。
        /// 面板销毁时由 FPanelBaseUI:DestroyPanelRaw 自动调用。
        /// 等价于 UGUI 的 UILuaBehaviour.UnTouchGUIMsg + ClearClick。
        /// </summary>
        public void ClearAllListeners()
        {
            // 全局扫描
            ClearGlobalScan();

            // 精确绑定
            foreach (var entry in _explicitEntries)
            {
                if (entry.element != null)
                    entry.element.UnregisterCallback(entry.callback);
                entry.luaCallback?.Dispose();
            }
            _explicitEntries.Clear();
        }

        private void OnDestroy()
        {
            ClearAllListeners();
        }
    }
}
