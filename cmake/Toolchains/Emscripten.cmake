# cmake/Toolchains/Emscripten.cmake
# ============================================================================
# Emscripten Toolchain for BlueprintRuntime – Unity WebGL builds
# ============================================================================
#
# USAGE
# -----
# Option A – emcmake (recommended, auto-sets EMSCRIPTEN env):
#
#   emcmake cmake .. \
#       -DBUILD_RUNTIME_ONLY=ON \
#       -DCMAKE_BUILD_TYPE=Release
#
# Option B – explicit toolchain (when emcmake is not available):
#
#   cmake .. \
#       -DCMAKE_TOOLCHAIN_FILE=cmake/Toolchains/Emscripten.cmake \
#       -DEMSDK_ROOT=/path/to/emsdk \
#       -DBUILD_RUNTIME_ONLY=ON \
#       -DCMAKE_BUILD_TYPE=Release
#
# OUTPUT
# ------
# The static library BlueprintRuntime.a can be dropped directly into a
# Unity project's Assets/Plugins/WebGL/ folder.
#
# In Unity, set the library's Platform settings to "WebGL" only.
# Wrap any C function you want to call from C# with extern "C":
#
#   extern "C" {
#       void Blueprint_Run(const char* jsonPath);
#   }
#
# ============================================================================

# ---------------------------------------------------------------------------
# Locate the Emscripten SDK
# ---------------------------------------------------------------------------
if(DEFINED ENV{EMSDK} AND NOT EMSDK_ROOT)
    set(EMSDK_ROOT "$ENV{EMSDK}" CACHE PATH "Emscripten SDK root")
endif()

if(NOT EMSDK_ROOT)
    # Try common install locations
    foreach(_candidate
            "$ENV{HOME}/emsdk"
            "/opt/emsdk"
            "C:/emsdk"
    )
        if(EXISTS "${_candidate}/upstream/emscripten/cmake/Modules/Platform/Emscripten.cmake")
            set(EMSDK_ROOT "${_candidate}" CACHE PATH "Emscripten SDK root" FORCE)
            break()
        endif()
    endforeach()
endif()

if(EMSDK_ROOT)
    set(_EMSCRIPTEN_CMAKE
        "${EMSDK_ROOT}/upstream/emscripten/cmake/Modules/Platform/Emscripten.cmake")
    if(EXISTS "${_EMSCRIPTEN_CMAKE}")
        include("${_EMSCRIPTEN_CMAKE}")
    else()
        message(WARNING "EMSDK_ROOT set to '${EMSDK_ROOT}' but Emscripten.cmake not found there. "
                        "Make sure the SDK is activated: source ${EMSDK_ROOT}/emsdk_env.sh")
    endif()
else()
    message(WARNING
        "Emscripten SDK not found. Set EMSDK_ROOT or run 'source emsdk_env.sh' first.\n"
        "Example:\n"
        "  git clone https://github.com/emscripten-core/emsdk.git\n"
        "  cd emsdk && ./emsdk install latest && ./emsdk activate latest\n"
        "  source ./emsdk_env.sh\n"
        "  emcmake cmake /path/to/MyBlueprintEditor -DBUILD_RUNTIME_ONLY=ON"
    )
endif()

# ---------------------------------------------------------------------------
# Toolchain executables (used when NOT invoked via emcmake)
# ---------------------------------------------------------------------------
if(EMSDK_ROOT)
    set(_EMCC "${EMSDK_ROOT}/upstream/emscripten/emcc")
    set(_EMCXX "${EMSDK_ROOT}/upstream/emscripten/em++")
    if(EXISTS "${_EMCC}")
        set(CMAKE_C_COMPILER   "${_EMCC}"  CACHE FILEPATH "Emscripten C compiler")
        set(CMAKE_CXX_COMPILER "${_EMCXX}" CACHE FILEPATH "Emscripten C++ compiler")
    endif()
endif()

# ---------------------------------------------------------------------------
# System / target
# ---------------------------------------------------------------------------
set(CMAKE_SYSTEM_NAME      Emscripten)
set(CMAKE_SYSTEM_PROCESSOR wasm32)

# Emscripten builds are always 32-bit WASM
set(CMAKE_SIZEOF_VOID_P 4)

# Don't try to find host tools in the target sysroot
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)

# ---------------------------------------------------------------------------
# Defaults for BlueprintRuntime / Unity WebGL
# ---------------------------------------------------------------------------

# Always static for Emscripten (Unity WebGL cannot load shared .so)
set(BUILD_SHARED_LIBS  OFF CACHE BOOL "Always OFF for Emscripten" FORCE)
set(BUILD_RUNTIME_ONLY ON  CACHE BOOL "Skip Editor targets for Emscripten" FORCE)

# Disable DefaultFileSystem warning by default for WebGL
# (Unity provides its own VFS; callers should supply a custom IFileSystem)
# Uncomment to opt-in:
# set(BLUEPRINT_NO_FILESYSTEM ON CACHE BOOL "" FORCE)

# ---------------------------------------------------------------------------
# Unity WebGL C++ flags
# ---------------------------------------------------------------------------
# -fno-exceptions : Unity WebGL strips exception support
# -fno-rtti       : Unity WebGL strips RTTI
# -O2             : optimise for size/speed in the browser
set(CMAKE_CXX_FLAGS_INIT "-fno-exceptions -fno-rtti -O2")
set(CMAKE_C_FLAGS_INIT   "-O2")
