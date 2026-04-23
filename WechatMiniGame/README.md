# Blueprint Runtime × 微信小游戏

> **在微信小游戏里跑 AI Agent 蓝图**——Cocos Creator / LayaAir / 原生 WebGL 皆可。

## 能做什么？

- 🎮 **AI NPC 对话**：每个 NPC 是一个蓝图，LLM 驱动回应，玩家说什么都能接
- 🧩 **AI 互动剧本杀**：蓝图编辑剧情分支，LLM 生成对白
- 🔍 **AI 解谜关卡**：LLM 判断玩家的开放式答案
- 🐾 **AI 养成/电子宠物**：可学习、有性格的虚拟角色

配合本项目的**桌面编辑器**，剧本作者拖拽就能做出 AI 故事——无需写代码。

---

## 架构

```
┌────────────────────────────────────────────────┐
│    微信小游戏                                   │
│  ┌────────────────────────────────────────┐   │
│  │  游戏脚本（Cocos / LayaAir / TS/JS）    │   │
│  │     ↓ BlueprintBridge                   │   │
│  │  blueprint-wx-adapter.js                │   │
│  │  (wx.request ⇄ Emscripten 胶水)         │   │
│  │     ↓                                    │   │
│  │  BlueprintRuntime.wasm                   │   │
│  │    ├─ 70+ 节点（流程/变量/计算/JSON/...）│   │
│  │    ├─ LLM.Chat → wx.request 白名单域名  │   │
│  │    └─ 蓝图数据 (.bjson，10~50 KB)        │   │
│  └────────────────────────────────────────┘   │
└────────────────────────────────────────────────┘
```

**Runtime 代码零修改**：通过 `BP_SetHttpClient` 外部注入入口，JS 适配层把 HTTP 请求桥接到 `wx.request`。

---

## 快速开始（5 分钟）

### 步骤 1：编译 WASM

```bash
# 需要 Emscripten SDK (emcmake)
emcmake cmake -B build-wxgame \
    -DCMAKE_BUILD_TYPE=Release \
    -DBUILD_RUNTIME_ONLY=ON \
    -DBUILD_SHARED_LIBS=OFF \
    -DBUILD_TESTS=OFF \
    -DBLUEPRINT_WXGAME=ON

cmake --build build-wxgame --parallel
```

产物：
- `build-wxgame/BlueprintRuntime.js`   (~300 KB)
- `build-wxgame/BlueprintRuntime.wasm` (~2~3 MB gzipped)

**小游戏包体预算 30MB（首包 4MB），WASM 通常占 1~2 MB**，完全塞得进去。

### 步骤 2：放入游戏工程

复制以下文件到你的 Cocos/LayaAir 工程的 `assets/scripts/` 下：

```
WechatMiniGame/
  ├── blueprint-wx-adapter.js        ← 通用适配器
  ├── examples/
  │     └── CocosNPCDemo.ts           ← 参考接入代码
  └── build-wxgame/
        ├── BlueprintRuntime.js       ← Emscripten 胶水
        └── BlueprintRuntime.wasm     ← 蓝图引擎
```

### 步骤 3：配置微信小游戏白名单

在 **微信公众平台 → 小游戏 → 开发设置 → request 合法域名** 添加你使用的 LLM 域名：

| 推荐 LLM（国内可用） | 白名单域名 | 费用 |
|---|---|---|
| 智谱 GLM-4-Flash | `open.bigmodel.cn` | **永久免费** |
| DeepSeek | `api.deepseek.com` | 新人 500 万 token 免费 |
| 通义千问 | `dashscope.aliyuncs.com` | 月度免费额度 |

### 步骤 4：跑起来

在 Cocos Creator 新建场景，挂上 `CocosNPCDemo.ts`，Inspector 填入：
- **ApiKey**：去 [智谱开放平台](https://open.bigmodel.cn) 注册拿**免费 Key**
- **Personality**：`一个活泼好奇的猫咪酒馆老板，说话时喜欢加 "喵~"`

点预览 → 在输入框打字 → 按钮 → NPC 回复出现。

---

## 最小代码示例

```ts
import { BlueprintBridge } from './blueprint-wx-adapter.js';

// 1. 初始化（一次性）
await BlueprintBridge.init({
    wasmUrl:       'BlueprintRuntime.wasm',
    moduleFactory: globalThis.BlueprintRuntime,   // Emscripten 工厂函数
    host:          'wx',                          // 小游戏走 wx.request
});

// 2. 创建 Runner
const runner = BlueprintBridge.createRunner();

// 3. 拿 PrintString 输出作为 NPC 回复文本
runner.onPrint(msg => {
    dialogLabel.string = msg;
});

// 4. 加载蓝图（.bjson 内容作为字符串传入）
runner.loadFromJson(bjsonText);

// 5. 注入参数（LLM Key 永远不写在蓝图里！）
runner.setVariable('ApiKey',      '你的免费 Key');
runner.setVariable('BaseURL',     'https://open.bigmodel.cn/api/paas/v4');
runner.setVariable('Model',       'glm-4-flash');
runner.setVariable('Personality', '活泼的猫咪酒馆老板');

// 6. 玩家说话时触发
onPlayerInput(text => {
    runner.setVariable('PlayerInput', text);
    runner.execute();
});

// 7. 每帧 Tick（推动异步 HTTP 回调）
update(dt) {
    runner.tick(dt);
}

// 8. 场景销毁时
onDestroy() {
    runner.destroy();
}
```

---

## 示例蓝图

位于 `data/examples/wxgame/`：

| 蓝图 | 说明 |
|---|---|
| `AI_NPC_Dialog.bjson` | 基础 AI NPC 对话（本目录 `examples/CocosNPCDemo.ts` 用的就是它） |

更多模板陆续加入。欢迎 PR！

---

## 常见问题

### Q：WASM 包体太大怎么办？
- 用 `-O2` + `wasm-opt -Oz`（Emscripten 自动做）
- 裁剪掉不用的模块：`-DBLUEPRINT_NO_LUA=ON -DBLUEPRINT_NO_PROTOBUF=ON`
- 把 `.wasm` 放 CDN，**不打进包体**（启动时下载，可分包）

### Q：LLM 回复慢，怎么办？
- GLM-4-Flash 免费版约 2~5 秒/回复
- 界面显示「...」占位，别让玩家以为卡死
- 可以在蓝图里先 `PrintString("思考中...")` 再发 LLM 请求

### Q：怎么保护 API Key？
- **绝对不要**把 Key 硬编码在蓝图 JSON 里
- Key 应该：① 通过后端代理转发，或 ② 用户自己填（工具类小游戏）
- 本示例所有 LLM 参数都走变量注入，就是为了这点

### Q：能跑实时游戏（高帧率动作）吗？
- 蓝图每次 execute 毫秒级开销，完全够用
- 但 LLM.Chat 是网络请求（秒级），不适合每帧调
- 典型模式：**蓝图控制逻辑，引擎负责渲染和输入**

---

## 路线图

- [x] HTTP JSBridge 适配器
- [x] Cocos Creator 接入示例
- [x] AI NPC 对话示例蓝图
- [ ] LayaAir 接入示例
- [ ] 微信本地存储 JSBridge（替代 BP_FileRead/Write）
- [ ] 抖音小游戏（tt.request）实测
- [ ] 离线 LLM（ONNX / WebLLM）接入

欢迎提 Issue / PR！
