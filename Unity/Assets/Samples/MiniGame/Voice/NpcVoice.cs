// ─────────────────────────────────────────────────────────────────────
// NpcVoice.cs — NPC 语音合成（TTS）
//
// 监听 Controller 的 OnReply 事件 → HTTP POST 到 TTS API → 播放 AudioClip
// 支持供应商：
//   1. 智谱 CogView TTS（国内免费额度）
//   2. OpenAI TTS（兼容 DeepSeek/本地 API）
//   3. 微软 Azure TTS（高品质，小游戏级）
//   4. 自定义（任意 POST → 返回音频的 API）
//
// WebGL/小游戏注意：UnityWebRequest 返回的音频必须是 WAV/OGG，
//   MP3 在 WebGL 下不支持 AudioClip.Create。
// ─────────────────────────────────────────────────────────────────────

using BlueprintRuntime.Samples.AINpc;
using System;
using System.Collections;
using System.Text;
using UnityEngine;
using UnityEngine.Networking;

namespace BlueprintRuntime.Samples.MiniGame
{
    [RequireComponent(typeof(AudioSource))]
    public class NpcVoice : MonoBehaviour
    {
        // ── 供应商 ──────────────────────────────────────────────────
        public enum TtsProvider { OpenAI, Azure, Custom }

        [Header("TTS 配置")]
        [SerializeField] private TtsProvider provider = TtsProvider.OpenAI;

        [Tooltip("TTS API 地址\nOpenAI: https://api.openai.com/v1/audio/speech\n" +
                 "DeepSeek: 暂不支持 TTS\n智谱: 暂无公开 TTS\n" +
                 "Azure: https://<region>.tts.speech.microsoft.com/cognitiveservices/v1")]
        [SerializeField] private string apiUrl = "https://api.openai.com/v1/audio/speech";

        [SerializeField] private string apiKey = "";

        [Tooltip("OpenAI: alloy/echo/fable/onyx/nova/shimmer\nAzure: zh-CN-XiaoxiaoNeural")]
        [SerializeField] private string voiceId = "nova";

        [Tooltip("OpenAI: tts-1/tts-1-hd")]
        [SerializeField] private string model = "tts-1";

        [Range(0.25f, 4f)]
        [SerializeField] private float speed = 1.0f;

        [Header("行为")]
        [Tooltip("自动监听同 GameObject 上的 Controller 的 OnReply 事件")]
        [SerializeField] private bool autoSubscribe = true;

        [Tooltip("最大文本长度（超过截断，避免烧钱）")]
        [SerializeField] private int maxTextLength = 200;

        [Tooltip("播放前清除上一句（true=打断式，false=队列式）")]
        [SerializeField] private bool interruptPrevious = true;

        [Header("代理（WebGL/小游戏推荐走代理）")]
        [Tooltip("如果非空，请求走此代理（和 LlmProxyConfig 类似）")]
        [SerializeField] private string proxyUrl = "";

        // ── 事件 ────────────────────────────────────────────────────
        /// <summary>开始请求 TTS（可显示 loading）</summary>
        public event Action OnSpeechRequested;
        /// <summary>AudioClip 就绪，即将播放</summary>
        public event Action<AudioClip> OnSpeechReady;
        /// <summary>TTS 请求失败</summary>
        public event Action<string> OnSpeechError;

        private AudioSource _audio;
        private Coroutine _current;

        void Awake()
        {
            _audio = GetComponent<AudioSource>();
            _audio.playOnAwake = false;
        }

        void Start()
        {
            if (!autoSubscribe) return;

            // 自动订阅同物体上的任意 Controller
            var basic     = GetComponent<AINpcController>();
            var streaming = GetComponent<AINpcStreamingController>();
            var emotional = GetComponent<EmotionalNpcController>();

            if (basic     != null) basic.OnReply     += Speak;
            if (streaming != null) streaming.OnReplyDone += Speak;
            if (emotional != null) emotional.OnReply     += Speak;
        }

        // ── 公共 API ────────────────────────────────────────────────

        /// <summary>手动朗读一段文字</summary>
        public void Speak(string text)
        {
            if (string.IsNullOrWhiteSpace(text)) return;

            // 截断
            if (text.Length > maxTextLength)
                text = text.Substring(0, maxTextLength) + "...";

            // 去掉 markdown / emoji（简单清洗）
            text = CleanForTts(text);

            if (interruptPrevious && _current != null)
            {
                StopCoroutine(_current);
                _audio.Stop();
            }

            _current = StartCoroutine(RequestAndPlay(text));
        }

