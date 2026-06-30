# CMakeLists.txt - GDExtension that wraps BlueprintRuntime for Godot 4.5
#
# Builds: blueprint_gdext  (a shared library Godot loads via blueprint.gdextension)
# Links against:
#   - godot-cpp (4.5 branch, built as a subdirectory)
#   - BlueprintRuntime (your prebuilt C++ engine DLL + import lib)
#
# Configure (from this directory):
#   cmake -B build -G "Visual Studio 17 2022" -A x64 -DCMAKE_BUILD_TYPE=Release
#   cmake --build build --config Release
#
# Output goes to: game/bin/blueprint_gdext.windows.<cfg>.x86_64.dll

cmake_minimum_required(VERSION 3.20)
project(blueprint_gdext LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ---------------------------------------------------------------------------
# Paths
# ---------------------------------------------------------------------------
# Root of the MyBlueprintEditor repo (one level up from GodotIntegration/).
get_filename_component(BP_REPO_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/.." ABSOLUTE)

# BlueprintRuntime headers (the C API lives here).
set(BP_RUNTIME_INCLUDE "${BP_REPO_ROOT}/Runtime")

# Prebuilt BlueprintRuntime artifacts. Adjust if you rebuild elsewhere.
#   - DLL: copied next to the game executable / into game/bin
#   - import lib (.lib): needed at link time on MSVC
# NOTE: build-windows is an MSVC build (vcxproj). The runtime DLL lands in
# bin/Release while its import .lib lands in Runtime/Release. Both are MSVC ABI,
# which matches godot-cpp built with MSVC.
set(BP_RUNTIME_DLL  "${BP_REPO_ROOT}/build-windows/bin/Release/BlueprintRuntime.dll")
set(BP_RUNTIME_LIB  "${BP_REPO_ROOT}/build-windows/Runtime/Release/BlueprintRuntime.lib")

# Fallbacks: try the dist/ folder if build-windows isn't present.
if(NOT EXISTS "${BP_RUNTIME_DLL}")
    set(BP_RUNTIME_DLL "${BP_REPO_ROOT}/dist/UnityPlugins/Windows/x86_64/BlueprintRuntime.dll")
endif()

# ---------------------------------------------------------------------------
# godot-cpp (must be cloned at GodotIntegration/godot-cpp, branch 4.5)
# ---------------------------------------------------------------------------
if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/godot-cpp/CMakeLists.txt")
    message(FATAL_ERROR
        "godot-cpp not found. Clone it first:\n"
        "  git clone --branch 4.5 --recurse-submodules https://github.com/godotengine/godot-cpp.git godot-cpp")
endif()

add_subdirectory(godot-cpp)

# ---------------------------------------------------------------------------
# Our extension library
# ---------------------------------------------------------------------------
file(GLOB BP_GDEXT_SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp")

add_library(blueprint_gdext SHARED ${BP_GDEXT_SOURCES})

target_include_directories(blueprint_gdext PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/src"
    "${BP_RUNTIME_INCLUDE}")

target_link_libraries(blueprint_gdext PRIVATE godot-cpp)

# Link the BlueprintRuntime import library (MSVC needs the .lib).
if(EXISTS "${BP_RUNTIME_LIB}")
    target_link_libraries(blueprint_gdext PRIVATE "${BP_RUNTIME_LIB}")
else()
    message(WARNING
        "BlueprintRuntime import lib not found at ${BP_RUNTIME_LIB}.\n"
        "On MSVC you need BlueprintRuntime.lib. Rebuild the runtime with MSVC, "
        "or generate a .lib from the DLL exports.")
endif()

# ---------------------------------------------------------------------------
# Output naming: Godot expects  <name>.<platform>.<config>.<arch>.dll
# We keep it simple and let blueprint.gdextension point at the produced file.
# ---------------------------------------------------------------------------
set(BP_OUT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/game/bin")
set_target_properties(blueprint_gdext PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${BP_OUT_DIR}"
    LIBRARY_OUTPUT_DIRECTORY "${BP_OUT_DIR}"
    PREFIX ""
    OUTPUT_NAME "blueprint_gdext.windows.template_debug.x86_64")

# Copy the BlueprintRuntime DLL next to the extension so Godot can load it.
add_custom_command(TARGET blueprint_gdext POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory "${BP_OUT_DIR}"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "${BP_RUNTIME_DLL}" "${BP_OUT_DIR}/"
    COMMENT "Copying BlueprintRuntime.dll next to the GDExtension")
