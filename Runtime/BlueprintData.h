// Runtime/BlueprintData.h - 蓝图数据结构
// 该文件定义了完整的蓝图数据结构，用于存储和传输蓝图信息

#pragma once

#include "Types.h"
#include "NodeDefinition.h"
#include <vector>
#include <unordered_map>
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
    
    // 辅助方法：查找节点
    const NodeInstance* findNode(NodeId nodeId) const
    {
        for (const auto& node : nodes)
        {
            if (node.id == nodeId) return &node;
        }
        return nullptr;
    }
    
    // 辅助方法：查找节点（可修改）
    NodeInstance* findNode(NodeId nodeId)
    {
        for (auto& node : nodes)
        {
            if (node.id == nodeId) return &node;
        }
        return nullptr;
    }
    
    // 辅助方法：查找链接
    const LinkInstance* findLink(LinkId linkId) const
    {
        for (const auto& link : links)
        {
            if (link.id == linkId) return &link;
        }
        return nullptr;
    }
    
    // 辅助方法：根据引脚ID查找链接
    std::vector<const LinkInstance*> findLinksByPin(PinId pinId) const
    {
        std::vector<const LinkInstance*> result;
        for (const auto& link : links)
        {
            if (link.startPinId == pinId || link.endPinId == pinId)
            {
                result.push_back(&link);
            }
        }
        return result;
    }
    
    // 辅助方法：查找连接到指定引脚的所有引脚
    std::vector<PinId> findConnectedPins(PinId pinId) const
    {
        std::vector<PinId> result;
        for (const auto& link : links)
        {
            if (link.startPinId == pinId && link.endPinId != InvalidPinId)
            {
                result.push_back(link.endPinId);
            }
            else if (link.endPinId == pinId && link.startPinId != InvalidPinId)
            {
                result.push_back(link.startPinId);
            }
        }
        return result;
    }
    
    // 辅助方法：查找引脚所属的节点
    const NodeInstance* findNodeByPin(PinId pinId) const
    {
        for (const auto& node : nodes)
        {
            for (const auto& pin : node.pins)
            {
                if (pin.id == pinId) return &node;
            }
        }
        return nullptr;
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
    }
};

} // namespace Runtime
} // namespace NodeEditor
