// Runtime/BlueprintData.h - 蓝图数据结构
// 该文件定义了完整的蓝图数据结构，用于存储和传输蓝图信息

#pragma once

#include "Types.h"
#include "NodeDefinition.h"
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <queue>
#include <string>
#include <mutex>
#include <shared_mutex>

namespace NodeEditor {
namespace Runtime {

// ============================================================================
// 节点实例数据
// ============================================================================

struct NodeInstance
{
    NodeId              id = InvalidNodeId;
    std::string         definitionId;           // 节点类型ID（对应 NodeDefinition::id）
    int                 definitionVersion = 1;  // 节点定义版本号（保存时记录 NodeDefinition::version）
    std::string         name;                   // 自定义名称（可选）
    NodePosition        position;               // 节点位置
    NodeSize            size;                   // 节点尺寸
    
    // 引脚实例数据（存储实际的引脚ID和数据）
    std::vector<PinInfo> pins;                  // 引脚列表
    
    // 节点状态
    bool                isEnabled = true;       // 是否启用
    bool                isCollapsed = false;    // 是否折叠
    
    // 自定义属性
    std::unordered_map<std::string, std::string> customProperties;
    
    // 节点数据（存储节点的配置数据）
    std::unordered_map<std::string, Variant> nodeData;
};

// ============================================================================
// 链接实例数据
// ============================================================================

struct LinkInstance
{
    LinkId  id = InvalidLinkId;
    PinId   startPinId = InvalidPinId;
    PinId   endPinId = InvalidPinId;
    
    // 链接状态
    bool    isEnabled = true;
};

// ============================================================================
// 变量定义
// ============================================================================

struct VariableDefinition
{
    std::string     name;
    PinDataType     dataType = PinDataType::Unknown;
    Variant         defaultValue;
    bool            isExposed = true;           // 是否在编辑器中显示
    std::string     category;                   // 变量类别
    std::string     tooltip;                    // 提示信息
};

// ============================================================================
// 注释/分组区域
// ============================================================================

struct CommentRegion
{
    std::string     id;
    std::string     text;
    NodePosition    position;
    NodeSize        size;
    std::string     color;                      // 背景色（十六进制）
    float           alpha = 0.5f;               // 透明度
};

// ============================================================================
// Schema 版本常量
// ============================================================================

// 当前蓝图文件格式版本号。
// 修改规则：
//   - 新增可选字段（向后兼容）→ 不递增
//   - 修改已有字段语义 / 删除字段 / 改变引脚格式 → 递增
//
// 版本历史：
//   v1 — 初始版本。isCollapsed 存在 customProperties["__collapsed"]；
//         variables 字段可能缺失。
//   v2 — isCollapsed 升为 NodeInstance 顶层字段（runtime JSON 对齐 editor JSON）；
//         variables 顶层数组标准化（缺失时补空数组）；
//         customProperties["__collapsed"] 废弃，迁移时自动转换并移除。
constexpr int BLUEPRINT_CURRENT_SCHEMA_VERSION = 2;

// ============================================================================
// 蓝图元数据
// ============================================================================

struct BlueprintMetadata
{
    int                         schemaVersion = BLUEPRINT_CURRENT_SCHEMA_VERSION; // 文件格式版本
    std::string                 name;           // 蓝图名称
    std::string                 description;    // 蓝图描述
    std::string                 author;         // 作者
    std::string                 version;        // 用户蓝图版本号
    std::string                 createdAt;      // 创建时间
    std::string                 updatedAt;      // 更新时间
    std::vector<std::string>    tags;           // 标签
    std::unordered_map<std::string, std::string> customProperties; // 自定义属性
};

// ============================================================================
// 蓝图数据
// ============================================================================

struct BlueprintData
{
    BlueprintData() = default;

