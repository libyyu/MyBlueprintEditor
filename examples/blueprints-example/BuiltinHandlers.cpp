// BuiltinHandlers.cpp -- 内置节点运行时处理器注册（入口）
//
// 按分类拆分到 builtins/ 子目录：
//   builtins/Handlers_Flow.cpp     - Flow Control（Branch, DoN, ForLoop, WhileLoop, ...）
//   builtins/Handlers_Action.cpp   - Actions（SetTimer, OutputAction, InputActionFire, TraceByChannel）
//   builtins/Handlers_Math.cpp     - Math（+, -, *, /, <, >, ==, AND, OR, IntToFloat, Abs, Clamp, ...）
//   builtins/Handlers_Debug.cpp    - Debug（PrintString, Log）
//   builtins/Handlers_String.cpp   - Misc/String（MakeString, AppendString, StringLength, StringSplit, ...）
//   builtins/Handlers_Array.cpp    - Misc/Array（ArrayLength, ArrayGet, ForEachLoop）
//   builtins/Handlers_Tree.cpp     - Behavior Tree（Sequence, MoveTo, RandomWait）
//   builtins/Handlers_Houdini.cpp  - Houdini（HoudiniTransform, HoudiniGroup）
//   builtins/Handlers_Misc.cpp     - Misc（GetVariable, SetVariable）
//
#include "BlueprintEditor.h"

void BlueprintEditor::RegisterBuiltinHandlers()
{
    // Default handler: pass-through
    m_DefaultHandler = [this](RTContext& ctx) {
        auto node = ctx.GetCurrentNode();
        if (!node) return true;
        ctx.Log("  [Default Handler] pass-through");
        return true;
    };

    // --- 注册各分类的处理器 ---
    RegisterHandlers_Flow();
    RegisterHandlers_Action();
    RegisterHandlers_Math();
    RegisterHandlers_Debug();
    RegisterHandlers_String();
    RegisterHandlers_Array();
    RegisterHandlers_Tree();
    RegisterHandlers_Houdini();
    RegisterHandlers_Misc();
}
