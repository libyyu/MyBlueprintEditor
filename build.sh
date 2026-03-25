#!/usr/bin/env bash
# ==============================================================================
# build.sh – MyBlueprintEditor cross-platform build script
#
# Usage:
#   ./build.sh [platform] [options...]
#
# Platforms:
#   linux         Linux x64 (GLFW + OpenGL3) – default when no platform given
#   macos         macOS (GLFW + OpenGL3)
#   wasm          WebAssembly via Emscripten (Runtime only, static)
#   android       Android ARM64-v8a (Runtime only, shared/static)
#   ios           iOS arm64 (Runtime only, static)
#   runtime       Current host – Runtime-only build (no Editor)
#
# Options:
#   debug         Build Debug configuration (default: Release)
#   release       Build Release configuration
#   clean         Wipe build directory before building
#   noexamples    Skip building examples
#   shared        Build BlueprintRuntime as a shared library
#   --ndk <path>  Path to Android NDK (required for android platform)
#   --api <N>     Android min API level (default: 21)
#   --emsdk <path> Path to Emscripten SDK root (default: $EMSDK env var)
#
# Examples:
#   ./build.sh                         # Linux Release
#   ./build.sh macos debug             # macOS Debug
#   ./build.sh wasm                    # WASM Release (needs Emscripten)
#   ./build.sh android --ndk ~/ndk     # Android Release
#   ./build.sh ios                     # iOS Release (macOS host required)
#   ./build.sh runtime shared          # Runtime-only shared lib, host platform
# ==============================================================================

set -euo pipefail

# ── Colours ───────────────────────────────────────────────────────────────────
RED='\033[0;31m'; GREEN='\033[0;32m'; YELLOW='\033[1;33m'
CYAN='\033[0;36m'; BOLD='\033[1m'; NC='\033[0m'

info()    { echo -e "${CYAN}[INFO]${NC}  $*"; }
success() { echo -e "${GREEN}[OK]${NC}    $*"; }
warn()    { echo -e "${YELLOW}[WARN]${NC}  $*"; }
error()   { echo -e "${RED}[ERROR]${NC} $*" >&2; exit 1; }

# ── Defaults ──────────────────────────────────────────────────────────────────
PROJECT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PLATFORM=""
BUILD_TYPE="Release"
CLEAN_BUILD=0
BUILD_EXAMPLES="ON"
BUILD_SHARED="OFF"
RUNTIME_ONLY=0
NDK_PATH=""
ANDROID_API=21
EMSDK_PATH="${EMSDK:-}"

# ── Argument parsing ──────────────────────────────────────────────────────────
while [[ $# -gt 0 ]]; do
    case "$1" in
        linux|macos|wasm|android|ios|runtime)
            PLATFORM="$1"; shift ;;
        debug)
            BUILD_TYPE="Debug"; shift ;;
        release)
            BUILD_TYPE="Release"; shift ;;
        clean)
            CLEAN_BUILD=1; shift ;;
        noexamples)
            BUILD_EXAMPLES="OFF"; shift ;;
        shared)
            BUILD_SHARED="ON"; shift ;;
        --ndk)
            NDK_PATH="$2"; shift 2 ;;
        --api)
            ANDROID_API="$2"; shift 2 ;;
        --emsdk)
            EMSDK_PATH="$2"; shift 2 ;;
        -h|--help)
            sed -n '2,/^# ===/p' "$0" | sed 's/^# \?//'
            exit 0 ;;
        *)
            error "Unknown option: $1  (run with --help for usage)" ;;
    esac
done

# ── Detect host platform if none given ────────────────────────────────────────
if [[ -z "$PLATFORM" ]]; then
    case "$(uname -s)" in
        Linux)   PLATFORM="linux" ;;
        Darwin)  PLATFORM="macos" ;;
        *)       PLATFORM="runtime" ;;
    esac
    info "No platform specified – detected: ${BOLD}${PLATFORM}${NC}"
fi

# ── Derive build directory & CMake flags ─────────────────────────────────────
BUILD_DIR="${PROJECT_DIR}/build-${PLATFORM}"
CMAKE_EXTRA_ARGS=()

