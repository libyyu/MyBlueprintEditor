# tools/ — BlueprintEditor MCP Server

## 概述

`mcp_server.py` 通过 MCP 协议将 BlueprintRuntime 暴露给 AI 客户端（Claude Desktop、Cursor 等），
使 AI 能用自然语言操作蓝图：列出文件、读取内容、执行蓝图、创建新蓝图。

---

## 安装依赖

```bash
py -m pip install mcp
```

---

## 快速验证

```bash
cd MyBlueprintEditor
py tools/test_mcp.py
```

预期输出：找到 11 个 .bjson 文件，PrintString 输出 "Hello from MCP Server!"，变量注入正常。

---

## 接入 Claude Desktop

1. 找到 Claude Desktop 配置文件：
   - Windows: `%APPDATA%\Claude\claude_desktop_config.json`
   - macOS:   `~/Library/Application Support/Claude/claude_desktop_config.json`

2. 将 `tools/claude_desktop_config.json` 里的 `mcpServers` 块合并进去（修改路径为实际路径）

3. 重启 Claude Desktop

4. 在 Claude 对话里可以说：
   > "列出所有蓝图文件"
   > "执行 examples/AgentDemo.bjson，ApiKey 设置为 sk-xxx"
   > "帮我创建一个蓝图：ForLoop 0~4，每次打印索引的平方"

---

## 可用工具

| 工具 | 说明 |
|---|---|
| `list_blueprints` | 列出所有 .bjson 文件（路径、名称、大小） |
| `get_blueprint_content` | 读取蓝图 JSON 内容 |
| `execute_blueprint` | 执行蓝图文件，返回 PrintString 输出 |
| `execute_blueprint_json` | 执行内联 JSON 蓝图（不需要保存文件） |
| `get_variable` | 执行蓝图后读取指定变量的值 |
| `create_blueprint` | 将 JSON 字符串保存为 .bjson 文件 |
| `get_blueprint_schema` | 返回 .bjson 格式说明和常用节点类型参考 |

---

## 环境变量

| 变量 | 默认值 | 说明 |
|---|---|---|
| `BLUEPRINT_DLL` | `build/bin/Release/BlueprintRuntime.dll` | DLL 路径 |
| `BLUEPRINT_ROOT` | 项目根目录 | .bjson 搜索根目录 |

---

## 工作原理

```
Claude Desktop
    ↓ MCP (stdio JSON-RPC)
mcp_server.py
    ↓ ctypes
BlueprintRuntime.dll
    BP_LoadFromJson → BP_Execute → BP_DispatchEvent("OnBeginPlay")
    → PrintString output collected via BP_SetPrintCallback
```

执行流程：
1. `BP_CreateRunner()` — 创建 runner 实例
2. `BP_SetPrintCallback()` — 注册输出收集回调
3. `BP_LoadFromJson()` — 加载蓝图 JSON
4. `BP_Execute()` — 执行数据流节点（SetVariable 等）
5. `BP_DispatchEvent("OnBeginPlay")` — 触发事件链（ForLoop、LLM.Chat 等）
6. 收集所有 PrintString 输出并返回

---

## 典型使用示例

### 执行 Agent 蓝图（传入 API Key 和用户问题）

Claude 提示词：
```
执行 examples/MultiTurnAgent.bjson，设置变量：
  ApiKey = "sk-your-key"
  UserQuestion = "解释什么是蓝图编辑器"
```

MCP Server 会调用：
```python
execute_blueprint(
    path="examples/MultiTurnAgent.bjson",
    variables={"ApiKey": "sk-your-key", "UserQuestion": "解释什么是蓝图编辑器"}
)
```

### 创建新蓝图

Claude 提示词：
```
帮我创建一个蓝图：ForLoop 0~9，每次打印"第 {i} 次迭代"
保存到 examples/MyLoop.bjson
```

Claude 会调用 `get_blueprint_schema` 了解格式，然后调用 `create_blueprint` 生成文件，
最后调用 `execute_blueprint_json` 验证输出。
