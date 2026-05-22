// runtime-example.cpp  ─  BlueprintRuntime CLI
//
// USAGE
//   runtime-example                                 Run built-in demo
//   runtime-example <file.bjson> [options]          Execute a blueprint file
//   runtime-example --test [flow_test.json]         Run built-in unit tests
//   runtime-example --test-new                      Run new-feature unit tests
//   runtime-example --repl                          Interactive REPL mode
//
// EXECUTION OPTIONS
//   -v KEY=VALUE            Inject a blueprint variable before execution
//   -e EVENT                Dispatch a named event (default: OnBeginPlay)
//                           Use -e "" to skip OnBeginPlay
//   --lua <script.lua>      Load and execute a Lua script before running
//                           (overrides auto BlueprintEntry.lua search)
//   --no-entry-lua          Skip automatic BlueprintEntry.lua loading
//   --max-time <secs>       Max wall-clock time to wait for async work (default: 30)
//   --tick-rate <ms>        Frame loop interval in ms (default: 16)
//   --no-deps               Skip automatic dependency loading
//
// OUTPUT OPTIONS
//   --quiet, -q             Suppress all output except PrintString
//   --output json           After execution, print all variables as JSON
//   --output vars           Print variable name=value pairs (default for --output)
//   --watch                 Re-execute when the file changes (Ctrl+C to stop)
//
// ENVIRONMENT VARIABLES
//   BP_APIKEY               Injected as ApiKey variable
//   BP_BASEURL              Injected as BaseURL variable
//   BP_MODEL                Injected as Model variable
//   BP_VAR_<NAME>           Injected as variable <NAME>  (e.g. BP_VAR_Timeout=10)
//
// AUTO LUA LOADING
//   When no --lua is given, the runtime automatically searches for
//   BlueprintEntry.lua in:  1) blueprint file directory  2) exe directory
//   Use --no-entry-lua to disable this behaviour.
//
// EXAMPLES
//   runtime-example data/examples/ReActAgent.bjson -v ApiKey=sk-xxx -v UserQuery="hello"
//   runtime-example data/examples/WeComBot.bjson -v BotId=xxx -v Secret=yyy --watch
//   runtime-example agent.bjson -q --output json
//   runtime-example agent.bjson -e OnTick --lua hooks.lua
//   runtime-example --repl

#include <iostream>
#include <fstream>
#include <string>
#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <vector>
#include <map>
#include <thread>
#include <chrono>
#include <sstream>
#include <algorithm>
#include <functional>
#include <csignal>
#include <atomic>

#if defined(_WIN32) || defined(_WIN64)
#  ifndef WIN32_LEAN_AND_MEAN
#    define WIN32_LEAN_AND_MEAN
#  endif
#  include <windows.h>
#elif defined(__linux__)
#  include <unistd.h>
#elif defined(__APPLE__)
#  include <mach-o/dyld.h>
#endif

#include "BlueprintRunner.h"
#include "BlueprintExporter.h"
#include "BuiltinHandlers.h"
#include "MainThreadDispatcher.h"
#include "BlueprintCAPI.h"
#include "crude_json.h"

using namespace NodeEditor::Runtime;

// ============================================================================
// Signal handling for --watch / REPL
// ============================================================================
static std::atomic<bool> g_interrupted{false};

static void signalHandler(int) { g_interrupted = true; }

// ============================================================================
// Lightweight unit test framework (unchanged)
// ============================================================================

static int g_pass = 0;
static int g_fail = 0;

#define CHECK(cond, msg) \
    do { \
        if (cond) { std::cout << "  [PASS] " << (msg) << "\n"; ++g_pass; } \
        else      { std::cout << "  [FAIL] " << (msg) << "\n"; ++g_fail; } \
    } while(0)

// ============================================================================
// Test 1-7 (unchanged, omitted for brevity — same as before)
// ============================================================================
static void test_registry_no_dangling_ptr();
static void test_variant_conversion();
static void test_variant_map();
static void test_flow_blueprint(const std::string& flowTestPath);
static void test_map_nodes();
static void test_crude_json_utf8();
static void test_add_type_promotion();

static int runTests(const std::string& flowTestPath)
{
    g_pass = 0; g_fail = 0;
    std::cout << "========== BlueprintRuntime Unit Tests ==========\n";
    test_registry_no_dangling_ptr();
    test_variant_conversion();
    test_variant_map();
    test_flow_blueprint(flowTestPath);
    test_map_nodes();
    test_crude_json_utf8();
    test_add_type_promotion();
    std::cout << "\n========== Result ==========\n";
    std::cout << "PASS: " << g_pass << "  FAIL: " << g_fail << "\n";
    return (g_fail == 0) ? 0 : 1;
}

// ============================================================================
// Parse  KEY=VALUE  (value may contain '=')
// ============================================================================
static bool parseKV(const std::string& s, std::string& key, std::string& val)
{
    auto pos = s.find('=');
    if (pos == std::string::npos) return false;
    key = s.substr(0, pos);
    val = s.substr(pos + 1);
    return !key.empty();
}

