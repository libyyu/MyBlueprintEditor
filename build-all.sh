#!/usr/bin/env bash
# ==============================================================================
# build-all.sh – 一键构建所有平台产物并收集到 Unity 插件目录
#
# 用法:
#   ./build-all.sh [options]
#
# 选项:
#   debug              构建 Debug（默认 Release）
#   clean              每个平台构建前清理 build 目录
#   --ndk <path>       Android NDK 路径（android 平台必填）
#   --api <N>          Android 最低 API Level（默认 21）
#   --emsdk <path>     Emscripten SDK 根目录（默认 $EMSDK 环境变量）
#   --unity <path>     Unity Plugins 目标目录（默认 Unity/Runtime/Plugins）
#   --skip <platform>  跳过指定平台（可多次使用，如 --skip android --skip ios）
#   --only <platform>  只构建指定平台（可多次使用）
#   -h/--help
#
# 构建的平台:
#   host     当前宿主（Linux→linux, macOS→macos）完整编辑器 + 运行时 .so/.dylib
#   wasm     WebAssembly 静态库（需要 Emscripten）
#   android  Android ARM64-v8a .so（需要 NDK）
#   ios      iOS arm64 静态库（仅 macOS 宿主）
#   windows  Windows x64 .dll（需要 MinGW-w64 交叉编译器）
#
# 产物收集到 Unity/Runtime/Plugins/:
#   Windows/x86_64/BlueprintRuntime.dll
#   Android/arm64-v8a/libBlueprintRuntime.so
#   iOS/libBlueprintRuntime.a
#   WebGL/libBlueprintRuntime.a
#   macOS/libBlueprintRuntime.bundle  （仅 macOS 宿主）
#   Linux/x86_64/libBlueprintRuntime.so（仅 Linux 宿主）
#
# 示例:
#   ./build-all.sh                          # 构建所有可用平台（Release）
#   ./build-all.sh debug                    # Debug 模式
#   ./build-all.sh --skip android --skip ios  # 跳过移动端
#   ./build-all.sh --only wasm --only windows # 只构建 WASM 和 Windows
#   ./build-all.sh --ndk ~/Android/Sdk/ndk/25.2.9519653
# ==============================================================================

set -euo pipefail

RED='\033[0;31m'; GREEN='\033[0;32m'; YELLOW='\033[1;33m'
CYAN='\033[0;36m'; BOLD='\033[1m'; NC='\033[0m'

info()    { echo -e "${CYAN}[INFO]${NC}  $*"; }
success() { echo -e "${GREEN}[OK]${NC}    $*"; }
warn()    { echo -e "${YELLOW}[WARN]${NC}  $*"; }
error()   { echo -e "${RED}[ERROR]${NC} $*" >&2; exit 1; }
step()    { echo -e "\n${BOLD}──────────────────────────────────────────${NC}"; \
            echo -e "${BOLD}  $*${NC}"; \
            echo -e "${BOLD}──────────────────────────────────────────${NC}"; }

PROJECT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_SH="${PROJECT_DIR}/build.sh"
[[ -f "$BUILD_SH" ]] || error "build.sh not found at ${BUILD_SH}"
chmod +x "$BUILD_SH"

# ── Defaults ──────────────────────────────────────────────────────────────────
BUILD_TYPE="Release"
CLEAN_FLAG=""
NDK_PATH=""
ANDROID_API=21
EMSDK_PATH="${EMSDK:-}"
UNITY_PLUGINS_DIR="${PROJECT_DIR}/Unity/Runtime/Plugins"
SKIP_PLATFORMS=()
ONLY_PLATFORMS=()

HOST_OS="$(uname -s)"

# ── Argument parsing ──────────────────────────────────────────────────────────
while [[ $# -gt 0 ]]; do
    case "$1" in
        debug)          BUILD_TYPE="Debug"; shift ;;
        release)        BUILD_TYPE="Release"; shift ;;
        clean)          CLEAN_FLAG="clean"; shift ;;
        --ndk)          NDK_PATH="$2"; shift 2 ;;
        --api)          ANDROID_API="$2"; shift 2 ;;
        --emsdk)        EMSDK_PATH="$2"; shift 2 ;;
        --unity)        UNITY_PLUGINS_DIR="$2"; shift 2 ;;
        --skip)         SKIP_PLATFORMS+=("$2"); shift 2 ;;
        --only)         ONLY_PLATFORMS+=("$2"); shift 2 ;;
        -h|--help)
            sed -n '3,/^# ===/p' "$0" | sed 's/^# \?//'; exit 0 ;;
        *) error "Unknown option: $1 (use --help)" ;;
    esac
done

