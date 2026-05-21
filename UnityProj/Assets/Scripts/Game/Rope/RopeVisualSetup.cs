// RopeVisualSetup.cs
// 运行时自动给 RopeSpawner 添加 RopeRenderer 组件
// 挂载到 RopeSpawner 同一 GameObject 上（或在 Awake 里自动添加）
//
// 这个脚本在 RopeSpawner.Awake() 之前执行，保证 RopeRenderer 已就绪。

using UnityEngine;

namespace CutRope.Game
{
    /// <summary>
    /// 自动给所有 RopeSpawner 配置高质量渲染器。
    /// 挂载在 RopeSpawner 同一 GameObject 即可。
    /// </summary>
    [RequireComponent(typeof(RopeSpawner))]
    public class RopeVisualSetup : MonoBehaviour
    {
        private void Awake()
        {
            var spawner = GetComponent<RopeSpawner>();
            if (spawner == null) return;

            // 如果已手动配置 RopeRenderer，不要覆盖
            if (spawner.ropeRenderer != null) return;

            // 自动添加并配置 RopeRenderer
            var rr = gameObject.GetComponent<RopeRenderer>()
                  ?? gameObject.AddComponent<RopeRenderer>();

            // 确保有 LineRenderer 给 RopeRenderer 使用
            var lr = gameObject.GetComponent<LineRenderer>()
                  ?? gameObject.AddComponent<LineRenderer>();

            rr.interpolationSteps = 6;
            rr.tension            = 0.5f;
            rr.widthTop           = 0.10f;
            rr.widthBottom        = 0.06f;
            rr.colorTop           = new Color(0.55f, 0.32f, 0.08f);
            rr.colorMid           = new Color(0.80f, 0.50f, 0.15f);
            rr.colorBottom        = new Color(0.65f, 0.38f, 0.10f);
            rr.showAnchorPin      = true;
            rr.pinRadius          = 0.12f;
            rr.pinColor           = new Color(0.72f, 0.72f, 0.78f);
            rr.textureScrollSpeed = 0f;  // 无纹理时关掉动画

            spawner.ropeRenderer = rr;
            spawner.lineRenderer  = lr;  // 兼容：RopeRenderer 内部用同一个 LineRenderer

            Debug.Log($"[RopeVisualSetup] Configured RopeRenderer on {gameObject.name}");
        }
    }
}
