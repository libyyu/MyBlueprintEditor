// godot_lua_loader.cpp - Godot FileAccess-backed Lua module resolver.

#include "godot_lua_loader.h"

#include <godot_cpp/classes/file_access.hpp>
#include <godot_cpp/variant/utility_functions.hpp>
#include <godot_cpp/variant/packed_byte_array.hpp>

#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

// 见 blueprint_node.cpp / game_launcher.cpp 同处注释：不要再套 extern "C"。
#include "BlueprintCAPI.h"

using namespace godot;

namespace {

// Ordered root prefixes, configured at install time (process-global, like the
// engine's single Lua VM). e.g. {"res://lua", "res://blueprints"}.
std::vector<std::string> g_roots;

// modname "ui.FGUIMan" -> "ui/FGUIMan.lua"
String module_to_rel(const char *modname) {
    String m = String::utf8(modname);
    String out;
    for (int i = 0; i < m.length(); ++i) {
        char32_t c = m[i];
        out += (c == U'.') ? String("/") : String::chr(c);
    }
    return out + ".lua";
}

char *dup_cstr(const String &s) {
    CharString u = s.utf8();
    int n = u.length();
    char *buf = static_cast<char *>(std::malloc(n + 1));
    if (!buf) return nullptr;
    std::memcpy(buf, u.get_data(), n);
    buf[n] = 0;
    return buf;
}

// --- C callbacks handed to BP_SetLuaModuleResolver ------------------------

int BLUEPRINT_CAPI_CALL resolve_module(const char *modname, char **outData,
                                       int *outSize, char **outChunk, void * /*ud*/) {
    String rel = module_to_rel(modname);
    for (const std::string &root : g_roots) {
        String full = String::utf8(root.c_str());
        if (!full.ends_with("/")) full += "/";
        full += rel;
        if (!FileAccess::file_exists(full)) continue;

        Ref<FileAccess> f = FileAccess::open(full, FileAccess::READ);
        if (f.is_null()) continue;
        PackedByteArray bytes = f->get_buffer(f->get_length());
        f->close();

        int n = static_cast<int>(bytes.size());
        char *buf = static_cast<char *>(std::malloc(n > 0 ? n : 1));
        if (!buf) return 0;
        if (n > 0) std::memcpy(buf, bytes.ptr(), static_cast<size_t>(n));
        *outData = buf;
        *outSize = n;
        if (outChunk) *outChunk = dup_cstr(full);
        return 1;
    }
    return 0; // not found in any root
}

void BLUEPRINT_CAPI_CALL free_module(char *ptr, void * /*ud*/) {
    std::free(ptr);
}

} // namespace

void install_godot_lua_loader(void *runner, const PackedStringArray &roots) {
    g_roots.clear();
    for (int i = 0; i < roots.size(); ++i) {
        g_roots.push_back(std::string(roots[i].utf8().get_data()));
    }
    BP_SetLuaModuleResolver(static_cast<BP_Runner>(runner), resolve_module, free_module, nullptr);
    UtilityFunctions::print(String("[LuaLoader] installed, roots=") + String::num_int64(roots.size()));
}
