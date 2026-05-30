// Obstacle.cs
// 关卡内障碍物（毒刺/陷阱）
//
// - Candy 碰到触发失败
// - Tag: "Spike"
// - 可设置为移动障碍（蓝图控制 moveSpeed/moveRange）

using UnityEngine;
using XLua;

namespace CutRope.Game
{
    [LuaCallCSharp]
    [RequireComponent(typeof(CircleCollider2D))]
    public class Obstacle : MonoBehaviour
    {
        [Header("障碍物参数")]
        public bool  isMoving   = false;
        public float moveSpeed  = 1.5f;
        public float moveRange  = 2f;     // 左右摆动幅度

        private Vector3 _origin;
        private float   _phase;

        private void Awake()
        {
            var col = GetComponent<CircleCollider2D>();
            col.isTrigger = true;
            gameObject.tag = "Spike";
            _origin = transform.position;
            _phase  = Random.Range(0f, Mathf.PI * 2f); // 随机起始相位，多个障碍不同步
        }

        private void Update()
        {
            if (!isMoving) return;
            float x = _origin.x + Mathf.Sin(Time.time * moveSpeed + _phase) * moveRange;
            transform.position = new Vector3(x, _origin.y, _origin.z);
        }

        // Lua 可调用：启动/停止移动
        public void SetMoving(bool moving) => isMoving = moving;
        public void SetMoveSpeed(float s)  => moveSpeed = s;
        public void SetMoveRange(float r)  => moveRange = r;
    }
}
