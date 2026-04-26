# Unity AI NPC Samples

> 在 Unity 里用 Blueprint Runtime 实现 AI 对话 NPC，**支持 Windows / Mac / iOS / Android / WebGL 全平台**。

## 本目录内容

```
Unity/Samples/AINpc/
├── BlueprintService.cs             ← 单例服务（每个场景需一份）
├── AINpcController.cs              ← NPC 控制器（整段回复版）
├── DialogUI.cs                     ← UI（打字机 + 思考动画）
├── AINpcStreamingController.cs     ← NPC 控制器（流式版）
├── StreamingDialogUI.cs            ← UI（token 实时追加 + 光标闪烁）
├── EmotionalNpcController.cs       ← NPC 控制器（情绪化，LLM 返回 JSON）
├── OpenWorld/                      ← 开放世界风（3D 气泡 + 距离触发 + 深度 Panel）
│   ├── WorldSpeechBubble.cs
│   ├── NpcProximityTrigger.cs
│   ├── InteractionPrompt.cs
│   ├── OpenWorldDialogPanel.cs
│   └── README.md                   ← 开放世界专用教程
├── WebGLNotes.md                   ← WebGL 编译 + CORS 指南
└── README.md                       ← 本文件
```

对应蓝图：
```
data/examples/wxgame/
├── AI_NPC_Dialog.bjson             ← 基础整段对话
├── AI_NPC_Streaming.bjson          ← 流式对话
└── AI_NPC_Emotional.bjson          ← 情绪 + 动作 JSON
```

## 3 种形态对比

|  | 基础版 | 流式版 | 情绪版 |
|---|---|---|---|
| **蓝图** | `AI_NPC_Dialog` | `AI_NPC_Streaming` | `AI_NPC_Emotional` |
| **Controller** | `AINpcController` | `AINpcStreamingController` | `EmotionalNpcController` |
| **UI** | `DialogUI`（打字机） | `StreamingDialogUI`（实时追加） | 自行配合 Animator |
| **LLM 节点** | `LLM.Chat` | `LLM.StreamChat` | `LLM.Chat`（JSON 约束） |
| **响应时间** | 首字 2~3s | 首字 < 1s | 首字 2~3s |
| **沉浸感** | ⭐⭐⭐⭐ | ⭐⭐⭐⭐⭐ | ⭐⭐⭐⭐⭐ |
| **Token 成本** | 基准 | 相同 | +10%（JSON 格式化） |
| **WebGL 支持** | ✅ 完美 | 🟡 降级为整段 + 打字机模拟 | ✅ 完美 |
| **适合场景** | 传统 RPG、卡牌 | 视觉小说、休闲聊天 | 养成、模拟经营 |

## 5 分钟接入（基础版）

### 步骤 1：拷贝文件到 Unity 工程

```
YourUnityProject/Assets/
├── Plugins/
│   ├── x86_64/BlueprintRuntime.dll      ← Windows 原生库
│   ├── WebGL/libBlueprintRuntime.a      ← WebGL 静态库
│   └── ...（其他平台）
├── Scripts/
│   ├── BlueprintRuntime/                ← Unity/Runtime/Scripts/ 整个拷贝
│   └── Samples/AINpc/                   ← 本目录整个拷贝
└── Resources/
    └── Blueprints/
        └── AI_NPC_Dialog.bjson          ← 蓝图文件（作为 TextAsset 加载）
```

编译各平台库：参见 `Unity/README.md`。WebGL 参见 `WebGLNotes.md`。

### 步骤 2：搭建场景

1. 新建空物体 `BlueprintService`，挂 `BlueprintService.cs`
2. 在 Inspector 填：
   - **Llm Base Url**: `https://open.bigmodel.cn/api/paas/v4`
   - **Llm Api Key**: 去 <https://open.bigmodel.cn> 注册拿免费 Key
   - **Llm Model**: `glm-4-flash`
3. 新建空物体 `NPC_MeowMeow`，挂 `AINpcController.cs`
4. 在 Inspector：
   - **Npc Name**: `喵喵店长`
   - **Personality**: （默认即可，或自己改）
   - **Dialog Blueprint**: 把 `AI_NPC_Dialog.bjson` 作为 TextAsset 拖进来

### 步骤 3：搭建 UI

新建 UGUI Canvas，加以下元素：
- `TMP_Text bubbleText`   — NPC 回复气泡
- `GameObject thinkingDots` + `TMP_Text thinkingDotsText` — "..." 指示器
- `TMP_InputField inputField` — 玩家输入框
- `Button sendButton` — 发送按钮

挂 `DialogUI.cs`，在 Inspector 把上面 5 个 UI 元素绑定好，`Target Npc` 指向 `NPC_MeowMeow`。

### 步骤 4：运行

点 Play → 输入"你好" → 等 2~3 秒 → NPC 用打字机效果回复。

---

## 流式版接入（体验升级）

把 `AINpcController` 换成 `AINpcStreamingController`，把 `DialogUI` 换成 `StreamingDialogUI`，蓝图换 `AI_NPC_Streaming.bjson`。

**效果**：
- 玩家发送后 < 1 秒第一个字出现
- 文字一个个蹦出（真·流式，非模拟）
- 光标 `|` 闪烁
- WebGL 下降级为整段返回 + 打字机模拟（体验接近桌面）

**为什么能双向兼容？** `StreamingDialogUI` 里有 `#if UNITY_WEBGL` 分支：
- 桌面：每个 chunk 立即 append
- WebGL：chunk = 完整回复，自动启动协程逐字模拟

