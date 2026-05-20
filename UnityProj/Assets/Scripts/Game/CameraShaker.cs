// CameraShaker.cs
// 摄像机震屏组件，挂在 Main Camera 上，供蓝图节点调用

using System.Collections;
using UnityEngine;
using XLua;

namespace CutRope.Game
{
    [LuaCallCSharp]
    public class CameraShaker : MonoBehaviour
    {
        public static CameraShaker Instance { get; private set; }

        private void Awake()
        {
            Instance = this;
        }

        /// <summary>Lua / 蓝图调用：触发震屏</summary>
        public void Shake(float intensity, float duration)
        {
            StartCoroutine(DoShake(intensity, duration));
        }

        private IEnumerator DoShake(float intensity, float duration)
        {
            Vector3 origPos = transform.localPosition;
            float elapsed = 0f;
            while (elapsed < duration)
            {
                elapsed += Time.deltaTime;
                float x = origPos.x + Random.Range(-intensity, intensity);
                float y = origPos.y + Random.Range(-intensity, intensity);
                transform.localPosition = new Vector3(x, y, origPos.z);
                yield return null;
            }
            transform.localPosition = origPos;
        }

        private void OnDestroy()
        {
            if (Instance == this) Instance = null;
        }
    }
}