    // shared_mutex 不可拷贝/移动，手动定义以保证每个实例拥有独立的锁
    BlueprintData(const BlueprintData& other)
        : metadata(other.metadata)
        , nodes(other.nodes)
        , links(other.links)
        , variables(other.variables)
        , comments(other.comments)
        , viewInfo(other.viewInfo)
        , m_indexDirty(true)   // 新对象重建索引，不拷贝缓存
    {}

    BlueprintData(BlueprintData&& other) noexcept
        : metadata(std::move(other.metadata))
        , nodes(std::move(other.nodes))
        , links(std::move(other.links))
        , variables(std::move(other.variables))
        , comments(std::move(other.comments))
        , viewInfo(std::move(other.viewInfo))
        , m_indexDirty(true)   // 移动后重建索引
    {}

    BlueprintData& operator=(const BlueprintData& other)
    {
        if (this != &other)
        {
            metadata  = other.metadata;
            nodes     = other.nodes;
            links     = other.links;
            variables = other.variables;
            comments  = other.comments;
            viewInfo  = other.viewInfo;
            // 不拷贝 m_indexMutex；不拷贝索引缓存，标记为 dirty 重建
            std::unique_lock<std::shared_mutex> wlock(m_indexMutex);
            m_nodeIdIndex.clear();
            m_pinToNodeIndex.clear();
            m_linkIdIndex.clear();
            m_startPinLinks.clear();
            m_endPinLinks.clear();
            m_indexDirty = true;
        }
        return *this;
    }

    BlueprintData& operator=(BlueprintData&& other) noexcept
    {
        if (this != &other)
        {
            metadata  = std::move(other.metadata);
            nodes     = std::move(other.nodes);
            links     = std::move(other.links);
            variables = std::move(other.variables);
            comments  = std::move(other.comments);
            viewInfo  = std::move(other.viewInfo);
            std::unique_lock<std::shared_mutex> wlock(m_indexMutex);
            m_nodeIdIndex.clear();
            m_pinToNodeIndex.clear();
            m_linkIdIndex.clear();
            m_startPinLinks.clear();
            m_endPinLinks.clear();
            m_indexDirty = true;
        }
        return *this;
    }

    // 元数据
    BlueprintMetadata                   metadata;
    
    // 节点列表
    std::vector<NodeInstance>           nodes;
    
    // 链接列表
    std::vector<LinkInstance>           links;
    
    // 变量列表
    std::vector<VariableDefinition>     variables;
    
    // 注释/分组区域
    std::vector<CommentRegion>          comments;
    
    // 视图信息（用于编辑器）
    struct ViewInfo
    {
        NodePosition    viewPosition;       // 视图位置
        float           viewScale = 1.0f;   // 视图缩放
    } viewInfo;

    // ============================================================================
    // 索引管理 —— O(1) 查找加速
    //
    // 线程安全语义：
    //   · 并发读（findNode / findLink / findLinksByPin 等）：安全，使用共享锁
    //   · 写（rebuildIndices / invalidateIndices）：需要独占锁，调用方须确保
    //     此时没有并发读写 nodes/links（通常在加载阶段单线程调用）
    //   · nodes / links 向量本身不加锁；如需并发修改数据，需外层同步
    // ============================================================================

    // 重建所有索引（在数据加载、节点/链接增删后调用）
    // 调用方须持有外层独占锁，或确保此时无并发访问
    void rebuildIndices() const
    {
        std::unique_lock<std::shared_mutex> lock(m_indexMutex);
        rebuildIndicesLocked();
    }

    // 标记索引需要重建（下次访问时懒重建）
    void invalidateIndices() const
    {
        std::unique_lock<std::shared_mutex> lock(m_indexMutex);
        m_indexDirty = true;
    }

    // 确保索引可用（内部懒重建，线程安全）
    void ensureIndices() const
    {
        // 快速路径：共享锁下检查 dirty 标志
        {
            std::shared_lock<std::shared_mutex> rlock(m_indexMutex);
            if (!m_indexDirty) return;
        }
        // 慢路径：升级为独占锁后重建（double-check）
        std::unique_lock<std::shared_mutex> wlock(m_indexMutex);
        if (m_indexDirty) rebuildIndicesLocked();
    }

