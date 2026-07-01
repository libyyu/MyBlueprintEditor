// blueprint_node.h - GDExtension binding for BlueprintRuntime
//
// Wraps the BlueprintRuntime C API (Runtime/BlueprintCAPI.h) as a Godot Node.
// Drop a BlueprintNode into any scene, point it at a .bjson file, and it will:
//   - create a BPRunner in _ready()
//   - load + execute the blueprint
//   - call BP_Tick() every frame in _process()
//   - forward engine Print output to Godot's print()
//
// This is the Godot equivalent of UGFramework's BlueprintBehaviour (Unity).
// Unlike the Unity path there is NO shared lua_State juggling: the Lua VM lives
// entirely inside BlueprintRuntime (built with BLUEPRINT_LUA=ON).

#ifndef BLUEPRINT_NODE_H
#define BLUEPRINT_NODE_H

#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/packed_string_array.hpp>

// Opaque runner handle from the C API (void*). We keep it as void* here to avoid
// leaking the C header into Godot translation units; the .cpp includes the real header.
namespace godot {

class BlueprintNode : public Node {
    GDCLASS(BlueprintNode, Node)

private:
    void *_runner = nullptr;      // BP_Runner (void*)
    String _blueprint_path;       // res:// or absolute path to a .bjson
    bool _autorun = true;         // auto Execute on _ready
    bool _tick_enabled = true;    // call BP_Tick each frame
    bool _loaded = false;

protected:
    static void _bind_methods();

public:
    BlueprintNode();
    ~BlueprintNode();

    // Godot lifecycle
    void _ready() override;
    void _process(double delta) override;
    void _exit_tree() override;

    // --- exposed to GDScript ---
    // Load a blueprint from a file path (res:// is resolved to an OS path).
    bool load_blueprint(const String &path);
    // Load a blueprint directly from a JSON/bjson string.
    bool load_from_json(const String &json);
    // Run data-flow + dispatch OnBeginPlay.
    bool execute();
    // Dispatch a named event (e.g. "OnBeginPlay", "on_win").
    bool dispatch_event(const String &event_id);
    // Manual tick (normally automatic via _process).
    void tick(double delta);
    bool is_loaded() const;
    String get_last_error() const;

    // Load a Lua script file into this runner's VM (require-resolved, runs it).
    bool run_lua_file(const String &path);
    // Execute an inline Lua chunk in this runner's VM (project-layer helper).
    bool run_lua(const String &code);

    // Internal (C++ only): the raw BP_Runner handle, for project-layer code that
    // needs the runner's lua_State via BP_GetLuaState (e.g. godot_lua_bindings).
    void *runner_ptr() const { return _runner; }
    // Query the dependency paths declared in a blueprint file's metadata
    // (without loading it). Useful for verifying the resource graph.
    PackedStringArray get_dependencies(const String &path) const;

    // Variable get/set bridged to BP_*Variable* C API.
    void set_var_int(const String &name, int64_t value);
    int64_t get_var_int(const String &name) const;
    void set_var_float(const String &name, double value);
    double get_var_float(const String &name) const;
    void set_var_string(const String &name, const String &value);
    String get_var_string(const String &name) const;
    void set_var_bool(const String &name, bool value);
    bool get_var_bool(const String &name) const;

    // --- properties ---
    void set_blueprint_path(const String &p);
    String get_blueprint_path() const;
    void set_autorun(bool v);
    bool get_autorun() const;
    void set_tick_enabled(bool v);
    bool get_tick_enabled() const;
};

} // namespace godot

#endif // BLUEPRINT_NODE_H
