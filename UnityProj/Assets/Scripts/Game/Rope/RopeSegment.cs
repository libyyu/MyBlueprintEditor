// RopeSegment.cs
// 绳子节点组件 — 每个节点是一个带 Rigidbody2D 的小球，相邻节点通过 HingeJoint2D 连接
//
// 使用方式：
//   由 RopeSpawner 批量生成，不需要手动挂载。
//   切断时调用 RopeSegment.Cut(segmentIndex)，从该节点断开连接。

using UnityEngine;

namespace CutRope.Game
{
    [RequireComponent(typeof(Rigidbody2D))]
    public class RopeSegment : MonoBehaviour
    {
        // ── 引用（由 RopeSpawner 注入）───────────────────────────────
        public RopeSpawner Owner      { get; set; }
        public int         Index      { get; set; }
        public RopeSegment NextSegment { get; set; }  // 链表：指向下一节

        private HingeJoint2D _joint;
        public  Rigidbody2D  Rb    { get; private set; }
        /// <summary>预先缓存的 HingeJoint2D，供 RopeSpawner 配置使用。</summary>
        public  HingeJoint2D Joint { get; private set; }

        private void Awake()
        {
            Rb     = GetComponent<Rigidbody2D>();
            // 优先从 prefab 取；若 prefab 漏配则运行时补加，保证不为 null
            _joint = GetComponent<HingeJoint2D>() ?? gameObject.AddComponent<HingeJoint2D>();
            Joint  = _joint;
        }

        /// <summary>断开与上一节的连接（销毁关节）</summary>
        public void BreakJoint()
        {
            if (_joint != null)
            {
                Destroy(_joint);
                _joint = null;
            }
        }

        private void OnTriggerEnter2D(Collider2D other)
        {
            // 通知绳子所有者（用于气泡/星星收集等交互）
            Owner?.OnSegmentTrigger(this, other);
        }
    }
}
