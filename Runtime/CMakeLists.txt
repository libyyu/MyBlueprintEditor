# ==============================================================================
# BlueprintRuntime – cross-platform library (static or shared)
# ==============================================================================
#
# Targets exported by this file:
#   BlueprintRuntime   – the library target (STATIC or SHARED)
#
# Key options (set from parent or command-line):
#   BUILD_SHARED_LIBS          ON  → shared (.dll / .so / .dylib)
#                              OFF → static (.lib / .a)          [default]
#   BUILD_RUNTIME_ONLY         ON  → skip Editor / Application targets
#   BLUEPRINT_NO_FILESYSTEM    ON  → exclude DefaultFileSystem (Emscripten/WebGL)
#
# Emscripten / Unity WebGL:
#   Pass -DCMAKE_TOOLCHAIN_FILE=cmake/Toolchains/Emscripten.cmake
#   (or let emcmake inject it).  The library is always built as STATIC for
#   Emscripten because Emscripten does not support shared libraries for
#   Unity WebGL builds.
# ==============================================================================

cmake_minimum_required(VERSION 3.12)

# ---------------------------------------------------------------------------
# Detect whether this file is the top-level CMakeLists (standalone Runtime build)
# ---------------------------------------------------------------------------
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    project(BlueprintRuntime VERSION 1.0.0 LANGUAGES C CXX)
    set(BLUEPRINT_STANDALONE TRUE)
    # 独立构建时，根目录默认为上一级
    if(NOT DEFINED BLUEPRINT_ROOT_DIR)
        set(BLUEPRINT_ROOT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/..")
    endif()
    # crude_json 需要在当前项目中构建（非独立构建时由根 CMakeLists 负责）
    add_subdirectory("${BLUEPRINT_ROOT_DIR}/Utils/Json" "${CMAKE_CURRENT_BINARY_DIR}/crude_json")
else()
    set(BLUEPRINT_STANDALONE FALSE)
endif()

# ---------------------------------------------------------------------------
# Emscripten: always static (Unity WebGL requirement)
# ---------------------------------------------------------------------------
if(EMSCRIPTEN)
    set(_RUNTIME_LIB_TYPE STATIC)
    message(STATUS "[BlueprintRuntime] Emscripten detected → forcing STATIC library")
else()
    if(BUILD_SHARED_LIBS)
        set(_RUNTIME_LIB_TYPE SHARED)
    else()
        set(_RUNTIME_LIB_TYPE STATIC)
    endif()
endif()

message(STATUS "[BlueprintRuntime] Building as ${_RUNTIME_LIB_TYPE}")

# ---------------------------------------------------------------------------
# Sources
# ---------------------------------------------------------------------------
set(RUNTIME_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/NodeDefinition.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/BlueprintExporter.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/BlueprintRunner.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/FrameTimerManager.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/MainThreadDispatcher.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/BuiltinNodeDefs.cpp
    # BuiltinHandlers – split by category
    ${CMAKE_CURRENT_SOURCE_DIR}/BuiltinHandlers.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/handlers/BuiltinHandlers_Flow.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/handlers/BuiltinHandlers_Action.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/handlers/BuiltinHandlers_Math.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/handlers/BuiltinHandlers_Debug.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/handlers/BuiltinHandlers_String.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/handlers/BuiltinHandlers_Map.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/handlers/BuiltinHandlers_Array.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/handlers/BuiltinHandlers_Set.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/handlers/BuiltinHandlers_Tree.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/handlers/BuiltinHandlers_Houdini.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/handlers/BuiltinHandlers_Time.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/handlers/BuiltinHandlers_Data.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/handlers/BuiltinHandlers_Misc.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/FileSystem.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/BlueprintCAPI.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/ScriptNodeLoader.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/FunctionLibrary.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/EventBus.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/StringPool.cpp
)

set(RUNTIME_PUBLIC_HEADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/BlueprintExport.h
    ${CMAKE_CURRENT_SOURCE_DIR}/BlueprintCAPI.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Types.h
    ${CMAKE_CURRENT_SOURCE_DIR}/NodeDefinition.h
    ${CMAKE_CURRENT_SOURCE_DIR}/BlueprintData.h
    ${CMAKE_CURRENT_SOURCE_DIR}/BlueprintExporter.h
    ${CMAKE_CURRENT_SOURCE_DIR}/BlueprintRunner.h
    ${CMAKE_CURRENT_SOURCE_DIR}/FrameTimerManager.h
    ${CMAKE_CURRENT_SOURCE_DIR}/MainThreadDispatcher.h
    ${CMAKE_CURRENT_SOURCE_DIR}/BuiltinNodeDefs.h
    ${CMAKE_CURRENT_SOURCE_DIR}/BuiltinHandlers.h
    ${CMAKE_CURRENT_SOURCE_DIR}/FileSystem.h
)

# ---------------------------------------------------------------------------
# Optional: Lua scripting support
# ---------------------------------------------------------------------------
option(BLUEPRINT_LUA "Enable Lua scripting support (requires Lua 5.4)" ON)

# LUA_LINK_STATIC – controls how Lua is linked when BlueprintRuntime is SHARED.
#
#   ON  (default): Lua is built as STATIC and baked into BlueprintRuntime.dll/so/dylib.
#                  Consumers only need the single BlueprintRuntime library file.
#                  System Lua is intentionally skipped (system libs are usually .so/.dylib
#                  only; we use FetchContent to build a .a we can absorb).
#
#   OFF           : Lua follows BUILD_SHARED_LIBS — a separate liblua54.so/dll is produced.
#                  Use when you want to share a Lua VM with another library (e.g. xLua)
#                  via BP_SetExternalLuaState(), and the dynamic Lua is already present.
#
# Has no effect when BlueprintRuntime itself is STATIC (Lua is always baked in that case).
# Has no effect on iOS / Emscripten (always STATIC regardless).
option(LUA_LINK_STATIC "Bake Lua as static into BlueprintRuntime SHARED library" ON)

# Platform capability check:
# - iOS        : STATIC only (App Store forbids dynamic loading of arbitrary .dylib)
# - Emscripten : STATIC only (wasm has no real dlopen; shared libs not supported)
# - Android    : follows LUA_LINK_STATIC when SHARED, else STATIC
# - Others (Windows, Linux, macOS): follows LUA_LINK_STATIC when SHARED, else STATIC
# No platform forces BLUEPRINT_LUA OFF; callers may still set it OFF explicitly.

if(BLUEPRINT_LUA)
    # --- Determine Lua library type per platform ---
    if(EMSCRIPTEN OR (CMAKE_SYSTEM_NAME STREQUAL "iOS"))
        # Must be STATIC regardless of any option
        set(_LUA_LIB_TYPE STATIC)
    elseif(BUILD_SHARED_LIBS AND NOT LUA_LINK_STATIC)
        # Caller explicitly opted out of baking — build Lua as SHARED
        set(_LUA_LIB_TYPE SHARED)
    else()
        # Default: STATIC (baked into BlueprintRuntime, whether it is SHARED or STATIC)
        set(_LUA_LIB_TYPE STATIC)
    endif()
    message(STATUS "[BlueprintRuntime] Lua library type: ${_LUA_LIB_TYPE} (LUA_LINK_STATIC=${LUA_LINK_STATIC})")

    # --- Locate Lua 5.4 ---
    # Priority: find_package first (system / vcpkg / user-installed),
    # fall back to FetchContent if not found.
    #
    # EXCEPTION: when _LUA_LIB_TYPE is STATIC we need a linkable .a archive.
    # System packages (apt liblua5.4-dev, brew lua) typically ship only .so/.dylib,
    # so we skip system discovery in that case and go straight to FetchContent to
    # build our own .a that can be baked into BlueprintRuntime.
    #
    # Ubuntu: apt install liblua5.4-dev → headers at /usr/include/lua5.4,
    #         CMake FindLua may not find it by default; help it via pkg-config.
    # macOS:  brew install lua → pkg-config provides lua5.4 / lua-5.4
    if(_LUA_LIB_TYPE STREQUAL "SHARED")
        # SHARED Lua: system library is fine, try pkg-config / find_package first.
        find_package(PkgConfig QUIET)
        if(PKG_CONFIG_FOUND AND NOT LUA_FOUND)
            pkg_check_modules(LUA54_PC QUIET lua5.4 lua-5.4)
            if(LUA54_PC_FOUND)
                set(LUA_INCLUDE_DIR "${LUA54_PC_INCLUDE_DIRS}")
                set(LUA_LIBRARIES   "${LUA54_PC_LINK_LIBRARIES}")
                set(LUA_FOUND TRUE)
                message(STATUS "[BlueprintRuntime] Lua found via pkg-config: ${LUA_LIBRARIES}")
            endif()
        endif()

        if(NOT LUA_FOUND)
            find_package(Lua 5.4 QUIET)
        endif()
    else()
        # STATIC Lua: skip system discovery — system libs are usually .so/.dylib only.
        # Always build from source via FetchContent so we get a proper .a to bake in.
        message(STATUS "[BlueprintRuntime] Lua STATIC mode: skipping system Lua, using FetchContent")
    endif()

    if(NOT LUA_FOUND)
        message(STATUS "[BlueprintRuntime] Lua not found via find_package/pkg-config, using FetchContent")
        include(FetchContent)
        FetchContent_Declare(lua54
            URL      https://www.lua.org/ftp/lua-5.4.7.tar.gz
            DOWNLOAD_EXTRACT_TIMESTAMP ON
        )
        # Use FetchContent_Populate (instead of MakeAvailable) so we control the
        # source directory ourselves without CMake trying to add_subdirectory.
        # MakeAvailable would also work, but on multi-config generators (Xcode, VS)
        # the source dir may not be populated yet when file(GLOB) runs.
        FetchContent_GetProperties(lua54)
        if(NOT lua54_POPULATED)
            FetchContent_Populate(lua54)
        endif()

        # Build Lua from source (STATIC or SHARED depending on platform)
        set(LUA54_SRC_DIR "${lua54_SOURCE_DIR}/src")

        # Collect Lua core sources (exclude lua.c and luac.c which are standalone executables)
        file(GLOB LUA54_SOURCES "${LUA54_SRC_DIR}/*.c")
        list(REMOVE_ITEM LUA54_SOURCES "${LUA54_SRC_DIR}/lua.c" "${LUA54_SRC_DIR}/luac.c")

        set(_LUA_TARGET_NAME "lua54")
        if(NOT TARGET ${_LUA_TARGET_NAME})
            add_library(${_LUA_TARGET_NAME} ${_LUA_LIB_TYPE} ${LUA54_SOURCES})
            target_include_directories(${_LUA_TARGET_NAME}
                PUBLIC
                    $<BUILD_INTERFACE:${LUA54_SRC_DIR}>
                    $<INSTALL_INTERFACE:include/lua54>
            )

            # Platform-specific compile definitions for Lua
            # Lua 5.4 only recognises: LUA_USE_LINUX, LUA_USE_MACOSX, LUA_USE_POSIX,
            # LUA_USE_DLOPEN, LUA_USE_WINDOWS (implicit default).
            # There is NO LUA_USE_IOS or LUA_USE_ANDROID in the official source.
            if(EMSCRIPTEN)
                # Emscripten: POSIX math/signal subset is available; no readline,
                # no real dlopen (Emscripten's dlopen is a stub — omit LUA_USE_DLOPEN
                # so Lua doesn't try to load dynamic modules at runtime).
                target_compile_definitions(${_LUA_TARGET_NAME} PRIVATE LUA_USE_POSIX)
            elseif(CMAKE_SYSTEM_NAME STREQUAL "iOS")
                # iOS: POSIX math available, but system()/popen()/pclose() are banned
                # by Apple and will cause compile errors on iPhoneSimulator/iphoneos SDK.
                # LUA_USE_POSIX enables POSIX math/signal subset without readline.
                # LUA_NOENV skips LUA_PATH/LUA_CPATH env-var lookup (meaningless on iOS).
                target_compile_definitions(${_LUA_TARGET_NAME} PRIVATE
                    LUA_USE_POSIX
                    LUA_NOENV
                )
                # Stub out the three banned POSIX APIs used in loslib.c.
                # Macros with parentheses/commas must be passed via -D in compile
                # options — target_compile_definitions cannot handle them.
                #   l_system(c)      → os.execute() always fails (returns -1)
                #   lua_popen(L,c,m) → io.popen() always fails (returns NULL)
                #   lua_pclose(L,f)  → io.pclose() no-op (returns 0)
                target_compile_options(${_LUA_TARGET_NAME} PRIVATE
                    "-Dl_system(c)=(-1)"
                    "-Dlua_popen(L,c,m)=(NULL)"
                    "-Dlua_pclose(L,f)=(0)"
                )
            elseif(CMAKE_SYSTEM_NAME STREQUAL "Android")
                # Android NDK: POSIX + dlopen available, no readline.
                # LUA_USE_LINUX enables readline guard but NDK lacks readline headers;
                # use LUA_USE_POSIX + LUA_USE_DLOPEN instead.
                target_compile_definitions(${_LUA_TARGET_NAME} PRIVATE
                    LUA_USE_POSIX
                    LUA_USE_DLOPEN
                )
            elseif(UNIX AND NOT APPLE)
                # Linux and other POSIX systems (readline available)
                target_compile_definitions(${_LUA_TARGET_NAME} PRIVATE LUA_USE_LINUX)
            elseif(APPLE)
                # macOS (not iOS — caught above)
                target_compile_definitions(${_LUA_TARGET_NAME} PRIVATE LUA_USE_MACOSX)
            endif()
            # Windows: no extra define needed (LUA_USE_WINDOWS is the default fallback)

            # SHARED library: export symbols
            if(_LUA_LIB_TYPE STREQUAL "SHARED")
                # Lua 5.4 uses LUA_BUILD_AS_DLL to toggle __declspec(dllexport/dllimport)
                # on Windows; on GCC/Clang we use visibility attributes.
                if(MSVC)
                    target_compile_definitions(${_LUA_TARGET_NAME}
                        PRIVATE LUA_BUILD_AS_DLL    # exporting (building the DLL)
                    )
                    # Consumers also need LUA_BUILD_AS_DLL so that lua.h resolves
                    # to __declspec(dllimport).  However, luaconf.h checks LUA_CORE/LUA_LIB
                    # to decide export vs import, so consumers only need LUA_BUILD_AS_DLL
                    # (without LUA_CORE/LUA_LIB → they get dllimport).
                    # We set it PUBLIC so BlueprintRuntime (consumer) sees it too.
                    target_compile_definitions(${_LUA_TARGET_NAME}
                        PUBLIC LUA_BUILD_AS_DLL
                    )
                else()
                    target_compile_options(${_LUA_TARGET_NAME} PRIVATE
                        -fvisibility=default   # Lua needs all API symbols visible
                    )
                endif()

                # Output the Lua shared library alongside BlueprintRuntime / executables
                set_target_properties(${_LUA_TARGET_NAME} PROPERTIES
                    RUNTIME_OUTPUT_DIRECTORY           "${CMAKE_BINARY_DIR}/bin"
                    RUNTIME_OUTPUT_DIRECTORY_DEBUG     "${CMAKE_BINARY_DIR}/bin/Debug"
                    RUNTIME_OUTPUT_DIRECTORY_RELEASE   "${CMAKE_BINARY_DIR}/bin/Release"
                    LIBRARY_OUTPUT_DIRECTORY           "${CMAKE_BINARY_DIR}/bin"
                    LIBRARY_OUTPUT_DIRECTORY_DEBUG     "${CMAKE_BINARY_DIR}/bin/Debug"
                    LIBRARY_OUTPUT_DIRECTORY_RELEASE   "${CMAKE_BINARY_DIR}/bin/Release"
                )

                # macOS: set install name
                if(APPLE)
                    set_target_properties(${_LUA_TARGET_NAME} PROPERTIES
                        MACOSX_RPATH ON
                        INSTALL_NAME_DIR "@rpath"
                    )
                endif()
            endif()

            # Suppress warnings in third-party Lua code
            if(MSVC)
                target_compile_options(${_LUA_TARGET_NAME} PRIVATE /W0 /utf-8)
            else()
                target_compile_options(${_LUA_TARGET_NAME} PRIVATE -w)
            endif()

            set_property(TARGET ${_LUA_TARGET_NAME} PROPERTY FOLDER "External")
            # Explicitly tell CMake this is a C target (pure C code).
            # Required on Xcode/multi-config generators where CMake cannot always
            # infer the linker language from FetchContent-provided sources.
            set_property(TARGET ${_LUA_TARGET_NAME} PROPERTY LINKER_LANGUAGE C)
            set_property(TARGET ${_LUA_TARGET_NAME} PROPERTY C_STANDARD 99)
            set_property(TARGET ${_LUA_TARGET_NAME} PROPERTY POSITION_INDEPENDENT_CODE ON)
        endif()

        # Provide variables compatible with FindLua
        set(LUA_LIBRARIES ${_LUA_TARGET_NAME})
        set(LUA_INCLUDE_DIR "${LUA54_SRC_DIR}")
    endif()

    # Append Lua binding sources
    list(APPEND RUNTIME_SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/LuaScriptEngine.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/LuaBindings.cpp
    )
    list(APPEND RUNTIME_PUBLIC_HEADERS
        ${CMAKE_CURRENT_SOURCE_DIR}/LuaScriptEngine.h
        ${CMAKE_CURRENT_SOURCE_DIR}/LuaBindings.h
    )
endif()

# ---------------------------------------------------------------------------
# Target
# ---------------------------------------------------------------------------
add_library(BlueprintRuntime ${_RUNTIME_LIB_TYPE}
    ${RUNTIME_SOURCES}
    ${RUNTIME_PUBLIC_HEADERS}
)

# Alias so consumers can always write target_link_libraries(... BlueprintRuntime::BlueprintRuntime)
add_library(BlueprintRuntime::BlueprintRuntime ALIAS BlueprintRuntime)

# ---------------------------------------------------------------------------
# Dependencies
# ---------------------------------------------------------------------------
target_link_libraries(BlueprintRuntime PUBLIC crude_json)

# Lua scripting (conditional)
if(BLUEPRINT_LUA)
    # SHARED Lua: link PUBLIC so consumers get the import library / RPATH.
    # STATIC Lua: link PRIVATE (Lua symbols are baked into BlueprintRuntime).
    if(_LUA_LIB_TYPE STREQUAL "SHARED")
        target_link_libraries(BlueprintRuntime PUBLIC ${LUA_LIBRARIES})
    else()
        target_link_libraries(BlueprintRuntime PRIVATE ${LUA_LIBRARIES})
    endif()
    target_include_directories(BlueprintRuntime PRIVATE ${LUA_INCLUDE_DIR})
    target_compile_definitions(BlueprintRuntime PUBLIC BLUEPRINT_HAS_LUA)
    message(STATUS "[BlueprintRuntime] Lua scripting enabled (${_LUA_LIB_TYPE}, include: ${LUA_INCLUDE_DIR})")
endif()

# ---------------------------------------------------------------------------
# Include directories
# ---------------------------------------------------------------------------
target_include_directories(BlueprintRuntime
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<INSTALL_INTERFACE:include/BlueprintRuntime>
)

# ---------------------------------------------------------------------------
# Compile definitions
# ---------------------------------------------------------------------------

# Shared-library export/import macros
if(_RUNTIME_LIB_TYPE STREQUAL "SHARED")
    # When compiling the DLL itself
    target_compile_definitions(BlueprintRuntime PRIVATE BLUEPRINT_BUILD_DLL)
    # Consumers who link against the target get BLUEPRINT_USE_DLL automatically;
    # they must NOT define BLUEPRINT_BUILD_DLL or BLUEPRINT_STATIC.
    # (BLUEPRINT_API resolves to __declspec(dllimport) / nothing on other platforms)
else()
    # Static: consumers and the library itself both see BLUEPRINT_STATIC
    target_compile_definitions(BlueprintRuntime PUBLIC BLUEPRINT_STATIC)
endif()

# Optional: strip out DefaultFileSystem (useful for Emscripten / Unity)
if(BLUEPRINT_NO_FILESYSTEM OR EMSCRIPTEN_NO_FILESYSTEM)
    target_compile_definitions(BlueprintRuntime PUBLIC BLUEPRINT_NO_FILESYSTEM)
    message(STATUS "[BlueprintRuntime] DefaultFileSystem excluded (BLUEPRINT_NO_FILESYSTEM)")
endif()

# ---------------------------------------------------------------------------
# Compiler flags – platform-specific
# ---------------------------------------------------------------------------
set_target_properties(BlueprintRuntime PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
    POSITION_INDEPENDENT_CODE ON  # required for shared libs on Linux/macOS
)

# Shared library: output .dll/.so/.dylib alongside executables (bin/ directory)
# so that BlueprintEditor.exe can find BlueprintRuntime.dll at runtime.
if(_RUNTIME_LIB_TYPE STREQUAL "SHARED")
    set_target_properties(BlueprintRuntime PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY           "${CMAKE_BINARY_DIR}/bin"
        RUNTIME_OUTPUT_DIRECTORY_DEBUG     "${CMAKE_BINARY_DIR}/bin/Debug"
        RUNTIME_OUTPUT_DIRECTORY_RELEASE   "${CMAKE_BINARY_DIR}/bin/Release"
        LIBRARY_OUTPUT_DIRECTORY           "${CMAKE_BINARY_DIR}/bin"
        LIBRARY_OUTPUT_DIRECTORY_DEBUG     "${CMAKE_BINARY_DIR}/bin/Debug"
        LIBRARY_OUTPUT_DIRECTORY_RELEASE   "${CMAKE_BINARY_DIR}/bin/Release"
    )

    # macOS + Unity: output as .bundle instead of .dylib.
    # Unity on macOS loads native plugins as .bundle files placed under
    # Assets/Plugins/  (or Assets/Plugins/x86_64/ for Intel,
    #                    Assets/Plugins/arm64/   for Apple Silicon).
    # A .bundle is structurally identical to a .dylib; only the suffix differs.
    # This option is ON by default so CI / delivery builds produce the right file.
    # Turn it OFF if you need a plain .dylib for non-Unity consumers on macOS.
    option(BUILD_MACOS_BUNDLE "Output BlueprintRuntime as .bundle on macOS (required by Unity)" ON)

    if(APPLE AND NOT IOS AND BUILD_MACOS_BUNDLE)
        set_target_properties(BlueprintRuntime PROPERTIES
            SUFFIX ".bundle"
        )
        message(STATUS "[BlueprintRuntime] macOS: output suffix set to .bundle (Unity plugin)")
    endif()
endif()

if(EMSCRIPTEN)
    # Unity WebGL compile flags
    target_compile_options(BlueprintRuntime PUBLIC
        -fno-exceptions          # Unity WebGL does not support C++ exceptions
        -fno-rtti                # Unity WebGL strips RTTI
        -O2
    )
    # NOTE: Emscripten link flags (-sWASM=1, -sALLOW_MEMORY_GROWTH=1, etc.)
    # are NOT set here because BlueprintRuntime is always a STATIC library
    # under Emscripten.  INTERFACE link options on a static library only
    # propagate when the consumer links via target_link_libraries(); Unity
    # performs its own linking step, so these flags would never take effect.
    # If you build a standalone Emscripten app (non-Unity), add the flags
    # in your final executable's CMakeLists.txt instead.
    message(STATUS "[BlueprintRuntime] Emscripten flags applied (-fno-exceptions, -fno-rtti, -O2)")

elseif(MSVC)
    target_compile_options(BlueprintRuntime PRIVATE
        /W4
        /utf-8
        /EHsc
    )
    # Export .lib alongside .dll
    if(_RUNTIME_LIB_TYPE STREQUAL "SHARED")
        set_target_properties(BlueprintRuntime PROPERTIES
            WINDOWS_EXPORT_ALL_SYMBOLS OFF  # rely on BLUEPRINT_API macros
        )
    endif()

elseif(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang|AppleClang")
    target_compile_options(BlueprintRuntime PRIVATE
        -Wall -Wextra
    )
    if(_RUNTIME_LIB_TYPE STREQUAL "SHARED")
        # Hide symbols by default; only BLUEPRINT_API ones are exported
        target_compile_options(BlueprintRuntime PRIVATE
            -fvisibility=hidden
        )
    endif()
endif()

# ---------------------------------------------------------------------------
# macOS: set the dylib/bundle install name (so consumers find it after install)
# ---------------------------------------------------------------------------
if(APPLE AND NOT IOS AND _RUNTIME_LIB_TYPE STREQUAL "SHARED")
    set_target_properties(BlueprintRuntime PROPERTIES
        MACOSX_RPATH ON
        INSTALL_NAME_DIR "@rpath"
    )
endif()

# ---------------------------------------------------------------------------
# Folder organisation (IDEs)
# ---------------------------------------------------------------------------
set_property(TARGET BlueprintRuntime PROPERTY FOLDER "Libraries")

# ---------------------------------------------------------------------------
# Install rules
# ---------------------------------------------------------------------------
include(GNUInstallDirs)

install(TARGETS BlueprintRuntime crude_json
    EXPORT BlueprintRuntimeTargets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}   # .dll on Windows
)

# Install Lua library when built from FetchContent
if(BLUEPRINT_LUA AND TARGET ${LUA_LIBRARIES})
    install(TARGETS ${LUA_LIBRARIES}
        EXPORT BlueprintRuntimeTargets
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    )
    # Install Lua headers for consumers
    if(DEFINED LUA54_SRC_DIR)
        install(FILES
            "${LUA54_SRC_DIR}/lua.h"
            "${LUA54_SRC_DIR}/luaconf.h"
            "${LUA54_SRC_DIR}/lualib.h"
            "${LUA54_SRC_DIR}/lauxlib.h"
            "${LUA54_SRC_DIR}/lua.hpp"
            DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/lua54
        )
    endif()
endif()

install(FILES ${RUNTIME_PUBLIC_HEADERS}
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/BlueprintRuntime
)

# Install crude_json headers alongside
install(FILES
    ${BLUEPRINT_ROOT_DIR}/Utils/Json/crude_json.h
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/crude_json
)

# CMake package config (so find_package(BlueprintRuntime) works)
install(EXPORT BlueprintRuntimeTargets
    FILE        BlueprintRuntimeTargets.cmake
    NAMESPACE   BlueprintRuntime::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/BlueprintRuntime
)

include(CMakePackageConfigHelpers)

configure_package_config_file(
    "${BLUEPRINT_ROOT_DIR}/cmake/BlueprintRuntimeConfig.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/BlueprintRuntimeConfig.cmake"
    INSTALL_DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/BlueprintRuntime"
)

write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/BlueprintRuntimeConfigVersion.cmake"
    VERSION       1.0.0
    COMPATIBILITY AnyNewerVersion
)

