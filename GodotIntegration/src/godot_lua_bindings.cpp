// godot_lua_bindings.cpp - project-layer Scene.*/UI.* Lua bindings.
// Engine is untouched: we grab the lua_State from BP_GetLuaState and drive the
// Lua C API + Godot directly here.

#include "godot_lua_bindings.h"

#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/classes/scene_tree.hpp>
#include <godot_cpp/classes/window.hpp>
#include <godot_cpp/classes/control.hpp>
#include <godot_cpp/classes/button.hpp>
#include <godot_cpp/classes/label.hpp>
#include <godot_cpp/classes/line_edit.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/callable.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

extern "C" {
#include "BlueprintCAPI.h"   // BP_GetLuaState only
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
}

#include <string>
#include <unordered_map>

using namespace godot;

// ---------------------------------------------------------------------------
// Click handler registry: "panel\0node" -> Lua registry ref, per lua_State.
// Defined at file scope (external linkage within this TU) so both the anonymous
// namespace helpers AND GdUiClickRelay::_on_pressed can reach it.
// ---------------------------------------------------------------------------
struct ClickTable { std::unordered_map<std::string, int> refs; };

static ClickTable *clickTable(lua_State *L, bool create) {
    lua_getfield(L, LUA_REGISTRYINDEX, "__godot_ui_clicks");
    ClickTable *t = static_cast<ClickTable *>(lua_touserdata(L, -1));
    lua_pop(L, 1);
    if (!t && create) {
        t = new ClickTable();
        lua_pushlightuserdata(L, t);
        lua_setfield(L, LUA_REGISTRYINDEX, "__godot_ui_clicks");
    }
    return t;
}

// ---------------------------------------------------------------------------
// GdUiClickRelay: forwards a Godot Button "pressed" signal -> stored Lua fn.
// Class is REGISTERED AT MODULE INIT (register_types.cpp), never at runtime.
// ---------------------------------------------------------------------------
void GdUiClickRelay::_on_pressed() {
    if (!state) return;
    ClickTable *t = clickTable(state, false);
    if (!t) return;
    auto it = t->refs.find(key);
    if (it == t->refs.end()) return;
    lua_rawgeti(state, LUA_REGISTRYINDEX, it->second);
    if (lua_isfunction(state, -1)) {
        if (lua_pcall(state, 0, 0, 0) != LUA_OK) lua_pop(state, 1);
    } else {
        lua_pop(state, 1);
    }
}

void GdUiClickRelay::_bind_methods() {
    ClassDB::bind_method(D_METHOD("_on_pressed"), &GdUiClickRelay::_on_pressed);
}

// ---------------------------------------------------------------------------
// GdUiPanelReadyRelay: one-shot forwarder for UiService.panel_ready → Lua cb.
// ---------------------------------------------------------------------------
void GdUiPanelReadyRelay::_on_panel_ready(const String &panel_id, const Variant &panel) {
    // Only react to the panel we are waiting for (one panel_ready signal is
    // shared by all concurrent async loads).
    if (String::utf8(want_id.c_str()) != panel_id) return;

    bool ok = (panel.get_type() != Variant::NIL) &&
              (Object::cast_to<Object>(panel) != nullptr);
    if (state && lua_ref >= 0) {
        lua_rawgeti(state, LUA_REGISTRYINDEX, lua_ref);
        if (lua_isfunction(state, -1)) {
            lua_pushboolean(state, ok ? 1 : 0);
            if (lua_pcall(state, 1, 0, 0) != LUA_OK) lua_pop(state, 1);
        } else {
            lua_pop(state, 1);
        }
        luaL_unref(state, LUA_REGISTRYINDEX, lua_ref);
        lua_ref = -1;
    }
    // One-shot: disconnect + free self next idle.
    queue_free();
}

void GdUiPanelReadyRelay::_bind_methods() {
    ClassDB::bind_method(D_METHOD("_on_panel_ready", "panel_id", "panel"),
                         &GdUiPanelReadyRelay::_on_panel_ready);
}

