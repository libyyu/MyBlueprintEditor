// blueprint_node.cpp - implementation of the BlueprintRuntime GDExtension binding.

#include "blueprint_node.h"
#include "godot_file_bridge.h"

#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/project_settings.hpp>
#include <godot_cpp/classes/file_access.hpp>
#include <godot_cpp/variant/utility_functions.hpp>
#include <godot_cpp/variant/packed_byte_array.hpp>

#include <string>
#include <vector>

// The plain-C BlueprintRuntime API. Path is relative to the project Runtime/ dir;
// the include dir is wired up in CMakeLists.txt.
extern "C" {
#include "BlueprintCAPI.h"
}

using namespace godot;

// ---------------------------------------------------------------------------
// Output callbacks: forward BlueprintRuntime PrintString/Log output to Godot.
// Both BP_SetPrintCallback and BP_SetLogCallback take BP_LogCallback, i.e.
// void(BP_LogLevel, const char*). We use one trampoline for both.
// ---------------------------------------------------------------------------
static void bp_output_trampoline(BP_LogLevel level, const char *message) {
    if (!message) return;
    // The engine emits UTF-8. godot-cpp's String(const char*) decodes as Latin-1,
    // which mojibakes Chinese node names. Use String::utf8() to decode correctly.
    String m = String("[BP] ") + String::utf8(message);
    if (level >= BP_LOG_ERROR) {
        UtilityFunctions::printerr(m);
    } else {
        UtilityFunctions::print(m);
    }
}

// Install the Godot FileAccess-backed reader exactly once per process, so all
// blueprint loads (and their auto-resolved dependencies) go through Godot's VFS.
static bool s_file_reader_installed = false;

static void ensure_file_reader() {
    if (s_file_reader_installed) return;
    // Empty res_root: BlueprintNode passes already-qualified res:// paths, and
    // dependency paths are resolved relative to the blueprint dir we set via
    // BP_SetBasePath, which the bridge keeps in Godot path form.
    install_godot_file_reader("");
    s_file_reader_installed = true;
}

// Parent directory (Godot path form) of a blueprint path, used as base dir for
// dependency resolution.
static String parent_dir(const String &path) {
    int slash = path.rfind("/");
    if (slash < 0) return String();
    return path.substr(0, slash);
}

BlueprintNode::BlueprintNode() {}

BlueprintNode::~BlueprintNode() {
    if (_runner) {
        BP_DestroyRunner(static_cast<BP_Runner>(_runner));
        _runner = nullptr;
    }
}

void BlueprintNode::_ready() {
    // Don't run inside the editor (only in a running game/scene).
    if (Engine::get_singleton()->is_editor_hint()) {
        return;
    }

    BP_InitDefaultHttpClient();
    ensure_file_reader(); // route all reads through Godot FileAccess (res://, web, pck)

    _runner = static_cast<void *>(BP_CreateRunner());
    if (!_runner) {
        UtilityFunctions::printerr("[BlueprintNode] BP_CreateRunner failed (OOM?)");
        return;
    }

    BP_Runner r = static_cast<BP_Runner>(_runner);
    BP_SetPrintCallback(r, bp_output_trampoline);
    BP_SetLogCallback(r, bp_output_trampoline);

    if (!_blueprint_path.is_empty()) {
        load_blueprint(_blueprint_path);
        if (_autorun && _loaded) {
            execute();
        }
    }
}

void BlueprintNode::_process(double delta) {
    if (Engine::get_singleton()->is_editor_hint()) return;
    if (!_runner || !_tick_enabled) return;

    BP_Runner r = static_cast<BP_Runner>(_runner);
    // Process async results posted from background threads, then advance timers.
    BP_DrainQueue();
    BP_Tick(r, static_cast<float>(delta));
}

void BlueprintNode::_exit_tree() {
    if (_runner) {
        BP_DestroyRunner(static_cast<BP_Runner>(_runner));
        _runner = nullptr;
        _loaded = false;
    }
}

