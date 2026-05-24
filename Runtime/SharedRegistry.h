// Runtime/SharedRegistry.h
//
// 进程级共享注册表（HandlerRegistry + NodeDefRegistry）。
// 所有 BlueprintRunner 共享同一份注册表，避免每 Runner 复制几百个内置 handler。
//
// 注册来源（按使用方式区分）：
//   · C++ 内置：RegisterBuiltinHandlers(...) 在进程启动时一次性注册
//   · C-API：BP_RegisterHandler / BP_RegisterNodeDef 通过 RunnerRegistry 反向写入
//   · Lua：Blueprint.RegisterHandler / RegisterNodeDef 写入并由 LuaOwnershipTracker
//          跟踪所有权，VM 关闭时自动 Unregister
//
// 线程安全：注册/注销/查询均加互斥锁。Find 返回值类型注意：
//   · HandlerRegistry::Find 返回拷贝（NodeHandler 本身就是 shared 引用计数的 functor）
//   · NodeDefRegistry::Find 返回 const* 指向内部存储；Get 后立即使用，不要长期持有
//
// 析构顺序：Meyers singleton。如果有持有 lua_State 的 closure，它的析构会触发
//   LuaStateGuard.lock() 检查，guard 已死则 no-op（参见 LuaBindings.cpp）。

#pragma once

#include "BlueprintExport.h"
#include "NodeDefinition.h"
#include <functional>
#include <unordered_map>
#include <mutex>
#include <string>
#include <vector>

namespace NodeEditor {
namespace Runtime {

class ExecutionContext;
using NodeHandler = std::function<bool(ExecutionContext& context)>;
#ifdef _MSC_VER
#   pragma warning(push)
#   pragma warning(disable: 4251)  // STL members in DLL-exported class
#endif
// =============================================================================
// HandlerRegistry — 全局节点处理器注册表
// =============================================================================
class BLUEPRINT_API HandlerRegistry
{
public:
    static HandlerRegistry& Instance();

    // 注册 / 替换 handler。重复 id 后注册覆盖前面（无警告，调用方自负）。
    void Register(const std::string& defId, NodeHandler handler);

    // 注销。id 不存在则 no-op。
    void Unregister(const std::string& defId);

    // 批量注销（VM 关闭时按 id 集合清理）
    void UnregisterMany(const std::vector<std::string>& defIds);

    // 查找。未注册返回空 NodeHandler（operator bool == false）。
    NodeHandler Find(const std::string& defId) const;

    bool Has(const std::string& defId) const;

    // 进程退出时 registry 可能先于 LuaScriptEngine 析构，用于安全检查
    static bool IsAlive();

    // 调试用：当前已注册的 handler 数
    size_t Size() const;

    // 清空（仅用于测试或进程退出前的显式清理）
    void Clear();

private:
    HandlerRegistry();
    ~HandlerRegistry();
    HandlerRegistry(const HandlerRegistry&)            = delete;
    HandlerRegistry& operator=(const HandlerRegistry&) = delete;

    mutable std::mutex                              m_mutex;
    std::unordered_map<std::string, NodeHandler>    m_handlers;
};

// =============================================================================
// NodeDefRegistry — 全局节点定义注册表（动态注册的 NodeDefinition）
// =============================================================================
//
// 注：C++ 内置的 NodeDef（BuiltinNodeDefs.cpp 中静态定义）由 BuiltinNodeRegistry
// 单独管理，编辑器/运行时合并查询时优先级：本表 > Builtin。
class BLUEPRINT_API NodeDefRegistry
{
public:
    static NodeDefRegistry& Instance();

    void Register(const NodeDefinition& def);
    void Unregister(const std::string& id);
    void UnregisterMany(const std::vector<std::string>& ids);

    // 返回内部存储指针；调用方在 Find/Unregister 间不得跨多线程持有。
    const NodeDefinition* Find(const std::string& id) const;

    bool Has(const std::string& id) const;
    size_t Size() const;

    static bool IsAlive();

    // 拍平复制一份所有定义（编辑器面板用，避免持有内部锁）
    std::vector<NodeDefinition> GetAll() const;

    void Clear();

private:
    NodeDefRegistry();
    ~NodeDefRegistry();
    NodeDefRegistry(const NodeDefRegistry&)            = delete;
    NodeDefRegistry& operator=(const NodeDefRegistry&) = delete;

    mutable std::mutex                                  m_mutex;
    std::unordered_map<std::string, NodeDefinition>     m_defs;
};

#ifdef _MSC_VER
#   pragma warning(pop)
#endif

} // namespace Runtime
} // namespace NodeEditor