    // 获取引脚→节点索引映射（用于拓扑排序等需要 pinId→nodeIndex 的场景）
    // ⚠️ 返回内部引用，调用方须确保此期间无并发写操作（单线程调用安全）
    const std::unordered_map<PinId, size_t>& getPinToNodeIndex() const
    {
        ensureIndices();
        return m_pinToNodeIndex;
    }

    // 辅助方法：查找节点 — O(1)，线程安全（共享锁）
    const NodeInstance* findNode(NodeId nodeId) const
    {
        ensureIndices();
        std::shared_lock<std::shared_mutex> rlock(m_indexMutex);
        auto it = m_nodeIdIndex.find(nodeId);
        return (it != m_nodeIdIndex.end()) ? &nodes[it->second] : nullptr;
    }

    // 辅助方法：查找节点（可修改）— 单线程场景，不加共享锁
    NodeInstance* findNode(NodeId nodeId)
    {
        ensureIndices();
        std::shared_lock<std::shared_mutex> rlock(m_indexMutex);
        auto it = m_nodeIdIndex.find(nodeId);
        return (it != m_nodeIdIndex.end()) ? &nodes[it->second] : nullptr;
    }

    // 辅助方法：查找链接 — O(1)，线程安全（共享锁）
    const LinkInstance* findLink(LinkId linkId) const
    {
        ensureIndices();
        std::shared_lock<std::shared_mutex> rlock(m_indexMutex);
        auto it = m_linkIdIndex.find(linkId);
        return (it != m_linkIdIndex.end()) ? &links[it->second] : nullptr;
    }

    // 辅助方法：根据引脚ID查找链接 — 线程安全（共享锁保护索引读）
    std::vector<const LinkInstance*> findLinksByPin(PinId pinId) const
    {
        ensureIndices();
        std::vector<const LinkInstance*> result;
        std::shared_lock<std::shared_mutex> rlock(m_indexMutex);

        auto itStart = m_startPinLinks.find(pinId);
        if (itStart != m_startPinLinks.end())
            for (size_t idx : itStart->second)
                result.push_back(&links[idx]);

        auto itEnd = m_endPinLinks.find(pinId);
        if (itEnd != m_endPinLinks.end())
            for (size_t idx : itEnd->second)
                result.push_back(&links[idx]);

        return result;
    }

    // 辅助方法：获取从指定引脚出发的所有下游引脚ID
    std::vector<PinId> getDownstreamPinIds(PinId startPinId) const
    {
        ensureIndices();
        std::vector<PinId> result;
        std::shared_lock<std::shared_mutex> rlock(m_indexMutex);
        auto it = m_startPinLinks.find(startPinId);
        if (it != m_startPinLinks.end())
            for (size_t idx : it->second)
                result.push_back(links[idx].endPinId);
        return result;
    }

    // 辅助方法：查找连接到指定引脚的所有引脚
    std::vector<PinId> findConnectedPins(PinId pinId) const
    {
        ensureIndices();
        std::vector<PinId> result;
        std::shared_lock<std::shared_mutex> rlock(m_indexMutex);

        auto itStart = m_startPinLinks.find(pinId);
        if (itStart != m_startPinLinks.end())
            for (size_t idx : itStart->second)
                if (links[idx].endPinId != InvalidPinId)
                    result.push_back(links[idx].endPinId);

        auto itEnd = m_endPinLinks.find(pinId);
        if (itEnd != m_endPinLinks.end())
            for (size_t idx : itEnd->second)
                if (links[idx].startPinId != InvalidPinId)
                    result.push_back(links[idx].startPinId);

        return result;
    }

