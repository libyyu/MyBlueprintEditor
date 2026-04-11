/**
 * wecom_to_blueprint.js
 * 极简企微桥接：仅负责企微鉴权 + 消息转发到 Blueprint HTTP Server
 *
 * 架构：
 *   企微 AI机器人 ──WebSocket SDK──> 本脚本 ──HTTP POST──> 蓝图(WeComBot.bjson)
 *
 * 蓝图处理所有业务逻辑（LLM调用、工具调用等），本脚本只做协议转换。
 *
 * 配置（config.json 里加）：
 *   "wecom_blueprint": {
 *     "botId":  "your-bot-id",
 *     "secret": "your-secret",
 *     "blueprintUrl": "http://127.0.0.1:7800"
 *   }
 *
 * 启动：
 *   node wecom_to_blueprint.js [--config /path/to/config.json]
 */

'use strict';

const fs    = require('fs');
const path  = require('path');
const axios = require('axios');
const AiBot = require('@wecom/aibot-node-sdk');
const { generateReqId } = require('@wecom/aibot-node-sdk');

// ── 配置加载 ──────────────────────────────────────────────────────────────────
const args = process.argv.slice(2);
let configPath = null;
for (let i = 0; i < args.length; i++) {
  if (args[i] === '--config' && args[i + 1]) { configPath = path.resolve(args[i + 1]); break; }
}
if (!configPath) configPath = path.join(__dirname, 'config.json');
if (!fs.existsSync(configPath)) {
  console.error(`[WeComBP] Config not found: ${configPath}`);
  process.exit(1);
}
const cfg     = JSON.parse(fs.readFileSync(configPath, 'utf-8'));
const bpCfg   = cfg.wecom_blueprint || {};
const BOT_ID  = bpCfg.botId  || cfg.wecom?.long_connection?.botId;
const SECRET  = bpCfg.secret || cfg.wecom?.long_connection?.secret;
const BP_URL  = bpCfg.blueprintUrl || 'http://127.0.0.1:7800';
const TIMEOUT = bpCfg.timeoutMs   || 120000;
const DEBUG   = cfg.debug === true;

if (!BOT_ID || !SECRET) {
  console.error('[WeComBP] botId and secret are required. Set wecom_blueprint.botId / secret in config.json');
  process.exit(1);
}

const dbg = (...a) => { if (DEBUG) console.log('[DBG]', ...a); };

// ── 消息去重 ───────────────────────────────────────────────────────────────────
const seen = new Set();
setInterval(() => { if (seen.size > 2000) seen.clear(); }, 60000);

// ── 转发消息到蓝图 HTTP Server ─────────────────────────────────────────────────
async function forwardToBlueprint(body) {
  dbg('→ Blueprint POST:', JSON.stringify(body).slice(0, 120));
  const resp = await axios.post(BP_URL, body, {
    timeout: TIMEOUT,
    headers: { 'Content-Type': 'application/json' }
  });
  return resp.data;
}

// ── 企微消息处理 ───────────────────────────────────────────────────────────────
async function handleMessage(wsClient, frame, evtType) {
  const body  = frame.body;
  const msgid = body?.msgid;
  if (!msgid || seen.has(msgid)) return;
  seen.add(msgid);

  let text = '';
  if (evtType === 'message.text') {
    text = body?.text?.content || '';
  } else if (evtType === 'message.mixed') {
    text = (body?.mixed_msg?.item || [])
      .filter(i => i.msg_type === 'text')
      .map(i => i.text?.content || '')
      .join('\n');
  } else if (evtType === 'message.voice') {
    text = body?.voice?.translated_content || '';
  }

  if (!text.trim()) return;

  const from = body?.from?.userid || 'unknown';
  console.log(`[WeComBP] ← ${from}: "${text.slice(0, 80)}"`);

  const streamId = generateReqId('stream');
  // 先回复"处理中"
  try {
    await wsClient.replyStream(frame, streamId, '⏳ 正在处理，请稍候...', false);
  } catch {}

  let reply = '';
  try {
    // 把完整的企微消息 body 转发给蓝图，蓝图自己解析
    const result = await forwardToBlueprint(body);
    reply = result?.reply || '';
    if (!reply) reply = '（蓝图无回复）';
  } catch (e) {
    console.error('[WeComBP] Blueprint error:', e.message);
    reply = `❌ 处理失败: ${e.message}`;
  }

  try {
    await wsClient.replyStream(frame, streamId, reply, true);
    console.log(`[WeComBP] → Replied (${reply.length} chars)`);
  } catch (e) {
    console.error('[WeComBP] Reply error:', e.message);
  }
}

// ── 启动 ───────────────────────────────────────────────────────────────────────
async function main() {
  console.log('=== WeComBP — Blueprint Gateway (minimal) ===');
  console.log(`Bot ID : ${BOT_ID.slice(0, 6)}***`);
  console.log(`BP URL : ${BP_URL}`);

  // 健康检查蓝图服务
  try {
    await axios.get(BP_URL, { timeout: 3000 });
    console.log('[WeComBP] Blueprint HTTP server reachable ✅');
  } catch {
    console.warn('[WeComBP] ⚠️  Blueprint HTTP server not reachable at ' + BP_URL);
    console.warn('[WeComBP]    Start blueprint first: open WeComBot.bjson and run it');
  }

  const wsClient = new AiBot.WSClient({
    botId: BOT_ID, secret: SECRET,
    maxReconnectAttempts: -1,
    logger: {
      debug: (...a) => dbg('[SDK]', ...a),
      info:  (...a) => console.log('[SDK]', ...a),
      warn:  (...a) => console.warn('[SDK]', ...a),
      error: (...a) => console.error('[SDK]', ...a),
    }
  });

  wsClient.on('connected',     () => console.log('[WeComBP] WS connected'));
  wsClient.on('authenticated', () => console.log('[WeComBP] Authenticated ✅'));
  wsClient.on('disconnected',  r  => console.log('[WeComBP] Disconnected:', r));
  wsClient.on('reconnecting',  n  => console.log(`[WeComBP] Reconnecting #${n}...`));
  wsClient.on('error',         e  => console.error('[WeComBP] Error:', e));

  wsClient.on('event.enter_chat', async (frame) => {
    try {
      await wsClient.replyWelcome(frame, {
        msgtype: 'text',
        text: { content: '👋 您好！我是蓝图智能助手，直接发送问题即可。' }
      });
    } catch {}
  });

  for (const evtType of ['message.text', 'message.mixed', 'message.voice']) {
    wsClient.on(evtType, (frame) => {
      handleMessage(wsClient, frame, evtType).catch(e =>
        console.error('[WeComBP] handleMessage error:', e.message));
    });
  }

  wsClient.connect();

  process.on('SIGINT',  () => { try { wsClient.disconnect(); } catch {} setTimeout(() => process.exit(0), 500); });
  process.on('SIGTERM', () => { try { wsClient.disconnect(); } catch {} setTimeout(() => process.exit(0), 500); });
}

main().catch(e => { console.error('[WeComBP] Fatal:', e); process.exit(1); });
