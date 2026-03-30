// Runtime/NodeDefinition.cpp -- DefaultNodeRegistry 实现
// 从 NodeDefinition.h 移出，避免头文件内联大量 STL 容器操作
// 导致多个编译单元各自实例化，以及潜在的 ODR/重复实例化问题。
#include "NodeDefinition.h"

namespace NodeEditor {
namespace Runtime {

bool DefaultNodeRegistry::registerNode(const NodeDefinition& definition)
{
    if (definition.id.empty()) return false;

    m_nodeDefinitions[definition.id] = definition;

    // 统一重建缓存：unordered_map 在 insert/rehash 时会使所有迭代器和指针失效，
    // 因此无论新增还是更新都必须重建，避免缓存中残留野指针。
    rebuildAllDefsCache();

    return true;
}

bool DefaultNodeRegistry::unregisterNode(const std::string& nodeId)
{
    auto it = m_nodeDefinitions.find(nodeId);
    if (it == m_nodeDefinitions.end()) return false;

    m_nodeDefinitions.erase(it);
    rebuildAllDefsCache();
    return true;
}

const NodeDefinition* DefaultNodeRegistry::getNodeDefinition(const std::string& nodeId) const
{
    auto it = m_nodeDefinitions.find(nodeId);
    return (it != m_nodeDefinitions.end()) ? &it->second : nullptr;
}

const std::vector<const NodeDefinition*>& DefaultNodeRegistry::getAllNodeDefinitions() const
{
    return m_allDefsCache;
}

std::vector<NodeDefinition> DefaultNodeRegistry::getNodesByCategory(const std::string& categoryId) const
{
    std::vector<NodeDefinition> result;
    for (const auto& pair : m_nodeDefinitions)
    {
        if (pair.second.category == categoryId)
            result.push_back(pair.second);
    }
    return result;
}

std::vector<NodeDefinition> DefaultNodeRegistry::getNodesByCategoryPrefix(const std::string& prefix) const
{
    std::vector<NodeDefinition> result;
    for (const auto& pair : m_nodeDefinitions)
    {
        const auto& cat = pair.second.category;
        if (cat == prefix ||
            (cat.size() > prefix.size() &&
             cat.compare(0, prefix.size(), prefix) == 0 &&
             cat[prefix.size()] == '/'))
        {
            result.push_back(pair.second);
        }
    }
    return result;
}

bool DefaultNodeRegistry::registerCategory(const NodeCategory& category)
{
    if (category.id.empty()) return false;
    m_categories[category.id] = category;
    return true;
}

std::vector<NodeCategory> DefaultNodeRegistry::getAllCategories() const
{
    std::vector<NodeCategory> result;
    for (const auto& pair : m_categories)
        result.push_back(pair.second);
    return result;
}

std::vector<std::string> DefaultNodeRegistry::getAllCategoryPaths() const
{
    std::vector<std::string> result;
    std::unordered_map<std::string, bool> seen;
    for (const auto& pair : m_nodeDefinitions)
    {
        const auto& cat = pair.second.category;
        if (!cat.empty() && seen.find(cat) == seen.end())
        {
            seen[cat] = true;
            result.push_back(cat);
        }
    }
    return result;
}

void DefaultNodeRegistry::rebuildAllDefsCache() const
{
    m_allDefsCache.clear();
    m_allDefsCache.reserve(m_nodeDefinitions.size());
    for (const auto& pair : m_nodeDefinitions)
        m_allDefsCache.push_back(&pair.second);
}

} // namespace Runtime
} // namespace NodeEditor
