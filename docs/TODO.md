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

# 性能优化 TODO（2026-04-05 分析）

> 分析背景：当前编辑器日常使用（节点 <50、循环 <100 次）无性能问题。
> 以下优化针对**批量处理 / 大循环 / 高频 FuncLib 调用**等压测场景。

---

## 优先级 1 🔴：`executedHere` 全量扫描 → O(N²) 风险

**位置**：`Runtime/BlueprintRunner.cpp` — `executeDownstreamFromPin()` 内部循环

**问题**：
ForLoop 每次迭代调用 `executeDownstreamFromPin`，执行完每个节点后检测 `m_flowExecutedNodes`
新增条目，并全量遍历合并到局部 `executedHere`：
```cpp
for (auto flowId : m_flowExecutedNodes)   // ← 全量遍历，随迭代线性增长
    executedHere.insert(flowId);
```
当 `Last Index = 10000`，且循环体有多个节点时，整体接近 O(N²)。

**为什么这样设计**：
`executedHere` 是为了解决真实 bug ——  ForLoop 循环体中 Branch 等控制流节点通过
`ActivateOutputFlow` 递归执行了下游节点，加入全局 `m_flowExecutedNodes`；下次迭代
不能用全局集合跳过（否则第二次迭代所有节点被误跳过），必须用局部 `executedHere`
记录"本次 executeDownstreamFromPin 调用内已执行过的节点"。设计正确，只是合并方式低效。

**优化方案**：引入 `flowInsertLog`（局部 vector）追踪本次调用期间 `m_flowExecutedNodes`
的新增顺序，用范围遍历替代全量合并：
```cpp
// executeDownstreamFromPin 开头
std::vector<NodeId> flowInsertLog;

// 替换原来的 m_flowExecutedNodes.insert(id)
if (m_flowExecutedNodes.insert(id).second)
    flowInsertLog.push_back(id);

// 替换 snapshotSize 对比逻辑
size_t prevLogSize = flowInsertLog.size();  // 执行节点前记录
executeNodeInternal(*node);
for (size_t k = prevLogSize; k < flowInsertLog.size(); ++k)  // 只遍历新增部分
    if (flowInsertLog[k] != id)
        executedHere.insert(flowInsertLog[k]);
```
复杂度从 O(N×总量) 降到 O(N×每次新增)，接近 O(N)。局部变量，无状态副作用，安全。

---

## 优先级 2 🟡：`BuildFuncSubGraph` BFS 每步线性扫描全部 links

**位置**：`Runtime/BlueprintRunner.cpp` — `BuildFuncSubGraph()` 静态函数

**问题**：
BFS 中每个节点的每个引脚都遍历 `libData.links` 全表（O(L)），整体 O(N×P×L)。
每次 `FuncLib.xxx` 节点执行都重新构建子图，无缓存，ForLoop 内调用则重复 N 次构建。

**为什么这样设计**：
`libData` 在运行时理论上可变（编辑器支持热更新），未做缓存避免失效问题；
子图作为值类型独立 Load 进 subRunner，每次新副本天然隔离。设计合理，只是可以加速。

**优化方案分两层**：

### 层 1（低风险）：BuildFuncSubGraph 内部建局部索引（函数内，单次有效）

```cpp
// BuildFuncSubGraph 开头一次性建立
std::unordered_map<PinId, std::vector<size_t>> startPinToLinks;
std::unordered_map<PinId, std::vector<size_t>> endPinToLinks;
for (size_t i = 0; i < libData.links.size(); ++i) {
    if (!libData.links[i].isEnabled) continue;
    startPinToLinks[libData.links[i].startPinId].push_back(i);
    endPinToLinks[libData.links[i].endPinId].push_back(i);
}
// BFS 中用索引替代 for(lk : libData.links) 全扫
```
复杂度 O(N×P×L) → O(N×P)，局部变量无副作用，完全安全。

