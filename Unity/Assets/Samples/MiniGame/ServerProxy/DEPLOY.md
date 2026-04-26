# Cloudflare Worker LLM 代理部署教程

> 10 分钟上线、免费 10 万次/天、保护你的 API Key 不被前端抓包。

## 为什么必须用代理？

Unity WebGL / 微信小游戏是**前端代码**，玩家能抓包：
- Chrome DevTools F12 → Network
- 微信开发者工具 → Network 面板
- Charles / Fiddler（更高级）

如果你把 API Key 填在 Unity 里发布，**会在几小时内被烧光额度**。

---

## 架构

```
玩家手机（Unity WebGL / 小游戏）
          │
          │  1. POST https://你的-worker.workers.dev/chat/completions
          │     Body: {model, messages, ...}
          │     （不带真 API Key）
          ▼
   Cloudflare Worker（本教程部署的）
          │
          │  2. 注入真 API Key，转发到上游 LLM
          │
          ▼
   智谱 / DeepSeek / 通义 / OpenAI
          │
          │  3. 响应原样返回
          ▼
   Worker → Unity（透明，游戏不感知有代理）
```

---

## 步骤 1：准备 LLM API Key

任选一家：

| 供应商 | 免费额度 | 注册地址 |
|---|---|---|
| **智谱 GLM-4-Flash**（推荐） | **永久免费无限量** | <https://open.bigmodel.cn> |
| DeepSeek | 新人 500 万 tokens | <https://platform.deepseek.com> |
| 通义千问 DashScope | 月度免费额度 | <https://dashscope.aliyuncs.com> |

注册后拿到 API Key（`sk-xxx` 或类似格式），**先保存到记事本**。

---

## 步骤 2：注册 Cloudflare 账号

打开 <https://dash.cloudflare.com/sign-up> 注册（需要邮箱）。

**不需要备案，不需要信用卡**，直接免费用。

---

## 步骤 3：创建 Worker

1. 登录后左侧菜单点 **Workers & Pages** → **Create** → **Create Worker**
2. 给 Worker 起个名字，如 `llm-proxy-your-game`
3. 点 **Deploy**（先部署一个默认 Hello World）
4. 部署成功后能看到 URL：`https://llm-proxy-your-game.你的用户名.workers.dev`

---

## 步骤 4：粘贴代理代码

1. 回到 Worker 详情页，点右上 **Edit code**
2. 左侧编辑器里**全选删除**默认代码
3. 打开本仓库的 `Unity/Samples/MiniGame/ServerProxy/llm-proxy.worker.js`
4. **全文拷贝**粘贴到 Worker 编辑器
5. 右上点 **Save and deploy**

---

## 步骤 5：配置环境变量

这一步把 API Key "藏"到服务端，不会暴露给前端。

1. Worker 详情页 → **Settings** → **Variables** → **Environment Variables**
2. 点 **Add variable**，添加三个：

| Variable name | Value | Type |
|---|---|---|
| `UPSTREAM_BASE` | `https://open.bigmodel.cn/api/paas/v4` | Plaintext |
| `UPSTREAM_API_KEY` | `你在步骤1拿到的 API Key` | **Encrypted** |
| `ALLOWED_ORIGINS` | `https://servicewechat.com,https://你的游戏域名.com` | Plaintext |

> **⚠️ 重要**：`UPSTREAM_API_KEY` 务必点 **Encrypt** 图标（锁标记）。加密后连你自己都看不到明文，更安全。

3. 点 **Save and deploy**

### `ALLOWED_ORIGINS` 详解

- 微信小游戏的 `Origin` 是 `https://servicewechat.com/`
- 浏览器 WebGL 的 `Origin` 是你的游戏网站域名
- 多个用英文逗号分隔
- **留空 = 允许所有 Origin**（仅开发用，上线前必须设置）

---

## 步骤 6：测试

用 curl 或任何 HTTP 客户端测试：

```bash
curl -X POST https://llm-proxy-your-game.你的用户名.workers.dev/chat/completions \
  -H "Content-Type: application/json" \
  -d '{
    "model": "glm-4-flash",
    "messages": [{"role":"user","content":"你好"}]
  }'
```

正常应该返回类似：

```json
{
  "choices": [{"message": {"role":"assistant","content":"你好！有什么我能帮你的吗？"}}],
  ...
}
```

如果返回 `Origin not allowed`，把你用的域名加到 `ALLOWED_ORIGINS`（开发测试可以临时设为 `*`）。

---

## 步骤 7：在 Unity 里切换到 Proxy 模式

