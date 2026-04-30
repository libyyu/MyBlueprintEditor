#!/usr/bin/env bash
# ==============================================================================
# package.sh – One-click build + package for MyBlueprintEditor
#
# Usage:
#   ./package.sh [preset] [options]
#   bash package.sh [preset] [options]
#
# Presets:
#   full    All features (Lua VM + Protobuf nodes)        [default]
#   nolua   No built-in Lua (for xLua/tolua integration)
#   min     Minimal (core blueprint engine only, no Lua, no Protobuf)
#
# Options:
#   --skip-build  Skip compilation, use existing build artifacts
#   -h / --help   Show this help
#
# Works on: Linux, macOS, Windows (MSYS2/MinGW/Git Bash)
# ==============================================================================

# ── Colours ──────────────────────────────────────────────────────────────────
RED='\033[0;31m'; GREEN='\033[0;32m'; YELLOW='\033[1;33m'
CYAN='\033[0;36m'; BOLD='\033[1m'; NC='\033[0m'

info()    { echo -e "${CYAN}[INFO]${NC}  $*"; }
success() { echo -e "${GREEN}[ OK ]${NC} $*"; }
warn()    { echo -e "${YELLOW}[WARN]${NC} $*"; }
die()     { echo -e "${RED}[ERROR]${NC} $*" >&2; exit 1; }

if [ ! "${BASH_SOURCE}" ]; then
	SCRIPT_PATH=$(dirname $0)
else
	SCRIPT_PATH=$(dirname "${BASH_SOURCE}")
fi

PROJECT_DIR="$(cd "$(dirname "$SCRIPT_PATH")" && pwd)"
DIST_DIR="${PROJECT_DIR}/dist"
HOST_OS="$(uname -s)"
SKIP_BUILD=0
PRESET=""

# ── Argument parsing ─────────────────────────────────────────────────────────
show_help() {
    sed -n '3,/^# ===/p' "$0" | sed 's/^# \?//'
    exit 0
}

while [[ $# -gt 0 ]]; do
    case "$1" in
        full|Full|FULL)       PRESET="full";  shift ;;
        nolua|NoLua|NOLUA)    PRESET="nolua"; shift ;;
        min|Min|MIN)          PRESET="min";   shift ;;
        --skip-build)         SKIP_BUILD=1;   shift ;;
        -h|--help)            show_help ;;
        *) die "Unknown option: $1 (use --help)" ;;
    esac
done

# ── Interactive menu (if no preset given) ────────────────────────────────────
if [[ -z "$PRESET" ]]; then
    echo ""
    echo -e "${BOLD}============================================${NC}"
    echo -e "${BOLD}  MyBlueprintEditor - Package Tool${NC}"
    echo -e "${BOLD}============================================${NC}"
    echo ""
    echo "  [1] Full (recommended)"
    echo "      Runtime + Lua VM + Protobuf"
    echo ""
    echo "  [2] No Lua"
    echo "      Runtime + Protobuf, for xLua/tolua"
    echo ""
    echo "  [3] Minimal"
    echo "      Core engine only, smallest size"
    echo ""
    read -rp "  Select [1/2/3] (default 1): " CHOICE
    CHOICE="${CHOICE:-1}"
    case "$CHOICE" in
        1) PRESET="full" ;;
        2) PRESET="nolua" ;;
        3) PRESET="min" ;;
        *) die "Invalid choice: $CHOICE" ;;
    esac
fi

# ── Resolve preset variables ─────────────────────────────────────────────────
case "$PRESET" in
    full)
        DLL_NAME="BlueprintRuntime"
        SO_NAME="libBlueprintRuntime"
        BUNDLE_NAME="libBlueprintBundle"
        BUNDLE_LUA="ON"
        BUNDLE_PROTO="ON"
        PRESET_DESC="Full"
        ;;
    nolua)
        DLL_NAME="BlueprintRuntimeNoLua"
        SO_NAME="libBlueprintRuntimeNoLua"
        BUNDLE_NAME="libBlueprintBundleNoLua"
        BUNDLE_LUA="OFF"
        BUNDLE_PROTO="ON"
        PRESET_DESC="NoLua"
        ;;
    min)
        DLL_NAME="BlueprintRuntimeNoLua"
        SO_NAME="libBlueprintRuntimeNoLua"
        BUNDLE_NAME="libBlueprintBundleMin"
        BUNDLE_LUA="OFF"
        BUNDLE_PROTO="OFF"
        PRESET_DESC="Minimal"
        ;;
esac

