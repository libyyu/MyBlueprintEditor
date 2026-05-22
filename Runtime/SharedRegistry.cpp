// Runtime/SharedRegistry.cpp

#include "SharedRegistry.h"

namespace NodeEditor {
namespace Runtime {

// =============================================================================
// HandlerRegistry
// =============================================================================

HandlerRegistry& HandlerRegistry::Instance()
{
    static HandlerRegistry inst;
    return inst;
}

// 进程退出时 registry 可能先于 LuaScriptEngine 析构。
// 析构后把 s_dead 置 true，Unregister 检查后直接跳过，避免 use-after-free。
static bool s_handlerRegistryDead = false;

struct HandlerRegistryDeadFlag {
    ~HandlerRegistryDeadFlag() { s_handlerRegistryDead = true; }
};
static HandlerRegistryDeadFlag s_handlerRegistryDeadFlag;

bool HandlerRegistry::IsAlive() { return !s_handlerRegistryDead; }
}

void HandlerRegistry::Register(const std::string& defId, NodeHandler handler)
{
    std::lock_guard<std::mutex> lk(m_mutex);
    m_handlers[defId] = std::move(handler);
}

void HandlerRegistry::Unregister(const std::string& defId)
{
    std::lock_guard<std::mutex> lk(m_mutex);
    m_handlers.erase(defId);
}

void HandlerRegistry::UnregisterMany(const std::vector<std::string>& defIds)
{
    std::lock_guard<std::mutex> lk(m_mutex);
    for (const auto& id : defIds) m_handlers.erase(id);
}

NodeHandler HandlerRegistry::Find(const std::string& defId) const
{
    std::lock_guard<std::mutex> lk(m_mutex);
    auto it = m_handlers.find(defId);
    return (it != m_handlers.end()) ? it->second : NodeHandler{};
}

bool HandlerRegistry::Has(const std::string& defId) const
{
    std::lock_guard<std::mutex> lk(m_mutex);
    return m_handlers.find(defId) != m_handlers.end();
}

size_t HandlerRegistry::Size() const
{
    std::lock_guard<std::mutex> lk(m_mutex);
    return m_handlers.size();
}

void HandlerRegistry::Clear()
{
    std::lock_guard<std::mutex> lk(m_mutex);
    m_handlers.clear();
}

// =============================================================================
// NodeDefRegistry
// =============================================================================

NodeDefRegistry& NodeDefRegistry::Instance()
{
    static NodeDefRegistry inst;
    return inst;
}

static bool s_nodeDefRegistryDead = false;
struct NodeDefRegistryDeadFlag {
    ~NodeDefRegistryDeadFlag() { s_nodeDefRegistryDead = true; }
};
static NodeDefRegistryDeadFlag s_nodeDefRegistryDeadFlag;

bool NodeDefRegistry::IsAlive() { return !s_nodeDefRegistryDead; }

void NodeDefRegistry::Register(const NodeDefinition& def)
{
    std::lock_guard<std::mutex> lk(m_mutex);
    m_defs[def.id] = def;
}

void NodeDefRegistry::Unregister(const std::string& id)
{
    std::lock_guard<std::mutex> lk(m_mutex);
    m_defs.erase(id);
}

void NodeDefRegistry::UnregisterMany(const std::vector<std::string>& ids)
{
    std::lock_guard<std::mutex> lk(m_mutex);
    for (const auto& id : ids) m_defs.erase(id);
}

const NodeDefinition* NodeDefRegistry::Find(const std::string& id) const
{
    std::lock_guard<std::mutex> lk(m_mutex);
    auto it = m_defs.find(id);
    return (it != m_defs.end()) ? &it->second : nullptr;
}

bool NodeDefRegistry::Has(const std::string& id) const
{
    std::lock_guard<std::mutex> lk(m_mutex);
    return m_defs.find(id) != m_defs.end();
}

size_t NodeDefRegistry::Size() const
{
    std::lock_guard<std::mutex> lk(m_mutex);
    return m_defs.size();
}

std::vector<NodeDefinition> NodeDefRegistry::GetAll() const
{
    std::lock_guard<std::mutex> lk(m_mutex);
    std::vector<NodeDefinition> out;
    out.reserve(m_defs.size());
    for (const auto& kv : m_defs) out.push_back(kv.second);
    return out;
}

void NodeDefRegistry::Clear()
{
    std::lock_guard<std::mutex> lk(m_mutex);
    m_defs.clear();
}

} // namespace Runtime
} // namespace NodeEditor
