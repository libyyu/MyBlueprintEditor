// IUIPanelBridge.cs
// UI Panel 操作桥接接口 —— 让 Lua 通过统一 API 操作 UGUI / UITK 元素
//
// 实现类：
//   UILuaBehaviour  —— UGUI 后端（路径式寻址：root:Find(name)）
//   UITKLuaBridge   —— UITK 后端（扁平寻址：rootVE.Q(name)）
//
// 寻址规则：
//   - UGUI: name 直接传给 transform.Find，支持 "TopBar/lbl_score" 这种路径
//   - UITK: name 是 VisualElement.name，扁平命名；如果传入路径，自动取最后一段
//   两侧均带 name -> element 缓存，重复访问 O(1)。
//
// 设计原则：
//   - 公共方法名 / 参数顺序完全一致 —— Lua 调用代码后端无关
//   - 类型自适配 —— SetText 自动识别 TMP_Text / UI.Text / Label / Button
//   - 失败静默 + 警告日志 —— 元素不存在不抛异常，方便 Lua 端鲁棒性

using System;
using UnityEngine;
using XLua;

namespace CutRope.Framework
{
    /// <summary>
    /// Panel UI 操作统一接口。
    /// Lua 通过 self.m_bridge:XXX(name, ...) 调用，无需关心后端。
    /// </summary>
    [LuaCallCSharp]
    public interface IUIPanelBridge
    {
        // ─── 元素查找 ────────────────────────────────────────────────────
        // 返回原生元素：UGUI = GameObject，UITK = VisualElement
        // Lua 收到后可继续走 xLua 直接操作（高级用法）
        object Q(string name);

        // ─── 文本 ────────────────────────────────────────────────────────
        // 自适配：UGUI 识别 TMP_Text / UI.Text；UITK 识别 Label / Button / TextField
        void   SetText(string name, string text);
        string GetText(string name);

        // ─── 显隐 / 启用 ─────────────────────────────────────────────────
        // SetDisplay: 完全隐藏，不占布局空间
        //   UGUI = GameObject.SetActive  ；UITK = display Flex/None
        // SetVisibility: 隐藏但保留布局
        //   UGUI = CanvasGroup.alpha 或 Image.enabled；UITK = visibility Visible/Hidden
        // SetEnabled: 是否可交互
        //   UGUI = Selectable.interactable；UITK = VisualElement.SetEnabled
        void SetDisplay   (string name, bool visible);
        void SetVisibility(string name, bool visible);
        void SetEnabled   (string name, bool enabled);

        // ─── 视觉 ────────────────────────────────────────────────────────
        // SetColor: 颜色 (rgba 0-1)
        //   UGUI = Image.color / Graphic.color / TMP_Text.color
        //   UITK = style.color（Label/Button 文本色）/ style.backgroundColor (其他)
        // SetSprite: UGUI 直传 Sprite；UITK 通过 Texture/Sprite 转 backgroundImage
        void SetColor (string name, float r, float g, float b, float a);
        void SetSprite(string name, Sprite sprite);

        // ─── 表单控件 ────────────────────────────────────────────────────
        // Slider: UGUI = UI.Slider.value；UITK = Slider.value（都是 float）
        // Toggle: UGUI = Toggle.isOn；UITK = Toggle.value（都是 bool）
        void  SetSliderValue(string name, float value);
        float GetSliderValue(string name);
        void  SetToggleValue(string name, bool value);
        bool  GetToggleValue(string name);

        // ─── 事件 ────────────────────────────────────────────────────────
        // 累加语义：同 name 多次 RegisterClick 全部触发，对齐 UGUI AddClick
        // 回调签名：function(name) end —— name 是触发的元素名
        void RegisterClick  (string name, LuaFunction callback);
        void UnregisterClick(string name);

        // ─── 清理 ────────────────────────────────────────────────────────
        // Panel 销毁时调用，释放所有 LuaFunction、卸载所有 listener
        void ClearAllListeners();
    }
}