case "$PLATFORM" in
    linux|macos)
        CMAKE_EXTRA_ARGS+=(
            "-DCMAKE_BUILD_TYPE=${BUILD_TYPE}"
            "-DBUILD_EXAMPLES=${BUILD_EXAMPLES}"
            "-DBUILD_SHARED_LIBS=${BUILD_SHARED}"
        )
        ;;

    runtime)
        RUNTIME_ONLY=1
        CMAKE_EXTRA_ARGS+=(
            "-DCMAKE_BUILD_TYPE=${BUILD_TYPE}"
            "-DBUILD_RUNTIME_ONLY=ON"
            "-DBUILD_SHARED_LIBS=${BUILD_SHARED}"
            "-DBUILD_EXAMPLES=${BUILD_EXAMPLES}"
        )
        ;;

    wasm)
        RUNTIME_ONLY=1
        # Locate emcmake
        if [[ -z "$EMSDK_PATH" ]]; then
            if command -v emcmake &>/dev/null; then
                EMSDK_PATH="$(dirname "$(command -v emcmake)")"
            else
                error "Emscripten not found. Install the Emscripten SDK or pass --emsdk <path>.\n  See: https://emscripten.org/docs/getting_started/downloads.html"
            fi
        fi
        if [[ ! -f "${EMSDK_PATH}/emcmake" && ! -f "${EMSDK_PATH}/upstream/emscripten/emcmake" ]]; then
            # Try sourcing emsdk_env.sh
            if [[ -f "${EMSDK_PATH}/emsdk_env.sh" ]]; then
                # shellcheck source=/dev/null
                source "${EMSDK_PATH}/emsdk_env.sh" >/dev/null 2>&1
            fi
        fi
        EMCMAKE="$(command -v emcmake 2>/dev/null)" || error "emcmake not found after sourcing EMSDK. Check your Emscripten installation."
        CMAKE_EXTRA_ARGS+=(
            "-DBUILD_RUNTIME_ONLY=ON"
            "-DCMAKE_BUILD_TYPE=${BUILD_TYPE}"
        )
        ;;

    android)
        RUNTIME_ONLY=1
        if [[ -z "$NDK_PATH" ]]; then
            # Try common locations
            for candidate in \
                "${ANDROID_NDK:-}" \
                "${ANDROID_NDK_HOME:-}" \
                "${HOME}/Library/Android/sdk/ndk-bundle" \
                "${HOME}/Android/Sdk/ndk-bundle" \
                "/opt/android-ndk"; do
                if [[ -n "$candidate" && -d "$candidate" ]]; then
                    NDK_PATH="$candidate"; break
                fi
            done
        fi
        [[ -n "$NDK_PATH" ]] || error "Android NDK not found. Pass --ndk <path> or set \$ANDROID_NDK."
        CMAKE_EXTRA_ARGS+=(
            "-DCMAKE_TOOLCHAIN_FILE=${NDK_PATH}/build/cmake/android.toolchain.cmake"
            "-DANDROID_ABI=arm64-v8a"
            "-DANDROID_PLATFORM=android-${ANDROID_API}"
            "-DBUILD_RUNTIME_ONLY=ON"
            "-DBUILD_SHARED_LIBS=${BUILD_SHARED}"
            "-DBUILD_EXAMPLES=OFF"
            "-DCMAKE_BUILD_TYPE=${BUILD_TYPE}"
        )
        ;;

    ios)
        RUNTIME_ONLY=1
        [[ "$(uname -s)" == "Darwin" ]] || error "iOS builds must be run on macOS."
        # Try to find iOS toolchain (use the one bundled in cmake/ios or a well-known one)
        IOS_TOOLCHAIN="${PROJECT_DIR}/cmake/ios.toolchain.cmake"
        if [[ ! -f "$IOS_TOOLCHAIN" ]]; then
            # Fallback: use Xcode's built-in CMake support
            IOS_TOOLCHAIN=""
        fi
        if [[ -n "$IOS_TOOLCHAIN" ]]; then
            CMAKE_EXTRA_ARGS+=("-DCMAKE_TOOLCHAIN_FILE=${IOS_TOOLCHAIN}")
        fi
        CMAKE_EXTRA_ARGS+=(
            "-DCMAKE_SYSTEM_NAME=iOS"
            "-DCMAKE_OSX_ARCHITECTURES=arm64"
            "-DCMAKE_OSX_DEPLOYMENT_TARGET=13.0"
            "-DBUILD_RUNTIME_ONLY=ON"
            "-DBUILD_SHARED_LIBS=OFF"
            "-DBUILD_EXAMPLES=OFF"
            "-DCMAKE_BUILD_TYPE=${BUILD_TYPE}"
            "-G" "Xcode"
        )
        ;;

    *)
        error "Unknown platform: ${PLATFORM}" ;;
