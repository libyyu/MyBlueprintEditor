# 各平台打包脚本

发版打包脚本位于 `GodotIntegration/scripts/`，统一入口 `pack.sh`。
每个平台都走**三层构建**：① 引擎核心（`build.sh`）→ ② GDExtension（`CMakeLists.txt`）→ ③ Godot 导出。
小游戏是唯一例外（不经 Godot，见 §5）。

```
scripts/
├── pack.sh            总入口，分发到各平台
├── pack_common.sh     公共配置 + 函数（run_build_sh / build_gdext / godot_export / make_patch）
├── pack_android.sh    Android APK/AAB
├── pack_ios.sh        iOS（须 macOS）
├── pack_web.sh        Web / wasm
└── pack_minigame.sh   微信/抖音小游戏（不经 Godot）
```

## 0. 通用前置 / 环境变量

| 变量 | 用途 |
|---|---|
| `GODOT_BIN` | Godot 可执行（导出用）。Win：`.../Godot_v4.5-stable_win64_console.exe` |
| `ANDROID_NDK_HOME` | Android NDK 路径（android 需要）|
| （Emscripten）| web / minigame 前先 `source emsdk_env` |

导出前提：**先在 Godot 编辑器里为目标平台建好导出预设**（生成 `game/export_presets.cfg`）并安装对应导出模板；Android 还需配 keystore，iOS 需签名。

## 1. 统一入口

```bash
bash scripts/pack.sh <target>
#   windows | linux | macos | android | ios | web | minigame | patch | all
```
`patch` = 只打热更补丁 pck + version.json（见 docs/06）。
`all` = windows + web + minigame + patch（android/ios 需专用环境，单独跑）。

## 2. 桌面（Windows / Linux / macOS）

```bash
GODOT_BIN=<godot> bash scripts/pack.sh windows   # → dist/game.exe
GODOT_BIN=<godot> bash scripts/pack.sh linux     # → dist/game.x86_64
GODOT_BIN=<godot> bash scripts/pack.sh macos     # → dist/game.zip
```
引擎 shared（dll/so/dylib）+ GDExtension 链 import lib，Godot 导出。GDExtension 的 `BlueprintRuntime` 库会被 `copy_if_different` 拷到 `game/bin/`。

## 3. Android

```bash
export ANDROID_NDK_HOME=/path/to/ndk
GODOT_BIN=<godot> bash scripts/pack.sh android apk   # 或 aab
```
- ① `build.sh android`（arm64-v8a 的 `.so`）
- ② GDExtension 用 NDK 工具链交叉编译，`-DBP_BUILD_DIR=build-android`
- ③ Godot 导出 APK/AAB
- `.gdextension` 需有 `android.*.arm64` 条目，`game/bin/android/` 放对应 `.so`（docs/01 §8）

## 4. iOS（须 macOS）

```bash
GODOT_BIN=<godot> bash scripts/pack.sh ios   # → dist/ios/game.xcodeproj
```
iOS 禁动态加载 → 引擎 + GDExtension **都静态**。CMakeLists 检测到 iOS 自动静态链接引擎 `.a`。导出得 Xcode 工程，再签名出 ipa。

## 5. Web / wasm

```bash
source /path/to/emsdk/emsdk_env.sh
GODOT_BIN=<godot> bash scripts/pack.sh web   # → dist/web/index.html
```
Web 禁动态加载 → 引擎编成 wasm 静态 `.a`，GDExtension 用 `emcmake` 静态链接进 wasm。托管需带 `SharedArrayBuffer` 相关 http 头（COOP/COEP）。

> Godot 对 Web GDExtension 的支持仍在完善；若导出模板不支持自定义 GDExtension，见 docs/01 §7 的两条路线。

## 6. 微信 / 抖音小游戏（不经 Godot）

```bash
source /path/to/emsdk/emsdk_env.sh
bash scripts/pack.sh minigame   # → dist/minigame/
```

**小游戏是独立宿主形态**，跟 Godot 那套并行：

```
微信小游戏
 └─ 你的小游戏脚本 (Cocos / LayaAir / 原生 TS/JS)
     └─ blueprint-wx-adapter.js   (wx.request ⇄ Emscripten 胶水)
         └─ BlueprintRuntime.wasm  (BLUEPRINT_WXGAME=ON 编出，70+ 节点 + Lua)
             └─ blueprints/*.bjson
```

- 引擎用 `BLUEPRINT_WXGAME=ON` 编 wasm（导出 `addFunction` 等小游戏所需符号）
- HTTP（`LLM.Chat` 等）经 `wx-adapter.js` 桥到 `wx.request`，Runtime 零修改
- **小游戏不含 Godot 渲染/UI**：UI 用小游戏引擎（Cocos/Laya）自绘，蓝图只跑逻辑/AI
- 产物拷进你的小游戏工程，按 `WechatMiniGame/README.md` 用 `BlueprintBridge` 初始化

## 7. 平台能力矩阵

| 平台 | 引擎链接 | GDExtension | 有 Godot 渲染/UI | 热更 |
|---|---|---|---|---|
| Windows/Linux/macOS | shared | dll/so/dylib | ✅ | ✅ .pck |
| Android | shared .so | .so | ✅ | ✅ .pck |
| iOS | **static .a** | 静态链接 | ✅ | 资源可 .pck，原生层随包 |
| Web | **static .a** | 静态进 wasm | ✅ | ✅ .pck（远端拉）|
| 微信/抖音小游戏 | **static wasm** | ❌ 不用 Godot | ❌（宿主引擎自绘）| 蓝图/资源随小游戏分包 |

> 铁律重申：**能进 `.pck`/分包的资源与脚本可热更；原生二进制（引擎、GDExtension）不可热更**，走整包发版。