// ---------------------------------------------------------------------------
// Process-level context: which Node to reach autoloads through, and the current
// lua_State whose UI.on_click handlers we dispatch to.
// ---------------------------------------------------------------------------
namespace {

Node      *g_ctx = nullptr;
lua_State *g_L   = nullptr;

SceneTree *tree() { return g_ctx ? g_ctx->get_tree() : nullptr; }

Node *autoload(const char *name) {
    SceneTree *t = tree();
    if (!t || !t->get_root()) return nullptr;
    return t->get_root()->get_node_or_null(NodePath(String("/root/") + name));
}

Node *panel_child(const char *panel_id, const char *rel) {
    Node *ui = autoload("UiService");
    if (!ui) return nullptr;
    Variant pv = ui->call("get_panel", String::utf8(panel_id));
    Node *panel = Object::cast_to<Node>(pv);
    if (!panel) return nullptr;
    if (!rel || !*rel) return panel;
    return panel->get_node_or_null(NodePath(String::utf8(rel)));
}

std::string clickKey(const char *p, const char *n) {
    std::string k = p ? p : ""; k.push_back('\0'); k += n ? n : ""; return k;
}

// ---- Scene.* ----
int l_scene_change(lua_State *L) {
    const char *p = luaL_checkstring(L, 1);
    Node *s = autoload("SceneService");
    bool ok = s && (bool)s->call("change_scene", String::utf8(p));
    lua_pushboolean(L, ok); return 1;
}
int l_scene_additive(lua_State *L) {
    const char *p = luaL_checkstring(L, 1);
    Node *s = autoload("SceneService");
    bool ok = false;
    if (s) { Variant r = s->call("load_additive", String::utf8(p), Variant()); ok = Object::cast_to<Node>(r) != nullptr; }
    lua_pushboolean(L, ok); return 1;
}
int l_scene_unload(lua_State *L) {
    const char *p = luaL_checkstring(L, 1);
    Node *s = autoload("SceneService");
    if (s) s->call("unload", String::utf8(p));
    return 0;
}
int l_scene_instance(lua_State *L) {
    const char *p = luaL_checkstring(L, 1);
    Node *s = autoload("SceneService");
    int64_t id = 0;
    if (s) { Variant r = s->call("instance_scene", String::utf8(p)); Node *n = Object::cast_to<Node>(r); if (n) id = (int64_t)n->get_instance_id(); }
    lua_pushinteger(L, (lua_Integer)id); return 1;
}
int l_scene_preload(lua_State *L) {
    const char *p = luaL_checkstring(L, 1);
    Node *s = autoload("SceneService");
    bool ok = false;
    if (s) { Variant r = s->call("preload_resource", String::utf8(p)); ok = r.get_type() != Variant::NIL; }
    lua_pushboolean(L, ok); return 1;
}

// ---- UI.* ----
int l_ui_open(lua_State *L) {
    const char *id = luaL_checkstring(L, 1);
    Node *ui = autoload("UiService");
    bool ok = false;
    if (ui) { Variant r = ui->call("open_panel", String::utf8(id), String()); ok = Object::cast_to<Node>(r) != nullptr; }
    lua_pushboolean(L, ok); return 1;
}
// UI.open_panel_async(panel_id, callback)  -- callback(ok:boolean) 在加载完成后调用
int l_ui_open_async(lua_State *L) {
    const char *id = luaL_checkstring(L, 1);
    luaL_checktype(L, 2, LUA_TFUNCTION);
    Node *ui = autoload("UiService");
    if (!ui) {
        // 无 UiService：立即回调 false
        lua_pushvalue(L, 2);
        if (lua_isfunction(L, -1)) { lua_pushboolean(L, 0); if (lua_pcall(L, 1, 0, 0) != LUA_OK) lua_pop(L, 1); }
        return 0;
    }
    // 存 Lua 回调到注册表
    lua_pushvalue(L, 2);
    int ref = luaL_ref(L, LUA_REGISTRYINDEX);

    // 建一个一次性中继，连到 UiService.panel_ready，匹配本 panel_id 后回调 Lua。
    GdUiPanelReadyRelay *relay = memnew(GdUiPanelReadyRelay);
    relay->want_id = id;
    relay->lua_ref = ref;
    relay->state = L;
    ui->add_child(relay);
    // NOT one-shot: multiple concurrent async loads share one panel_ready signal;
    // each relay ignores emissions whose panel_id != its want_id, and only
    // frees itself once its own panel arrives (see _on_panel_ready).
    ui->connect("panel_ready", Callable(relay, "_on_panel_ready"));

    // 发起异步加载（open_panel_async 内部走后台线程加载 + _process 轮询）
    ui->call("open_panel_async", String::utf8(id), String());
    return 0;
}
int l_ui_close(lua_State *L) {
    const char *id = luaL_checkstring(L, 1);
    Node *ui = autoload("UiService");
    if (ui) ui->call("close_panel", String::utf8(id));
    return 0;
}
int l_ui_set_text(lua_State *L) {
    const char *id = luaL_checkstring(L, 1);
    const char *node = luaL_checkstring(L, 2);
    const char *text = luaL_checkstring(L, 3);
    Node *n = panel_child(id, node);
    if (n) {
        if (Label *l = Object::cast_to<Label>(n)) l->set_text(String::utf8(text));
        else if (Button *b = Object::cast_to<Button>(n)) b->set_text(String::utf8(text));
        else if (LineEdit *e = Object::cast_to<LineEdit>(n)) e->set_text(String::utf8(text));
        else n->set("text", String::utf8(text));
    }
    return 0;
}
int l_ui_get_input(lua_State *L) {
    const char *id = luaL_checkstring(L, 1);
    const char *node = luaL_checkstring(L, 2);
    Node *n = panel_child(id, node);
    if (!n) { lua_pushnil(L); return 1; }
    String s;
    if (LineEdit *e = Object::cast_to<LineEdit>(n)) s = e->get_text();
    else if (Label *l = Object::cast_to<Label>(n)) s = l->get_text();
    else s = String(n->get("text"));
    CharString u = s.utf8();
    lua_pushlstring(L, u.get_data(), (size_t)u.length());
    return 1;
}
int l_ui_on_click(lua_State *L) {
    const char *id = luaL_checkstring(L, 1);
    const char *node = luaL_checkstring(L, 2);
    luaL_checktype(L, 3, LUA_TFUNCTION);

    ClickTable *t = clickTable(L, true);
    std::string key = clickKey(id, node);
    auto it = t->refs.find(key);
    if (it != t->refs.end()) luaL_unref(L, LUA_REGISTRYINDEX, it->second);
    lua_pushvalue(L, 3);
    t->refs[key] = luaL_ref(L, LUA_REGISTRYINDEX);

    Node *n = panel_child(id, node);
    Button *b = Object::cast_to<Button>(n);
    if (b) {
        GdUiClickRelay *relay = memnew(GdUiClickRelay);
        relay->key = key;
        relay->state = L;
        b->add_child(relay);
        b->connect("pressed", Callable(relay, "_on_pressed"));
    } else {
        UtilityFunctions::printerr(String("[GodotLua] on_click: not a Button: ") +
                                   String::utf8(id) + "/" + String::utf8(node));
    }
    return 0;
}

} // namespace

