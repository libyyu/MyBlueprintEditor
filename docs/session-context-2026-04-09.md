# MyBlueprintEditor 开发会话备份
**日期**: 2026-04-09 ~ 2026-04-10  
**分支**: dev

---

## 项目简介

基于 ImGui 的节点蓝图编辑器，C++17 + CMake，支持 Windows/Linux/macOS/WebGL/Android/iOS。

- **Runtime/**：BlueprintRuntime 库（静态/动态/WebGL）
- **BlueprintEditor/**：编辑器主程序（DX11 + ImGui）
- **tools/**：MCP Server（Python）、WeComBridge（Node.js）
- **data/examples/**：示例蓝图（.bjson）
- **data/p4review/**：P4 代码审查蓝图
- **tests/**：Google Test 单元测试（99个用例）

构建命令：`cmake --build build-windows --config Release`  
测试命令：`ctest --test-dir build-windows -C Release`

---

## 本次会话完成的工作

### 1. Bug 修复（8项）

| # | 文件 | 问题 | 修复 |
|---|------|------|------|
| 1 | `BuiltinHandlers_String.cpp` | `String.Regex` try/catch 在 WebGL（-fno-exceptions）编译失败 | `#if defined(__cpp_exceptions) && !defined(__EMSCRIPTEN__)` 宏保护 |
| 2 | `MultiTurnAgent.bjson` | `round<5` 节点孤立（`GetVariable round` Value 引脚 dataType=4 应为 2） | 改 `dataType: 2` |
| 3 | `ReflectAgent.bjson` | BaseURL/ApiKey 连线对调，一条连线 `isEnabled: false` | 调换连线修正 |
| 4 | `FileOperations.cpp` | 外部拖入同名 bjson 可打开两个 Tab | `DoOpenFile` 开头加路径规范化+去重检查 |
| 5 | `BuiltinNodeDefs.cpp` | `GetVariable`/`SetVariable` 在右键菜单找不到 | category 从 `"Misc"` 改为 `"Variables"` |
| 6 | `BlueprintRunner.cpp` | 断点单步 `Code.Run` 后 onError+onSuccess 都执行 | `m_stepPendingNodes` 遍历时过滤 `m_flowExecutedNodes` |
| 7 | `P4ReviewAgent.bjson` | Build Diff Command Arg1 是 Exec 类型 | 连线从 `14010→16003` 改为 `22010→16003` |
| 8 | `P4BatchReviewAgent.bjson` | For Each CL 孤立节点 | `definitionId` 改为 `ForEachLoop`，引脚名对齐 |

### 2. Less/Greater 节点引脚名修复

- `BuiltinNodeDefs.cpp`：`Less`/`Greater` 引脚名全是空字符串 `""`，改为 `A`/`B`/`Result`
- `ReflectAgent.bjson`、`MultiTurnAgent.bjson` 对应节点引脚名也同步修正

### 3. 对话框 DPI 适配

- `Dialogs.cpp`：`ShowUnsavedChangesDialog` 按钮宽度改为 `100.0f * dpiScale`，窗口最小宽度 `360 * dpiScale`
- `InspectorPanel.cpp`：Add Variable 弹窗和类型切换弹窗所有硬编码宽度改为 `× dpiScale`

### 4. MCP Server 增强

- `mcp_server.py`：新增 `--http` REST 模式（`python mcp_server.py --http --port 7799`）
- `execute_blueprint_json` 增加 `dispatch_beginplay` 参数
- `list_blueprints` 返回变量列表字段
- `get_blueprint_schema` 补全所有节点说明

### 5. 新增节点（C++ + 节点定义）

| 节点 | 说明 |
|------|------|
| `Agent.Plan` | 目标分解 → 步骤 JSON |
| `Agent.Reflect` | 评估输出质量 pass/fail |
| `Context.Compress` | LLM 摘要压缩历史 |
| `HTTP.Retry` | 自动重试 HTTP 请求 |
| `String.Regex` | 正则匹配/查找/替换 |

### 6. 新增示例蓝图

- `data/examples/ReflectAgent.bjson`
- `data/examples/LongContextAgent.bjson`
- `data/examples/PlanExecuteAgent.bjson`
- `data/p4review/P4ReviewAgent.bjson`（单 CL 审查）
- `data/p4review/P4BatchReviewAgent.bjson`（批量审查）
- `data/p4review/P4DetectClient.bjson`（检测 P4 客户端）

### 7. WeComBridge（企微-蓝图桥接服务）

新增 `tools/wecom-bridge/`：

```
wecom_bridge.js     # 主程序（Node.js，@wecom/aibot-node-sdk + axios）
package.json
config.example.json # 配置模板
README.md
```

**架构**：
```
企微用户 → WS 长连接 → wecom_bridge.js → POST /tool → Blueprint MCP → 蓝图执行
```

**启动**：
```bash
# 终端1：Blueprint MCP HTTP
python tools/mcp_server.py --http --port 7799

# 终端2：WeComBridge
cd tools/wecom-bridge && npm install && npm start
```

**路由配置**（`config.json`）：
```json
{
  "wecom": {
    "mode": "long_connection",
    "long_connection": { "botId": "...", "secret": "..." }
  },
  "blueprint": {
    "mcpServerUrl": "http://localhost:7799",
    "defaultBlueprint": "data/examples/WebSearchAgent.bjson",
    "queryVariableName": "UserQuery",
    "fixedVariables": { "ApiKey": "sk-xxx", "BaseURL": "...", "Model": "gpt-4o" },
    "routes": {
      "review": "data/p4review/P4ReviewAgent.bjson",
      "batch review": "data/p4review/P4BatchReviewAgent.bjson"
    }
  }
}
```

内置命令：`-help` `-status` `-ping` `-blueprint <path>` `-list`

### 8. 变量面板 UX 改进（UE4 风格）

**变量列表行重构**（`InspectorPanel.cpp`）：
- 整行作为拖拽热区（原来只有 14px 色标）
- 默认显示文字，**双击**才进入输入框编辑
- Esc 取消，Enter/失焦确认

**变量拖拽到画布**（`EditorUI.cpp`）：
- 松手直接弹出 Get/Set 选择菜单
- 生成节点：隐藏 Name 输入引脚，节点标题显示 `"Get VarName"` / `"Set VarName"`
- Value 引脚类型根据变量实际类型自动修正（不再全是 String）

**关键技术细节**：
- Drop target 必须在 `ed::End()` 之后创建（NodeEditor 在 Begin/End 间独占鼠标）
- canvas 屏幕区域在 `ed::Begin()` 前用 `GetCursorScreenPos()` 记录到 `doc->canvasScreenMin/Max`
- 加载 bjson 时（`FileOperations.cpp`）自动处理变量节点：隐藏 Name 引脚，修正 Value 类型，自动命名

---

## 关键代码位置

| 功能 | 文件 | 行号/函数 |
|------|------|---------|
| 拖拽生成变量节点 | `BlueprintEditor/EditorUI.cpp` | `pendingVarDrop` 处理块 + `##VarDropMenu` popup |
| 加载时修正变量节点 | `BlueprintEditor/FileOperations.cpp` | `LoadEditorData` → "UE4 风格变量节点" 注释块 |
| 变量面板渲染 | `BlueprintEditor/InspectorPanel.cpp` | `DrawVariablePanel()` |
| Drop target | `BlueprintEditor/EditorUI.cpp` | `ed::End()` 之后 "变量/节点库拖拽放置" 块 |
| 断点单步 | `Runtime/BlueprintRunner.cpp` | `StepNextNode()` → `m_stepPendingNodes` 遍历 |
| String.Regex handler | `Runtime/handlers/BuiltinHandlers_String.cpp` | `handlers["String.Regex"]` |
| Agent.Plan handler | `Runtime/handlers/BuiltinHandlers_AI.cpp` | `handlers["Agent.Plan"]` |
| WeComBridge 主程序 | `tools/wecom-bridge/wecom_bridge.js` | `handleIncoming()` + `executeBlueprintAndReply()` |

---

## MCP Server 配置（CodeBuddy/Claude Desktop）

```json
{
  "mcpServers": {
    "blueprint": {
      "type": "stdio",
      "command": "python",
      "args": ["C:/path/to/MyBlueprintEditor/tools/mcp_server.py"]
    }
  }
}
```

> `BLUEPRINT_ROOT` 和 `BLUEPRINT_DLL` 有默认推导值，无需显式配置 env。

---

## 待办事项

参见 `TODO.md`。主要已完成项：
- ✅ P4 Code Review 蓝图（单/批量）
- ✅ String.Regex 节点
- ✅ WeComBridge 企微集成
- ✅ UE4 风格变量节点（拖拽、隐藏 Name 引脚、类型修正）
- ✅ 断点调试 async 节点双路径 bug
