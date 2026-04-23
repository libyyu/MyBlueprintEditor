// BlueprintService.cs
// ─────────────────────────────────────────────────────────────────────────────
// 单例服务：管理 BlueprintRuntime 的全局生命周期
//
// 职责：
//   1. 启动时注入 HTTP 客户端（WebGL / 原生自动分支）
//   2. 统一 Tick 所有活跃 Runner（每帧 DrainQueue + Tick）
//   3. 集中配置 LLM API Key（可从 Inspector / 玩家设置 / 远端读取）
//
// 使用：把本脚本挂到任意 GameObject，设 DontDestroyOnLoad
// ─────────────────────────────────────────────────────────────────────────────

using System;
using System.Collections.Generic;
using UnityEngine;
using BlueprintRuntime;

namespace BlueprintRuntime.Samples.AINpc
{
    public class BlueprintService : MonoBehaviour
    {
        // ── 单例 ───────────────────────────────────────────────────
        public static BlueprintService Instance { get; private set; }

        // ── LLM 配置（所有 NPC 共享） ───────────────────────────────
        [Header("LLM 配置")]
        [Tooltip("LLM 接入域名。免费推荐：智谱 https://open.bigmodel.cn/api/paas/v4")]
        [SerializeField] private string llmBaseUrl = "https://open.bigmodel.cn/api/paas/v4";

        [Tooltip("API Key。去 https://open.bigmodel.cn 注册免费拿。不要把正式 Key 提交到 Git！")]
        [SerializeField] private string llmApiKey = "";

        [Tooltip("LLM 模型。免费：glm-4-flash。付费推荐：glm-4 / deepseek-chat")]
        [SerializeField] private string llmModel = "glm-4-flash";

        public string LlmBaseUrl => llmBaseUrl;
        public string LlmApiKey  => llmApiKey;
        public string LlmModel   => llmModel;

        /// <summary>运行时修改 API Key（例如玩家在设置页填入后调用）</summary>
        public void SetApiKey(string key) => llmApiKey = key ?? "";

        // ── Runner 管理 ───────────────────────────────────────────
        private readonly List<BPRunner> _runners = new List<BPRunner>();

        // ── 状态 ──────────────────────────────────────────────────
        public event Action OnReady;    // HTTP 初始化完成
        public bool IsReady { get; private set; }

        void Awake()
        {
            if (Instance != null && Instance != this)
            {
                Destroy(gameObject);
                return;
            }
            Instance = this;
            DontDestroyOnLoad(gameObject);
        }

        void Start()
        {
#if UNITY_WEBGL && !UNITY_EDITOR
            // WebGL：HTTP 客户端由 Emscripten + fetch API 提供
            // 默认 CreateDefaultHttpClient 在 WebGL 下就是 HttpClient_Emscripten
            // 注意：LLM 域名必须在 Unity Player Settings → WebGL → Publishing Settings 的 CORS 允许列表里
            // 或由 LLM 供应商返回正确的 CORS 响应头
#endif
            // 初始化默认 HTTP 客户端（Runtime 内部幂等，重复调用无副作用）
            BPRunner.InitDefaultHttpClient();
            IsReady = true;
            OnReady?.Invoke();

            if (string.IsNullOrEmpty(llmApiKey))
            {
                Debug.LogWarning("[BlueprintService] LLM API Key 未设置，AI 对话将无法工作。" +
                    "去 https://open.bigmodel.cn 注册拿免费 Key，填入 Inspector。");
            }
        }

        /// <summary>创建一个新 Runner。服务会自动每帧 Tick 它。</summary>
        public BPRunner CreateRunner()
        {
            if (!IsReady)
                Debug.LogWarning("[BlueprintService] CreateRunner 在 Ready 之前被调用，HTTP 可能还未就绪");

            var r = new BPRunner();
            _runners.Add(r);
            return r;
        }

        /// <summary>释放 Runner。NPC Destroy 时必须调用。</summary>
        public void ReleaseRunner(BPRunner r)
        {
            if (r == null) return;
            _runners.Remove(r);
            r.Dispose();
        }

        void Update()
        {
            float dt = Time.unscaledDeltaTime;
            // 倒序遍历以防 Tick 回调里自销毁
            for (int i = _runners.Count - 1; i >= 0; i--)
            {
                var r = _runners[i];
                if (r == null) { _runners.RemoveAt(i); continue; }
                try
                {
                    // 优化：空闲 runner 跳过 Tick（无 HTTP 异步/无定时器）
                    if (!r.HasPendingWork) continue;
                    r.Tick(dt);
                }
                catch (Exception e) { Debug.LogError($"[BlueprintService] Tick failed: {e}"); }
            }
        }

        void OnDestroy()
        {
            foreach (var r in _runners)
                r?.Dispose();
            _runners.Clear();
            if (Instance == this) Instance = null;
        }
    }
}
