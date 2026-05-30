// Star.cs
// 关卡内星星收集物
//
// - 悬浮在场景中，Candy 的 CircleCollider2D(trigger) 碰到时触发收集
// - 收集后播放缩放消失动画，通知 LevelController
// - Tag: "Star"

using System.Collections;
using UnityEngine;
using XLua;

namespace CutRope.Game
{
    [LuaCallCSharp]
    [RequireComponent(typeof(CircleCollider2D))]
    public class Star : MonoBehaviour
    {
        [Header("星星参数")]
        public int starIndex = 0;          // 第几颗（0/1/2）
        public float collectScale = 1.5f;  // 收集时放大倍数
        public float collectTime  = 0.25f; // 动画时长

        public bool IsCollected { get; private set; }

        // Lua 回调
        public System.Action<Star> OnCollected { get; set; }

        private void Awake()
        {
            var col = GetComponent<CircleCollider2D>();
            col.isTrigger = true;
            gameObject.tag = "Star";
        }

        private void OnTriggerEnter2D(Collider2D other)
        {
            if (IsCollected) return;
            if (!other.CompareTag("Candy") && other.GetComponent<Candy>() == null) return;
            Collect();
        }

        public void Collect()
        {
            if (IsCollected) return;
            IsCollected = true;
            OnCollected?.Invoke(this);
            StartCoroutine(CollectAnim());
        }

        private IEnumerator CollectAnim()
        {
            Vector3 origScale = transform.localScale;
            float t = 0f;
            while (t < collectTime)
            {
                t += Time.deltaTime;
                float p = t / collectTime;
                // 先放大再缩小消失
                float s = p < 0.5f
                    ? Mathf.Lerp(1f, collectScale, p * 2f)
                    : Mathf.Lerp(collectScale, 0f, (p - 0.5f) * 2f);
                transform.localScale = origScale * s;
                yield return null;
            }
            gameObject.SetActive(false);
        }

        private void OnDestroy()
        {
            OnCollected = null;
        }
    }
}
