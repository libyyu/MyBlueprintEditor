// game_launcher.cpp - process-level bootstrap implementation.

#include "game_launcher.h"
#include "blueprint_node.h"
#include "godot_file_bridge.h"
#ifdef BLUEPRINT_HAS_LUA
#include "godot_lua_bindings.h"
#include <lua.hpp>
#endif

#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/variant/utility_functions.hpp>
#include <godot_cpp/classes/file_access.hpp>

// BlueprintCAPI.h 内部已用 #ifdef __cplusplus extern "C" 包裹函数声明，
// 这里不能再套一层 extern "C" { ... }——Emscripten 下 <emscripten.h> 里含 C++ 模板，
// 塞进 C 链接块会触发 "templates must have C++ linkage" 编译错误。
#include <godot_lua_bindings.h>

#include "BlueprintCAPI.h"

using namespace godot;

#ifdef _WIN32
#include <tchar.h>
#include <Windows.h>
#include <Shlwapi.h>
#include <io.h>
#include <fcntl.h>
#include <iostream>

static void BindCrtHandlesToStdHandles(bool bindStdIn, bool bindStdOut, bool bindStdErr)
{
    // Re-initialize the C runtime "FILE" handles with clean handles bound to "nul". We do this because it has been
    // observed that the file number of our standard handle file objects can be assigned internally to a value of -2
    // when not bound to a valid target, which represents some kind of unknown internal invalid state. In this state our
    // call to "_dup2" fails, as it specifically tests to ensure that the target file number isn't equal to this value
    // before allowing the operation to continue. We can resolve this issue by first "re-opening" the target files to
    // use the "nul" device, which will place them into a valid state, after which we can redirect them to our target
    // using the "_dup2" function.
    if (bindStdIn)
    {
        FILE* dummyFile;
        freopen_s(&dummyFile, "nul", "r", stdin);
    }
    if (bindStdOut)
    {
        FILE* dummyFile;
        freopen_s(&dummyFile, "nul", "w", stdout);
    }
    if (bindStdErr)
    {
        FILE* dummyFile;
        freopen_s(&dummyFile, "nul", "w", stderr);
    }

    // Redirect unbuffered stdin from the current standard input handle
    if (bindStdIn)
    {
        HANDLE stdHandle = GetStdHandle(STD_INPUT_HANDLE);
        if (stdHandle != INVALID_HANDLE_VALUE)
        {
            int fileDescriptor = _open_osfhandle((intptr_t)stdHandle, _O_TEXT);
            if (fileDescriptor != -1)
            {
                FILE* file = _fdopen(fileDescriptor, "r");
                if (file != NULL)
                {
                    int dup2Result = _dup2(_fileno(file), _fileno(stdin));
                    if (dup2Result == 0)
                    {
                        setvbuf(stdin, NULL, _IONBF, 0);
                    }
                }
            }
        }
    }

    // Redirect unbuffered stdout to the current standard output handle
    if (bindStdOut)
    {
        HANDLE stdHandle = GetStdHandle(STD_OUTPUT_HANDLE);
        if (stdHandle != INVALID_HANDLE_VALUE)
        {
            int fileDescriptor = _open_osfhandle((intptr_t)stdHandle, _O_TEXT);
            if (fileDescriptor != -1)
            {
                FILE* file = _fdopen(fileDescriptor, "w");
                if (file != NULL)
                {
                    int dup2Result = _dup2(_fileno(file), _fileno(stdout));
                    if (dup2Result == 0)
                    {
                        setvbuf(stdout, NULL, _IONBF, 0);
                    }
                }
            }
        }
    }

    // Redirect unbuffered stderr to the current standard error handle
    if (bindStdErr)
    {
        HANDLE stdHandle = GetStdHandle(STD_ERROR_HANDLE);
        if (stdHandle != INVALID_HANDLE_VALUE)
        {
            int fileDescriptor = _open_osfhandle((intptr_t)stdHandle, _O_TEXT);
            if (fileDescriptor != -1)
            {
                FILE* file = _fdopen(fileDescriptor, "w");
                if (file != NULL)
                {
                    int dup2Result = _dup2(_fileno(file), _fileno(stderr));
                    if (dup2Result == 0)
                    {
                        setvbuf(stderr, NULL, _IONBF, 0);
                    }
                }
            }
        }
    }

    // Clear the error state for each of the C++ standard stream objects. We need to do this, as attempts to access the
    // standard streams before they refer to a valid target will cause the iostream objects to enter an error state. In
    // versions of Visual Studio after 2005, this seems to always occur during startup regardless of whether anything
    // has been read from or written to the targets or not.
    if (bindStdIn)
    {
        std::wcin.clear();
        std::cin.clear();
    }
    if (bindStdOut)
    {
        std::wcout.clear();
        std::cout.clear();
    }
    if (bindStdErr)
    {
        std::wcerr.clear();
        std::cerr.clear();
    }
}

