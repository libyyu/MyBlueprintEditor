// UGUIPanelBackend.cs
// IUIPanelBackend 的 UGUI/Prefab 实现
// 封装现有 GameObject + Canvas.sortingOrder 逻辑，行为与原来完全一致

using UnityEngine;
using XLua;

namespace UGFramework.Runtime
{
    [LuaCallCSharp]
    public class UGUIPanelBackend : IUIPanelBackend
    {
        private readonly GameObject _root;
        private UILuaBehaviour _bridge;

        public UGUIPanelBackend(GameObject root)
        {
            _root = root;
        }

        // ── IUIPanelBackend ──────────────────────────────────────────────

        public bool IsUIToolkit => false;

        public GameObject RootGameObject => _root;

        public bool IsValid => _root != null && !_root.Equals(null);

        public void SetVisible(bool visible)
        {
            if (IsValid)
                _root.SetActive(visible);
        }

        public bool GetVisible()
        {
            return IsValid && _root.activeSelf;
        }

        public void SetSortingOrder(int order)
        {
            if (!IsValid) return;
            var canvas = _root.GetComponent<Canvas>();
            if (canvas != null)
            {
                canvas.overrideSorting = true;
                canvas.sortingOrder    = order;
            }
        }

        /// <summary>
        /// 返回或懒创建 UILuaBehaviour。
        /// Lua 侧通过 m_bridge 拿到这个对象后可调用 TouchGUIMsg / AddClick 等。
        /// </summary>
        public object GetEventBridge()
        {
            if (!IsValid) return null;
            if (_bridge == null || _bridge.Equals(null))
            {
                _bridge = _root.GetComponent<UILuaBehaviour>()
                       ?? _root.AddComponent<UILuaBehaviour>();
            }
            return _bridge;
        }

        public void Destroy()
        {
            if (IsValid)
                Object.Destroy(_root);
        }

        public static UGUIPanelBackend Create(GameObject go)
        {
            return new UGUIPanelBackend(go);
        }
    }
}
