# MyBlueprintEditor 开发会话备份（续）
**日期**: 2026-04-10 下午  
**分支**: dev  
**前置备份**: `docs/session-context-2026-04-09.md`

---

## 今日完成的工作

### 1. 编辑器 Bug 修复

| 问题 | 修复位置 | 说明 |
|------|---------|------|
| `FileOperations.cpp` 编译错误（`NormalizePath` 前向引用） | `FileOperations.cpp:DoOpenFile` | 改为内联 lambda `normPath` |
| `Less`/`Greater` 节点孤立（引脚名为空字符串） | `BuiltinNodeDefs.cpp` + `ReflectAgent.bjson` + `MultiTurnAgent.bjson` | 给 Less/Greater 补引脚名 `A`/`B`/`Result` |
| 确认对话框 DPI 适配 | `Dialogs.cpp` | 按钮宽/窗口宽随 `FontSize/13.0f` 缩放 |
| 变量拖到 canvas 无反应（drop target 在 `ed::Begin/End` 内） | `EditorUI.cpp` | 把 drop target 移到 `ed::End()` 之后 |
| 变量节点 Value 引脚类型全是 String | `FileOperations.cpp` + `EditorUI.cpp` | 加载/创建时根据变量定义修正 Value 引脚类型 |
| 类型标签点击无反应（`TextColored` → `IsItemClicked` 不可靠） | `InspectorPanel.cpp` | 改为 `InvisibleButton` + `DrawList::AddText` |
| P4 Batch `ForEach` 孤立（`definitionId: "ForEach"` 不存在） | `P4BatchReviewAgent.bjson` | 改为 `ForEachLoop`，引脚名对齐 |
| P4 Review `Build Diff Command` Arg1 是 Exec 类型 | `P4ReviewAgent.bjson` | 连线 `14010→16003` 改为 `22010→16003` |

### 2. 变量面板 UX 重构（UE4 风格）

**`InspectorPanel.cpp` `DrawVariablePanel()`**：

```
旧：[●] [InputText 始终显示] [String] [🗑]
新：[●] [文字（整行可拖拽）] [String] [🗑]
         └─ 双击进入 InputText 编辑
```

- 整行用透明 `Selectable` 覆盖，拖拽热区从 14px 扩展到全行宽
- 默认显示文字（`DrawList::AddText`），双击切换为 `InputText`
- Enter/失焦确认，Esc 取消
- 类型标签改为 `InvisibleButton` + 叠字（确保点击可靠）

### 3. 变量节点 UE4 风格

**拖拽创建**（`EditorUI.cpp`）：
- 松手弹出 Get/Set 菜单
- 生成节点：`Name` 引脚 `IsHidden=true`，节点标题 `"Get VarName"` / `"Set VarName"`
- `Value` 引脚类型从变量定义获取（不再固定 String）

**加载时自动处理**（`FileOperations.cpp` `LoadEditorData`）：
- 隐藏 `Name` 引脚
- 根据 `data.variables` 修正 `Value` 引脚类型
- 通用名 `"Get Variable"` 自动改为 `"Get XXX"`

**技术关键**：
- Drop target 必须在 `ed::End()` 之后（NodeEditor 在 Begin/End 间独占鼠标输入）
- `canvasScreenMin/Max` 在 `ed::Begin()` 前用 `GetCursorScreenPos()` 记录

---

## 关键代码位置（增量）

| 功能 | 文件 | 关键位置 |
|------|------|---------|
| Drop target（ed::End 后） | `EditorUI.cpp` | `// 变量/节点库拖拽放置（必须在 ed::End() 之后）` |
| canvas 区域记录 | `EditorUI.cpp` | `canvasScreenMin = ImGui::GetCursorScreenPos()` 在 `ed::Begin` 前 |
| 变量行重构 | `InspectorPanel.cpp` | `static int s_editingIdx` 双击编辑逻辑 |
| Value 引脚类型修正（加载） | `FileOperations.cpp` | `LoadEditorData` → `// UE4 风格变量节点` 块 |
| Value 引脚类型修正（创建） | `EditorUI.cpp` | `spawnAndClose` lambda 中查 `ActiveDoc()->variables` |

