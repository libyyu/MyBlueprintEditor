In random order:
* Documentation: Make one

Done:
* ~~ImGui: Factor out changes to ImGui to use vanilla version.~~
* ~~Editor: Fix variable naming (mainly add `m_` prefix)~~
* ~~Editor: Split NodeEditorImpl.cpp to multiple files, file has grown too big.~~
* ~~Editor: Factor out use of `picojson.h`~~
* ~~Editor: Move use of `<optional>` to optional code extensions~~



#57 - join `ax::NodeEditor::EditorContext` with `struct EditorContext` and remove `reinterpret_cast<>`

---

# 性能优化 TODO（2026-04-05 分析 / 2026-04-07 已完成 P0）

> 分析背景：当前编辑器日常使用（节点 <50、循环 <100 次）无性能问题。
> 以下优化针对**批量处理 / 大循环 / 高频 FuncLib 调用**等压测场景。

---

## ✅ 已完成（2026-04-07 P0 优化）

### 1. `executedHere` 全量扫描 → O(N) 范围遍历

**问题**：`executeDownstreamFromPin` 每次执行节点后全量遍历 `m_flowExecutedNodes` 合并到 `executedHere`，ForLoop 大循环接近 O(N²)。

**修复**：
- `flowInsertLog` 提升为成员变量 `m_flowInsertLog`（原为局部变量，嵌套递归调用时内层插入对外层不可见，导致重复执行节点）
- 用范围遍历 `[prevLogSize, newSize)` 替代全量扫描
- 同步在 `Execute()` / `DispatchEvent()` 开头清空

### 2. `BuildFuncSubGraph` BFS 局部 links 索引

**修复**：函数开头一次性建立 `startPinLinks / endPinLinks` 索引，BFS 内 O(1) 查找替代 O(L) 全扫。

### 3. FuncLib 子图结果缓存 `m_funcSubGraphCache`

**修复**：`BlueprintRunner` 新增成员，FuncLib 外部库路径先查缓存，miss 才构建。`invalidateTopoCache()` 时同步清空。

---

## 待处理

### 优先级 3 🟢：Timer 注册时深拷贝 `savedPinNameToId`

**位置**：`Runtime/BlueprintRunner.cpp` — `wrapCallbackWithContextRestore()` 和 `RunAsync()`

**问题**：每次注册 Timer / RunAsync 都深拷贝整个 `pinNameToId` unordered_map。

**优化方案**：只保存 `savedNode` 指针，回调触发时从节点 pins 重建：
```cpp
state->currentNode = savedNode;
if (savedNode) {
    state->pinNameToId.clear();
    for (const auto& pin : savedNode->pins)
        state->pinNameToId[pin.name] = pin.id;
}
```

---

## 暂缓 ⏸

- **`prepareNodeContext` map→vector**：改动面大，收益极低，等其他优化稳定后再评估。
- **StepNext BFS 缓存**：调试专用，每步 <1ms，无需优化。

---

# AI Agent / MCP（2026-04-07 更新）

---

## 当前完成状态

### AI Agent 节点（Runtime）

| 功能 | 状态 | 备注 |
|------|------|------|
| `LLM.Chat`（非流式，tool_calls 三路出口） | ✅ | |
| `LLM.StreamChat`（SSE 流式，逐 token） | ✅ | 不支持流式 tool_calls，见缺口3 |
| `IHttpClient` 抽象 + cpp-httplib 实现 | ✅ | |
| WebGL Emscripten 降级实现 | ✅ | |
| `JSON.Build/SetPath/ArrayPush/MakeMessage/Extract/Validate` | ✅ | |
| `JSON.ParseToolCall / MakeToolResult / ToolCallCount` | ✅ | ToolCallCount handler 已补齐 |
| `Tool.ForEach / Tool.Match` | ✅ | |
| `Tool.CallByName`（动态工具路由，无 8 个上限） | ✅ 新增 | 按名称动态查找 FuncLib 函数 |
| `Memory.LoadHistory / SaveHistory` | ✅ | |
| `String.Template`（`{{变量}}` 占位符） | ✅ | |
| Lua 侧：`json.* / http.*`（同步） | ✅ | http.* 同步阻塞，见缺口2 |
| 示例：AgentDemo / ToolUseAgent / MultiTurnAgent / ReActAgent | ✅ | |
| 单元测试（MockHttpClient，24 个用例） | ✅ | |

### C API（BlueprintCAPI）

| 接口 | 状态 | 备注 |
|------|------|------|
| `BP_CreateRunner / BP_DestroyRunner` | ✅ | |
| `BP_LoadFromJson` | ✅ | 不加载依赖，纯 JSON 加载 |
| `BP_LoadFromJsonWithBaseDir` | ✅ 新增 | 自动加载 dependencies + 静默加载 BlueprintEntry.lua |
| `BP_LoadFromFile` | ✅ 升级 | 改用 LoadFromFileWithDeps + 自动设 basePath + 静默加载 Lua |
| `BP_Execute / BP_DispatchEvent` | ✅ | |
| `BP_Tick` | ✅ 升级 | 现在同时驱动 `OnGlobalTick(dt)` |
| `BP_GetActiveTimerCount` | ✅ 新增 | 判断是否需要继续 Tick |
| `BP_SetBasePath` | ✅ 新增 | 手动设置 ExecuteBlueprint 路径解析基准 |
| `BP_LoadLuaScript` | ✅ 新增 | 手动加载 Lua 脚本 |
| `BP_DispatchEvent` | ✅ 新增 | 触发命名事件（OnBeginPlay 等） |
| `BP_SetVariableXxx / BP_GetVariableXxx` | ✅ | |
| `BP_SetLogCallback / BP_SetPrintCallback` | ✅ | |
| `BP_RegisterNodeDef / BP_RegisterHandler` | ✅ | |

