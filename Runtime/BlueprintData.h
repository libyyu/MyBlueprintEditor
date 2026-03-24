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

namespace NodeEditor {
namespace Runtime {

// ============================================================================
// 节点实例数据
// ============================================================================

struct NodeInstance
{
    NodeId              id = InvalidNodeId;
    std::string         definitionId;           // 节点类型ID（对应 NodeDefinition::id）
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
// 蓝图元数据
// ============================================================================

struct BlueprintMetadata
{
    std::string                 name;           // 蓝图名称
    std::string                 description;    // 蓝图描述
    std::string                 author;         // 作者
    std::string                 version;        // 版本号
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
    // ============================================================================

    // 重建所有索引（在数据加载、节点/链接增删后调用）
    void rebuildIndices() const
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
            {
                m_pinToNodeIndex[pin.id] = i;
            }
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

    // 标记索引需要重建
    void invalidateIndices() const { m_indexDirty = true; }

    // 确保索引可用
    void ensureIndices() const
    {
        if (m_indexDirty) rebuildIndices();
    }

    // 获取引脚→节点索引映射（用于拓扑排序等需要 pinId→nodeIndex 的场景）
    const std::unordered_map<PinId, size_t>& getPinToNodeIndex() const
    {
        ensureIndices();
        return m_pinToNodeIndex;
    }
    
    // 辅助方法：查找节点 — O(1) 哈希查找
    const NodeInstance* findNode(NodeId nodeId) const
    {
        ensureIndices();
        auto it = m_nodeIdIndex.find(nodeId);
        return (it != m_nodeIdIndex.end()) ? &nodes[it->second] : nullptr;
    }
    
    // 辅助方法：查找节点（可修改）
    NodeInstance* findNode(NodeId nodeId)
    {
        ensureIndices();
        auto it = m_nodeIdIndex.find(nodeId);
        return (it != m_nodeIdIndex.end()) ? &nodes[it->second] : nullptr;
    }
    
    // 辅助方法：查找链接 — O(1) 哈希查找
    const LinkInstance* findLink(LinkId linkId) const
    {
        ensureIndices();
        auto it = m_linkIdIndex.find(linkId);
        return (it != m_linkIdIndex.end()) ? &links[it->second] : nullptr;
    }
    
    // 辅助方法：根据引脚ID查找链接 — 使用索引加速
    std::vector<const LinkInstance*> findLinksByPin(PinId pinId) const
    {
        ensureIndices();
        std::vector<const LinkInstance*> result;
        
        // 查找以 pinId 为起始引脚的链接
        auto itStart = m_startPinLinks.find(pinId);
        if (itStart != m_startPinLinks.end())
        {
            for (size_t idx : itStart->second)
                result.push_back(&links[idx]);
        }
        
        // 查找以 pinId 为终止引脚的链接
        auto itEnd = m_endPinLinks.find(pinId);
        if (itEnd != m_endPinLinks.end())
        {
            for (size_t idx : itEnd->second)
                result.push_back(&links[idx]);
        }
        
        return result;
    }
    
    // 辅助方法：获取从指定引脚出发的所有下游引脚ID（仅通过 startPinId 查找）
    std::vector<PinId> getDownstreamPinIds(PinId startPinId) const
    {
        ensureIndices();
        std::vector<PinId> result;
        auto it = m_startPinLinks.find(startPinId);
        if (it != m_startPinLinks.end())
        {
            for (size_t idx : it->second)
                result.push_back(links[idx].endPinId);
        }
        return result;
    }
    
    // 辅助方法：查找连接到指定引脚的所有引脚 — 使用索引加速
    std::vector<PinId> findConnectedPins(PinId pinId) const
    {
        ensureIndices();
        std::vector<PinId> result;
        
        // 以 pinId 为起始引脚的链接 → 取 endPinId
        auto itStart = m_startPinLinks.find(pinId);
        if (itStart != m_startPinLinks.end())
        {
            for (size_t idx : itStart->second)
                if (links[idx].endPinId != InvalidPinId)
                    result.push_back(links[idx].endPinId);
        }
        
        // 以 pinId 为终止引脚的链接 → 取 startPinId
        auto itEnd = m_endPinLinks.find(pinId);
        if (itEnd != m_endPinLinks.end())
        {
            for (size_t idx : itEnd->second)
                if (links[idx].startPinId != InvalidPinId)
                    result.push_back(links[idx].startPinId);
        }
        
        return result;
    }
    
    // 辅助方法：查找引脚所属的节点 — O(1) 哈希查找
    const NodeInstance* findNodeByPin(PinId pinId) const
    {
        ensureIndices();
        auto it = m_pinToNodeIndex.find(pinId);
        return (it != m_pinToNodeIndex.end()) ? &nodes[it->second] : nullptr;
    }
    
    // 辅助方法：获取输入节点列表
    std::vector<NodeId> getInputNodes(NodeId nodeId) const
    {
        std::vector<NodeId> result;
        const NodeInstance* node = findNode(nodeId);
        if (!node) return result;
        
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
                    if (sourceNode)
                    {
                        result.push_back(sourceNode->id);
                    }
                }
            }
        }
        return result;
    }
    
    // 辅助方法：获取数据输入节点列表（仅通过非exec引脚连接的上游节点）
    std::vector<NodeId> getDataInputNodes(NodeId nodeId) const
    {
        std::vector<NodeId> result;
        const NodeInstance* node = findNode(nodeId);
        if (!node) return result;
        
        for (const auto& pin : node->pins)
        {
            if (pin.kind == PinKind::Input && !pin.isExec)
            {
                auto linkPins = findLinksByPin(pin.id);
                for (const auto* link : linkPins)
                {
                    const NodeInstance* sourceNode = findNodeByPin(link->startPinId);
                    if (sourceNode)
                    {
                        result.push_back(sourceNode->id);
                    }
                }
            }
        }
        return result;
    }
    
    // 辅助方法：获取通过 exec 输出引脚连接的下游节点列表
    std::vector<NodeId> getExecOutputNodes(NodeId nodeId) const
    {
        std::vector<NodeId> result;
        const NodeInstance* node = findNode(nodeId);
        if (!node) return result;
        
        for (const auto& pin : node->pins)
        {
            if (pin.kind == PinKind::Output && pin.isExec)
            {
                auto findLinks = findLinksByPin(pin.id);
                for (const auto* link : findLinks)
                {
                    const NodeInstance* targetNode = findNodeByPin(link->endPinId);
                    if (targetNode)
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
    
    // 辅助方法：获取输出节点列表
    std::vector<NodeId> getOutputNodes(NodeId nodeId) const
    {
        std::vector<NodeId> result;
        const NodeInstance* node = findNode(nodeId);
        if (!node) return result;
        
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
                    if (targetNode)
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
    // 哈希索引（mutable 因为它们是缓存，不影响逻辑 const 性）
    mutable std::unordered_map<NodeId, size_t>  m_nodeIdIndex;       // nodeId → nodes[] 下标
    mutable std::unordered_map<PinId, size_t>   m_pinToNodeIndex;    // pinId → nodes[] 下标
    mutable std::unordered_map<LinkId, size_t>  m_linkIdIndex;       // linkId → links[] 下标
    mutable std::unordered_map<PinId, std::vector<size_t>> m_startPinLinks;  // startPinId → links[] 下标列表
    mutable std::unordered_map<PinId, std::vector<size_t>> m_endPinLinks;    // endPinId → links[] 下标列表
    mutable bool                                m_indexDirty = true;
};

} // namespace Runtime
} // namespace NodeEditor
