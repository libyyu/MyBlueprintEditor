/**
 * wecom_bridge.js
 * 企业微信 ↔ Blueprint MCP 桥接服务
 *
 * 功能：
 *   - 通过 @wecom/aibot-node-sdk 长连接接收企微消息
 *   - 解析路由规则，选择合适的蓝图
 *   - 调用 Blueprint MCP HTTP Server (mcp_server.py --http) 执行蓝图
 *   - 将蓝图 output[] 流式/非流式回复给企微用户
 *
 * 启动：
 *   node wecom_bridge.js [--config /path/to/config.json]
 */

'use strict';

const fs      = require('fs');
const path    = require('path');
const axios   = require('axios');
const AiBot   = require('@wecom/aibot-node-sdk');
const { generateReqId } = require('@wecom/aibot-node-sdk');

// ─── 配置加载 ───────────────────────────────────────────────────────────────

function loadConfig() {
  const args = process.argv.slice(2);
  let configPath = null;
  for (let i = 0; i < args.length; i++) {
    if (args[i] === '--config' && args[i + 1]) {
      configPath = path.resolve(args[i + 1]);
      break;
    }
  }
  if (!configPath) {
    configPath = path.join(__dirname, 'config.json');
  }
  if (!fs.existsSync(configPath)) {
    console.error(`[WeComBridge] Config file not found: ${configPath}`);
    console.error(`[WeComBridge] Copy config.example.json to config.json and fill in your credentials.`);
    process.exit(1);
  }
  const raw = fs.readFileSync(configPath, 'utf-8');
  // 去掉 JSON5 风格注释（_comment 字段）
  return JSON.parse(raw);
}

const cfg = loadConfig();
const DEBUG = cfg.debug === true;

function dbg(...args) {
  if (DEBUG) console.log('[DBG]', ...args);
}

// ─── Blueprint MCP 客户端 ────────────────────────────────────────────────────

const BP_URL     = cfg.blueprint?.mcpServerUrl || 'http://localhost:7799';
const BP_DEFAULT = cfg.blueprint?.defaultBlueprint || '';
const BP_QVAR    = cfg.blueprint?.queryVariableName || 'UserQuery';
const BP_FIXED   = cfg.blueprint?.fixedVariables || {};
const BP_ROUTES  = cfg.blueprint?.routes || {};
const BP_TIMEOUT = cfg.blueprint?.timeoutMs || 120000;

/** 根据用户文本选择蓝图路径 */
function resolveBlueprint(text) {
  // 长关键词优先
  const keys = Object.keys(BP_ROUTES).sort((a, b) => b.length - a.length);
  for (const kw of keys) {
    if (text.includes(kw)) {
      dbg(`Route matched: "${kw}" → ${BP_ROUTES[kw]}`);
      return BP_ROUTES[kw];
    }
  }
  return BP_DEFAULT;
}

/** 调用 Blueprint MCP HTTP 接口 */
async function callBlueprint(blueprintPath, userText) {
  const variables = { ...BP_FIXED, [BP_QVAR]: userText };
  dbg(`callBlueprint: ${blueprintPath}`, variables);

  const resp = await axios.post(
    `${BP_URL}/tool`,
    { tool: 'execute_blueprint', arguments: { path: blueprintPath, variables } },
    { timeout: BP_TIMEOUT }
  );

  const data = resp.data;
  if (data.error) throw new Error(data.error);

  // 提取 output 文本
  const items = data.result || [];
  const parts = [];
  for (const item of items) {
    if (item.type !== 'text') continue;
    try {
      const parsed = typeof item.text === 'string' ? JSON.parse(item.text) : item.text;
      if (parsed.error) {
        parts.push(`❌ ${parsed.error}`);
      } else if (Array.isArray(parsed.output) && parsed.output.length > 0) {
        parts.push(...parsed.output);
      } else if (parsed.value != null) {
        parts.push(String(parsed.value));
      } else {
        parts.push(JSON.stringify(parsed));
      }
    } catch {
      parts.push(item.text);
    }
  }
  return parts.join('\n').trim() || '（蓝图执行完成，无文本输出）';
}

