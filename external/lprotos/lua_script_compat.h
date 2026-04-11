#pragma once
// Minimal compatibility shim replacing base/lua_script.hpp
// Only the symbols used by LuaDynamicProtobuf.cpp are provided.

#ifdef __cplusplus
extern "C" {
#endif
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
#ifdef __cplusplus
}
#endif

#include <cstdint>
#include <cstdio>
#include <cstdarg>

namespace lua {

using int64  = int64_t;
using uint64 = uint64_t;

// push int64 / uint64 as Lua integer (lua_Integer is 64-bit in Lua 5.3+)
inline void push(lua_State* L, int64 v)  { lua_pushinteger(L, (lua_Integer)v); }
inline void push(lua_State* L, uint64 v) { lua_pushinteger(L, (lua_Integer)v); }

// get int64 / uint64 from stack
inline void get(lua_State* L, int idx, int64*  out) { *out = (int64)luaL_checkinteger(L, idx); }
inline void get(lua_State* L, int idx, uint64* out) { *out = (uint64)luaL_checkinteger(L, idx); }

// log helpers — just print to stdout/stderr
inline void lua_log(lua_State*, const char* fmt, ...) {
    va_list ap; va_start(ap, fmt);
    vprintf(fmt, ap); va_end(ap);
    printf("\n");
}
inline void lua_printerror(lua_State*, const char* fmt, ...) {
    va_list ap; va_start(ap, fmt);
    vfprintf(stderr, fmt, ap); va_end(ap);
    fprintf(stderr, "\n");
}

// on-close cleaner: register a __gc-style callback via registry
// We use a simple approach: push a userdata with __gc metamethod.
inline void add_lua_close_cleaner(lua_State* L, int(*fn)(lua_State*)) {
    // Create a userdata whose __gc calls fn when Lua state closes
    void** ud = (void**)lua_newuserdata(L, sizeof(void*));
    *ud = (void*)(intptr_t)fn;
    lua_newtable(L);  // metatable
    lua_pushstring(L, "__gc");
    lua_pushcfunction(L, [](lua_State* LL) -> int {
        void** p = (void**)lua_touserdata(LL, 1);
        if (p && *p) {
            auto f = (int(*)(lua_State*))(*p);
            f(LL);
        }
        return 0;
    });
    lua_rawset(L, -3);
    lua_setmetatable(L, -2);
    // Store in registry so it lives as long as the state
    lua_rawsetp(L, LUA_REGISTRYINDEX, (void*)fn);
}

} // namespace lua
