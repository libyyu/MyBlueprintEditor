// Runtime/BlueprintExport.h - DLL/SO export macros for BlueprintRuntime
//
// Usage:
//   - When building the shared library:  define BLUEPRINT_BUILD_DLL
//   - When consuming the shared library: define BLUEPRINT_USE_DLL  (auto-set by CMake via target_compile_definitions)
//   - For static builds: nothing needed, BLUEPRINT_API expands to nothing
//
// All public API symbols in Runtime headers should be decorated with BLUEPRINT_API.

#pragma once

// -----------------------------------------------------------------------
// Platform / compiler detection
// -----------------------------------------------------------------------

#if defined(_WIN32) || defined(_WIN64)
#   define BLUEPRINT_PLATFORM_WINDOWS 1
#elif defined(__EMSCRIPTEN__)
#   define BLUEPRINT_PLATFORM_EMSCRIPTEN 1
#elif defined(__APPLE__)
#   define BLUEPRINT_PLATFORM_APPLE 1
#elif defined(__linux__)
#   define BLUEPRINT_PLATFORM_LINUX 1
#endif

// -----------------------------------------------------------------------
// Export / import macros
// -----------------------------------------------------------------------

#if defined(BLUEPRINT_STATIC)
    // Static library – no import/export decoration needed
#   define BLUEPRINT_API
#   define BLUEPRINT_LOCAL

#elif defined(BLUEPRINT_PLATFORM_WINDOWS)
#   if defined(BLUEPRINT_BUILD_DLL)
#       define BLUEPRINT_API    __declspec(dllexport)
#   else
#       define BLUEPRINT_API    __declspec(dllimport)
#   endif
#   define BLUEPRINT_LOCAL

#elif defined(__GNUC__) || defined(__clang__)
    // GCC / Clang on Linux, macOS, Emscripten
#   if defined(BLUEPRINT_BUILD_DLL)
#       define BLUEPRINT_API    __attribute__((visibility("default")))
#       define BLUEPRINT_LOCAL  __attribute__((visibility("hidden")))
#   else
#       define BLUEPRINT_API
#       define BLUEPRINT_LOCAL
#   endif

#else
#   define BLUEPRINT_API
#   define BLUEPRINT_LOCAL
#endif

// -----------------------------------------------------------------------
// Emscripten / Unity WebGL notes
// -----------------------------------------------------------------------
#if defined(BLUEPRINT_PLATFORM_EMSCRIPTEN)
    // DefaultFileSystem on Emscripten uses emscripten_wget_data() (synchronous XHR)
    // to load files from Unity's StreamingAssets path.  No manual initialisation
    // is required – just place your .json blueprint files in StreamingAssets/ and
    // call BlueprintRunner::LoadFromFile("myblueprint.json") as normal.
    //
    // The base URL defaults to "StreamingAssets".  Override at compile time:
    //   -DBLUEPRINT_STREAMING_ASSETS_BASE=\"MyGame/StreamingAssets\"
    //
    // WriteFile is intentionally unsupported (returns false).  If you need
    // persistence, call SetDefaultFileSystem() with an IDBFS-backed implementation.
    //
    // Define BLUEPRINT_NO_FILESYSTEM to strip DefaultFileSystem entirely and
    // supply your own IFileSystem via SetDefaultFileSystem().
#endif
