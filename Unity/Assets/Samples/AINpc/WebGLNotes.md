# Unity WebGL 接入说明

## 编译 Runtime 为 WebGL 静态库

```bash
# 1. 激活 Emscripten SDK
cd /path/to/emsdk && source ./emsdk_env.sh      # Windows 用 . .\emsdk_env.ps1

# 2. 配置（Runtime-only 静态库模式）
cd /path/to/MyBlueprintEditor
emcmake cmake -B build-wasm \
    -DCMAKE_BUILD_TYPE=Release \
    -DBUILD_RUNTIME_ONLY=ON \
    -DBUILD_SHARED_LIBS=OFF \
    -DBUILD_TESTS=OFF

# 3. 编译
cmake --build build-wasm --parallel

# 4. 产物路径
# build-wasm/Runtime/libBlueprintRuntime.a
```

## 放入 Unity 工程

```
YourUnityProject/
├── Assets/
│   ├── Plugins/
│   │   ├── WebGL/
│   │   │   └── libBlueprintRuntime.a      ← 拷贝到这里
│   │   └── ...
│   ├── Scripts/
│   │   ├── BlueprintRuntime.cs            ← 已有
│   │   └── Samples/AINpc/                 ← 本目录整个拷进去
│   └── Resources/
│       └── Blueprints/
│           ├── AI_NPC_Dialog.bjson        ← 作为 TextAsset 加载
│           └── AI_NPC_Streaming.bjson
```

在 Unity 里打开 Plugin Inspector：
1. 选中 `libBlueprintRuntime.a`
2. Include Platforms → 勾选 **WebGL**，取消其他

## CORS / LLM 域名白名单

**关键点**：WebGL 的 HTTP 请求走浏览器 fetch()，受 **CORS 策略** 限制。

### 做法 1：用支持 CORS 的 LLM 供应商（推荐）

| 供应商 | 免费 | 支持 CORS |
|---|---|---|
| [智谱 GLM](https://open.bigmodel.cn) | ✅ GLM-4-Flash 完全免费 | ✅（默认允许） |
| [DeepSeek](https://platform.deepseek.com) | ✅ 新人额度 | ✅ |
| [通义千问 DashScope](https://dashscope.aliyuncs.com) | 月度免费 | ✅ |
| Groq | 免费 | ✅（但国内需代理） |

### 做法 2：自建代理（生产环境推荐）

即使 LLM 支持 CORS，**把 API Key 暴露到前端是非常危险的**——任何人打开 DevTools 都能拿走 Key 然后烧光你的额度。生产环境必须：

```
Unity WebGL 游戏 → 你的后端代理（带 Key） → LLM API
                      ↑
                    加用户鉴权 + 限流
```

代理只需转发 HTTP 请求，可以用：
- **Cloudflare Workers**（免费，全球 CDN，30 行代码）
- **Vercel Edge Functions**（免费）
- **Node.js + Express**（自己服务器）

Unity 里 `BlueprintService.Instance.SetApiKey("")` 留空，把 `BaseURL` 指向代理地址即可。

## WebGL 限制与适配

| 功能 | 桌面 | WebGL | 适配说明 |
|---|---|---|---|
| `LLM.Chat` | ✅ | ✅ | 直接可用 |
| `LLM.StreamChat` | ✅ 真流式 | 🟡 降级为整段 | `StreamingDialogUI` 做了"打字机模拟" |
| 文件 `BP_LoadFromFile` | ✅ | 🟡 | 改用 `LoadFromJson(TextAsset.text)` |
| 多线程 HTTP | ✅ | ❌ | 主线程 fetch，不阻塞渲染即可 |
| WebSocket | ✅ | ✅ | 走浏览器 WebSocket API |
| `Code.Run`（子进程） | ✅ | ❌ | WebGL 无权限 |
| 本地存储 `Save.*` | ✅ 文件系统 | 🟡 | 映射到 `localStorage` 或 IDBFS |

## 典型 WebGL 场景代码

```csharp
void Start()
{
    // 从 Resources 加载蓝图（推荐方式）
    var bp = Resources.Load<TextAsset>("Blueprints/AI_NPC_Streaming");

    // 或从 StreamingAssets（通过 UnityWebRequest，需要 coroutine）
    // StartCoroutine(LoadFromStreamingAssets("AI_NPC_Streaming.bjson"));

    var runner = BlueprintService.Instance.CreateRunner();
    runner.LoadFromJson(bp.text);
    // ...
}
```

## 调试

WebGL 下原生 `Debug.Log` 会输出到浏览器 Console，蓝图的 `PrintString` 通过 `OnPrint` 事件回调同步触发。

开发阶段建议：
1. 先在 **Editor** 里调通（桌面 DLL）
2. 再做 **Windows Standalone** 验证
3. 最后做 **WebGL Build**

WebGL 构建耗时长（5~15 分钟首次），不要频繁迭代。

## 包体优化

Runtime WASM 约 2~3 MB gzipped。若要进一步裁剪：

```bash
emcmake cmake -B build-wasm \
    -DBLUEPRINT_NO_LUA=ON \          # 裁 Lua 脚本引擎（-400KB）
    -DBLUEPRINT_NO_PROTOBUF=ON \     # 裁 Protobuf（-300KB）
    -DBLUEPRINT_NO_FILESYSTEM=ON \   # 裁默认文件系统（-50KB）
    ...
```

典型裁剪后：1.5 MB gzipped，塞进 Unity WebGL 包没压力。
