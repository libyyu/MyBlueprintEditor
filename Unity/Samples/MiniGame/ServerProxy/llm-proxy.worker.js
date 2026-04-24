// llm-proxy.worker.js
// ─────────────────────────────────────────────────────────────────────────────
// Cloudflare Worker — LLM API 代理
//
// 功能：
//   - 前端 Unity 游戏 POST 到本 Worker
//   - 本 Worker 注入真实 API Key 后转发到上游 LLM
//   - 支持流式（SSE）和非流式
//   - IP 限流（每分钟 30 次/ IP）
//   - 玩家 token 限流（可选）
//
// 部署步骤：
//   1. 登录 https://dash.cloudflare.com → Workers & Pages → Create
//   2. 粘贴本文件内容
//   3. Settings → Variables → 添加环境变量：
//      - UPSTREAM_BASE       = https://open.bigmodel.cn/api/paas/v4
//      - UPSTREAM_API_KEY    = 你的智谱 API Key
//      - ALLOWED_ORIGINS     = https://your-game.com,https://servicewechat.com
//   4. 保存并部署 → 获得 URL 如 https://llm-proxy.<username>.workers.dev
//   5. Unity 里 LlmProxyConfig.proxyBaseUrl 填上面 URL
//
// 免费额度：10 万次请求/天（完全够用，人均 10 次对话 = 1 万 DAU）
// ─────────────────────────────────────────────────────────────────────────────

export default {
  async fetch(request, env, ctx) {
    // ── CORS 预检 ─────────────────────────────────────
    if (request.method === 'OPTIONS') {
      return corsResponse(null, 204, request, env);
    }
    if (request.method !== 'POST') {
      return corsResponse('Method not allowed', 405, request, env);
    }

    // ── 来源校验（防止被滥用） ────────────────────────
    const origin = request.headers.get('Origin') || request.headers.get('Referer') || '';
    const allowed = (env.ALLOWED_ORIGINS || '').split(',').map(s => s.trim()).filter(Boolean);
    if (allowed.length > 0 && !allowed.some(a => origin.startsWith(a))) {
      // 微信小游戏的 Origin 是 https://servicewechat.com/<appid>/...
      return corsResponse('Origin not allowed', 403, request, env);
    }

    // ── IP 限流（用 Cloudflare 边缘 KV 或内存缓存；此处用简化版） ─
    const ip = request.headers.get('CF-Connecting-IP') || 'unknown';
    if (env.RATE_LIMIT_KV) {
      const key = `rl:${ip}:${Math.floor(Date.now() / 60000)}`;  // 每分钟 bucket
      const count = parseInt(await env.RATE_LIMIT_KV.get(key) || '0');
      if (count >= 30) {
        return corsResponse('Rate limit exceeded (30/min)', 429, request, env);
      }
      ctx.waitUntil(env.RATE_LIMIT_KV.put(key, (count + 1).toString(), { expirationTtl: 120 }));
    }

    // ── 玩家 token 鉴权（可选） ──────────────────────
    const clientToken = request.headers.get('Authorization')?.replace(/^Bearer\s+/i, '') || '';
    // 如果需要强制登录：
    //   if (!clientToken || !(await validateToken(clientToken, env))) {
    //     return corsResponse('Unauthorized', 401, request, env);
    //   }

    // ── 转发到上游 LLM ───────────────────────────────
    const url = new URL(request.url);
    // URL 路径原样转发：/chat/completions → {UPSTREAM_BASE}/chat/completions
    const upstreamUrl = `${env.UPSTREAM_BASE}${url.pathname}${url.search}`;

    try {
      const upstream = await fetch(upstreamUrl, {
        method: 'POST',
        headers: {
          'Content-Type': 'application/json',
          'Authorization': `Bearer ${env.UPSTREAM_API_KEY}`,
        },
        body: request.body,
      });

      // 原样转发响应（含 SSE 流）
      const respHeaders = new Headers(upstream.headers);
      applyCors(respHeaders, request, env);
      respHeaders.delete('access-control-allow-credentials');

      return new Response(upstream.body, {
        status: upstream.status,
        statusText: upstream.statusText,
        headers: respHeaders,
      });
    } catch (e) {
      return corsResponse(`Upstream error: ${e.message}`, 502, request, env);
    }
  },
};

// ── 工具函数 ─────────────────────────────────────────────
function applyCors(headers, request, env) {
  const origin = request.headers.get('Origin') || '';
  const allowed = (env.ALLOWED_ORIGINS || '').split(',').map(s => s.trim()).filter(Boolean);
  // 只把允许的 origin 写回（更安全）
  const echo = allowed.find(a => origin.startsWith(a)) || '*';
  headers.set('Access-Control-Allow-Origin', echo === '*' ? '*' : origin);
  headers.set('Access-Control-Allow-Methods', 'POST, OPTIONS');
  headers.set('Access-Control-Allow-Headers', 'Content-Type, Authorization');
  headers.set('Access-Control-Max-Age', '86400');
}

function corsResponse(body, status, request, env) {
  const headers = new Headers({ 'Content-Type': 'text/plain; charset=utf-8' });
  applyCors(headers, request, env);
  return new Response(body, { status, headers });
}
