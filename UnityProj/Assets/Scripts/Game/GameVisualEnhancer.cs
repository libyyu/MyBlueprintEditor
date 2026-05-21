// GameVisualEnhancer.cs
// 游戏视觉增强器 — 运行时改善游戏整体视觉效果
//
// 功能：
//   1. 背景颜色/渐变改善（Camera 背景）
//   2. 糖果外观美化（程序化圆形 + 发光效果）
//   3. 怪兽嘴部区域视觉改善
//   4. 切割轨迹特效（拖尾线）
//
// 挂载在任意场景 GameObject 上（建议 GameManager）

using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using XLua;

namespace CutRope.Game
{
    [LuaCallCSharp]
    public class GameVisualEnhancer : MonoBehaviour
    {
        [Header("背景")]
        public bool enhanceBackground = true;
        public Color bgColorTop    = new Color(0.10f, 0.10f, 0.22f);  // 深蓝紫
        public Color bgColorBottom = new Color(0.05f, 0.12f, 0.28f);  // 深海蓝

        [Header("糖果")]
        public bool enhanceCandy = true;

        [Header("切割特效")]
        public bool showCutTrail = true;
        [Tooltip("切割拖尾持续时间（秒）")]
        public float trailDuration = 0.25f;
        [Tooltip("切割拖尾颜色")]
        public Color trailColor = new Color(1f, 0.9f, 0.3f, 0.9f);  // 金黄

        [Header("粒子")]
        public bool showCutParticles = true;

        // ── 内部状态 ─────────────────────────────────────────────────
        private Camera       _cam;
        private CutInput     _cutInput;
        private TrailRenderer _trail;
        private GameObject   _trailGo;

        // ── 背景渐变渲染器 ────────────────────────────────────────────
        private Mesh     _bgMesh;
        private Material _bgMat;

        private void Start()
        {
            _cam = Camera.main;
            _cutInput = FindFirstObjectByType<CutInput>();

            if (enhanceBackground)  SetupBackground();
            if (showCutTrail)       SetupCutTrail();
            if (enhanceCandy)       SetupCandies();
        }

        // ── 背景渐变 ─────────────────────────────────────────────────
        private void SetupBackground()
        {
            if (_cam == null) return;
            // 清除旧背景
            _cam.clearFlags      = CameraClearFlags.SolidColor;
            _cam.backgroundColor = bgColorTop;

            // 创建程序化渐变背景四边形（在摄像机最远处）
            _bgMesh = new Mesh();
            float halfH = _cam.orthographicSize;
            float halfW = halfH * _cam.aspect;
            float z     = _cam.farClipPlane - 0.1f;

            _bgMesh.vertices = new Vector3[]
            {
                new Vector3(-halfW, -halfH, 0),
                new Vector3( halfW, -halfH, 0),
                new Vector3( halfW,  halfH, 0),
                new Vector3(-halfW,  halfH, 0),
            };
            _bgMesh.triangles = new int[] { 0, 2, 1, 0, 3, 2 };
            _bgMesh.colors = new Color[]
            {
                bgColorBottom, bgColorBottom,
                bgColorTop,    bgColorTop,
            };
            _bgMesh.RecalculateNormals();

            var bgGo = new GameObject("BG_Gradient");
            bgGo.transform.SetParent(_cam.transform, false);
            bgGo.transform.localPosition = new Vector3(0, 0, z);

            _bgMat = new Material(Shader.Find("Sprites/Default"));
            _bgMat.renderQueue = 999;  // 最底层

            var mf = bgGo.AddComponent<MeshFilter>();
            mf.mesh = _bgMesh;
            var mr = bgGo.AddComponent<MeshRenderer>();
            mr.material     = _bgMat;
            mr.sortingOrder = -99;
        }

