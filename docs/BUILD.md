# Build Guide

This document covers how to build **BlueprintRuntime** (and optionally the full Editor) across all supported platforms and configurations.

---

## Table of Contents

1. [Prerequisites](#prerequisites)
2. [Windows – Full Build (Static, Editor + Runtime)](#windows--full-build)
3. [Windows – Shared DLL (Runtime only)](#windows--shared-dll)
4. [Linux / macOS – Static Library](#linux--macos--static-library)
5. [Linux / macOS – Shared Library](#linux--macos--shared-library)
6. [Emscripten / Unity WebGL](#emscripten--unity-webgl)
7. [CMake Options Reference](#cmake-options-reference)
8. [Using BlueprintRuntime in Your Project](#using-blueprintruntime-in-your-project)
9. [Custom File System (Emscripten / Unity)](#custom-file-system)

---

## Prerequisites

| Tool | Minimum version |
|------|----------------|
| CMake | 3.14 |
| C++ compiler | C++14 support (MSVC 2017+, GCC 7+, Clang 5+) |
| Emscripten SDK | 3.1+ (WebGL/Unity only) |

---

## Windows – Full Build

Builds BlueprintRuntime (static `.lib`) + ImGuiNodeEditor + BlueprintEditor executable.

```bat
cmake -B build -A x64
cmake --build build --config Release
# Output: build\bin\BlueprintEditor.exe
#         build\Runtime\Release\BlueprintRuntime.lib
```

---

## Windows – Shared DLL

```bat
cmake -B build-shared -A x64 -DBUILD_SHARED_LIBS=ON -DBUILD_RUNTIME_ONLY=ON
cmake --build build-shared --config Release
# Output: build-shared\Runtime\Release\BlueprintRuntime.dll
#         build-shared\Runtime\Release\BlueprintRuntime.lib  (import lib)
```

Consumers must define nothing extra – the import lib handles `__declspec(dllimport)` automatically via the `BLUEPRINT_API` macro.

---

## Linux / macOS – Static Library

```sh
cmake -B build -DCMAKE_BUILD_TYPE=Release -DBUILD_RUNTIME_ONLY=ON
cmake --build build
# Output: build/Runtime/libBlueprintRuntime.a
```

---

## Linux / macOS – Shared Library

```sh
cmake -B build-shared \
    -DCMAKE_BUILD_TYPE=Release \
    -DBUILD_SHARED_LIBS=ON \
    -DBUILD_RUNTIME_ONLY=ON
cmake --build build-shared
# Output: build-shared/Runtime/libBlueprintRuntime.so   (Linux)
#         build-shared/Runtime/libBlueprintRuntime.dylib (macOS)
```

---

## Emscripten / Unity WebGL

### Step 1 – Install Emscripten SDK

```sh
git clone https://github.com/emscripten-core/emsdk.git
cd emsdk
./emsdk install latest
./emsdk activate latest
source ./emsdk_env.sh   # Windows: emsdk_env.bat
```

### Step 2 – Build (recommended: use `emcmake`)

```sh
emcmake cmake -B build-wasm \
    -DBUILD_RUNTIME_ONLY=ON \
    -DCMAKE_BUILD_TYPE=Release
cmake --build build-wasm
# Output: build-wasm/Runtime/libBlueprintRuntime.a
```

Or use the explicit toolchain file:

```sh
cmake -B build-wasm \
    -DCMAKE_TOOLCHAIN_FILE=cmake/Toolchains/Emscripten.cmake \
    -DEMSDK_ROOT=/path/to/emsdk \
    -DBUILD_RUNTIME_ONLY=ON \
    -DCMAKE_BUILD_TYPE=Release
cmake --build build-wasm
```

### Step 3 – Integrate with Unity WebGL

1. Copy `build-wasm/Runtime/libBlueprintRuntime.a` to your Unity project's `Assets/Plugins/WebGL/` folder.
2. In Unity Inspector, set the plugin's platform to **WebGL only**.
3. Place your `.json` blueprint files inside `Assets/StreamingAssets/` in Unity.
4. Expose C functions with `extern "C"` for P/Invoke from C#:

```cpp
// In your integration .cpp (compiled as part of the Unity WebGL build)
#include "BlueprintRunner.h"
#include <emscripten.h>

extern "C" {

EMSCRIPTEN_KEEPALIVE
void Blueprint_Run(const char* jsonPath)
{
    // DefaultFileSystem automatically fetches from StreamingAssets/<jsonPath>
    // via synchronous XHR – no manual initialisation needed.
    NodeEditor::Runtime::BlueprintRunner runner;
    runner.LoadFromFile(jsonPath);   // e.g. "myblueprint.json"
    runner.Execute();
}

} // extern "C"
```

5. From C#:

```csharp
[DllImport("__Internal")]
private static extern void Blueprint_Run(string jsonPath);

void Start() {
    Blueprint_Run("myblueprint.json");
}
```

#### Overriding the StreamingAssets base path

By default files are fetched from `StreamingAssets/<path>`.  To change the base URL,
pass the define at CMake configure time:

```sh
emcmake cmake -B build-wasm \
    -DBUILD_RUNTIME_ONLY=ON \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_CXX_FLAGS="-DBLUEPRINT_STREAMING_ASSETS_BASE=\\\"MyGame/StreamingAssets\\\""
```

#### WriteFile on WebGL

`DefaultFileSystem::WriteFile` returns `false` on WebGL (no persistent FS by default).
If you need to save state, inject a custom `IFileSystem` backed by IDBFS:

```cpp
// Call once before any Blueprint_Run that writes files
NodeEditor::Runtime::SetDefaultFileSystem(
    std::make_shared<MyIdbfsFileSystem>());
```

---

## CMake Options Reference

| Option | Default | Description |
|--------|---------|-------------|
| `BUILD_SHARED_LIBS` | `OFF` | Build BlueprintRuntime as a shared library |
| `BUILD_RUNTIME_ONLY` | `OFF` | Skip Editor / Application targets |
| `BUILD_EXAMPLES` | `ON` | Build example applications |
| `BLUEPRINT_NO_FILESYSTEM` | `OFF` | Exclude `DefaultFileSystem`; caller must provide `IFileSystem` via `SetDefaultFileSystem()` |

---

## Using BlueprintRuntime in Your Project

### CMake `find_package` (after `cmake --install`)

```cmake
find_package(BlueprintRuntime CONFIG REQUIRED)
target_link_libraries(MyTarget PRIVATE BlueprintRuntime::BlueprintRuntime)
```

### CMake `add_subdirectory`

```cmake
set(BUILD_RUNTIME_ONLY ON)
add_subdirectory(path/to/MyBlueprintEditor)
target_link_libraries(MyTarget PRIVATE BlueprintRuntime)
```

---

## Custom File System

`DefaultFileSystem` already handles all three platforms out of the box:

| Platform | Mechanism | ReadFile | WriteFile |
|----------|-----------|----------|-----------|
| Windows / Linux / macOS | `std::fstream` | ✅ | ✅ |
| Emscripten / Unity WebGL | `emscripten_wget_data` (sync XHR from StreamingAssets) | ✅ | ❌ (returns false) |

If you need different behaviour (e.g. IDBFS writes on WebGL, engine asset manager,
encrypted pak), inherit `IFileSystem` and call `SetDefaultFileSystem()`:

```cpp
#include "FileSystem.h"

class MyFS : public NodeEditor::Runtime::IFileSystem
{
public:
    bool ReadFile(const std::string& path, std::string& out, std::string& err) override
    { /* ... */ return true; }

    bool WriteFile(const std::string& path, const std::string& content, std::string& err) override
    { /* ... */ return true; }

    bool FileExists(const std::string& path) override
    { /* ... */ return false; }
};

// Call once at startup:
NodeEditor::Runtime::SetDefaultFileSystem(std::make_shared<MyFS>());
```

To strip `DefaultFileSystem` entirely from the binary, pass `-DBLUEPRINT_NO_FILESYSTEM=ON`.