void register_godot_lua_bindings(void *runner, Node *ctx) {
    g_ctx = ctx;
    lua_State *L = BP_GetLuaState(static_cast<BP_Runner>(runner));
    if (L == nullptr) {
        UtilityFunctions::printerr("[GodotLua] BP_GetLuaState returned null (no Lua VM yet)");
        return;
    }
    g_L = L;

    // Scene table
    lua_newtable(L);
    lua_pushcfunction(L, l_scene_change);   lua_setfield(L, -2, "change");
    lua_pushcfunction(L, l_scene_change);   lua_setfield(L, -2, "load");
    lua_pushcfunction(L, l_scene_additive); lua_setfield(L, -2, "load_additive");
    lua_pushcfunction(L, l_scene_unload);   lua_setfield(L, -2, "unload");
    lua_pushcfunction(L, l_scene_instance); lua_setfield(L, -2, "instance");
    lua_pushcfunction(L, l_scene_preload);  lua_setfield(L, -2, "preload");
    lua_setglobal(L, "Scene");

    // UI table
    lua_newtable(L);
    lua_pushcfunction(L, l_ui_open);       lua_setfield(L, -2, "open_panel");
    lua_pushcfunction(L, l_ui_open_async); lua_setfield(L, -2, "open_panel_async");
    lua_pushcfunction(L, l_ui_close);      lua_setfield(L, -2, "close_panel");
    lua_pushcfunction(L, l_ui_set_text);  lua_setfield(L, -2, "set_text");
    lua_pushcfunction(L, l_ui_get_input); lua_setfield(L, -2, "get_input_text");
    lua_pushcfunction(L, l_ui_on_click);  lua_setfield(L, -2, "on_click");
    lua_setglobal(L, "UI");

    UtilityFunctions::print("[GodotLua] Scene/UI Lua bindings registered on this VM");
}
