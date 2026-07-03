# pack_common.sh —— 各平台打包脚本的公共配置与函数。
# 被 pack_android.sh / pack_web.sh / pack_minigame.sh / pack_ios.sh source。
#
# 三层构建顺序（所有平台一致）：
#   ① 引擎核心 BlueprintRuntime  → 用仓库根 build.sh（内部走 busybox）
#   ② GDExtension blueprint_gdext → 用 GodotIntegration/CMakeLists.txt（跨平台已就绪）
#   ③ Godot 导出                  → godot --headless --export-release
#
# 运行方式（二选一）：
#   - 真 bash（推荐）：bash scripts/pack.sh <target>
#   - RunShell.bat（busybox，Windows）：RunShell.bat pack.sh <target>
#     RunShell.bat 会 set USE_BUSYBOX=1，本脚本据此用 $0 兜底定位目录。
#set -euo pipefail
HOST_OS="$(uname -s)"

# 是否用 RunShell.bat（busybox）驱动。RunShell.bat 会 set USE_BUSYBOX=1。
# 用 ${VAR:-} 兜底：即使外部没定义该变量，set -u 下也不会报 "parameter not set"。
IS_BUSYBOX=
if [ "${USE_BUSYBOX:-}" = "1" ]; then
  IS_BUSYBOX=true
fi

IS_MACOSX=
if [ "$HOST_OS" = "Darwin" ]; then
  IS_MACOSX=true
fi

# 定位脚本自身目录。busybox 的 sh 不可靠支持 ${BASH_SOURCE}，用 $0 兜底。
if [ -z "${SCRIPT_DIR:-}" ] ; then
  if [ "$IS_BUSYBOX" = "true" ]; then
    SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
  else
    SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE:-$0}")" && pwd)"
  fi
fi

# --- 路径（相对本脚本，可被环境变量覆盖）---
GODOT_INT_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"          # GodotIntegration/
REPO_ROOT="$(cd "$GODOT_INT_DIR/.." && pwd)"           # MyBlueprintEditor/
GAME_DIR="$GODOT_INT_DIR/game"                         # Godot 工程
BUSYBOX="$REPO_ROOT/tools/busybox.exe"                 # Windows 下的 sh 驱动

# --- 工具（按需用环境变量指定）---
: "${GODOT_BIN:=godot}"          # Godot 编辑器/导出可执行；Win 用 Godot_v4.5-stable_win64_console.exe
: "${GODOT_TEMPLATES:=}"         # 导出模板路径（可选，默认用已安装模板）
: "${DIST_DIR:=$GODOT_INT_DIR/dist}"     # 打包产物输出根
: "${EMSDK_PATH:=${EMSDK:-}}"     # Emscripten
: "${NDK_PATH:=${ANDROID_NDK_HOME:-}}"   # Android NDK

log()  { printf '\033[1;36m[pack]\033[0m %s\n' "$*"; }
warn() { printf '\033[1;33m[pack:warn]\033[0m %s\n' "$*"; }
die()  { printf '\033[1;31m[pack:err]\033[0m %s\n' "$*" >&2; exit 1; }

is_windows_host() {
    case "$HOST_OS" in
        MINGW*|MSYS*|CYGWIN*|Windows_NT|WINDOWS*) return 0 ;;
        *) return 1 ;;
    esac
}

# 在仓库根跑 build.sh。Windows 下用仓库自带 busybox 驱动（build.sh 依赖它）；
# 其它平台用系统 sh。
run_build_sh() {
  log "① running build.sh $*"
  local args="$*"
  local tmpdir; tmpdir="$(pwd)"
  cd "$REPO_ROOT"
  sh build.sh $args
  cd "$tmpdir"
}

# ② 编 GDExtension。参数：<平台build目录名> [额外cmake参数...]
#   例：build_gdext build-android -DCMAKE_TOOLCHAIN_FILE=... -DANDROID_ABI=arm64-v8a -DBP_BUILD_DIR=build-android
build_gdext() {
  local builddir="$1"; shift
  log "② building GDExtension → $builddir"
  local tmpdir=`pwd`
  cd "$GODOT_INT_DIR"
  cmake -B "$builddir" "$@"
  cmake --build "$builddir" --config Release --target blueprint_gdext
  cd "$tmpdir"
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

EMCMAKE=
check_emcmake() {
  if [ -z "$EMSDK_PATH" ]; then
      if command -v emcmake >/dev/null 2>&1; then
          EMSDK_PATH="$(dirname "$(command -v emcmake)")"
      elif command -v emsdk >/dev/null 2>&1; then
          EMSDK_PATH="$(dirname "$(command -v emsdk)")"
          echo "EMSDK_PATH $EMSDK_PATH"
          if [ -f "${EMSDK_PATH}/emsdk_env.sh" ]; then
              . "${EMSDK_PATH}/emsdk_env.sh" #>/dev/null 2>&1 || true
          fi
      else
          die "Emscripten not found. Install the Emscripten SDK or pass EMSDK"
      fi
  fi
  if [ ! -f "${EMSDK_PATH}/emcmake" ] && [ ! -f "${EMSDK_PATH}/upstream/emscripten/emcmake" ]; then
      if [ -f "${EMSDK_PATH}/emsdk_env.sh" ]; then
          . "${EMSDK_PATH}/emsdk_env.sh" >/dev/null 2>&1 || true
      fi
  fi
      
  if is_windows_host; then
      EMCMAKE="$(command -v ${EMSDK_PATH}/upstream/emscripten/emcmake.py 2>/dev/null)" || error "emcmake.py not found after sourcing EMSDK. Check your Emscripten installation."
  else
      EMCMAKE="$(command -v emcmake 2>/dev/null)" || error "emcmake not found after sourcing EMSDK. Check your Emscripten installation."
  fi
  echo "Using Emscripten EMCMAKE: ${EMCMAKE}"

  if [ -z "$EMCMAKE" ]; then
    die "需要 Emscripten（先 source emsdk_env）"
  fi
}

check_android() {
  [[ -n "$NDK_PATH" ]] || die "需要 ANDROID_NDK_HOME / NDK_PATH"
}
