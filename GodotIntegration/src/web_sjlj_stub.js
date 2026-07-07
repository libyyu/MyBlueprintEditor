/*
 * web_sjlj_stub.js — Emscripten Web (SIDE_MODULE) 专用 JS-library stub
 *
 * 背景：
 *   emsdk 3.1.50 编 Lua ldo.c 时，无论如何都会 emit 对旧模式 SjLj JS-helper
 *   的引用：
 *     saveSetjmp / testSetjmp / emscripten_longjmp
 *   这些不是 C 函数——它们是 Emscripten JS runtime 里的 helper（原本由
 *   emcc 内置的 library_setjmp.js 提供）。
 *
 *   Godot 4.5+ Web export template 用新模式 wasm-native SjLj 编译，template
 *   的 JS runtime 里【没有】这三个旧模式 helper 的实现，dylink 时 side module
 *   通过 stub 表调用它们 → stubs.<computed> 报：
 *     TypeError: resolved is not a function
 *
 * 修复：用 --js-library 显式给我们的 side module 注入空 JS 实现，dylink 时
 * 直接从 side module 自己的 JS runtime 里解析。
 *
 * 安全性：Lua 使用 setjmp 只是为了错误传播（pcall / error）。
 *   - 无错误路径：完全不走 longjmp，正常 return，行为等价
 *   - 有错误路径：longjmp 变 no-op → 错误不被捕获，直接崩溃
 *   → 用户 Lua 代码保证 pcall 内无 uncaught error 时可用。
 *
 * 未来正解：升级 emsdk 到与 Godot template 完全对齐的版本（4.0.20+），或者
 * 用 --js-library 提供真正的错误传播实现（把错误码存到 Module.__sjlj_error）。
 */

mergeInto(LibraryManager.library, {
    saveSetjmp: function(env, label, table, size) {
        // 旧模式 SjLj 保存 CPU 状态。空实现返回 0 表示"首次调用"，Lua 视为
        // 保护块起点，控制流正常继续。
        return 0;
    },

    testSetjmp: function(id, table, size) {
        // 旧模式 SjLj 匹配 jmp_buf id。空实现返回 0 表示"未匹配"，Lua 会
        // 走"错误未被本层捕获，往上抛"分支——但由于 emscripten_longjmp 也是
        // 空 stub，最终会 fall-through 到 undefined behavior。
        return 0;
    },

    emscripten_longjmp: function(env, val) {
        // 旧模式 SjLj 跳转。空实现 = no-op。
        //
        // ⚠️ 语义丢失：Lua error() / pcall 的错误传播会失效。用户脚本若真的
        // 抛错，控制流会继续执行到 luaD_throw 之后的 unreachable 分支，导致
        // wasm trap。这是 Godot Web GDExtension 目前的固有限制。
    }
});