bool BlueprintNode::load_blueprint(const String &path) {
    if (!_runner) return false;
    BP_Runner r = static_cast<BP_Runner>(_runner);

    // With the Godot file reader installed, BP_LoadFromFile reads the blueprint
    // AND auto-loads metadata.dependencies — all through Godot FileAccess.
    // We keep the path in Godot form (res://...) so it works on web/pck too.
    // BP_LoadFromFile internally calls BP_SetBasePath(parent dir), so relative
    // dependency paths resolve against the blueprint's own folder.
    int rc = BP_LoadFromFile(r, path.utf8().get_data());
    _loaded = (rc == 0);
    if (!_loaded) {
        UtilityFunctions::printerr(String("[BlueprintNode] load failed: ") + get_last_error());
        return false;
    }

    // Report resolved dependencies for visibility.
    UtilityFunctions::print(String("[BlueprintNode] loaded: ") + path);
    return _loaded;
}

bool BlueprintNode::load_from_json(const String &json) {
    if (!_runner) return false;
    BP_Runner r = static_cast<BP_Runner>(_runner);
    int rc = BP_LoadFromJson(r, json.utf8().get_data());
    _loaded = (rc == 0);
    if (!_loaded) {
        UtilityFunctions::printerr(String("[BlueprintNode] load_from_json failed: ") + get_last_error());
    }
    return _loaded;
}

bool BlueprintNode::execute() {
    if (!_runner || !_loaded) return false;
    BP_Runner r = static_cast<BP_Runner>(_runner);
    int rc = BP_ExecuteAll(r); // Execute + dispatch OnBeginPlay
    if (rc != 0) {
        UtilityFunctions::printerr(String("[BlueprintNode] execute failed: ") + get_last_error());
        return false;
    }
    return true;
}

bool BlueprintNode::dispatch_event(const String &event_id) {
    if (!_runner || !_loaded) return false;
    BP_Runner r = static_cast<BP_Runner>(_runner);
    return BP_DispatchEvent(r, event_id.utf8().get_data()) == 0;
}

void BlueprintNode::tick(double delta) {
    if (!_runner) return;
    BP_Runner r = static_cast<BP_Runner>(_runner);
    BP_DrainQueue();
    BP_Tick(r, static_cast<float>(delta));
}

bool BlueprintNode::is_loaded() const { return _loaded; }

String BlueprintNode::get_last_error() const {
    if (!_runner) return String("no runner");
    BP_Runner r = static_cast<BP_Runner>(_runner);
    char buf[1024] = {0};
    BP_GetLastError(r, buf, sizeof(buf));
    return String::utf8(buf);
}

PackedStringArray BlueprintNode::get_dependencies(const String &path) const {
    PackedStringArray deps;
    // Read the blueprint bytes through the same Godot reader the engine uses.
    Ref<FileAccess> f = FileAccess::open(path, FileAccess::READ);
    if (f.is_null()) {
        UtilityFunctions::printerr(String("[BlueprintNode] get_dependencies: cannot open ") + path);
        return deps;
    }
    PackedByteArray bytes = f->get_buffer(f->get_length());
    f->close();
    bytes.append(0); // NUL-terminate for the C API

    BP_Meta meta = BP_MetaParseFromJson(reinterpret_cast<const char *>(bytes.ptr()));
    if (!meta) return deps;

    int n = BP_MetaGetDependencyCount(meta);
    for (int i = 0; i < n; ++i) {
        char buf[512] = {0};
        BP_MetaGetDependency(meta, i, buf, sizeof(buf));
        deps.append(String::utf8(buf));
    }
    BP_MetaFree(meta);
    return deps;
}

// --- variables ---
void BlueprintNode::set_var_int(const String &name, int64_t value) {
    if (_runner) BP_SetVariableInt(static_cast<BP_Runner>(_runner), name.utf8().get_data(), value);
}
int64_t BlueprintNode::get_var_int(const String &name) const {
    if (!_runner) return 0;
    return BP_GetVariableInt(static_cast<BP_Runner>(_runner), name.utf8().get_data());
}
void BlueprintNode::set_var_float(const String &name, double value) {
    if (_runner) BP_SetVariableFloat(static_cast<BP_Runner>(_runner), name.utf8().get_data(), value);
}
double BlueprintNode::get_var_float(const String &name) const {
    if (!_runner) return 0.0;
    return BP_GetVariableFloat(static_cast<BP_Runner>(_runner), name.utf8().get_data());
}
void BlueprintNode::set_var_string(const String &name, const String &value) {
    if (_runner) BP_SetVariableString(static_cast<BP_Runner>(_runner), name.utf8().get_data(), value.utf8().get_data());
}
String BlueprintNode::get_var_string(const String &name) const {
    if (!_runner) return String();
    char buf[2048] = {0};
    int n = BP_GetVariableString(static_cast<BP_Runner>(_runner), name.utf8().get_data(), buf, sizeof(buf));
    if (n < 0) return String();
    return String::utf8(buf);
}
void BlueprintNode::set_var_bool(const String &name, bool value) {
    if (_runner) BP_SetVariableBool(static_cast<BP_Runner>(_runner), name.utf8().get_data(), value ? 1 : 0);
}
bool BlueprintNode::get_var_bool(const String &name) const {
    if (!_runner) return false;
    return BP_GetVariableBool(static_cast<BP_Runner>(_runner), name.utf8().get_data()) != 0;
}