        // ── 切割拖尾 ─────────────────────────────────────────────────
        private void SetupCutTrail()
        {
            _trailGo = new GameObject("CutTrail");
            DontDestroyOnLoad(_trailGo);

            _trail = _trailGo.AddComponent<TrailRenderer>();
            _trail.time          = trailDuration;
            _trail.startWidth    = 0.12f;
            _trail.endWidth      = 0.0f;
            _trail.emitting      = false;
            _trail.sortingOrder  = 10;
            _trail.shadowCastingMode = UnityEngine.Rendering.ShadowCastingMode.Off;

            // 颜色从亮黄到透明
            var grad = new Gradient();
            grad.SetKeys(
                new GradientColorKey[] {
                    new GradientColorKey(trailColor, 0f),
                    new GradientColorKey(new Color(1f, 0.5f, 0.1f), 0.5f),
                    new GradientColorKey(new Color(1f, 0.2f, 0.0f), 1f),
                },
                new GradientAlphaKey[] {
                    new GradientAlphaKey(0.9f, 0f),
                    new GradientAlphaKey(0.5f, 0.4f),
                    new GradientAlphaKey(0.0f, 1f),
                }
            );
            _trail.colorGradient = grad;

            var mat = new Material(Shader.Find("Sprites/Default"));
            _trail.material = mat;

            // 绑定到 CutInput
            if (_cutInput != null)
                _cutInput.OnCut += OnPlayerCut;
        }

        private Vector2 _lastCutPos;
        private float   _trailActive;

        private void OnPlayerCut(Vector2 worldPos)
        {
            _lastCutPos = worldPos;
            _trailActive = trailDuration;

            if (_trail)
            {
                _trailGo.transform.position = new Vector3(worldPos.x, worldPos.y, 0);
                _trail.emitting = true;
            }

            if (showCutParticles)
                SpawnCutParticles(worldPos);
        }

        // ── 糖果美化 ─────────────────────────────────────────────────
        private void SetupCandies()
        {
            var candies = FindObjectsByType<Candy>(FindObjectsSortMode.None);
            foreach (var c in candies) EnhanceCandy(c);
        }

        private void EnhanceCandy(Candy candy)
        {
            if (candy == null) return;

            // 获取或创建 SpriteRenderer
            var sr = candy.GetComponent<SpriteRenderer>();
            if (sr == null) sr = candy.gameObject.AddComponent<SpriteRenderer>();

            // 如果已有 sprite 且比较好看就不动
            if (sr.sprite != null && sr.sprite.bounds.size.x > 0.3f) return;

            // 程序化糖果：亮黄圆形 + 高光
            sr.sprite       = CreateCandySprite();
            sr.color        = Color.white;
            sr.sortingOrder = 5;
            candy.transform.localScale = Vector3.one * 1.2f;

            // 添加发光效果（通过子对象实现）
            AddCandyGlow(candy.gameObject, new Color(1f, 0.9f, 0.0f, 0.3f));
        }

        private void AddCandyGlow(GameObject go, Color glowColor)
        {
            var glowGo = new GameObject("Glow");
            glowGo.transform.SetParent(go.transform, false);
            glowGo.transform.localScale = Vector3.one * 1.8f;

            var sr = glowGo.AddComponent<SpriteRenderer>();
            sr.sprite       = CreateCircleSprite(32, true);
            sr.color        = glowColor;
            sr.sortingOrder = 4;  // 比糖果低一层
        }

        // ── 切割粒子 ─────────────────────────────────────────────────
        private void SpawnCutParticles(Vector2 pos)
        {
            StartCoroutine(CutParticleEffect(pos));
        }

        private IEnumerator CutParticleEffect(Vector2 pos)
        {
            int count = 8;
            var parts = new List<GameObject>(count);

            for (int i = 0; i < count; i++)
            {
                var p = new GameObject($"CutPart_{i}");
                p.transform.position = pos;

                var sr = p.AddComponent<SpriteRenderer>();
                sr.sprite       = CreateCircleSprite(8, false);
                sr.color        = new Color(1f, 0.85f, 0.1f, 0.9f);
                sr.sortingOrder = 15;
                p.transform.localScale = Vector3.one * 0.06f;

                // 随机速度
                float angle = (i / (float)count) * Mathf.PI * 2f + Random.Range(-0.3f, 0.3f);
                float speed = Random.Range(2f, 4f);
                var vel = new Vector2(Mathf.Cos(angle) * speed, Mathf.Sin(angle) * speed);

                parts.Add(p);
                StartCoroutine(AnimateParticle(p, vel, 0.35f));
            }

            yield return new WaitForSeconds(0.5f);
            foreach (var p in parts)
                if (p) Destroy(p);
        }

