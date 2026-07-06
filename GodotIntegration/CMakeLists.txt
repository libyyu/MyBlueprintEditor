# CMakeLists.txt - GDExtension that wraps BlueprintRuntime for Godot 4.x
# =============================================================================
# Builds: blueprint_gdext — a library Godot loads via blueprint.gdextension.
# Links against:
#   - godot-cpp (branch matching your engine, built as a subdirectory)
#   - BlueprintRuntime (your C++ engine): shared on desktop, STATIC on Web/iOS.
#
# This file is CROSS-PLATFORM. It mirrors the engine's own conventions
# (see ../Runtime/CMakeLists.txt): platform detection via WIN32 / APPLE /
# ANDROID / EMSCRIPTEN / CMAKE_SYSTEM_NAME, and an opt-out Lua switch.
#
# -----------------------------------------------------------------------------
# Options (mirror the engine)
# -----------------------------------------------------------------------------
#   GDEXT_WITH_LUA   ON  → compile the Scene/UI Lua bindings (godot_lua_bindings)
#                          and enable BLUEPRINT_HAS_LUA. Must match how the
#                          engine was built (engine option BLUEPRINT_LUA).
#                    OFF → skip Lua bindings entirely (engine built with
#                          BLUEPRINT_LUA=OFF / build-nolua).
#   BP_BUILD_DIR     Which engine build dir to source artifacts/headers from,
#                    e.g. build-windows / build-wasm / build-android. Auto-
#                    detected per platform if not set.
#
# -----------------------------------------------------------------------------
# Configure examples
# -----------------------------------------------------------------------------
#   Windows (MSVC):
#     cmake -B build -G "Visual Studio 17 2022" -A x64
#     cmake --build build --config Release --target blueprint_gdext
#   Linux/macOS:
#     cmake -B build -DCMAKE_BUILD_TYPE=Release
#     cmake --build build --target blueprint_gdext
#   Android (arm64):
#     cmake -B build-android -DCMAKE_TOOLCHAIN_FILE=$NDK/build/cmake/android.toolchain.cmake \
#           -DANDROID_ABI=arm64-v8a -DANDROID_PLATFORM=android-24 -DBP_BUILD_DIR=build-android
#   Web (Emscripten) — engine is STATIC .a, so the extension statically links it:
#     emcmake cmake -B build-wasm -DBP_BUILD_DIR=build-wasm
#   iOS — engine is STATIC:
#     cmake -B build-ios -DCMAKE_SYSTEM_NAME=iOS -DBP_BUILD_DIR=build-ios
# =============================================================================

