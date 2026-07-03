// godot_file_bridge.cpp - implementation of the Godot FileAccess <-> BlueprintRuntime bridge.

#include "godot_file_bridge.h"

#include <godot_cpp/classes/file_access.hpp>
#include <godot_cpp/classes/project_settings.hpp>
#include <godot_cpp/variant/utility_functions.hpp>
#include <godot_cpp/variant/packed_byte_array.hpp>

#include <cstdlib>
#include <cstring>
#include <string>

// 见 blueprint_node.cpp / game_launcher.cpp 同处注释：不要再套 extern "C"。
#include "BlueprintCAPI.h"

using namespace godot;

namespace {

// Path prefix prepended to relative paths the engine requests.
std::string g_res_root;

// Normalize an engine-supplied path into a Godot-loadable path.
// Rules:
//   - already a Godot virtual path (res:// / user://) -> use as-is
//   - absolute OS path (has drive letter or leading slash) -> use as-is
//     (FileAccess can open absolute OS paths in non-export builds)
//   - otherwise treat as relative to g_res_root
String to_godot_path(const char *raw) {
    String p = String::utf8(raw);
    if (p.begins_with("res://") || p.begins_with("user://")) {
        return p;
    }
    // crude absolute-path detection: "X:/..." or "/..."
    if (p.length() >= 2 && p[1] == ':') return p;
    if (p.begins_with("/")) return p;

    if (g_res_root.empty()) return p;
    String root = String::utf8(g_res_root.c_str());
    if (!root.ends_with("/")) root += "/";
    // strip any leading "./"
    while (p.begins_with("./")) p = p.substr(2);
    return root + p;
}

// --- C callbacks handed to BP_SetFileReader -------------------------------

int BLUEPRINT_CAPI_CALL bridge_read(const char *path, char **outData, int *outSize, void * /*ud*/) {
    String gp = to_godot_path(path);
    if (!FileAccess::file_exists(gp)) {
        // Try the raw path as a fallback (covers absolute OS paths in editor).
        gp = String::utf8(path);
        if (!FileAccess::file_exists(gp)) {
            return 0;
        }
    }
    Ref<FileAccess> f = FileAccess::open(gp, FileAccess::READ);
    if (f.is_null()) {
        UtilityFunctions::printerr(String("[FileBridge] open NULL: ") + gp);
        return 0;
    }
    int64_t len = f->get_length();
    PackedByteArray bytes = f->get_buffer(len);
    f->close();
    UtilityFunctions::print(String("[FileBridge] read ") + gp +
                            " len=" + String::num_int64(len) +
                            " got=" + String::num_int64(bytes.size()));

    int n = static_cast<int>(bytes.size());
    char *buf = static_cast<char *>(std::malloc(n > 0 ? n : 1));
    if (!buf) return 0;
    if (n > 0) std::memcpy(buf, bytes.ptr(), static_cast<size_t>(n));
    *outData = buf;
    *outSize = n;
    return 1;
}

void BLUEPRINT_CAPI_CALL bridge_free(char *data, void * /*ud*/) {
    std::free(data);
}

int BLUEPRINT_CAPI_CALL bridge_exists(const char *path, void * /*ud*/) {
    String gp = to_godot_path(path);
    if (FileAccess::file_exists(gp)) return 1;
    return FileAccess::file_exists(String::utf8(path)) ? 1 : 0;
}

} // namespace

void install_godot_file_reader(const char *res_root) {
    g_res_root = (res_root && *res_root) ? res_root : "";
    BP_SetFileReader(bridge_read, bridge_free, bridge_exists, nullptr);
    UtilityFunctions::print(String("[FileBridge] installed, res_root='") +
                            String::utf8(g_res_root.c_str()) + "'");
}

void uninstall_godot_file_reader() {
    BP_SetFileReader(nullptr, nullptr, nullptr, nullptr);
}
