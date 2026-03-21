// BuiltinNodeDefs.cpp -- 内置节点定义注册（入口）
//
// 按分类拆分到 builtins/ 子目录：
//   builtins/NodeDefs_Flow.cpp     - Flow Control（Branch, DoN, ForLoop, WhileLoop, ...）
//   builtins/NodeDefs_Action.cpp   - Actions（InputActionFire, SetTimer, TraceByChannel, ...）
//   builtins/NodeDefs_Math.cpp     - Math（+, -, *, /, <, >, ==, AND, OR, IntToFloat, Abs, Clamp, ...）
//   builtins/NodeDefs_Debug.cpp    - Debug（PrintString, MakeString, AppendString, StringLength, Log）
//   builtins/NodeDefs_Tree.cpp     - Behavior Tree（Sequence, MoveTo, RandomWait）
//   builtins/NodeDefs_Houdini.cpp  - Houdini（HoudiniTransform, HoudiniGroup）
//   builtins/NodeDefs_Misc.cpp     - Misc（Message, Comment, GetVariable, SetVariable）
//
#include "BlueprintEditor.h"

void BlueprintEditor::RegisterBuiltinNodeDefinitions()
{
    // --- 注册分类 ---
    auto addCat = [this](const char* id, const char* name) {
        RTNodeCategory cat;
        cat.id = id;
        cat.name = name;
        m_NodeRegistry.registerCategory(cat);
    };
    addCat("Flow",     "Flow Control");
    addCat("Action",   "Actions");
    addCat("Math",     "Math");
    addCat("Debug",    "Debug");
    addCat("Tree",     "Behavior Tree");
    addCat("Houdini",  "Houdini");
    addCat("Misc",     "Misc");

    // --- 注册各分类的节点定义 ---
    RegisterNodeDefs_Flow();
    RegisterNodeDefs_Action();
    RegisterNodeDefs_Math();
    RegisterNodeDefs_Debug();
    RegisterNodeDefs_Tree();
    RegisterNodeDefs_Houdini();
    RegisterNodeDefs_Misc();
}
