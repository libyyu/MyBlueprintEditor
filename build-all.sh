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
#   host     当前宿主（Linux→linux, macOS→macos）完整编辑器 + 运行时
#   wasm     WebAssembly（需要 Emscripten）
#   android  Android ARM64-v8a（需要 NDK）
#   ios      iOS arm64（仅 macOS 宿主）
#   windows  Windows x64（需要 MinGW-w64 交叉编译器）
#
# 产物收集到 Unity/Runtime/Plugins/:
#   Windows/x86_64/
#     BlueprintRuntime.dll          ← 运行时主体
#     liblua54.dll                  ← Lua VM（动态链接）
#   Android/arm64-v8a/
#     libBlueprintRuntime.so
#     liblua54.so
#   iOS/
#     libBlueprintBundle.a          ← Runtime + crude_json + lua54 合并包
#   WebGL/
#     libBlueprintBundle.a          ← 同上
#   macOS/
#     libBlueprintRuntime.bundle/.dylib
#     liblua54.dylib/.so
#   Linux/x86_64/
#     libBlueprintRuntime.so
#     liblua54.so
#
# 注: iOS/WebGL 使用 BlueprintBundle（Runtime+JSON+Lua 全合并静态库），
#     其余动态链接平台单独收集各库。
#
# 示例:
#   ./build-all.sh
#   ./build-all.sh debug
#   ./build-all.sh --skip android --skip ios
#   ./build-all.sh --only wasm --only windows
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
    if [[ ${#ONLY_PLATFORMS[@]} -gt 0 ]]; then
        for o in "${ONLY_PLATFORMS[@]}"; do
            [[ "$o" == "$p" ]] && return 0
        done
        return 1
    fi
    for s in "${SKIP_PLATFORMS[@]}"; do
        [[ "$s" == "$p" ]] && return 1
    done
    return 0
}

# ── 构建状态追踪 ──────────────────────────────────────────────────────────────
declare -A BUILD_STATUS

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

# ── 单文件收集辅助 ────────────────────────────────────────────────────────────
# collect <platform> <src> <dst_dir> <dst_name>
collect() {
    local platform="$1" src="$2" dst_dir="$3" dst_name="$4"
    [[ "${BUILD_STATUS[$platform]:-NA}" != "OK" ]] && return 0

    if [[ ! -f "$src" ]]; then
        warn "Artifact not found: ${src}"
        return 0   # 不把缺产物升级为失败（可选产物场景）
    fi

    mkdir -p "$dst_dir"
    cp -f "$src" "${dst_dir}/${dst_name}"
    success "  [${platform}] → ${dst_dir}/${dst_name}"
}

# collect_required：找不到文件则标记 FAIL
collect_required() {
    local platform="$1" src="$2" dst_dir="$3" dst_name="$4"
    [[ "${BUILD_STATUS[$platform]:-NA}" != "OK" ]] && return 0

    if [[ ! -f "$src" ]]; then
        warn "Required artifact not found: ${src}"
        BUILD_STATUS["$platform"]="FAIL"
        return 0
    fi

    mkdir -p "$dst_dir"
    cp -f "$src" "${dst_dir}/${dst_name}"
    success "  [${platform}] → ${dst_dir}/${dst_name}"
}

# ── 按平台收集所有产物 ────────────────────────────────────────────────────────
# 约定路径：
#   动态库（.so/.dylib/.dll）→ build-<platform>/bin/[Release|Debug]/
#   静态库（.a）             → build-<platform>/Runtime/
#   BlueprintBundle          → build-<platform>/bin/[Release|Debug]/libBlueprintBundle.a
collect_all() {
    info "Collecting artifacts → ${UNITY_PLUGINS_DIR}"
    local BT="${BUILD_TYPE}"   # Release 或 Debug

    # ── Windows ──────────────────────────────────────────────────────────────
    local WIN_BIN="${PROJECT_DIR}/build-windows-dll/bin/${BT}"
    collect_required "windows" \
        "${WIN_BIN}/BlueprintRuntime.dll" \
        "${UNITY_PLUGINS_DIR}/Windows/x86_64" "BlueprintRuntime.dll"
    # liblua54.dll（动态链接时存在，BUNDLE_LUA=ON 时不存在）
    collect "windows" \
        "${WIN_BIN}/liblua54.dll" \
        "${UNITY_PLUGINS_DIR}/Windows/x86_64" "liblua54.dll"

    # ── Android ──────────────────────────────────────────────────────────────
    local AND_BIN="${PROJECT_DIR}/build-android"
    collect_required "android" \
        "${AND_BIN}/bin/${BT}/libBlueprintRuntime.so" \
        "${UNITY_PLUGINS_DIR}/Android/arm64-v8a" "libBlueprintRuntime.so"
    collect "android" \
        "${AND_BIN}/bin/${BT}/liblua54.so" \
        "${UNITY_PLUGINS_DIR}/Android/arm64-v8a" "liblua54.so"

    # ── iOS：BlueprintBundle（含 Lua）+ BlueprintBundleNoLua ─────────────────
    local IOS_BIN="${PROJECT_DIR}/build-ios/bin/${BT}"
    for bundle_name in "libBlueprintBundle.a" "libBlueprintBundleNoLua.a"; do
        local IOS_BUNDLE=""
        for candidate in \
            "${IOS_BIN}/${bundle_name}" \
            "${PROJECT_DIR}/build-ios/bin/${BT}-iphoneos/${bundle_name}" \
            "${PROJECT_DIR}/build-ios/Runtime/${BT}/${bundle_name}" \
            "${PROJECT_DIR}/build-ios/Runtime/${bundle_name}"; do
            [[ -f "$candidate" ]] && { IOS_BUNDLE="$candidate"; break; }
        done
        if [[ -n "$IOS_BUNDLE" ]]; then
            collect "ios" "$IOS_BUNDLE" "${UNITY_PLUGINS_DIR}/iOS" "${bundle_name}"
        fi
    done
    # 若两个 Bundle 都没有，回退到裸 Runtime
    if [[ ! -f "${UNITY_PLUGINS_DIR}/iOS/libBlueprintBundle.a" && \
          ! -f "${UNITY_PLUGINS_DIR}/iOS/libBlueprintBundleNoLua.a" ]]; then
        collect_required "ios" \
            "${IOS_BIN}/libBlueprintRuntime.a" \
            "${UNITY_PLUGINS_DIR}/iOS" "libBlueprintRuntime.a"
    fi

    # ── WebGL：BlueprintBundle（含 Lua）+ BlueprintBundleNoLua ───────────────
    local WASM_BIN="${PROJECT_DIR}/build-wasm/bin/${BT}"
    for bundle_name in "libBlueprintBundle.a" "libBlueprintBundleNoLua.a"; do
        local WASM_BUNDLE=""
        for candidate in \
            "${WASM_BIN}/${bundle_name}" \
            "${PROJECT_DIR}/build-wasm/Runtime/${bundle_name}" \
            "${PROJECT_DIR}/build-wasm/bin/${bundle_name}"; do
            [[ -f "$candidate" ]] && { WASM_BUNDLE="$candidate"; break; }
        done
        if [[ -n "$WASM_BUNDLE" ]]; then
            collect "wasm" "$WASM_BUNDLE" "${UNITY_PLUGINS_DIR}/WebGL" "${bundle_name}"
        fi
    done
    # 若两个 Bundle 都没有，回退到裸 Runtime
    if [[ ! -f "${UNITY_PLUGINS_DIR}/WebGL/libBlueprintBundle.a" && \
          ! -f "${UNITY_PLUGINS_DIR}/WebGL/libBlueprintBundleNoLua.a" ]]; then
        collect_required "wasm" \
            "${PROJECT_DIR}/build-wasm/Runtime/libBlueprintRuntime.a" \
            "${UNITY_PLUGINS_DIR}/WebGL" "libBlueprintRuntime.a"
    fi

    # ── macOS（仅 macOS 宿主） ────────────────────────────────────────────────
    if [[ "$HOST_OS" == "Darwin" ]]; then
        local MAC_BIN="${PROJECT_DIR}/build-macos/bin"
        local mac_rt=""
        for ext in bundle dylib so; do
            local c="${MAC_BIN}/libBlueprintRuntime.${ext}"
            [[ -f "$c" ]] && { mac_rt="$c"; break; }
        done
        if [[ -n "$mac_rt" ]]; then
            mkdir -p "${UNITY_PLUGINS_DIR}/macOS"
            cp -f "$mac_rt" "${UNITY_PLUGINS_DIR}/macOS/$(basename "$mac_rt")"
            success "  [macos] → ${UNITY_PLUGINS_DIR}/macOS/$(basename "$mac_rt")"
        else
            warn "macOS BlueprintRuntime artifact not found under build-macos/bin/"
        fi
        # Lua dylib
        for ext in dylib so; do
            local c="${MAC_BIN}/liblua54.${ext}"
            if [[ -f "$c" ]]; then
                cp -f "$c" "${UNITY_PLUGINS_DIR}/macOS/$(basename "$c")"
                success "  [macos] → ${UNITY_PLUGINS_DIR}/macOS/$(basename "$c")"
                break
            fi
        done
    fi

    # ── Linux（仅 Linux 宿主） ────────────────────────────────────────────────
    if [[ "$HOST_OS" == "Linux" ]]; then
        local LIN_BIN="${PROJECT_DIR}/build-linux/bin"
        collect_required "linux" \
            "${LIN_BIN}/libBlueprintRuntime.so" \
            "${UNITY_PLUGINS_DIR}/Linux/x86_64" "libBlueprintRuntime.so"
        collect "linux" \
            "${LIN_BIN}/liblua54.so" \
            "${UNITY_PLUGINS_DIR}/Linux/x86_64" "liblua54.so"
    fi
}

# ── Banner ────────────────────────────────────────────────────────────────────
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

# ── 通用额外 CMake 参数（通过 build.sh 的尾部 -- 转发不方便，改用 env 注入） ─
# build.sh 目前不支持转发任意 CMake 参数，这里用 CMAKE_EXTRA_FLAGS env 注入
# （需要 build.sh 支持）；或者直接在 build dir 里 re-configure。
# 当前策略：先用 build.sh 正常构建，然后对静态平台在同一 build 目录追加
# -DBUILD_BUNDLE=ON -DBUNDLE_LUA=ON 重新 configure 并构建 BlueprintBundle target。

build_bundle_target() {
    # build_bundle_target <build_dir> <build_type> <bundle_lua ON|OFF>
    local bdir="$1" btype="$2" bundle_lua="${3:-OFF}"
    [[ -d "$bdir" ]] || return 0
    local suffix=""
    [[ "$bundle_lua" == "ON" ]] && suffix=" (with Lua)" || suffix=" (no Lua)"
    info "  Configuring BlueprintBundle${suffix} in ${bdir} ..."
    cmake -S "${PROJECT_DIR}" -B "${bdir}" \
        -DBUILD_BUNDLE=ON \
        -DBUNDLE_LUA="${bundle_lua}" \
        -DCMAKE_BUILD_TYPE="${btype}" > /dev/null
    cmake --build "${bdir}" --target BlueprintBundle \
        --config "${btype}" --parallel \
        "$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)"
}

# ── 1. 宿主平台 ───────────────────────────────────────────────────────────────
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
    if [[ "${BUILD_STATUS[wasm]:-}" == "OK" ]]; then
        step "Building BlueprintBundle (wasm) – with Lua + without Lua"
        build_bundle_target "${PROJECT_DIR}/build-wasm" "${BUILD_TYPE}" "ON" \
            && success "BlueprintBundle+Lua (wasm) OK" \
            || warn "BlueprintBundle+Lua (wasm) failed"
        build_bundle_target "${PROJECT_DIR}/build-wasm" "${BUILD_TYPE}" "OFF" \
            && success "BlueprintBundleNoLua (wasm) OK" \
            || warn "BlueprintBundleNoLua (wasm) failed"
    fi
else
    BUILD_STATUS["wasm"]="NA"
    warn "emcmake not found – skipping WASM. Pass --emsdk <path> to enable."
fi

# ── 3. Android ────────────────────────────────────────────────────────────────
ANDROID_ARGS=()
_ndk_found=0
if [[ -n "$NDK_PATH" ]]; then
    _ndk_found=1
else
    for _ndk in "${ANDROID_NDK:-}" "${ANDROID_NDK_HOME:-}" \
                "${HOME}/Library/Android/sdk/ndk-bundle" \
                "${HOME}/Android/Sdk/ndk-bundle"; do
        if [[ -n "$_ndk" && -d "$_ndk" ]]; then
            NDK_PATH="$_ndk"; _ndk_found=1; break
        fi
    done
fi

if [[ $_ndk_found -eq 1 ]]; then
    ANDROID_ARGS+=("--ndk" "$NDK_PATH" "--api" "$ANDROID_API")
    run_build "android" "${ANDROID_ARGS[@]}"
else
    BUILD_STATUS["android"]="NA"
    warn "Android NDK not found – skipping. Pass --ndk <path> to enable."
fi

# ── 4. iOS（仅 macOS） ────────────────────────────────────────────────────────
if [[ "$HOST_OS" == "Darwin" ]]; then
    run_build "ios"
    if [[ "${BUILD_STATUS[ios]:-}" == "OK" ]]; then
        step "Building BlueprintBundle (ios) – with Lua + without Lua"
        build_bundle_target "${PROJECT_DIR}/build-ios" "${BUILD_TYPE}" "ON" \
            && success "BlueprintBundle+Lua (ios) OK" \
            || warn "BlueprintBundle+Lua (ios) failed"
        build_bundle_target "${PROJECT_DIR}/build-ios" "${BUILD_TYPE}" "OFF" \
            && success "BlueprintBundleNoLua (ios) OK" \
            || warn "BlueprintBundleNoLua (ios) failed"
    fi
else
    BUILD_STATUS["ios"]="NA"
    warn "iOS build requires macOS host – skipping."
fi

# ── 5. Windows（MinGW-w64 交叉编译） ─────────────────────────────────────────
if command -v x86_64-w64-mingw32-gcc &>/dev/null; then
    run_build "windows-dll"
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
echo ""
echo -e "  收集的产物（动态平台）:"
echo -e "    Windows/x86_64/  BlueprintRuntime.dll  liblua54.dll"
echo -e "    Android/arm64/   libBlueprintRuntime.so  liblua54.so"
echo -e "    Linux/x86_64/    libBlueprintRuntime.so  liblua54.so"
echo -e "    macOS/           libBlueprintRuntime.bundle  liblua54.dylib"
echo -e "  收集的产物（静态平台，全合并包）:"
echo -e "    iOS/             libBlueprintBundle.a      (Runtime+JSON+Lua)"
echo -e "    iOS/             libBlueprintBundleNoLua.a (Runtime+JSON, 无Lua)"
echo -e "    WebGL/           libBlueprintBundle.a      (Runtime+JSON+Lua)"
echo -e "    WebGL/           libBlueprintBundleNoLua.a (Runtime+JSON, 无Lua)"
echo -e "${BOLD}============================================${NC}"

if [[ $ALL_OK -eq 1 ]]; then
    echo -e "${GREEN}${BOLD}  All available platforms built successfully!${NC}"
else
    echo -e "${YELLOW}${BOLD}  Some platforms failed. Check output above.${NC}"
    exit 1
fi
