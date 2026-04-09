# WeComBridge — 企业微信 × Blueprint 蓝图网关

将企微 AI Agent 的用户消息路由到 Blueprint MCP 执行引擎，实现：

```
企微用户输入
    ↕ WebSocket 长连接（@wecom/aibot-node-sdk）
WeComBridge（本服务）
    ↕ HTTP POST /tool
Blueprint MCP Server（mcp_server.py --http）
    ↕ C++ DLL
BlueprintRuntime（蓝图执行引擎）
    ↕ 节点调用
LLM.Chat / Web.Search / HTTP.Retry / Agent.Reflect ...
```

---

## 快速开始

### 1. 前置条件

| 条件 | 说明 |
|------|------|
| Node.js 18+ | `node --version` |
| Blueprint MCP HTTP 服务 | 在蓝图项目根目录运行（见下） |
| 企微 AI Agent 应用 | 需要 `long_connection` 模式的 BotId + Secret |

**启动 Blueprint MCP HTTP Server：**
```bash
# 在 MyBlueprintEditor/ 根目录
python tools/mcp_server.py --http --port 7799
```

### 2. 安装依赖

```bash
cd tools/wecom-bridge
npm install
```

### 3. 配置

```bash
cp config.example.json config.json
# 编辑 config.json，填写 wecom.long_connection.botId 和 secret
```

最小配置（长连接模式）：
```json
{
  "wecom": {
    "mode": "long_connection",
    "long_connection": {
      "botId":  "你的企微AI应用BotId",
      "secret": "你的企微AI应用Secret"
    }
  },
  "blueprint": {
    "mcpServerUrl": "http://localhost:7799",
    "defaultBlueprint": "data/examples/WebSearchAgent.bjson",
    "queryVariableName": "UserQuery",
    "fixedVariables": {
      "ApiKey":  "sk-xxx",
      "BaseURL": "https://api.openai.com/v1",
      "Model":   "gpt-4o"
    }
  },
  "debug": false
}
```

### 4. 启动

```bash
npm start
# 或指定配置文件
node wecom_bridge.js --config /path/to/my-config.json
```

---

## 完整三步启动

**终端 1：Blueprint MCP Server**
```bash
cd C:\path\to\MyBlueprintEditor
python tools/mcp_server.py --http --port 7799
```

**终端 2：WeComBridge**
```bash
cd tools/wecom-bridge
npm start
```

**然后在企微里找到你的 AI Agent，发送消息即可。**

---

## 内置命令

在企微对话中发送以 `-` 开头的命令：

| 命令 | 简写 | 说明 |
|------|------|------|
| `-help` | `-h` | 显示帮助 |
| `-status` | `-st` | 显示 Bridge 和 MCP Server 状态 |
| `-ping` | `-p` | 测试连通性 |
| `-blueprint <路径>` | `-bp` | 临时指定蓝图执行（覆盖路由） |
| `-list` | `-ls` | 列出所有可用蓝图 |

**示例：**
```
-bp data/examples/ReActAgent.bjson
今天北京天气怎么样？
```
等效于：用 ReActAgent 执行"今天北京天气怎么样？"

---

## 路由规则

`config.json` 中的 `blueprint.routes` 按**关键词匹配**选择蓝图，长关键词优先：

```json
"routes": {
  "搜索":    "data/examples/WebSearchAgent.bjson",
  "分析文件": "data/examples/FileAgent.bjson",
  "代码":    "data/examples/ReActAgent.bjson"
}
```

用户说"帮我搜索最新 AI 新闻" → 匹配"搜索" → 执行 `WebSearchAgent.bjson`

无匹配关键词 → 执行 `defaultBlueprint`

---

## 典型使用场景

### 场景 1：智能问答机器人
```json
{
  "blueprint": {
    "defaultBlueprint": "data/examples/ReActAgent.bjson",
    "fixedVariables": { "ApiKey": "sk-xxx", "Model": "gpt-4o" }
  }
}
```
用户任意提问 → ReAct Agent 思考 + 工具调用 → 回复

### 场景 2：多技能路由
```json
{
  "blueprint": {
    "defaultBlueprint": "data/examples/AgentDemo.bjson",
    "routes": {
      "搜索": "data/examples/WebSearchAgent.bjson",
      "天气": "data/examples/WebSearchAgent.bjson",
      "代码": "data/examples/ReActAgent.bjson",
      "总结": "data/examples/FileAgent.bjson"
    }
  }
}
```

### 场景 3：定时播报（结合 cron）
不通过用户消息触发，而是用蓝图内的 Scheduler 功能定时调用 `mcp_server.py`，结果用 HTTP.Post 推送企微 Webhook。

---

## 企微 AI Agent 应用申请

`long_connection` 模式需要企微管理员在后台创建「AI Agent 应用」：

1. 企业微信管理后台 → **应用管理** → **创建应用** → 选择 **AI Agent**
2. 进入应用详情 → **长连接配置** → 获取 `BotId` 和 `Secret`
3. 成员在企微搜索该 AI Agent 名称，发起对话

> 如果没有管理权限，也可用**群机器人 Webhook**（`mode: "webhook"`），但只能推送消息，无法接收用户输入。

---

## 与 CodeBuddy CLI 结合（可选）

如果希望企微用户的问题由 CodeBuddy AI 理解后再选择蓝图（更灵活），可以在 `config.json` 中把 `defaultBlueprint` 改为一个"调度蓝图"，该蓝图内用 `LLM.Chat` + `Tool.Match` 判断意图再调用具体蓝图。

不需要修改本服务代码，全部逻辑在蓝图里实现。
