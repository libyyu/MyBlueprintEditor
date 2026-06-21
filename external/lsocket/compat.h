#ifndef COMPAT_H
#define COMPAT_H

#include "lua.hpp"
#ifndef _LUA_JIT
#if LUA_VERSION_NUM==501
void luaL_setfuncs (lua_State *L, const luaL_Reg *l, int nup);
#endif
#endif

#endif
