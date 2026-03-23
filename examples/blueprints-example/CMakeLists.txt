# ============================================================================
# blueprints-example 蓝图编辑器
# ============================================================================
#
# 注意：本目录的构建由上级 examples/CMakeLists.txt 中的
#       add_simple_example(blueprints-example) 宏驱动，
#       该宏使用 GLOB_RECURSE 自动发现本目录下所有 .cpp/.h 文件。
#
# 源文件清单（供参考）：
#   main.cpp              - 入口点
#   BlueprintEditor.h     - 编辑器类声明
#   BlueprintEditor.cpp   - 编辑器核心逻辑
#   EditorTypes.h         - 编辑器数据类型定义
#   EditorUI.cpp          - UI 渲染
#   ExecutionPanel.cpp    - 执行面板 UI
#   FileOperations.cpp    - 文件操作（新建/打开/保存/另存为）
#
#   --- 节点定义 & 处理器 ---
#   BuiltinNodeDefs.cpp           - 委托给 Runtime/BuiltinNodeDefs.cpp
#   BuiltinHandlers.cpp           - 委托给 Runtime/BuiltinHandlers.cpp + Editor 层覆盖
#   （具体实现已迁移到 Runtime 层，详见 Runtime/BuiltinNodeDefs.cpp 和
#    Runtime/BuiltinHandlers.cpp）
#
#   utilities/builders.h/.cpp   - 节点构建辅助
#   utilities/drawing.h/.cpp    - 绘制辅助
#   utilities/widgets.h/.cpp    - 控件辅助
#
# blueprints-example 特有的纹理资源由上级 CMakeLists.txt 的
# copy_resources target 统一处理。
