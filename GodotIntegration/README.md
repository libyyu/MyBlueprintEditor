# BlueprintRuntime × Godot 4.5 集成（阶段 0-1 骨架）

把你的 C++ 蓝图引擎 `BlueprintRuntime` 通过 GDExtension 接入 Godot 4.5。
这是迁移路线的**阶段 1 验证点**：在真实 Godot 游戏循环里加载并运行 `blueprint_1_1.bjson`。

## 目录结构

```
GodotIntegration/
├── godot-cpp/                 # godot-cpp 4.5（git clone，已就位）
├── src/                       # GDExtension C++ 绑定层
│   ├── blueprint_node.h/.cpp  # BlueprintNode：把 C API 包成 Godot Node
│   └── register_types.h/.cpp  # GDExtension 入口，注册 BlueprintNode
├── CMakeLists.txt             # 链接 godot-cpp + BlueprintRuntime
└── game/                      # Godot 工程（用 Godot 4.5 打开这个目录）
    ├── project.godot
    ├── test_blueprint.tscn    # 测试场景
    ├── test_blueprint.gd      # 驱动脚本
    ├── blueprints/blueprint_1_1.bjson
    └── bin/
        ├── blueprint.gdextension          # 扩展清单
        ├── blueprint_gdext.*.dll          # 编译产物（CMake 生成）
        └── BlueprintRuntime.dll           # 引擎本体（自动拷入）
```

## 关键设计

- **BlueprintNode** 继承 Godot `Node`，等价于 Unity 的 `BlueprintBehaviour`：
  `_ready()` 建 runner、加载蓝图、Execute；`_process()` 每帧 `BP_DrainQueue + BP_Tick`。
- **无共享 VM 烦恼**：Lua VM 完全活在 `BlueprintRuntime` 内部（`BLUEPRINT_LUA=ON`）。
  Godot 不碰 Lua，少了 Unity 那套 `SetExternalLuaState` 注入和野指针防护。
- **输出回流**：引擎的 `PrintString/Log` 通过回调转到 Godot 的 `print()`。
- **GDScript API**：`load_blueprint / execute / dispatch_event / set_var_* / get_var_*`。

## 构建步骤（Windows / MSVC）

```bash
cd GodotIntegration
cmake -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release --target blueprint_gdext
```

产物：`game/bin/blueprint_gdext.windows.template_debug.x86_64.dll`，
且会自动把 `BlueprintRuntime.dll` 拷到 `game/bin/`。

## 运行验证

```bash
"C:/Users/maxweili/MyProjects/Godot_v4.5/Godot_v4.5-stable_win64_console.exe" \
    --path "C:/Users/maxweili/MyProjects/MyBlueprintEditor/GodotIntegration/game"
```

或：用 Godot 4.5 编辑器打开 `game/` 目录，按 F5 运行。

**成功标志**（控制台输出）：
```
=== Blueprint GDExtension test ===
[BlueprintNode] loaded: .../blueprint_1_1.bjson
is_loaded: true
Health var round-trip: 100
[BP] ...（蓝图里 PrintString 节点的输出）
=== test running; BP_Tick is called every frame ===
```

看到 `is_loaded: true` 即代表**迁移路线成立**——你的引擎已在 Godot 里跑起来了。

## 依赖说明

- `BlueprintRuntime.dll` + `BlueprintRuntime.lib`（import 库）来自
  `build-windows/`（MSVC 产物，与 godot-cpp 的 MSVC 编译 ABI 一致）。
- 若重新编译了引擎，更新 CMakeLists.txt 顶部的 `BP_RUNTIME_DLL/LIB` 路径即可。

## 下一步（阶段 2+）

1. 把 `UnityProj/Assets/Lua/blueprints/*.lua` 的节点 handler 迁过来（Lua 几乎原样）。
2. 把 `CS.UnityEngine.*` / `CS.UGFramework.*` 调用改成走 C API 节点回调到 Godot。
3. 资源系统（YooAsset → Godot ResourceLoader）、UI、输入逐步补齐。
