// InteractionPrompt.cs
// ─────────────────────────────────────────────────────────────────────────────
// 开放世界风 "按 E 对话" 屏幕提示 UI
//
// 特性：
//   - 自动收集场景中所有 NpcProximityTrigger
//   - 每帧判断哪个 NPC 最近（进入 interact 范围且视野最前）
//   - 显示 "按 E 与 [NPC 名] 对话"
//   - 玩家按 E → 打开对话 UI（自行实现回调）
//
// 使用：
//   - 把本脚本挂到 Canvas 下的一个 Panel（含 TMP_Text 提示）
//   - 绑定 hintText（TMP_Text）
//   - 订阅 OnInteractPressed 或 onInteract UnityEvent
// ─────────────────────────────────────────────────────────────────────────────

using System.Collections.Generic;
using UnityEngine;
using UnityEngine.Events;
using TMPro;

namespace BlueprintRuntime.Samples.AINpc.OpenWorld
{
    public class InteractionPrompt : MonoBehaviour
    {
        [Header("UI")]
        [SerializeField] private GameObject panel;
        [SerializeField] private TMP_Text   hintText;

        [Header("玩家输入")]
        [SerializeField] private KeyCode interactKey = KeyCode.E;
        [SerializeField] private string  promptFormat = "按 <color=#FFCC00>[{0}]</color> 与 <b>{1}</b> 对话";

        [Header("事件")]
        /// <summary>玩家按键交互时触发（参数：当前最近的 NPC Trigger）</summary>
        public UnityEvent<NpcProximityTrigger> onInteract;

        private readonly List<NpcProximityTrigger> _triggers = new List<NpcProximityTrigger>();
        private NpcProximityTrigger _currentNearest;
        private Transform _player;

        void Start()
        {
            _triggers.AddRange(FindObjectsOfType<NpcProximityTrigger>());
            var pgo = GameObject.FindGameObjectWithTag("Player");
            if (pgo != null) _player = pgo.transform;

            if (panel != null) panel.SetActive(false);
        }

        void Update()
        {
            if (_player == null) return;

            // 找最近的 "在 interact 范围内" 的 NPC
            NpcProximityTrigger nearest = null;
            float nearestDist = float.MaxValue;
            foreach (var t in _triggers)
            {
                if (t == null || !t.IsPlayerInInteract) continue;
                float d = Vector3.Distance(t.transform.position, _player.position);
                if (d < nearestDist) { nearestDist = d; nearest = t; }
            }

            // 切换 UI
            if (nearest != _currentNearest)
            {
                _currentNearest = nearest;
                if (panel != null) panel.SetActive(nearest != null);
                if (hintText != null && nearest != null)
                    hintText.text = string.Format(promptFormat, interactKey.ToString(), GetNpcName(nearest));
            }

            if (_currentNearest != null && Input.GetKeyDown(interactKey))
            {
                onInteract?.Invoke(_currentNearest);
            }
        }

        private static string GetNpcName(NpcProximityTrigger t)
        {
            var b = t.GetComponent<AINpcController>();
            if (b != null) return b.NpcName;
            var s = t.GetComponent<AINpcStreamingController>();
            if (s != null) return s.NpcName;
            var e = t.GetComponent<EmotionalNpcController>();
            if (e != null) return e.NpcName;
            return t.gameObject.name;
        }

        /// <summary>手动添加/移除 Trigger（场景中动态生成 NPC 时调用）</summary>
        public void Register(NpcProximityTrigger t)
        {
            if (t != null && !_triggers.Contains(t)) _triggers.Add(t);
        }
        public void Unregister(NpcProximityTrigger t)
        {
            _triggers.Remove(t);
        }
    }
}