esac

# ── Banner ────────────────────────────────────────────────────────────────────
echo ""
echo -e "${BOLD}============================================${NC}"
echo -e "${BOLD}  Blueprint Editor – Build Script${NC}"
echo -e "  Platform      : ${CYAN}${PLATFORM}${NC}"
echo -e "  Configuration : ${CYAN}${BUILD_TYPE}${NC}"
echo -e "  Runtime only  : ${CYAN}${RUNTIME_ONLY}${NC}"
echo -e "  Shared libs   : ${CYAN}${BUILD_SHARED}${NC}"
echo -e "  Examples      : ${CYAN}${BUILD_EXAMPLES}${NC}"
echo -e "  Build dir     : ${CYAN}${BUILD_DIR}${NC}"
echo -e "${BOLD}============================================${NC}"
echo ""

# ── Step 1 – Clean ────────────────────────────────────────────────────────────
if [[ $CLEAN_BUILD -eq 1 ]]; then
    info "[1/3] Cleaning build directory..."
    rm -rf "${BUILD_DIR}"
    success "Cleaned."
else
    info "[1/3] Skipping clean (pass 'clean' to force)"
fi

mkdir -p "${BUILD_DIR}"

# ── Step 2 – CMake configure ─────────────────────────────────────────────────
info "[2/3] Running CMake configure..."
if [[ "$PLATFORM" == "wasm" ]]; then
    "${EMCMAKE}" cmake -S "${PROJECT_DIR}" -B "${BUILD_DIR}" "${CMAKE_EXTRA_ARGS[@]}"
else
    cmake -S "${PROJECT_DIR}" -B "${BUILD_DIR}" "${CMAKE_EXTRA_ARGS[@]}"
fi
success "CMake configure complete."

# ── Step 3 – Build ───────────────────────────────────────────────────────────
info "[3/3] Building..."
PARALLEL_JOBS="${PARALLEL_JOBS:-$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)}"

if [[ "$PLATFORM" == "ios" ]]; then
    # Xcode generator requires --config
    cmake --build "${BUILD_DIR}" --config "${BUILD_TYPE}" -- -jobs "${PARALLEL_JOBS}"
else
    cmake --build "${BUILD_DIR}" --config "${BUILD_TYPE}" --parallel "${PARALLEL_JOBS}"
fi

# ── Done ──────────────────────────────────────────────────────────────────────
echo ""
echo -e "${BOLD}============================================${NC}"
echo -e "${GREEN}${BOLD}  Build succeeded!${NC} (${PLATFORM} / ${BUILD_TYPE})"
echo -e "  Output: ${CYAN}${BUILD_DIR}/bin${NC}"
if [[ "$PLATFORM" == "wasm" ]]; then
    echo -e "  WASM lib: ${CYAN}${BUILD_DIR}/Runtime/libBlueprintRuntime.a${NC}"
    echo -e "  → Drop into Assets/Plugins/WebGL/ for Unity"
elif [[ "$PLATFORM" == "android" ]]; then
    echo -e "  Android lib: ${CYAN}${BUILD_DIR}/Runtime/libBlueprintRuntime.so (or .a)${NC}"
elif [[ "$PLATFORM" == "ios" ]]; then
    echo -e "  iOS lib: ${CYAN}${BUILD_DIR}/Runtime/libBlueprintRuntime.a${NC}"
fi
echo -e "${BOLD}============================================${NC}"
