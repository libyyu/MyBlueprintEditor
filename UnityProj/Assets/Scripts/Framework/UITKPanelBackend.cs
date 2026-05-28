// UITKPanelBackend.cs
// IUIPanelBackend 的 UI Toolkit 实现
//
// 工作流程：
//   1. 由 FPanelLoader 在加载 .uxml 资源后创建此 Backend
//   2. 创建一个宿主 GameObject，挂 UIDocument 组件
//   3. 将 VisualTreeAsset clone 到 UIDocument
//   4. 挂 UITKLuaBridge 作为事件桥接器

using UnityEngine;
using UnityEngine.UIElements;
using XLua;

namespace CutRope.Framework
{
    [LuaCallCSharp]
    public class UITKPanelBackend : IUIPanelBackend
    {
        private readonly GameObject   _hostGO;
        private readonly UIDocument   _doc;
        private readonly UITKLuaBridge _bridge;

        /// <summary>
        /// 构造：传入已经创建好的宿主 GO（挂有 UIDocument + UITKLuaBridge）
        /// 由 FPanelLoader 的 UITK 加载分支调用
        /// </summary>
        public UITKPanelBackend(GameObject hostGO, UIDocument doc, UITKLuaBridge bridge)
        {
            _hostGO = hostGO;
            _doc    = doc;
            _bridge = bridge;
        }

        // ── IUIPanelBackend ──────────────────────────────────────────────

        public bool IsUIToolkit => true;

        public GameObject RootGameObject => _hostGO;

        public bool IsValid => _hostGO != null && !_hostGO.Equals(null);

        public void SetVisible(bool visible)
        {
            if (!IsValid) return;
            // UIDocument 用 rootVisualElement 的 display 控制显隐
            if (_doc != null && _doc.rootVisualElement != null)
                _doc.rootVisualElement.style.display = visible
                    ? DisplayStyle.Flex
                    : DisplayStyle.None;
            // 同步 GO active，保持与 UGUI 行为一致（用于 IsValid 检查等）
            _hostGO.SetActive(visible);
        }

        public bool GetVisible()
        {
            if (!IsValid) return false;
            if (_doc != null && _doc.rootVisualElement != null)
                return _doc.rootVisualElement.style.display != DisplayStyle.None;
            return _hostGO.activeSelf;
        }

        public void SetSortingOrder(int order)
        {
            if (!IsValid || _doc == null) return;
            _doc.sortingOrder = order;
        }

        /// <summary>返回 UITKLuaBridge，供 Lua 的 FPanelBaseUI.m_bridge 使用</summary>
        public object GetEventBridge() => _bridge;

        public void Destroy()
        {
            if (IsValid)
                Object.Destroy(_hostGO);
        }

        // ── UITK 专属 ────────────────────────────────────────────────────

        /// <summary>直接暴露 UIDocument，供高级用法</summary>
        public UIDocument Document => _doc;

        /// <summary>直接暴露 rootVisualElement</summary>
        public VisualElement RootVisualElement => _doc?.rootVisualElement;

        // ── 工厂方法：由 FPanelLoader 调用 ──────────────────────────────

        /// <summary>
        /// 从 VisualTreeAsset 创建 UITKPanelBackend。
        /// panelSettings 优先级：外部传入 > GameLauncher.Instance 自动选择 > null（Unity 默认）。
        /// sortingOrder >= 90000 自动选择 uitkOverlaySettings，其余选 uitkGameSettings。
        /// </summary>
        public static UITKPanelBackend Create(
            VisualTreeAsset vta,
            PanelSettings   panelSettings,
            Transform       parentTransform,
            string          panelName,
            int             sortingOrder = 0)
        {
            // 自动选择 PanelSettings
            if (panelSettings == null && GameLauncher.Instance != null)
                panelSettings = GameLauncher.Instance.GetPanelSettingsForOrder(sortingOrder);

            // 1. 创建宿主 GO
            var hostGO = new GameObject($"UITK_{panelName}");
            hostGO.layer = LayerMask.NameToLayer("UI");
            if (parentTransform != null)
                hostGO.transform.SetParent(parentTransform, false);

            // 2. 挂 UIDocument
            var doc = hostGO.AddComponent<UIDocument>();
            if (panelSettings != null)
                doc.panelSettings = panelSettings;
            doc.visualTreeAsset = vta;

            // 3. 挂桥接器
            var bridge = hostGO.AddComponent<UITKLuaBridge>();
            bridge.Init(doc);

            return new UITKPanelBackend(hostGO, doc, bridge);
        }
    }
}