echo ""
echo -e "  Preset      : ${CYAN}${PRESET_DESC}${NC}"
echo -e "  DLL         : ${DLL_NAME}.dll"
echo -e "  SO          : ${SO_NAME}.so"
echo -e "  WASM        : ${BUNDLE_NAME}.a"
echo -e "  PROJECTPATH : ${PROJECT_DIR}"
echo ""

# ── Helper: parallel job count ───────────────────────────────────────────────
NPROC="$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)"

# ── Detect host type ─────────────────────────────────────────────────────────
# MSYS/MinGW/Git-Bash: uname -s → MINGW*, MSYS*, CYGWIN*
# busybox on Windows:  uname -s → Windows_NT
is_windows_host() {
    case "$HOST_OS" in
        MINGW*|MSYS*|CYGWIN*|Windows_NT|WINDOWS*) return 0 ;;
        *) return 1 ;;
    esac
}
find_command() {
	which $1 > /dev/null 2>&1
	if [ $? == 0 ]; then
	    echo true
	else
	    echo false
	fi
}

# ── 1/3: Build ───────────────────────────────────────────────────────────────
if [[ $SKIP_BUILD -eq 1 ]]; then
    info "[1/3] Skip build, using existing artifacts"
else
    info "[1/3] Building all platforms..."
    echo ""

    # -- Check toolchains --
    HAS_EMSDK=0
    HAS_NDK=0
    [[ -n "${EMSDK:-}" ]] && HAS_EMSDK=1
    if [ $HAS_EMSDK != 1 ] && [ "$(find_command emsdk)" = "true" ]; then
        EMSDK_PATH="$(which emsdk)"
        EMSDK_DIR="$(dirname "$EMSDK_PATH")"
        export EMSDK="$EMSDK_DIR"
        HAS_EMSDK=1
    fi
    [[ -n "${ANDROID_NDK:-}" || -n "${ANDROID_NDK_HOME:-}" ]] && HAS_NDK=1
    BUILD_SH="${PROJECT_DIR}/build.sh"
    BUILD_BAT="${PROJECT_DIR}/build.bat"

    # -- Host platform (Windows / Linux / macOS) --
    if is_windows_host; then
        info "  [Windows] Building..."
        _win_built=0
        # Configure with lua=ON
        cmake -S "$PROJECT_DIR" -B "${PROJECT_DIR}/build-windows" -DBLUEPRINT_LUA=ON > /dev/null 2>&1
        if [[ ! $? -eq 0 ]]; then
            warn "  [Windows] CONFIGURE FAILED"; exit 1
        fi

        sh "$BUILD_SH" windows release
        if [[ $? -eq 0 ]]; then
            success "  [Windows] OK"
        else
            warn "  [Windows] FAILED"; exit 1
        fi

        # NoLua variant
        if [[ "$PRESET" != "full" ]]; then
            info "  [Windows] Building NoLua variant..."
            local_build_dir="${PROJECT_DIR}/build-windows"
            cmake -S "$PROJECT_DIR" -B "$local_build_dir" -DBLUEPRINT_LUA=OFF > /dev/null 2>&1
            cmake --build "$local_build_dir" --target BlueprintRuntime --config Release --parallel "$NPROC" > /dev/null 2>&1
            # Rename output
            for ext in dll so dylib; do
                src="${local_build_dir}/bin/Release/BlueprintRuntime.${ext}"
                dst="${local_build_dir}/bin/Release/BlueprintRuntimeNoLua.${ext}"
                [[ -f "$src" ]] && cp -f "$src" "$dst"
            done
            # Restore LUA=ON
            cmake -S "$PROJECT_DIR" -B "$local_build_dir" -DBLUEPRINT_LUA=ON > /dev/null 2>&1
            cmake --build "$local_build_dir" --target BlueprintRuntime --config Release --parallel "$NPROC" > /dev/null 2>&1
            success "  [Windows] NoLua OK"
        fi
    elif [[ "$HOST_OS" == "Linux" ]]; then
        info "  [Linux]   Building..."
        bash "$BUILD_SH" linux release > /dev/null 2>&1 \
            && success "  [Linux]   OK" \
            || { warn "  [Linux]   FAILED"; exit 1; }

        if [[ "$PRESET" != "full" ]]; then
            info "  [Linux]   Building NoLua variant..."
            local_build_dir="${PROJECT_DIR}/build-linux"
            cmake -S "$PROJECT_DIR" -B "$local_build_dir" -DBLUEPRINT_LUA=OFF > /dev/null 2>&1
            cmake --build "$local_build_dir" --target BlueprintRuntime --config Release --parallel "$NPROC" > /dev/null 2>&1
            for f in "${local_build_dir}/bin/libBlueprintRuntime".*; do
                [[ -f "$f" ]] && cp -f "$f" "${f/libBlueprintRuntime/libBlueprintRuntimeNoLua}"
            done
            cmake -S "$PROJECT_DIR" -B "$local_build_dir" -DBLUEPRINT_LUA=ON > /dev/null 2>&1
            cmake --build "$local_build_dir" --target BlueprintRuntime --config Release --parallel "$NPROC" > /dev/null 2>&1
            success "  [Linux]   NoLua OK"
        fi
    elif [[ "$HOST_OS" == "Darwin" ]]; then
        info "  [macOS]   Building..."
        bash "$BUILD_SH" macos release > /dev/null 2>&1 \
            && success "  [macOS]   OK" \
            || { warn "  [macOS]   FAILED"; exit 1; }

        if [[ "$PRESET" != "full" ]]; then
            info "  [macOS]   Building NoLua variant..."
            local_build_dir="${PROJECT_DIR}/build-macos"
            cmake -S "$PROJECT_DIR" -B "$local_build_dir" -DBLUEPRINT_LUA=OFF > /dev/null 2>&1
            cmake --build "$local_build_dir" --target BlueprintRuntime --config Release --parallel "$NPROC" > /dev/null 2>&1
            for f in "${local_build_dir}/bin/libBlueprintRuntime".*; do
                [[ -f "$f" ]] && cp -f "$f" "${f/libBlueprintRuntime/libBlueprintRuntimeNoLua}"
            done
            cmake -S "$PROJECT_DIR" -B "$local_build_dir" -DBLUEPRINT_LUA=ON > /dev/null 2>&1
            cmake --build "$local_build_dir" --target BlueprintRuntime --config Release --parallel "$NPROC" > /dev/null 2>&1
            success "  [macOS]   NoLua OK"
        fi
    fi

    # -- WebGL --
    if [[ $HAS_EMSDK -eq 1 ]]; then
        echo -e "\n"
        info "  [WebGL]   Building..."
        sh "$BUILD_SH" wasm release
        if [[ $? -eq 0 ]]; then
            success "  [WebGL]   Runtime OK"
            info "  [WebGL]   Merging Bundle..."
            cmake -S "$PROJECT_DIR" -B "${PROJECT_DIR}/build-wasm" \
                -UBUILD_BUNDLE -UBUNDLE_LUA -UBUNDLE_PROTOBUF -UBUNDLE_NAME \
                -DBUILD_BUNDLE=ON -DBUNDLE_LUA="${BUNDLE_LUA}" \
                -DBUNDLE_PROTOBUF="${BUNDLE_PROTO}" \
                -DCMAKE_BUILD_TYPE=Release > /dev/null 2>&1
            cmake --build "${PROJECT_DIR}/build-wasm" --target BlueprintBundle \
                --config Release --parallel "$NPROC" > /dev/null 2>&1
            success "  [WebGL]   Bundle OK"
        else
            warn "  [WebGL]   FAILED"
        fi
    else
        echo -e "\n"
        warn "  [WebGL]   SKIP - Emscripten not found. Set \$EMSDK to enable."
    fi

    # -- Android --
    if [[ $HAS_NDK -eq 1 ]]; then
        _NDK="${ANDROID_NDK:-${ANDROID_NDK_HOME:-}}"
        echo -e "\n"
        info "  [Android] Building..."
        sh "$BUILD_SH" android release --ndk "$_NDK" > /dev/null 2>&1 \
            && success "  [Android] OK" \
            || warn "  [Android] FAILED"
    else
        echo -e "\n"
        warn "  [Android] SKIP - NDK not found. Set \$ANDROID_NDK to enable."
    fi

    # -- iOS (macOS only) --
    if [[ "$HOST_OS" == "Darwin" ]]; then
        echo -e "\n"
        info "  [iOS]     Building..."
        bash "$BUILD_SH" ios release > /dev/null 2>&1 \
            && success "  [iOS]     OK" \
            || warn "  [iOS]     FAILED"
    fi
