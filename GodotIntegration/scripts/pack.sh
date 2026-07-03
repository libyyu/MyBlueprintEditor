# pack.sh —— 发版打包总入口，分发到各平台脚本。
#
#   用法：bash pack.sh <target>
#   target: windows | linux | macos | android | ios | web | minigame | patch | all
#
# 环境变量（按目标平台需要）：
#   GODOT_BIN            Godot 可执行（导出用）。Win: .../Godot_v4.5-stable_win64_console.exe
#   ANDROID_NDK_HOME     Android NDK（android 需要）
#   (Emscripten)         web / minigame 前先 source emsdk_env
#set -euo pipefail
export SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
source "$SCRIPT_DIR/pack_common.sh"

prepare_target() {
  if is_windows_host; then
    TARGET="windows"
  fi
}


TARGET="${1:-}"
[[ -n "$TARGET" ]] || prepare_target
[[ -n "$TARGET" ]] || die "用法: bash pack.sh <windows|linux|macos|android|ios|web|minigame|patch|all>"

pack_desktop() {  # $1 = windows|linux|macos ; $2 = preset名 ; $3 = 输出文件
  local plat="$1" preset="$2" out="$3"
  log "① engine core ($plat)"; run_build_sh "$plat"
  if [[ "$plat" == "windows" ]]; then
    build_gdext "build" -G "Visual Studio 17 2022" -A x64 -DBP_BUILD_DIR=build-windows -DGDEXT_WITH_LUA=ON
  else
    build_gdext "build-$plat" -DCMAKE_BUILD_TYPE=Release -DBP_BUILD_DIR="build-$plat" -DGDEXT_WITH_LUA=ON
  fi
  godot_export "$preset" "$DIST_DIR/$out"
  log "$plat 完成 → $DIST_DIR/$out"
}

pack_web() {
  check_emcmake
  
  # ① 引擎核心（wasm 静态 .a）
  log "① building engine core (wasm/emscripten, static)"
  run_build_sh wasm

  # ② GDExtension（emcmake 驱动；CMakeLists 检测到 EMSCRIPTEN 会静态链接引擎 .a）
  log "② building GDExtension (emscripten)"
  local tmpdir=`pwd`
  cd "$GODOT_INT_DIR"
  emcmake cmake -B build-wasm-gdext \
  -DBP_BUILD_DIR="build-wasm" \
  -DGDEXT_WITH_LUA=ON \
  -DCMAKE_BUILD_TYPE=Release
  cmake --build build-wasm-gdext --config Release --target blueprint_gdext
  cd "$tmpdir"

  # ③ Godot 导出 Web
  godot_export "Web" "$DIST_DIR/web/index.html"
  log "Web 完成 → $DIST_DIR/web/（需用支持 SharedArrayBuffer 的 http 头托管）"
  warn "Godot 对 Web GDExtension 支持仍在完善；若导出模板不支持，见 docs/01 §7 路线 A/B"
}

pack_android() {
  check_android

  FORMAT="${1:-apk}"
  ABI="${ANDROID_ABI:-arm64-v8a}"
  API="${ANDROID_API:-24}"
  
  # ① 引擎核心（shared .so，Android）
  log "① building engine core (android, $ABI, $API)"
  run_build_sh android

  # ② GDExtension（用 NDK 工具链，链接 android 引擎 .so；带 Lua）
  build_gdext "build-android" \
  -DCMAKE_TOOLCHAIN_FILE="$NDK_PATH/build/cmake/android.toolchain.cmake" \
  -DANDROID_ABI="$ABI" \
  -DANDROID_PLATFORM="android-$API" \
  -DBP_BUILD_DIR="build-android" \
  -DGDEXT_WITH_LUA=ON \
  -DCMAKE_BUILD_TYPE=Release

  # ③ Godot 导出
  if [[ "$FORMAT" == "aab" ]]; then
    godot_export "Android" "$DIST_DIR/game.aab"
  else
    godot_export "Android" "$DIST_DIR/game.apk"
  fi
  log "Android 完成 → $DIST_DIR/game.$FORMAT"
  warn "确保 .gdextension 有 android.*.$ABI 条目，且 game/bin/android/ 下有对应 .so（见 docs/01 §8）"
}

