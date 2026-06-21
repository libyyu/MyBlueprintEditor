#ifndef LUASOCKET_H
#define LUASOCKET_H
/*=========================================================================*\
* LuaSocket toolkit
* Networking support for the Lua language
* Diego Nehab
* 9/11/1999
\*=========================================================================*/
#include "lua.hpp"

/*-------------------------------------------------------------------------*\
* Current socket library version
\*-------------------------------------------------------------------------*/
#define LUASOCKET_VERSION    "LuaSocket 3.0-rc1"
#define LUASOCKET_COPYRIGHT  "Copyright (C) 1999-2013 Diego Nehab"

/*-------------------------------------------------------------------------*\
* This macro prefixes all exported API functions
\*-------------------------------------------------------------------------*/
// #ifndef LUASOCKET_API
// #define LUASOCKET_API extern
// #endif

// #ifdef LUASOCKET_STATIC
// #	define LUASOCKET_API
// #else
// #	ifdef _MSC_VER
// #		ifdef LUASOCKET_SHARED
// #			define LUASOCKET_API __declspec(dllexport)
// #		else
// #			define LUASOCKET_API __declspec(dllimport)
// #		endif
// #	else
// #		define LUASOCKET_API
// #	endif
// #endif
#define LUASOCKET_API LUALIB_API
/*-------------------------------------------------------------------------*\
* Initializes the library.
\*-------------------------------------------------------------------------*/
LUASOCKET_API int luaopen_socket_core(lua_State *L);

#endif /* LUASOCKET_H */
