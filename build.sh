#!/bin/sh
# ==============================================================================
# build.sh – MyBlueprintEditor cross-platform build script
#
# Usage:
#   sh build.sh [platform] [options...]
#
# Platforms:
#   linux         Linux x64 (GLFW + OpenGL3) – default on Linux
#   macos         macOS (GLFW + OpenGL3)     – default on macOS
#   windows       Windows x64 (native MSVC or direct cmake)
#   windows-dll   Windows x64 via MinGW-w64: full editor + shared DLL
#   dll           Windows x64 via MinGW-w64: Runtime DLL only (no Editor)
#   wasm          WebAssembly via Emscripten (Runtime only, static)
#   android       Android ARM64-v8a (Runtime only)
#   ios           iOS arm64 (Runtime only, static; macOS host required)
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
#   sh build.sh                         # Linux Release
#   sh build.sh macos debug             # macOS Debug
#   sh build.sh windows                 # Windows native (MSVC/cmake)
#   sh build.sh windows-dll             # Windows DLL via MinGW-w64
#   sh build.sh wasm                    # WASM Release (needs Emscripten)
#   sh build.sh android --ndk ~/ndk     # Android Release
#   sh build.sh ios                     # iOS Release (macOS host required)
#   sh build.sh runtime shared          # Runtime-only shared lib, host platform
# ==============================================================================

set -eu

# ── Colours ───────────────────────────────────────────────────────────────────
RED='\033[0;31m'; GREEN='\033[0;32m'; YELLOW='\033[1;33m'
CYAN='\033[0;36m'; BOLD='\033[1m'; NC='\033[0m'

info()    { printf "${CYAN}[INFO]${NC}  %s\n" "$*"; }
success() { printf "${GREEN}[OK]${NC}    %s\n" "$*"; }
warn()    { printf "${YELLOW}[WARN]${NC}  %s\n" "$*"; }
error()   { printf "${RED}[ERROR]${NC} %s\n" "$*" >&2; exit 1; }

# ── Defaults ──────────────────────────────────────────────────────────────────
HOST_OS="$(uname -s)"
PROJECT_DIR="$(cd "$(dirname "$0")" && pwd)"
PLATFORM=""
BUILD_TYPE="Release"
CLEAN_BUILD=0
BUILD_EXAMPLES="ON"
BUILD_SHARED="OFF"
RUNTIME_ONLY=0
NDK_PATH=""
ANDROID_API=21
EMSDK_PATH="${EMSDK:-}"
RUNTIME_NAME=""
BLUEPRINT_LUA="ON"
LUA_LINK_STATIC="ON"
BLUEPRINT_PROTOBUF="ON"
BUILD_MACOS_BUNDLE="ON"
BLUEPRINT_LINK_XLUA="ON"
BUILD_BUNDLE="OFF"
BUNDLE_LUA="OFF"
BUNDLE_PROTOBUF="OFF"


# ── Argument parsing ──────────────────────────────────────────────────────────
while [ $# -gt 0 ]; do
    case "$1" in
        linux|macos|wasm|android|ios|runtime|windows|windows-dll|dll)
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
        --runtime-name)
            RUNTIME_NAME="$2"; shift 2 ;;
        --nolua)
            BLUEPRINT_LUA="OFF"; shift ;;
        --lua-dynamic)
            LUA_LINK_STATIC="OFF"; shift ;;
        --no-protobuf)
            BLUEPRINT_PROTOBUF="OFF"; shift ;;
        --macos-bundle-off)
            BUILD_MACOS_BUNDLE="OFF"; shift ;;
        --merge-libs)
            BUILD_BUNDLE="ON"; shift ;;
        --merge-lua)
            BUNDLE_LUA="ON"; shift ;;
        --merge-protobuf)
            BUNDLE_PROTOBUF="ON"; shift ;;
        --no-xlua)
            BLUEPRINT_LINK_XLUA="OFF"; shift ;;
        -h|--help)
            sed -n '2,/^# ===/p' "$0" | sed 's/^# \{0,1\}//'
            exit 0 ;;
        *)
            error "Unknown option: $1  (run with --help for usage)" ;;
    esac