install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/BlueprintRuntimeConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/BlueprintRuntimeConfigVersion.cmake"
    DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/BlueprintRuntime"
)

# ==============================================================================
# BlueprintBundle – merged static library for Unity / engine delivery
# ==============================================================================
#
# Only built when _RUNTIME_LIB_TYPE is STATIC (iOS, Emscripten, or explicit
# BUILD_SHARED_LIBS=OFF).  Dynamic platforms don't need merging.
#
# Output: libBlueprintBundle.a  (contains BlueprintRuntime + crude_json [+ lua54])
#
# Build the bundle target explicitly:
#   cmake --build . --target BlueprintBundle
#
# or enable it by default with -DBUILD_BUNDLE=ON (adds it to ALL).
#
# Lua merging:
#   BUNDLE_LUA=OFF (default) – liblua54.a is NOT merged into the bundle.
#                              Use this when the host app already provides a
#                              Lua VM (e.g. xLua on iOS).  Call
#                              BP_SetExternalLuaState() at runtime to share it.
#   BUNDLE_LUA=ON            – liblua54.a IS merged.  Use only when Blueprint
#                              is the sole Lua user in the final binary.
# ==============================================================================

option(BUILD_BUNDLE "Build merged static library BlueprintBundle for engine delivery" OFF)
option(BUNDLE_LUA   "Include liblua54 in BlueprintBundle (OFF = use external Lua VM)" OFF)

