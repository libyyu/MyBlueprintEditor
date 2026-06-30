# 全平台编译指南

本工程由三层组成，编译顺序固定为 **① 引擎核心 → ② godot-cpp → ③ GDExtension 绑定**。

```
┌─ ① BlueprintRuntime（C/C++ 引擎核心）  → dll/so/dylib/a/wasm
├─ ② godot-cpp（Godot 4.5 绑定库）        → libgodot-cpp.*.a/.lib
└─ ③ blueprint_gdext（本工程 GDExtension）→ blueprint_gdext.<plat>.<cfg>.<arch>.<ext>
```

三者都用 CMake。引擎核心用仓库根的 `build.sh`（busybox 驱动），GDExtension 用 `GodotIntegration/CMakeLists.txt`。

---

## 0. 通用前置

| 依赖 | 说明 |
|------|------|
| CMake ≥ 3.20 | 三层都用 |
| Python 3 | godot-cpp 生成绑定代码需要 |
| Godot 4.5 引擎 | 运行/导出（解压即用，无需安装） |
| godot-cpp **4.5 分支** | `git clone --branch 4.5 --recurse-submodules https://github.com/godotengine/godot-cpp.git`（已置于 `GodotIntegration/godot-cpp`） |

> **版本对齐铁律**：godot-cpp 分支必须与 Godot 引擎大版本一致。本工程锁定 **4.5**。引擎升级到 4.6/4.7 时，godot-cpp 也要切到对应分支并重编 GDExtension，否则加载报 ABI 不匹配。

引擎核心的编译开关（`build.sh` 默认全开）：
`BLUEPRINT_LUA=ON`（内置 Lua VM）、`BLUEPRINT_PROTOBUF=ON`、`BLUEPRINT_LUASOCKET=ON`。

---

## 1. Windows (x86_64, MSVC)

### ① 引擎核心
```bash
cd <repo>
./tools/busybox.exe sh build.sh windows
# 产物: build-windows/bin/Release/BlueprintRuntime.dll
#       build-windows/Runtime/Release/BlueprintRuntime.lib   (import lib)
```
> 重编报 “compiler is not a full path” = 旧 CMakeCache 钉死了已升级的 MSVC 工具集。
> 删 `build-windows/CMakeCache.txt` 和 `build-windows/CMakeFiles/<cmakever>/` 后重跑（保留 .obj 增量）。

### ② + ③ godot-cpp 与 GDExtension
```bash
cd GodotIntegration
cmake -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release --target blueprint_gdext
# 产物: game/bin/Release/blueprint_gdext.windows.template_debug.x86_64.dll
```
构建后把 `Release/` 下的两个 dll（`blueprint_gdext.*.dll` + `BlueprintRuntime.dll`）拷到 `game/bin/` 根，与 `blueprint.gdextension` 的 `res://bin/` 对齐。

---

## 2. Linux (x86_64, GCC/Clang)

### ① 引擎核心
```bash
cd <repo>
./build.sh linux
# 产物: build-linux/bin/libBlueprintRuntime.so
```
### ② + ③
```bash
cd GodotIntegration
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
# 产物: game/bin/libblueprint_gdext.linux.template_debug.x86_64.so
```
把 `libBlueprintRuntime.so` 与 GDExtension `.so` 一起放进 `game/bin/`。`.gdextension` 增加 `linux.*.x86_64` 条目（见第 8 节）。

---

## 3. macOS (arm64 / x86_64)

### ① 引擎核心
```bash
cd <repo>
./build.sh macos
# 产物: build-macos/bin/libBlueprintRuntime.dylib
```
### ② + ③
```bash
cd GodotIntegration
cmake -B build -DCMAKE_BUILD_TYPE=Release -DCMAKE_OSX_ARCHITECTURES="arm64;x86_64"
cmake --build build -j
# 产物: game/bin/libblueprint_gdext.macos.template_debug.framework (或 .dylib)
```
> 签名/公证：分发给他人时对 `.dylib`/`.framework` 做 `codesign`。本机开发无需。

---

## 4. Android (arm64-v8a)

需要 Android NDK（`build.sh` 会自动探测 `$ANDROID_NDK`）。

### ① 引擎核心
```bash
cd <repo>
./build.sh android --ndk /path/to/ndk
# 产物: build-android/bin/libBlueprintRuntime.so (arm64-v8a)
```
### ③ GDExtension（用 NDK 工具链编 godot-cpp + 绑定）
```bash
cd GodotIntegration
cmake -B build-android \
  -DCMAKE_TOOLCHAIN_FILE=$ANDROID_NDK/build/cmake/android.toolchain.cmake \
  -DANDROID_ABI=arm64-v8a -DANDROID_PLATFORM=android-21 \
  -DCMAKE_BUILD_TYPE=Release
cmake --build build-android -j
```
把两个 `.so` 放进 `game/bin/android/arm64/`，`.gdextension` 加 `android.*.arm64` 条目。
导出时在 Godot 的 Android 预设里确保 GDExtension 库被打入 APK。

---

## 5. iOS (arm64, 静态链接)

iOS 不允许动态库随意加载，**引擎与 GDExtension 都走静态链接**，`.gdextension` 用 `__Internal`。必须在 macOS 上编。