done

# ── Detect host platform if none given ────────────────────────────────────────
if [ -z "$PLATFORM" ]; then
    case "$(uname -s)" in
        Linux)                          PLATFORM="linux" ;;
        Darwin)                         PLATFORM="macos" ;;
        MINGW*|MSYS*|CYGWIN*|Windows*) PLATFORM="windows" ;;
        *)                              PLATFORM="runtime" ;;
    esac
    printf "No platform specified - detected: ${GREEN}${BOLD}${PLATFORM}${NC}\n"
fi

# ── Locate MinGW toolchain (for windows-dll / dll) ────────────────────────────
find_mingw_toolchain() {
    for candidate in \
        "x86_64-w64-mingw32-gcc" \
        "/usr/bin/x86_64-w64-mingw32-gcc" \
        "/usr/local/bin/x86_64-w64-mingw32-gcc"; do
        if command -v "$candidate" >/dev/null 2>&1; then
            MINGW_PREFIX="${candidate%-gcc}"
            return 0
        fi
    done
    error "MinGW-w64 cross-compiler not found. Install: apt install mingw-w64 / brew install mingw-w64"
}

is_windows_host() {
    case "$HOST_OS" in
        MINGW*|MSYS*|CYGWIN*|Windows_NT|WINDOWS*) return 0 ;;
        *) return 1 ;;
    esac
}

# ── Derive build directory & CMake flags ─────────────────────────────────────
BUILD_DIR="${PROJECT_DIR}/build-${PLATFORM}"
CMAKE_EXTRA_ARGS=""
EMCMAKE=""