static void ShowConsole(bool newConsole=false)
{
    //// 分配一个控制台，以便于输出一些有用的信息
    if (!newConsole) {
        AttachConsole(ATTACH_PARENT_PROCESS);
        BindCrtHandlesToStdHandles(false, true, true);
    }
    else
    {
        AllocConsole();
        BindCrtHandlesToStdHandles(false, true, true);
    }

    HANDLE oHandle = GetStdHandle(STD_OUTPUT_HANDLE);
    if (oHandle != INVALID_HANDLE_VALUE)
    {
        DWORD mode;
        if (GetConsoleMode(oHandle, &mode))
        {
            // attempt to enable 0x4: ENABLE_VIRTUAL_TERMINAL_PROCESSING
            SetConsoleMode(oHandle, mode | 0x4);
        }
    }

    HANDLE eHandle = GetStdHandle(STD_ERROR_HANDLE);
    if (eHandle != INVALID_HANDLE_VALUE)
    {
        DWORD mode;
        if (GetConsoleMode(eHandle, &mode))
        {
            // attempt to enable 0x4: ENABLE_VIRTUAL_TERMINAL_PROCESSING
            SetConsoleMode(eHandle, mode | 0x4);
        }
    }

    ::SetConsoleOutputCP(65001);
}

#endif

GameLauncher::GameLauncher() {
#ifdef _WIN32
    ShowConsole();
#endif
    
#ifdef BLUEPRINT_HAS_LUA
    // Sensible default require root.
    _lua_roots.append("res://lua");
#endif
}

void GameLauncher::_process(double delta) {
    if (Engine::get_singleton()->is_editor_hint()) return;
    // Process async results posted from background threads, then advance timers.
    BP_DrainQueue();
}

void GameLauncher::_ready() {
    if (Engine::get_singleton()->is_editor_hint()) return;
    // setup() is ALWAYS needed (both the update phase and the game phase rely on
    // the environment it installs), so run it as early as possible. auto_setup
    // only controls whether we do it here in _ready or let Boot.gd do it first;
    // either way it runs before anything loads a blueprint.
    if (_auto_setup) setup();
}


bool GameLauncher::setup() {
    if (_did_setup) return true;
    _did_setup = true;
    // Process-level environment — needed by EVERY phase (update + game):
    BP_InitDefaultHttpClient();        // LLM/http nodes
    install_godot_file_reader("");     // all file reads -> Godot FileAccess
    UtilityFunctions::print("[GameLauncher] setup complete (file reader)");
    return true;
}

void GameLauncher::begin_update_phase() {
    setup();  
    
#ifdef BLUEPRINT_HAS_LUA
    ensure_global_lua_state();
    bind_lua_api();
    init_lua_search();
#endif
    
    // guarantee env is up (idempotent)
    if (_update_node) return; // already running
    _update_node = memnew(BlueprintNode);
    _update_node->set_autorun(false);
    add_child(_update_node);
    UtilityFunctions::print("[GameLauncher] update phase begun (isolated VM)");
#ifdef BLUEPRINT_HAS_LUA
    run_lua_file("UpdateLogic");
#endif
}

