# Unity → 微信小游戏完整方案

> 让 Unity 开发者一站式把 AI NPC 蓝图打包成可上线的微信小游戏。

## 工作流全景

```
┌─────────────────────────────────────────────────────────────┐
│  Unity 工程                                                   │
│  ┌──────────────────────────────────────────────────────┐   │
│  │  BlueprintRuntime.cs          ← 核心 P/Invoke         │   │
│  │  Samples/AINpc/*.cs            ← NPC 控制 + UI 三件套 │   │
│  │  Samples/AINpc/OpenWorld/*.cs  ← 开放世界风组件       │   │
│  │  Samples/MiniGame/                                    │   │
│  │    ├── MiniGameBootstrap.cs    ← 启动总管             │   │
│  │    ├── Storage/                ← 持久化（对话历史）    │   │
│  │    ├── Loader/                 ← CDN 动态蓝图 + 代理  │   │
│  │    ├── WeChat/                 ← wx.* 桥接 C# API     │   │
│  │    ├── ServerProxy/            ← Cloudflare Worker    │   │
│  │    └── Editor/                 ← 一键构建菜单         │   │
│  └──────────────────────────────────────────────────────┘   │
└─────────────────────────────────────────────────────────────┘
                           │ Blueprint/构建 WebGL
                           ▼
                   Unity WebGL 输出（.wasm + .data）
                           │
                           ▼
              微信 Unity WebGL 转换工具（官方）
              minigame-unity-webgl-transform
                           │
                           ▼
                   微信小游戏工程（.json + game.js）
                           │ 上传 + 审核
                           ▼
                       线上小游戏
```

---

## 文件清单

```
Samples/MiniGame/
├── MiniGameBootstrap.cs                      ← 场景启动脚本
│
├── Storage/
│   ├── BlueprintStorage.cs                   ← 跨平台存储 API
│   ├── NpcMemory.cs                          ← NPC 跨会话记忆（好感度/历史）
│   └── Plugins/WebGL/BlueprintStorage.jslib
│
├── World/                                     ← ★ 多 NPC 世界编排
│   ├── WorldState.cs                         ← 时间/天气/全局事件
│   ├── NpcRegistry.cs                        ← NPC 注册 + 附近感知 + 话题广播
│   ├── NpcBehaviorGate.cs                    ← 好感度段位切换 Personality
│   └── AffinityHeart.cs                      ← 头顶 ❤❤♡♡♡ UI
│
├── Loader/
│   ├── BlueprintLoader.cs                    ← CDN 动态蓝图 + ETag 热更新
│   └── LlmProxyConfig.cs                     ← LLM 代理配置
│
├── WeChat/
│   ├── WeChatSDK.cs                          ← Share/Login/Toast/RewardedAd
│   ├── BlueprintWeChatNodes.cs               ← 把 wx.* 注册为蓝图节点
│   ├── ShareViralSystem.cs                   ← ★ 分享裂变统计 + 冷却
│   ├── ShareRewardPanel.cs                   ← ★ 分享奖励 UI（3 种模式）
│   └── Plugins/WebGL/WeChatSDK.jslib
│
├── ServerProxy/
│   ├── llm-proxy.worker.js                   ← Cloudflare Worker 代理
│   └── DEPLOY.md                             ← ★ 10 分钟部署教程（含截图）
│
├── Editor/
│   └── MiniGameBuildMenu.cs                  ← "Blueprint/构建 WebGL..."
│
└── README.md
```

对应蓝图：
```
data/examples/wxgame/
├── AI_NPC_Dialog.bjson         ← 基础对话
├── AI_NPC_Streaming.bjson      ← 流式对话
├── AI_NPC_Emotional.bjson      ← 情绪 + 动作 JSON
├── AI_NPC_Memory.bjson         ← 好感度/见面次数/事实库
└── AI_NPC_WorldAware.bjson     ← ★ 世界感知（时间/天气/附近 NPC）
```

---

## 30 分钟跑通流程

### 前置准备
1. Unity 2021.3+ (LTS)
2. 去 <https://open.bigmodel.cn> 注册，拿**免费 GLM-4-Flash API Key**
3. 用 `cmake` 编译 Runtime：
   ```bash
   # Windows DLL（Editor 测试用）
   cmake -B build -DBUILD_SHARED_LIBS=ON -DBUILD_RUNTIME_ONLY=ON -DCMAKE_BUILD_TYPE=Release
   cmake --build build --config Release
   # → build/Runtime/Release/BlueprintRuntime.dll
   ```
   WebGL 编译见 `Unity/README.md`。