// ============================================================================
// Variant from string (auto-detect int / float / bool / string)
// ============================================================================
static Variant variantFromString(const std::string& s)
{
    if (s == "true"  || s == "True"  || s == "TRUE")  return Variant(true);
    if (s == "false" || s == "False" || s == "FALSE") return Variant(false);
    // try integer
    {
        char* end = nullptr;
        long long iv = std::strtoll(s.c_str(), &end, 10);
        if (end != s.c_str() && *end == '\0') return Variant(int64_t(iv));
    }
    // try float
    {
        char* end = nullptr;
        double dv = std::strtod(s.c_str(), &end);
        if (end != s.c_str() && *end == '\0') return Variant(dv);
    }
    return Variant(s);
}

// ============================================================================
// Collect env-var variables (BP_APIKEY / BP_BASEURL / BP_MODEL / BP_VAR_*)
// ============================================================================
static std::map<std::string,std::string> collectEnvVars()
{
    std::map<std::string,std::string> out;
    struct KnownAlias { const char* envName; const char* varName; };
    static const KnownAlias aliases[] = {
        {"BP_APIKEY",  "ApiKey"},
        {"BP_BASEURL", "BaseURL"},
        {"BP_MODEL",   "Model"},
    };
    for (const auto& a : aliases)
    {
        const char* v = std::getenv(a.envName);
        if (v && *v) out[a.varName] = v;
    }
    // Generic BP_VAR_<NAME>
#ifdef _WIN32
    // On Windows enumerate environment block
    extern char** environ;
#else
    extern char** environ;
#endif
    for (char** ep = environ; ep && *ep; ++ep)
    {
        std::string entry(*ep);
        if (entry.substr(0, 7) == "BP_VAR_")
        {
            std::string key, val;
            if (parseKV(entry.substr(7), key, val))
                out[key] = val;
        }
    }
    return out;
}

// ============================================================================
// Print all variables as JSON or KEY=VALUE
// ============================================================================
static void printVariables(BlueprintRunner& runner,
                            const std::string& format,   // "json" | "vars"
                            bool quiet)
{
    const auto& vars = runner.GetAllVariables();
    if (vars.empty()) return;

    if (format == "json")
    {
        std::cout << "{\n";
        bool first = true;
        for (const auto& kv : vars)
        {
            if (!first) std::cout << ",\n";
            first = false;
            std::cout << "  \"" << kv.first << "\": ";
            const auto& v = kv.second;
            switch (v.type) {
            case PinDataType::Boolean:
                std::cout << (v.asBool() ? "true" : "false"); break;
            case PinDataType::Integer:
                std::cout << v.asInt(); break;
            case PinDataType::Float:
                std::cout << v.asFloat(); break;
            default:
                // string: JSON-escape minimal
                {
                    std::string sv = v.asString();
                    std::string escaped;
                    for (char c : sv) {
                        if (c == '"') escaped += "\\\"";
                        else if (c == '\\') escaped += "\\\\";
                        else if (c == '\n') escaped += "\\n";
                        else if (c == '\r') escaped += "\\r";
                        else escaped += c;
                    }
                    std::cout << "\"" << escaped << "\"";
                }
                break;
            }
        }
        std::cout << "\n}" << std::endl;
    }
    else // "vars"
    {
        for (const auto& kv : vars)
            std::cout << kv.first << "=" << kv.second.asString() << "\n";
        std::cout.flush();
    }
}

// ============================================================================
// Single execution pass (shared by run-once and --watch)
// ============================================================================
struct RunOptions {
    std::string filePath;
    std::vector<std::pair<std::string,std::string>> vars;  // KEY=VALUE pairs
    std::vector<std::string> events;   // events to dispatch; empty = ["OnBeginPlay"]
    std::string luaScript;             // "" = none (auto-search BlueprintEntry.lua)
    float maxTimeSec  = 30.0f;
    int   tickRateMs  = 16;
    bool  quiet       = false;
    bool  noDeps      = false;
    bool  noEntryLua  = false;         // --no-entry-lua: skip auto BlueprintEntry.lua
    std::string outputFmt; // "" | "json" | "vars"
};

