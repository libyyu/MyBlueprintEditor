// BuiltinHandlers.cpp -- 内置节点运行时处理器注册（入口）
//
// 委托给 Runtime 层的独立函数 RegisterBuiltinHandlers()，
// Editor 通过 SetLogCallback 接管 Runtime 的日志输出，
// 无需覆盖任何处理器。
//
#include "BlueprintEditor.h"
#include "BuiltinHandlers.h"

void BlueprintEditor::RegisterBuiltinHandlers()
{
    // 注册时仅填充 m_HandlerRegistry 映射表
    // 实际的 runner 和路径在 ExecuteBlueprint 时才使用
    static RTBlueprintRunner dummyRunner;
    ::NodeEditor::Runtime::RegisterBuiltinHandlers(
        dummyRunner, std::string(), &m_HandlerRegistry);
}
