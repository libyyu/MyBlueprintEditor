// AINpcStreamingController.cs
// ─────────────────────────────────────────────────────────────────────────────
// AI NPC 控制器 — 流式版（逐 token 追加，沉浸感最强）
//
// 蓝图约定（配合 AI_NPC_Streaming.bjson）：
//   LLM.StreamChat.onChunk → PrintString("CHUNK:<token>")
//   LLM.StreamChat.onDone  → PrintString("DONE:<fullText>")
//   LLM.StreamChat.onError → PrintString("ERROR:<msg>")  level=Error
//
// 这种前缀约定的好处：
//   - 不修改 Runtime 源码
//   - Unity 侧用一个 OnPrint 事件就能区分所有状态
//   - 蓝图作者可在编辑器直接调试（PrintString 就是输出）
//
// 为什么 WebGL 也能跑？
//   HttpClient_Emscripten 对 StreamAsync 做了降级：后台拿完整响应后，
//   主线程把整个 body 当作一个 chunk 分发给蓝图。所以：
//     - Native 桌面：真·流式（多个 chunk）
//     - WebGL：整段返回后"伪流式"（一个 chunk = 完整回复）
//   UI 层会自动适配两种情况（chunk 追加到同一气泡）
// ─────────────────────────────────────────────────────────────────────────────

using System;
using UnityEngine;
using BlueprintRuntime;

namespace BlueprintRuntime.Samples.AINpc
{
    public class AINpcStreamingController : MonoBehaviour
    {
        [Header("NPC 设定")]
        [SerializeField] private string npcName = "喵喵店长";

        [TextArea(3, 10)]
        [SerializeField] private string personality =
            "你是一家幻想世界酒馆的猫咪店长，叫喵喵。说话温柔好奇，句尾常加 \"喵~\"。" +
            "玩家是冒险者。用 2~3 句话回复。";

        [Header("对话蓝图（流式）")]
        [SerializeField] private TextAsset streamingBlueprint;

        [Header("行为")]
        [SerializeField] private float cooldown = 1.0f;

        // ── 事件 ───────────────────────────────────────────────────
        /// <summary>NPC 开始思考</summary>
        public event Action OnThinking;

        /// <summary>NPC 开始回复（第一个 chunk 到达前，UI 可以清空气泡）</summary>
        public event Action OnReplyStart;

        /// <summary>收到一个 token chunk（追加到气泡末尾）</summary>
        public event Action<string> OnChunk;

        /// <summary>流式回复结束（参数：完整文本；UI 可做完成动效）</summary>
        public event Action<string> OnReplyDone;

        /// <summary>出错</summary>
        public event Action<string> OnError;

        public string NpcName   => npcName;
        public bool   IsStreaming { get; private set; }

        private BPRunner _runner;
        private float   _lastSayTime = -1;
        private bool    _replyStarted;

        void Start()
        {
            var svc = BlueprintService.Instance;
            if (svc == null || streamingBlueprint == null)
            {
                Debug.LogError($"[{npcName}] BlueprintService 或 streamingBlueprint 未配置");
                return;
            }

            _runner = svc.CreateRunner();
            _runner.OnPrint += HandlePrint;
            _runner.OnLog   += (lv, msg) => {
                if (lv >= BPLogLevel.Verbose) Debug.LogWarning($"[BP-{npcName}] {msg}");
            };
            try
            { _runner.LoadFromJson(streamingBlueprint.text); }
            catch (Exception e)
            {
                Debug.LogError($"[{npcName}] 流式蓝图加载失败");
                return;
            }

            _runner.SetVariable("Personality", personality);
            _runner.SetVariable("BaseURL",     svc.LlmBaseUrl);
            _runner.SetVariable("ApiKey",      svc.LlmApiKey);
            _runner.SetVariable("Model",       svc.LlmModel);
        }

        public bool Say(string playerInput)
        {
            if (_runner == null) { Debug.LogError($"[{npcName}] Say failed: runner is null"); return false; }
            if (IsStreaming) { Debug.Log($"[{npcName}] Say skipped: already streaming"); return false; }
            if (_lastSayTime != -1 && Time.time - _lastSayTime < cooldown) { Debug.Log($"[{npcName}] Say skipped: cooldown"); return false; }
            if (string.IsNullOrWhiteSpace(playerInput)) { Debug.Log($"[{npcName}] Say skipped: empty input"); return false; }

            _lastSayTime   = Time.time;
            IsStreaming    = true;
            _replyStarted  = false;
            OnThinking?.Invoke();

            Debug.Log($"[{npcName}] Say() executing blueprint with input: {playerInput.Substring(0, Mathf.Min(50, playerInput.Length))}...");
            _runner.SetVariable("PlayerInput", playerInput);
            _runner.Execute();
            Debug.Log($"[{npcName}] Execute() done, HasPendingWork={_runner.HasPendingWork}");
            return true;
        }

        private void HandlePrint(BPLogLevel level, string msg)
        {
            if (string.IsNullOrEmpty(msg)) return;
            Debug.Log($"[{npcName}] PrintCallback: {msg}");

            // 按前缀分派
            if (msg.StartsWith("CHUNK:"))
            {
                if (!_replyStarted)
                {
                    _replyStarted = true;
                    OnReplyStart?.Invoke();
                }
                OnChunk?.Invoke(msg.Substring(6));
            }
            else if (msg.StartsWith("DONE:"))
            {
                IsStreaming   = false;
                _replyStarted = false;
                OnReplyDone?.Invoke(msg.Substring(5));
            }
            else if (msg.StartsWith("ERROR:"))
            {
                IsStreaming   = false;
                _replyStarted = false;
                OnError?.Invoke(msg.Substring(6));
            }
            else
            {
                // 无前缀：当整段回复处理（向后兼容基础蓝图）
                IsStreaming = false;
                if (!_replyStarted) OnReplyStart?.Invoke();
                OnChunk?.Invoke(msg);
                OnReplyDone?.Invoke(msg);
            }
        }

        void OnDestroy()
        {
            if (_runner != null)
            {
                _runner.OnPrint -= HandlePrint;
                BlueprintService.Instance?.ReleaseRunner(_runner);
                _runner = null;
            }
        }
    }
}