### 层 2（中等风险）：Runner 级别缓存 `funcId → BlueprintData`

```cpp
// BlueprintRunner 新增成员
std::unordered_map<std::string, BlueprintData> m_funcSubGraphCache;

// invalidateTopoCache() 时同步清空
m_funcSubGraphCache.clear();
```
在 FuncLib 分支先查缓存，miss 时构建并存入。ForLoop 内重复调用同一函数只构建一次。
**注意**：`m_reachableDirty` / `m_topoCacheDirty` 触发时必须同步清空，否则热更新蓝图后
使用旧子图。和现有 dirty 机制对齐，可安全实施。

---

## 优先级 3 🟢：Timer 注册时深拷贝 `savedPinNameToId`

**位置**：`Runtime/BlueprintRunner.cpp` — `wrapCallbackWithContextRestore()` 和 `RunAsync()`

**问题**：
每次注册 Timer / RunAsync 都深拷贝整个 `pinNameToId` unordered_map（含堆内存分配）：
```cpp
auto savedPinNameToId = m_state->pinNameToId;  // ← unordered_map 深拷贝
```
低频 Timer 无感；大量并发异步节点时有累积开销。

**为什么这样设计**：
Timer 回调异步触发时 `m_state->pinNameToId` 已被覆盖，必须快照恢复。设计正确。

**优化方案**：只保存 `savedNode` 指针，回调触发时从节点 pins 重建（O(引脚数)，无堆分配）：
```cpp
// 回调触发时替代 map 拷贝恢复
state->currentNode = savedNode;
if (savedNode) {
    state->pinNameToId.clear();
    for (const auto& pin : savedNode->pins)
        state->pinNameToId[pin.name] = pin.id;
}
```
**前提**：`savedNode` 指向 `m_blueprint.nodes` 中的元素，runner 运行期间 nodes 不重新分配，
指针稳定。编辑器"运行中不修改蓝图"保证此条件，安全。

---

## 暂缓 ⏸：`prepareNodeContext` 中 3 个 map → vector

**位置**：`Runtime/BlueprintRunner.cpp` — `prepareNodeContext()`

**问题**：每次执行节点都 clear + rebuild 3 个 `unordered_map`（`pinNameToId` /
`inputPinNameToId` / `outputPinNameToId`），高频 ForLoop 有轻微累积。

**为什么不动**：
- 3 个 map 的拆分是为了解决**同名输入/输出引脚冲突 bug**，不能合并回 1 个。
- 用 `vector<pair<string,PinId>>` + 线性查找替代，改动面大（所有 `find()` 调用点）。
- 引脚数通常 2~8，实际收益极小，风险不低。**等其他优化稳定后再评估。**

---

## 暂缓 ⏸：StepNext 每步 BFS 数据依赖

**位置**：`Runtime/BlueprintRunner.cpp` — `StepNextNode()` 的 `executeOneNode` lambda

**问题**：每次 StepNext 都重新 BFS 收集数据上游依赖并遍历全 topo order。

**为什么不动**：
- 调试专用（手动点击），每步 <1ms，用户感知不到。
- 代码清晰优于微优化。
- 未来若有需要，可在 `m_stepPendingNodes` 填充时同步缓存数据依赖集合。

---

## 实施顺序建议

| 顺序 | 项目 | 改动范围 | 风险 | 预期收益 |
|------|------|----------|------|----------|
| 1 | executedHere 范围遍历 | 1个函数，局部变量 | 低 | 高（ForLoop大循环） |
| 2 | BuildFuncSubGraph 局部索引（层1） | 1个函数，局部变量 | 低 | 中 |
| 3 | FuncLib 子图结果缓存（层2） | 新增 map + 缓存失效 | 中 | 高（循环内FuncLib） |
| 4 | Timer savedNode 替代 map 拷贝 | 2处，逻辑简单 | 低 | 低 |
| ⏸ | prepareNodeContext map→vector | 全量替换 | 中 | 极低 |
| ⏸ | StepNext BFS 缓存 | — | — | 无需 |

