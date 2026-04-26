# Unity AI NPC — 开放世界风接入

> 玩家走近 NPC 看到头顶 3D 气泡、按 E 打开深度对话 Panel。类似《荒野大镖客2》《赛博朋克2077》的路边 NPC 交互。

## 效果

```
  玩家 ────── 12m ──────► 🧑‍🍳 NPC
                          └── GreetRadius 触发
                          └── "嗨，冒险者~" 头顶气泡自动弹出

  玩家 ────── 3m ──────► 🧑‍🍳 NPC
                          └── InteractRadius 触发
                          └── 屏幕提示 "按 [E] 与 喵喵店长 对话"

  玩家按 E                  打开深度对话 Panel
                          └── 历史记录滚动
                          └── 支持多轮对话
                          └── Esc 退出
```

---

## 文件清单

```
OpenWorld/
├── WorldSpeechBubble.cs        ← 3D 头顶气泡（跟随/Billboard/距离淡出/打字机）
├── NpcProximityTrigger.cs      ← 距离触发器（Greet/Interact 双层 + 视野检测）
├── InteractionPrompt.cs        ← 屏幕 "按 E" 提示（自动找最近 NPC）
├── OpenWorldDialogPanel.cs     ← 深度对话 Panel（历史记录 + 多 NPC 切换）
└── README.md
```

**依赖**：上一级的 `AINpcController` / `AINpcStreamingController` / `EmotionalNpcController`。三种都兼容——OpenWorld 组件会自动识别。

---

## 场景搭建（15 分钟）

### 1. 基础设施

```
Hierarchy
├── Main Camera
├── Player (tag=Player)                    ← 玩家角色
├── BlueprintService                       ← 挂 BlueprintService.cs
└── UI Canvas (Screen Space - Overlay)
    ├── InteractPromptPanel                ← 挂 InteractionPrompt.cs
    │   └── HintText (TMP_Text)
    └── DialogPanel                        ← 挂 OpenWorldDialogPanel.cs
        ├── NpcNameText
        ├── HistoryScrollView
        │   └── Content (VerticalLayoutGroup + ContentSizeFitter)
        ├── InputField
        ├── SendButton
        └── CloseButton
```

### 2. 单个 NPC

```
Hierarchy
└── NPC_MeowMeow
    ├── Mesh / Animator (可选)
    ├── AINpcStreamingController.cs        ← 或基础/情绪版三选一
    ├── NpcProximityTrigger.cs
    │   - greetRadius     = 10
    │   - interactRadius  = 3
    │   - greetPrompt     = （默认）
    │
    └── SpeechBubble (子物体)
        ├── Canvas (Render Mode = World Space)
        │   ├── Width/Height = 400x120
        │   ├── Scale = 0.01,0.01,0.01     ← 很重要
        │   └── CanvasGroup
        ├── WorldSpeechBubble.cs
        │   - streamNpc     = [父物体的 Controller]
        │   - followTarget  = [父物体 Transform]
        │   - worldOffset   = (0, 2.2, 0)
        │   - maxVisibleDistance = 25
        │   - fadeStartDistance  = 18
        │   - autoHideDelay      = 6
        │   - canvasGroup = [子 Canvas 的 CanvasGroup]
        │   - bubbleText  = [子 TMP_Text]
        └── (Canvas 下的 Image 背景 + TMP_Text 内容 + Image 三角尾巴)
```

### 3. 把 Panel 接起来

选中 `InteractPromptPanel`（挂 `InteractionPrompt`）：
- Inspector → `onInteract (UnityEvent)` → `+` → 拖入 `DialogPanel`
- 选择函数 `OpenWorldDialogPanel.OpenForTrigger(NpcProximityTrigger)`

这样玩家走进 3m 按 E → InteractionPrompt 发出事件 → OpenWorldDialogPanel 打开。

### 4. 场景里复制多个 NPC

直接复制 `NPC_MeowMeow` 整个物体，改名、改位置、改 `AINpcController` 的 `Personality`（"猫咪店长" → "铁匠大叔" → "流浪诗人"……），即可。`InteractionPrompt` 会自动发现所有 `NpcProximityTrigger`。

---

## 核心机制详解

### 1. WorldSpeechBubble：4 层叠加的距离-alpha 系统

| 层 | 控制来源 | 作用 |
|---|---|---|
| **文本层** | `_buf.Length > 0` | 有内容才显示 |
| **淡入/淡出** | `FadeCo` 协程 | 进入/离开范围平滑过渡 |
| **距离衰减** | `LateUpdate` 计算 | 远处自动变透明 |
| **自动收起** | `AutoHideCo` | NPC 静默 N 秒后收起 |

四者以 `Mathf.Min` 组合，任何一个变 0 都立刻隐藏。

### 2. NpcProximityTrigger：双层半径 + 滞回

