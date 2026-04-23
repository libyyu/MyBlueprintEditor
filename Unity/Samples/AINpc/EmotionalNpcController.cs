// EmotionalNpcController.cs
// ─────────────────────────────────────────────────────────────────────────────
// 情绪化 NPC — 让 LLM 同时输出情绪标签和建议的动作
//
// 蓝图约定（配合 AI_NPC_Emotional.bjson）：
//   LLM 返回 JSON：{ "reply": "...", "emotion": "happy|sad|angry|surprised|neutral",
//                   "action": "idle|wave|jump|nod|shake" }
//   蓝图解析后 PrintString("REPLY:...")  + PrintString("EMOTION:...") + PrintString("ACTION:...")
//
// 使用：
//   - UI 订阅 OnReply 显示文字
//   - NPC Animator 订阅 OnEmotion 切换表情
//   - NPC Animator 订阅 OnAction 播放动作
// ─────────────────────────────────────────────────────────────────────────────

using System;
using UnityEngine;
using BlueprintRuntime;

namespace BlueprintRuntime.Samples.AINpc
{
    public enum NpcEmotion { Neutral, Happy, Sad, Angry, Surprised }
    public enum NpcAction  { Idle, Wave, Jump, Nod, Shake }

    public class EmotionalNpcController : MonoBehaviour
    {
        [Header("NPC 设定")]
        [SerializeField] private string npcName = "喵喵店长";

        [TextArea(4, 12)]
        [SerializeField] private string personality =
            "你是一家幻想世界酒馆的猫咪店长喵喵。说话温柔好奇，句尾常加\"喵~\"。" +
            "你必须严格返回 JSON：{\"reply\":\"回复文字(2~3句)\",\"emotion\":\"happy|sad|angry|surprised|neutral\",\"action\":\"idle|wave|jump|nod|shake\"}" +
            "不要加 markdown，不要解释，只返回 JSON。";

        [Header("情绪蓝图")]
        [SerializeField] private TextAsset emotionalBlueprint;

        [SerializeField] private float cooldown = 1.0f;

        // ── 事件 ───────────────────────────────────────────────────
        public event Action              OnThinking;
        public event Action<string>      OnReply;          // 纯文本回复
        public event Action<NpcEmotion>  OnEmotion;        // 切换表情
        public event Action<NpcAction>   OnAction;         // 播放动作
        public event Action<string>      OnError;

        public string NpcName => npcName;

        private BPRunner _runner;
        private float   _lastSayTime;
        private bool    _thinking;

        void Start()
        {
            var svc = BlueprintService.Instance;
            if (svc == null || emotionalBlueprint == null) {
                Debug.LogError($"[{npcName}] 未配置"); return;
            }

            _runner = svc.CreateRunner();
            _runner.OnPrint += HandlePrint;

            if (!_runner.LoadFromJson(emotionalBlueprint.text)) {
                Debug.LogError($"[{npcName}] 蓝图加载失败"); return;
            }

            _runner.SetVariable("Personality", personality);
            _runner.SetVariable("BaseURL",     svc.LlmBaseUrl);
            _runner.SetVariable("ApiKey",      svc.LlmApiKey);
            _runner.SetVariable("Model",       svc.LlmModel);
        }

        public bool Say(string text)
        {
            if (_runner == null || _thinking) return false;
            if (Time.time - _lastSayTime < cooldown) return false;
            if (string.IsNullOrWhiteSpace(text)) return false;

            _lastSayTime = Time.time;
            _thinking    = true;
            OnThinking?.Invoke();
            _runner.SetVariable("PlayerInput", text);
            _runner.Execute();
            return true;
        }

        private void HandlePrint(BPLogLevel lv, string msg)
        {
            if (string.IsNullOrEmpty(msg)) return;

            if (msg.StartsWith("REPLY:"))
            {
                _thinking = false;
                OnReply?.Invoke(msg.Substring(6));
            }
            else if (msg.StartsWith("EMOTION:"))
            {
                var e = msg.Substring(8).Trim().ToLowerInvariant();
                OnEmotion?.Invoke(ParseEmotion(e));
            }
            else if (msg.StartsWith("ACTION:"))
            {
                var a = msg.Substring(7).Trim().ToLowerInvariant();
                OnAction?.Invoke(ParseAction(a));
            }
            else if (msg.StartsWith("ERROR:") || lv == BPLogLevel.Error)
            {
                _thinking = false;
                OnError?.Invoke(msg);
            }
        }

        static NpcEmotion ParseEmotion(string s) => s switch {
            "happy"     => NpcEmotion.Happy,
            "sad"       => NpcEmotion.Sad,
            "angry"     => NpcEmotion.Angry,
            "surprised" => NpcEmotion.Surprised,
            _           => NpcEmotion.Neutral
        };

        static NpcAction ParseAction(string s) => s switch {
            "wave"  => NpcAction.Wave,
            "jump"  => NpcAction.Jump,
            "nod"   => NpcAction.Nod,
            "shake" => NpcAction.Shake,
            _       => NpcAction.Idle
        };

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

    // ─────────────────────────────────────────────────────────────────────
    // 可选的辅助组件：把 NpcEmotion/NpcAction 桥接到 Animator
    // 挂在同一个 GameObject 上即可，Animator 需要 Trigger: happy/sad/.. wave/jump/..
    // ─────────────────────────────────────────────────────────────────────
    [RequireComponent(typeof(EmotionalNpcController))]
    public class EmotionalNpcAnimatorBridge : MonoBehaviour
    {
        [SerializeField] private Animator   animator;
        [SerializeField] private SpriteRenderer faceSprite;    // 可选：2D 表情贴图
        [SerializeField] private Sprite[]   emotionSprites;    // Neutral/Happy/Sad/Angry/Surprised

        void Awake()
        {
            if (animator == null) animator = GetComponentInChildren<Animator>();
        }

        void Start()
        {
            var ctrl = GetComponent<EmotionalNpcController>();
            ctrl.OnEmotion += OnEmotion;
            ctrl.OnAction  += OnAction;
        }

        void OnEmotion(NpcEmotion e)
        {
            if (faceSprite != null && emotionSprites != null && (int)e < emotionSprites.Length)
                faceSprite.sprite = emotionSprites[(int)e];

            if (animator != null)
                animator.SetTrigger(e.ToString().ToLowerInvariant());
        }

        void OnAction(NpcAction a)
        {
            if (animator != null)
                animator.SetTrigger(a.ToString().ToLowerInvariant());
        }
    }
}
