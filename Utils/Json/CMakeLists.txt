# ==============================================================================
# crude_json – lightweight JSON parser library
# ==============================================================================
#
# Provides the crude_json::value type used by BlueprintRuntime and
# ImGuiNodeEditor.  Wrapped as a proper CMake library target so that
# consumers link via target_link_libraries() instead of pulling the
# .cpp directly into their SOURCES list.
#
# To replace with nlohmann/json or simdjson in the future, swap out this
# CMakeLists.txt (or provide an adapter header that maps the crude_json
# API onto the new library).
# ==============================================================================

add_library(crude_json STATIC
    crude_json.cpp
    crude_json.h
)

add_library(crude_json::crude_json ALIAS crude_json)

target_include_directories(crude_json
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<INSTALL_INTERFACE:include/crude_json>
)

set_target_properties(crude_json PROPERTIES
    CXX_STANDARD 14
    CXX_STANDARD_REQUIRED ON
    POSITION_INDEPENDENT_CODE ON
    FOLDER "Libraries"
)

# ---------------------------------------------------------------------------
# Platform-specific compile options
# ---------------------------------------------------------------------------
if(MSVC)
    target_compile_options(crude_json PRIVATE /W4 /utf-8)
elseif(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang|AppleClang")
    target_compile_options(crude_json PRIVATE -Wall -Wextra -Wpedantic)
endif()
