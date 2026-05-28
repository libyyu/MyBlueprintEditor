// UITKLuaBridge.cs
// UI Toolkit 版事件桥接器，等价于 UGUI 的 UILuaBehaviour
//
// 功能：
//   1. TouchAllButtons(msgHandler) —— 扫描所有 Button 元素，统一路由到 OnClick(name)
//      完全等价于 UILuaBehaviour.TouchButton()，Lua 侧 OnClick 参数从 GameObject 变为 name string
//   2. RegisterClick(name, func)   —— 精确绑定单个元素点击
//   3. Q(name)                     —— 查找 VisualElement，供 Lua 直接操作
//   4. SetText(name, text)         —— 快捷设置 Label/Button 文本
//   5. SetDisplay(name, visible)   —— 快捷控制元素显隐
//   6. ClearAllListeners()         —— 清理全部事件（面板销毁时自动调用）

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
        private struct ListenerEntry
        {
            public VisualElement element;
            public EventCallback<ClickEvent> callback;
        }

        private UIDocument _doc;
        private readonly List<ListenerEntry> _listeners = new List<ListenerEntry>();
        private LuaFunction _globalOnClick;   // TouchAllButtons 模式的全局回调

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
        /// Lua 使用示例：
        ///   -- FPanelBaseUI 在检测到 UITK 后端时自动调用
        ///   function FPanelXxx:OnClick(name)
        ///       if name == "btn_start" then self:OnClickStart() end
        ///   end
        /// </summary>
        public void TouchAllButtons(LuaTable msgHandler)
        {
            ClearAllListeners();
            _globalOnClick = msgHandler?.Get<LuaFunction>("onClick");

            if (_globalOnClick == null || _doc == null) return;

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
                _listeners.Add(new ListenerEntry { element = btn, callback = cb });
            }
        }

        // ── 模式二：精确绑定（等价 btn.onClick.AddListener）──────────────

        /// <summary>
        /// 精确绑定单个元素点击事件。
        /// 
        /// Lua 使用示例：
        ///   self.m_bridge:RegisterClick("btn_start", function(name) self:OnClickStart() end)
        /// </summary>
        public void RegisterClick(string name, LuaFunction callback)
        {
            if (_doc == null || callback == null) return;
            var el = _doc.rootVisualElement.Q(name);
            if (el == null)
            {
                Debug.LogWarning($"[UITKLuaBridge] RegisterClick: element '{name}' not found");
                return;
            }

            EventCallback<ClickEvent> cb = _ => callback.Call(name);
            el.RegisterCallback(cb);
            _listeners.Add(new ListenerEntry { element = el, callback = cb });
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

        public void ClearAllListeners()
        {
            foreach (var entry in _listeners)
            {
                if (entry.element != null)
                    entry.element.UnregisterCallback(entry.callback);
            }
            _listeners.Clear();

            _globalOnClick?.Dispose();
            _globalOnClick = null;
        }

        private void OnDestroy()
        {
            ClearAllListeners();
        }
    }
}
