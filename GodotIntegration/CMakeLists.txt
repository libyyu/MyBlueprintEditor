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
add_subdirectory(godot-cpp)

# ---------------------------------------------------------------------------
# Sources — Lua bindings are conditional on GDEXT_WITH_LUA
# ---------------------------------------------------------------------------
set(BP_GDEXT_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/src/register_types.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/blueprint_node.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/game_launcher.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/godot_file_bridge.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/godot_lua_loader.cpp")

if(GDEXT_WITH_LUA)
    list(APPEND BP_GDEXT_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/godot_lua_bindings.cpp")
endif()

add_library(blueprint_gdext SHARED ${BP_GDEXT_SOURCES})

target_include_directories(blueprint_gdext PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/src"
    "${BP_RUNTIME_INCLUDE}")

target_link_libraries(blueprint_gdext PRIVATE godot-cpp)

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
if(_LINK_ENGINE_STATIC)
    # Web / iOS: statically link the engine .a (it already bakes Lua in).
    set(BP_RUNTIME_STATIC "${BP_BUILD_ABS}/Runtime/libBlueprintRuntime.a")
    if(NOT EXISTS "${BP_RUNTIME_STATIC}")
        # dist/ fallback (e.g. prebuilt WebGL archive)
        set(BP_RUNTIME_STATIC "${BP_REPO_ROOT}/dist/UnityPlugins/WebGL/libBlueprintRuntime.a")
    endif()
    if(EXISTS "${BP_RUNTIME_STATIC}")
        target_link_libraries(blueprint_gdext PRIVATE "${BP_RUNTIME_STATIC}")
        message(STATUS "[blueprint_gdext] static-linking engine: ${BP_RUNTIME_STATIC}")
    else()
        message(FATAL_ERROR
            "Static engine archive not found for ${_PLAT_TAG}.\n"
            "Expected: ${BP_RUNTIME_STATIC}\n"
            "Build the engine as STATIC for this platform first.")
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
    RUNTIME_OUTPUT_DIRECTORY "${BP_OUT_DIR}"
    LIBRARY_OUTPUT_DIRECTORY "${BP_OUT_DIR}"
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
