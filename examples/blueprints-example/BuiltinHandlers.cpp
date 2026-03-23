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
    // 委托给 Runtime 层注册所有内置处理器
    // 第三个参数接收处理器映射表，供子蓝图继承
    ::NodeEditor::Runtime::RegisterBuiltinHandlers(
        m_PersistentRunner, m_CurrentFilePath, &m_HandlerRegistry);
}