if(_RUNTIME_LIB_TYPE STREQUAL "STATIC")

    # Collect input archives -------------------------------------------------
    set(_BUNDLE_INPUTS
        $<TARGET_FILE:BlueprintRuntime>
        $<TARGET_FILE:crude_json>
    )

    # Include Lua only when explicitly requested AND it is static
    if(BLUEPRINT_LUA AND BUNDLE_LUA AND (NOT _LUA_LIB_TYPE STREQUAL "SHARED"))
        # LUA_LIBRARIES may be a CMake target (self-built via FetchContent)
        # or a plain file path (system Lua found via find_package/pkg-config).
        # $<TARGET_FILE:...> only works with targets — use TARGET guard.
        if(TARGET ${LUA_LIBRARIES})
            list(APPEND _BUNDLE_INPUTS $<TARGET_FILE:${LUA_LIBRARIES}>)
        else()
            list(APPEND _BUNDLE_INPUTS ${LUA_LIBRARIES})
        endif()
        message(STATUS "[BlueprintBundle] Lua WILL be merged into bundle (BUNDLE_LUA=ON)")
    elseif(BLUEPRINT_LUA AND (NOT _LUA_LIB_TYPE STREQUAL "SHARED"))
        message(STATUS "[BlueprintBundle] Lua will NOT be merged (BUNDLE_LUA=OFF) — use BP_SetExternalLuaState() to provide external VM")
    endif()

    # Output path ------------------------------------------------------------
    # BUNDLE_NAME 可由外部指定文件名（不含 lib 前缀和 .a 后缀），
    # 默认为 BlueprintBundle；BUNDLE_LUA=OFF 时自动降级为 BlueprintBundleNoLua。
    if(NOT DEFINED BUNDLE_NAME OR BUNDLE_NAME STREQUAL "")
        if(BUNDLE_LUA)
            set(BUNDLE_NAME "BlueprintBundle")
        else()
            set(BUNDLE_NAME "BlueprintBundleNoLua")
        endif()
    endif()
    set(_BUNDLE_OUT "${CMAKE_BINARY_DIR}/bin/Release/lib${BUNDLE_NAME}.a")
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        set(_BUNDLE_OUT "${CMAKE_BINARY_DIR}/bin/Debug/lib${BUNDLE_NAME}.a")
    endif()

    # Platform-specific merge command ----------------------------------------
    if(APPLE)
        # macOS / iOS: use xcrun libtool (Apple's static linker tool).
        # 'libtool' in PATH on GitHub Actions macOS runners may resolve to the
        # GNU libtool, which does NOT support -static.  Force Apple's libtool
        # via xcrun to be safe.
        find_program(_APPLE_LIBTOOL libtool HINTS /usr/bin /usr/local/bin)
        if(NOT _APPLE_LIBTOOL)
            set(_APPLE_LIBTOOL libtool)
        endif()
        set(_MERGE_CMD
            xcrun ${_APPLE_LIBTOOL} -static -o "${_BUNDLE_OUT}" ${_BUNDLE_INPUTS}
        )
    elseif(EMSCRIPTEN)
        # Emscripten: emar (llvm-ar wrapper)
        set(_MERGE_CMD
            ${CMAKE_AR} -rcsT "${_BUNDLE_OUT}" ${_BUNDLE_INPUTS}
        )
    else()
        # Linux / Android NDK: generate the MRI script at build time (after
        # generator expressions have been evaluated) then feed it to ar -M.
        # We cannot pass $<TARGET_FILE:...> via -D to a cmake -P script because
        # generator expressions are not re-evaluated inside cmake -P.  Instead
        # we write the MRI file using a second COMMAND that runs *after* the
        # generator expressions are resolved, then invoke ar directly.
        set(_MRI_FILE "${CMAKE_CURRENT_BINARY_DIR}/bundle.mri")

        # Build the MRI content as a generator expression string
        # Each ADDLIB line is appended for every input archive.
        set(_MRI_CONTENT "CREATE ${_BUNDLE_OUT}")
        foreach(_inp IN LISTS _BUNDLE_INPUTS)
            string(APPEND _MRI_CONTENT "\nADDLIB ${_inp}")
        endforeach()
        string(APPEND _MRI_CONTENT "\nSAVE\nEND")

        set(_MERGE_COMMANDS
            COMMAND ${CMAKE_COMMAND} -E echo "${_MRI_CONTENT}" > "${_MRI_FILE}"
            COMMAND ${CMAKE_AR} -M < "${_MRI_FILE}"
        )
    endif()

    # Custom target ----------------------------------------------------------
    if(BUILD_BUNDLE)
        set(_BUNDLE_ALL ALL)
    else()
        set(_BUNDLE_ALL "")
    endif()

    if(APPLE OR EMSCRIPTEN)
        add_custom_target(BlueprintBundle ${_BUNDLE_ALL}
            COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_BINARY_DIR}/bin/Release"
            COMMAND ${_MERGE_CMD}
            DEPENDS BlueprintRuntime crude_json
            COMMENT "Merging static libs → libBlueprintBundle.a"
            VERBATIM
        )
    else()
        # Linux/Android: two-step (write MRI then ar -M).
        # Generator expressions in _MRI_CONTENT are evaluated at build time.
        set(_MRI_FILE "${CMAKE_CURRENT_BINARY_DIR}/bundle.mri")
        add_custom_target(BlueprintBundle ${_BUNDLE_ALL}
            COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_BINARY_DIR}/bin/Release"
            # Step 1: write the MRI file with resolved archive paths
            COMMAND ${CMAKE_COMMAND} -E echo
                "CREATE ${_BUNDLE_OUT}"    > "${_MRI_FILE}"
            COMMAND ${CMAKE_COMMAND} -E echo
                "ADDLIB $<TARGET_FILE:BlueprintRuntime>" >> "${_MRI_FILE}"
            COMMAND ${CMAKE_COMMAND} -E echo
                "ADDLIB $<TARGET_FILE:crude_json>"       >> "${_MRI_FILE}"
            COMMAND ${CMAKE_COMMAND} -E echo
                "SAVE"                                   >> "${_MRI_FILE}"
            COMMAND ${CMAKE_COMMAND} -E echo
                "END"                                    >> "${_MRI_FILE}"
            # Step 2: merge
            COMMAND ${CMAKE_AR} -M < "${_MRI_FILE}"
            COMMAND ${CMAKE_COMMAND} -E echo
                "[BlueprintBundle] merged → ${_BUNDLE_OUT}"
            DEPENDS BlueprintRuntime crude_json
            COMMENT "Merging static libs → libBlueprintBundle.a"
            VERBATIM
        )
    endif()

    # Install the bundle archive alongside the individual libs ---------------
    install(FILES "${_BUNDLE_OUT}"
        DESTINATION ${CMAKE_INSTALL_LIBDIR}
        OPTIONAL    # don't fail if BlueprintBundle target was never built
    )

    message(STATUS "[BlueprintBundle] Merge target configured → ${_BUNDLE_OUT}")

else()
    message(STATUS "[BlueprintBundle] Skipped (SHARED build, no merging needed)")
endif()
