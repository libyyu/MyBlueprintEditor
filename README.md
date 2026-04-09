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
- **160+ 内置节点**：Flow、Math、String、Array、Network、AI/LLM、AI/Agent 等分类
- **AI Agent 节点体系**：LLM.Chat / StreamChat / Tool.* / Agent.Plan / Agent.Reflect / Memory.* / Context.Compress / MCP.Call
- **MCP Server**：一行命令将蓝图执行能力暴露给 Claude Desktop、CodeBuddy 等任意 MCP 客户端
- **Lua 脚本扩展**：运行时注册自定义节点处理器（[开发指南](docs/Lua-Extension-Guide.md)）
- **多文档 + 工程系统**：VSCode 风格多标签页，`.bproj` 工程文件
- **断点调试**：节点级断点、单步执行、执行高亮
- **跨平台**：Windows (MSVC/DX11)、Linux/macOS (GCC/GLFW+OpenGL3)、WebGL (Emscripten)、Android

## AI Agent & MCP Server

BlueprintRuntime 内置完整的 AI Agent 节点体系，配套 MCP Server 可直接接入任意 AI 客户端（Claude Desktop、CodeBuddy、Cursor 等）。

### 内置 AI 节点（`data/examples/` 有完整示例蓝图）

| 分类 | 节点 | 说明 |
|------|------|------|
| LLM | `LLM.Chat` | OpenAI 兼容 Chat，支持 tool_calls 三路分叉 |
| LLM | `LLM.StreamChat` | SSE 流式输出，逐 token |
| Tool | `Tool.ForEach` | 串行遍历 tool_calls |
| Tool | `Tool.ForEachParallel` | 并发遍历 tool_calls（HTTP 工具） |
| Tool | `Tool.Match` | 最多 8 路静态工具名路由 |
| Tool | `Tool.CallByName` | 动态路由到同名 FuncLib 函数，无上限 |
| Tool | `Tool.Define` | 可视化生成 LLM function schema |
| Agent | `Agent.Plan` | 目标分解 → 步骤 JSON（Plan-Execute 模式） |
| Agent | `Agent.Reflect` | 输出质量评估，pass/fail + 改进建议 |
| Memory | `Memory.LoadHistory` / `SaveHistory` | 对话历史文件持久化 |
| Context | `Context.Compress` | LLM 摘要压缩旧历史，防 context 溢出 |
| MCP | `MCP.Call` | 调用外部 MCP Server（JSON-RPC 2.0） |
| Network | `HTTP.Get/Post/Retry` | HTTP 请求，含自动重试 |
| Network | `Web.Search` | DuckDuckGo 搜索，无需 API Key |

### 示例蓝图

| 文件 | 模式 | 说明 |
|------|------|------|
| `AgentDemo.bjson` | Single-turn | 最简 LLM 调用 |
| `ReActAgent.bjson` | ReAct | 真实工具（天气+搜索）循环 |
| `ReflectAgent.bjson` | Reflect | 生成 → 评估 → 重试 |
| `PlanExecuteAgent.bjson` | Plan-Execute | Agent.Plan 分解 + Tool.CallByName 执行 |
| `LongContextAgent.bjson` | Long Context | Context.Compress 自动压缩历史 |
| `MultiTurnAgent.bjson` | Multi-turn | WhileLoop 多轮工具调用 |
| `WebSearchAgent.bjson` | Search+Summary | 搜索 + LLM 总结 |
| `FileAgent.bjson` | File I/O | 读文件 → LLM 分析 → 写文件 |
| `MCPCallDemo.bjson` | MCP Client | 调用外部 MCP Server |
| `LuaReActAgent.bjson` | Lua Agent | 纯 Lua 实现 ReAct |

### 启动 MCP Server

```bash
# 安装依赖
pip install mcp

# stdio 模式（接入 Claude Desktop / CodeBuddy）
python tools/mcp_server.py

# HTTP REST 模式（任意程序直接调用）
python tools/mcp_server.py --http --port 7799
```

**Claude Desktop / CodeBuddy IDE 配置**（`tools/claude_desktop_config.json`）：

```json
{
  "mcpServers": {
    "blueprint": {
      "command": "python",
      "args": ["C:/path/to/MyBlueprintEditor/tools/mcp_server.py"],
      "env": {
        "BLUEPRINT_DLL": "C:/path/to/build-windows/bin/Release/BlueprintRuntime.dll",
        "BLUEPRINT_ROOT": "C:/path/to/MyBlueprintEditor"
      }
    }
  }
}
```

接入后在对话中可以说：
```
执行 data/examples/ReActAgent.bjson，ApiKey=sk-xxx，UserQuery="今天北京天气怎么样？"
帮我创建一个蓝图：调用 gpt-4o 回答问题并打印结果
```

完整说明见 [tools/README.md](tools/README.md) 和 [data/examples/README.md（已移至examples/README.md）](examples/README.md)。

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