### MCP Server（tools/mcp_server.py）

| 功能 | 状态 | 备注 |
|------|------|------|
| Python MCP Server（mcp 1.27.0） | ✅ | stdio 模式 |
| `list_blueprints` | ✅ | 扫描工程 .bjson 文件 |
| `get_blueprint_content` | ✅ | 读取 JSON 内容 |
| `execute_blueprint`（文件路径版） | ✅ | BP_LoadFromFile + Execute + DispatchEvent + Tick 循环 |
| `execute_blueprint_json`（内联 JSON） | ✅ | BP_LoadFromJsonWithBaseDir |
| `get_variable` | ✅ | 执行后读取变量值 |
| `create_blueprint` | ✅ | 保存 .bjson 文件 |
| `get_blueprint_schema` | ✅ | 格式说明 + 常用节点速查 |
| Tick 循环（异步 Delay/ExecuteBlueprint） | ✅ | 最长等待 30s，16ms 间隔 |
| 依赖库自动加载（FuncLib） | ✅ | 通过 BP_LoadFromFile |
| BlueprintEntry.lua 自动加载 | ✅ | 蓝图目录 → DLL 目录搜索 |
| Claude Desktop 配置文件 | ✅ | `tools/claude_desktop_config.json` |

---

## 待修复缺口

### 缺口 1 ✅：`JSON.ToolCallCount` handler — 已完成

### 缺口 2 🟡：Lua `http.*` 同步阻塞主线程

**问题**：`LuaLib_JsonHttp.cpp` 的 `syncRequest` 用 `mutex + condition_variable` 阻塞，在主线程调用会卡死帧循环。

**影响**：仅影响 Lua 脚本在主线程调 `http.*` 的场景，纯离线脚本不受影响。

**未来方案**：提供 `http.request_async(url, cb)` 回调版本，复用 `IHttpClient::SendAsync + MainThreadDispatcher`。

### 缺口 3 🟢：流式 `tool_calls` 不支持

**问题**：`LLM.StreamChat` 只处理 `delta.content`，不处理流式 `delta.tool_calls`。

**影响**：只影响"同时需要流式输出 + 工具调用"的场景，非流式 `LLM.Chat` 的 tool_calls 完全正常。

**未来方案**：`StreamAsync` 的 `onDone` 回调中额外解析 SSE 累积的 `tool_calls` delta，拼合后输出到 `ToolCallsJSON`。

---

## 后续计划

### P2：AI Agent 进阶

| 项目 | 估时 | 说明 |
|------|------|------|
| 修复 Lua `http.*` 异步阻塞（缺口2） | 半天 | 提供回调版异步接口 |
| 流式 tool_calls 支持（缺口3） | 半天 | StreamChat 增量拼合 |
| 并行工具调用 | 1天 | Tool.ForEach 并发执行多个 tool_call |

### P3：编辑器体验

| 项目 | 说明 |
|------|------|
| 节点搜索记忆上次分类 | 每次打开恢复上次选中分类 |
| 变量面板快速类型切换 | 当前改类型需删重建 |
| Log 面板时间戳过滤 | 补充时间维度过滤 |

### P4：MCP Server 进阶（按需）

| 项目 | 说明 |
|------|------|
| 编辑器内嵌 REST 接口（方向 A 进阶） | AI 操控编辑器实时刷新，工作量约 3 天 |
| MCP.CallTool / MCP.ListTools 节点（方向 B） | 蓝图内直接调用外部 MCP Server |
| Agent.Memory 向量存储（RAG） | 接入 Chroma/Milvus，需外部服务 |

---

## MCP Server 使用方法

### 快速测试

```bash
cd MyBlueprintEditor
pip install mcp
py tools/test_mcp.py        # 基础测试
py tools/test_main.py       # Main.bjson 含异步 Delay 完整测试
```

### 接入 Claude Desktop

将 `tools/claude_desktop_config.json` 中的 `mcpServers` 块合并到：
- Windows: `%APPDATA%\Claude\claude_desktop_config.json`

重启 Claude Desktop 后即可自然语言操作蓝图：

```
"列出所有蓝图文件"
"执行 examples/AgentDemo.bjson，ApiKey 设置为 sk-xxx"
"帮我创建一个蓝图：ForLoop 0~9，每次打印索引的平方"
```

### BlueprintEntry.lua 支持

将 `BlueprintEntry.lua` 放在蓝图文件旁边或 DLL 旁边：
- `BP_LoadFromFile` 会自动静默加载（不存在则跳过）
- `BP_Tick` 会自动驱动 `OnGlobalTick(dt)`

```lua
-- BlueprintEntry.lua 示例
function OnGlobalTick(dt)
    -- 每帧心跳逻辑
end

-- 注册自定义节点（可选）
Blueprint.RegisterNodeDef(...)
Blueprint.RegisterHandler(...)
```