# ── Platform filter helpers ───────────────────────────────────────────────────
should_build() {
    local p="$1"
    # --only 白名单优先
    if [[ ${#ONLY_PLATFORMS[@]} -gt 0 ]]; then
        for o in "${ONLY_PLATFORMS[@]}"; do
            [[ "$o" == "$p" ]] && return 0
        done
        return 1
    fi
    # --skip 黑名单
    for s in "${SKIP_PLATFORMS[@]}"; do
        [[ "$s" == "$p" ]] && return 1
    done
    return 0
}

# ── 构建状态追踪 ──────────────────────────────────────────────────────────────
declare -A BUILD_STATUS   # platform → "OK" | "SKIP" | "FAIL" | "NA"

run_build() {
    local platform="$1"; shift
    local extra_args=("$@")

    if ! should_build "$platform"; then
        BUILD_STATUS["$platform"]="SKIP"
        warn "Skipping platform: ${platform}"
        return
    fi

    step "Building: ${platform} (${BUILD_TYPE})"
    local cmd=("${BUILD_SH}" "$platform" "$BUILD_TYPE" "${extra_args[@]}")
    [[ -n "$CLEAN_FLAG" ]] && cmd+=("$CLEAN_FLAG")

    if "${cmd[@]}"; then
        BUILD_STATUS["$platform"]="OK"
        success "Platform ${platform} built successfully."
    else
        BUILD_STATUS["$platform"]="FAIL"
        warn "Platform ${platform} build FAILED (continuing with other platforms)"
    fi
}

# ── 产物收集 ──────────────────────────────────────────────────────────────────
collect() {
    local platform="$1"
    local src="$2"
    local dst_dir="$3"
    local dst_name="$4"

    [[ "${BUILD_STATUS[$platform]:-NA}" != "OK" ]] && return

    if [[ ! -f "$src" ]]; then
        warn "Expected artifact not found: ${src}"
        BUILD_STATUS["${platform}"]="FAIL"
        return
    fi

    mkdir -p "$dst_dir"
    cp -f "$src" "${dst_dir}/${dst_name}"
    success "Collected [${platform}]: ${dst_dir}/${dst_name}"
}

# ── 收集所有平台产物到 Unity Plugins ──────────────────────────────────────────
collect_all() {
    info "Collecting artifacts to: ${UNITY_PLUGINS_DIR}"

    # Windows
    collect "windows" \
        "${PROJECT_DIR}/build-windows/bin/${BUILD_TYPE}/BlueprintRuntime.dll" \
        "${UNITY_PLUGINS_DIR}/Windows/x86_64" \
        "BlueprintRuntime.dll"

    # Android
    collect "android" \
        "${PROJECT_DIR}/build-android/Runtime/libBlueprintRuntime.so" \
        "${UNITY_PLUGINS_DIR}/Android/arm64-v8a" \
        "libBlueprintRuntime.so"

    # iOS
    collect "ios" \
        "${PROJECT_DIR}/build-ios/Runtime/${BUILD_TYPE}/libBlueprintRuntime.a" \
        "${UNITY_PLUGINS_DIR}/iOS" \
        "libBlueprintRuntime.a"

    # WebGL
    collect "wasm" \
        "${PROJECT_DIR}/build-wasm/Runtime/libBlueprintRuntime.a" \
        "${UNITY_PLUGINS_DIR}/WebGL" \
        "libBlueprintRuntime.a"

    # macOS
    if [[ "$HOST_OS" == "Darwin" ]]; then
        # CMake 可能输出 .bundle 或 .dylib，都尝试
        local macos_src=""
        for ext in bundle dylib so; do
            local candidate="${PROJECT_DIR}/build-macos/bin/libBlueprintRuntime.${ext}"
            [[ -f "$candidate" ]] && { macos_src="$candidate"; break; }
        done
        if [[ -n "$macos_src" ]]; then
            local fname="$(basename "$macos_src")"
            mkdir -p "${UNITY_PLUGINS_DIR}/macOS"
            cp -f "$macos_src" "${UNITY_PLUGINS_DIR}/macOS/${fname}"
            success "Collected [macos]: ${UNITY_PLUGINS_DIR}/macOS/${fname}"
        else
            warn "macOS artifact not found under build-macos/bin/"
        fi
    fi

    # Linux
    if [[ "$HOST_OS" == "Linux" ]]; then
        collect "linux" \
            "${PROJECT_DIR}/build-linux/bin/libBlueprintRuntime.so" \
            "${UNITY_PLUGINS_DIR}/Linux/x86_64" \
            "libBlueprintRuntime.so"
    fi
}

# ── 主流程 ────────────────────────────────────────────────────────────────────
echo ""
echo -e "${BOLD}============================================${NC}"
echo -e "${BOLD}  Blueprint Editor – Build All Platforms${NC}"
echo -e "  Host OS       : ${CYAN}${HOST_OS}${NC}"
echo -e "  Configuration : ${CYAN}${BUILD_TYPE}${NC}"
echo -e "  Unity Plugins : ${CYAN}${UNITY_PLUGINS_DIR}${NC}"
[[ ${#SKIP_PLATFORMS[@]} -gt 0 ]] && \
    echo -e "  Skip          : ${YELLOW}${SKIP_PLATFORMS[*]}${NC}"
[[ ${#ONLY_PLATFORMS[@]} -gt 0 ]] && \
    echo -e "  Only          : ${CYAN}${ONLY_PLATFORMS[*]}${NC}"
echo -e "${BOLD}============================================${NC}"

# ── 1. 宿主平台（完整编辑器 + 共享运行时） ───────────────────────────────────
case "$HOST_OS" in
    Linux)
        run_build "linux" "shared"
        ;;
    Darwin)
        run_build "macos" "shared"
        ;;
    *)
        warn "Unknown host OS '${HOST_OS}', skipping host platform build"
        BUILD_STATUS["host"]="NA"
        ;;
esac

# ── 2. WebAssembly ────────────────────────────────────────────────────────────
WASM_ARGS=()
[[ -n "$EMSDK_PATH" ]] && WASM_ARGS+=("--emsdk" "$EMSDK_PATH")
if command -v emcmake &>/dev/null || [[ -n "$EMSDK_PATH" ]]; then
    run_build "wasm" "${WASM_ARGS[@]}"
else
    BUILD_STATUS["wasm"]="NA"
    warn "emcmake not found – skipping WASM build. Pass --emsdk <path> to enable."
fi

# ── 3. Android ────────────────────────────────────────────────────────────────
ANDROID_ARGS=()
if [[ -n "$NDK_PATH" ]]; then
    ANDROID_ARGS+=("--ndk" "$NDK_PATH" "--api" "$ANDROID_API")
    run_build "android" "${ANDROID_ARGS[@]}"
else
    # 尝试从环境变量找 NDK
    for _ndk in "${ANDROID_NDK:-}" "${ANDROID_NDK_HOME:-}" \
                "${HOME}/Library/Android/sdk/ndk-bundle" \
                "${HOME}/Android/Sdk/ndk-bundle"; do
        if [[ -n "$_ndk" && -d "$_ndk" ]]; then
            ANDROID_ARGS+=("--ndk" "$_ndk" "--api" "$ANDROID_API")
            run_build "android" "${ANDROID_ARGS[@]}"
            break
        fi
    done
    if [[ ${#ANDROID_ARGS[@]} -eq 0 ]]; then
        BUILD_STATUS["android"]="NA"
        warn "Android NDK not found – skipping Android build. Pass --ndk <path> to enable."
    fi
fi

# ── 4. iOS（仅 macOS） ────────────────────────────────────────────────────────
if [[ "$HOST_OS" == "Darwin" ]]; then
    run_build "ios"
else
    BUILD_STATUS["ios"]="NA"
    warn "iOS build requires macOS host – skipping."
fi

# ── 5. Windows（通过 MinGW-w64 交叉编译，Linux/macOS 宿主） ──────────────────
if command -v x86_64-w64-mingw32-gcc &>/dev/null; then
    run_build "windows-dll"
    # windows-dll 对应 build-windows-dll 目录，帮 collect 映射一下
    BUILD_STATUS["windows"]="${BUILD_STATUS[windows-dll]:-NA}"
else
    BUILD_STATUS["windows"]="NA"
    warn "MinGW-w64 not found – skipping Windows cross-build."
    info "  Install: apt install mingw-w64  /  brew install mingw-w64"
    info "  Or run build-all.bat on Windows for native MSVC build."
fi

# ── 收集产物 ──────────────────────────────────────────────────────────────────
collect_all

# ── 汇总报告 ──────────────────────────────────────────────────────────────────
echo ""
echo -e "${BOLD}============================================${NC}"
echo -e "${BOLD}  Build All – Summary${NC}"
echo -e "${BOLD}============================================${NC}"

ALL_OK=1
for p in linux macos wasm android ios windows; do
    status="${BUILD_STATUS[$p]:-NA}"
    case "$status" in
        OK)   echo -e "  ${GREEN}✓${NC} ${p}" ;;
        SKIP) echo -e "  ${YELLOW}–${NC} ${p} (skipped)" ;;
        NA)   echo -e "  ${YELLOW}–${NC} ${p} (not available on this host)" ;;
        FAIL) echo -e "  ${RED}✗${NC} ${p} (FAILED)"; ALL_OK=0 ;;
    esac
done

echo ""
echo -e "  Unity Plugins : ${CYAN}${UNITY_PLUGINS_DIR}${NC}"
echo -e "${BOLD}============================================${NC}"

if [[ $ALL_OK -eq 1 ]]; then
    echo -e "${GREEN}${BOLD}  All available platforms built successfully!${NC}"
else
    echo -e "${YELLOW}${BOLD}  Some platforms failed. Check output above.${NC}"
    exit 1
fi
