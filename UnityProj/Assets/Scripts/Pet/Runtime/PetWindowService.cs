// PetWindowService.cs
// 桌宠窗口服务 —— 薄 C# 引擎层（C# 只暴露引擎能力，不含业务逻辑）。
//
// 职责：把"透明 / 无边框 / 置顶 / 点击穿透 / 移动窗口"等平台原生能力
//       封装成跨平台接口，并通过 [LuaCallCSharp] 暴露给 Lua / 蓝图调用。
//
// 平台策略：
//   Windows  → Win32 + DWM 实现（当前已实现）
//   macOS    → 预留（NSWindow，后续接入）
//   Android  → 不适用窗口透明，预留为空实现（桌宠以悬浮窗/全屏 Overlay 形态另做）
//
// Lua 侧用法：
//   local win = CS.Pet.Runtime.PetWindowService.Instance
//   win:SetTopmost(true)
//   win:SetClickThrough(false)
//   win:MoveWindow(100, 100)

using System;
using System.Runtime.InteropServices;
using UnityEngine;
using XLua;

namespace Pet.Runtime
{
    /// <summary>
    /// 桌宠窗口服务。单例，跨场景常驻。
    /// 仅暴露窗口相关的原生能力，所有"何时调用"的决策交给 Lua / 蓝图。
    /// </summary>
    [LuaCallCSharp]
    [DefaultExecutionOrder(-90)] // 晚于 PetLauncher(-100)，早于普通逻辑
    public class PetWindowService : MonoBehaviour
    {
        public static PetWindowService Instance { get; private set; }

        [Header("透明色键 (RRGGBB，背景将被抠成透明)")]
        [Tooltip("Windows 下作为 LWA_COLORKEY，需与相机 Clear 背景色一致")]
        public Color32 colorKey = new Color32(1, 1, 1, 255); // #010101

        [Header("启动即应用透明置顶窗口 (仅打包后生效)")]
        public bool applyOnStart = true;

        /// <summary>窗口能力是否在当前平台可用（仅 Windows 独立运行可用）。</summary>
        public bool IsSupported
        {
            get
            {
#if UNITY_STANDALONE_WIN && !UNITY_EDITOR
                return true;
#else
                return false;
#endif
            }
        }

        private void Awake()
        {
            if (Instance != null && Instance != this)
            {
                Destroy(gameObject);
                return;
            }
            Instance = this;
            DontDestroyOnLoad(gameObject);
        }

        private void Start()
        {
            if (applyOnStart)
                ApplyTransparentWindow();
        }

        private void OnDestroy()
        {
            if (Instance == this) Instance = null;
        }

        // =====================================================================
        // 公共 API（Lua / 蓝图调用入口）
        // =====================================================================

        /// <summary>应用"透明 + 无边框 + 置顶"窗口样式。打包后调用一次即可。</summary>
        public void ApplyTransparentWindow()
        {
#if UNITY_STANDALONE_WIN && !UNITY_EDITOR
            Win.ApplyTransparent(colorKey);
#else
            Debug.Log("[PetWindow] ApplyTransparentWindow skipped (unsupported platform / editor)");
#endif
        }

        /// <summary>设置窗口是否置顶。</summary>
        public void SetTopmost(bool topmost)
        {
#if UNITY_STANDALONE_WIN && !UNITY_EDITOR
            Win.SetTopmost(topmost);
#endif
        }

        /// <summary>
        /// 设置点击穿透：true = 鼠标事件穿过桌宠落到桌面 / 下层窗口。
        /// 用于桌宠"假装不挡路"，或在交互时临时关闭穿透。
        /// </summary>
        public void SetClickThrough(bool through)
        {
#if UNITY_STANDALONE_WIN && !UNITY_EDITOR
            Win.SetClickThrough(through);
#endif
        }

        /// <summary>移动窗口左上角到屏幕坐标 (x, y)。</summary>
        public void MoveWindow(int x, int y)
        {
#if UNITY_STANDALONE_WIN && !UNITY_EDITOR
            Win.MoveWindow(x, y);
#endif
        }

        /// <summary>获取主显示器分辨率（宽）。</summary>
        public int ScreenWidth => Display.main.systemWidth;

        /// <summary>获取主显示器分辨率（高）。</summary>
        public int ScreenHeight => Display.main.systemHeight;