static int executeSingle(const RunOptions& opt)
{
    namespace fs = std::filesystem;

    auto log   = [&](const std::string& msg){ if (!opt.quiet) std::cout << msg << "\n"; };
    auto print = [&](const std::string& msg){ std::cout << msg << "\n"; };  // always

    if (!fs::exists(opt.filePath))
    {
        std::cerr << "ERROR: File not found: " << opt.filePath << "\n";
        return 1;
    }

    std::string basePath = fs::path(opt.filePath).parent_path().string();
    if (basePath.empty()) basePath = ".";

    if (!opt.quiet)
    {
        std::cout << "=== BlueprintRuntime CLI ===\n";
        std::cout << "File: " << opt.filePath << "\n";
        if (!opt.luaScript.empty())
            std::cout << "Lua:  " << opt.luaScript << "\n";
        std::cout << "\n";
    }

    BlueprintRunner runner;
    runner.SetLogCallback([&](LogLevel, const std::string& msg)
        { if (!opt.quiet) std::cout << msg << "\n"; });
    runner.SetPrintCallback([&](LogLevel, const std::string& msg)
        { std::cout << msg << "\n"; });

    BP_InitDefaultHttpClient();

    // Load
    bool loaded = opt.noDeps
        ? runner.LoadFromFile(opt.filePath)
        : runner.LoadFromFileWithDeps(opt.filePath);
    if (!loaded)
    {
        std::cerr << "ERROR loading: " << runner.GetLastError() << "\n";
        return 1;
    }

    const auto& bp = runner.GetBlueprintData();
    if (!opt.quiet)
    {
        std::cout << "Blueprint: " << (bp.metadata.name.empty() ? "(unnamed)" : bp.metadata.name) << "\n";
        std::cout << "Nodes:     " << bp.nodes.size() << "  Links: " << bp.links.size() << "\n\n";
    }

    RegisterBuiltinHandlers(runner, basePath);
    runner.SetDefaultHandler([&opt](ExecutionContext& ctx) {
        auto node = ctx.GetCurrentNode();
        if (!opt.quiet && node)
            std::cout << "[default] pass-through: " << node->name << "\n";
        return true;
    });

    // Env vars (lower priority than -v)
    for (const auto& kv : collectEnvVars())
        runner.SetVariable(kv.first, variantFromString(kv.second));

    // CLI -v variables (override env vars)
    for (const auto& kv : opt.vars)
        runner.SetVariable(kv.first, variantFromString(kv.second));

#ifdef BLUEPRINT_HAS_LUA
    // Lua script injection：--lua 显式指定，或自动搜索 BlueprintEntry.lua
    if (!opt.noEntryLua)
    {
        std::string luaToLoad = opt.luaScript;

        if (luaToLoad.empty())
        {
            // 自动搜索顺序：
            //   1. 蓝图文件所在目录/BlueprintEntry.lua
            //   2. exe 所在目录/BlueprintEntry.lua
            std::vector<std::string> candidates;
            candidates.push_back(basePath + "/BlueprintEntry.lua");

#if defined(_WIN32) || defined(_WIN64)
            char exePath[1024] = {};
            if (GetModuleFileNameA(nullptr, exePath, sizeof(exePath)))
            {
                fs::path exeDir = fs::path(exePath).parent_path();
                candidates.push_back((exeDir / "BlueprintEntry.lua").string());
            }
#else
            // Linux / macOS：通过 /proc/self/exe 或 _NSGetExecutablePath 获取 exe 路径
#  if defined(__linux__)
            char exePath[1024] = {};
            ssize_t len = readlink("/proc/self/exe", exePath, sizeof(exePath)-1);
            if (len > 0) {
                exePath[len] = '\0';
                fs::path exeDir = fs::path(exePath).parent_path();
                candidates.push_back((exeDir / "BlueprintEntry.lua").string());
            }
#  elif defined(__APPLE__)
            char exePath[1024] = {};
            uint32_t sz = sizeof(exePath);
            if (_NSGetExecutablePath(exePath, &sz) == 0) {
                fs::path exeDir = fs::path(exePath).parent_path();
                candidates.push_back((exeDir / "BlueprintEntry.lua").string());
            }
#  endif
#endif
            for (const auto& c : candidates)
            {
                if (fs::exists(c)) { luaToLoad = c; break; }
            }
        }

        if (!luaToLoad.empty())
        {
            if (!runner.LoadExtensionScript(luaToLoad))
            {
                std::cerr << "ERROR loading Lua: " << luaToLoad << "\n";
                if (!opt.luaScript.empty()) return 1;  // 显式指定时才致命
            }
            else if (!opt.quiet)
                std::cout << "Lua loaded: " << luaToLoad << "\n";
        }
    }
#endif

    if (!opt.quiet) std::cout << "=== Execution Started ===\n";

    auto t0 = std::chrono::high_resolution_clock::now();

    auto result = runner.Execute();

    // Dispatch events
    std::vector<std::string> events = opt.events.empty()
        ? std::vector<std::string>{"OnBeginPlay"}
        : opt.events;

    for (const auto& ev : events)
    {
        if (ev.empty()) continue;
        auto er = runner.DispatchEvent(ev);
        if (er.success && !er.executedNodeIds.empty())
        {
            result.nodesExecuted += er.nodesExecuted;
            for (auto nid : er.executedNodeIds)
                result.executedNodeIds.push_back(nid);
        }
        else if (!er.success && !er.errorMessage.empty())
        {
            if (!opt.quiet) std::cerr << "[WARN] " << ev << ": " << er.errorMessage << "\n";
        }
    }

    // Tick loop
    if (runner.HasPendingWork())
    {
        if (!opt.quiet)
            std::cout << "[Tick] active=" << runner.GetTimerManager().GetActiveTimerCount()
                      << " async=" << runner.PendingAsyncCount() << "\n";

        auto loopStart = std::chrono::high_resolution_clock::now();
        auto lastTick  = loopStart;

        while (runner.HasPendingWork() && !g_interrupted)
        {
            auto now = std::chrono::high_resolution_clock::now();
            if (std::chrono::duration<double>(now - loopStart).count() > opt.maxTimeSec)
            {
                if (!opt.quiet) std::cout << "[Tick] timeout (" << opt.maxTimeSec << "s)\n";
                break;
            }
            float dt = std::chrono::duration<float>(now - lastTick).count();
            lastTick = now;
            MainThreadDispatcher::Get().DrainQueue();
            runner.Tick(dt);
            std::this_thread::sleep_for(std::chrono::milliseconds(opt.tickRateMs));
        }
    }

    auto t1 = std::chrono::high_resolution_clock::now();
    double elapsed = std::chrono::duration<double, std::milli>(t1 - t0).count();

    if (!opt.quiet)
    {
        std::cout << "\n=== " << (result.success ? "OK" : "FAILED")
                  << " | " << result.nodesExecuted << " nodes"
                  << " | " << elapsed << " ms ===\n";
        if (!result.success && !result.errorMessage.empty())
            std::cerr << "Error: " << result.errorMessage << "\n";
    }

    // Output variables
    if (!opt.outputFmt.empty())
        printVariables(runner, opt.outputFmt, opt.quiet);

    return result.success ? 0 : 1;
}