### 第一步：Unity 工程搭建

1. 新建 Unity 项目（任意模板）
2. 拷贝文件到 `Assets/`：
   ```
   Assets/
   ├── Plugins/
   │   ├── x86_64/BlueprintRuntime.dll       ← Editor/Standalone
   │   ├── WebGL/libBlueprintRuntime.a       ← WebGL（拷到这里）
   │   ├── WebGL/BlueprintStorage.jslib      ← 从 MiniGame/Storage/Plugins/WebGL/ 拷贝
   │   └── WebGL/WeChatSDK.jslib             ← 从 MiniGame/WeChat/Plugins/WebGL/ 拷贝
   ├── Scripts/
   │   ├── BlueprintRuntime.cs               ← Unity/Runtime/Scripts/ 拷贝
   │   ├── AINpc/                            ← Unity/Samples/AINpc/ 整个拷贝
   │   └── MiniGame/                         ← Unity/Samples/MiniGame/ 整个拷贝（Plugins 除外）
   └── Resources/
       └── Blueprints/
           └── AI_NPC_Dialog.bjson           ← data/examples/wxgame/ 里的 .bjson
   ```

### 第二步：创建 LLM 配置

菜单 `Assets → Create → Blueprint → LLM Config`，命名 `LlmConfig`，Inspector 填：
- **Mode**: `Direct`（开发期）
- **Direct Base Url**: `https://open.bigmodel.cn/api/paas/v4`
- **Direct Api Key**: 刚才注册的 Key
- **Direct Model**: `glm-4-flash`

### 第三步：场景搭建

```
Hierarchy
├── Main Camera
├── Bootstrap                          ← 挂 MiniGameBootstrap.cs，Inspector：
│   │                                     - LlmConfig = 上一步的 ScriptableObject
│   │                                     - BlueprintCdnBaseUrl = ""（先用本地）
│   │
│   └── BlueprintService（自动创建）
│
├── Player (tag=Player)
│
├── NPC_MeowMeow
│   ├── AINpcStreamingController.cs
│   │   - StreamingBlueprint = 拖 AI_NPC_Streaming.bjson
│   │   - Personality = "..."
│   ├── NpcProximityTrigger.cs
│   ├── NpcMemory.cs                    ← 加上这个就有跨会话记忆
│   │   - NpcId = "NPC_MeowMeow"
│   ├── BlueprintWeChatNodes.cs         ← 让蓝图能调 WeChat.Share
│   │   - TargetController = 指向 AINpcStreamingController
│   └── SpeechBubble (子物体)
│       ├── Canvas (World Space)
│       └── WorldSpeechBubble.cs
│
└── UI Canvas (Screen Space)
    ├── InteractPromptPanel
    └── OpenWorldDialogPanel
```

### 第四步：Editor 里跑通

点 Play → 走近 NPC → 头顶弹气泡 → 按 E → Panel 弹出 → 输入"你好"→ NPC 用人设回复。

**第二次进游戏**：`NpcMemory.cs` 会自动加载上次的对话历史，NPC 会说"你又来啦~"。

### 第五步：构建 WebGL

菜单：**Blueprint → 应用微信小游戏推荐 PlayerSettings**  
菜单：**Blueprint → 构建 WebGL → 微信小游戏**

### 第六步：转成小游戏

按微信官方转换工具指南：
<https://github.com/wechat-miniprogram/minigame-unity-webgl-transform>

1. 下载转换工具
2. 用工具选中 WebGL 输出目录 → 选输出 wxa 目录
3. 用微信开发者工具打开 wxa 目录
4. 扫码预览 → 上传 → 体验版 / 正式版

---

## 生产环境 checklist

上线前务必做这 4 件事：

### 1. LLM 代理（保护 Key）

**否则**：玩家打开微信开发者工具抓包，你的 Key 会在几小时内被烧光。

照 `ServerProxy/llm-proxy.worker.js` 注释里的步骤部署 Cloudflare Worker（免费，10 分钟完事）。

Unity 里 `LlmConfig.Mode` 切到 `Proxy`，填 Worker URL。

### 2. 微信 request 合法域名白名单

微信公众平台 → 开发设置 → request 合法域名 → 添加：
- 你的 Cloudflare Worker 域名（如 `https://llm-proxy.你的用户名.workers.dev`）
- 或者 LLM 供应商原域名（Direct 模式用）
- 如果用动态蓝图加载：CDN 域名（如 `https://cdn.你的游戏.com`）