---

## 情绪版接入（最高级）

把 NPC GameObject 挂：
- `EmotionalNpcController.cs`（替代上面的控制器）
- `EmotionalNpcAnimatorBridge.cs`（辅助组件，桥接到 Animator）
- `Animator`（Animation Controller 需要 Trigger：`happy`/`sad`/`angry`/`surprised`/`neutral`/`wave`/`jump`/`nod`/`shake`）

蓝图换 `AI_NPC_Emotional.bjson`，它让 LLM 返回：
```json
{
  "reply": "真的吗！我好开心喵~",
  "emotion": "happy",
  "action": "jump"
}
```

**Controller 事件**：
- `OnReply(string)` → UI 显示文字
- `OnEmotion(NpcEmotion)` → NPC 脸部表情切换
- `OnAction(NpcAction)` → NPC 动作（跳/挥手/点头）

这样 NPC 会真实"活起来"：说到开心事蹦一下，说到难过事低头——**全部由 LLM 决定**，不用策划一条条写分支。

---

## 对话 UI 的 6 种表现形式

### 1. 打字机气泡（本目录 `DialogUI`）
字符逐个蹦出，点击气泡可跳过。经典 JRPG 风，温馨稳重。

### 2. 流式 token 滚动（本目录 `StreamingDialogUI`）
像 ChatGPT 一样逐 token 追加，AI 感最强，沉浸度最高。

### 3. 思考中动画（所有方案都含）
`...` 循环、🤔 表情、💭 泡泡。从发送到首字之间的**必备过渡**。

### 4. 选项式多分支（进阶，自行实现）
让 LLM 在回复同时**生成 3 个玩家后续选项**（JSON 数组），UI 显示按钮。选项动态生成，比传统对话树更灵活。

### 5. 情绪反馈（本目录 `EmotionalNpcController`）
LLM 同时输出情绪和动作，NPC Animator 联动，让角色真的"活"起来。

### 6. 3D 世界空间气泡（`OpenWorld/` 子目录 — 完整实现）
Canvas = World Space，挂在 NPC 头顶跟随移动，远离时淡出。适合开放世界、模拟经营。

**详见** [`OpenWorld/README.md`](./OpenWorld/README.md)，里面实现了：
- `WorldSpeechBubble` — 3D 头顶气泡（Billboard + 距离淡出 + 打字机 + 自动收起）
- `NpcProximityTrigger` — 双层半径触发（Greet 10m 打招呼 / Interact 3m 按 E 对话）+ 视野检测
- `InteractionPrompt` — 屏幕 "按 E" 提示（自动追踪最近 NPC）
- `OpenWorldDialogPanel` — 深度对话 Panel（历史记录、多 NPC 切换）

---

## 常见问题

### Q: 为什么不用 C# 直接发 HTTP，要过蓝图？
- **可视化编辑**：策划可以在桌面编辑器里改 NPC 性格、对话逻辑、加判断节点，不用改代码
- **多模型切换**：蓝图里用 `LLM.ChatWithFailover` 节点可以配置多个 Provider，自动故障转移
- **复杂工作流**：比如"先查数据库→用 LLM 生成→过敏感词过滤→再显示"整条流水线
- **热更新**：把蓝图做成 ScriptableObject 或远程下载，改对话不用发版

### Q: WebGL 下流式会 CORS 失败吗？
**大概率会**——浏览器 fetch() 对 SSE（text/event-stream）的 CORS 策略更严。智谱、DeepSeek 等主流 LLM 目前 CORS 配置是允许 JSON 响应的，但 SSE 行为不一致。

**最佳实践**：生产环境用**自建代理**（Cloudflare Workers，30 行代码），Unity 直接请求你的域名，代理再转发到 LLM。见 `WebGLNotes.md` 末尾。

### Q: 如何做"对话历史"聊天记录？
`DialogUI` 示例只显示当前气泡。加 ScrollView 保留历史：
1. 用 `VerticalLayoutGroup` + `ContentSizeFitter`
2. 每次 `OnReply` 时 `Instantiate` 一个气泡 Prefab 塞进去
3. 每次 `Say` 时再加一个玩家气泡
4. ScrollRect 滚动到底

### Q: 上下文记忆怎么做？
当前蓝图每次只传当前 `PlayerInput`，是**无记忆单轮对话**。要做多轮：
1. 蓝图里维护 `Messages` 变量（`JSON.MakeMessageArray`）
2. 每次 `LLM.Chat` 返回后 `Append` 到 `Messages`
3. 下次 `LLM.Chat` 传整个 `Messages` 作为上下文
4. 保留最近 N 轮（用 `Array.Slice`），避免 token 爆炸

参考 `data/examples/MultiTurnAgent.bjson`。

### Q: Token 用超了怎么办？
- 免费层：GLM-4-Flash 完全免费，随便用
- 付费层：蓝图里 `LLM.Chat` 节点把 `Temperature` 调低（0.3）、`MaxTokens` 缩小（150），Token 消耗直接腰斩
- 极限优化：用 `LLM.ChatWithFailover` 配置多 Provider，主用便宜模型，重要对话才走贵模型

---

## 下一步

- [`WebGLNotes.md`](./WebGLNotes.md) — WebGL 编译、CORS、包体优化
- 桌面编辑器 — 直接双击打开 `.bjson` 可视化编辑 NPC 对话逻辑
- [`docs/TODO.md`](../../../docs/TODO.md) — 路线图

有问题欢迎提 Issue！
