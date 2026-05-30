// IUIPanelBackend.cs
// UI 面板后端抽象接口 —— 让 Lua UI 框架同时支持 UGUI Prefab 和 UI Toolkit
//
// 实现类：
//   UGUIPanelBackend  —— 封装现有 GameObject + Canvas 流程
//   UITKPanelBackend  —— 封装 UIDocument + VisualElement 流程

using UnityEngine;
using XLua;

namespace UGFramework.Runtime
{
    /// <summary>
    /// UI 面板后端统一接口。
    /// Lua 框架层通过此接口操作面板，不感知底层是 UGUI 还是 UI Toolkit。
    /// </summary>
    [LuaCallCSharp]
    public interface IUIPanelBackend
    {
        /// <summary>是否为 UI Toolkit 后端</summary>
        bool IsUIToolkit { get; }

        /// <summary>面板根 GameObject（UGUI: Prefab 实例; UITK: UIDocument 宿主 GO）</summary>
        GameObject RootGameObject { get; }

        /// <summary>面板是否有效（已加载且未销毁）</summary>
        bool IsValid { get; }

        /// <summary>显隐控制</summary>
        void SetVisible(bool visible);
        bool GetVisible();

        /// <summary>Canvas 层级排序（UGUI: Canvas.sortingOrder; UITK: UIDocument.sortingOrder）</summary>
        void SetSortingOrder(int order);

        /// <summary>
        /// 获取后端事件桥接器（供 Lua 访问）。
        /// UGUI 后端返回 UILuaBehaviour；UITK 后端返回 UITKLuaBridge。
        /// Lua 侧通过 FPanelBaseUI.m_bridge 访问，无需关心具体类型。
        /// </summary>
        object GetEventBridge();

        /// <summary>销毁面板</summary>
        void Destroy();
    }
}