// ============================================================================
// --watch mode
// ============================================================================
static int runWatch(const RunOptions& opt)
{
    namespace fs = std::filesystem;
    signal(SIGINT, signalHandler);

    std::cout << "[watch] Watching " << opt.filePath << " (Ctrl+C to stop)\n\n";

    auto lastWrite = fs::last_write_time(opt.filePath);
    executeSingle(opt);

    while (!g_interrupted)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        if (!fs::exists(opt.filePath)) continue;
        auto curWrite = fs::last_write_time(opt.filePath);
        if (curWrite != lastWrite)
        {
            lastWrite = curWrite;
            std::cout << "\n[watch] Change detected, re-executing...\n\n";
            MainThreadDispatcher::Get().DrainQueue(); // flush pending from last run
            executeSingle(opt);
        }
    }
    std::cout << "\n[watch] Stopped.\n";
    return 0;
}

// ============================================================================
// --repl  Interactive mode
// ============================================================================
static int runRepl()
{
    std::cout << "BlueprintRuntime REPL — type 'help' for commands\n\n";

    auto makeRunner = []{ return std::make_unique<BlueprintRunner>(); };
    auto runner = makeRunner();
    bool hasBlueprint = false;
    std::string basePath = ".";
    bool quiet = false;

    auto setupCallbacks = [&](){
        runner->SetLogCallback([&](LogLevel, const std::string& m){ if(!quiet) std::cout<<m<<"\n"; });
        runner->SetPrintCallback([](LogLevel, const std::string& m){ std::cout<<m<<"\n"; });
    };
    setupCallbacks();
    BP_InitDefaultHttpClient();

    signal(SIGINT, signalHandler);

    auto printHelp = [&](){
        std::cout <<
            "Commands:\n"
            "  load <file.bjson>        Load a blueprint\n"
            "  run [--no-deps]          Execute (Execute + OnBeginPlay)\n"
            "  event <name>             Dispatch a named event\n"
            "  set <VAR> <value>        Set a variable\n"
            "  get <VAR>                Get a variable value\n"
            "  vars                     List all current variables\n"
            "  lua <code>               Evaluate Lua snippet (if compiled with Lua)\n"
            "  tick [seconds]           Run tick loop for N seconds (default 1)\n"
            "  quiet [on|off]           Toggle verbose output\n"
            "  reset                    Reset runner state (keep blueprint loaded)\n"
            "  clear                    Unload current blueprint\n"
            "  help                     Show this help\n"
            "  exit / quit              Exit REPL\n\n";
    };

    printHelp();

    while (!g_interrupted)
    {
        std::cout << "bp> ";
        std::cout.flush();

        std::string line;
        if (!std::getline(std::cin, line))
            break;

        // trim
        while (!line.empty() && (line.front()==' '||line.front()=='\t')) line.erase(line.begin());
        while (!line.empty() && (line.back()==' '||line.back()=='\t'||line.back()=='\r'||line.back()=='\n')) line.pop_back();
        if (line.empty()) continue;

        // tokenize
        std::vector<std::string> toks;
        std::istringstream iss(line);
        std::string tok;
        while (iss >> tok) toks.push_back(tok);
        if (toks.empty()) continue;

        const std::string& cmd = toks[0];

        if (cmd == "exit" || cmd == "quit" || cmd == "q") break;

        else if (cmd == "help" || cmd == "h") printHelp();

        else if (cmd == "load")
        {
            if (toks.size() < 2) { std::cout << "Usage: load <file.bjson>\n"; continue; }
            bool noDeps = (toks.size() >= 3 && toks[2] == "--no-deps");
            namespace fs = std::filesystem;
            if (!fs::exists(toks[1])) { std::cerr << "File not found: " << toks[1] << "\n"; continue; }
            basePath = fs::path(toks[1]).parent_path().string();
            if (basePath.empty()) basePath = ".";
            runner = makeRunner();
            setupCallbacks();
            RegisterBuiltinHandlers(*runner, basePath);
            bool ok = noDeps ? runner->LoadFromFile(toks[1]) : runner->LoadFromFileWithDeps(toks[1]);
            if (ok)
            {
                hasBlueprint = true;
                const auto& bp = runner->GetBlueprintData();
                std::cout << "Loaded: " << (bp.metadata.name.empty() ? toks[1] : bp.metadata.name)
                          << " (" << bp.nodes.size() << " nodes, " << bp.links.size() << " links)\n";
            }
            else std::cerr << "ERROR: " << runner->GetLastError() << "\n";
        }

        else if (cmd == "run")
        {
            if (!hasBlueprint) { std::cout << "No blueprint loaded. Use: load <file>\n"; continue; }
            auto r = runner->Execute();
            auto er = runner->DispatchEvent("OnBeginPlay");
            if (er.success) r.nodesExecuted += er.nodesExecuted;
            std::cout << (r.success ? "OK" : "FAILED") << " | " << r.nodesExecuted << " nodes\n";
            if (!r.success && !r.errorMessage.empty()) std::cerr << "Error: " << r.errorMessage << "\n";
        }

        else if (cmd == "event")
        {
            if (!hasBlueprint) { std::cout << "No blueprint loaded.\n"; continue; }
            if (toks.size() < 2) { std::cout << "Usage: event <name>\n"; continue; }
            auto er = runner->DispatchEvent(toks[1]);
            std::cout << (er.success ? "OK" : "FAILED") << " | " << er.nodesExecuted << " nodes\n";
        }

        else if (cmd == "set")
        {
            if (toks.size() < 3) { std::cout << "Usage: set <VAR> <value>\n"; continue; }
            // rest of line after cmd and var name is the value
            std::string val;
            for (size_t i = 2; i < toks.size(); ++i) { if (i>2) val+=' '; val+=toks[i]; }
            runner->SetVariable(toks[1], variantFromString(val));
            std::cout << toks[1] << " = " << val << "\n";
        }

        else if (cmd == "get")
        {
            if (toks.size() < 2) { std::cout << "Usage: get <VAR>\n"; continue; }
            auto v = runner->GetVariable(toks[1]);
            if (v.type == PinDataType::Unknown) std::cout << "(not set)\n";
            else std::cout << toks[1] << " = " << v.asString() << "\n";
        }

        else if (cmd == "vars")
        {
            const auto& vars = runner->GetAllVariables();
            if (vars.empty()) { std::cout << "(no variables)\n"; continue; }
            for (const auto& kv : vars)
                std::cout << "  " << kv.first << " = " << kv.second.asString() << "\n";
        }

        else if (cmd == "tick")
        {
            float secs = 1.0f;
            if (toks.size() >= 2) secs = std::stof(toks[1]);
            auto deadline = std::chrono::steady_clock::now() + std::chrono::duration<float>(secs);
            auto lastT = std::chrono::steady_clock::now();
            int frames = 0;
            while (std::chrono::steady_clock::now() < deadline && !g_interrupted)
            {
                auto now = std::chrono::steady_clock::now();
                float dt = std::chrono::duration<float>(now - lastT).count();
                lastT = now;
                MainThreadDispatcher::Get().DrainQueue();
                runner->Tick(dt);
                ++frames;
                std::this_thread::sleep_for(std::chrono::milliseconds(16));
            }
            std::cout << "Ticked " << frames << " frames ("
                      << secs << "s). Active timers: "
                      << runner->GetTimerManager().GetActiveTimerCount() << "\n";
        }

        else if (cmd == "reset")
        {
            runner->ResetState();
            std::cout << "State reset.\n";
        }

        else if (cmd == "clear")
        {
            runner = makeRunner();
            setupCallbacks();
            hasBlueprint = false;
            std::cout << "Blueprint cleared.\n";
        }

        else if (cmd == "quiet")
        {
            if (toks.size() >= 2) quiet = (toks[1] == "on" || toks[1] == "1" || toks[1] == "true");
            else quiet = !quiet;
            std::cout << "Quiet mode: " << (quiet ? "on" : "off") << "\n";
        }

#ifdef BLUEPRINT_HAS_LUA
        else if (cmd == "lua")
        {
            if (!hasBlueprint) { std::cout << "No blueprint loaded.\n"; continue; }
            // rest of line is Lua code
            std::string code;
            for (size_t i = 1; i < toks.size(); ++i) { if (i>1) code+=' '; code+=toks[i]; }
            if (!runner->LoadExtensionScriptString(code, "=repl"))
                std::cerr << "Script error\n";
        }
#endif

        else
        {
            std::cout << "Unknown command: " << cmd << ". Type 'help'.\n";
        }
    }

    std::cout << "Bye.\n";
    return 0;
}

