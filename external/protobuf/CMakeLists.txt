# ==============================================================================
# protobuf  (lite subset)
# ==============================================================================
#
# Builds a static library  protobuf_s  from the vendored source tree.
# Only the files needed by BlueprintRuntime are included:
#   google/protobuf/*.cc
#   google/protobuf/io/*.cc
#   google/protobuf/stubs/*.cc
#   google/protobuf/compiler/parser.cc
#   google/protobuf/compiler/importer.cc
#
# To upgrade:  replace the contents of external/protobuf/google/ with the new
# version (same layout) and bump the version string below.
#
# Platform support:
#   Windows  MSVC / MinGW   ✅
#   Linux    GCC / Clang    ✅  (-DHAVE_PTHREAD)
#   macOS    Clang          ✅  (-DHAVE_PTHREAD)
#   Android                 ✅  (-DHAVE_PTHREAD)
#   iOS                     ✅  (-DHAVE_PTHREAD)
#   WebGL    Emscripten     ✅  (single-threaded stub, no pthread needed)
# ==============================================================================

cmake_minimum_required(VERSION 3.14)
project(protobuf VERSION 3.21.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ---------------------------------------------------------------------------
# Source collection (exclude test files)
# ---------------------------------------------------------------------------
file(GLOB _PB_SOURCES
    "${CMAKE_CURRENT_LIST_DIR}/google/protobuf/*.cc"
    "${CMAKE_CURRENT_LIST_DIR}/google/protobuf/io/*.cc"
    "${CMAKE_CURRENT_LIST_DIR}/google/protobuf/stubs/*.cc"
    "${CMAKE_CURRENT_LIST_DIR}/google/protobuf/compiler/parser.cc"
    "${CMAKE_CURRENT_LIST_DIR}/google/protobuf/compiler/importer.cc"
)
file(GLOB _PB_SOURCES_EXCLUDE
    "${CMAKE_CURRENT_LIST_DIR}/google/protobuf/*test*"
    "${CMAKE_CURRENT_LIST_DIR}/google/protobuf/io/*test*"
    "${CMAKE_CURRENT_LIST_DIR}/google/protobuf/stubs/*test*"
)
list(REMOVE_ITEM _PB_SOURCES ${_PB_SOURCES_EXCLUDE})

# ---------------------------------------------------------------------------
# Target
# ---------------------------------------------------------------------------
add_library(protobuf_s STATIC ${_PB_SOURCES})

target_include_directories(protobuf_s PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}>
    $<INSTALL_INTERFACE:include>
)

# ---------------------------------------------------------------------------
# Platform-specific compile definitions
# ---------------------------------------------------------------------------
target_compile_definitions(protobuf_s PUBLIC
    _SILENCE_STDEXT_HASH_DEPRECATION_WARNINGS
)

if(EMSCRIPTEN)
    # WebGL: single-threaded, no pthread, no threading header needed.
    # GOOGLE_PROTOBUF_OS_EMSCRIPTEN is already set by platform_macros.h.
    # We only need to tell common.cc to skip the pthread branch.
    target_compile_definitions(protobuf_s PRIVATE
        PROTOBUF_USE_EXCEPTIONS=0
    )
    # Suppress warnings common in Emscripten Clang
    target_compile_options(protobuf_s PRIVATE
        -Wno-deprecated-declarations
        -Wno-unused-parameter
    )
elseif(WIN32)
    # Windows: uses win32 API internally, nothing extra needed
else()
    # Linux / macOS / Android / iOS: use pthreads
    target_compile_definitions(protobuf_s PRIVATE
        HAVE_PTHREAD
    )
    # Suppress common warnings in GCC/Clang
    target_compile_options(protobuf_s PRIVATE
        -Wno-deprecated-declarations
        -Wno-unused-parameter
        -Wno-sign-compare
    )
endif()

# ---------------------------------------------------------------------------
# MSVC-specific
# ---------------------------------------------------------------------------
if(MSVC)
    target_compile_options(protobuf_s PRIVATE /W0 /utf-8)
endif()

# ---------------------------------------------------------------------------
# IDE folder grouping
# ---------------------------------------------------------------------------
set_target_properties(protobuf_s PROPERTIES FOLDER "external")
