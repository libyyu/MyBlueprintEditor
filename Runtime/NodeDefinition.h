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
    // ── Runtime 字段（BlueprintRunner 执行时使用）────────────────────────────
    std::string                     id;              // 节点类型ID（唯一）
    std::string                     name;            // 显示名称
    std::string                     category;        // 所属类别
    NodeType                        type = NodeType::Blueprint;
    int                             version = 1;     // 节点定义版本号（引脚变化时递增）
    std::vector<PinDefinition>      inputPins;
    std::vector<PinDefinition>      outputPins;
    bool                            isAbstract = false;
    bool                            isPure = true;

    // ── Editor-only 字段（Runtime 层不消费，仅编辑器渲染使用）───────────────
    std::string                     description;     // 节点描述（Tooltip）
    std::string                     icon;            // 图标 Unicode（FontAwesome）
    std::string                     color;           // 标题栏颜色（十六进制 RRGGBB）
    NodeSize                        defaultSize;     // 默认尺寸（Comment 节点）

    // ── 扩展属性（新字段先放这里，稳定后提升为顶层）────────────────────────
    std::unordered_map<std::string, std::string> customProperties;
};

// ============================================================================
// 节点注册表接口
// ============================================================================

class BLUEPRINT_API INodeRegistry
{
public:
    virtual ~INodeRegistry() = default;
    
    // 注册节点定义
    virtual bool registerNode(const NodeDefinition& definition) = 0;
    
    // 注销节点定义
    virtual bool unregisterNode(const std::string& nodeId) = 0;
    
    // 获取节点定义
    virtual const NodeDefinition* getNodeDefinition(const std::string& nodeId) const = 0;
    
    // 获取所有节点定义（返回 const 引用，避免拷贝）
    virtual const std::vector<const NodeDefinition*>& getAllNodeDefinitions() const = 0;
    
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
// 默认节点注册表实现（方法实现在 NodeDefinition.cpp）
// ============================================================================

#ifdef _MSC_VER
#   pragma warning(push)
#   pragma warning(disable: 4251)  // STL 成员在 DLL 接口中不可见（安全：同一 CRT）
#endif

class BLUEPRINT_API DefaultNodeRegistry : public INodeRegistry
{
public:
    bool registerNode(const NodeDefinition& definition) override;
    bool unregisterNode(const std::string& nodeId) override;
    const NodeDefinition* getNodeDefinition(const std::string& nodeId) const override;
    const std::vector<const NodeDefinition*>& getAllNodeDefinitions() const override;
    std::vector<NodeDefinition> getNodesByCategory(const std::string& categoryId) const override;
    std::vector<NodeDefinition> getNodesByCategoryPrefix(const std::string& prefix) const override;
    bool registerCategory(const NodeCategory& category) override;
    std::vector<NodeCategory> getAllCategories() const override;
    std::vector<std::string> getAllCategoryPaths() const override;

private:
    void rebuildAllDefsCache() const;

    std::unordered_map<std::string, NodeDefinition>   m_nodeDefinitions;
    std::unordered_map<std::string, NodeCategory>     m_categories;
    mutable std::vector<const NodeDefinition*>        m_allDefsCache;
};

#ifdef _MSC_VER
#   pragma warning(pop)
#endif

} // namespace Runtime
} // namespace NodeEditor
