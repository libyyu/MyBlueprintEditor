#!/usr/bin/env bash
# 打包 Web（WebGL / wasm）。
# Web 禁动态加载 → 引擎编成 STATIC .a（Emscripten），GDExtension 静态链接进 wasm。
# 前置：已 source emsdk（emcc/emcmake 可用）；Godot 装了 Web 导出模板。
#   用法：GODOT_BIN=... bash pack_web.sh
source "$(dirname "${BASH_SOURCE[0]}")/pack_common.sh"

command -v emcmake >/dev/null 2>&1 || die "需要 Emscripten（先 source emsdk_env）"

# ① 引擎核心（wasm 静态 .a）
log "① building engine core (wasm/emscripten, static)"
run_build_sh wasm

# ② GDExtension（emcmake 驱动；CMakeLists 检测到 EMSCRIPTEN 会静态链接引擎 .a）
log "② building GDExtension (emscripten)"
( cd "$GODOT_INT_DIR"
  emcmake cmake -B build-wasm-gdext \
    -DBP_BUILD_DIR="build-wasm" \
    -DGDEXT_WITH_LUA=ON \
    -DCMAKE_BUILD_TYPE=Release
  cmake --build build-wasm-gdext --config Release --target blueprint_gdext )

# ③ Godot 导出 Web
godot_export "Web" "$DIST_DIR/web/index.html"
log "Web 完成 → $DIST_DIR/web/（需用支持 SharedArrayBuffer 的 http 头托管）"
warn "Godot 对 Web GDExtension 支持仍在完善；若导出模板不支持，见 docs/01 §7 路线 A/B"