// ============================================================================
// Print help
// ============================================================================
static void printUsage(const char* prog)
{
    std::cout <<
        "BlueprintRuntime CLI\n\n"
        "USAGE\n"
        "  " << prog << " <file.bjson> [options]    Execute a blueprint\n"
        "  " << prog << " --repl                    Interactive REPL\n"
        "  " << prog << " --test [flow_test.json]   Built-in unit tests\n"
        "  " << prog << " --test-new                New-feature unit tests\n"
        "  " << prog << " --help                    Show this help\n\n"
        "EXECUTION OPTIONS\n"
        "  -v KEY=VALUE         Inject a variable (can repeat, e.g. -v ApiKey=sk-xxx)\n"
        "  -e EVENT             Dispatch event (default: OnBeginPlay; '' = none)\n"
        "  --lua <script.lua>   Load Lua script before execution\n"
        "                       (overrides auto BlueprintEntry.lua search)\n"
        "  --no-entry-lua       Skip automatic BlueprintEntry.lua loading\n"
        "  --max-time <secs>    Async wait timeout (default: 30)\n"
        "  --tick-rate <ms>     Frame interval in ms (default: 16)\n"
        "  --no-deps            Skip automatic dependency loading\n\n"
        "OUTPUT OPTIONS\n"
        "  -q, --quiet          Suppress all output except PrintString\n"
        "  --output json        Print variables as JSON after execution\n"
        "  --output vars        Print variables as KEY=VALUE after execution\n"
        "  --watch              Re-execute on file change (Ctrl+C to stop)\n\n"
        "ENVIRONMENT VARIABLES\n"
        "  BP_APIKEY / BP_BASEURL / BP_MODEL  → ApiKey / BaseURL / Model\n"
        "  BP_VAR_<NAME>=value               → variable <NAME>\n\n"
        "EXAMPLES\n"
        "  " << prog << " examples/ReActAgent.bjson -v ApiKey=sk-xxx -v UserQuery=hello\n"
        "  " << prog << " examples/FeishuBot.bjson --watch\n"
        "  " << prog << " agent.bjson -q --output json\n"
        "  " << prog << " agent.bjson -e OnTick --max-time 60\n";
}

