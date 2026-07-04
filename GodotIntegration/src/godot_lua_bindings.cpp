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
#include <godot_cpp/classes/range.hpp>
#include <godot_cpp/classes/check_box.hpp>
#include <godot_cpp/classes/check_button.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/callable.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

// BlueprintCAPI.h / lua.h 内部已用 __cplusplus + extern "C" 包裹自身声明。
// 此处禁止再包 extern "C" { ... }：Emscripten 下 BlueprintCAPI.h 会 include
// <emscripten.h>（含 C++ 模板），塞进 C 链接块会编译失败。
#include "BlueprintCAPI.h"   // BP_GetLuaState only
#include <lua.hpp>

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
// GdInputActionRelay: forwards InputService.action_triggered → Lua fn(action).
// ---------------------------------------------------------------------------
void GdInputActionRelay::_on_action(const String &action, const String &edge) {
    // Only the "pressed" edge, and only our action.
    if (edge != String("pressed")) return;
    if (String::utf8(want_action.c_str()) != action) return;
    if (!state || lua_ref < 0) return;
    lua_rawgeti(state, LUA_REGISTRYINDEX, lua_ref);
    if (lua_isfunction(state, -1)) {
        CharString a = action.utf8();
        lua_pushlstring(state, a.get_data(), (size_t)a.length());
        if (lua_pcall(state, 1, 0, 0) != LUA_OK) lua_pop(state, 1);
    } else {
        lua_pop(state, 1);
    }
}

void GdInputActionRelay::_bind_methods() {
    ClassDB::bind_method(D_METHOD("_on_action", "action", "edge"),
                         &GdInputActionRelay::_on_action);
}

// ---------------------------------------------------------------------------
// GdUiValueRelay: forwards a widget's value/text change → Lua fn(value_string).
// ---------------------------------------------------------------------------
static void dispatch_value(lua_State *state, int lua_ref, const String &value) {
    if (!state || lua_ref < 0) return;
    lua_rawgeti(state, LUA_REGISTRYINDEX, lua_ref);
    if (lua_isfunction(state, -1)) {
        CharString v = value.utf8();
        lua_pushlstring(state, v.get_data(), (size_t)v.length());
        if (lua_pcall(state, 1, 0, 0) != LUA_OK) lua_pop(state, 1);
    } else {
        lua_pop(state, 1);
    }
}
void GdUiValueRelay::_on_text_changed(const String &new_text) {
    dispatch_value(state, lua_ref, new_text);
}
void GdUiValueRelay::_on_value_changed(double value) {
    dispatch_value(state, lua_ref, String::num(value));
}
void GdUiValueRelay::_on_toggled(bool pressed) {
    dispatch_value(state, lua_ref, pressed ? String("true") : String("false"));
}

void GdUiValueRelay::_bind_methods() {
    ClassDB::bind_method(D_METHOD("_on_text_changed", "new_text"),
                         &GdUiValueRelay::_on_text_changed);
    ClassDB::bind_method(D_METHOD("_on_value_changed", "value"),
                         &GdUiValueRelay::_on_value_changed);
    ClassDB::bind_method(D_METHOD("_on_toggled", "pressed"),
                         &GdUiValueRelay::_on_toggled);
}

// ---------------------------------------------------------------------------
// GdTimerRelay: forwards a TimerService timer fire → Lua fn(). One-shot relays
// free their Lua ref + self after firing (matches Timer.after semantics).
// ---------------------------------------------------------------------------
void GdTimerRelay::_fire() {
    if (state && lua_ref >= 0) {
        lua_rawgeti(state, LUA_REGISTRYINDEX, lua_ref);
        if (lua_isfunction(state, -1)) {
            if (lua_pcall(state, 0, 0, 0) != LUA_OK) lua_pop(state, 1);
        } else {
            lua_pop(state, 1);
        }
        if (once) {
            luaL_unref(state, LUA_REGISTRYINDEX, lua_ref);
            lua_ref = -1;
            queue_free();
        }
    }
}

