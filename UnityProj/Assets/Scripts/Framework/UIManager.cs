// UIManager.cs
// UI 面板管理器 — 负责从 YooAsset 异步加载面板 Prefab、缓存、显示/隐藏/销毁
//
// 用法（Lua）：
//   local UI = require 'ui/ui_manager'
//   UI.open('MainMenu', nil, function(view) ... end)
//   UI.close('MainMenu')
//   UI.close_all()

using System;
using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using XLua;
using YooAsset;

namespace CutRope.Framework
{
    [LuaCallCSharp]
    public class UIManager : MonoBehaviour
    {
        // ── 单例 ─────────────────────────────────────────────────────
        public static UIManager Instance { get; private set; }

        [Header("UI 根节点（留空则自动创建）")]
        public Transform uiRoot;

        // 已加载的面板缓存：address → GameObject
        private readonly Dictionary<string, GameObject> _panels = new Dictionary<string, GameObject>();

        // ── 生命周期 ─────────────────────────────────────────────────
        private void Awake()
        {
            if (Instance != null && Instance != this) { Destroy(gameObject); return; }
            Instance = this;
            DontDestroyOnLoad(gameObject);

            if (uiRoot == null)
            {
                var go = new GameObject("UIRoot");
                DontDestroyOnLoad(go);
                uiRoot = go.transform;
            }
        }

        private void OnDestroy()
        {
            if (Instance == this) Instance = null;
        }

        // ── 公共 API ─────────────────────────────────────────────────

        /// <summary>
        /// 异步打开面板：加载 Prefab → 实例化 → 挂到 UIRoot → 调用 Show
        /// </summary>
        /// <param name="address">YooAsset address，如 "UI/MainMenu"</param>
        /// <param name="param">传给 IView.Show 的参数（JSON 字符串），可为 null</param>
        /// <param name="onComplete">完成回调，参数为 IView 实例</param>
        public void OpenPanel(string address, string param = null, Action<IView> onComplete = null)
        {
            StartCoroutine(OpenPanelCoroutine(address, param, onComplete));
        }

        /// <summary>隐藏面板（不销毁）</summary>
        public void ClosePanel(string address)
        {
            if (_panels.TryGetValue(address, out var go))
            {
                var view = go.GetComponent<IView>();
                view?.Hide();
            }
            else
            {
                Debug.LogWarning($"[UIManager] Panel not loaded: '{address}'");
            }
        }

        /// <summary>销毁并移除面板</summary>
        public void DisposePanel(string address)
        {
            if (_panels.TryGetValue(address, out var go))
            {
                var view = go.GetComponent<IView>();
                view?.Dispose();
                _panels.Remove(address);
            }
        }

        /// <summary>隐藏所有面板</summary>
        public void CloseAll()
        {
            foreach (var kv in _panels)
                kv.Value.GetComponent<IView>()?.Hide();
        }

        // ── Lua 绑定（静态） ─────────────────────────────────────────

        [CSharpCallLua]
        public delegate void LuaViewCallback(IView view);

        /// <summary>供 Lua 调用：OpenPanel</summary>
        public static void LuaOpen(string address, string param, LuaViewCallback onComplete)
        {
            if (Instance == null) { Debug.LogError("[UIManager] Instance is null"); return; }
            Instance.OpenPanel(address, param,
                onComplete != null ? (Action<IView>)(v => onComplete(v)) : null);
        }

        /// <summary>供 Lua 调用：ClosePanel</summary>
        public static void LuaClose(string address)
        {
            Instance?.ClosePanel(address);
        }

        /// <summary>供 Lua 调用：DisposePanel</summary>
        public static void LuaDispose(string address)
        {
            Instance?.DisposePanel(address);
        }

        /// <summary>供 Lua 调用：CloseAll</summary>
        public static void LuaCloseAll()
        {
            Instance?.CloseAll();
        }

        // ── 协程 ─────────────────────────────────────────────────────
        private IEnumerator OpenPanelCoroutine(string address, string param, Action<IView> onComplete)
        {
            GameObject go;

            // 已缓存则直接复用
            if (_panels.TryGetValue(address, out go))
            {
                var cachedView = go.GetComponent<IView>();
                cachedView?.Show(param);
                onComplete?.Invoke(cachedView);
                yield break;
            }

            // 从 YooAsset 加载
            var package = YooAssets.GetPackage("DefaultPackage");
            if (package == null)
            {
                Debug.LogError("[UIManager] DefaultPackage not found");
                yield break;
            }

            var handle = package.LoadAssetAsync<GameObject>(address);
            yield return handle;

            if (handle.Status != EOperationStatus.Succeed)
            {
                Debug.LogError($"[UIManager] Load panel failed '{address}': {handle.LastError}");
                yield break;
            }

            go = Instantiate(handle.AssetObject as GameObject, uiRoot);
            go.name = address.Replace("/", "_");
            _panels[address] = go;

            var view = go.GetComponent<IView>();
            if (view == null)
            {
                Debug.LogWarning($"[UIManager] Prefab '{address}' has no IView component. Add one.");
            }

            view?.Show(param);
            onComplete?.Invoke(view);
        }
    }
}
