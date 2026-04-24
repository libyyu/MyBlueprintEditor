// MiniGameBootstrap.cs
// ─────────────────────────────────────────────────────────────────────────────
// 小游戏启动入口 — 一站式初始化
//
// 挂到场景第一个激活的 GameObject（如 "Bootstrap"），Awake/Start 期间：
//   1. 初始化 BlueprintService（HTTP 客户端）
//   2. 加载 LlmProxyConfig ScriptableObject
//   3. 创建 BlueprintLoader 单例（CDN 动态蓝图）
//   4. 预加载激励视频广告（如配置）
//   5. 微信登录（如配置）
//
// 之后所有 NPC Controller 的 Start 里，直接用 BlueprintService.Instance.Llm* 即可。
// ─────────────────────────────────────────────────────────────────────────────

using System;
using UnityEngine;

namespace BlueprintRuntime.Samples.MiniGame
{
    [DefaultExecutionOrder(-100)]  // 早于一切 NPC 初始化
    public class MiniGameBootstrap : MonoBehaviour
    {
        [Header("LLM 配置")]
        [Tooltip("LlmProxyConfig ScriptableObject。Direct=开发，Proxy=生产")]
        [SerializeField] private LlmProxyConfig llmConfig;

        [Header("动态蓝图（可选）")]
        [Tooltip("从 CDN 加载蓝图的基地址。为空则不启用动态加载")]
        [SerializeField] private string blueprintCdnBaseUrl = "";

        [Header("微信小游戏（可选）")]
        [SerializeField] private bool autoLogin = false;
        [SerializeField] private string rewardAdUnitId = "";

        void Awake()
        {
            // 1. BlueprintService（若场景还没放）
            if (BlueprintService.Instance == null)
            {
                var svc = FindObjectOfType<BlueprintService>();
                if (svc == null)
                {
                    var go = new GameObject("BlueprintService");
                    go.AddComponent<BlueprintService>();
                }
            }

            // 2. BlueprintLoader（CDN 动态蓝图）
            if (!string.IsNullOrEmpty(blueprintCdnBaseUrl) && BlueprintLoader.Instance == null)
            {
                var go = new GameObject("BlueprintLoader");
                var loader = go.AddComponent<BlueprintLoader>();
                loader.BaseUrl = blueprintCdnBaseUrl;
            }
        }

        void Start()
        {
            // 3. 把 LlmConfig 推到 BlueprintService
            ApplyLlmConfig();

            // 4. 微信集成
#if UNITY_WEBGL && !UNITY_EDITOR
            if (WeChat.WeChatSDK.IsInWeChat)
            {
                if (autoLogin)
                    WeChat.WeChatSDK.Login(gameObject, nameof(OnWxLogin));

                if (!string.IsNullOrEmpty(rewardAdUnitId))
                    WeChat.WeChatSDK.PreloadRewardedVideoAd(rewardAdUnitId, gameObject, nameof(OnRewardedAd));
            }
#endif

            // 5. 注册所有自定义蓝图节点（在 BlueprintService 创建 Runner 后调用）
            RegisterCustomNodes();
        }

        /// <summary>注册所有游戏系统的蓝图节点。
        /// 由 Bootstrap 统一管理，新增系统只需在这里加一行。</summary>
        private void RegisterCustomNodes()
        {
            // 订阅 BlueprintService 的 Runner 创建事件
            // 由于 Service 管理多 Runner，每个 Runner 都需注册
            if (BlueprintService.Instance != null)
            {
                BlueprintService.Instance.OnRunnerCreated += runner =>
                {
                    WeChat.BlueprintWeChatNodes.RegisterAll(runner);
                    Quest.BlueprintQuestNodes.RegisterAll(runner);
                    BlueprintItemNodes.RegisterAll(runner);
                    BlueprintPlayerNodes.RegisterAll(runner);
                    BlueprintShopNodes.RegisterAll(runner);
                    BlueprintMemoryNodes.RegisterAll(runner);
                    BlueprintTutorialNodes.RegisterAll(runner);
                };
            }
        }

        private void ApplyLlmConfig()
        {
            if (llmConfig == null)
            {
                Debug.LogWarning("[MiniGameBootstrap] LlmConfig 未设置，NPC 对话无法工作");
                return;
            }
            if (!llmConfig.IsValid)
            {
                Debug.LogWarning($"[MiniGameBootstrap] LlmConfig ({llmConfig.mode}) 配置不完整");
            }

            // 通过反射设 BlueprintService 的 llmApiKey 字段（因为它是 private SerializeField）
            // 更干净的做法：让 BlueprintService 加个公共 SetLlm 方法
            var svc = BlueprintService.Instance;
            if (svc != null)
            {
                // BlueprintService 已有 SetApiKey；这里直接用它
                svc.SetApiKey(llmConfig.EffectiveApiKey);
                // BaseURL/Model 的注入通过 NPC.Start 时读 svc 的只读属性完成
                // 但那些是 [SerializeField] 的 private 字段，所以我们需要改一下 —
                // 在 BlueprintService 加一个 OverrideLlm 公共方法（下面代码不依赖反射）
                svc.OverrideLlm(llmConfig.EffectiveBaseUrl, llmConfig.EffectiveApiKey, llmConfig.EffectiveModel);
            }
        }

        // ── 微信回调 ─────────────────────────────────────────────────
        public void OnWxLogin(string json)
        {
            Debug.Log($"[MiniGameBootstrap] wx.login result: {json}");
            // TODO: 把 code 发到你的服务器换 openid，用来做账号绑定
        }

        public void OnRewardedAd(string result)
        {
            // result: "ok"=看完 / "close"=中途关闭 / "error:..." =失败
            Debug.Log($"[MiniGameBootstrap] rewardedAd: {result}");
            if (result == "ok")
            {
                OnRewardGranted?.Invoke();
            }
        }

        /// <summary>玩家完整看完激励视频广告时触发。在这里发奖励。</summary>
        public event Action OnRewardGranted;

        /// <summary>外部触发看广告</summary>
        public void ShowRewardedVideo() => WeChat.WeChatSDK.ShowRewardedVideoAd();
    }
}