// ============================================================================
// main
// ============================================================================

// Forward declaration (implemented in test_new_features.cpp)
int runNewFeatureTests();

int main(int argc, char* argv[])
{
#if defined(_WIN32) || defined(_WIN64)
    // 设置控制台输出为 UTF-8，避免中文乱码
    SetConsoleOutputCP(65001);
    SetConsoleCP(65001);
#endif

    // --help / -h
    for (int i = 1; i < argc; ++i)
        if (std::string(argv[i]) == "--help" || std::string(argv[i]) == "-h")
            { printUsage(argv[0]); return 0; }

    // --test
    for (int i = 1; i < argc; ++i)
        if (std::string(argv[i]) == "--test")
        {
            std::string path = "flow_test.json";
            if (i+1 < argc && argv[i+1][0] != '-') path = argv[i+1];
            return runTests(path);
        }

    // --test-new
    for (int i = 1; i < argc; ++i)
        if (std::string(argv[i]) == "--test-new")
            return runNewFeatureTests();

    // --repl
    for (int i = 1; i < argc; ++i)
        if (std::string(argv[i]) == "--repl")
            return runRepl();

    // No file argument → built-in demo (legacy behavior)
    if (argc < 2)
    {
        std::cout << "[Demo] No file specified. Run with --help for usage.\n";
        std::cout << "Running built-in math pipeline demo...\n\n";
        // Minimal inline demo so the binary is still useful without args
        BlueprintRunner runner;
        runner.SetPrintCallback([](LogLevel, const std::string& m){ std::cout << m << "\n"; });
        runner.RegisterHandler("constant_float", [](ExecutionContext& ctx){
            ctx.SetOutputValue("Value", ctx.GetNodeData("value")); return true;
        });
        runner.RegisterHandler("math_add", [](ExecutionContext& ctx){
            double r = ctx.GetInputValue("A").asFloat() + ctx.GetInputValue("B").asFloat();
            ctx.SetOutputValue("Result", Variant(r));
            std::cout << "  " << ctx.GetInputValue("A").asFloat() << " + "
                      << ctx.GetInputValue("B").asFloat() << " = " << r << "\n";
            return true;
        });
        runner.RegisterHandler("output_display", [](ExecutionContext& ctx){
            std::cout << "[result] " << ctx.GetInputValue("Input").asFloat() << "\n";
            return true;
        });

        BlueprintData bp; bp.metadata.name = "Demo";
        auto mkNode = [](int id, const std::string& def, int outId, std::initializer_list<int> inIds) {
            NodeInstance n; n.id=id; n.definitionId=def;
            for (int pid : inIds) { PinInfo p; p.id=pid; p.kind=PinKind::Input;  p.name="dummy"; n.pins.push_back(p); }
            { PinInfo p; p.id=outId; p.kind=PinKind::Output; p.name="Value"; n.pins.push_back(p); }
            return n;
        };
        // constant A=10
        { auto n = mkNode(1,"constant_float",101,{}); n.nodeData["value"]=Variant(10.0); bp.nodes.push_back(n); }
        // constant B=3
        { auto n = mkNode(2,"constant_float",201,{}); n.nodeData["value"]=Variant(3.0);  bp.nodes.push_back(n); }
        // add
        {
            NodeInstance n; n.id=3; n.definitionId="math_add";
            PinInfo a; a.id=301; a.kind=PinKind::Input;  a.name="A"; n.pins.push_back(a);
            PinInfo b; b.id=302; b.kind=PinKind::Input;  b.name="B"; n.pins.push_back(b);
            PinInfo r; r.id=303; r.kind=PinKind::Output; r.name="Result"; n.pins.push_back(r);
            bp.nodes.push_back(n);
        }
        // display
        {
            NodeInstance n; n.id=4; n.definitionId="output_display";
            PinInfo i; i.id=401; i.kind=PinKind::Input; i.name="Input"; n.pins.push_back(i);
            bp.nodes.push_back(n);
        }
        bp.links.push_back({1001,101,301}); bp.links.push_back({1002,201,302});
        bp.links.push_back({1003,303,401});
        bp.rebuildIndices();
        runner.Load(bp);
        auto res = runner.Execute();
        std::cout << "\n" << (res.success ? "OK" : "FAILED") << "\n";
        return res.success ? 0 : 1;
    }

    // ── Parse arguments ──────────────────────────────────────────────────────
    RunOptions opt;
    opt.filePath = argv[1];
    bool watch = false;
    bool eventsExplicit = false;

    for (int i = 2; i < argc; ++i)
    {
        std::string a = argv[i];

        if ((a == "-v" || a == "--var") && i+1 < argc)
        {
            std::string key, val;
            if (parseKV(argv[++i], key, val)) opt.vars.emplace_back(key, val);
            else std::cerr << "WARNING: bad -v format: " << argv[i] << "\n";
        }
        else if ((a == "-e" || a == "--event") && i+1 < argc)
        {
            if (!eventsExplicit) { opt.events.clear(); eventsExplicit = true; }
            std::string ev = argv[++i];
            if (!ev.empty()) opt.events.push_back(ev);
        }
        else if (a == "--lua" && i+1 < argc) opt.luaScript = argv[++i];
        else if (a == "--no-entry-lua") opt.noEntryLua = true;
        else if (a == "--max-time" && i+1 < argc) opt.maxTimeSec = std::stof(argv[++i]);
        else if (a == "--tick-rate" && i+1 < argc) opt.tickRateMs = std::stoi(argv[++i]);
        else if (a == "--no-deps") opt.noDeps = true;
        else if (a == "-q" || a == "--quiet") opt.quiet = true;
        else if (a == "--watch") watch = true;
        else if (a == "--output" && i+1 < argc) opt.outputFmt = argv[++i];
        else if (a == "--output") opt.outputFmt = "vars";  // bare --output
        else {
            std::cerr << "Unknown option: " << a << ". Use --help.\n";
            return 1;
        }
    }

    signal(SIGINT, signalHandler);

    if (watch) return runWatch(opt);
    return executeSingle(opt);
}