void GameLauncher::end_update_phase() {
    if (_update_node) {
        // Destroy the update runner FIRST so its shared_ptr no longer keeps the
        // old VM alive, then close the old lua_State.
        _update_node->queue_free();
        _update_node = nullptr;
    }

#ifdef BLUEPRINT_HAS_LUA
    reset_global_lua_state();      // re-bind resolver to the fresh game VM (created on demand)
#endif
    
    UtilityFunctions::print("[GameLauncher] update phase ended; fresh game VM ready");
}

void GameLauncher::enter_game_logic() {
    setup();
    UtilityFunctions::print("[GameLauncher] game logic entered");
#ifdef BLUEPRINT_HAS_LUA
    ensure_global_lua_state();
    bind_lua_api();
    init_lua_search();
    run_lua_file("GameLogic");
#endif
    
    
}

#ifdef BLUEPRINT_HAS_LUA
void GameLauncher::bind_lua_api() {
    if (!_global_lua_state) {
        UtilityFunctions::printerr("[GameLauncher] bind_lua_api: _global_lua_state is null");
        return;
    }
    register_godot_lua_bindings(_global_lua_state, this);
}
void GameLauncher::set_lua_roots(const PackedStringArray &roots) { _lua_roots = roots; }
PackedStringArray GameLauncher::get_lua_roots() const { return _lua_roots; }
void* GameLauncher::get_global_lua_state() const { return _global_lua_state; }
void* GameLauncher::ensure_global_lua_state()
{
    if (_global_lua_state) return _global_lua_state;
    lua_State* lua = BP_GetDefaultLuaState(0);
    _global_lua_state = lua;
    return lua;
}
void GameLauncher::reset_global_lua_state()
{
    if (!_global_lua_state) return;
    //BP_NotifyLuaStateClosing((lua_State*)_global_lua_state);
    BP_ResetSharedLuaVM();
    _global_lua_state = nullptr;
}
bool GameLauncher::run_lua_file(const String &path)
{
    if (!_global_lua_state)
    {
        UtilityFunctions::printerr("[GameLauncher] run_lua_file: _global_lua_state is null");
        return false;
    }
    String code("package.loaded['" + path + "'] = nil");
    code += ("\nrequire('" + path + "')");
    return run_lua_code(code);
}
bool GameLauncher::run_lua_code(const String &code)
{
    if (!_global_lua_state)
    {
        UtilityFunctions::printerr("[GameLauncher] run_lua_code: _global_lua_state is null");
        return false;
    }
    lua_State* L = (lua_State*)_global_lua_state;
    CharString c = code.utf8();
    if (0 != luaL_loadstring(L, c.get_data()))
    {
        const char* errmsg = lua_tostring(L, -1);
        UtilityFunctions::printerr(String("[GameLauncher] run_lua_code compile: ") + String::utf8(errmsg));
        lua_pop(L, 1);
        return false;
    }
    
    if (lua_pcall(L, 0, 0, 0) != LUA_OK) {
        const char* errmsg = lua_tostring(L, -1);
        UtilityFunctions::printerr(String("[GameLauncher] run_lua_code exec: ") + String::utf8(errmsg));
        lua_pop(L, 1);
        return false;
    }
    return true;
}
bool GameLauncher::require_lua_file(const String &code)
{
    if (!_global_lua_state)
    {
        UtilityFunctions::printerr("[GameLauncher] require_lua_file: _global_lua_state is null");
        return false;
    }
    return run_lua_code("require('" + code + "')");
}