case "$PLATFORM" in
    linux|macos)
        CMAKE_EXTRA_ARGS="-DCMAKE_BUILD_TYPE=${BUILD_TYPE} -DBUILD_EXAMPLES=${BUILD_EXAMPLES} -DBUILD_SHARED_LIBS=${BUILD_SHARED} -DBUILD_MACOS_BUNDLE=${BUILD_MACOS_BUNDLE} -DRUNTIME_NAME=${RUNTIME_NAME} -DBLUEPRINT_LUA=${BLUEPRINT_LUA} -DLUA_LINK_STATIC=${LUA_LINK_STATIC} -DBLUEPRINT_PROTOBUF=${BLUEPRINT_PROTOBUF} -DBUILD_BUNDLE=${BUILD_BUNDLE} -DBUNDLE_LUA=${BUNDLE_LUA} -DBUNDLE_PROTOBUF=${BUNDLE_PROTOBUF} -DBLUEPRINT_LINK_XLUA=${BLUEPRINT_LINK_XLUA}"
        ;;

    windows)
        CMAKE_EXTRA_ARGS="-DCMAKE_BUILD_TYPE=${BUILD_TYPE} -DBUILD_EXAMPLES=${BUILD_EXAMPLES} -DBUILD_SHARED_LIBS=ON -DBUILD_MACOS_BUNDLE=${BUILD_MACOS_BUNDLE} -DRUNTIME_NAME=${RUNTIME_NAME} -DBLUEPRINT_LUA=${BLUEPRINT_LUA} -DLUA_LINK_STATIC=${LUA_LINK_STATIC} -DBLUEPRINT_PROTOBUF=${BLUEPRINT_PROTOBUF} -DBUILD_BUNDLE=${BUILD_BUNDLE} -DBUNDLE_LUA=${BUNDLE_LUA} -DBUNDLE_PROTOBUF=${BUNDLE_PROTOBUF} -DBLUEPRINT_LINK_XLUA=${BLUEPRINT_LINK_XLUA}"
        BUILD_SHARED="ON"
        ;;

    runtime)
        RUNTIME_ONLY=1
        CMAKE_EXTRA_ARGS="-DCMAKE_BUILD_TYPE=${BUILD_TYPE} -DBUILD_RUNTIME_ONLY=ON -DBUILD_SHARED_LIBS=${BUILD_SHARED} -DBUILD_EXAMPLES=${BUILD_EXAMPLES} -DRUNTIME_NAME=${RUNTIME_NAME} -DBLUEPRINT_LUA=${BLUEPRINT_LUA} -DLUA_LINK_STATIC=${LUA_LINK_STATIC} -DBLUEPRINT_PROTOBUF=${BLUEPRINT_PROTOBUF} -DBUILD_BUNDLE=${BUILD_BUNDLE} -DBUNDLE_LUA=${BUNDLE_LUA} -DBUNDLE_PROTOBUF=${BUNDLE_PROTOBUF} -DBLUEPRINT_LINK_XLUA=${BLUEPRINT_LINK_XLUA}"
        ;;

    windows-dll)
        find_mingw_toolchain
        BUILD_SHARED="ON"
        CMAKE_EXTRA_ARGS="-DCMAKE_BUILD_TYPE=${BUILD_TYPE} -DCMAKE_SYSTEM_NAME=Windows -DCMAKE_C_COMPILER=${MINGW_PREFIX}-gcc -DCMAKE_CXX_COMPILER=${MINGW_PREFIX}-g++ -DCMAKE_RC_COMPILER=${MINGW_PREFIX}-windres -DBUILD_SHARED_LIBS=ON -DBUILD_EXAMPLES=${BUILD_EXAMPLES} -DRUNTIME_NAME=${RUNTIME_NAME} -DBLUEPRINT_LUA=${BLUEPRINT_LUA} -DLUA_LINK_STATIC=${LUA_LINK_STATIC} -DBLUEPRINT_PROTOBUF=${BLUEPRINT_PROTOBUF} -DBUILD_BUNDLE=${BUILD_BUNDLE} -DBUNDLE_LUA=${BUNDLE_LUA} -DBUNDLE_PROTOBUF=${BUNDLE_PROTOBUF} -DBLUEPRINT_LINK_XLUA=${BLUEPRINT_LINK_XLUA}"
        ;;

    dll)
        find_mingw_toolchain
        RUNTIME_ONLY=1
        BUILD_SHARED="ON"
        CMAKE_EXTRA_ARGS="-DCMAKE_BUILD_TYPE=${BUILD_TYPE} -DCMAKE_SYSTEM_NAME=Windows -DCMAKE_C_COMPILER=${MINGW_PREFIX}-gcc -DCMAKE_CXX_COMPILER=${MINGW_PREFIX}-g++ -DCMAKE_RC_COMPILER=${MINGW_PREFIX}-windres -DBUILD_RUNTIME_ONLY=ON -DBUILD_SHARED_LIBS=ON -DBUILD_EXAMPLES=${BUILD_EXAMPLES} -DRUNTIME_NAME=${RUNTIME_NAME} -DBLUEPRINT_LUA=${BLUEPRINT_LUA} -DLUA_LINK_STATIC=${LUA_LINK_STATIC} -DBLUEPRINT_PROTOBUF=${BLUEPRINT_PROTOBUF} -DBUILD_BUNDLE=${BUILD_BUNDLE} -DBUNDLE_LUA=${BUNDLE_LUA} -DBUNDLE_PROTOBUF=${BUNDLE_PROTOBUF} -DBLUEPRINT_LINK_XLUA=${BLUEPRINT_LINK_XLUA}"
        ;;

    wasm)
        RUNTIME_ONLY=1
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
                error "Emscripten not found. Install the Emscripten SDK or pass --emsdk <path>."
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
        LUA_LINK_STATIC="OFF"
        CMAKE_EXTRA_ARGS="-DBUILD_RUNTIME_ONLY=ON -DCMAKE_BUILD_TYPE=${BUILD_TYPE} -DRUNTIME_NAME=${RUNTIME_NAME} -DBLUEPRINT_LUA=${BLUEPRINT_LUA} -DLUA_LINK_STATIC=OFF -DBLUEPRINT_PROTOBUF=${BLUEPRINT_PROTOBUF} -DBUILD_BUNDLE=${BUILD_BUNDLE} -DBUNDLE_LUA=${BUNDLE_LUA} -DBUNDLE_PROTOBUF=${BUNDLE_PROTOBUF} -DBLUEPRINT_LINK_XLUA=${BLUEPRINT_LINK_XLUA}"
        ;;

    android)
        RUNTIME_ONLY=1
        if [ -z "$NDK_PATH" ]; then
            for candidate in \
                "${ANDROID_NDK:-}" \
                "${ANDROID_NDK_HOME:-}" \
                "${HOME}/Library/Android/sdk/ndk-bundle" \
                "${HOME}/Android/Sdk/ndk-bundle" \
                "/opt/android-ndk"; do
                if [ -n "$candidate" ] && [ -d "$candidate" ]; then
                    NDK_PATH="$candidate"; break
                fi
            done
        fi
        [ -n "$NDK_PATH" ] || error "Android NDK not found. Pass --ndk <path> or set \$ANDROID_NDK."
        CMAKE_EXTRA_ARGS="-DCMAKE_TOOLCHAIN_FILE=${NDK_PATH}/build/cmake/android.toolchain.cmake -DANDROID_ABI=arm64-v8a -DANDROID_PLATFORM=android-${ANDROID_API} -DBUILD_RUNTIME_ONLY=ON -DBUILD_SHARED_LIBS=${BUILD_SHARED} -DBUILD_EXAMPLES=OFF -DCMAKE_BUILD_TYPE=${BUILD_TYPE} -DRUNTIME_NAME=${RUNTIME_NAME} -DBLUEPRINT_LUA=${BLUEPRINT_LUA} -DLUA_LINK_STATIC=${LUA_LINK_STATIC} -DBLUEPRINT_PROTOBUF=${BLUEPRINT_PROTOBUF} -DBUILD_BUNDLE=${BUILD_BUNDLE} -DBUNDLE_LUA=${BUNDLE_LUA} -DBUNDLE_PROTOBUF=${BUNDLE_PROTOBUF} -DBLUEPRINT_LINK_XLUA=${BLUEPRINT_LINK_XLUA}"
        ;;

    ios)
        RUNTIME_ONLY=1
        [ "$(uname -s)" = "Darwin" ] || error "iOS builds must be run on macOS."
        IOS_TOOLCHAIN="${PROJECT_DIR}/cmake/ios.toolchain.cmake"
        IOS_TC_ARG=""
        if [ -f "$IOS_TOOLCHAIN" ]; then
            IOS_TC_ARG="-DCMAKE_TOOLCHAIN_FILE=${IOS_TOOLCHAIN}"
        fi
        LUA_LINK_STATIC="OFF"
        CMAKE_EXTRA_ARGS="${IOS_TC_ARG} -DCMAKE_SYSTEM_NAME=iOS -DCMAKE_OSX_ARCHITECTURES=arm64 -DCMAKE_OSX_DEPLOYMENT_TARGET=13.0 -DBUILD_RUNTIME_ONLY=ON -DBUILD_SHARED_LIBS=OFF -DBUILD_EXAMPLES=OFF -DCMAKE_BUILD_TYPE=${BUILD_TYPE} -G Xcode  -DRUNTIME_NAME=${RUNTIME_NAME} -DBLUEPRINT_LUA=${BLUEPRINT_LUA} -DLUA_LINK_STATIC=OFF -DBLUEPRINT_PROTOBUF=${BLUEPRINT_PROTOBUF} -DBUILD_BUNDLE=${BUILD_BUNDLE} -DBUNDLE_LUA=${BUNDLE_LUA} -DBUNDLE_PROTOBUF=${BUNDLE_PROTOBUF} -DBLUEPRINT_LINK_XLUA=${BLUEPRINT_LINK_XLUA}"
        ;;

    *)
        error "Unknown platform: ${PLATFORM}" ;;