// ============================================================================
// Unit test implementations (Test 1-7, same logic as before)
// ============================================================================

static void test_registry_no_dangling_ptr()
{
    std::cout << "\n[Test 1] DefaultNodeRegistry – rehash dangling pointer fix\n";
    DefaultNodeRegistry reg;
    for (int i = 0; i < 200; ++i) {
        NodeDefinition def; def.id = "node_" + std::to_string(i); def.name = "Node " + std::to_string(i);
        reg.registerNode(def);
    }
    const auto& defs = reg.getAllNodeDefinitions();
    CHECK(defs.size() == 200, "200 nodes registered");
    bool ok = true;
    for (const auto* p : defs) { if (!p||p->id.empty()||reg.getNodeDefinition(p->id)!=p){ok=false;break;} }
    CHECK(ok, "All cached pointers valid");
    NodeDefinition upd; upd.id="node_0"; upd.name="Updated";
    reg.registerNode(upd);
    const auto* p0 = reg.getNodeDefinition("node_0");
    CHECK(p0 && p0->name == "Updated", "Update works, pointer still valid");
    CHECK(reg.getAllNodeDefinitions().size() == 200, "Size unchanged after update");
}

static void test_variant_conversion()
{
    std::cout << "\n[Test 2] Variant asInt/asFloat (strtoll/strtod)\n";
    Variant vs(std::string("42"));
    CHECK(vs.asInt()==42, "\"42\"→asInt==42");
    CHECK(vs.asFloat()==42.0, "\"42\"→asFloat==42.0");
    Variant vf(std::string("3.14"));
    CHECK(vf.asFloat()>3.13&&vf.asFloat()<3.15, "\"3.14\"→asFloat≈3.14");
    CHECK(vf.asInt()==3, "\"3.14\"→asInt==3");
    Variant vb(std::string("abc"));
    CHECK(vb.asInt()==0, "\"abc\"→asInt==0");
    CHECK(vb.asFloat()==0.0, "\"abc\"→asFloat==0.0");
}

static void test_variant_map()
{
    std::cout << "\n[Test 3] Variant Map API\n";
    Variant m;
    m.mapSet("foo", Variant(std::string("bar")));
    m.mapSet("count", Variant(int64_t(42)));
    CHECK(m.mapHasKey("foo"),  "mapHasKey foo");
    CHECK(!m.mapHasKey("baz"), "!mapHasKey baz");
    CHECK(m.mapGet("foo").asString()=="bar", "mapGet foo==bar");
    CHECK(m.mapGet("count").asInt()==42, "mapGet count==42");
    m.mapRemove("foo");
    CHECK(m.mapSize()==1, "mapSize==1 after remove");
}