### ① 引擎核心（静态库）
```bash
cd <repo>
./build.sh ios
# 产物: build-ios/.../libBlueprintRuntime.a
```
### ③ GDExtension（静态库）
```bash
cd GodotIntegration
cmake -B build-ios \
  -DCMAKE_SYSTEM_NAME=iOS -DCMAKE_OSX_ARCHITECTURES=arm64 \
  -DGODOTCPP_TARGET_TYPE=static -DCMAKE_BUILD_TYPE=Release
cmake --build build-ios
```
`.gdextension` 用：`ios.release.arm64 = "res://bin/ios/libblueprint.a"`，`entry_symbol` 不变。
Godot iOS 导出时把 `.a` 链入 Xcode 工程。

---

## 6. WebGL / HTML5 (wasm, Emscripten)

引擎核心已支持 wasm（仓库里已有 `build-wasm/Runtime/libBlueprintRuntime.a`）。
GDExtension 在 Web 上要随 Godot Web 导出模板一起编为 wasm；当前 Godot 对 Web GDExtension 的支持仍在完善，**推荐两种路线**：

- **路线 A（引擎随 GDExtension 编进 wasm）**：用 emcmake 编 godot-cpp + 绑定 + 静态链接 `libBlueprintRuntime.a`，产出 wasm side module，随 Web 导出。
  ```bash
  cd <repo> && ./build.sh wasm --emsdk /path/to/emsdk
  ```
- **路线 B（不经 Godot，纯 wasm + JS）**：复用仓库 `WechatMiniGame/blueprint-wx-adapter.js`，直接在浏览器里跑 `BlueprintRuntime.wasm`，UI 用 HTML/Canvas。**小游戏走这条**（见第 7 节）。

> Web 上文件读取必须走宿主 VFS——本工程的 `godot_file_bridge` 在 Web 导出下自动改用 Godot 的 `FileAccess`（HTTP 拉取），蓝图/Lua 加载无需改代码。

---

## 7. 微信/抖音小游戏（不走 Godot）

小游戏环境禁原生 dll、内存受限，**任何游戏引擎都不适用**。正确路线：

```
小游戏 JS  →  blueprint-wx-adapter.js  →  BlueprintRuntime.wasm
                (wx.request 桥接 LLM/http)   (BLUEPRINT_LUA=ON, .bjson 数据)
```
- 引擎编 wasm：`./build.sh wasm`
- 适配层：仓库 `WechatMiniGame/blueprint-wx-adapter.js`（已存在）
- 资源：`.bjson` + Lua 作为文本资源打进小游戏包，通过适配层的文件回调读取
- **Runtime 代码零修改**：HTTP 经 `BP_SetHttpClient` 注入到 `wx.request`

> 这条线与"换 Godot"无关，是并行的发布通道。

---

## 8. `.gdextension` 多平台清单

`game/bin/blueprint.gdextension` 汇总所有平台（按需补齐）：

```ini
[configuration]
entry_symbol = "blueprint_library_init"
compatibility_minimum = "4.5"

[libraries]
windows.debug.x86_64   = "res://bin/blueprint_gdext.windows.template_debug.x86_64.dll"
windows.release.x86_64 = "res://bin/blueprint_gdext.windows.template_debug.x86_64.dll"
linux.debug.x86_64     = "res://bin/libblueprint_gdext.linux.template_debug.x86_64.so"
linux.release.x86_64   = "res://bin/libblueprint_gdext.linux.template_release.x86_64.so"
macos.debug            = "res://bin/libblueprint_gdext.macos.template_debug.framework"
macos.release          = "res://bin/libblueprint_gdext.macos.template_release.framework"
android.debug.arm64    = "res://bin/android/libblueprint_gdext.android.template_debug.arm64.so"
android.release.arm64  = "res://bin/android/libblueprint_gdext.android.template_release.arm64.so"
ios.release.arm64      = "res://bin/ios/libblueprint_gdext.ios.arm64.a"

[dependencies]
windows.debug.x86_64   = { "res://bin/BlueprintRuntime.dll" : "" }
windows.release.x86_64 = { "res://bin/BlueprintRuntime.dll" : "" }
linux.debug.x86_64     = { "res://bin/libBlueprintRuntime.so" : "" }
linux.release.x86_64   = { "res://bin/libBlueprintRuntime.so" : "" }
# iOS/Web 为静态链接，无 dependencies 条目
```

> `template_debug` / `template_release` 由 Godot 导出模式自动选择。开发期（编辑器内运行）总是用 `template_debug`。

---

## 9. 平台速查表

| 平台 | 引擎核心产物 | 链接方式 | GDExtension | 文件读取 |
|------|-------------|----------|-------------|----------|
| Windows | `.dll` + `.lib` | 动态 | `.dll` | FileAccess(OS) |
| Linux | `.so` | 动态 | `.so` | FileAccess(OS) |
| macOS | `.dylib` | 动态 | `.framework` | FileAccess(OS) |
| Android | `.so` | 动态 | `.so` | FileAccess(pck) |
| iOS | `.a` | **静态** | `.a`(`__Internal`) | FileAccess(pck) |
| WebGL | `.a`/wasm | **静态** | wasm | FileAccess(HTTP) |
| 小游戏 | wasm | 静态 | 不适用 | JS 适配层 |