// --- properties ---
void BlueprintNode::set_blueprint_path(const String &p) { _blueprint_path = p; }
String BlueprintNode::get_blueprint_path() const { return _blueprint_path; }
void BlueprintNode::set_autorun(bool v) { _autorun = v; }
bool BlueprintNode::get_autorun() const { return _autorun; }
void BlueprintNode::set_tick_enabled(bool v) { _tick_enabled = v; }
bool BlueprintNode::get_tick_enabled() const { return _tick_enabled; }

// ---------------------------------------------------------------------------
// Bindings
// ---------------------------------------------------------------------------
void BlueprintNode::_bind_methods() {
    ClassDB::bind_method(D_METHOD("load_blueprint", "path"), &BlueprintNode::load_blueprint);
    ClassDB::bind_method(D_METHOD("load_from_json", "json"), &BlueprintNode::load_from_json);
    ClassDB::bind_method(D_METHOD("execute"), &BlueprintNode::execute);
    ClassDB::bind_method(D_METHOD("dispatch_event", "event_id"), &BlueprintNode::dispatch_event);
    ClassDB::bind_method(D_METHOD("tick", "delta"), &BlueprintNode::tick);
    ClassDB::bind_method(D_METHOD("is_loaded"), &BlueprintNode::is_loaded);
    ClassDB::bind_method(D_METHOD("get_last_error"), &BlueprintNode::get_last_error);
    ClassDB::bind_method(D_METHOD("get_dependencies", "path"), &BlueprintNode::get_dependencies);

    ClassDB::bind_method(D_METHOD("set_var_int", "name", "value"), &BlueprintNode::set_var_int);
    ClassDB::bind_method(D_METHOD("get_var_int", "name"), &BlueprintNode::get_var_int);
    ClassDB::bind_method(D_METHOD("set_var_float", "name", "value"), &BlueprintNode::set_var_float);
    ClassDB::bind_method(D_METHOD("get_var_float", "name"), &BlueprintNode::get_var_float);
    ClassDB::bind_method(D_METHOD("set_var_string", "name", "value"), &BlueprintNode::set_var_string);
    ClassDB::bind_method(D_METHOD("get_var_string", "name"), &BlueprintNode::get_var_string);
    ClassDB::bind_method(D_METHOD("set_var_bool", "name", "value"), &BlueprintNode::set_var_bool);
    ClassDB::bind_method(D_METHOD("get_var_bool", "name"), &BlueprintNode::get_var_bool);

    // Inspector-editable properties
    ClassDB::bind_method(D_METHOD("set_blueprint_path", "path"), &BlueprintNode::set_blueprint_path);
    ClassDB::bind_method(D_METHOD("get_blueprint_path"), &BlueprintNode::get_blueprint_path);
    ADD_PROPERTY(PropertyInfo(Variant::STRING, "blueprint_path", PROPERTY_HINT_FILE, "*.bjson"),
                 "set_blueprint_path", "get_blueprint_path");

    ClassDB::bind_method(D_METHOD("set_autorun", "v"), &BlueprintNode::set_autorun);
    ClassDB::bind_method(D_METHOD("get_autorun"), &BlueprintNode::get_autorun);
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "autorun"), "set_autorun", "get_autorun");

    ClassDB::bind_method(D_METHOD("set_tick_enabled", "v"), &BlueprintNode::set_tick_enabled);
    ClassDB::bind_method(D_METHOD("get_tick_enabled"), &BlueprintNode::get_tick_enabled);
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "tick_enabled"), "set_tick_enabled", "get_tick_enabled");
}
