# AI 魔法酒馆 — 完整搭建指引

## 一键自动搭建（推荐）

1. 确保 `Assets/LlmConfig.asset` 已创建并填好 API Key
2. 菜单：**Blueprint → 生成 AI 酒馆 Demo 场景**
3. 等 2 秒 → 自动保存为 `Assets/AITavernDemo.unity`
4. 点 **Play** → WASD 走近 NPC → 看到头顶气泡 → 按 E 对话

---

## 手动搭建（30 分钟详解）

### 前置准备

1. **API Key**：去 https://open.bigmodel.cn 注册，免费拿 GLM-4-Flash Key
2. **中文字体**（可选但强烈推荐）：
   - `Window → TextMeshPro → Font Asset Creator`
   - Source Font: `C:\Windows\Fonts\msyh.ttc`（微软雅黑）
   - Character Set: `Unicode Range (Hex)` → 填 `4E00-9FFF,3000-303F,FF00-FFEF,0020-007E`
   - Atlas: `4096 x 4096`
   - Generate → Save 为 `Assets/Fonts/ChineseFont SDF.asset`
3. **DLL**：确保 `Assets/Plugins/x86_64/BlueprintRuntime.dll` 存在（从 `build-windows/bin/Release/` 拷贝）

### Step 1：创建 LLM 配置

- Project 面板右键 → **Create → Blueprint → LLM Config** → 命名 `LlmConfig`
- Inspector 填：
  - Mode: `Direct`
  - Direct Base Url: `https://open.bigmodel.cn/api/paas/v4`
  - Direct Api Key: `你的 Key`
  - Direct Model: `glm-4-flash`

### Step 2：场景 Hierarchy

```
Demo （场景）
│
├── TavernFloor ────────────── Plane, Scale(3,1,3), 棕色材质
├── Walls (可选) ───────────── 3 个 Cube 围墙
├── BarCounter ─────────────── Cube, (长6宽1高1.2), 深棕色
├── Table ×2 ──────────────── Cube, 桌子
│
├── TavernLight ────────────── Directional Light, 暖色(1,0.85,0.6)
├── FireLight ──────────────── Point Light, 橙红色, Range=10
│
├── Bootstrap ──────────────── 空 GameObject
│   └── [MiniGameBootstrap]     llmConfig = LlmConfig.asset
│
├── Player ─────────────────── Capsule, Tag="Player", Position(0,1,-5)
│   ├── [Rigidbody]             FreezeRotation ✓
│   ├── [SimplePlayerController]
│   └── Main Camera ───────── 子物体, Tag="MainCamera"
│       ├── [Camera]            FOV=70
│       └── [AudioListener]
│
├── NPC_MeowMeow ──────────── Capsule, 橙色, Position(-2,1,3)
│   ├── [AINpcStreamingController]
│   │     NpcName = "喵喵店长"
│   │     Personality = "你是一家幻想世界酒馆的猫咪店长..."
│   │     StreamingBlueprint = AI_NPC_Streaming.bjson
│   ├── [NpcProximityTrigger]
│   │     Player = Player (Transform)
│   │     StreamNpc = 本物体的 AINpcStreamingController
│   ├── [NpcMemory]
│   │     NpcId = "NPC_MeowMeow"
│   ├── Nameplate ────────── TextMeshPro, "喵喵店长", 头顶1.5m
│   └── SpeechBubble ─────── 子物体, 头顶2.2m
│       ├── [Canvas]          World Space, Scale=0.01
│       ├── [CanvasGroup]
│       ├── [WorldSpeechBubble]
│       │     StreamNpc = 父的 AINpcStreamingController
│       │     FollowTarget = NPC_MeowMeow
│       │     BubbleText = 下面的 TMP
│       ├── BubbleBG ──── Image, 黑色半透明
│       └── BubbleText ── TextMeshProUGUI, 白色, 24号
│
├── NPC_IronHammer ─────────── 同上结构, 红色, (3,1,2)
│     "铁锤大叔" — 铁匠, 壮汉, 豪爽
│
├── NPC_Luna ───────────────── 同上结构, 紫色, (-3,1,-1)
│     "月灵术士" — 精灵, 神秘, 高冷
│
├── NPC_Pippin ─────────────── 同上结构, 绿色, (1,1,-2)
│     "皮皮鼠" — 会说话的老鼠, 胆小八卦
│
├── UICanvas ───────────────── Canvas, Screen Space Overlay
│   ├── InteractPromptPanel ── "按 E 对话" 提示 (默认隐藏)
│   │   └── [InteractionPrompt]
│   ├── DialogPanel ────────── 对话面板 (默认隐藏)
│   │   ├── NameBar ────── NPC 名字
│   │   ├── HistoryScroll ── ScrollRect + Content (VerticalLayoutGroup)
│   │   ├── InputRow ─────── InputField + SendButton
│   │   ├── CloseButton ──── ✕ 按钮
│   │   └── [OpenWorldDialogPanel]
│   └── HelpPanel ──────────── 左上角操作提示
│
└── EventSystem ────────────── EventSystem + StandaloneInputModule
```

### Step 3：关键连线

| 组件 | 字段 | 拖什么 |
|---|---|---|
| MiniGameBootstrap | Llm Config | LlmConfig.asset |
| AINpcStreamingController | Streaming Blueprint | AI_NPC_Streaming.bjson |
| NpcProximityTrigger | Player | Player 物体 |
| NpcProximityTrigger | Stream Npc | 同物体的 AINpcStreamingController |
| WorldSpeechBubble | Stream Npc | 父物体的 AINpcStreamingController |
| WorldSpeechBubble | Follow Target | NPC 物体 Transform |
| WorldSpeechBubble | Bubble Text | BubbleText (TextMeshProUGUI) |
| WorldSpeechBubble | Canvas Group | SpeechBubble 上的 CanvasGroup |
| WorldSpeechBubble | Target Camera | Main Camera |
| OpenWorldDialogPanel | 所有 UI 字段 | 对应的 UI 元素 |
| InteractionPrompt | Panel | InteractPromptPanel |
| InteractionPrompt | Hint Text | PromptText (TextMeshProUGUI) |

### Step 4：测试

1. Play → WASD 走向喵喵店长
2. 距离 < 8m → 头顶气泡自动出现 "你好呀冒险者喵~"
3. 距离 < 3m → 屏幕显示 "按 E 对话"
4. 按 E → 底部对话面板弹出 → 输入 "给我推荐一杯酒" → 发送
5. NPC 流式回复出现在面板 + 头顶气泡
6. ESC 关闭面板 → 走向下一个 NPC

### Step 5：发布到微信小游戏

见 `ServerProxy/DEPLOY.md`

---

## NPC 人设参考

| NPC | 人设关键词 | 互动特点 |
|---|---|---|
| 🐱 **喵喵店长** | 猫咪、温柔、好奇 | 新手引导、推荐酒水、句尾"喵~" |
| 🔨 **铁锤大叔** | 铁匠、壮汉、豪爽 | 讲冒险故事、推荐武器、喝酒比赛 |
| 🌙 **月灵术士** | 精灵、神秘、高冷 | 占星、魔法知识、哲学对话 |
| 🐭 **皮皮鼠** | 老鼠、胆小、八卦 | 偷听八卦、透露秘密、搞笑 |

4 个 NPC 性格互补，玩家有动力和每个都聊一遍。