### 3. 包体控制

Runtime `.a` 约 2~3 MB（Brotli 压缩后）。整包建议保持 < 4MB 首包（微信首包限制）。
- 大资源放**分包**（触发新场景才下载）
- 蓝图用 **`BlueprintLoader` 动态下载**，不打进首包

### 4. 内容审查

LLM 偶尔会输出不合规内容（涉政、黄赌毒）。必须：
- 蓝图里加**关键词过滤节点**（`String.Contains` + `Branch`）
- 或代理服务端过一道敏感词检测（开源库 `node-sensitive` 等）
- 或直接用供应商的内容安全接口（智谱/通义都有）

游戏过审失败 99% 是因为内容安全问题。

---

## 常见问题

### Q: 必须用微信转换工具吗？能直接跑 Unity 原生 WebGL 吗？
能，但两个大坑：
1. 微信小游戏**不是标准浏览器环境**，DOM/CSS 不完整，Unity WebGL 默认模板的 loading 页不会显示
2. `XMLHttpRequest` / `fetch` / `WebSocket` 行为有差异

官方转换工具就是解决这些差异的 shim 层。**必须用**。

### Q: WebGL 下蓝图的 `LLM.StreamChat` 能真流式吗？
不能。`HttpClient_Emscripten` 对 SSE 做了降级（一次性等到全响应）。但 `StreamingDialogUI` 的打字机模拟让体验接近真流式。

### Q: 我的游戏没 3D 角色，能用吗？
当然。`WorldSpeechBubble` 只需要一个 `Transform`（任何 2D/3D GameObject 都行），甚至可以直接放在屏幕上做 2D 聊天窗。

### Q: 蓝图热更新怎么做？
用 `BlueprintLoader.LoadAsync("v2/NPC_MeowMeow.bjson", text => runner.LoadFromJson(text))`，CDN 上改 ETag 即可。客户端下次启动自动拿新版本，**不用重新审核**。

### Q: 玩家跨设备同步对话历史怎么做？
`NpcMemory` 现在存本地。要做云同步：
1. `WeChatSDK.Login` 拿 `code` → 发到服务器换 `openid`
2. `NpcMemory.Save()` 时同时 POST 到你的服务端
3. `NpcMemory.Load()` 时先从服务端拉

可以做成 `NpcMemory` 的一个 `CloudSyncProvider` 接口扩展。

---

## 下一步我可以帮你做

1. **多 NPC 世界编排**（NPC 互相对话 + 世界状态变量）
2. **玩家好感度 UI**（NPC 头上显示 ❤️ x N）
3. **NPC 根据好感度解锁剧情分支**
4. **分享裂变激励**（分享 3 个好友 → LLM 免费额度翻倍）
5. **完整的 Cloudflare Worker 部署教程**（截图）

告诉我继续做哪个。

---

## ★ "活着的村庄" 完整方案（多 NPC + 世界状态 + 好感度 + 裂变）

以上 5 个方向已全部实现！下面是把它们串起来的**完整场景搭建指南**。

### 场景蓝图

```
Hierarchy
├── Main Camera + Player(tag=Player)
│
├── Bootstrap                              ← 总管
│   ├── MiniGameBootstrap.cs               (LlmConfig / CDN / 广告)
│   ├── BlueprintService (自动创建)
│   ├── BlueprintLoader  (自动创建)
│   ├── WorldState                         ← 世界时间/天气/事件
│   │   - secondsPerPhase = 600           (每 10 分钟推进一个时段)
│   ├── NpcRegistry                        ← NPC 发现中心
│   │   - awarenessRadius = 50
│   │   - refreshInterval = 10
│   └── ShareViralSystem                   ← 分享统计
│       - shareTitle = "我在 AI 酒馆聊天..."
│
├── NPC_MeowMeow                           ← 喵喵店长
│   ├── AINpcStreamingController.cs
│   │   - StreamingBlueprint = AI_NPC_WorldAware.bjson
│   ├── NpcProximityTrigger.cs            (Greet 10m / Interact 3m)
│   ├── NpcMemory.cs                      (NpcId = "NPC_MeowMeow")
│   ├── NpcRegistrant.cs                  (injectBeforeEveryChat = true)
│   ├── NpcBehaviorGate.cs                ← 好感度段位切换
│   │   ├── Stranger  (0-20)
│   │   ├── Friend   (41-60) unlockEvent="玩家和喵喵成了朋友"
│   │   └── Lover    (81+)   unlockEvent="喵喵爱上了玩家"
│   ├── BlueprintWeChatNodes.cs           (让蓝图可调 WeChat.Share)
│   │
│   └── SpeechBubble (World Space Canvas)
│       ├── WorldSpeechBubble.cs
│       └── AffinityHeart (子物体 Canvas)
│           - 显示 ❤❤♡♡♡，好感变化飘字
│
├── NPC_Blacksmith                         ← 铁匠老王（同样配置，不同 Personality）
│   └── ... (复制 NPC_MeowMeow 改名 + 改性格 + 改位置)
│
├── NPC_Bard                               ← 游吟诗人莉莉
│   └── ...
│
└── UI Canvas (Screen Space)
    ├── InteractPromptPanel                (按 E 对话)
    ├── OpenWorldDialogPanel              (深度对话面板)
    └── ShareRewardPanel                   ← ★ 裂变奖励 UI
        - mode = Cumulative
        - targetShares = 3
        - rewardDesc = "解锁神秘 NPC：神秘人"
        - onRewardClaimed → 激活 NPC_Mysterious.SetActive(true)
```

