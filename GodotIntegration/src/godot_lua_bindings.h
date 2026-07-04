// godot_lua_bindings.h - PROJECT-LAYER Lua bindings for Scene / UI.
//
// IMPORTANT: this lives entirely in the GDExtension (business) layer. The
// BlueprintRuntime engine is NOT modified — we obtain the runner's lua_State via
// the engine's existing public C API `BP_GetLuaState(runner)` and register the
// `Scene.*` / `UI.*` global tables ourselves, forwarding to Godot's
// SceneService / UiService autoloads.
//
// Lua ⇄ Godot event flow (button click): we connect the Godot Button "pressed"
// signal to a relay that calls back into the stored Lua handler on this state.

#ifndef GODOT_LUA_BINDINGS_H
#define GODOT_LUA_BINDINGS_H

#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/variant.hpp>

#include <string>

struct lua_State;

namespace godot {
    
// A relay Node bound per click subscription; forwards a Godot Button "pressed"
// signal back into the stored Lua handler. MUST be registered as a GDExtension
// class at module init time (see register_types.cpp) — registering a class at
// runtime crashes Godot.
class GdUiClickRelay : public Node {
    GDCLASS(GdUiClickRelay, Node)
public:
    std::string key;             // "panel\0node"
    lua_State  *state = nullptr; // the VM whose click table holds the handler
    void _on_pressed();
protected:
    static void _bind_methods();
};

// One-shot relay for UiService.panel_ready(panel_id, panel). Forwards the async
// open result to the stored Lua callback (called with a boolean = success), then
// frees itself. ALSO registered at module init time (register_types.cpp).
class GdUiPanelReadyRelay : public Node {
    GDCLASS(GdUiPanelReadyRelay, Node)
public:
    std::string want_id;         // panel_id we are waiting for
    int         lua_ref = -1;    // LUA_NOREF-style ref to the Lua callback
    lua_State  *state = nullptr;
    void _on_panel_ready(const String &panel_id, const Variant &panel);
protected:
    static void _bind_methods();
};

// Relay for InputService.action_triggered(action, edge). Forwards a matching
// action's "pressed" edge into a stored Lua handler. Registered at module init.
class GdInputActionRelay : public Node {
    GDCLASS(GdInputActionRelay, Node)
public:
    std::string want_action;     // the action name this relay listens for
    int         lua_ref = -1;    // ref to the Lua callback fn(action)
    lua_State  *state = nullptr;
    void _on_action(const String &action, const String &edge);
protected:
    static void _bind_methods();
};

// Relay for widget value/text change signals. Forwards the new value (as a
// string) into a stored Lua handler fn(value). Registered at module init.
class GdUiValueRelay : public Node {
    GDCLASS(GdUiValueRelay, Node)
public:
    int         lua_ref = -1;    // ref to the Lua callback fn(value_string)
    lua_State  *state = nullptr;
    // LineEdit.text_changed(new_text: String)
    void _on_text_changed(const String &new_text);
    // Range.value_changed(value: double)  /  CheckBox.toggled(pressed: bool)
    void _on_value_changed(double value);
    void _on_toggled(bool pressed);
protected:
    static void _bind_methods();
};

// Relay for TimerService callbacks. TimerService.after/every take a Callable;
// we bind this relay's _fire as the Callable. On fire it invokes the stored Lua
// function; for one-shot timers it frees the Lua ref (and self) after firing.
// Registered at module init.
class GdTimerRelay : public Node {
    GDCLASS(GdTimerRelay, Node)
public:
    int         lua_ref = -1;    // ref to the Lua callback fn()
    lua_State  *state = nullptr;
    bool        once = false;    // one-shot: unref + free after first fire
    void _fire();
protected:
    static void _bind_methods();
};

} // namespace godot

// Register Scene.*/UI.* into the given runner's Lua VM. `runner` is BP_Runner
// (void*). `ctx` is any Node in the tree (to reach autoloads). Call once after
// the runner's Lua VM exists (e.g. right after load_blueprint / first Lua use).
// Safe to call again for a fresh VM (after BP_ResetSharedLuaVM).
void register_godot_lua_bindings(void *luaEnv, godot::Node *ctx);

#endif // GODOT_LUA_BINDINGS_H
