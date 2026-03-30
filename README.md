# MyBlueprintEditor

![Build](https://github.com/libyyu/MyBlueprintEditor/actions/workflows/build.yml/badge.svg)

通用可视化脚本编程框架 — 独立于引擎的蓝图运行时 + ImGui 蓝图编辑器。

```
┌─────────────────────────────────────────────────────┐
│  BlueprintRuntime  — 可嵌入任意 C++ 项目            │
│    → Unity WebGL / Unreal / Godot / 自研引擎        │
├─────────────────────────────────────────────────────┤
│  BlueprintEditor   — 跨平台 ImGui 编辑器             │
│    → Windows DX11 / Linux macOS OpenGL3             │
└─────────────────────────────────────────────────────┘
```

## 特性

- **Runtime 与 Editor 完全解耦**：BlueprintRuntime 可单独编译为静态/动态库嵌入任意项目
- **140+ 内置节点**：Flow、Math、String、Array、Map、Debug、Event 等分类
- **Lua 脚本扩展**：运行时注册自定义节点处理器
- **多文档 + 工程系统**：VSCode 风格多标签页，`.bp.proj` 工程文件
- **断点调试**：节点级断点、单步执行、执行高亮
- **跨平台**：Windows (MSVC/DX11)、Linux/macOS (GCC/GLFW+OpenGL3)、WebGL (Emscripten)、Android

## 快速开始

### 环境要求

- CMake 3.14+
- C++17 编译器（MSVC 2019+、GCC 9+、Clang 10+）
- Windows：Visual Studio 2019/2022
- Linux/macOS：libglfw3-dev + libgl1-mesa-dev

### Windows 构建（推荐）

```bat
:: 完整编辑器（默认：shared runtime + DX11）
build.bat

:: Debug 版
build.bat debug

:: 静态运行时
build.bat windows-static

:: 仅运行时 DLL（无编辑器）
build.bat dll
```

输出目录：`build-windows/bin/Release/`

### Linux / macOS

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
./build/bin/BlueprintEditor
```

### WebGL (Emscripten)

```bash
emcmake cmake -B build-wasm -DBUILD_RUNTIME_ONLY=ON -DCMAKE_BUILD_TYPE=Release
cmake --build build-wasm
# → build-wasm/Runtime/libBlueprintRuntime.a
# 放入 Unity 项目的 Assets/Plugins/WebGL/
```

### Android

```bat
build.bat android --ndk C:\path\to\ndk
```

## 运行单元测试

```bash
# Windows
cmake --build build-windows --config Release --target blueprint_tests
ctest --test-dir build-windows -C Release --output-on-failure

# Linux
cmake --build build --target blueprint_tests
ctest --test-dir build --output-on-failure
```

## 项目结构

```
BlueprintEditor/    编辑器 UI（ImGui）
Runtime/            运行时核心（执行引擎、节点系统、序列化）
Editor/             ImGuiNodeEditor 库
Application/        平台抽象（Win32/DX11 或 GLFW/OpenGL3）
Utils/Json/         crude_json 轻量 JSON 解析器
examples/           示例程序
tests/              单元测试（Google Test）
data/               字体、图标、示例蓝图
cmake/              CMake 辅助文件（natvis、Toolchains）
```

## 嵌入到你的项目

```cmake
# CMakeLists.txt
add_subdirectory(MyBlueprintEditor/Runtime)
target_link_libraries(YourTarget PRIVATE BlueprintRuntime)
```

```cpp
#include "BlueprintRunner.h"
#include "BuiltinNodeDefs.h"
#include "BuiltinHandlers.h"

using namespace NodeEditor::Runtime;

BlueprintRunner runner;
RegisterBuiltinHandlers(runner, ".");
runner.LoadFromFile("my_blueprint.json");
runner.Execute();
```

## build.bat 选项

| 命令 | 说明 |
|---|---|
| `build.bat` | Windows Release，shared runtime + 完整编辑器 |
| `build.bat debug` | Windows Debug |
| `build.bat windows-static` | Windows Release，静态 runtime |
| `build.bat dll` | Windows Release，Runtime-only 共享 DLL |
| `build.bat wasm` | WebGL (Emscripten) |
| `build.bat android --ndk <path>` | Android ARM64 |
| `build.bat clean` | 清理构建目录后重新构建 |

## 许可

See [LICENSE](LICENSE).
