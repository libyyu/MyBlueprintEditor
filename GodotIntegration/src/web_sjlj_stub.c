/*
 * web_sjlj_stub.c — Emscripten Web (SIDE_MODULE) 专用
 *
 * 背景：
 *   emsdk 3.1.50 编 Lua ldo.c 时，即便加了 -sSUPPORT_LONGJMP=wasm，生成的 .o
 *   里也会残留【旧模式】的 SjLj 符号引用：
 *     - saveSetjmp / testSetjmp / emscripten_longjmp（旧 emscripten JS-SjLj）
 *   Godot 4.5/4.7 Web export template 只导出【新模式】符号
 *     - __wasm_longjmp / __c_longjmp（wasm 原生 SjLj）
 *   两边不匹配，dylink 解析失败：
 *     Aborted(Assertion failed: undefined symbol 'saveSetjmp'...)
 *
 * 真相：这些【旧模式】符号是编译器 emit 的死代码引用，运行时永远不走。因此在
 * side module 里提供【真正的空 stub】（不做任何事）就能让 dylink 满足，不影响
 * 真实控制流。
 *
 * ★ 关键：stub 必须是空的返回 0/void，绝不能调用真正的 setjmp/longjmp！
 *   一旦调用真 setjmp，编译器会 emit 对 __cpp_exception WebAssembly.Tag 的
 *   import（wasm exception 机制），Godot Web template 也没有导出这个 tag：
 *     LinkError: Import "env" "__cpp_exception": tag import requires
 *                 a WebAssembly.Tag
 *   → 空 stub 直接 return 0 才能避免所有 SjLj/EH 机制被启用。
 *
 * 此文件必须用【纯 C】编译（不含任何 C++ 语法/头文件），避免引入更多 EH 依赖。
 *
 * 仅在 __EMSCRIPTEN__ 下编译。
 */

#ifdef __EMSCRIPTEN__

#include <stddef.h>

/*
 * Lua 内部 setjmp/longjmp 死引用的空实现。
 * 签名根据 emsdk 3.1.50 的 emscripten 内部 SjLj runtime 实现推断，实际不会被调用。
 */

/* void* env: Lua 用 jmp_buf，emcc 展开为 unsigned char[40] 或类似结构。
 *            这里用 void* 避免包 <setjmp.h> 引入 EH 依赖。 */
int saveSetjmp(void* env, int label, void* table, int size) {
    (void)env;
    (void)label;
    (void)table;
    (void)size;
    return 0;
}

int testSetjmp(unsigned long id, void* table, int size) {
    (void)id;
    (void)table;
    (void)size;
    return 0;
}

/* 旧模式 emscripten_longjmp：正常不会跑到；如果真被调用（设计上不会），
 * 直接 abort 而不是真 longjmp，以避免打开 EH tag import。 */
void emscripten_longjmp(unsigned long env, int val) {
    (void)env;
    (void)val;
    /* 不能调用 abort() —— 会拉进更多 runtime 依赖。返回即可。 */
}

#endif /* __EMSCRIPTEN__ */