void GdTimerRelay::_bind_methods() {
    ClassDB::bind_method(D_METHOD("_fire"), &GdTimerRelay::_fire);
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

// UI.on_text_changed(panel, node, fn)  -- fn(new_text) 每次 LineEdit 文本变化
int l_ui_on_text_changed(lua_State *L) {
    const char *id = luaL_checkstring(L, 1);
    const char *node = luaL_checkstring(L, 2);
    luaL_checktype(L, 3, LUA_TFUNCTION);
    Node *n = panel_child(id, node);
    LineEdit *e = Object::cast_to<LineEdit>(n);
    if (!e) {
        UtilityFunctions::printerr(String("[GodotLua] on_text_changed: not a LineEdit: ") +
                                   String::utf8(id) + "/" + String::utf8(node));
        return 0;
    }
    lua_pushvalue(L, 3);
    int ref = luaL_ref(L, LUA_REGISTRYINDEX);
    GdUiValueRelay *relay = memnew(GdUiValueRelay);
    relay->state = L;
    relay->lua_ref = ref;
    e->add_child(relay);
    e->connect("text_changed", Callable(relay, "_on_text_changed"));
    return 0;
}

// UI.on_value_changed(panel, node, fn)  -- fn(value_string)
// 支持 Range(Slider/SpinBox/ProgressBar) 的 value_changed 与 CheckBox/CheckButton 的 toggled
int l_ui_on_value_changed(lua_State *L) {
    const char *id = luaL_checkstring(L, 1);
    const char *node = luaL_checkstring(L, 2);
    luaL_checktype(L, 3, LUA_TFUNCTION);
    Node *n = panel_child(id, node);
    if (!n) { return 0; }
    lua_pushvalue(L, 3);
    int ref = luaL_ref(L, LUA_REGISTRYINDEX);
    GdUiValueRelay *relay = memnew(GdUiValueRelay);
    relay->state = L;
    relay->lua_ref = ref;
    n->add_child(relay);
    if (Object::cast_to<Range>(n)) {
        n->connect("value_changed", Callable(relay, "_on_value_changed"));
    } else if (Object::cast_to<CheckBox>(n) || Object::cast_to<CheckButton>(n) ||
               Object::cast_to<Button>(n)) {
        n->connect("toggled", Callable(relay, "_on_toggled"));
    } else {
        UtilityFunctions::printerr(String("[GodotLua] on_value_changed: unsupported node: ") +
                                   String::utf8(id) + "/" + String::utf8(node));
        relay->queue_free();
        luaL_unref(L, LUA_REGISTRYINDEX, ref);
    }
    return 0;
}

// ---- Input.* ----
// Input.is_pressed(action) / just_pressed / just_released -> bool
int l_input_is_pressed(lua_State *L) {
    const char *a = luaL_checkstring(L, 1);
    Node *s = autoload("InputService");
    bool ok = s && (bool)s->call("is_pressed", String::utf8(a));
    lua_pushboolean(L, ok); return 1;
}
int l_input_just_pressed(lua_State *L) {
    const char *a = luaL_checkstring(L, 1);
    Node *s = autoload("InputService");
    bool ok = s && (bool)s->call("just_pressed", String::utf8(a));
    lua_pushboolean(L, ok); return 1;
}
int l_input_just_released(lua_State *L) {
    const char *a = luaL_checkstring(L, 1);
    Node *s = autoload("InputService");
    bool ok = s && (bool)s->call("just_released", String::utf8(a));
    lua_pushboolean(L, ok); return 1;
}
// Input.get_axis(neg_action, pos_action) -> number [-1,1]
int l_input_get_axis(lua_State *L) {
    const char *neg = luaL_checkstring(L, 1);
    const char *pos = luaL_checkstring(L, 2);
    Node *s = autoload("InputService");
    double v = 0.0;
    if (s) v = (double)s->call("get_axis", String::utf8(neg), String::utf8(pos));
    lua_pushnumber(L, v); return 1;
}
// Input.get_pointer() -> x, y (视口坐标)
int l_input_get_pointer(lua_State *L) {
    Node *s = autoload("InputService");
    Vector2 p;
    if (s) p = (Vector2)s->call("get_pointer_position");
    lua_pushnumber(L, p.x); lua_pushnumber(L, p.y); return 2;
}
// Input.on_action(action, fn)  -- fn(action) 在按下边沿触发
int l_input_on_action(lua_State *L) {
    const char *a = luaL_checkstring(L, 1);
    luaL_checktype(L, 2, LUA_TFUNCTION);
    Node *s = autoload("InputService");
    if (!s) { return 0; }
    lua_pushvalue(L, 2);
    int ref = luaL_ref(L, LUA_REGISTRYINDEX);
    GdInputActionRelay *relay = memnew(GdInputActionRelay);
    relay->want_action = a;
    relay->lua_ref = ref;
    relay->state = L;
    s->add_child(relay);
    // 连 InputService.action_triggered(action, edge)；relay 内部按 want_action + pressed 过滤。
    s->connect("action_triggered", Callable(relay, "_on_action"));
    return 0;
}

// ---- Timer.* ----
// 内部：建一个 relay 承载 Lua 回调，把 relay._fire 作为 Callable 交给 TimerService。
// 返回 TimerService 分配的 timer id（Lua 用来 cancel/reset）。
static int add_timer_common(lua_State *L, bool once) {
    double ttl = luaL_checknumber(L, 1);
    luaL_checktype(L, 2, LUA_TFUNCTION);
    bool late = false;
    if (lua_gettop(L) >= 3 && !lua_isnil(L, 3)) late = lua_toboolean(L, 3) != 0;

    Node *s = autoload("TimerService");
    if (!s) { lua_pushinteger(L, 0); return 1; }

    lua_pushvalue(L, 2);
    int ref = luaL_ref(L, LUA_REGISTRYINDEX);

    GdTimerRelay *relay = memnew(GdTimerRelay);
    relay->lua_ref = ref;
    relay->state = L;
    relay->once = once;
    s->add_child(relay);   // 挂到 TimerService 下，随其生命周期

    // 调 TimerService.after/every(ttl, Callable, late) -> id
    const char *method = once ? "after" : "every";
    Variant vid = s->call(method, ttl, Callable(relay, "_fire"), late);
    lua_pushinteger(L, (lua_Integer)(int64_t)vid);
    return 1;
}
int l_timer_after(lua_State *L) { return add_timer_common(L, true); }
int l_timer_every(lua_State *L) { return add_timer_common(L, false); }
int l_timer_cancel(lua_State *L) {
    lua_Integer id = luaL_checkinteger(L, 1);
    Node *s = autoload("TimerService");
    if (s) s->call("cancel", (int)id);
    return 0;
}
int l_timer_reset(lua_State *L) {
    lua_Integer id = luaL_checkinteger(L, 1);
    Node *s = autoload("TimerService");
    if (s) s->call("reset", (int)id);
    return 0;
}

// ---- Log.* ----
// 变参、print 风格：Log.info("a", 1, true, tbl) —— 每个参数经 Lua tostring 转字符串，
// 以空格连接成一条消息，再交给 LogService（异步写盘 + 控制台）。
static int log_common(lua_State *L, const char *method) {
    int n = lua_gettop(L);
    luaL_Buffer b;
    luaL_buffinit(L, &b);
    for (int i = 1; i <= n; ++i) {
        // 用 luaL_tolstring：等价 Lua 的 tostring（会走 __tostring 元方法），把结果压栈
        size_t len = 0;
        const char *s = luaL_tolstring(L, i, &len);   // push 转换结果
        luaL_addlstring(&b, s, len);
        lua_pop(L, 1);                                 // 弹出 tolstring 的结果
        if (i < n) luaL_addchar(&b, ' ');
    }
    luaL_pushresult(&b);                               // 栈顶 = 拼好的消息串
    size_t mlen = 0;
    const char *msg = lua_tolstring(L, -1, &mlen);
    String gmsg = String::utf8(msg ? msg : "", (int)mlen);
    lua_pop(L, 1);

    Node *s = autoload("LogService");
    if (s) s->call(method, gmsg);
    else UtilityFunctions::print(String("[Log] ") + gmsg); // 无服务时兜底
    return 0;
}
int l_log_debug(lua_State *L) { return log_common(L, "debug"); }
int l_log_info(lua_State *L)  { return log_common(L, "info"); }
int l_log_warn(lua_State *L)  { return log_common(L, "warn"); }
int l_log_error(lua_State *L) { return log_common(L, "error"); }

} // namespace

