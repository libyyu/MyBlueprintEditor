// RopeSpawner.cs
// 绳子生成器 — 在运行时创建一条由多个 RopeSegment 节点组成的绳子
//
// 挂载到绳子锚点 GameObject 上，Inspector 配置节点数量、长度、质量等。
// 绳子末端自动连接到 Candy（糖果）对象。
//
// 关键方法：
//   Spawn(candy)   — 生成绳子，末端挂到 candy
//   Cut(index)     — 在第 index 节点处切断绳子
//   CutAtWorld(p)  — 在世界坐标 p 最近处切断（供 CutInput 调用）

using System.Collections.Generic;
using UnityEngine;
using XLua;

namespace CutRope.Game
{
    [LuaCallCSharp]
    public class RopeSpawner : MonoBehaviour
    {
        [Header("绳子参数")]
        [Tooltip("节点数量（越多越流畅，越耗性能）")]
        public int   segmentCount  = 12;
        [Tooltip("两节点间距（米）")]
        public float segmentLength = 0.15f;
        [Tooltip("节点质量")]
        public float segmentMass   = 0.05f;

        [Header("预制体")]
        [Tooltip("RopeSegment prefab（带 Rigidbody2D + HingeJoint2D + CircleCollider2D）")]
        public GameObject segmentPrefab;

        [Header("渲染")]
        [Tooltip("用于绘制绳子的 LineRenderer（可为 null，此时用 Gizmos 调试）")]
        public LineRenderer lineRenderer;

        // ── 运行时状态 ────────────────────────────────────────────────
        private readonly List<RopeSegment> _segments = new List<RopeSegment>();
        private Candy _candy;
        public  bool  IsAlive => _segments.Count > 0;

        // ── 事件（Lua 可注入） ────────────────────────────────────────
        public System.Action<RopeSpawner> OnRopeCut;  // 任意节点被切断时

        // ── 公共 API ─────────────────────────────────────────────────

        /// <summary>生成绳子，末端连接到 candy</summary>
        public void Spawn(Candy candy)
        {
            if (segmentPrefab == null)
            {
                Debug.LogError("[RopeSpawner] segmentPrefab is null");
                return;
            }

            _candy = candy;
            Vector3 pos = transform.position;

            for (int i = 0; i < segmentCount; i++)
            {
                var go  = Instantiate(segmentPrefab, pos, Quaternion.identity, transform);
                var seg = go.GetComponent<RopeSegment>();
                seg.Owner = this;
                seg.Index = i;
                if (i > 0) seg.NextSegment = _segments[i - 1];

                var rb = seg.Rb;
                rb.mass = segmentMass;

                // HingeJoint2D：连到上一节或锚点
                var joint = go.GetComponent<HingeJoint2D>() ?? go.AddComponent<HingeJoint2D>();
                if (i == 0)
                {
                    // 第0节：connectedBody 为 null，通过 connectedAnchor 锁到世界锚点
                    joint.connectedBody   = null;
                    joint.anchor          = Vector2.zero;
                    joint.connectedAnchor = transform.position;  // 世界坐标锚点
                }
                else
                {
                    joint.connectedBody   = _segments[i - 1].Rb;
                    joint.anchor          = Vector2.zero;
                    joint.connectedAnchor = Vector2.zero;
                }

                _segments.Add(seg);
                pos.y -= segmentLength;
            }

            // 末端连接糖果
            if (candy != null)
            {
                var lastSeg   = _segments[_segments.Count - 1];
                var candyJoint = candy.gameObject.GetComponent<HingeJoint2D>()
                              ?? candy.gameObject.AddComponent<HingeJoint2D>();
                candyJoint.connectedBody = lastSeg.Rb;
                candyJoint.anchor        = Vector2.zero;
                candyJoint.connectedAnchor = Vector2.zero;
            }

            UpdateLineRenderer();
        }

        /// <summary>在第 index 节点处切断绳子</summary>
        public void Cut(int index)
        {
            if (index < 0 || index >= _segments.Count) return;
            _segments[index].BreakJoint();
            OnRopeCut?.Invoke(this);
            Debug.Log($"[RopeSpawner] Cut at index {index}");
        }

        /// <summary>在世界坐标最近处切断（CutInput 调用）</summary>
        public void CutAtWorld(Vector2 worldPos)
        {
            int    nearest = -1;
            float  minDist = float.MaxValue;

            for (int i = 0; i < _segments.Count; i++)
            {
                float d = Vector2.Distance(worldPos, _segments[i].transform.position);
                if (d < minDist) { minDist = d; nearest = i; }
            }

            // 只切割距离切割点足够近的节点（避免误触）
            if (nearest >= 0 && minDist < segmentLength * 2f)
                Cut(nearest);
        }

        // ── LineRenderer 更新 ─────────────────────────────────────────
        private void LateUpdate()
        {
            if (_segments.Count > 0 && lineRenderer != null)
                UpdateLineRenderer();
        }

        private void UpdateLineRenderer()
        {
            if (lineRenderer == null || _segments.Count == 0) return;

            var points = new Vector3[_segments.Count + (_candy ? 1 : 0)];
            for (int i = 0; i < _segments.Count; i++)
                points[i] = _segments[i].transform.position;
            if (_candy)
                points[_segments.Count] = _candy.transform.position;

            lineRenderer.positionCount = points.Length;
            lineRenderer.SetPositions(points);
        }

        // ── 节点触发回调（来自 RopeSegment）────────────────────────────
        public void OnSegmentTrigger(RopeSegment seg, Collider2D other)
        {
            // 气泡/星星等道具交互，后续扩展
        }

        private void OnDestroy()
        {
            _segments.Clear();
        }
    }
}
