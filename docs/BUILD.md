# Build Guide

This document covers how to build **BlueprintRuntime** (and optionally the full Editor) across all supported platforms and configurations.

---

## Table of Contents

1. [Prerequisites](#prerequisites)
2. [Quick Start – Build Scripts](#quick-start--build-scripts)
3. [Windows – Full Build (Static, Editor + Runtime)](#windows--full-build)
4. [Windows – Shared DLL](#windows--shared-dll)
5. [Linux / macOS – Static Library](#linux--macos--static-library)
6. [Linux / macOS – Shared Library](#linux--macos--shared-library)
7. [Emscripten / Unity WebGL](#emscripten--unity-webgl)
8. [Android](#android)
9. [iOS](#ios)
10. [CMake Options Reference](#cmake-options-reference)
11. [Using BlueprintRuntime in Your Project](#using-blueprintruntime-in-your-project)
12. [Custom File System (Emscripten / Unity)](#custom-file-system)

---

## Prerequisites

| Tool | Minimum version | Required for |
|------|----------------|--------------|
| CMake | 3.14 | All platforms |
| C++ compiler | C++17 (MSVC 2019+, GCC 9+, Clang 10+) | All platforms |
| GLFW3 + OpenGL | any | Linux / macOS full editor |
| Emscripten SDK | 3.1+ | WebGL / Unity WebGL |
| Android NDK | r21+ | Android |
| Xcode | 14+ | iOS (macOS host only) |
| MinGW-w64 | any | Windows DLL cross-compile from Linux/macOS |

---

## Quick Start – Build Scripts

The repo ships two ready-to-use build scripts that wrap all CMake calls.  
Use `build.sh` on Linux/macOS, `build.bat` on Windows.

### Platform overview

| Platform token | Script | What gets built | Output |
|----------------|--------|-----------------|--------|
| `linux` *(default on Linux)* | `build.sh` | Full editor + static runtime | `build-linux/bin/` |
| `macos` *(default on macOS)* | `build.sh` | Full editor + static runtime | `build-macos/bin/` |
| `windows` *(default)* | `build.bat` | Full editor + static runtime | `build-windows\bin\` |
| `windows-dll` | `build.bat` / `build.sh` | Full editor + **shared DLL** | `build-windows-dll\bin\` |
| `dll` | `build.bat` / `build.sh` | Runtime **DLL only** (no editor) | `build-dll\bin\` |
| `runtime` | both | Runtime-only, static (no editor) | `build-runtime/bin/` |
| `wasm` | both | Runtime-only static `.a` (Emscripten) | `build-wasm/Runtime/` |
| `android` | both | Runtime-only `.so` / `.a` (ARM64) | `build-android/Runtime/` |
| `ios` | `build.sh` | Runtime-only static `.a` (arm64) | `build-ios/Runtime/` |

### Common options (both scripts)

| Option | Description |
|--------|-------------|
| `debug` / `release` | Build type (default: `release`) |
| `clean` | Wipe the build directory first |
| `noexamples` | Skip example targets |
| `shared` | Force shared library (overrides platform default) |
| `--ndk <path>` | Android NDK root |
| `--api <N>` | Android min API level (default: 21) |
| `--emsdk <path>` | Emscripten SDK root (or set `$EMSDK`) |

### Examples

```sh
# Linux: Release full editor (auto-detected platform)
./build.sh

# macOS: Debug build
./build.sh macos debug

# Windows: Release full editor (static)
build.bat

# Windows: Release full editor + shared DLL
build.bat windows-dll

# Windows: Runtime DLL only
build.bat dll

# Windows: Runtime DLL only (Debug)
build.bat dll debug

# WASM (Emscripten in $PATH)
./build.sh wasm
build.bat wasm

# Android
./build.sh android --ndk ~/Library/Android/sdk/ndk/25.2.9519653
build.bat android --ndk C:\android-ndk

# iOS (macOS only)
./build.sh ios

# Runtime-only shared lib, current host
./build.sh runtime shared
build.bat runtime shared
```

> **Tip:** Each platform gets its own build directory (`build-<platform>/`), so you can keep multiple platform builds side-by-side without conflicts.

---

## Windows – Full Build

Builds BlueprintRuntime (static `.lib`) + ImGuiNodeEditor + BlueprintEditor executable.

**Via build script (recommended):**
```bat
build.bat
:: Output: build-windows\bin\BlueprintEditor.exe
::         build-windows\Runtime\Release\BlueprintRuntime.lib
```

**Raw CMake:**
```bat
cmake -B build -A x64
cmake --build build --config Release
```

---

## Windows – Shared DLL

**Via build script (recommended):**
```bat
:: Full editor + DLL
build.bat windows-dll
:: Output: build-windows-dll\bin\BlueprintEditor.exe
::         build-windows-dll\bin\Release\BlueprintRuntime.dll
::         build-windows-dll\Runtime\Release\BlueprintRuntime.lib  (import lib)

:: Runtime DLL only (no editor)
build.bat dll
:: Output: build-dll\bin\Release\BlueprintRuntime.dll
::         build-dll\Runtime\Release\BlueprintRuntime.lib
```

**Raw CMake:**
```bat
cmake -B build-shared -A x64 -DBUILD_SHARED_LIBS=ON -DBUILD_RUNTIME_ONLY=ON
cmake --build build-shared --config Release
```

Consumers must define nothing extra – the import lib handles `__declspec(dllimport)` automatically via the `BLUEPRINT_API` macro.

---

## Linux / macOS – Static Library

**Via build script (recommended):**
```sh
./build.sh           # Linux (auto-detected)
./build.sh macos     # macOS
# Output: build-linux/bin/  or  build-macos/bin/
```

**Raw CMake:**
```sh
cmake -B build -DCMAKE_BUILD_TYPE=Release -DBUILD_RUNTIME_ONLY=ON
cmake --build build
# Output: build/Runtime/libBlueprintRuntime.a
```

---

## Linux / macOS – Shared Library

**Via build script (recommended):**
```sh
./build.sh runtime shared      # Runtime-only .so/.dylib
./build.sh shared              # Full editor + shared lib
```

**Raw CMake:**
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

### Step 2 – Build

**Via build script (recommended):**
```sh
./build.sh wasm              # Linux/macOS host
build.bat wasm               # Windows host
# Output: build-wasm/Runtime/libBlueprintRuntime.a
```

Pass `--emsdk <path>` if `emcmake` is not already on `$PATH`.

**Raw CMake:**
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

## Android

Builds `libBlueprintRuntime.so` (shared) or `.a` (static) for Android ARM64-v8a.  
Requires the **Android NDK** (r21+).

**Via build script (recommended):**
```sh
# Linux/macOS host
./build.sh android --ndk ~/Library/Android/sdk/ndk/25.2.9519653

# Windows host
build.bat android --ndk C:\android-ndk

# Shared library
./build.sh android shared --ndk ~/ndk

# Specify min API level (default: 21)
./build.sh android --ndk ~/ndk --api 24
```

The NDK can also be provided via environment variable (`$ANDROID_NDK` or `$ANDROID_NDK_HOME`).

**Raw CMake:**
```sh
cmake -B build-android \
    -DCMAKE_TOOLCHAIN_FILE=~/ndk/build/cmake/android.toolchain.cmake \
    -DANDROID_ABI=arm64-v8a \
    -DANDROID_PLATFORM=android-21 \
    -DBUILD_RUNTIME_ONLY=ON \
    -DCMAKE_BUILD_TYPE=Release
cmake --build build-android
# Output: build-android/Runtime/libBlueprintRuntime.a (or .so)
```

---

## iOS

Builds `libBlueprintRuntime.a` (static) for iOS arm64.  
**Must be run on a macOS host with Xcode 14+ installed.**

**Via build script (recommended):**
```sh
./build.sh ios
./build.sh ios debug
# Output: build-ios/Runtime/libBlueprintRuntime.a
```

**Raw CMake:**
```sh
cmake -B build-ios \
    -DCMAKE_SYSTEM_NAME=iOS \
    -DCMAKE_OSX_ARCHITECTURES=arm64 \
    -DCMAKE_OSX_DEPLOYMENT_TARGET=13.0 \
    -DBUILD_RUNTIME_ONLY=ON \
    -DBUILD_SHARED_LIBS=OFF \
    -DCMAKE_BUILD_TYPE=Release \
    -G Xcode
cmake --build build-ios --config Release
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