cmake_minimum_required(VERSION 3.20)
project(blueprint_gdext LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

option(GDEXT_WITH_LUA "Compile the Scene/UI Lua bindings (must match engine BLUEPRINT_LUA)" ON)

# ---------------------------------------------------------------------------
# Platform classification (matches the engine's scheme)
# ---------------------------------------------------------------------------
set(_IS_WEB    FALSE)
set(_IS_IOS    FALSE)
set(_IS_ANDROID FALSE)
if(EMSCRIPTEN)
    set(_IS_WEB TRUE)
elseif(CMAKE_SYSTEM_NAME STREQUAL "iOS")
    set(_IS_IOS TRUE)
elseif(ANDROID OR CMAKE_SYSTEM_NAME STREQUAL "Android")
    set(_IS_ANDROID TRUE)
endif()

# On Web and iOS the engine is a STATIC library (no dynamic loading allowed),
# so the GDExtension must statically link BlueprintRuntime + Lua into itself.
# Everywhere else the engine is a shared library we link by import lib and copy
# next to the extension.
if(_IS_WEB OR _IS_IOS)
    set(_LINK_ENGINE_STATIC TRUE)
else()
    set(_LINK_ENGINE_STATIC FALSE)
endif()

# Short platform tag used both for the default build dir and Godot's output name.
if(WIN32)
    set(_PLAT_TAG "windows")
elseif(_IS_WEB)
    set(_PLAT_TAG "web")
elseif(_IS_IOS)
    set(_PLAT_TAG "ios")
elseif(_IS_ANDROID)
    set(_PLAT_TAG "android")
elseif(APPLE)
    set(_PLAT_TAG "macos")
elseif(UNIX)
    set(_PLAT_TAG "linux")
else()
    set(_PLAT_TAG "unknown")
endif()

# Architecture tag for Godot's expected filename.
if(_IS_WEB)
    set(_ARCH_TAG "wasm32")
elseif(_IS_ANDROID)
    set(_ARCH_TAG "${ANDROID_ABI}")   # e.g. arm64-v8a
elseif(_IS_IOS)
    set(_ARCH_TAG "arm64")
else()
    set(_ARCH_TAG "x86_64")
endif()

message(STATUS "[blueprint_gdext] platform=${_PLAT_TAG} arch=${_ARCH_TAG} "
               "static_engine=${_LINK_ENGINE_STATIC} lua=${GDEXT_WITH_LUA}")

# ---------------------------------------------------------------------------
# Paths
# ---------------------------------------------------------------------------
get_filename_component(BP_REPO_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/.." ABSOLUTE)
set(BP_RUNTIME_INCLUDE "${BP_REPO_ROOT}/Runtime")

# Which engine build directory to source artifacts + lua headers from.
# Auto-pick a sensible default per platform; override with -DBP_BUILD_DIR=...
if(NOT DEFINED BP_BUILD_DIR OR BP_BUILD_DIR STREQUAL "")
    if(WIN32)
        set(BP_BUILD_DIR "build-windows")
    elseif(_IS_WEB)
        set(BP_BUILD_DIR "build-wasm")
    else()
        # Linux/macOS/Android/iOS: engine build dir is project-specific; caller
        # should pass -DBP_BUILD_DIR. Fall back to a per-platform default name.
        set(BP_BUILD_DIR "build-${_PLAT_TAG}")
    endif()
endif()
set(BP_BUILD_ABS "${BP_REPO_ROOT}/${BP_BUILD_DIR}")
message(STATUS "[blueprint_gdext] engine build dir: ${BP_BUILD_ABS}")

# ---------------------------------------------------------------------------
# godot-cpp (submodule at GodotIntegration/godot-cpp)
# ---------------------------------------------------------------------------
if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/godot-cpp/CMakeLists.txt")
    message(FATAL_ERROR
        "godot-cpp not found. Pull the submodule first:\n"
        "  git submodule update --init --recursive")
endif()

# ---------------------------------------------------------------------------
# Web(Emscripten)：强制 godot-cpp 关掉 pthreads/共享内存，与引擎静态库对齐。
#
# 原因：build.sh wasm 编 BlueprintRuntime.a 时没加 -pthread（含 NodeDefinition.cpp.o
# 等所有 .o 都没有 atomics/bulk-memory feature 标记）。若 godot-cpp 默认开
# GODOTCPP_THREADS=ON → 链接期传 -sUSE_PTHREADS=1 → wasm-ld 拒绝混链：
#   error: --shared-memory is disallowed by NodeDefinition.cpp.o because it was
#          not compiled with 'atomics' or 'bulk-memory' features.
#
# 若日后要 Godot Web 多线程（需 SharedArrayBuffer + COOP/COEP header），必须：
#   1) 用 -pthread 重编 build-wasm 里的引擎全部 .a
#   2) 移除下面这行（或改成 ON）
#   3) 用 threads-enabled Godot Web export template
# ---------------------------------------------------------------------------
if(_IS_WEB)
    set(GODOTCPP_THREADS OFF CACHE BOOL "Disable pthreads to match single-threaded engine .a" FORCE)
endif()

add_subdirectory(godot-cpp)

# ---------------------------------------------------------------------------
# Sources — Lua bindings are conditional on GDEXT_WITH_LUA
# ---------------------------------------------------------------------------
set(BP_GDEXT_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/src/register_types.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/blueprint_node.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/game_launcher.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/godot_file_bridge.cpp")

if(GDEXT_WITH_LUA)
    list(APPEND BP_GDEXT_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/godot_lua_bindings.cpp")
endif()

# Web：补 saveSetjmp/testSetjmp stub。emsdk 3.1.50 编 Lua ldo.c 会残留旧模式
# SjLj 符号(saveSetjmp/testSetjmp)，而 Godot 引擎主模块只提供新模式
# (__wasm_longjmp/__c_longjmp)，导致 side module 加载时 undefined symbol abort。
# 这两个旧符号运行时不会被真正调用，提供空实现让 dylink 能解析即可。见 web_sjlj_stub.c。
if(_IS_WEB)
    list(APPEND BP_GDEXT_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/web_sjlj_stub.c")
endif()

add_library(blueprint_gdext SHARED ${BP_GDEXT_SOURCES})

target_include_directories(blueprint_gdext PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/src"
    "${BP_RUNTIME_INCLUDE}")

target_link_libraries(blueprint_gdext PRIVATE godot-cpp)

# ---------------------------------------------------------------------------
# Emscripten (Web) 的 SIDE_MODULE + PUBLIC 编译/链接 flags 全部由 godot-cpp 的
# cmake/web.cmake 提供（-sSIDE_MODULE=1 / -sSUPPORT_LONGJMP=wasm / -fPIC 等）。
# 我们已在 add_subdirectory(godot-cpp) 之前通过 GODOTCPP_THREADS=OFF 与引擎 .a
# 对齐（禁用 pthreads/共享内存），此处无需再自定义 target_link_options。
#
# 切勿在这里补一句 -sUSE_PTHREADS=1！那会与引擎 .a 不匹配，触发 wasm-ld:
#   --shared-memory is disallowed by <engine>.o
# ---------------------------------------------------------------------------

# ---------------------------------------------------------------------------
# Web(Emscripten)：让产物成为带 dylink 段的真正 SIDE_MODULE。
#
# 现象：运行时浏览器报
#   Uncaught Error: need the dylink section to be first
# 且链接时 em++ 警告：
#   linking a library with `-shared` will emit a static object file ...
#   If you want to build a runtime shared library use the SIDE_MODULE setting.
#
# 真因（从 build.ninja 的 LINK_FLAGS 反查确认）：
#   LINK_FLAGS = -shared ... -s        ← 注意结尾的 -s，且【没有】 -sSIDE_MODULE=1
#   -sSIDE_MODULE=1 只加在 compile 阶段（godot-cpp target_compile_options），
#   link 阶段没传，于是 emcc 退回 `-shared` 模拟模式 → emit 静态 object →
#   产物根本没有 dylink 段（不是被重排，是压根没生成）。
#   结尾的 -s 是 strip，同样有害。
#
# 修复：在 link 阶段【显式】补 -sSIDE_MODULE=1，让 emcc 产真正的 side module
#   （带 dylink 段且在首位）；不要去动 CMAKE_*_LINKER_FLAGS_<CONFIG>
#   （上一版清空它们无效，因为 -s 来自别处）。
#   注意：SIDE_MODULE 下不能再叠 -O0/wasm-opt 相关魔改，交给 emcc 默认处理。
# ---------------------------------------------------------------------------
if(_IS_WEB)
    # 关键修复：link 阶段显式补 -sSIDE_MODULE=1。
    #
    # 现象：运行时浏览器报 "need the dylink section to be first"。
    # 真因（从 build.ninja 的 LINK_FLAGS 反查确认）：-sSIDE_MODULE=1 只加在了
    #   compile 阶段（godot-cpp target_compile_options），link 阶段没传，于是 emcc
    #   退回 `-shared` 模拟模式 emit 静态 object → 产物根本没有 dylink 段。
    # 修复后 check_dylink.py 确认：首段变为 custom 'dylink.0' → Godot Web 可加载。
    #
    # 注：LINK_FLAGS 末尾的 -s(来自 CMAKE_SHARED_LINKER_FLAGS_RELEASE 默认值)在
    #   SIDE_MODULE 模式下并不破坏段布局，实测保留也能正常出 dylink，无需清除。
    target_link_options(blueprint_gdext PRIVATE -sSIDE_MODULE=1)
    message(STATUS "[blueprint_gdext] web: force -sSIDE_MODULE=1 at link "
                   "(emit real side module with dylink section first)")

    # setjmp/longjmp：Lua(ldo.c) 用 setjmp 做错误处理。emcc 编 .c 时 .o 里总会留
    # saveSetjmp/testSetjmp/emscripten_longjmp 字符串；它们是走 emscripten 模式还是
    # wasm 原生 SjLj，由【最终 link 阶段】的 -sSUPPORT_LONGJMP 决定。
    # 若 link 期没传 wasm，.so 里这三个就变成真·未定义 import（MAIN_MODULE 引擎不
    # 提供）→ 运行时 Aborted: undefined symbol 'saveSetjmp'。
    # 所以这里【在 link 阶段显式补】，把 Lua 的 setjmp 转成 wasm SjLj，消除 import。
    target_link_options(blueprint_gdext PRIVATE -sSUPPORT_LONGJMP=wasm)
    message(STATUS "[blueprint_gdext] web: link -sSUPPORT_LONGJMP=wasm "
                   "(resolve Lua setjmp to native wasm SjLj, drop saveSetjmp import)")

    # 调试期：保留函数名，让浏览器 RuntimeError 栈显示符号而非裸地址。
    # 定位完 null function 崩溃点后可去掉（-g2 会显著增大 .so / 拖慢加载）。
    option(GDEXT_WEB_DEBUG_SYMBOLS "Keep wasm function names for stack traces" ON)
    if(GDEXT_WEB_DEBUG_SYMBOLS)
        target_link_options(blueprint_gdext PRIVATE -g2)
        message(STATUS "[blueprint_gdext] web: -g2 (debug symbols in stack traces)")
    endif()
endif()

# ---------------------------------------------------------------------------
# Lua wiring (only when GDEXT_WITH_LUA). We compile godot_lua_bindings.cpp which
# uses the Lua C API against the runner's lua_State (via BP_GetLuaState). The
# engine header gates BP_GetLuaState behind BLUEPRINT_HAS_LUA, so define it.
# Lua headers come from the engine's FetchContent'd copy in the chosen build dir.
# ---------------------------------------------------------------------------
if(GDEXT_WITH_LUA)
    set(BP_LUA_INCLUDE "${BP_BUILD_ABS}/_deps/lua54-src/src")
    if(NOT EXISTS "${BP_LUA_INCLUDE}/lua.h")
        message(WARNING
            "Lua headers not found at ${BP_LUA_INCLUDE}. Build the engine WITH Lua "
            "in ${BP_BUILD_DIR} first, or set -DBP_BUILD_DIR to a build that has "
            "_deps/lua54-src, or configure with -DGDEXT_WITH_LUA=OFF.")
    endif()
    target_include_directories(blueprint_gdext PRIVATE "${BP_LUA_INCLUDE}")
    target_compile_definitions(blueprint_gdext PRIVATE BLUEPRINT_HAS_LUA=1)
endif()

# ---------------------------------------------------------------------------
# Link BlueprintRuntime — shared (import lib) on desktop/android,
#                         static (.a, absorb lua) on web/ios.
# ---------------------------------------------------------------------------
if(_IS_WEB)
    # Web(Emscripten)：链接【多个原子静态库】，而不是 libBlueprintBundle.a。
    #
    # 为什么不用 Bundle：libBlueprintBundle.a 是"库套库"（嵌套 .a——成员是别的 .a
    #   而非 .o），只有桌面/MSVC 链接器能吃。emscripten 的 wasm-ld 不认嵌套归档：
    #     wasm-ld: error: unknown file type: libBlueprintBundle.a(libBlueprintRuntime.a)
    #
    # 解法：直接平级链 build-wasm 里的原子库（内容都是 .o）：
    #   Runtime + lua54 + protobuf(+ luasocket/json 若被引用)。
    #   用 --whole-archive 强制把符号全打进 side module（否则 --gc-sections 会把
    #   "Runtime 内部引用、GDExtension 没直接调"的 protobuf 实现 gc 成 undefined
    #   import → 运行时 reportUndefinedSymbols 崩在 kTypeToCppTypeMap 等符号）。
    set(_BP_WEB_ARCHIVES "")
    foreach(_rel
            "Runtime/libBlueprintRuntime.a"
            "Runtime/liblua54.a"
            "external/protobuf/libprotobuf_s.a"
            "external/lprotos/liblprotos_s.a"
            "Utils/Json/libcrude_json.a")
        set(_abs "${BP_BUILD_ABS}/${_rel}")
        if(EXISTS "${_abs}")
            list(APPEND _BP_WEB_ARCHIVES "${_abs}")
        endif()
    endforeach()
    if(NOT _BP_WEB_ARCHIVES)
        message(FATAL_ERROR
            "[blueprint_gdext] No atomic engine archives found under ${BP_BUILD_ABS}.\n"
            "Build the engine for wasm first (build.sh wasm).")
    endif()
    # 整个 archive 列表包在一对 whole-archive 之间
    target_link_options(blueprint_gdext PRIVATE
        "-Wl,--whole-archive" ${_BP_WEB_ARCHIVES} "-Wl,--no-whole-archive")
    message(STATUS "[blueprint_gdext] web: whole-archive link atomic libs: ${_BP_WEB_ARCHIVES}")

elseif(_LINK_ENGINE_STATIC)
    # iOS: 静态链引擎 .a（iOS 工具链能吃 Bundle，若有则优先）
    set(BP_RUNTIME_STATIC "")
    foreach(_cand
            "${BP_BUILD_ABS}/bin/Release/libBlueprintBundle.a"
            "${BP_BUILD_ABS}/bin/libBlueprintBundle.a"
            "${BP_BUILD_ABS}/Runtime/libBlueprintRuntime.a"
            "${BP_REPO_ROOT}/dist/UnityPlugins/WebGL/libBlueprintRuntime.a")
        if(NOT BP_RUNTIME_STATIC AND EXISTS "${_cand}")
            set(BP_RUNTIME_STATIC "${_cand}")
        endif()
    endforeach()
    if(BP_RUNTIME_STATIC)
        target_link_libraries(blueprint_gdext PRIVATE "${BP_RUNTIME_STATIC}")
        message(STATUS "[blueprint_gdext] static-linking engine: ${BP_RUNTIME_STATIC}")
    else()
        message(FATAL_ERROR
            "Static engine archive not found for ${_PLAT_TAG}.")
    endif()
else()
    # Desktop (Win/Linux/macOS) + Android: shared engine.
    if(WIN32)
        set(BP_RUNTIME_SHARED "${BP_BUILD_ABS}/bin/Release/BlueprintRuntime.dll")
        set(BP_RUNTIME_IMPORT "${BP_BUILD_ABS}/Runtime/Release/BlueprintRuntime.lib")
        if(NOT EXISTS "${BP_RUNTIME_SHARED}")
            set(BP_RUNTIME_SHARED "${BP_REPO_ROOT}/dist/UnityPlugins/Windows/x86_64/BlueprintRuntime.dll")
        endif()
    elseif(APPLE)
        set(BP_RUNTIME_SHARED "${BP_BUILD_ABS}/bin/libBlueprintRuntime.dylib")
        set(BP_RUNTIME_IMPORT "${BP_RUNTIME_SHARED}")   # dylib links directly
    else() # Linux / Android
        set(BP_RUNTIME_SHARED "${BP_BUILD_ABS}/bin/libBlueprintRuntime.so")
        set(BP_RUNTIME_IMPORT "${BP_RUNTIME_SHARED}")   # so links directly
    endif()

    if(EXISTS "${BP_RUNTIME_IMPORT}")
        target_link_libraries(blueprint_gdext PRIVATE "${BP_RUNTIME_IMPORT}")
    else()
        message(WARNING
            "BlueprintRuntime link library not found at ${BP_RUNTIME_IMPORT}.\n"
            "Build the engine (shared) for ${_PLAT_TAG} in ${BP_BUILD_DIR} first.")
    endif()
endif()

# ---------------------------------------------------------------------------
# Output naming: Godot expects  <name>.<platform>.<config>.<arch>.<ext>
# We keep a single fixed name that the .gdextension manifest points at.
# NOTE: kept as template_debug to match the existing manifest; adjust per your
# export config if you ship template_release separately.
# ---------------------------------------------------------------------------
set(BP_OUT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/game/bin")
set_target_properties(blueprint_gdext PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY           "${BP_OUT_DIR}"
    RUNTIME_OUTPUT_DIRECTORY_DEBUG     "${BP_OUT_DIR}"
    RUNTIME_OUTPUT_DIRECTORY_RELEASE   "${BP_OUT_DIR}"
    LIBRARY_OUTPUT_DIRECTORY "${BP_OUT_DIR}"
    LIBRARY_OUTPUT_DIRECTORY_DEBUG "${BP_OUT_DIR}"
    LIBRARY_OUTPUT_DIRECTORY_DEBUG "${BP_OUT_DIR}"
    PREFIX ""
    OUTPUT_NAME "blueprint_gdext.${_PLAT_TAG}.template_debug.${_ARCH_TAG}")

# ---------------------------------------------------------------------------
# Copy the engine shared library next to the extension (desktop/android only).
# Web/iOS static-link the engine into the extension, so nothing to copy.
# ---------------------------------------------------------------------------
if(NOT _LINK_ENGINE_STATIC AND DEFINED BP_RUNTIME_SHARED)
    add_custom_command(TARGET blueprint_gdext POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory "${BP_OUT_DIR}"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "${BP_RUNTIME_SHARED}" "${BP_OUT_DIR}/"
        COMMENT "Copying BlueprintRuntime shared library next to the GDExtension")
endif()