        /// <summary>立刻停止朗读</summary>
        public void StopSpeaking()
        {
            if (_current != null) StopCoroutine(_current);
            _audio.Stop();
        }

        // ── 内部实现 ────────────────────────────────────────────────

        private IEnumerator RequestAndPlay(string text)
        {
            OnSpeechRequested?.Invoke();

            string url = string.IsNullOrEmpty(proxyUrl) ? apiUrl : proxyUrl;
            string body;
            string contentType;
            string authHeader;

            switch (provider)
            {
                case TtsProvider.OpenAI:
                    body = JsonUtility.ToJson(new OpenAITtsRequest
                    {
                        model = model,
                        input = text,
                        voice = voiceId,
                        speed = speed,
                        response_format = "wav"   // WebGL 兼容
                    });
                    contentType = "application/json";
                    authHeader = $"Bearer {apiKey}";
                    break;

                case TtsProvider.Azure:
                    // Azure SSML
                    body = $"<speak version='1.0' xml:lang='zh-CN'>" +
                           $"<voice name='{voiceId}'>" +
                           $"<prosody rate='{speed}'>{EscapeXml(text)}</prosody>" +
                           $"</voice></speak>";
                    contentType = "application/ssml+xml";
                    authHeader = apiKey;  // Azure 用 Ocp-Apim-Subscription-Key
                    break;

                default: // Custom
                    body = $"{{\"text\":\"{EscapeJson(text)}\",\"voice\":\"{voiceId}\"}}";
                    contentType = "application/json";
                    authHeader = string.IsNullOrEmpty(apiKey) ? null : $"Bearer {apiKey}";
                    break;
            }

            var req = new UnityWebRequest(url, "POST");
            req.uploadHandler = new UploadHandlerRaw(Encoding.UTF8.GetBytes(body));
            req.downloadHandler = new DownloadHandlerAudioClip(new Uri(url), AudioType.WAV);
            req.SetRequestHeader("Content-Type", contentType);

            if (provider == TtsProvider.Azure)
            {
                req.SetRequestHeader("Ocp-Apim-Subscription-Key", authHeader);
                req.SetRequestHeader("X-Microsoft-OutputFormat", "riff-16khz-16bit-mono-pcm");
            }
            else if (!string.IsNullOrEmpty(authHeader))
            {
                req.SetRequestHeader("Authorization", authHeader);
            }

            req.timeout = 15;
            yield return req.SendWebRequest();

#if UNITY_2020_1_OR_NEWER
            if (req.result != UnityWebRequest.Result.Success)
#else
            if (req.isNetworkError || req.isHttpError)
#endif
            {
                string err = $"TTS failed: {req.responseCode} {req.error}";
                Debug.LogWarning($"[NpcVoice] {err}");
                OnSpeechError?.Invoke(err);
                req.Dispose();
                yield break;
            }

            var clip = DownloadHandlerAudioClip.GetContent(req);
            req.Dispose();

            if (clip == null || clip.length < 0.1f)
            {
                OnSpeechError?.Invoke("TTS returned empty audio");
                yield break;
            }

            clip.name = $"tts_{text.Substring(0, Mathf.Min(20, text.Length))}";
            OnSpeechReady?.Invoke(clip);

            _audio.clip = clip;
            _audio.Play();
        }

        // ── 工具方法 ────────────────────────────────────────────────

        private static string CleanForTts(string s)
        {
            // 去 markdown 粗体/斜体
            s = System.Text.RegularExpressions.Regex.Replace(s, @"[*_~`#]", "");
            // 去 emoji（简单过滤 surrogate pairs + 部分常见 Unicode 符号）
            var sb = new StringBuilder(s.Length);
            foreach (char c in s)
            {
                if (char.IsHighSurrogate(c) || char.IsLowSurrogate(c)) continue;
                if (c >= 0x2600 && c <= 0x27BF) continue;  // misc symbols
                if (c >= 0xFE00 && c <= 0xFE0F) continue;  // variation selectors
                sb.Append(c);
            }
            return sb.ToString().Trim();
        }

        private static string EscapeJson(string s) =>
            s.Replace("\\", "\\\\").Replace("\"", "\\\"").Replace("\n", "\\n").Replace("\r", "");

        private static string EscapeXml(string s) =>
            s.Replace("&", "&amp;").Replace("<", "&lt;").Replace(">", "&gt;")
             .Replace("\"", "&quot;").Replace("'", "&apos;");

        // ── OpenAI TTS 请求体 ──────────────────────────────────────
        [Serializable]
        private struct OpenAITtsRequest
        {
            public string model;
            public string input;
            public string voice;
            public float speed;
            public string response_format;
        }
    }
}