        // =====================================================================
        // Windows 原生实现
        // =====================================================================
#if UNITY_STANDALONE_WIN && !UNITY_EDITOR
        private static class Win
        {
            [DllImport("user32.dll")] static extern IntPtr FindWindow(string cls, string name);
            [DllImport("user32.dll")] static extern bool SetWindowPos(IntPtr hWnd, IntPtr after, int x, int y, int cx, int cy, uint flags);
            [DllImport("user32.dll")] static extern int GetWindowLongPtrA(IntPtr hWnd, int idx);
            [DllImport("user32.dll")] static extern int SetWindowLongPtrA(IntPtr hWnd, int idx, uint val);
            [DllImport("user32.dll")] static extern int SetLayeredWindowAttributes(IntPtr hwnd, int crKey, int alpha, int flags);
            [DllImport("Dwmapi.dll")] static extern uint DwmExtendFrameIntoClientArea(IntPtr hWnd, ref MARGINS m);

            struct MARGINS { public int l, r, t, b; }

            static readonly IntPtr HWND_TOPMOST = new IntPtr(-1);
            static readonly IntPtr HWND_NOTOPMOST = new IntPtr(-2);
            const uint SWP_NOSIZE = 0x0001, SWP_NOMOVE = 0x0002, SWP_NOACTIVATE = 0x0010;
            const int GWL_EXSTYLE = -20, GWL_STYLE = -16;
            const int WS_EX_LAYERED = 0x00080000, WS_EX_TRANSPARENT = 0x20, WS_EX_TOOLWINDOW = 0x00000080;
            const int WS_EX_ACCEPTFILES = 0x00000010;
            const int WS_BORDER = 0x00800000, WS_CAPTION = 0x00C00000;
            const int LWA_COLORKEY = 0x00000001;

            static IntPtr _hwnd = IntPtr.Zero;
            static int _baseExStyle;

            static IntPtr Hwnd()
            {
                if (_hwnd == IntPtr.Zero)
                    _hwnd = FindWindow(null, Application.productName);
                return _hwnd;
            }

            public static void ApplyTransparent(Color32 key)
            {
                var h = Hwnd();
                if (h == IntPtr.Zero) { Debug.LogWarning("[PetWindow] window handle not found"); return; }

                SetWindowPos(h, HWND_TOPMOST, Screen.mainWindowPosition.x, Screen.mainWindowPosition.y,
                    Screen.width, Screen.height, SWP_NOMOVE | SWP_NOSIZE);

                int ex = GetWindowLongPtrA(h, GWL_EXSTYLE);
                int st = GetWindowLongPtrA(h, GWL_STYLE);
                _baseExStyle = ex | WS_EX_LAYERED | WS_EX_ACCEPTFILES | WS_EX_TOOLWINDOW;
                SetWindowLongPtrA(h, GWL_EXSTYLE, (uint)_baseExStyle);
                SetWindowLongPtrA(h, GWL_STYLE, (uint)(st & ~WS_BORDER & ~WS_CAPTION));

                var m = new MARGINS { l = -1 };
                DwmExtendFrameIntoClientArea(h, ref m);

                int crKey = key.r | (key.g << 8) | (key.b << 16);
                SetLayeredWindowAttributes(h, crKey, 255, LWA_COLORKEY);
            }

            public static void SetTopmost(bool topmost)
            {
                var h = Hwnd();
                if (h == IntPtr.Zero) return;
                SetWindowPos(h, topmost ? HWND_TOPMOST : HWND_NOTOPMOST, 0, 0, 0, 0,
                    SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
            }

            public static void SetClickThrough(bool through)
            {
                var h = Hwnd();
                if (h == IntPtr.Zero) return;
                int ex = through ? (_baseExStyle | WS_EX_TRANSPARENT)
                                 : (_baseExStyle & ~WS_EX_TRANSPARENT);
                SetWindowLongPtrA(h, GWL_EXSTYLE, (uint)ex);
            }

            public static void MoveWindow(int x, int y)
            {
                var h = Hwnd();
                if (h == IntPtr.Zero) return;
                SetWindowPos(h, HWND_TOPMOST, x, y, 0, 0, SWP_NOSIZE | SWP_NOACTIVATE);
            }
        }
#endif
    }
}
