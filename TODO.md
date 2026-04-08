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

- [x] **6. MCP.Call 节点接真实 MCP Server**
  - 实现标准 JSON-RPC 2.0 协议（默认）和 simple 模式切换 ✓
  - 正确解析 result.content[0].text，支持多条 content 拼接 ✓
  - 新增 Protocol / RawResult 引脚 ✓
  - examples/MCPCallDemo.bjson 示例蓝图 ✓
  - tools/mcp_test_server.py 本地测试服务器（内置 read_file/write_file/list_dir/web_fetch/echo）✓

## 🟢 低优先级：生态完整性

- [x] **7. Agent 模板库 README**
  - examples/README.md：API Key 配置、运行方法、变量注入说明 ✓

- [x] **8. Code.Run 超时强制终止**
  - Windows: CreateProcess + TerminateProcess ✓
  - Linux/macOS: fork/kill(SIGKILL) ✓