// ─── 内置命令 ────────────────────────────────────────────────────────────────

const COMMANDS = {
  help: async (args, ctx) => {
    const lines = [
      '📋 **可用命令**\n',
      '**-help (-h)**　　显示此帮助',
      '**-status (-st)**　显示服务状态',
      '**-ping (-p)**　　测试连通性',
      '**-blueprint (-bp)** <路径>　指定蓝图执行（覆盖路由）',
      '**-list (-ls)**　　列出可用蓝图',
      '',
      '不以 `-` 开头的消息直接送入蓝图处理。',
    ];
    return lines.join('\n');
  },

  status: async (args, ctx) => {
    const uptime = process.uptime();
    const h = Math.floor(uptime / 3600);
    const m = Math.floor((uptime % 3600) / 60);
    const s = Math.floor(uptime % 60);
    // 健康检查 MCP server
    let mcpStatus = '❓ 未知';
    try {
      const r = await axios.get(`${BP_URL}/health`, { timeout: 3000 });
      mcpStatus = r.data?.status === 'ok' ? '✅ 正常' : '⚠️ ' + JSON.stringify(r.data);
    } catch (e) {
      mcpStatus = `❌ 无法连接 (${e.message})`;
    }
    return [
      '📊 **WeComBridge 状态**\n',
      `**运行时间**: ${h}h ${m}m ${s}s`,
      `**MCP Server**: ${BP_URL}  ${mcpStatus}`,
      `**默认蓝图**: ${BP_DEFAULT || '（未配置）'}`,
      `**路由规则**: ${Object.keys(BP_ROUTES).join(' | ') || '无'}`,
    ].join('\n');
  },

  ping: async () => '🏓 pong!',

  blueprint: async (args, ctx) => {
    const bpPath = args.trim();
    if (!bpPath) return '❌ 用法: -blueprint <蓝图路径>\n例如: -blueprint data/examples/WebSearchAgent.bjson';
    ctx.overrideBlueprintPath = bpPath;
    return null; // 继续走正常蓝图执行流程
  },

  list: async () => {
    try {
      const r = await axios.post(`${BP_URL}/tool`, { tool: 'list_blueprints', arguments: {} }, { timeout: 10000 });
      const items = r.data?.result || [];
      const bps = [];
      for (const item of items) {
        try {
          const parsed = typeof item.text === 'string' ? JSON.parse(item.text) : item.text;
          if (Array.isArray(parsed)) {
            for (const bp of parsed) bps.push(`• ${bp.path}  (${bp.size_bytes} B)`);
          }
        } catch {}
      }
      return bps.length > 0
        ? '📂 **可用蓝图**\n\n' + bps.join('\n')
        : '（暂无蓝图）';
    } catch (e) {
      return `❌ 获取蓝图列表失败: ${e.message}`;
    }
  },
};

// 别名
COMMANDS.h  = COMMANDS.help;
COMMANDS.st = COMMANDS.status;
COMMANDS.p  = COMMANDS.ping;
COMMANDS.bp = COMMANDS.blueprint;
COMMANDS.ls = COMMANDS.list;

/** 解析命令，返回 {name, args} 或 null */
function parseCommand(text) {
  const trimmed = text.trim();
  if (!trimmed.startsWith('-')) return null;
  const m = trimmed.match(/^-(\w+)\s*(.*)/s);
  if (!m) return null;
  return { name: m[1].toLowerCase(), args: m[2] || '' };
}

// ─── Webhook 模式 ────────────────────────────────────────────────────────────

async function startWebhookMode() {
  const webhookUrl = cfg.wecom?.webhook?.url;
  if (!webhookUrl) {
    console.error('[WeComBridge] webhook.url is required in webhook mode');
    process.exit(1);
  }
  console.log('[WeComBridge] Webhook mode: can only SEND messages (no incoming messages).');
  console.log('[WeComBridge] Use long_connection mode for bidirectional messaging.');

  // Webhook 模式无法接收消息，只能定时/主动推送
  // 这里仅保留接口，实际使用场景请通过蓝图内的 HTTP.Post 节点推送
}

// ─── 长连接模式 ──────────────────────────────────────────────────────────────