    // 辅助方法：查找引脚所属的节点 — O(1)，线程安全（共享锁）
    const NodeInstance* findNodeByPin(PinId pinId) const
    {
        ensureIndices();
        std::shared_lock<std::shared_mutex> rlock(m_indexMutex);
        auto it = m_pinToNodeIndex.find(pinId);
        return (it != m_pinToNodeIndex.end()) ? &nodes[it->second] : nullptr;
    }
    
    // 辅助方法：获取输入节点列表（已去重）
    std::vector<NodeId> getInputNodes(NodeId nodeId) const
    {
        std::vector<NodeId> result;
        const NodeInstance* node = findNode(nodeId);
        if (!node) return result;
        
        std::unordered_set<NodeId> seen;
        // 遍历所有输入引脚
        for (const auto& pin : node->pins)
        {
            if (pin.kind == PinKind::Input)
            {
                // 查找连接到此引脚的链接
                auto linkPins = findLinksByPin(pin.id);
                for (const auto* link : linkPins)
                {
                    const NodeInstance* sourceNode = findNodeByPin(link->startPinId);
                    if (sourceNode && seen.insert(sourceNode->id).second)
                    {
                        result.push_back(sourceNode->id);
                    }
                }
            }
        }
        return result;
    }
    
    // 辅助方法：获取数据输入节点列表（仅通过非exec引脚连接的上游节点，已去重）
    std::vector<NodeId> getDataInputNodes(NodeId nodeId) const
    {
        std::vector<NodeId> result;
        const NodeInstance* node = findNode(nodeId);
        if (!node) return result;
        
        std::unordered_set<NodeId> seen;
        for (const auto& pin : node->pins)
        {
            if (pin.kind == PinKind::Input && !pin.isExec)
            {
                auto linkPins = findLinksByPin(pin.id);
                for (const auto* link : linkPins)
                {
                    const NodeInstance* sourceNode = findNodeByPin(link->startPinId);
                    if (sourceNode && seen.insert(sourceNode->id).second)
                    {
                        result.push_back(sourceNode->id);
                    }
                }
            }
        }
        return result;
    }
    
    // 辅助方法：获取通过 exec 输出引脚连接的下游节点列表（已去重）
    std::vector<NodeId> getExecOutputNodes(NodeId nodeId) const
    {
        std::vector<NodeId> result;
        const NodeInstance* node = findNode(nodeId);
        if (!node) return result;
        
        std::unordered_set<NodeId> seen;
        for (const auto& pin : node->pins)
        {
            if (pin.kind == PinKind::Output && pin.isExec)
            {
                auto findLinks = findLinksByPin(pin.id);
                for (const auto* link : findLinks)
                {
                    const NodeInstance* targetNode = findNodeByPin(link->endPinId);
                    if (targetNode && seen.insert(targetNode->id).second)
                        result.push_back(targetNode->id);
                }
            }
        }
        return result;
    }
    
    // 辅助方法：计算节点的 exec 输出引脚总数（不论是否有连接）
    // 用于判断该节点是否为控制流节点（>=2 说明有分支语义，如 Branch 的 True/False）
    int countExecOutputPins(NodeId nodeId) const
    {
        int count = 0;
        const NodeInstance* node = findNode(nodeId);
        if (!node) return 0;
        
        for (const auto& pin : node->pins)
        {
            if (pin.kind == PinKind::Output && pin.isExec)
                ++count;
        }
        return count;
    }
    
    // 辅助方法：判断节点是否为"事件源"节点
    // 事件源节点没有 exec 输入引脚，但有 exec 输出引脚（如 CustomEvent、InputActionFire）
    // 这类节点不应在主循环中执行，只在被外部触发（如 Timer、输入事件）时执行
    bool isEventSourceNode(NodeId nodeId) const
    {
        const NodeInstance* node = findNode(nodeId);
        if (!node) return false;
        
        bool hasExecInput = false;
        bool hasExecOutput = false;
        for (const auto& pin : node->pins)
        {
            if (pin.kind == PinKind::Input && pin.isExec)
                hasExecInput = true;
            if (pin.kind == PinKind::Output && pin.isExec)
                hasExecOutput = true;
        }
        return !hasExecInput && hasExecOutput;
    }
    