esac

# ── Banner ────────────────────────────────────────────────────────────────────
echo ""
printf "${BOLD}====================================================================${NC}\n"
printf "${BOLD}  Blueprint Editor - Build Script${NC}\n"
printf "  Platform      : ${CYAN}%s${NC}\n" "$PLATFORM"
printf "  Configuration : ${CYAN}%s${NC}\n" "$BUILD_TYPE"
printf "  Runtime only  : ${CYAN}%s${NC}\n" "$RUNTIME_ONLY"
printf "  Shared libs   : ${CYAN}%s${NC}\n" "$BUILD_SHARED"
printf "  LuaVM         : ${CYAN}%s${NC}\n" "$BLUEPRINT_LUA"
printf "  Protobuf      : ${CYAN}%s${NC}\n" "$BLUEPRINT_PROTOBUF"
if [ "$BUILD_SHARED" = "ON" ]; then
    printf "  Static Lua    : ${CYAN}%s${NC}\n" "$LUA_LINK_STATIC"
else
    printf "  Bundle runtime      : ${CYAN}%s${NC}\n" "$BUILD_BUNDLE"
    if [ "$BUILD_BUNDLE" = "ON" ] && [ "$BUNDLE_LUA" = "ON" ]; then
        printf "  -> Bundle Lua      : ${CYAN}%s${NC}\n" "$BUNDLE_LUA"
        printf "  -> Bundle Protobuf : ${CYAN}%s${NC}\n" "$BUNDLE_PROTOBUF"
    fi
