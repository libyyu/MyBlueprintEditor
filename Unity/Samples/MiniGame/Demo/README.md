# AI 魔法酒馆 — 完整示例游戏搭建指南

> 3 分钟可玩的 AI NPC 小游戏 Demo，展示框架全部能力。

## 游戏概览

```
╔═══════════════════════════════════════════════════════════════╗
║                    AI 魔法酒馆                                ║
║                                                               ║
║  你是一个刚到小镇的冒险者，走进了一家魔法酒馆。               ║
║  酒馆里有 4 个 NPC，每个都有独特的性格和 AI 驱动的对话。      ║
║  你可以：聊天、购物、接任务、送礼、解锁隐藏角色。             ║
║                                                               ║
║  每个 NPC 会：记住你说过的话、根据好感度改变态度、             ║
║  感知时间/天气/其他 NPC、互相八卦你的事迹。                    ║
╚═══════════════════════════════════════════════════════════════╝
```

## 4 个 NPC

| NPC | 身份 | 蓝图 | 特殊能力 |
|---|---|---|---|
| **喵喵** | 猫咪酒保 | AI_NPC_ShopKeeper.bjson | 卖药水，好感高打折 |
| **老王** | 铁匠 | AI_NPC_WorldAware.bjson | 卖武器，聊冒险故事 |
| **莉莉** | 游吟诗人 | AI_NPC_Questgiver.bjson | 发任务，讲传说 |
| **???** | 神秘人 | AI_NPC_Memory.bjson | 分享 3 次解锁 |

## 搭建步骤（Unity 2021.3+）

### 1. 导入文件

```
Assets/
├── Plugins/
│   └── BlueprintRuntime.dll  (从 build-windows/bin/Release/ 复制)
├── Scripts/
│   └── BlueprintRuntime/     (从 Unity/Runtime/Scripts/ 复制)
├── Samples/
│   ├── AINpc/                (从 Unity/Samples/AINpc/ 复制)
│   └── MiniGame/             (从 Unity/Samples/MiniGame/ 复制)
└── StreamingAssets/
    └── Blueprints/           (从 data/examples/wxgame/ 复制全部 .bjson)
```

### 2. 创建场景

1. **新建场景** → 保存为 `TavernScene`
2. 创建空物体 `GameSystems`，挂载：
   - `MiniGameBootstrap`（填 LLM 配置）
   - `WorldState`
   - `NpcRegistry`
   - `ContentFilter`
   - `RateLimiter`
   - `SaveManager`
   - `GameAudioManager`
   - `TutorialSystem`
   - `TavernDemo`

3. 创建 4 个 NPC (Capsule + 名字标签)，每个挂载：
   - `AINpcStreamingController`（拖入对应蓝图 TextAsset）
   - `NpcMemory`（设置唯一 NpcId）
   - `NpcRegistrant`
   - `NpcBehaviorGate`（配置好感度段位）
   - `NpcProximityTrigger`
   - `NpcSchedule`（配置日程）
   - `WorldSpeechBubble`（3D 气泡子物体）
   - `AffinityHeart`（头顶心子物体）

4. 喵喵和老王额外挂 `NpcShop`（配置商品列表）
5. 神秘人设为 **inactive**

### 3. UI Canvas

创建 Screen Space Canvas，添加：
- `InteractionPrompt`（按 E 提示）
- `OpenWorldDialogPanel`（深度对话面板）
- `DialogChoicePanel`（选项按钮）
- `QuestTrackerUI`（左上角任务列表）
- `QuestRewardPopup`（任务完成弹窗）
- `InventoryUI`（按 I 打开背包）
- `ShopUI`（商店面板）
- `SaveSlotUI`（存档面板）
- `ShareRewardPanel`（分享奖励）
- `MinimapSystem`（右上角小地图）

### 4. 填写 LLM 配置

在 `MiniGameBootstrap` 的 Inspector 里：
- **Mode**: `Proxy`（推荐）或 `Direct`（测试）
- **BaseURL**: `https://open.bigmodel.cn/api/paas/v4`
- **ApiKey**: 去 https://open.bigmodel.cn 注册（免费）
- **Model**: `glm-4-flash`

### 5. 运行！

点 Play → 走到 NPC 身边 → 按 E → 开始聊天

## 核心体验流程

```
玩家进入酒馆
  │
  ├─→ 走近喵喵 → 头顶气泡"欢迎光临喵~"
  │    ├─→ 按 E → 深度对话面板
  │    ├─→ "有什么推荐的？" → LLM 推荐商品 + 显示选项
  │    ├─→ 买药水 → 金币扣除 → 好感度+5
  │    └─→ 多聊几次 → 好感度升级 → "老朋友折扣"
  │
  ├─→ 走近莉莉 → "冒险者，我有个任务给你"
  │    ├─→ 接受任务 → 左上角出现任务追踪
  │    ├─→ 完成条件 → 弹出奖励 → 金币+经验
  │    └─→ 升级 → 全村 NPC 都知道"有人升级了"
  │
  ├─→ 分享给 3 个好友 → 神秘人解锁！
  │    └─→ "我在这里等你很久了..."
  │
  ├─→ 等到深夜 → 喵喵回家了 → "店长已经休息了"
  │    └─→ 老王还在打铁 → "深夜打铁最安静"
  │
  └─→ 退出 → 自动存档 → 下次进来 NPC 记得一切
```

## 系统联动图

```
WorldState（时间/天气）──────────────────────────────────┐
    │                                                     │
    ├─→ NpcSchedule（NPC 位置切换）                       │
    ├─→ NpcRegistrant（注入 WorldTime/Weather）            │
    ├─→ GameAudioManager（BGM 切换）                      │
    │                                                     ▼
PlayerStats ──→ NpcRegistrant ──→ LLM SystemPrompt ──→ NPC 回复
    │                                                     │
    ├─→ Inventory ──→ Shop ──→ 金币变化                   │
    ├─→ QuestSystem ──→ 奖励 ──→ 好感度                  │
    │                                                     │
NpcMemory ◄──── 自动记录对话/好感度 ◄────────────────────┘
    │
    ├─→ NpcBehaviorGate（段位切换 Personality）
    ├─→ AffinityHeart（头顶 ❤ 显示）
    └─→ SaveManager（跨会话持久化）

ContentFilter ──→ 玩家输入过滤 + LLM 输出过滤
RateLimiter   ──→ 冷却 + 每日额度 + 看广告加次数
ShareViralSystem ──→ 分享 3 次 → 解锁神秘人 NPC
```

## 组件总数

| 类别 | 数量 |
|---|---|
| C# 脚本 | 45+ |
| 蓝图模板 (.bjson) | 7 |
| JSON 节点定义 | 28 个 |
| 蓝图节点注册模块 | 7 个 |
| 总代码行数 | ~8000 |

## 发布检查清单

- [ ] LLM API Key 配置正确
- [ ] ContentFilter 的敏感词库已填充
- [ ] RateLimiter 每日额度合理（建议 100 次/天）
- [ ] 所有 NPC 的 NpcId 唯一
- [ ] WebGL 构建测试通过
- [ ] 微信开发者工具预览正常
- [ ] request 合法域名白名单已添加