fi

# ── 2/3: Collect artifacts ───────────────────────────────────────────────────
echo -e "\n"
info "[2/3] Collecting artifacts to dist/..."

rm -rf "$DIST_DIR"
mkdir -p "${DIST_DIR}/UnityPlugins/Windows/x86_64"
mkdir -p "${DIST_DIR}/UnityPlugins/Android/arm64-v8a"
mkdir -p "${DIST_DIR}/UnityPlugins/WebGL"
mkdir -p "${DIST_DIR}/Editor"

COLLECTED=0

# Helper: try to copy a file, print status
collect_file() {
    local src="$1" dst="$2" label="$3"
    if [[ -f "$src" ]]; then
        cp -f "$src" "$dst"
        success "  ${label}"
        COLLECTED=$((COLLECTED + 1))
    else
        echo -e "  ${YELLOW}--${NC}  ${label} (not found)"
    fi
}

# -- Windows DLL --
# MSVC builds to build-windows/bin/Release, MinGW to build-windows-dll/bin/Release
for win_dir in "${PROJECT_DIR}/build-windows/bin/Release" \
               "${PROJECT_DIR}/build-windows-dll/bin/Release" \
               "${PROJECT_DIR}/build-windows/bin"; do
    if [[ -f "${win_dir}/${DLL_NAME}.dll" ]]; then
        WIN_BIN="$win_dir"
        break
    fi
