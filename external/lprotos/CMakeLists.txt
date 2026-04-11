# ==============================================================================
# lprotos  — Lua bindings for protobuf dynamic messages
# ==============================================================================
#
# Provides:
#   lprotos_s   — static library (link into BlueprintRuntime when BLUEPRINT_LUA=ON)
#   lprotos     — shared library (.dll/.so, loadable as Lua C module: require "lprotos")
#
# Depends on:
#   protobuf_s  — vendored protobuf (external/protobuf)
#   Lua 5.4     — found via find_package(Lua) or passed LUA_INCLUDE_DIR / LUA_LIBRARIES
#
# Platform support: same as protobuf_s (Windows / Linux / macOS / Android / iOS / WebGL)
#
# Usage:
#   add_subdirectory(external/lprotos)
#   target_link_libraries(MyTarget PRIVATE lprotos_s)
# ==============================================================================

cmake_minimum_required(VERSION 3.14)
project(lprotos VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ---------------------------------------------------------------------------
# Source files (exclude main.cpp if present)
# ---------------------------------------------------------------------------
file(GLOB _LP_SOURCES "${CMAKE_CURRENT_LIST_DIR}/*.cpp")
list(FILTER _LP_SOURCES EXCLUDE REGEX ".*main\\.cpp$")

# ---------------------------------------------------------------------------
# Locate Lua — prefer the FetchContent lua54 target from BlueprintRuntime
# ---------------------------------------------------------------------------
if(TARGET lua54)
    # Use the FetchContent Lua 5.4 target directly — most reliable
    set(_LPROTOS_LUA_TARGET lua54)
    get_target_property(_LPROTOS_LUA_INC lua54 SOURCE_DIR)
    # SOURCE_DIR of the imported target is the FetchContent src dir
    # The actual headers are in <src>/src
    set(_LPROTOS_LUA_INC "${CMAKE_BINARY_DIR}/_deps/lua54-src/src")
    if(NOT EXISTS "${_LPROTOS_LUA_INC}/lua.h")
        # Fallback: try interface include directories
        get_target_property(_LPROTOS_LUA_INC lua54 INTERFACE_INCLUDE_DIRECTORIES)
    endif()
elseif(LUA_INCLUDE_DIR AND EXISTS "${LUA_INCLUDE_DIR}/lua.h")
    set(_LPROTOS_LUA_INC "${LUA_INCLUDE_DIR}")
    set(_LPROTOS_LUA_TARGET "${LUA_LIBRARIES}")
else()
    find_package(Lua QUIET)
    if(NOT LUA_FOUND)
        message(WARNING "[lprotos] Lua not found — lprotos targets will not be built.")
        return()
    endif()
    set(_LPROTOS_LUA_INC "${LUA_INCLUDE_DIR}")
    set(_LPROTOS_LUA_TARGET "${LUA_LIBRARIES}")
endif()

message(STATUS "[lprotos] Using Lua include: ${_LPROTOS_LUA_INC}")

# ---------------------------------------------------------------------------
# Ensure protobuf_s is available
# ---------------------------------------------------------------------------
if(NOT TARGET protobuf_s)
    if(EXISTS "${CMAKE_CURRENT_LIST_DIR}/../protobuf/CMakeLists.txt")
        add_subdirectory("${CMAKE_CURRENT_LIST_DIR}/../protobuf"
                         "${CMAKE_CURRENT_BINARY_DIR}/protobuf")
    else()
        message(WARNING "[lprotos] protobuf_s target not found — lprotos will not be built.")
        return()
    endif()
endif()

# ---------------------------------------------------------------------------
# Static library  (lprotos_s)
# ---------------------------------------------------------------------------
add_library(lprotos_s STATIC ${_LP_SOURCES})

target_include_directories(lprotos_s PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}>
    $<BUILD_INTERFACE:${_LPROTOS_LUA_INC}>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/../protobuf>
    $<INSTALL_INTERFACE:include>
)

target_link_libraries(lprotos_s
    PUBLIC  protobuf_s
    PRIVATE ${_LPROTOS_LUA_TARGET}
)

target_compile_definitions(lprotos_s PUBLIC
    WITH_LDYNAMICPROTOBUF
    WITH_LDYNAMICPROTOBUF_STATIC
)

if(MSVC)
    target_compile_options(lprotos_s PRIVATE /W0 /utf-8)
else()
    target_compile_options(lprotos_s PRIVATE
        -Wno-deprecated-declarations
        -Wno-unused-parameter
    )
endif()

set_target_properties(lprotos_s PROPERTIES FOLDER "external")

# ---------------------------------------------------------------------------
# Shared library  (lprotos) — loadable as  require "lprotos"  from Lua
# Skipped on Emscripten (no shared libs) and iOS (no dylib plugins)
# ---------------------------------------------------------------------------
if(NOT EMSCRIPTEN AND NOT (CMAKE_SYSTEM_NAME STREQUAL "iOS"))

    add_library(lprotos SHARED ${_LP_SOURCES})

    target_include_directories(lprotos PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}
        ${_LPROTOS_LUA_INC}
        ${CMAKE_CURRENT_LIST_DIR}/../protobuf
    )

    target_link_libraries(lprotos
        PRIVATE protobuf_s
        PRIVATE ${_LPROTOS_LUA_TARGET}
    )

    target_compile_definitions(lprotos PRIVATE
        WITH_LDYNAMICPROTOBUF
        WITH_LDYNAMICPROTOBUF_SHARED
    )

    if(MSVC)
        target_compile_options(lprotos PRIVATE /W0 /utf-8)
    else()
        target_compile_options(lprotos PRIVATE
            -Wno-deprecated-declarations
            -Wno-unused-parameter
        )
    endif()

    # Output to  bin/{Debug,Release}/lua.plugins/
    set_target_properties(lprotos PROPERTIES
        FOLDER "external"
        LIBRARY_OUTPUT_DIRECTORY           "${CMAKE_BINARY_DIR}/bin/lua.plugins"
        LIBRARY_OUTPUT_DIRECTORY_DEBUG     "${CMAKE_BINARY_DIR}/bin/Debug/lua.plugins"
        LIBRARY_OUTPUT_DIRECTORY_RELEASE   "${CMAKE_BINARY_DIR}/bin/Release/lua.plugins"
        RUNTIME_OUTPUT_DIRECTORY           "${CMAKE_BINARY_DIR}/bin/lua.plugins"
        RUNTIME_OUTPUT_DIRECTORY_DEBUG     "${CMAKE_BINARY_DIR}/bin/Debug/lua.plugins"
        RUNTIME_OUTPUT_DIRECTORY_RELEASE   "${CMAKE_BINARY_DIR}/bin/Release/lua.plugins"
        PREFIX ""   # no "lib" prefix on Linux — Lua expects lprotos.so not liblprotos.so
    )

    message(STATUS "[lprotos] Shared library target added (lprotos)")
endif()

message(STATUS "[lprotos] Static library target added (lprotos_s)")
