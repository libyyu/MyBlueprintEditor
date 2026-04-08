# Blueprint Agent Examples

## 快速开始

### 1. 构建

```bat
build.bat          # Windows Release（生成 build-windows/bin/Release/）
```

### 2. 配置 API Key

所有 Agent 示例通过蓝图变量传入 API Key，**不要把 Key 写死在 .bjson 文件里**。

**方法一：命令行（推荐调试）**

```bat
build-windows\bin\Release\runtime-example.exe examples\ReActAgent.bjson
```

示例通过 `--variable` 注入（`runtime-example` 不支持此参数，需要改代码）。  
目前最简单的方式是临时修改蓝图变量默认值，或通过 MCP 注入。

**方法二：MCP 执行（推荐）**

```bash
# 启动 MCP Server
python tools/mcp_server.py

# 在 Claude Desktop 中使用：
execute_blueprint {
  "path": "examples/ReActAgent.bjson",
  "variables": {
    "ApiKey": "sk-xxx",
    "BaseURL": "https://api.openai.com/v1",
    "Model": "gpt-4o"
  }
}
```

---

## 示例清单

### AgentDemo.bjson
**最简单的 LLM 调用示例**。单轮对话，向 LLM 提问并打印回答。  
变量：`ApiKey`、`BaseURL`、`Model`

### ReActAgent.bjson ⭐
**真实工具的 ReAct Agent**。  
- `get_weather`：调用 [wttr.in](https://wttr.in)（无需 API Key）获取天气
- `web_search`：调用 DuckDuckGo Lite HTML（无需 API Key）搜索网页  
- 使用 `FireEvent/CustomEventNode` 实现异步循环

变量：`ApiKey`、`BaseURL`、`Model`、`UserQuery`

**运行示例：**
```
UserQuery = "What's the weather in Beijing and Shanghai, and any recent AI news?"
```

### MultiTurnAgent.bjson
**多轮工具调用 Agent**（WhileLoop 版本）。  
与 ReActAgent 功能类似，使用 WhileLoop 实现循环（最多 5 轮）。

变量：`ApiKey`、`BaseURL`、`Model`

### ToolUseAgent.bjson
**两轮工具调用示例**。第一轮 LLM 决定调用 `get_weather`，第二轮基于工具结果回答。  
变量：`ApiKey`、`BaseURL`、`Model`

### WebSearchAgent.bjson ⭐
**搜索 + 总结 Agent**。`Web.Search` 搜索 → `LLM.Chat` 总结结果。  
不需要工具调用循环，适合单次搜索任务。  
变量：`ApiKey`、`BaseURL`、`Model`、`SearchQuery`

**运行示例：**
```
SearchQuery = "latest AI news 2025"
```

### FileAgent.bjson ⭐
**文件分析/变换 Agent**。`File.ReadText` 读文件 → `LLM.Chat` 分析 → `File.WriteText` 写结果。  
适合：代码审查、文档改写、文件格式转换。  
变量：`ApiKey`、`BaseURL`、`Model`、`InputFile`、`OutputFile`、`Prompt`

**运行示例：**
```
InputFile  = "input.txt"
OutputFile = "output.txt"
Prompt     = "Review this code and suggest improvements:"
```

### LuaReActAgent.bjson ⭐
**Lua 实现的 ReAct Agent**。工具逻辑全部用 Lua 脚本实现，蓝图只是触发入口。  
加载蓝图时自动执行 `examples/BlueprintEntry.lua`，其中注册 `OnBeginPlay` 异步回调。  
变量：`ApiKey`、`BaseURL`、`Model`、`UserQuery`

Lua 工具实现：`examples/lua/agent_tools.lua`

---

## MCP Server 接入 Claude Desktop

复制以下配置到 Claude Desktop 配置文件：

- **Windows**：`%APPDATA%\Claude\claude_desktop_config.json`
- **macOS**：`~/Library/Application Support/Claude/claude_desktop_config.json`

```json
{
  "mcpServers": {
    "blueprint": {
      "command": "python",
      "args": ["C:/Users/maxweili/MyProjects/MyBlueprintEditor/tools/mcp_server.py"],
      "env": {
        "BLUEPRINT_DLL": "C:/Users/maxweili/MyProjects/MyBlueprintEditor/build-windows/bin/Release/BlueprintRuntime.dll",
        "BLUEPRINT_ROOT": "C:/Users/maxweili/MyProjects/MyBlueprintEditor"
      }
    }
  }
}
```

接入后可在 Claude Desktop 对话中使用：

```
list_blueprints                          # 列出所有蓝图
get_blueprint_schema                     # 查看蓝图格式说明
execute_blueprint {"path": "examples/WebSearchAgent.bjson",
  "variables": {"ApiKey": "sk-xxx", "SearchQuery": "AI news"}}
create_blueprint {"path": "examples/MyAgent.bjson", "content": "..."}
```

---

## 节点快速参考

| 节点 | 说明 | 关键引脚 |
|------|------|----------|
| `LLM.Chat` | 调用 OpenAI 兼容 API | BaseURL/ApiKey/Model/Messages/Tools → onReply/onToolCall/onError/Reply/ToolCallsJSON |
| `Web.Search` | DuckDuckGo Lite 搜索（无需 Key） | Query/MaxResults → onSuccess/Results/ResultText |
| `HTTP.Get` | 发 GET 请求 | URL → onSuccess/ResponseBody |
| `HTTP.Post` | 发 POST 请求 | URL/Body → onSuccess/ResponseBody |
| `File.ReadText` | 读文件 | Path → onSuccess/Content |
| `File.WriteText` | 写文件 | Path/Content → onSuccess |
| `Code.Run` | 执行命令（native only） | Command → onSuccess/Stdout/ExitCode |
| `Tool.ForEach` | 遍历 tool_calls | ToolCallsJSON → onTool/ToolName/Arguments/ToolCallId |
| `Tool.Match` | 按工具名路由 | ToolName/Case0..7 → Match0..7/Default |
| `FireEvent` | 异步触发事件（下一帧执行） | EventName |
| `CustomEventNode` | 事件接收器 | EventName（nodeData） |
