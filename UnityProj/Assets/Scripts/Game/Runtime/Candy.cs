// Candy.cs
// 糖果组件 — 游戏目标物体，需要被喂给怪兽
//
// 生命周期：
//   1. 悬挂在绳子末端（由 RopeSpawner 连接）
//   2. 绳子切断后自由落体
//   3. 进入怪兽嘴巴触发器 → 调用 OnEaten()
//   4. 落入陷阱/超出边界 → 调用 OnFailed()
//
// Lua 事件钩子：
//   candy.OnEaten  = function() ... end
//   candy.OnFailed = function() ... end

using UnityEngine;
using XLua;

namespace CutRope.Game
{
    [LuaCallCSharp]
    [RequireComponent(typeof(Rigidbody2D))]
    [RequireComponent(typeof(CircleCollider2D))]
    public class Candy : MonoBehaviour
    {
        [Header("糖果设置")]
        public float outOfBoundsY = -10f;  // 低于此 Y 坐标视为出界

        // ── Lua 事件钩子 ─────────────────────────────────────────────
        public System.Action OnEaten  { get; set; }
        public System.Action OnFailed { get; set; }

        // ── 状态 ─────────────────────────────────────────────────────
        public bool IsEaten  { get; private set; }
        public bool IsFailed { get; private set; }

        private Rigidbody2D _rb;

        private void Awake()
        {
            _rb = GetComponent<Rigidbody2D>();
        }

        private void Update()
        {
            // 出界检测
            if (!IsEaten && !IsFailed && transform.position.y < outOfBoundsY)
            {
                TriggerFailed();
            }
        }

        private void OnTriggerEnter2D(Collider2D other)
        {
            if (IsEaten || IsFailed) return;

            if (other.CompareTag("Monster"))
            {
                TriggerEaten();
            }
            else if (other.CompareTag("Spike") || other.CompareTag("Trap"))
            {
                TriggerFailed();
            }
        }

        private void TriggerEaten()
        {
            if (IsEaten) return;
            IsEaten = true;
            Debug.Log("[Candy] Eaten!");
            OnEaten?.Invoke();
        }

        private void TriggerFailed()
        {
            if (IsFailed) return;
            IsFailed = true;
            Debug.Log("[Candy] Failed (out of bounds or trap)");
            OnFailed?.Invoke();
        }

        private void OnDestroy()
        {
            OnEaten  = null;
            OnFailed = null;
        }
    }
}
