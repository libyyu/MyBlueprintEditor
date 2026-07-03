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
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
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

case "$TARGET" in
  windows)  pack_desktop windows "Windows Desktop" "game.exe" ;;
  linux)    pack_desktop linux   "Linux/X11"       "game.x86_64" ;;
  macos)    pack_desktop macos   "macOS"           "game.zip" ;;
  android)  bash "$SCRIPT_DIR/pack_android.sh" "${2:-apk}" ;;
  ios)      bash "$SCRIPT_DIR/pack_ios.sh" ;;
  web)      bash "$SCRIPT_DIR/pack_web.sh" ;;
  minigame) bash "$SCRIPT_DIR/pack_minigame.sh" ;;
  patch)    make_patch ;;
  all)
    warn "all = windows + web + minigame + patch（android/ios 需专用环境，单独跑）"
    pack_desktop windows "Windows Desktop" "game.exe"
    bash "$SCRIPT_DIR/pack_web.sh"      || warn "web 跳过（需 emsdk）"
    bash "$SCRIPT_DIR/pack_minigame.sh" || warn "minigame 跳过（需 emsdk）"
    make_patch ;;
  *) die "未知 target: $TARGET" ;;
esac
