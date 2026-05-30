// RopeRenderer.cs
// 高质量绳子渲染器 — 使用贝塞尔曲线插值让绳子更平滑、更真实
//
// 功能：
//   - 在物理节点之间做 Catmull-Rom 样条插值，生成平滑曲线
//   - 支持锚点钉子装饰（顶部悬挂点）
//   - 绳子末端「扭曲」纹理动画
//   - 切割时播放断裂闪光效果

using System.Collections.Generic;
using UnityEngine;
using XLua;

namespace CutRope.Game
{
    /// <summary>高质量绳子渲染器，配合 RopeSpawner 使用</summary>
    [LuaCallCSharp]
    [RequireComponent(typeof(LineRenderer))]
    public class RopeRenderer : MonoBehaviour
    {
        // ── 曲线插值参数 ─────────────────────────────────────────────
        [Header("曲线平滑")]
        [Tooltip("每段物理节点之间插多少个曲线点（越大越平滑，建议 4-8）")]
        [Range(2, 12)]
        public int interpolationSteps = 6;

        [Tooltip("Catmull-Rom 张力（0=折线，1=最大弯曲，推荐 0.5）")]
        [Range(0f, 1f)]
        public float tension = 0.5f;

        // ── 绳子外观 ─────────────────────────────────────────────────
        [Header("绳子外观")]
        [Tooltip("绳子顶部（锚点侧）宽度")]
        public float widthTop    = 0.10f;
        [Tooltip("绳子底部（糖果侧）宽度")]
        public float widthBottom = 0.06f;

        [Tooltip("绳子顶部颜色")]
        public Color colorTop    = new Color(0.55f, 0.32f, 0.08f);  // 深棕
        [Tooltip("绳子中部颜色")]
        public Color colorMid    = new Color(0.80f, 0.50f, 0.15f);  // 橙棕
        [Tooltip("绳子底部颜色")]
        public Color colorBottom = new Color(0.65f, 0.38f, 0.10f);  // 棕

        // ── 锚点钉子 ─────────────────────────────────────────────────
        [Header("锚点装饰")]
        [Tooltip("是否显示顶部钉子")]
        public bool showAnchorPin = true;
        [Tooltip("钉子半径")]
        public float pinRadius = 0.12f;
        [Tooltip("钉子颜色")]
        public Color pinColor  = new Color(0.7f, 0.7f, 0.75f);  // 金属灰

        // ── 绳子纹理动画 ─────────────────────────────────────────────
        [Header("纹理动画（可选）")]
        [Tooltip("纹理滚动速度（0=不滚动）")]
        public float textureScrollSpeed = 0.3f;

        // ── 内部引用 ─────────────────────────────────────────────────
        private LineRenderer _lr;
        private GameObject   _pinGo;    // 锚点钉子 GameObject
        private float        _texOffset;

        // 外部注入的节点位置列表（由 RopeSpawner 每帧推送）
        private readonly List<Vector3> _physicsPoints = new List<Vector3>();
        private bool _dirty;

        // ── 初始化 ───────────────────────────────────────────────────
        private void Awake()
        {
            _lr = GetComponent<LineRenderer>();
            ApplyLineRendererStyle();
            if (showAnchorPin) CreateAnchorPin();
        }

        private void ApplyLineRendererStyle()
        {
            _lr.numCornerVertices = 6;
            _lr.numCapVertices    = 6;
            _lr.useWorldSpace     = true;
            _lr.shadowCastingMode = UnityEngine.Rendering.ShadowCastingMode.Off;
            _lr.receiveShadows    = false;
            _lr.sortingOrder      = 2;

            // 宽度曲线：从粗（顶）到细（底）
            var wc = new AnimationCurve(
                new Keyframe(0f, widthTop),
                new Keyframe(0.4f, (widthTop + widthBottom) * 0.5f),
                new Keyframe(1f, widthBottom)
            );
            _lr.widthCurve = wc;

            // 颜色渐变：三段
            var grad = new Gradient();
            grad.SetKeys(
                new GradientColorKey[] {
                    new GradientColorKey(colorTop,    0.00f),
                    new GradientColorKey(colorMid,    0.45f),
                    new GradientColorKey(colorBottom, 1.00f),
                },
                new GradientAlphaKey[] {
                    new GradientAlphaKey(1f, 0f),
                    new GradientAlphaKey(1f, 1f),
                }
            );
            _lr.colorGradient = grad;
        }

        private void CreateAnchorPin()
        {
            _pinGo = new GameObject("AnchorPin");
            _pinGo.transform.SetParent(transform, false);

            var sr = _pinGo.AddComponent<SpriteRenderer>();
            sr.sprite       = CreateCircleSprite(32);
            sr.color        = pinColor;
            sr.sortingOrder = 3;

            _pinGo.transform.localScale = Vector3.one * pinRadius * 2f;
            _pinGo.SetActive(false);  // 等到有数据才显示
        }

