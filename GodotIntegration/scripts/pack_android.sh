#!/usr/bin/env bash
# 打包 Android（APK / AAB）。
# 前置：ANDROID_NDK_HOME 指向 NDK；Godot 已安装 Android 导出模板 + 配好 keystore。
#   用法：GODOT_BIN=... ANDROID_NDK_HOME=... bash pack_android.sh [apk|aab]
source "$(dirname "${BASH_SOURCE[0]}")/pack_common.sh"

FORMAT="${1:-apk}"
ABI="${ANDROID_ABI:-arm64-v8a}"
API="${ANDROID_API:-24}"
[[ -n "$NDK_PATH" ]] || die "需要 ANDROID_NDK_HOME / NDK_PATH"

# ① 引擎核心（shared .so，Android）
log "① building engine core (android, $ABI)"
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
