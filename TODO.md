# MyBlueprintEditor TODO

## 🔴 高优先级：让现有能力真正跑起来

- [x] **1. 打通 MCP Server → Claude Desktop**
  - 更新 mcp_server.py 默认 DLL 路径到 build-windows ✓
  - 补充 docs/claude_desktop_config.json 示例（含变量注入说明）✓

- [x] **2. ReActAgent 接真实工具**
  - get_weather 工具改接 wttr.in（HTTP.Get 节点，无需 API Key）✓
  - web_search 工具接 Web.Search 节点（DuckDuckGo Lite）✓
  - 更新 examples/ReActAgent.bjson ✓

- [x] **3. Lua Agent 示例**
  - 新建 examples/LuaReActAgent.bjson ✓
  - 工具实现：examples/lua/agent_tools.lua ✓
  - 自动加载入口：examples/BlueprintEntry.lua ✓

## 🟡 中优先级：扩展有用工具节点

- [x] **4. Web.Search 真实测试 + 蓝图示例**
  - 新建 examples/WebSearchAgent.bjson ✓

- [x] **5. File Agent 示例**
  - 新建 examples/FileAgent.bjson ✓

- [ ] **6. MCP.Call 节点接真实 MCP Server**
  - 实现 MCP.Call handler（当前为 stub）
  - 接入 filesystem-mcp / fetch-mcp

## 🟢 低优先级：生态完整性

- [x] **7. Agent 模板库 README**
  - examples/README.md：API Key 配置、运行方法、变量注入说明 ✓

- [x] **8. Code.Run 超时强制终止**
  - Windows: CreateProcess + TerminateProcess ✓
  - Linux/macOS: fork/kill(SIGKILL) ✓