fi
printf "  Examples      : ${CYAN}%s${NC}\n" "$BUILD_EXAMPLES"
printf "  Build dir     : ${CYAN}%s${NC}\n" "$BUILD_DIR"
printf "${BOLD}====================================================================${NC}\n"
echo ""

# ── Step 1 - Clean ────────────────────────────────────────────────────────────
if [ $CLEAN_BUILD -eq 1 ]; then
    info "[1/3] Cleaning build directory..."
    rm -rf "${BUILD_DIR}"
    success "Cleaned."
else
    info "[1/3] Skipping clean (pass 'clean' to force)"
fi

mkdir -p "${BUILD_DIR}"

# ── Step 2 - CMake configure ─────────────────────────────────────────────────
info "[2/3] Running CMake configure..."
if [ "$PLATFORM" = "wasm" ]; then
    # shellcheck disable=SC2086
    if is_windows_host; then
        python "${EMCMAKE}" cmake -S "${PROJECT_DIR}" -B "${BUILD_DIR}" ${CMAKE_EXTRA_ARGS}
    else
        "${EMCMAKE}" cmake -S "${PROJECT_DIR}" -B "${BUILD_DIR}" ${CMAKE_EXTRA_ARGS}
    fi
else
    # shellcheck disable=SC2086
    cmake -S "${PROJECT_DIR}" -B "${BUILD_DIR}" ${CMAKE_EXTRA_ARGS}
fi
success "CMake configure complete."

# ── Step 3 - Build ───────────────────────────────────────────────────────────
info "[3/3] Building..."
PARALLEL_JOBS="${PARALLEL_JOBS:-$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)}"

if [ "$PLATFORM" = "ios" ]; then
    cmake --build "${BUILD_DIR}" --config "${BUILD_TYPE}" -- -jobs "${PARALLEL_JOBS}"
else
    cmake --build "${BUILD_DIR}" --config "${BUILD_TYPE}" --parallel "${PARALLEL_JOBS}"
fi

# ── Done ──────────────────────────────────────────────────────────────────────
echo ""
printf "${BOLD}====================================================================${NC}\n"
printf "${GREEN}${BOLD}  Build succeeded!${NC} (%s / %s)\n" "$PLATFORM" "$BUILD_TYPE"
printf "  Output: ${CYAN}%s/bin${NC}\n" "$BUILD_DIR"
if [ "$PLATFORM" = "wasm" ]; then
    printf "  WASM lib: ${CYAN}%s/Runtime/libBlueprintRuntime.a${NC}\n" "$BUILD_DIR"
    printf "  -> Drop into Assets/Plugins/WebGL/ for Unity\n"
elif [ "$PLATFORM" = "android" ]; then
    printf "  Android lib: ${CYAN}%s/Runtime/libBlueprintRuntime.so (or .a)${NC}\n" "$BUILD_DIR"
elif [ "$PLATFORM" = "ios" ]; then
    printf "  iOS lib: ${CYAN}%s/Runtime/libBlueprintRuntime.a${NC}\n" "$BUILD_DIR"
fi
printf "${BOLD}====================================================================${NC}\n"