// 去重：已处理的 msgid
const processedIds = new Set();
setInterval(() => {
  if (processedIds.size > 2000) processedIds.clear();
}, 60000);

// 待回复帧（msgid → {frame, ts}）
const pendingFrames = new Map();
const FRAME_EXPIRY = 15 * 60 * 1000;
setInterval(() => {
  const now = Date.now();
  for (const [id, entry] of pendingFrames) {
    if (now - entry.ts > FRAME_EXPIRY) pendingFrames.delete(id);
  }
}, 60000);

async function replyToFrame(wsClient, frame, text) {
  const streamId = generateReqId('stream');
  await wsClient.replyStream(frame, streamId, text, true);
}

async function sendNotification(text) {
  const notifyUrl = cfg.wecom?.notifyWebhookUrl;
  if (!notifyUrl) return;
  try {
    await axios.post(notifyUrl, {
      msgtype: 'markdown',
      markdown: { content: text }
    });
  } catch (e) {
    dbg('Notification failed:', e.message);
  }
}

async function startLongConnectionMode() {
  const lcCfg = cfg.wecom?.long_connection;
  if (!lcCfg?.botId || !lcCfg?.secret) {
    console.error('[WeComBridge] long_connection.botId and secret are required');
    process.exit(1);
  }

  console.log(`[WeComBridge] Starting long connection, botId: ${lcCfg.botId.slice(0, 6)}***`);

  const wsClient = new AiBot.WSClient({
    botId:  lcCfg.botId,
    secret: lcCfg.secret,
    maxReconnectAttempts: -1,
    logger: {
      debug: (...a) => dbg('[SDK]', ...a),
      info:  (...a) => console.log('[SDK]', ...a),
      warn:  (...a) => console.warn('[SDK]', ...a),
      error: (...a) => console.error('[SDK]', ...a),
    }
  });

  wsClient.on('connected',      () => console.log('[WeComBridge] WebSocket connected'));
  wsClient.on('authenticated',  () => {
    console.log('[WeComBridge] Authenticated');
    sendNotification('## ✅ WeComBridge 启动成功\n\nBlueprint MCP 桥接服务已就绪\n' +
      `> 时间: ${new Date().toLocaleString('zh-CN')}`);
  });
  wsClient.on('disconnected',   r => {
    console.log('[WeComBridge] Disconnected:', r);
    sendNotification(`## 🔴 WeComBridge 断开连接\n\n原因: ${r}\n> 时间: ${new Date().toLocaleString('zh-CN')}`);
  });
  wsClient.on('reconnecting',   n => console.log(`[WeComBridge] Reconnecting #${n}...`));
  wsClient.on('error',          e => console.error('[WeComBridge] Error:', e));

  // 欢迎语
  wsClient.on('event.enter_chat', async (frame) => {
    try {
      await wsClient.replyWelcome(frame, {
        msgtype: 'text',
        text: { content: '👋 您好！我是蓝图智能助手。\n\n直接发送问题即可，或输入 -help 查看命令。' }
      });
    } catch (e) { dbg('Welcome error:', e.message); }
  });

  // 消息处理（文本/混合/语音）
  for (const evtType of ['message.text', 'message.mixed', 'message.voice']) {
    wsClient.on(evtType, async (frame) => {
      await handleIncoming(wsClient, frame, evtType).catch(e => {
        console.error('[WeComBridge] handleIncoming error:', e.message);
      });
    });
  }

  wsClient.connect();

  // 优雅退出
  async function shutdown(sig) {
    console.log(`[WeComBridge] ${sig} received, shutting down...`);
    await sendNotification('## 🔴 WeComBridge 进程退出\n> 时间: ' + new Date().toLocaleString('zh-CN'));
    try { wsClient.disconnect(); } catch {}
    setTimeout(() => process.exit(0), 1500);
  }
  process.on('SIGINT',  () => shutdown('SIGINT'));
  process.on('SIGTERM', () => shutdown('SIGTERM'));
}

// ─── 消息处理核心 ─────────────────────────────────────────────────────────────

