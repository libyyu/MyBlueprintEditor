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
#   --- 节点定义 & 处理器（按分类拆分到 builtins/ 子目录）---
#   BuiltinNodeDefs.cpp           - 入口：注册分类 + 调用子注册
#   BuiltinHandlers.cpp           - 入口：注册默认处理器 + 调用子注册
#   builtins/NodeDefs_Flow.cpp    - Flow Control 节点定义
#   builtins/NodeDefs_Action.cpp  - Action 节点定义
#   builtins/NodeDefs_Math.cpp    - Math 节点定义
#   builtins/NodeDefs_Debug.cpp   - Debug 节点定义（PrintString, Log）
#   builtins/NodeDefs_String.cpp  - Misc/String 节点定义（MakeString, AppendString, StringSplit, ...）
#   builtins/NodeDefs_Array.cpp   - Misc/Array 节点定义（ArrayLength, ArrayGet, ForEachLoop）
#   builtins/NodeDefs_Tree.cpp    - Behavior Tree 节点定义
#   builtins/NodeDefs_Houdini.cpp - Houdini 节点定义
#   builtins/NodeDefs_Misc.cpp    - Misc 节点定义
#   builtins/Handlers_Flow.cpp    - Flow Control 处理器
#   builtins/Handlers_Action.cpp  - Action 处理器
#   builtins/Handlers_Math.cpp    - Math 处理器
#   builtins/Handlers_Debug.cpp   - Debug 处理器（PrintString, Log）
#   builtins/Handlers_String.cpp  - Misc/String 处理器
#   builtins/Handlers_Array.cpp   - Misc/Array 处理器
#   builtins/Handlers_Tree.cpp    - Behavior Tree 处理器
#   builtins/Handlers_Houdini.cpp - Houdini 处理器
#   builtins/Handlers_Misc.cpp    - Misc 处理器
#
#   utilities/builders.h/.cpp   - 节点构建辅助
#   utilities/drawing.h/.cpp    - 绘制辅助
#   utilities/widgets.h/.cpp    - 控件辅助
#
# blueprints-example 特有的纹理资源由上级 CMakeLists.txt 的
# copy_resources target 统一处理。
