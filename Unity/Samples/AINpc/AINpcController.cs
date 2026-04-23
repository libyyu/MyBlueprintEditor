// AINpcController.cs
// ─────────────────────────────────────────────────────────────────────────────
// AI NPC 控制器 — 基础版（整段回复）
//
// 挂载方式：
//   1. 把本脚本挂到 NPC 的 GameObject
//   2. Inspector 里：
//      - NpcName：NPC 名字（如"喵喵店长"）
//      - Personality：NPC 性格（多行文本）
//      - DialogBlueprint：拖入 AI_NPC_Dialog.bjson 作为 TextAsset
//   3. UI 脚本订阅 OnReply / OnThinking / OnError 事件
//
// 对话流程：
//   玩家输入 → Say(text) → 蓝图 Execute
//                          → LLM.Chat(异步)
//                          → PrintString(reply)
//                          → C# OnPrint → OnReply 触发
// ─────────────────────────────────────────────────────────────────────────────

using System;
using UnityEngine;
using BlueprintRuntime;

namespace BlueprintRuntime.Samples.AINpc
{
    public class AINpcController : MonoBehaviour
    {
        [Header("NPC 设定")]
        [SerializeField] private string npcName = "喵喵店长";

        [TextArea(3, 10)]
        [SerializeField] private string personality =
            "你是一家幻想世界酒馆的猫咪店长，名字叫喵喵。你说话温柔、好奇，" +
            "句尾经常加 \"喵~\"。玩家是刚踏入酒馆的冒险者。" +
            "请用 2~3 句话回复，不要用列表或 markdown。";

        [Header("对话蓝图（.bjson）")]
        [SerializeField] private TextAsset dialogBlueprint;

        [Header("行为参数")]
        [Tooltip("两次对话之间的最小间隔（秒），防止连点烧 token")]
        [SerializeField] private float cooldown = 1.0f;

        // ── 事件 ───────────────────────────────────────────────────
        /// <summary>NPC 开始思考（显示 "..." 动画）</summary>
        public event Action OnThinking;
        /// <summary>NPC 回复完毕（参数：回复文本）</summary>
        public event Action<string> OnReply;
        /// <summary>出错（参数：错误文本）</summary>
        public event Action<string> OnError;

        public string NpcName => npcName;
        public bool IsThinking { get; private set; }

        private BPRunner _runner;
        private float   _lastSayTime;

        void Start()
        {
            if (dialogBlueprint == null)
            {
                Debug.LogError($"[{npcName}] DialogBlueprint 未设置");
                return;
            }

            var svc = BlueprintService.Instance;
            if (svc == null)
            {
                Debug.LogError($"[{npcName}] 未找到 BlueprintService，请先在场景中放置 BlueprintService");
                return;
            }

            _runner = svc.CreateRunner();

            // PrintString 节点的输出 → NPC 回复
            _runner.OnPrint += HandlePrint;
            _runner.OnLog   += (lv, msg) =>
            {
                if (lv >= BPLogLevel.Warning)
                    Debug.LogWarning($"[BP-{npcName}] {msg}");
            };

            if (!_runner.LoadFromJson(dialogBlueprint.text))
            {
                Debug.LogError($"[{npcName}] 蓝图加载失败");
                return;
            }

            // 注入固定参数
            _runner.SetVariable("Personality", personality);
            _runner.SetVariable("BaseURL",     svc.LlmBaseUrl);
            _runner.SetVariable("ApiKey",      svc.LlmApiKey);
            _runner.SetVariable("Model",       svc.LlmModel);
        }

        /// <summary>玩家向 NPC 说话。冷却/空输入会被忽略。</summary>
        /// <returns>true=请求已发出；false=被冷却或空输入拒绝</returns>
        public bool Say(string playerInput)
        {
            if (_runner == null) return false;
            if (IsThinking) return false;
            if (Time.time - _lastSayTime < cooldown) return false;
            if (string.IsNullOrWhiteSpace(playerInput)) return false;

            _lastSayTime = Time.time;
            IsThinking   = true;
            OnThinking?.Invoke();

            _runner.SetVariable("PlayerInput", playerInput);
            _runner.Execute();
            return true;
        }

        private void HandlePrint(BPLogLevel level, string msg)
        {
            IsThinking = false;
            if (level == BPLogLevel.Error)
                OnError?.Invoke(msg);
            else
                OnReply?.Invoke(msg);
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