---

# AI Agent / MCP TODO（2026-04-05 分析）

## 现状评估

AI Agent 功能已相当完整，核心能力已可用：

| 功能 | 状态 | 文件 |
|------|------|------|
| LLM.Chat（非流式，支持 tool_calls 三路出口） | ✅ 完成 | `BuiltinHandlers_AI.cpp` |
| LLM.StreamChat（SSE 流式，逐 token 激活） | ✅ 完成 | `BuiltinHandlers_AI.cpp` |
| IHttpClient 抽象接口 + cpp-httplib 实现 | ✅ 完成 | `Http/` |
| WebGL Emscripten 降级实现 | ✅ 完成 | `Http/HttpClient_Emscripten.cpp` |
| JSON.Build/SetPath/ArrayPush/MakeMessage/Extract/Validate | ✅ 完成 | `BuiltinHandlers_AI.cpp` |
| JSON.ParseToolCall / MakeToolResult | ✅ 完成 | `BuiltinHandlers_AI.cpp` |
| Tool.ForEach / Tool.Match | ✅ 完成 | `BuiltinHandlers_AI.cpp` |
| Memory.LoadHistory / SaveHistory | ✅ 完成 | `BuiltinHandlers_AI.cpp` |
| String.Template（`{{变量}}`占位符替换） | ✅ 完成 | `BuiltinHandlers_AI.cpp` |
| Lua 侧：json.* / http.*（同步） | ✅ 完成 | `LuaLib_JsonHttp.cpp` |
| 示例：AgentDemo / ToolUseAgent / MultiTurnAgent | ✅ 完成 | `examples/` |
| 单元测试（MockHttpClient + 全路径覆盖） | ✅ 完成 | `tests/test_agent_demo.cpp` |

---

## 待修复缺口

### 缺口 1 🔴：`JSON.ToolCallCount` handler 未实现

**问题**：`BuiltinNodeDefs.cpp` 中节点定义已注册，但 `BuiltinHandlers_AI.cpp` 中没有对应
handler。调用时走"no handler"警告路径，并发工具调用场景失效。

**修复**：在 `RegisterHandlers_AI` 中添加：
```cpp
handlers["JSON.ToolCallCount"] = [](ExecutionContext& ctx) {
    auto json = ctx.GetInputValue("ToolCallsJSON").asString();
    auto v = crude_json::value::parse(json);
    int64_t count = v.is_array()
        ? static_cast<int64_t>(v.get<crude_json::array>().size())
        : 0;
    ctx.SetOutputValue("Count", Variant(count));
    return true;
};
```
改动量：5 行，零风险。

### 缺口 2 🟡：Lua `http.*` 同步阻塞主线程

**问题**：`LuaLib_JsonHttp.cpp` 的 `syncRequest` 用 `mutex + condition_variable`
阻塞等待 HTTP 响应，在主线程调用会卡死蓝图帧循环（Timer/Tick 停止推进）。

**影响**：仅影响在主线程执行 Lua 脚本且调用 `http.*` 的场景。
纯离线脚本（非交互）或 Lua 后台线程场景不受影响。

**未来方案**：提供 `http.request_async(url, cb)` 协程或回调版本，
内部复用 `IHttpClient::SendAsync` + `MainThreadDispatcher`。

### 缺口 3 🟢：流式 tool_calls 不支持

**问题**：`LLM.StreamChat` 只处理 `delta.content`，不处理流式 `delta.tool_calls`。
当 LLM 以流式方式输出 Function Calling（如 Claude streaming function calling）时，
`ToolCallsJSON` 输出为空。

**影响**：只影响**同时需要流式输出 + 工具调用**的场景。
非流式 `LLM.Chat` 的 tool_calls 完全正常。