done
WIN_BIN="${WIN_BIN:-${PROJECT_DIR}/build-windows/bin/Release}"

collect_file "${WIN_BIN}/${DLL_NAME}.dll" \
    "${DIST_DIR}/UnityPlugins/Windows/x86_64/BlueprintRuntime.dll" \
    "Windows: ${DLL_NAME}.dll"

if [[ "$PRESET" == "full" && -f "${WIN_BIN}/liblua54.dll" ]]; then
    collect_file "${WIN_BIN}/liblua54.dll" \
        "${DIST_DIR}/UnityPlugins/Windows/x86_64/liblua54.dll" \
        "Windows: liblua54.dll"
fi

# -- Editor --
for editor_src in "${WIN_BIN}/BlueprintEditor.exe" \
                  "${WIN_BIN}/BlueprintEditor"; do
    if [[ -f "$editor_src" ]]; then
        cp -f "$editor_src" "${DIST_DIR}/Editor/$(basename "$editor_src")"
        # Also copy runtime libs for editor
        for lib in BlueprintRuntime.dll BlueprintRuntime.so libBlueprintRuntime.so \
                   libBlueprintRuntime.dylib liblua54.dll liblua54.so liblua54.dylib; do
            [[ -f "${WIN_BIN}/${lib}" ]] && cp -f "${WIN_BIN}/${lib}" "${DIST_DIR}/Editor/${lib}"
        done
        success "  Editor: $(basename "$editor_src")"
        COLLECTED=$((COLLECTED + 1))
        break
    fi
done

# Also check host-native editor (Linux/macOS)
for host_dir in "${PROJECT_DIR}/build-linux/bin" \
                "${PROJECT_DIR}/build-macos/bin"; do
    if [[ -f "${host_dir}/BlueprintEditor" ]]; then
        cp -f "${host_dir}/BlueprintEditor" "${DIST_DIR}/Editor/BlueprintEditor"
        for lib in libBlueprintRuntime.so libBlueprintRuntime.dylib \
                   liblua54.so liblua54.dylib; do
            [[ -f "${host_dir}/${lib}" ]] && cp -f "${host_dir}/${lib}" "${DIST_DIR}/Editor/${lib}"
        done
        success "  Editor: BlueprintEditor ($(basename "$(dirname "$(dirname "$host_dir")")"))"
        COLLECTED=$((COLLECTED + 1))
        break
    fi
done

# -- Linux SO --
for lin_dir in "${PROJECT_DIR}/build-linux/bin/Release" \
               "${PROJECT_DIR}/build-linux/bin"; do
    if [[ -f "${lin_dir}/libBlueprintRuntime.so" ]]; then
        mkdir -p "${DIST_DIR}/UnityPlugins/Linux/x86_64"
        collect_file "${lin_dir}/libBlueprintRuntime.so" \
            "${DIST_DIR}/UnityPlugins/Linux/x86_64/libBlueprintRuntime.so" \
            "Linux: libBlueprintRuntime.so"
        [[ "$PRESET" == "full" && -f "${lin_dir}/liblua54.so" ]] && \
            collect_file "${lin_dir}/liblua54.so" \
            "${DIST_DIR}/UnityPlugins/Linux/x86_64/liblua54.so" \
            "Linux: liblua54.so"
        break
    fi
done

