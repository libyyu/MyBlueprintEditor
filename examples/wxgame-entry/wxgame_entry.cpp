// wxgame_entry.cpp
// ----------------------------------------------------------------------------
// 微信/抖音小游戏专用 wasm 入口。
//
// 目的：产出一份纯粹的 BlueprintRuntime.js + .wasm，只暴露引擎的 C API，
//       不夹带 runtime-example CLI / REPL / --watch / 单测 / 内置 demo 等
//       桌面调试用代码——那些代码会拖入 <iostream>/<filesystem>/<thread>/
//       <csignal> 等标准库，浪费小游戏包体和冷启动时间。
//
// 说明：
//   - Emscripten 要求可执行 target 必须有 main()。这里 main() 为空即可，
//     JS 侧永远不通过 main() 驱动逻辑，全部走 ccall/cwrap → _BP_* C API。
//   - -sMODULARIZE=1 + -sEXPORT_NAME=BlueprintRuntime 的组合下，
//     JS 侧调用 `globalThis.BlueprintRuntime(...)` 时会异步实例化模块，
//     此时 main() 会被自动执行一次；空 main 让这次执行成为零副作用 no-op。
//   - 所有蓝图运行时 C API（BP_CreateRunner / BP_LoadFromJson / BP_Tick / ...）
//     由 BlueprintRuntime 静态库提供，通过 -sEXPORTED_FUNCTIONS 保留符号。
// ----------------------------------------------------------------------------

extern "C" int main(int /*argc*/, char** /*argv*/) {
    // 故意留空。JS 侧通过 ccall/cwrap 调用 _BP_* 函数即可。
    return 0;
}