1. 打开你的 `LlmConfig` ScriptableObject（Assets/Create → Blueprint → LLM Config）
2. Inspector：
   - **Mode**: 改为 `Proxy`
   - **Proxy Base Url**: `https://llm-proxy-your-game.你的用户名.workers.dev`
   - **Proxy Client Token**: 留空（或填你自己的鉴权 token）
   - **Proxy Model**: `glm-4-flash`
3. 直接 Play — NPC 对话正常工作，**API Key 已完全不在前端**

---

## 步骤 8：微信小游戏白名单

微信公众平台 → 开发设置 → **request 合法域名** → 添加：

```
https://llm-proxy-your-game.你的用户名.workers.dev
```

**注意**：微信限制 request 域名只能是**根域名**（不能是路径）。

---

## 进阶：IP 限流

本教程的 worker.js 已集成简易限流（30 次/分钟/IP），需要绑定 KV Namespace 才能生效：

1. Cloudflare 左侧 → **Workers & Pages** → **KV**
2. **Create a namespace**，命名 `rate_limit`
3. 回到 Worker 详情 → **Settings** → **Variables** → **KV Namespace Bindings**
4. Add binding：
   - Variable name: `RATE_LIMIT_KV`
   - KV namespace: 选刚才创建的 `rate_limit`
5. **Save and deploy**

现在每 IP 每分钟最多 30 次请求，超过返回 429。

## 进阶：按玩家 token 限流

如果你有后端用户系统，可以：

1. 登录后服务端下发一个 `userToken`（JWT 或随机字符串）
2. Unity 里 `LlmConfig.Proxy Client Token = userToken`
3. Worker 解析 `Authorization: Bearer xxx`，查数据库验证 + 每用户限流

参考 `llm-proxy.worker.js` 中 "玩家 token 鉴权（可选）" 部分解开注释。

## 进阶：内容安全过滤

在 Worker 转发 LLM 响应之前加一道敏感词/内容安全 API：

```js
// 例如调用腾讯云内容安全 API
const safeResp = await fetch('https://cms.tencentcloudapi.com/...', {
  method: 'POST',
  body: JSON.stringify({ text: llmResponseContent }),
});
if ((await safeResp.json()).Label !== 'Normal') {
  return new Response('{"error":"content blocked"}', { status: 451 });
}
```

微信小游戏过审 99% 卡在内容安全，这一步能大幅提高过审率。

---

## 成本预估

| 用量 | Cloudflare Worker | 智谱 GLM-4-Flash |
|---|---|---|
| 1 万 DAU × 10 次对话/天 | **免费**（在 10 万次/天内） | **免费** |
| 10 万 DAU × 10 次对话/天 | $5/月（100 万次/月按量） | **免费** |
| 100 万 DAU × 10 次对话/天 | $50/月 | 建议换付费 GLM-4（仍比 OpenAI 便宜 10 倍） |

**结论**：10 万 DAU 以内，整条链路几乎 0 成本。

---

## 常见问题

### Q: Cloudflare Worker 在国内会被墙吗？
Cloudflare 部分边缘节点在国内访问有延迟（200~500ms），但不墙。如果对国内体验要求高：
- 方案 1：改用**腾讯云 SCF（云函数）**，国内延迟 < 50ms，每月 40 万次免费
- 方案 2：阿里云函数计算，类似

把 `llm-proxy.worker.js` 翻成 Node.js Express app，10 分钟就能迁过去。

### Q: 能缓存 LLM 响应吗？
可以。相同 Prompt 的响应可以缓存到 KV：

```js
const cacheKey = `llm:${hash(body)}`;
const cached = await env.RATE_LIMIT_KV.get(cacheKey);
if (cached) return new Response(cached, { headers: {...} });
// ... 转发 + 缓存响应
ctx.waitUntil(env.RATE_LIMIT_KV.put(cacheKey, respText, { expirationTtl: 3600 }));
```

热门问题命中率能到 30%+，直接省一半成本。

### Q: 怎么监控哪些玩家在烧我的额度？
Worker 里加日志记到 Cloudflare Logpush：

```js
console.log(JSON.stringify({ ip, userToken, tokens: estimateTokens(body) }));
```

付费版 $5/月的 Workers Paid plan 送 Analytics，能看每个 IP 的请求次数。

---

## 总结

你只需要做 3 件事就能上线一个**安全、免费、可扩展**的 LLM 代理：

1. 拷贝 `llm-proxy.worker.js` 到 Cloudflare Worker
2. 设 3 个环境变量（`UPSTREAM_BASE` / `UPSTREAM_API_KEY` / `ALLOWED_ORIGINS`）
3. Unity 里 `LlmConfig.Mode` 切到 `Proxy`

完成。
