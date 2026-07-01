#!/usr/bin/env bash
# pack_common.sh —— 各平台打包脚本的公共配置与函数。
# 被 pack_android.sh / pack_web.sh / pack_minigame.sh / pack_ios.sh source。
#
# 三层构建顺序（所有平台一致）：
#   ① 引擎核心 BlueprintRuntime  → 用仓库根 build.sh
#   ② GDExtension blueprint_gdext → 用 GodotIntegration/CMakeLists.txt（跨平台已就绪）
#   ③ Godot 导出                  → godot --headless --export-*
set -euo pipefail

# --- 路径（相对本脚本，可被环境变量覆盖）---
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
GODOT_INT_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"          # GodotIntegration/
REPO_ROOT="$(cd "$GODOT_INT_DIR/.." && pwd)"           # MyBlueprintEditor/
GAME_DIR="$GODOT_INT_DIR/game"                         # Godot 工程
BUSYBOX="$REPO_ROOT/tools/busybox.exe"                 # Windows 下的 sh 驱动

# --- 工具（按需用环境变量指定）---
: "${GODOT_BIN:=godot}"          # Godot 编辑器/导出可执行；Win 用 Godot_v4.5-stable_win64_console.exe
: "${GODOT_TEMPLATES:=}"         # 导出模板路径（可选，默认用已安装模板）
: "${NDK_PATH:=${ANDROID_NDK_HOME:-}}"   # Android NDK
: "${DIST_DIR:=$GODOT_INT_DIR/dist}"     # 打包产物输出根

log()  { printf '\033[1;36m[pack]\033[0m %s\n' "$*"; }
warn() { printf '\033[1;33m[pack:warn]\033[0m %s\n' "$*"; }
die()  { printf '\033[1;31m[pack:err]\033[0m %s\n' "$*" >&2; exit 1; }

# 在仓库根跑 build.sh（Windows 走 busybox）
run_build_sh() {
  local args="$*"
  ( cd "$REPO_ROOT"
    if [[ -x "$BUSYBOX" ]]; then "$BUSYBOX" sh build.sh $args
    else sh build.sh $args; fi )
}

# ② 编 GDExtension。参数：<平台build目录名> [额外cmake参数...]
#   例：build_gdext build-android -DCMAKE_TOOLCHAIN_FILE=... -DANDROID_ABI=arm64-v8a -DBP_BUILD_DIR=build-android
build_gdext() {
  local builddir="$1"; shift
  log "② building GDExtension → $builddir"
  ( cd "$GODOT_INT_DIR"
    cmake -B "$builddir" "$@"
    cmake --build "$builddir" --config Release --target blueprint_gdext )
}

# ③ Godot 导出。参数：<preset名> <输出文件>
godot_export() {
  local preset="$1" out="$2"
  [[ -f "$GAME_DIR/export_presets.cfg" ]] || die "缺少 export_presets.cfg（先在 Godot 编辑器里为该平台建导出预设）"
  log "③ Godot export: preset='$preset' → $out"
  "$GODOT_BIN" --headless --path "$GAME_DIR" --export-release "$preset" "$out"
}

# 生成补丁 + version.json（发版热更用）
make_patch() {
  log "打补丁 pck + version.json"
  "$GODOT_BIN" --headless --path "$GAME_DIR" --script res://tools/make_patch.gd
}

mkdir -p "$DIST_DIR"
