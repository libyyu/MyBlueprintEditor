# ============================================================================
# BlueprintEditor — 蓝图编辑器
# ============================================================================
#
# 本目录的构建由根 CMakeLists.txt 直接管理。
#
# 源文件清单：
#   main.cpp              - 入口点
#   BlueprintEditor.h     - 编辑器类声明
#   BlueprintEditor.cpp   - 编辑器核心逻辑
#   EditorTypes.h         - 编辑器数据类型定义
#   EditorUI.cpp          - UI 渲染（面板布局 / 节点绘制 / 菜单 / 状态栏）
#   InspectorPanel.cpp    - (已拆分) 检视器面板，见 inspector/ 子目录
#   inspector/
#     NodeListPanel.cpp        - 右侧 Tab Bar 主入口（Nodes/Variables/Functions/Events/Details）
#     VariablePanel.cpp        - 变量面板（增删改/类型/默认值）
#     DetailsPanel.cpp         - 节点 Details 面板
#     FunctionDetailsPanel.cpp - 函数参数面板 + SyncFunctionPinsToNodes
#   ExecutionPanel.cpp    - 执行面板 UI & 蓝图执行
#   FileOperations.cpp    - 文件操作（新建/打开/保存/另存为）
#   ClipboardOps.cpp      - 剪贴板操作（复制/粘贴/剪切）
#   Minimap.cpp           - 小地图导航
#   SearchOverlay.cpp     - 搜索覆盖层（Ctrl+F）
#
#   --- 节点定义 & 处理器 ---
#   BuiltinNodeDefs.cpp   - 委托给 Runtime/BuiltinNodeDefs
#   BuiltinHandlers.cpp   - 委托给 Runtime/BuiltinHandlers
#
#   utilities/builders.h/.cpp   - 节点构建辅助
#   utilities/drawing.h/.cpp    - 绘制辅助
#   utilities/widgets.h/.cpp    - 控件辅助
#
# 依赖链：
#   BlueprintEditor -> Application -> ImGuiNodeEditor -> imgui + BlueprintRuntime
