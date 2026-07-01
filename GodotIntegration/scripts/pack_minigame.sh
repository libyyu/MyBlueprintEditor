#!/usr/bin/env bash
# 打包 微信/抖音小游戏。
#
# 重要：小游戏形态**不经 Godot**（Godot 无法导出小游戏）。它是独立宿主：
#   引擎 BlueprintRuntime.wasm  +  WechatMiniGame/blueprint-wx-adapter.js  +  你的小游戏脚本(Cocos/Laya/原生)
# HTTP 走 wx.request（LLM.Chat 等），Runtime 代码零修改（BP_SetHttpClient 注入）。
#
# 前置：已 source emsdk（emcmake 可用）。
#   用法：bash pack_minigame.sh
source "$(dirname "${BASH_SOURCE[0]}")/pack_common.sh"

command -v emcmake >/dev/null 2>&1 || die "需要 Emscripten（先 source emsdk_env）"

OUT="$DIST_DIR/minigame"
mkdir -p "$OUT"

# ① 引擎核心：小游戏专用 wasm（BLUEPRINT_WXGAME=ON，导出 addFunction 等额外符号）
log "① building engine wasm for mini-game (BLUEPRINT_WXGAME=ON)"
( cd "$REPO_ROOT"
  emcmake cmake -B build-wxgame \
    -DCMAKE_BUILD_TYPE=Release \
    -DBUILD_RUNTIME_ONLY=ON \
    -DBUILD_SHARED_LIBS=OFF \
    -DBUILD_TESTS=OFF \
    -DBLUEPRINT_LUA=ON \
    -DLUA_LINK_STATIC=ON \
    -DBLUEPRINT_WXGAME=ON
  cmake --build build-wxgame --config Release )

# ② 收集产物：wasm + js 胶水 + adapter
log "② collecting artifacts → $OUT"
# Emscripten 产物名可能是 BlueprintRuntime.js/.wasm，按实际匹配拷贝
find build-wxgame -maxdepth 3 \( -name "*.wasm" -o -name "BlueprintRuntime*.js" \) \
  -exec cp -f {} "$OUT/" \; 2>/dev/null || true
cp -f "$REPO_ROOT/WechatMiniGame/blueprint-wx-adapter.js" "$OUT/" 2>/dev/null \
  || warn "未找到 blueprint-wx-adapter.js"

# ③ 蓝图资源（.bjson）——小游戏没有 Godot 资源系统，蓝图作为普通资源随包
mkdir -p "$OUT/blueprints"
find "$GAME_DIR/blueprints" -name "*.bjson" -exec cp -f {} "$OUT/blueprints/" \; 2>/dev/null || true

log "小游戏产物 → $OUT"
log "  - BlueprintRuntime.wasm / .js   引擎"
log "  - blueprint-wx-adapter.js        wx.request 桥接"
log "  - blueprints/*.bjson             蓝图数据"
warn "接入：把上述文件放进你的小游戏工程，按 WechatMiniGame/README.md 用 BlueprintBridge 初始化"
warn "注意：小游戏形态不含 Godot 渲染/UI；UI 用小游戏引擎(Cocos/Laya)自绘，蓝图只跑逻辑/AI"
