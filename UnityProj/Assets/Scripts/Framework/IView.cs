// IView.cs
// 所有游戏 UI 面板的基础接口
//
// 设计原则：
//   - UI 面板通过 YooAsset 加载 Prefab 实例化
//   - Lua 层通过 LuaCallCSharp 绑定控制 UI 逻辑
//   - Show/Hide 做动画状态切换，Dispose 才真正销毁

using System;
using UnityEngine;

namespace CutRope.Framework
{
    public interface IView
    {
        /// <summary>显示面板（可附带参数 JSON 字符串）</summary>
        void Show(string param = null);

        /// <summary>隐藏面板（不销毁，保留状态）</summary>
        void Hide();

        /// <summary>销毁面板（释放资源）</summary>
        void Dispose();

        /// <summary>面板是否可见</summary>
        bool IsVisible { get; }

        /// <summary>面板 GameObject</summary>
        GameObject ViewObject { get; }
    }
}
