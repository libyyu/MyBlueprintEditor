// ─────────────────────────────────────────────────────────────────────
// MinimapSystem.cs — 小地图系统
//
// 方案：第二相机（正交/俯视）渲染到 RenderTexture → 显示到 RawImage。
// NPC 标记使用世界坐标映射。
// ─────────────────────────────────────────────────────────────────────

using System.Collections.Generic;
using UnityEngine;
using UnityEngine.UI;

namespace BlueprintMiniGame
{
    public class MinimapSystem : MonoBehaviour
    {
        public static MinimapSystem Instance { get; private set; }

        [Header("小地图相机")]
        [Tooltip("留空则自动创建")]
        [SerializeField] private Camera minimapCamera;
        [SerializeField] private float  cameraHeight   = 50f;
        [SerializeField] private float  orthographicSize = 30f;
        [SerializeField] private int    renderTexSize   = 256;

        [Header("UI")]
        [SerializeField] private RawImage  minimapDisplay;       // 显示 RenderTexture
        [SerializeField] private RectTransform markerParent;     // 标记层（覆盖在 RawImage 上）
        [SerializeField] private GameObject npcMarkerPrefab;     // NPC 标记预制体（Image + 可选名字）
        [SerializeField] private GameObject questMarkerPrefab;   // 任务标记
        [SerializeField] private GameObject playerMarkerPrefab;  // 玩家标记

        [Header("跟随")]
        [SerializeField] private Transform playerTransform;
        [SerializeField] private bool      rotateWithPlayer = true;

        private RenderTexture _rt;
        private GameObject _playerMarker;
        private readonly Dictionary<string, MarkerData> _markers = new();

        private struct MarkerData
        {
            public GameObject   go;
            public RectTransform rt;
            public Transform     worldTarget;
        }

        void Awake()
        {
            if (Instance != null) { Destroy(gameObject); return; }
            Instance = this;
        }

        void Start()
        {
            SetupCamera();
            SetupPlayerMarker();

            // 自动注册所有已有 NPC
            if (NpcRegistry.Instance != null)
            {
                foreach (var npc in NpcRegistry.Instance.GetAll())
                    AddNpcMarker(npc.id, npc.displayName, npc.transform);
            }
        }

        void LateUpdate()
        {
            FollowPlayer();
            UpdateMarkers();
        }

        // ── 公共 API ────────────────────────────────────────────────

        public void AddNpcMarker(string id, string name, Transform target)
        {
            if (_markers.ContainsKey(id) || npcMarkerPrefab == null || markerParent == null) return;
            var go = Instantiate(npcMarkerPrefab, markerParent);
            var label = go.GetComponentInChildren<TMPro.TMP_Text>();
            if (label != null) label.text = name;
            _markers[id] = new MarkerData { go = go, rt = go.GetComponent<RectTransform>(), worldTarget = target };
        }

        public void AddQuestMarker(string id, Transform target)
        {
            if (_markers.ContainsKey("quest_" + id) || questMarkerPrefab == null) return;
            var go = Instantiate(questMarkerPrefab, markerParent);
            _markers["quest_" + id] = new MarkerData { go = go, rt = go.GetComponent<RectTransform>(), worldTarget = target };
        }

        public void RemoveMarker(string id)
        {
            if (_markers.TryGetValue(id, out var data))
            {
                if (data.go != null) Destroy(data.go);
                _markers.Remove(id);
            }
        }

        // ── 内部 ────────────────────────────────────────────────────

        private void SetupCamera()
        {
            _rt = new RenderTexture(renderTexSize, renderTexSize, 16);

            if (minimapCamera == null)
            {
                var camGo = new GameObject("MinimapCamera");
                camGo.transform.SetParent(transform);
                minimapCamera = camGo.AddComponent<Camera>();
                minimapCamera.orthographic = true;
                minimapCamera.clearFlags   = CameraClearFlags.SolidColor;
                minimapCamera.backgroundColor = new Color(0.15f, 0.2f, 0.15f);
                minimapCamera.cullingMask  = ~0;  // 所有层；实际项目应设专用层
            }

            minimapCamera.orthographicSize = orthographicSize;
            minimapCamera.targetTexture = _rt;

            if (minimapDisplay != null)
                minimapDisplay.texture = _rt;
        }

        private void SetupPlayerMarker()
        {
            if (playerMarkerPrefab != null && markerParent != null)
            {
                _playerMarker = Instantiate(playerMarkerPrefab, markerParent);
                // 玩家始终在中心
                var rt = _playerMarker.GetComponent<RectTransform>();
                if (rt != null) rt.anchoredPosition = Vector2.zero;
            }
        }

        private void FollowPlayer()
        {
            if (minimapCamera == null || playerTransform == null) return;

            var pos = playerTransform.position;
            minimapCamera.transform.position = new Vector3(pos.x, pos.y + cameraHeight, pos.z);
            minimapCamera.transform.rotation = Quaternion.Euler(90f, 0f, 0f);

            if (rotateWithPlayer && _playerMarker != null)
            {
                float angle = -playerTransform.eulerAngles.y;
                _playerMarker.GetComponent<RectTransform>().localRotation = Quaternion.Euler(0, 0, angle);
            }
        }

        private void UpdateMarkers()
        {
            if (minimapCamera == null || playerTransform == null || markerParent == null) return;

            float mapSize = markerParent.rect.width * 0.5f;
            float worldRange = orthographicSize;

            foreach (var kv in _markers)
            {
                var data = kv.Value;
                if (data.worldTarget == null || data.go == null) continue;

                Vector3 offset = data.worldTarget.position - playerTransform.position;
                float nx = offset.x / worldRange * mapSize;
                float ny = offset.z / worldRange * mapSize;

                // 裁剪到地图范围内
                float dist = Mathf.Sqrt(nx * nx + ny * ny);
                if (dist > mapSize)
                {
                    nx = nx / dist * mapSize;
                    ny = ny / dist * mapSize;
                    data.go.SetActive(true);  // 边缘仍显示（可改为隐藏）
                }
                else
                {
                    data.go.SetActive(true);
                }

                if (data.rt != null)
                    data.rt.anchoredPosition = new Vector2(nx, ny);
            }
        }

        void OnDestroy()
        {
            if (_rt != null) _rt.Release();
        }
    }
}