---

## 项目用途说明（针对 ApiKey 问题）

### 工具本身的两层价值

**Layer 1：可视化蓝图编辑器（不需要 ApiKey）**
- 节点图可视化设计工具，类似 UE4 蓝图
- 流控制（Branch/ForLoop/WhileLoop/Events）
- 数学/字符串/数组/Map/JSON 操作节点
- 文件 I/O、进程执行（Code.Run）、HTTP 请求
- 用途：游戏逻辑、工具脚本、自动化流程（不依赖 LLM）

**Layer 2：AI Agent 运行时（需要 LLM ApiKey）**
- `LLM.Chat`/`LLM.StreamChat`：调用 OpenAI 兼容 API
- `Agent.Plan`/`Agent.Reflect`/`Context.Compress`：高阶 Agent 节点
- `Tool.ForEach`/`Tool.Match`/`Tool.CallByName`：工具调用路由
- `Web.Search`（DuckDuckGo，**无需 ApiKey**）
- `HTTP.Retry`/`HTTP.Get`/`HTTP.Post`（**无需 ApiKey**）
- `Memory.LoadHistory`/`SaveHistory`

### 不需要 ApiKey 就能验证的功能

```bjson
// 示例：Web 搜索 + 格式化输出（完全免费）
OnBeginPlay → Web.Search(Query="AI新闻") → FormatString → PrintString
```

```bjson
// 示例：读文件 → 处理 → 写文件
OnBeginPlay → File.ReadText → StringReplace → File.WriteText
```

```bjson
// 示例：调用本地 API（如 Ollama 本地模型）
OnBeginPlay → HTTP.Post(URL="http://localhost:11434/api/generate") → JSON.GetPath → PrintString
```

### 接入免费/低成本 LLM 选项

| 方案 | ApiKey | 说明 |
|------|--------|------|
| **Ollama（本地）** | 不需要 | 本地跑 llama3/qwen/gemma，`BaseURL=http://localhost:11434/v1` |
| **Groq** | 免费额度很高 | `BaseURL=https://api.groq.com/openai/v1`，llama3 免费 |
| **OpenRouter** | 有免费模型 | `BaseURL=https://openrouter.ai/api/v1`，部分模型免费 |
| **SiliconFlow** | 有免费额度 | 国内，`BaseURL=https://api.siliconflow.cn/v1` |
| **腾讯混元** | 内网可申请 | `BaseURL=https://api.hunyuan.cloud.tencent.com/v1` |

**推荐先用 Ollama 验证**：
```bash
# 安装 Ollama，拉一个模型
ollama pull qwen2.5:7b

# 蓝图里配置
BaseURL = http://localhost:11434/v1
ApiKey  = ollama   # 任意字符串
Model   = qwen2.5:7b
```

### MCP 接入 CodeBuddy 用途（无需 ApiKey）

CodeBuddy 本身有 AI，MCP 工具的作用是让 CodeBuddy 能：
- 列出/读取/执行蓝图文件 → `list_blueprints` / `execute_blueprint`
- 创建新蓝图 → `create_blueprint`
- **蓝图里的 LLM 节点才需要 ApiKey，MCP 本身不需要**

所以用 CodeBuddy + Blueprint MCP 的场景：
1. CodeBuddy 理解用户意图 → 调用 `create_blueprint` 生成蓝图 → 执行（蓝图里不含 LLM 节点）
2. CodeBuddy 调用 `execute_blueprint` 跑一个纯逻辑蓝图（Web 搜索、文件处理等）

---

## 下一步建议

1. **先跑不需要 ApiKey 的蓝图** 验证整个链路：
   - `Web.Search` 节点搜索
   - `HTTP.Get` 调用公开 API
   - `Code.Run` 执行本地脚本

2. **本地 Ollama** 验证 LLM 节点（免费，无网络依赖）

3. **连接 WeComBridge** 验证企微集成（无需 ApiKey，用 Web.Search 蓝图即可）

4. **CodeBuddy MCP** 接入（让 CodeBuddy 帮你生成和执行蓝图）
