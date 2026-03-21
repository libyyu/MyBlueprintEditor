// Runtime/NodeDefinition.h - 节点定义结构
// 该文件定义了节点的模板和类型信息，用于创建和验证节点实例

#pragma once

#include "Types.h"
#include <vector>
#include <string>
#include <functional>

namespace NodeEditor {
namespace Runtime {

// ============================================================================
// 引脚定义（用于节点模板）
// ============================================================================

struct PinDefinition
{
    std::string     name;
    PinKind         kind = PinKind::Input;
    PinDataType     dataType = PinDataType::Unknown;
    Variant         defaultValue;
    bool            allowMultiple = false;
    bool            isExec = false;
    bool            isRequired = false;  // 是否必须连接
    std::string     tooltip;             // 提示信息
    
    // 自定义属性
    std::unordered_map<std::string, std::string> customProperties;
};

// ============================================================================
// 节点类别
// ============================================================================

struct NodeCategory
{
    std::string                 id;          // 类别ID
    std::string                 name;        // 显示名称
    std::string                 icon;        // 图标（可选）
    std::vector<std::string>    subCategories; // 子类别ID列表
};

// ============================================================================
// 节点模板定义
// ============================================================================

struct NodeDefinition
{
    std::string                     id;              // 节点类型ID（唯一）
    std::string                     name;            // 显示名称
    std::string                     category;        // 所属类别
    NodeType                        type = NodeType::Blueprint;
    
    // 引脚定义
    std::vector<PinDefinition>      inputPins;
    std::vector<PinDefinition>      outputPins;
    
    // 节点属性
    std::string                     description;     // 节点描述
    std::string                     icon;            // 图标（可选）
    std::string                     color;           // 颜色（十六进制）
    
    // 节点位置（用于预览）
    NodeSize                        defaultSize;     // 默认尺寸
    
    // 自定义属性
    std::unordered_map<std::string, std::string> customProperties;
    
    // 是否可实例化
    bool                            isAbstract = false;
    
    // 是否是纯函数节点（无副作用）
    bool                            isPure = true;
};

// ============================================================================
// 节点注册表接口
// ============================================================================

class INodeRegistry
{
public:
    virtual ~INodeRegistry() = default;
    
    // 注册节点定义
    virtual bool registerNode(const NodeDefinition& definition) = 0;
    
    // 注销节点定义
    virtual bool unregisterNode(const std::string& nodeId) = 0;
    
    // 获取节点定义
    virtual const NodeDefinition* getNodeDefinition(const std::string& nodeId) const = 0;
    
    // 获取所有节点定义
    virtual std::vector<NodeDefinition> getAllNodeDefinitions() const = 0;
    
    // 获取类别下的所有节点（精确匹配 category 字段）
    virtual std::vector<NodeDefinition> getNodesByCategory(const std::string& categoryId) const = 0;
    
    // 获取类别下的所有节点（前缀匹配，含子分类）
    // 例如 "Math" 会匹配 "Math", "Math/Arithmetic", "Math/Logic" 等
    virtual std::vector<NodeDefinition> getNodesByCategoryPrefix(const std::string& prefix) const = 0;
    
    // 注册类别
    virtual bool registerCategory(const NodeCategory& category) = 0;
    
    // 获取所有类别
    virtual std::vector<NodeCategory> getAllCategories() const = 0;
    
    // 获取所有唯一的子分类路径（用于构建多级菜单）
    // 返回所有出现过的 category 字段值（去重）
    virtual std::vector<std::string> getAllCategoryPaths() const = 0;
};

// ============================================================================
// 默认节点注册表实现
// ============================================================================

class DefaultNodeRegistry : public INodeRegistry
{
public:
    bool registerNode(const NodeDefinition& definition) override
    {
        if (definition.id.empty()) return false;
        
        m_nodeDefinitions[definition.id] = definition;
        return true;
    }
    
    bool unregisterNode(const std::string& nodeId) override
    {
        auto it = m_nodeDefinitions.find(nodeId);
        if (it == m_nodeDefinitions.end()) return false;
        
        m_nodeDefinitions.erase(it);
        return true;
    }
    
    const NodeDefinition* getNodeDefinition(const std::string& nodeId) const override
    {
        auto it = m_nodeDefinitions.find(nodeId);
        return (it != m_nodeDefinitions.end()) ? &it->second : nullptr;
    }
    
    std::vector<NodeDefinition> getAllNodeDefinitions() const override
    {
        std::vector<NodeDefinition> result;
        for (const auto& pair : m_nodeDefinitions)
        {
            result.push_back(pair.second);
        }
        return result;
    }
    
    std::vector<NodeDefinition> getNodesByCategory(const std::string& categoryId) const override
    {
        std::vector<NodeDefinition> result;
        for (const auto& pair : m_nodeDefinitions)
        {
            if (pair.second.category == categoryId)
            {
                result.push_back(pair.second);
            }
        }
        return result;
    }
    
    std::vector<NodeDefinition> getNodesByCategoryPrefix(const std::string& prefix) const override
    {
        std::vector<NodeDefinition> result;
        for (const auto& pair : m_nodeDefinitions)
        {
            const auto& cat = pair.second.category;
            // 精确匹配 或 前缀+/ 匹配
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
    
    bool registerCategory(const NodeCategory& category) override
    {
        if (category.id.empty()) return false;
        
        m_categories[category.id] = category;
        return true;
    }
    
    std::vector<NodeCategory> getAllCategories() const override
    {
        std::vector<NodeCategory> result;
        for (const auto& pair : m_categories)
        {
            result.push_back(pair.second);
        }
        return result;
    }
    
    std::vector<std::string> getAllCategoryPaths() const override
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
    
private:
    std::unordered_map<std::string, NodeDefinition>   m_nodeDefinitions;
    std::unordered_map<std::string, NodeCategory>     m_categories;
};

} // namespace Runtime
} // namespace NodeEditor