        // ── 公共 API（由 RopeSpawner 调用）──────────────────────────

        /// <summary>更新物理节点列表，下一帧重绘</summary>
        public void SetPoints(List<Vector3> points)
        {
            _physicsPoints.Clear();
            _physicsPoints.AddRange(points);
            _dirty = true;
        }

        /// <summary>清空绳子（切断后）</summary>
        public void Clear()
        {
            _physicsPoints.Clear();
            _lr.positionCount = 0;
            if (_pinGo) _pinGo.SetActive(false);
        }

        // ── 每帧更新 ─────────────────────────────────────────────────
        private void LateUpdate()
        {
            if (!_dirty && _physicsPoints.Count == 0) return;
            _dirty = false;

            if (_physicsPoints.Count < 2)
            {
                _lr.positionCount = 0;
                if (_pinGo) _pinGo.SetActive(false);
                return;
            }

            // 锚点钉子位置 = 第一个物理节点
            if (_pinGo && showAnchorPin)
            {
                _pinGo.SetActive(true);
                _pinGo.transform.position = _physicsPoints[0];
            }

            // 生成 Catmull-Rom 插值点
            var curve = BuildCatmullRom(_physicsPoints, interpolationSteps, tension);

            _lr.positionCount = curve.Count;
            _lr.SetPositions(curve.ToArray());

            // 纹理偏移动画
            if (textureScrollSpeed > 0f && _lr.material != null)
            {
                _texOffset += Time.deltaTime * textureScrollSpeed;
                _lr.material.mainTextureOffset = new Vector2(0f, _texOffset % 1f);
            }
        }

        // ── Catmull-Rom 样条插值 ─────────────────────────────────────
        /// <summary>
        /// 将 physicsPoints 用 Catmull-Rom 样条插值，生成平滑曲线点列表
        /// </summary>
        private static List<Vector3> BuildCatmullRom(
            List<Vector3> pts, int steps, float alpha)
        {
            var result = new List<Vector3>(pts.Count * steps);

            for (int i = 0; i < pts.Count - 1; i++)
            {
                // 控制点：超出边界时镜像延伸
                Vector3 p0 = i > 0              ? pts[i - 1] : pts[i] + (pts[i] - pts[i + 1]);
                Vector3 p1 = pts[i];
                Vector3 p2 = pts[i + 1];
                Vector3 p3 = i + 2 < pts.Count  ? pts[i + 2] : pts[i + 1] + (pts[i + 1] - pts[i]);

                for (int j = 0; j < steps; j++)
                {
                    float t = j / (float)steps;
                    result.Add(CatmullRom(p0, p1, p2, p3, t, alpha));
                }
            }
            // 末端点
            result.Add(pts[pts.Count - 1]);
            return result;
        }

        private static Vector3 CatmullRom(
            Vector3 p0, Vector3 p1, Vector3 p2, Vector3 p3,
            float t, float alpha)
        {
            float t2 = t * t;
            float t3 = t2 * t;

            // Catmull-Rom 公式（alpha 控制张力）
            Vector3 m1 = alpha * (p2 - p0);
            Vector3 m2 = alpha * (p3 - p1);

            return ( 2f * t3 - 3f * t2 + 1f) * p1
                 + (      t3 - 2f * t2 + t ) * m1
                 + (-2f * t3 + 3f * t2     ) * p2
                 + (      t3 -      t2     ) * m2;
        }

        // ── 工具方法 ─────────────────────────────────────────────────

        /// <summary>程序化生成圆形 Sprite（用于钉子）</summary>
        private static Sprite CreateCircleSprite(int resolution)
        {
            var tex  = new Texture2D(resolution, resolution, TextureFormat.RGBA32, false);
            var cx   = resolution / 2f - 0.5f;
            var cy   = resolution / 2f - 0.5f;
            var r    = resolution / 2f - 1f;

            for (int y = 0; y < resolution; y++)
            for (int x = 0; x < resolution; x++)
            {
                float dx = x - cx, dy = y - cy;
                float dist = Mathf.Sqrt(dx * dx + dy * dy);

                // 主圆 + 高光
                if (dist <= r)
                {
                    float edge = Mathf.Clamp01((r - dist) / 1.5f);
                    // 左上高光
                    float highlight = Mathf.Clamp01(1f - (dx + dy + r) / (r * 0.7f));
                    Color col = Color.Lerp(Color.white, Color.white * 0.6f, 1f - highlight);
                    col.a = edge;
                    tex.SetPixel(x, y, col);
                }
                else
                {
                    tex.SetPixel(x, y, Color.clear);
                }
            }
            tex.Apply();
            tex.filterMode = FilterMode.Bilinear;
            return Sprite.Create(tex,
                new Rect(0, 0, resolution, resolution),
                new Vector2(0.5f, 0.5f),
                resolution);
        }

        private void OnDestroy()
        {
            if (_pinGo) Destroy(_pinGo);
        }
    }
}
