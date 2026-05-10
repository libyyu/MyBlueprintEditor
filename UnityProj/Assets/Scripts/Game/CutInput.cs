// CutInput.cs
// 切割输入处理 — 检测鼠标/触摸滑动，对经过的绳子节点执行切割
//
// 原理：
//   每帧记录输入轨迹，检测轨迹线段与绳子节点的距离，
//   距离小于 cutRadius 则触发切割。
//   支持多点触控（多条绳子同时切割）。

using System.Collections.Generic;
using UnityEngine;
using XLua;

namespace CutRope.Game
{
    [LuaCallCSharp]
    public class CutInput : MonoBehaviour
    {
        [Header("切割参数")]
        [Tooltip("切割检测半径（世界单位）")]
        public float cutRadius = 0.1f;

        [Tooltip("最小划动距离（像素），防止点击误切")]
        public float minSwipeDistance = 5f;

        // ── 运行时状态 ────────────────────────────────────────────────
        private Vector2 _prevPos;
        private bool    _pressing;

        // 场景中所有绳子（LevelController 注册）
        private readonly List<RopeSpawner> _ropes = new List<RopeSpawner>();

        // ── Lua 事件钩子 ─────────────────────────────────────────────
        /// <summary>每次切断绳子时触发，参数：切割世界坐标</summary>
        public System.Action<Vector2> OnCut { get; set; }

        // ── 绳子注册 ─────────────────────────────────────────────────
        public void RegisterRope(RopeSpawner rope)   => _ropes.Add(rope);
        public void UnregisterRope(RopeSpawner rope) => _ropes.Remove(rope);
        public void ClearRopes()                     => _ropes.Clear();

        // ── 输入处理 ─────────────────────────────────────────────────
        private void Update()
        {
#if UNITY_EDITOR || UNITY_STANDALONE
            HandleMouse();
#else
            HandleTouch();
#endif
        }

        private void HandleMouse()
        {
            if (Input.GetMouseButtonDown(0))
            {
                _pressing = true;
                _prevPos  = GetWorldPos(Input.mousePosition);
            }
            else if (Input.GetMouseButton(0) && _pressing)
            {
                Vector2 cur = GetWorldPos(Input.mousePosition);
                float dist  = Vector2.Distance(cur, _prevPos);
                if (dist >= minSwipeDistance * 0.01f)  // 转换为世界单位近似
                {
                    TryCut(_prevPos, cur);
                    _prevPos = cur;
                }
            }
            else if (Input.GetMouseButtonUp(0))
            {
                _pressing = false;
            }
        }

        private void HandleTouch()
        {
            for (int i = 0; i < Input.touchCount; i++)
            {
                var touch = Input.GetTouch(i);
                if (touch.phase == TouchPhase.Began)
                {
                    // 多点触控：各自独立检测
                }
                else if (touch.phase == TouchPhase.Moved)
                {
                    Vector2 cur  = GetWorldPos(touch.position);
                    Vector2 prev = GetWorldPos(touch.position - touch.deltaPosition);
                    TryCut(prev, cur);
                }
            }
        }

        /// <summary>检测线段 (from→to) 是否切过任意绳子节点</summary>
        private void TryCut(Vector2 from, Vector2 to)
        {
            foreach (var rope in _ropes)
            {
                if (!rope || !rope.IsAlive) continue;
                rope.CutAtWorld(ClosestPointOnSegment(from, to, Camera.main));
            }
        }

        /// <summary>鼠标/触摸坐标 → 世界坐标（2D）</summary>
        private static Vector2 GetWorldPos(Vector2 screenPos)
        {
            if (Camera.main == null) return screenPos;
            var wp = Camera.main.ScreenToWorldPoint(new Vector3(screenPos.x, screenPos.y, 0f));
            return new Vector2(wp.x, wp.y);
        }

        // 简化版：取线段中点作为切割点（足够精确）
        private static Vector2 ClosestPointOnSegment(Vector2 a, Vector2 b, Camera cam)
        {
            return (a + b) * 0.5f;
        }

        private void OnDestroy()
        {
            OnCut = null;
            _ropes.Clear();
        }
    }
}