void GameLauncher::init_lua_search()
{
    if (!_global_lua_state)
    {
        UtilityFunctions::printerr("[GameLauncher] init_lua_search: _global_lua_state is null");
        return;
    }
    
    static auto luaSearcher = [](lua_State* L) -> int
    {
        lua_getfield(L, LUA_REGISTRYINDEX, "__game_launcher");
        auto* launcher = static_cast<GameLauncher*>(lua_touserdata(L, -1));
        lua_pop(L, 1);
        
        String s_fileName = String::utf8(luaL_checkstring(L, 1));
        s_fileName = s_fileName.replace(".", "/");
        String fileName = s_fileName;
        if (!fileName.ends_with(".lua")) fileName += ".lua";
        
        for (const String &root : launcher->get_lua_roots())
        {
            String full = root;
            if (!full.ends_with("/")) full += "/";
            full += fileName;
            if (!FileAccess::file_exists(full)) continue;
            UtilityFunctions::print("[GameLauncher] found lua: " + full);
        
            Ref<FileAccess> f = FileAccess::open(full, FileAccess::READ);
            if (f.is_null()) continue;
            PackedByteArray bytes = f->get_buffer(f->get_length());
            f->close();

            // int n = static_cast<int>(bytes.size());
            // char *buf = static_cast<char *>(std::malloc(n > 0 ? n : 1));
            // if (!buf) return 0;
            // if (n > 0) std::memcpy(buf, bytes.ptr(), static_cast<size_t>(n));
            // *outData = buf;
            // *outSize = n;
            // if (outChunk) *outChunk = dup_cstr(full);
            
            String chunk = "@"; chunk += fileName;
            if (luaL_loadbuffer(L, (char*)bytes.ptr(), bytes.size(), chunk.utf8()) != 0)
            {
                lua_error(L);
                return 0;
            }
            
            return 1;
        }
        return 0;
    };
    lua_State* L = (lua_State*)_global_lua_state;
    lua_pushlightuserdata(L, this);
    lua_setfield(L, LUA_REGISTRYINDEX, "__game_launcher");
    
    BP_SetLuaSearchFileFn(luaSearcher);
    
    BP_SetLuaPrintCallback([](int level, const char* message)
    {
        static String prefix = "[Lua] ";
        if (level >= static_cast<int>(BP_LogLevel::BP_LOG_ERROR))
        {
            UtilityFunctions::printerr(prefix, String::utf8(message));
        }
        else
        {
            UtilityFunctions::print(prefix, String::utf8(message));
        }
        
    });
}

#endif

void GameLauncher::set_auto_setup(bool v) { _auto_setup = v; }
bool GameLauncher::get_auto_setup() const { return _auto_setup; }

void GameLauncher::_bind_methods() {
    ClassDB::bind_method(D_METHOD("setup"), &GameLauncher::setup);
    ClassDB::bind_method(D_METHOD("is_ready"), &GameLauncher::is_ready);
    ClassDB::bind_method(D_METHOD("begin_update_phase"), &GameLauncher::begin_update_phase);
    ClassDB::bind_method(D_METHOD("end_update_phase"), &GameLauncher::end_update_phase);
    ClassDB::bind_method(D_METHOD("in_update_phase"), &GameLauncher::in_update_phase);
#ifdef BLUEPRINT_HAS_LUA
    ClassDB::bind_method(D_METHOD("run_lua_code"), &GameLauncher::run_lua_code);
    ClassDB::bind_method(D_METHOD("run_lua_file"), &GameLauncher::run_lua_file);
    ClassDB::bind_method(D_METHOD("require_lua_file"), &GameLauncher::require_lua_file);
    ClassDB::bind_method(D_METHOD("reset_global_lua_state"), &GameLauncher::reset_global_lua_state);
    ClassDB::bind_method(D_METHOD("bind_lua_api"), &GameLauncher::bind_lua_api);
    ClassDB::bind_method(D_METHOD("set_lua_roots", "roots"), &GameLauncher::set_lua_roots);
    ClassDB::bind_method(D_METHOD("get_lua_roots"), &GameLauncher::get_lua_roots);
    ADD_PROPERTY(PropertyInfo(Variant::PACKED_STRING_ARRAY, "lua_roots"),
                 "set_lua_roots", "get_lua_roots");
#endif

    ClassDB::bind_method(D_METHOD("set_auto_setup", "v"), &GameLauncher::set_auto_setup);
    ClassDB::bind_method(D_METHOD("get_auto_setup"), &GameLauncher::get_auto_setup);
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "auto_setup"), "set_auto_setup", "get_auto_setup");
}