async function handleIncoming(wsClient, frame, evtType) {
  const body  = frame.body;
  const msgid = body?.msgid;
  if (!msgid) return;

  // 去重
  if (processedIds.has(msgid)) { dbg('Dup:', msgid); return; }
  processedIds.add(msgid);

  // 提取文本
  let text = '';
  if (evtType === 'message.text') {
    text = body?.text?.content || '';
  } else if (evtType === 'message.mixed') {
    const items = body?.mixed_msg?.item || [];
    text = items.filter(i => i.msg_type === 'text').map(i => i.text?.content || '').join('\n');
  } else if (evtType === 'message.voice') {
    text = body?.voice?.translated_content || '';
  }

  const from = body?.from?.userid || 'unknown';
  console.log(`[WeComBridge] ← ${from}: "${text.slice(0, 80)}"`);

  // 保存待回复帧
  pendingFrames.set(msgid, { frame, ts: Date.now() });

  // 命令处理
  const cmd = parseCommand(text);
  if (cmd) {
    const handler = COMMANDS[cmd.name];
    if (!handler) {
      await replyToFrame(wsClient, frame, `❌ 未知命令: -${cmd.name}\n输入 -help 查看可用命令`);
      pendingFrames.delete(msgid);
      return;
    }
    const ctx = {};
    const cmdResult = await handler(cmd.args, ctx);

    // blueprint 命令返回 null 表示继续执行蓝图，overrideBlueprintPath 已设置
    if (cmdResult !== null) {
      await replyToFrame(wsClient, frame, cmdResult);
      pendingFrames.delete(msgid);
      return;
    }
    // 继续，使用 ctx.overrideBlueprintPath
    await executeBlueprintAndReply(wsClient, frame, msgid, text, ctx.overrideBlueprintPath);
    return;
  }

  // 普通消息 → 蓝图
  await executeBlueprintAndReply(wsClient, frame, msgid, text, null);
}

async function executeBlueprintAndReply(wsClient, frame, msgid, text, overridePath) {
  // 发送思考提示
  const streamId = generateReqId('stream');
  try {
    await wsClient.replyStream(frame, streamId, '⏳ 正在处理，请稍候...', false);
  } catch {}

  let reply;
  try {
    const bpPath = overridePath || resolveBlueprint(text);
    if (!bpPath) {
      reply = '⚠️ 未配置默认蓝图。请管理员设置 config.json 中的 blueprint.defaultBlueprint。';
    } else {
      console.log(`[WeComBridge] → Blueprint: ${bpPath}`);
      reply = await callBlueprint(bpPath, text);
    }
  } catch (e) {
    console.error('[WeComBridge] Blueprint error:', e.message);
    reply = `❌ 蓝图执行失败: ${e.message}`;
  }

  // 回复（finish the stream）
  try {
    await wsClient.replyStream(frame, streamId, reply, true);
    console.log(`[WeComBridge] → Reply sent (${reply.length} chars)`);
  } catch (e) {
    console.error('[WeComBridge] Reply error:', e.message);
  }

  pendingFrames.delete(msgid);
}

// ─── 入口 ─────────────────────────────────────────────────────────────────────

async function main() {
  console.log('=== WeComBridge — Blueprint MCP Gateway ===');
  console.log(`MCP Server : ${BP_URL}`);
  console.log(`Default BP : ${BP_DEFAULT || '（未配置）'}`);
  console.log(`Routes     : ${JSON.stringify(BP_ROUTES)}`);

  // 健康检查 MCP Server
  try {
    const r = await axios.get(`${BP_URL}/health`, { timeout: 5000 });
    console.log(`[WeComBridge] MCP Server health: ${JSON.stringify(r.data)}`);
  } catch {
    console.warn(`[WeComBridge] ⚠️  MCP Server not reachable at ${BP_URL}`);
    console.warn(`[WeComBridge]    Start it first: python tools/mcp_server.py --http --port 7799`);
  }

  const mode = cfg.wecom?.mode || 'long_connection';
  console.log(`[WeComBridge] Mode: ${mode}`);

  if (mode === 'webhook') {
    await startWebhookMode();
  } else {
    await startLongConnectionMode();
  }
}

main().catch(e => {
  console.error('[WeComBridge] Fatal:', e);
  process.exit(1);
});
