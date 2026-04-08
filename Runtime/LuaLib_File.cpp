// Runtime/LuaLib_File.cpp
// file.* Lua 全局库实现
//
//   file.read(path)              → content:string | nil, err:string
//   file.write(path, content)    → true | false, err:string
//   file.append(path, content)   → true | false, err:string
//   file.exists(path)            → bool
//   file.delete(path)            → true | false, err:string
//   file.size(path)              → int64 (-1 on error)
//   file.listdir(path [, pat])   → {name,...} | nil, err:string
//   file.mkdir(path)             → true | false, err:string
//   file.basename(path)          → string
//   file.dirname(path)           → string
//   file.join(base, part)        → string
//
// 平台差异由 DefaultFileSystem 内部处理；WebGL 写/目录操作返回 false + errMsg。

#ifdef BLUEPRINT_HAS_LUA

#include "LuaLib_Internal.h"
#include "LuaBindings.h"
#include "FileSystem.h"

#include <vector>
#include <string>

namespace NodeEditor {
namespace Runtime {

void RegisterLuaFileLib(lua_State* L)
{
    lua_newtable(L);

    // file.read(path) → content | nil, err
    {
        lua_CFunction fn = [](lua_State* Lx) -> int {
            const char* path = luaL_checkstring(Lx, 1);
            auto* fs = GetDefaultFileSystem().get();
            if (!fs) { lua_pushnil(Lx); lua_pushstring(Lx, "FileSystem not available"); return 2; }
            std::string content, err;
            if (fs->ReadFile(path, content, err)) {
                lua_pushlstring(Lx, content.c_str(), content.size()); return 1;
            }
            lua_pushnil(Lx); lua_pushstring(Lx, err.c_str()); return 2;
        };
        lua_pushcfunction(L, fn);
    }
    lua_setfield(L, -2, "read");

    // file.write(path, content) → true | false, err
    {
        lua_CFunction fn = [](lua_State* Lx) -> int {
            const char* path = luaL_checkstring(Lx, 1);
            size_t len = 0;
            const char* content = luaL_checklstring(Lx, 2, &len);
            auto* fs = GetDefaultFileSystem().get();
            if (!fs) { lua_pushboolean(Lx, 0); lua_pushstring(Lx, "FileSystem not available"); return 2; }
            std::string err;
            if (fs->WriteFile(path, std::string(content, len), err)) { lua_pushboolean(Lx, 1); return 1; }
            lua_pushboolean(Lx, 0); lua_pushstring(Lx, err.c_str()); return 2;
        };
        lua_pushcfunction(L, fn);
    }
    lua_setfield(L, -2, "write");

    // file.append(path, content) → true | false, err
    {
        lua_CFunction fn = [](lua_State* Lx) -> int {
            const char* path = luaL_checkstring(Lx, 1);
            size_t len = 0;
            const char* content = luaL_checklstring(Lx, 2, &len);
            auto* fs = GetDefaultFileSystem().get();
            if (!fs) { lua_pushboolean(Lx, 0); lua_pushstring(Lx, "FileSystem not available"); return 2; }
            std::string err;
            if (fs->AppendFile(path, std::string(content, len), err)) { lua_pushboolean(Lx, 1); return 1; }
            lua_pushboolean(Lx, 0); lua_pushstring(Lx, err.c_str()); return 2;
        };
        lua_pushcfunction(L, fn);
    }
    lua_setfield(L, -2, "append");

    // file.exists(path) → bool
    {
        lua_CFunction fn = [](lua_State* Lx) -> int {
            const char* path = luaL_checkstring(Lx, 1);
            auto* fs = GetDefaultFileSystem().get();
            lua_pushboolean(Lx, (fs && fs->FileExists(path)) ? 1 : 0);
            return 1;
        };
        lua_pushcfunction(L, fn);
    }
    lua_setfield(L, -2, "exists");

    // file.delete(path) → true | false, err
    {
        lua_CFunction fn = [](lua_State* Lx) -> int {
            const char* path = luaL_checkstring(Lx, 1);
            auto* fs = GetDefaultFileSystem().get();
            if (!fs) { lua_pushboolean(Lx, 0); lua_pushstring(Lx, "FileSystem not available"); return 2; }
            std::string err;
            if (fs->DeleteFile(path, false, err)) { lua_pushboolean(Lx, 1); return 1; }
            lua_pushboolean(Lx, 0); lua_pushstring(Lx, err.c_str()); return 2;
        };
        lua_pushcfunction(L, fn);
    }
    lua_setfield(L, -2, "delete");

    // file.size(path) → int64
    {
        lua_CFunction fn = [](lua_State* Lx) -> int {
            const char* path = luaL_checkstring(Lx, 1);
            auto* fs = GetDefaultFileSystem().get();
            lua_pushinteger(Lx, fs ? static_cast<lua_Integer>(fs->GetFileSize(path)) : -1);
            return 1;
        };
        lua_pushcfunction(L, fn);
    }
    lua_setfield(L, -2, "size");

    // file.listdir(path [, pattern]) → {name,...} | nil, err
    {
        lua_CFunction fn = [](lua_State* Lx) -> int {
            const char* path    = luaL_checkstring(Lx, 1);
            const char* pattern = luaL_optstring(Lx, 2, "");
            auto* fs = GetDefaultFileSystem().get();
            if (!fs) { lua_pushnil(Lx); lua_pushstring(Lx, "FileSystem not available"); return 2; }
            std::vector<std::string> names;
            std::string err;
            if (!fs->ListDir(path, pattern, names, err)) {
                lua_pushnil(Lx); lua_pushstring(Lx, err.c_str()); return 2;
            }
            lua_createtable(Lx, static_cast<int>(names.size()), 0);
            for (int i = 0; i < static_cast<int>(names.size()); ++i) {
                lua_pushstring(Lx, names[i].c_str());
                lua_rawseti(Lx, -2, i + 1);
            }
            return 1;
        };
        lua_pushcfunction(L, fn);
    }
    lua_setfield(L, -2, "listdir");

    // file.mkdir(path) → true | false, err
    {
        lua_CFunction fn = [](lua_State* Lx) -> int {
            const char* path = luaL_checkstring(Lx, 1);
            auto* fs = GetDefaultFileSystem().get();
            if (!fs) { lua_pushboolean(Lx, 0); lua_pushstring(Lx, "FileSystem not available"); return 2; }
            std::string err;
            if (fs->MakeDir(path, err)) { lua_pushboolean(Lx, 1); return 1; }
            lua_pushboolean(Lx, 0); lua_pushstring(Lx, err.c_str()); return 2;
        };
        lua_pushcfunction(L, fn);
    }
    lua_setfield(L, -2, "mkdir");

    // file.basename(path) → string
    {
        lua_CFunction fn = [](lua_State* Lx) -> int {
            const char* path = luaL_checkstring(Lx, 1);
            auto* fs = GetDefaultFileSystem().get();
            lua_pushstring(Lx, fs ? fs->GetBaseName(path).c_str() : "");
            return 1;
        };
        lua_pushcfunction(L, fn);
    }
    lua_setfield(L, -2, "basename");

    // file.dirname(path) → string
    {
        lua_CFunction fn = [](lua_State* Lx) -> int {
            const char* path = luaL_checkstring(Lx, 1);
            auto* fs = GetDefaultFileSystem().get();
            lua_pushstring(Lx, fs ? fs->GetDirName(path).c_str() : "");
            return 1;
        };
        lua_pushcfunction(L, fn);
    }
    lua_setfield(L, -2, "dirname");

    // file.join(base, part) → string
    {
        lua_CFunction fn = [](lua_State* Lx) -> int {
            const char* base = luaL_checkstring(Lx, 1);
            const char* part = luaL_checkstring(Lx, 2);
            auto* fs = GetDefaultFileSystem().get();
            lua_pushstring(Lx, fs ? fs->JoinPath(base, part).c_str() : "");
            return 1;
        };
        lua_pushcfunction(L, fn);
    }
    lua_setfield(L, -2, "join");

    lua_setglobal(L, "file");
}

} // namespace Runtime
} // namespace NodeEditor

#endif // BLUEPRINT_HAS_LUA