**未来方案**：`StreamAsync` 的 `onDone` 回调中额外解析 SSE 累积的
`tool_calls` delta，拼合后输出到 `ToolCallsJSON`。

---

## MCP 接入计划

### 背景

Model Context Protocol（MCP）是 Anthropic 开放的 AI 工具标准化协议，
允许 AI 助手（Claude、Cursor 等）通过标准接口访问外部工具和数据。

### 两个方向

#### 方向 A：把蓝图编辑器暴露为 MCP Server（AI 辅助编辑蓝图）

```
AI 助手（Claude Desktop / Cursor）
    ↕ MCP 协议
MCP Server（独立 Python 进程）
    ↕ HTTP REST / 读写 .bjson 文件
BlueprintRuntime / 编辑器
```

**AI 能做到的事**：
- 用自然语言描述逻辑，AI 自动组装蓝图节点连线
- 读取蓝图数据、添加/删除节点、修改引脚值
- 执行蓝图并返回运行结果

**实现路径**：
1. 编写 Python MCP Server（`mcp_server.py`，约 150 行）
2. 定义 Tools：`list_node_defs`、`create_blueprint`、`add_node`、`add_link`、`execute_blueprint` 等
3. **最小实现（零改编辑器）**：MCP Server 直接读写 `.bjson` 文件 +
   调用 `runtime-example.exe` 执行，无需编辑器改动
4. **进阶实现（实时联动）**：编辑器内嵌轻量 HTTP 接口（`httplib.h` 单头文件，
   约 200 行改动），AI 操作后编辑器实时刷新

**依赖**：Python + `mcp` 官方 SDK（`pip install mcp`）

---

#### 方向 B：蓝图内支持调用 MCP 工具（蓝图编排 AI 工作流）✅ 现有基础 90%

```
蓝图运行时 → MCP Client（新增节点）→ 外部 MCP Server
                                       （文件系统/数据库/搜索/API 等）
```

**现有能力已覆盖 90%**：
- `LLM.Chat` = 调用 LLM 工具
- `Tool.ForEach / Tool.Match` = MCP Tool 路由
- `IHttpClient` = 可直接复用做 MCP HTTP 通信

**额外需要新增的节点**：

| 节点 | 功能 | 复杂度 |
|------|------|--------|
| `MCP.ListTools` | GET MCP server 的工具列表 | 低（复用 `IHttpClient`） |
| `MCP.CallTool` | 调用 MCP 工具，返回结果 | 低（JSON-RPC POST） |
| `MCP.ReadResource` | 读取 MCP 资源（文件/数据库） | 低 |

**MCP JSON-RPC 格式**（唯一新增的理解成本）：
```json
{
  "jsonrpc": "2.0",
  "method": "tools/call",
  "params": { "name": "read_file", "arguments": { "path": "/tmp/a.txt" } },
  "id": 1
}
```
可以用现有的 `JSON.Build` + `HTTP.Request` 节点手动拼，或封装成专用节点。

---

### 推荐实施顺序

| 顺序 | 任务 | 优先级 | 估时 |
|------|------|--------|------|
| 1 | 修复 `JSON.ToolCallCount` handler（5行） | 🔴 先做 | 10分钟 |
| 2 | 方向 B：新增 `MCP.CallTool` / `MCP.ListTools` 节点 | 🟡 | 1天 |
| 3 | 方向 B：编写 MultiTurnAgent + MCP 工具调用示例 bjson | 🟡 | 半天 |
| 4 | 方向 A（最小版）：Python MCP Server 读写 .bjson + 执行 | 🟢 | 1天 |
| 5 | 方向 A（进阶版）：编辑器内嵌 REST 接口，实时联动 | 🟢 | 2天 |
| 6 | 修复 Lua http.* 异步阻塞问题 | 🟢 | 半天 |
| 7 | 流式 tool_calls 支持 | ⏸ 按需 | 1天 |
