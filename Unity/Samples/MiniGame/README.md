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
├── MiniGameBootstrap.cs                      ← 场景启动脚本（推荐放 Bootstrap 场景）
│
├── Storage/
│   ├── BlueprintStorage.cs                   ← 跨平台存储 API
│   ├── NpcMemory.cs                          ← NPC 跨会话记忆组件
│   └── Plugins/WebGL/BlueprintStorage.jslib  ← WebGL/小游戏 storage 桥
│
├── Loader/
│   ├── BlueprintLoader.cs                    ← CDN 动态下载 + 缓存 + ETag 校验
│   └── LlmProxyConfig.cs                     ← LLM 代理配置（Direct/Proxy 切换）
│
├── WeChat/
│   ├── WeChatSDK.cs                          ← Share/Login/Toast/RewardedAd C# API
│   ├── BlueprintWeChatNodes.cs               ← 把 wx.* 注册为蓝图节点
│   └── Plugins/WebGL/WeChatSDK.jslib         ← WebGL → wx.* jslib
│
├── ServerProxy/
│   └── llm-proxy.worker.js                   ← Cloudflare Worker 代理（生产必用）
│
├── Editor/
│   └── MiniGameBuildMenu.cs                  ← Unity 菜单"Blueprint/构建 WebGL..."
│
└── README.md
```

对应蓝图：
```
data/examples/wxgame/
├── AI_NPC_Dialog.bjson      ← 基础对话
├── AI_NPC_Streaming.bjson   ← 流式对话
├── AI_NPC_Emotional.bjson   ← 情绪+动作 JSON 输出
└── AI_NPC_Memory.bjson      ← 记忆增强（好感度/见面次数/事实库）
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