static void test_flow_blueprint(const std::string& flowTestPath)
{
    std::cout << "\n[Test 4] BlueprintRunner – flow_test.json\n";
    BlueprintRunner runner;
    RegisterBuiltinHandlers(runner, ".");
    std::vector<std::string> logs;
    runner.SetLogCallback([&](LogLevel, const std::string& m){ logs.push_back(m); });
    runner.SetPrintCallback([&](LogLevel, const std::string& m){ logs.push_back(m); });
    bool loaded = runner.LoadFromFile(flowTestPath);
    CHECK(loaded, "flow_test.json loaded");
    if (!loaded) { std::cout << "  Error: " << runner.GetLastError() << "\n"; return; }
    auto result = runner.Execute();
    CHECK(result.success, "Execute success");
    CHECK(runner.GetVariable("counter").asInt()==10, "counter==10 (0+1+2+3+4)");
    bool foundTrue = false;
    for (auto& l : logs) if (l.find("TRUE")!=std::string::npos){foundTrue=true;break;}
    CHECK(foundTrue, "Branch True branch executed");
}

static void test_map_nodes()
{
    std::cout << "\n[Test 5] Map nodes end-to-end\n";
    BlueprintRunner runner;
    RegisterBuiltinHandlers(runner, ".");
    BlueprintData bp; bp.metadata.name="MapTest";
    {
        NodeInstance n; n.id=1; n.definitionId="MapSet";
        PinInfo p0; p0.id=11; p0.kind=PinKind::Input; p0.isExec=true; n.pins.push_back(p0);
        PinInfo p1; p1.id=12; p1.kind=PinKind::Input; p1.dataType=PinDataType::Map; p1.name="Map"; n.pins.push_back(p1);
        PinInfo p2; p2.id=13; p2.kind=PinKind::Input; p2.dataType=PinDataType::Any; p2.name="Key"; p2.defaultValue=Variant(std::string("hello")); n.pins.push_back(p2);
        PinInfo p3; p3.id=14; p3.kind=PinKind::Input; p3.dataType=PinDataType::Any; p3.name="Value"; p3.defaultValue=Variant(std::string("world")); n.pins.push_back(p3);
        PinInfo p4; p4.id=15; p4.kind=PinKind::Output; p4.isExec=true; n.pins.push_back(p4);
        PinInfo p5; p5.id=16; p5.kind=PinKind::Output; p5.dataType=PinDataType::Map; p5.name="Map"; n.pins.push_back(p5);
        bp.nodes.push_back(n);
    }
    {
        NodeInstance n; n.id=2; n.definitionId="MapGet";
        PinInfo p0; p0.id=21; p0.kind=PinKind::Input; p0.dataType=PinDataType::Map; p0.name="Map"; n.pins.push_back(p0);
        PinInfo p1; p1.id=22; p1.kind=PinKind::Input; p1.dataType=PinDataType::Any; p1.name="Key"; p1.defaultValue=Variant(std::string("hello")); n.pins.push_back(p1);
        PinInfo p2; p2.id=23; p2.kind=PinKind::Output; p2.dataType=PinDataType::Any; p2.name="Value"; n.pins.push_back(p2);
        PinInfo p3; p3.id=24; p3.kind=PinKind::Output; p3.dataType=PinDataType::Boolean; p3.name="Found"; n.pins.push_back(p3);
        bp.nodes.push_back(n);
    }
    { LinkInstance lk; lk.id=1; lk.startPinId=16; lk.endPinId=21; bp.links.push_back(lk); }
    bp.rebuildIndices();
    bool loaded = runner.Load(bp);
    CHECK(loaded, "Map blueprint loaded");
    Variant em; em.type=PinDataType::Map;
    runner.SetPinValue(12, em);
    CHECK(runner.ExecuteNode(1).success, "MapSet success");
    CHECK(runner.ExecuteNode(2).success, "MapGet success");
    CHECK(runner.GetPinValue(24).asBool(), "Found==true");
    CHECK(runner.GetPinValue(23).asString()=="world", "Value==world");
}

static void test_crude_json_utf8()
{
    std::cout << "\n[Test 6] crude_json UTF-8\n";
    auto v = crude_json::value::parse(u8"{\"name\": \"蓝图测试\", \"value\": 42}");
    CHECK(!v.is_discarded(), "UTF-8 JSON parsed");
    CHECK(v["name"].get<std::string>()==u8"蓝图测试", "Chinese field value correct");
    CHECK(v["value"].get<double>()==42.0, "Numeric field correct");
}

static void test_add_type_promotion()
{
    std::cout << "\n[Test 7] Add type promotion\n";
    Variant a(int64_t(3)), b(int64_t(4));
    CHECK(a.type==PinDataType::Integer, "a is Integer");
    Variant r(a.asInt()+b.asInt());
    CHECK(r.type==PinDataType::Integer, "Integer+Integer→Integer");
    CHECK(r.asInt()==7, "3+4==7");
    CHECK(r.asString()=="7", "asString no float format");
}
