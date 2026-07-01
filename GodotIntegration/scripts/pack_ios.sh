#!/usr/bin/env bash
# 打包 iOS（必须在 macOS 上跑）。
# iOS 禁止动态库随意加载 → 引擎与 GDExtension 都走 STATIC，.gdextension 用 __Internal。
#   用法（macOS）：GODOT_BIN=... bash pack_ios.sh
source "$(dirname "${BASH_SOURCE[0]}")/pack_common.sh"

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