    // 辅助方法：收集所有事件源节点及其 exec 下游子图的节点集合
    // 这些节点在主执行循环中应被跳过
    std::unordered_set<NodeId> collectEventSubgraphs() const
    {
        std::unordered_set<NodeId> result;
        for (const auto& node : nodes)
        {
            if (!isEventSourceNode(node.id)) continue;
            // BFS 沿 exec 链收集事件子图
            std::queue<NodeId> q;
            q.push(node.id);
            result.insert(node.id);
            while (!q.empty())
            {
                NodeId cur = q.front();
                q.pop();
                auto downstream = getExecOutputNodes(cur);
                for (auto downId : downstream)
                {
                    if (result.insert(downId).second)
                        q.push(downId);
                }
            }
        }
        return result;
    }
    
    // 辅助方法：获取输出节点列表（已去重）
    std::vector<NodeId> getOutputNodes(NodeId nodeId) const
    {
        std::vector<NodeId> result;
        const NodeInstance* node = findNode(nodeId);
        if (!node) return result;
        
        std::unordered_set<NodeId> seen;
        // 遍历所有输出引脚
        for (const auto& pin : node->pins)
        {
            if (pin.kind == PinKind::Output)
            {
                // 查找连接到此引脚的链接
                auto findLinks = findLinksByPin(pin.id);
                for (const auto* link : findLinks)
                {
                    const NodeInstance* targetNode = findNodeByPin(link->endPinId);
                    if (targetNode && seen.insert(targetNode->id).second)
                    {
                        result.push_back(targetNode->id);
                    }
                }
            }
        }
        return result;
    }
    
    // 清空所有数据
    void clear()
    {
        metadata = BlueprintMetadata();
        nodes.clear();
        links.clear();
        variables.clear();
        comments.clear();
        viewInfo = ViewInfo();
        invalidateIndices();
    }

private:
    // 哈希索引（mutable：缓存，不影响逻辑 const 性；由 m_indexMutex 保护）
    mutable std::shared_mutex                           m_indexMutex;        // 读写锁：多读单写
    mutable std::unordered_map<NodeId, size_t>          m_nodeIdIndex;       // nodeId → nodes[] 下标
    mutable std::unordered_map<PinId, size_t>           m_pinToNodeIndex;    // pinId → nodes[] 下标
    mutable std::unordered_map<LinkId, size_t>          m_linkIdIndex;       // linkId → links[] 下标
    mutable std::unordered_map<PinId, std::vector<size_t>> m_startPinLinks;  // startPinId → links[] 下标列表
    mutable std::unordered_map<PinId, std::vector<size_t>> m_endPinLinks;    // endPinId → links[] 下标列表
    mutable bool                                        m_indexDirty = true;

    // 内部：在已持有独占锁的情况下重建索引（供 rebuildIndices/ensureIndices 调用）
    void rebuildIndicesLocked() const
    {
        m_nodeIdIndex.clear();
        m_pinToNodeIndex.clear();
        m_linkIdIndex.clear();
        m_startPinLinks.clear();
        m_endPinLinks.clear();

        for (size_t i = 0; i < nodes.size(); ++i)
        {
            m_nodeIdIndex[nodes[i].id] = i;
            for (const auto& pin : nodes[i].pins)
                m_pinToNodeIndex[pin.id] = i;
        }

        for (size_t i = 0; i < links.size(); ++i)
        {
            m_linkIdIndex[links[i].id] = i;
            if (links[i].isEnabled)
            {
                m_startPinLinks[links[i].startPinId].push_back(i);
                m_endPinLinks[links[i].endPinId].push_back(i);
            }
        }

        m_indexDirty = false;
    }
};

} // namespace Runtime
} // namespace NodeEditor