### 为什么这样有"活"感？

| 行为 | 实现机制 |
|---|---|
| 喵喵和铁匠会互相提起对方 | `NpcRegistry` 每次对话前注入 `NearbyNpcs = "铁匠老王(25米)；诗人莉莉(38米)"` |
| NPC 清晨精神，深夜犯困 | `WorldState.GetTimeText()` 注入 `WorldTime = "清晨"`，LLM 自动调整语气 |
| 玩家送礼后 NPC 叫你"老朋友" | `NpcBehaviorGate` 切换 Personality 到 Friend 段位 |
| NPC 头顶 ❤ 数量增加 | `AffinityHeart` 订阅 `NpcMemory.OnChanged` |
| 村里发生大事所有 NPC 都知道 | `WorldState.AddEvent` → 下次所有 NPC 都会读到 `RecentEvents` |
| 分享给 3 个好友解锁新 NPC | `ShareRewardPanel` + `ShareViralSystem` 自动记录 |

### 代码示例：好感度驱动 + 事件联动

```csharp
// 在 OpenWorldDialogPanel 或其他地方，监听玩家行为
void OnPlayerGaveGift(NpcMemory targetNpc, string giftName)
{
    targetNpc.AddAffinity(+10);
    targetNpc.RememberFact($"玩家送了我{giftName}");
    WorldState.Instance.AddEvent($"玩家给{targetNpc.NpcId}送了{giftName}");

    // 下次对话，此 NPC 会说"上次那个{giftName}我很喜欢喵~"
    //        别的 NPC 会说"听说你送喵喵礼物了，真大方啊"
}

// 好感度跨段位时自动触发（已由 NpcBehaviorGate 处理）
npcMeow.GetComponent<NpcBehaviorGate>().OnStageChanged += stage => {
    WeChatSDK.ShowToast($"你们的关系升级了：{stage.name}", WeChatSDK.ToastIcon.Success);
};
```

### 蓝图只要一个 `AI_NPC_WorldAware.bjson`

所有 NPC 共用同一个蓝图。**差异全在 Inspector 的 Personality 字段**——这就是 AI NPC 相对传统对话系统的核心优势：**一个蓝图 + N 个人设 = N 个活 NPC**。

### 上线 checklist（照着做一定能过审）

- [ ] 所有 NPC 的 `NpcId` 唯一（避免 NpcMemory 串档）
- [ ] `LlmConfig.Mode = Proxy`，走 Cloudflare Worker（见 `ServerProxy/DEPLOY.md`）
- [ ] 代理层加内容安全过滤（微信过审关键）
- [ ] `request 合法域名` 白名单加上 Worker 域名 + 你的 CDN 域名
- [ ] `Personality` 中不含政治/色情/暴力引导词
- [ ] `WorldState.AddEvent` 加的事件也过内容安全
- [ ] 冷却设置合理（`cooldown >= 1.5s` 防止连点烧 token）

---

## 下一步可以做的事情

1. **剧情任务系统**（Quest 蓝图 + 完成触发奖励）
2. **玩家属性/经验**（金币、等级、装备，和 NPC 对话动态反映）
3. **NPC 互相对话 Demo**（A 看到玩家走进 B 的店 → 评论）
4. **语音版 NPC**（TTS 朗读回复，用 `HTTP.Post` 节点 + 云端 TTS）
5. **纯 2D UI 版本**（不用世界空间 Canvas，做一个聊天 app 风的小游戏）