# -- macOS --
for mac_dir in "${PROJECT_DIR}/build-macos/bin/Release" \
               "${PROJECT_DIR}/build-macos/bin"; do
    for ext in dylib so bundle; do
        if [[ -f "${mac_dir}/libBlueprintRuntime.${ext}" ]]; then
            mkdir -p "${DIST_DIR}/UnityPlugins/macOS"
            collect_file "${mac_dir}/libBlueprintRuntime.${ext}" \
                "${DIST_DIR}/UnityPlugins/macOS/libBlueprintRuntime.${ext}" \
                "macOS: libBlueprintRuntime.${ext}"
            [[ "$PRESET" == "full" && -f "${mac_dir}/liblua54.dylib" ]] && \
                collect_file "${mac_dir}/liblua54.dylib" \
                "${DIST_DIR}/UnityPlugins/macOS/liblua54.dylib" \
                "macOS: liblua54.dylib"
            break 2
        fi
    done
done

# -- Android --
for and_dir in "${PROJECT_DIR}/build-android/bin/Release" \
               "${PROJECT_DIR}/build-android/bin"; do
    if [[ -f "${and_dir}/${SO_NAME}.so" ]]; then
        collect_file "${and_dir}/${SO_NAME}.so" \
            "${DIST_DIR}/UnityPlugins/Android/arm64-v8a/libBlueprintRuntime.so" \
            "Android: ${SO_NAME}.so"
        break
    elif [[ -f "${and_dir}/libBlueprintRuntime.so" ]]; then
        collect_file "${and_dir}/libBlueprintRuntime.so" \
            "${DIST_DIR}/UnityPlugins/Android/arm64-v8a/libBlueprintRuntime.so" \
            "Android: libBlueprintRuntime.so"
        break
    fi
done

# -- WebGL --
WASM_FOUND=0
for wasm_dir in "${PROJECT_DIR}/build-wasm/bin/Release" \
                "${PROJECT_DIR}/build-wasm/bin" \
                "${PROJECT_DIR}/build-wasm/Runtime"; do
    if [[ -f "${wasm_dir}/${BUNDLE_NAME}.a" ]]; then
        collect_file "${wasm_dir}/${BUNDLE_NAME}.a" \
            "${DIST_DIR}/UnityPlugins/WebGL/libBlueprintRuntime.a" \
            "WebGL: ${BUNDLE_NAME}.a"
        WASM_FOUND=1
        break
    fi
done
if [[ $WASM_FOUND -eq 0 ]]; then
    # Fallback: unbundled runtime
    for wasm_dir in "${PROJECT_DIR}/build-wasm/Runtime" \
                    "${PROJECT_DIR}/build-wasm/bin/Release"; do
        if [[ -f "${wasm_dir}/libBlueprintRuntime.a" ]]; then
            collect_file "${wasm_dir}/libBlueprintRuntime.a" \
                "${DIST_DIR}/UnityPlugins/WebGL/libBlueprintRuntime.a" \
                "WebGL: libBlueprintRuntime.a (unbundled fallback)"
            break
        fi
    done
fi

# -- iOS (macOS only) --
for ios_dir in "${PROJECT_DIR}/build-ios/bin/Release" \
               "${PROJECT_DIR}/build-ios/bin/Release-iphoneos" \
               "${PROJECT_DIR}/build-ios/bin"; do
    for bname in "libBlueprintBundle.a" "libBlueprintRuntime.a"; do
        if [[ -f "${ios_dir}/${bname}" ]]; then
            mkdir -p "${DIST_DIR}/UnityPlugins/iOS"
            collect_file "${ios_dir}/${bname}" \
                "${DIST_DIR}/UnityPlugins/iOS/libBlueprintRuntime.a" \
                "iOS: ${bname}"
            break 2
        fi
    done
done

# ── 3/3: Generate README ────────────────────────────────────────────────────
echo ""
info "[3/3] Done!"

cat > "${DIST_DIR}/README.txt" <<EOF
Preset: ${PRESET_DESC}
Date: $(date '+%Y-%m-%d %H:%M:%S')

Copy UnityPlugins/ to your Unity project Assets/Plugins/
Copy Unity/Assets/Scripts/BlueprintRuntime.cs to Assets/Scripts/
EOF

echo ""
echo -e "${BOLD}============================================${NC}"
echo -e "${BOLD}  Package complete!${NC}"
echo -e "  Preset    : ${CYAN}${PRESET_DESC}${NC}"
echo -e "  Output    : ${CYAN}${DIST_DIR}${NC}"
echo -e "  Platforms : ${GREEN}${COLLECTED}${NC}"
echo -e "${BOLD}============================================${NC}"
echo ""
echo "  dist/"
echo "    UnityPlugins/    -- copy to Unity Assets/Plugins/"
echo "    Editor/          -- blueprint editor"
echo "    README.txt"
echo ""