void register_godot_lua_bindings(void *luaEnv, Node *ctx) {
    g_ctx = ctx;
    lua_State *L = static_cast<lua_State*>(luaEnv);
    if (L == nullptr) {
        UtilityFunctions::printerr("[GodotLua] luaEnv is null (no Lua VM yet)");
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
    lua_pushcfunction(L, l_ui_open);              lua_setfield(L, -2, "open_panel");
    lua_pushcfunction(L, l_ui_open_async);        lua_setfield(L, -2, "open_panel_async");
    lua_pushcfunction(L, l_ui_close);             lua_setfield(L, -2, "close_panel");
    lua_pushcfunction(L, l_ui_set_text);          lua_setfield(L, -2, "set_text");
    lua_pushcfunction(L, l_ui_get_input);         lua_setfield(L, -2, "get_input_text");
    lua_pushcfunction(L, l_ui_on_click);          lua_setfield(L, -2, "on_click");
    lua_pushcfunction(L, l_ui_on_text_changed);   lua_setfield(L, -2, "on_text_changed");
    lua_pushcfunction(L, l_ui_on_value_changed);  lua_setfield(L, -2, "on_value_changed");
    lua_setglobal(L, "UI");

    // Input table
    lua_newtable(L);
    lua_pushcfunction(L, l_input_is_pressed);     lua_setfield(L, -2, "is_pressed");
    lua_pushcfunction(L, l_input_just_pressed);   lua_setfield(L, -2, "just_pressed");
    lua_pushcfunction(L, l_input_just_released);  lua_setfield(L, -2, "just_released");
    lua_pushcfunction(L, l_input_get_axis);       lua_setfield(L, -2, "get_axis");
    lua_pushcfunction(L, l_input_get_pointer);    lua_setfield(L, -2, "get_pointer");
    lua_pushcfunction(L, l_input_on_action);      lua_setfield(L, -2, "on_action");
    lua_setglobal(L, "Input");

    // Timer table
    lua_newtable(L);
    lua_pushcfunction(L, l_timer_after);   lua_setfield(L, -2, "after");
    lua_pushcfunction(L, l_timer_every);   lua_setfield(L, -2, "every");
    lua_pushcfunction(L, l_timer_cancel);  lua_setfield(L, -2, "cancel");
    lua_pushcfunction(L, l_timer_reset);   lua_setfield(L, -2, "reset");
    lua_setglobal(L, "Timer");

    // Log table
    lua_newtable(L);
    lua_pushcfunction(L, l_log_debug);  lua_setfield(L, -2, "debug");
    lua_pushcfunction(L, l_log_info);   lua_setfield(L, -2, "info");
    lua_pushcfunction(L, l_log_warn);   lua_setfield(L, -2, "warn");
    lua_pushcfunction(L, l_log_error);  lua_setfield(L, -2, "error");
    lua_setglobal(L, "Log");

    UtilityFunctions::print("[GodotLua] Scene/UI/Input/Timer/Log Lua bindings registered on this VM");
}
