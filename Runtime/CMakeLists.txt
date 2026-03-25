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
    project(BlueprintRuntime VERSION 1.0.0 LANGUAGES CXX)
    set(BLUEPRINT_STANDALONE TRUE)
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
    ${CMAKE_CURRENT_SOURCE_DIR}/BlueprintExporter.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/BlueprintRunner.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/FrameTimerManager.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/BuiltinNodeDefs.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/BuiltinHandlers.cpp
    ${BLUEPRINT_ROOT_DIR}/Utils/Json/crude_json.cpp
)

set(RUNTIME_PUBLIC_HEADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/BlueprintExport.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Types.h
    ${CMAKE_CURRENT_SOURCE_DIR}/NodeDefinition.h
    ${CMAKE_CURRENT_SOURCE_DIR}/BlueprintData.h
    ${CMAKE_CURRENT_SOURCE_DIR}/BlueprintExporter.h
    ${CMAKE_CURRENT_SOURCE_DIR}/BlueprintRunner.h
    ${CMAKE_CURRENT_SOURCE_DIR}/FrameTimerManager.h
    ${CMAKE_CURRENT_SOURCE_DIR}/BuiltinNodeDefs.h
    ${CMAKE_CURRENT_SOURCE_DIR}/BuiltinHandlers.h
    ${CMAKE_CURRENT_SOURCE_DIR}/FileSystem.h
)

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
# Include directories
# ---------------------------------------------------------------------------
target_include_directories(BlueprintRuntime
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<BUILD_INTERFACE:${BLUEPRINT_ROOT_DIR}/Utils>
        $<BUILD_INTERFACE:${BLUEPRINT_ROOT_DIR}/Utils/Json>
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
    CXX_STANDARD 14
    CXX_STANDARD_REQUIRED ON
    POSITION_INDEPENDENT_CODE ON  # required for shared libs on Linux/macOS
)

if(EMSCRIPTEN)
    # Unity WebGL compile flags
    target_compile_options(BlueprintRuntime PUBLIC
        -fno-exceptions          # Unity WebGL does not support C++ exceptions
        -fno-rtti                # Unity WebGL strips RTTI
        -O2
    )
    # Emscripten link flags (only meaningful when building the final .js/.wasm,
    # but harmless to set here for documentation purposes)
    target_link_options(BlueprintRuntime INTERFACE
        -sWASM=1
        -sALLOW_MEMORY_GROWTH=1
        -sUSE_WEBGL2=0           # Unity WebGL uses its own GL
        -sERROR_ON_UNDEFINED_SYMBOLS=0
    )
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
        -Wall -Wextra -Wpedantic
    )
    if(_RUNTIME_LIB_TYPE STREQUAL "SHARED")
        # Hide symbols by default; only BLUEPRINT_API ones are exported
        target_compile_options(BlueprintRuntime PRIVATE
            -fvisibility=hidden
            -fvisibility-inlines-hidden
        )
    endif()
endif()

# ---------------------------------------------------------------------------
# macOS: set the dylib install name (so consumers find it after install)
# ---------------------------------------------------------------------------
if(APPLE AND _RUNTIME_LIB_TYPE STREQUAL "SHARED")
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

install(TARGETS BlueprintRuntime
    EXPORT BlueprintRuntimeTargets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}   # .dll on Windows
)

install(FILES ${RUNTIME_PUBLIC_HEADERS}
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/BlueprintRuntime
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