pack_ios() {
  [[ "$(uname -s)" == "Darwin" ]] || die "iOS 打包必须在 macOS 上进行"

  # ① 引擎核心（STATIC .a，iOS）
  log "① building engine core (ios, static)"
  run_build_sh ios

  # ② GDExtension（静态；CMakeLists 检测到 iOS 会自动 _LINK_ENGINE_STATIC 并静态链接引擎 .a）
  build_gdext "build-ios" \
    -DCMAKE_SYSTEM_NAME=iOS \
    -DCMAKE_OSX_SYSROOT=iphoneos \
    -DBP_BUILD_DIR="build-ios" \
    -DGDEXT_WITH_LUA=ON \
    -DCMAKE_BUILD_TYPE=Release

  # ③ Godot 导出 Xcode 工程
  godot_export "iOS" "$DIST_DIR/ios/game.xcodeproj"
  log "iOS 完成 → $DIST_DIR/ios/（用 Xcode 打开签名 + 出 ipa）"
  warn ".gdextension 需 ios.* 条目指向静态库；引擎+GDExtension 均已静态链接（见 docs/01 §6）"
}

pack_minigame() {
  check_emcmake

  OUT="$DIST_DIR/minigame"
  mkdir -p "$OUT"
  
  # ① 引擎核心：小游戏专用 wasm（BLUEPRINT_WXGAME=ON，导出 addFunction 等额外符号）
  log "① building engine wasm for mini-game (BLUEPRINT_WXGAME=ON)"
  run_build_sh wxgame

  # ② 收集产物：wasm + js 胶水 + adapter
  log "② collecting artifacts → $OUT"
  # Emscripten 产物名可能是 BlueprintRuntime.js/.wasm，按实际匹配拷贝
  find build-wxgame -maxdepth 3 \( -name "*.wasm" -o -name "BlueprintRuntime*.js" \) \
    -exec cp -f {} "$OUT/" \; 2>/dev/null || true
  cp -f "$REPO_ROOT/WechatMiniGame/blueprint-wx-adapter.js" "$OUT/" 2>/dev/null \
    || warn "未找到 blueprint-wx-adapter.js"

  # ③ 蓝图资源（.bjson）——小游戏没有 Godot 资源系统，蓝图作为普通资源随包
  mkdir -p "$OUT/blueprints"
  find "$GAME_DIR/blueprints" -name "*.bjson" -exec cp -f {} "$OUT/blueprints/" \; 2>/dev/null || true

  log "小游戏产物 → $OUT"
  log "  - BlueprintRuntime.wasm / .js   引擎"
  log "  - blueprint-wx-adapter.js        wx.request 桥接"
  log "  - blueprints/*.bjson             蓝图数据"
  warn "接入：把上述文件放进你的小游戏工程，按 WechatMiniGame/README.md 用 BlueprintBridge 初始化"
  warn "注意：小游戏形态不含 Godot 渲染/UI；UI 用小游戏引擎(Cocos/Laya)自绘，蓝图只跑逻辑/AI"
}



# 开始执行
mkdir -p "$DIST_DIR" || die "创建 dist 目录失败"

case "$TARGET" in
  windows)  pack_desktop windows "Windows Desktop" "game.exe" ;;
  linux)    pack_desktop linux   "Linux/X11"       "game.x86_64" ;;
  macos)    pack_desktop macos   "macOS"           "game.zip" ;;
  android)  pack_android "${2:-apk}" ;;
  ios)      pack_ios ;;
  web)      pack_web ;;
  minigame) pack_minigame ;;
  patch)    make_patch ;;
  all)
    warn "all = windows + web + minigame + patch（android/ios 需专用环境，单独跑）"
    pack_desktop windows "Windows Desktop" "game.exe"
    pack_web
    pack_minigame
    make_patch ;;
  *) die "未知 target: $TARGET" ;;
esac