```
玩家距离 ──────────────────►
                  ◄──────────── greetRadius (10m)
                        ◄────── interactRadius (3m)
                  ◄──────────── leaveHysteresis (1.5m)
```

- 进入 10m 内才触发 Greet（避免远距离 spam）
- 进入 3m 才显示"按 E"提示
- 滞回机制避免玩家刚好站在边缘时反复进出

### 3. 视野检测（可选）

开启 `requireLineOfSight` 后：
- 射线从 NPC 眼睛射向玩家
- 中间被 `occluderMask`（墙/建筑）挡住 → 不触发打招呼
- **开放世界真实感关键**：NPC 看不见你就不会主动打招呼

### 4. 动态生成问候语

```
greetPrompt = "[系统提示] 玩家走到附近（{time}）。请用符合人设的一句话打招呼，不超过 20 字。"
```

`{time}` 自动替换为 `清晨/中午/下午/傍晚/深夜`。配合 NPC 的 Personality，LLM 就会生成：
- 清晨："早啊冒险者，来点热汤暖暖身子喵~"
- 深夜："这么晚了还不睡喵？要喝杯酒吗？"
- 下午："嘿！来得正好，今天的面包刚出炉~"

**没有任何策划硬编码对白**。这就是 AI NPC 的魅力。

---

## 与传统对话系统对比

| | 传统对话树 | AI NPC（本方案） |
|---|---|---|
| **对白内容** | 策划写死（Excel/YAML） | LLM 实时生成，带上下文感知 |
| **问候语** | 固定一句 "欢迎光临" | 按时段/天气/玩家状态变化 |
| **对话长度** | 有限（几十句） | 无限（玩家说什么都能接） |
| **NPC 人设修改** | 改表 + 改 Unity | **只改 Personality 字段** |
| **Token 成本** | 0 | GLM-4-Flash 免费 / 付费约 ¥0.001/次 |
| **离线运行** | ✅ | ❌（需要 LLM） |
| **情感表达** | 需写条件分支 | 用情绪版 NPC 自动输出 emotion |

**建议**：核心剧情 NPC 用传统对话树（剧情稳定性 > AI 创造力），路人 NPC / 氛围 NPC 用本方案（沉浸感拉满）。

---

## 性能与开销

### 单 NPC 开销
- `WorldSpeechBubble.LateUpdate`：< 0.01ms（纯数学 + 1 次 Distance）
- `NpcProximityTrigger.Update`：< 0.005ms（不开 LOS 检测时）
- **100 个 NPC 同屏：总 < 1ms**

### 视野检测优化
开启 `requireLineOfSight` 后每帧一次 Raycast。100 个 NPC 会有 100 次 Raycast——**打开 `Physics.queriesHitTriggers = false`** 并让遮挡物用简单碰撞体（BoxCollider），1000 个 NPC 仍能保持 60 FPS。

### LLM 调用优化
- **冷却**：`greetCooldown = 30s` 避免反复进出刷屏
- **缓存问候语**（进阶）：首次进入触发后，结果缓存 5 分钟
- **批量模式**（进阶）：大世界同时进入多个 NPC 的 Greet 范围时，只触发距离最近的一个

---

## 进阶玩法

### A. NPC 互相能"看到"对方
在 Greet 范围增加"检测其他 NPC"逻辑，让 NPC 之间触发对话（在蓝图里 `FireEvent` 给其他 Runner），形成真实的**村庄生态**。

### B. 玩家经过触发"背景闲聊"
玩家只是路过而没停下（速度 > 阈值），NPC 自言自语 "看那个冒险者走得好急啊..."，不打断玩家节奏又提升世界密度。

### C. 季节/天气系统联动
蓝图里读取 `GameState.Weather` 变量，SystemPrompt 动态拼接：
```
Personality + "当前是" + 天气 + "。请根据天气调整语气。"
```

雨天 NPC 会抱怨天气，晴天会愉快招呼。

### D. 玩家情感系统
给每个 NPC 维护一个 `Affinity` 好感度变量，对话时传入 Prompt：
```
"你对玩家的好感度是 {Affinity}（0=讨厌，100=爱慕）。"
```

LLM 会自动调整语气。玩家做某些行为（送礼/完成任务）就加好感，形成长期养成。

### E. 玩家说谎检测
玩家说了某些信息后，让 LLM 判断真伪（用 Structured Output 返回 `{is_lying: bool}`），NPC 反应联动。

---

## 下一步我可以帮你做

1. **对话历史持久化**（每个 NPC 记住上次聊了什么，跨游戏会话）
2. **NPC 互相对话的 Demo**（A 和 B 看到玩家走过去，A 对 B 说"那小伙子又来了"）
3. **玩家情感系统 + 好感度驱动 Prompt**
4. **按天气/时段动态 Prompt 的完整蓝图**

告诉我做哪个。
