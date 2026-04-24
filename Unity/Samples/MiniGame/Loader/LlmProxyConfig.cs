// LlmProxyConfig.cs
// ─────────────────────────────────────────────────────────────────────────────
// LLM 代理配置 — 保护 API Key 不被前端玩家抓包
//
// 生产环境必须走这个路径！否则 Key 会被：
//   - Chrome DevTools 抓包
//   - 微信开发者工具的 Network 面板
//   - 抓包工具 Charles / Fiddler
//   ... 烧光你的额度
//
// 正确架构：
//   Unity 游戏 ──► 你的代理服务器 ──► OpenAI/GLM/DeepSeek
//                  （Key 只存在于服务器端）
//
// 推荐代理方案（按成本排序）：
//
//   1. Cloudflare Workers（免费，全球 CDN）
//      配额：10 万次/天，完全够用
//      代码：30 行（见 ServerProxy/llm-proxy.worker.js）
//      域名：https://your-proxy.workers.dev
//
//   2. Vercel Edge Functions（免费）
//      配额：10 万次/天
//
//   3. Tencent Cloud 云函数（国内速度最快，微信小游戏推荐）
//      按量付费，每月 40 万次免费
//
// 本配置类职责：
//   - 把"用不用代理"这个选择做成可切换的开关
//   - 开发期：直连 LLM（填真实 Key，方便调试）
//   - 发布期：走代理（BaseURL 指向代理，ApiKey 留空或填玩家 token）
// ─────────────────────────────────────────────────────────────────────────────

using UnityEngine;

namespace BlueprintRuntime.Samples.MiniGame
{
    [CreateAssetMenu(fileName = "LlmConfig", menuName = "Blueprint/LLM Config", order = 0)]
    public class LlmProxyConfig : ScriptableObject
    {
        public enum Mode
        {
            /// <summary>直连 LLM 供应商（开发期用；API Key 暴露给前端）</summary>
            Direct,
            /// <summary>走你自己的代理服务器（生产推荐；API Key 在服务端）</summary>
            Proxy,
        }

        [Header("运行模式")]
        public Mode mode = Mode.Direct;

        [Header("Direct 模式（开发期）")]
        [Tooltip("LLM 供应商 API 基址。免费推荐：智谱 https://open.bigmodel.cn/api/paas/v4")]
        public string directBaseUrl = "https://open.bigmodel.cn/api/paas/v4";

        [Tooltip("API Key。注意：生产环境绝对不要留在这个字段，会被前端抓包")]
        public string directApiKey  = "";

        public string directModel   = "glm-4-flash";

        [Header("Proxy 模式（生产）")]
        [Tooltip("你的代理服务器 URL。如 https://llm-proxy.your-domain.workers.dev")]
        public string proxyBaseUrl  = "";

        [Tooltip("可选：玩家身份 token（用于代理端限流/鉴权）。留空则匿名。")]
        public string proxyClientToken = "";

        public string proxyModel   = "glm-4-flash";

        // ── 运行时属性（BlueprintService 用这个取配置） ─────────────
        public string EffectiveBaseUrl => mode == Mode.Direct ? directBaseUrl : proxyBaseUrl;
        public string EffectiveApiKey  => mode == Mode.Direct ? directApiKey  : proxyClientToken;
        public string EffectiveModel   => mode == Mode.Direct ? directModel   : proxyModel;

        public bool IsValid
        {
            get
            {
                if (mode == Mode.Direct)
                    return !string.IsNullOrEmpty(directBaseUrl) && !string.IsNullOrEmpty(directApiKey);
                return !string.IsNullOrEmpty(proxyBaseUrl);
            }
        }

        /// <summary>运行期切换模式（例如玩家在设置页切换）</summary>
        public void SwitchMode(Mode m) => mode = m;
    }
}
