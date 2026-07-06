/*
 * web_sjlj_stub.c — Emscripten Web (SIDE_MODULE) 专用
 *
 * 背景：
 *   emsdk 3.1.50 编译 Lua 的 ldo.c（用 setjmp/longjmp 做错误处理）时，无论
 *   -O0/-O2、无论是否加 -sSUPPORT_LONGJMP=wasm / -mllvm -wasm-enable-sjlj，
 *   生成的 .o 都会【同时】引用两套 SjLj 运行时符号：
 *     - 新模式（wasm EH）：__wasm_longjmp / __c_longjmp   —— Godot 引擎主模块【有】
 *     - 旧模式（JS stub）：saveSetjmp / testSetjmp         —— Godot 引擎主模块【没有】
 *   于是 side module 加载时 dylink 解析 saveSetjmp 失败：
 *     Aborted(Assertion failed: undefined symbol 'saveSetjmp'. perhaps a side
 *     module was not linked in? ...)
 *
 *   实际运行时 Lua 走的是【新模式】(__wasm_longjmp)，saveSetjmp/testSetjmp
 *   这两个旧模式符号【永远不会被真正调用】——它们只是 emsdk 3.1.50 编译期
 *   甩不掉的死引用。因此在 side module 内部提供空 stub 让 dylink 能解析即可，
 *   不影响任何真实控制流。
 *
 * 仅在 __EMSCRIPTEN__ 下编译进 blueprint_gdext。
 */
#ifdef __EMSCRIPTEN__

#include <stdint.h>

/*
 * Emscripten 旧版 legacy SjLj 的运行时签名（来自 emscripten/src/library_legacy_setjmp.js）：
 *   int   saveSetjmp(void* env, int label, void* table, int size);
 *   int   testSetjmp(uintptr_t id, void* table, int size);
 * 这里给出兼容签名的空实现。返回值取安全默认：
 *   - saveSetjmp 通常返回 table 指针（int 形式），此处返回传入 table 保持形状；
 *   - testSetjmp 返回 0 表示“未匹配”。
 * 由于新模式(__wasm_longjmp)才是真实路径，这两个函数不会进入实际逻辑。
 */
int saveSetjmp(void* env, int label, void* table, int size);
int testSetjmp(uintptr_t id, void* table, int size);

int saveSetjmp(void* env, int label, void* table, int size) {
    (void)env; (void)label; (void)size;
    return (int)(intptr_t)table;
}

int testSetjmp(uintptr_t id, void* table, int size) {
    (void)id; (void)table; (void)size;
    return 0;
}

#endif /* __EMSCRIPTEN__ */