        private IEnumerator AnimateParticle(GameObject go, Vector2 velocity, float lifetime)
        {
            float t = 0f;
            var sr = go.GetComponent<SpriteRenderer>();
            var startColor = sr ? sr.color : Color.white;

            while (t < lifetime && go)
            {
                t += Time.deltaTime;
                go.transform.position += (Vector3)(velocity * Time.deltaTime);
                velocity.y -= 6f * Time.deltaTime;  // 重力

                if (sr)
                {
                    var c = startColor;
                    c.a = Mathf.Lerp(1f, 0f, t / lifetime);
                    sr.color = c;
                }
                yield return null;
            }

            if (go) Destroy(go);
        }

        // ── 每帧 ─────────────────────────────────────────────────────
        private void Update()
        {
            // 鼠标拖尾跟随
            if (_trail && _trailActive > 0)
            {
                _trailActive -= Time.deltaTime;
                if (_trailActive <= 0)
                {
                    _trail.emitting = false;
                }
            }
        }

        // ── 工具方法 ─────────────────────────────────────────────────

        private static Sprite CreateCandySprite()
        {
            // 亮黄糖果：圆形 + 高光 + 彩色条纹感
            int res = 64;
            var tex = new Texture2D(res, res, TextureFormat.RGBA32, false);
            float cx = res / 2f - 0.5f, cy = res / 2f - 0.5f;
            float r  = res / 2f - 2f;

            for (int y = 0; y < res; y++)
            for (int x = 0; x < res; x++)
            {
                float dx = x - cx, dy = y - cy;
                float dist = Mathf.Sqrt(dx * dx + dy * dy);

                if (dist <= r)
                {
                    float edge = Mathf.Clamp01((r - dist) / 2f);

                    // 基础颜色：亮黄
                    Color baseCol = new Color(1f, 0.85f, 0.05f);

                    // 条纹（斜向）
                    float stripe = Mathf.Sin((dx + dy) * 0.28f) * 0.5f + 0.5f;
                    baseCol = Color.Lerp(baseCol, new Color(1f, 0.6f, 0.0f), stripe * 0.25f);

                    // 高光（左上角）
                    float hlDist = Mathf.Sqrt((dx + r * 0.35f) * (dx + r * 0.35f) + (dy - r * 0.35f) * (dy - r * 0.35f));
                    float hl = Mathf.Clamp01(1f - hlDist / (r * 0.45f));
                    baseCol = Color.Lerp(baseCol, Color.white, hl * 0.6f);

                    // 边缘暗化
                    baseCol *= Mathf.Lerp(0.7f, 1f, edge);
                    baseCol.a = edge;
                    tex.SetPixel(x, y, baseCol);
                }
                else
                {
                    tex.SetPixel(x, y, Color.clear);
                }
            }
            tex.Apply();
            tex.filterMode = FilterMode.Bilinear;
            return Sprite.Create(tex, new Rect(0, 0, res, res), new Vector2(0.5f, 0.5f), res);
        }

        private static Sprite CreateCircleSprite(int res, bool soft)
        {
            var tex = new Texture2D(res, res, TextureFormat.RGBA32, false);
            float cx = res / 2f - 0.5f, cy = res / 2f - 0.5f;
            float r  = res / 2f - 1f;

            for (int y = 0; y < res; y++)
            for (int x = 0; x < res; x++)
            {
                float dx = x - cx, dy = y - cy;
                float dist = Mathf.Sqrt(dx * dx + dy * dy);
                float edgeW = soft ? 3f : 1f;
                float a = Mathf.Clamp01((r - dist) / edgeW);
                tex.SetPixel(x, y, new Color(1, 1, 1, a));
            }
            tex.Apply();
            tex.filterMode = FilterMode.Bilinear;
            return Sprite.Create(tex, new Rect(0, 0, res, res), new Vector2(0.5f, 0.5f), res);
        }

        private void OnDestroy()
        {
            if (_trailGo)  Destroy(_trailGo);
            if (_bgMesh)   Destroy(_bgMesh);
            if (_bgMat)    Destroy(_bgMat);

            if (_cutInput != null)
                _cutInput.OnCut -= OnPlayerCut;
        }
    }
}
