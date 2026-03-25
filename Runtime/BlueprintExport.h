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
// Emscripten / Unity WebGL guards
// -----------------------------------------------------------------------

#if defined(BLUEPRINT_PLATFORM_EMSCRIPTEN)
    // Unity WebGL uses Emscripten; file I/O requires MEMFS or IDBFS.
    // The DefaultFileSystem uses std::fstream which maps to Emscripten's
    // virtual FS – this works but requires the caller to mount the correct
    // FS backend (e.g. EM_ASM + FS.mount(MEMFS, ...) or Unity's own VFS).
    // Define BLUEPRINT_NO_FILESYSTEM to compile out DefaultFileSystem and
    // provide your own IFileSystem implementation at runtime via
    // NodeEditor::Runtime::SetDefaultFileSystem().
#   if !defined(BLUEPRINT_NO_FILESYSTEM)
#       pragma message("BlueprintRuntime on Emscripten: DefaultFileSystem uses Emscripten VFS. " \
            "Define BLUEPRINT_NO_FILESYSTEM and call SetDefaultFileSystem() to provide a custom " \
            "Unity-compatible loader (e.g. UnityEngine.Resources).")
#   endif
#endif
