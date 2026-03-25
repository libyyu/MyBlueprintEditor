// ClipboardOps.cpp -- 节点剪贴板操作（复制/粘贴/剪切/复制）
#include "BlueprintEditor.h"
#include <algorithm>
#include <unordered_set>
#include <unordered_map>

// ============================================================================
// Copy - 复制选中节点到剪贴板
// ============================================================================

void BlueprintEditor::CopySelectedNodes()
{
    if (!ActiveDoc()) return;

    // 获取选中的节点
    int selCount = ed::GetSelectedObjectCount();
    if (selCount == 0) return;

    std::vector<ed::NodeId> selectedNodeIds;
    selectedNodeIds.resize(selCount);
    int nodeCount = ed::GetSelectedNodes(selectedNodeIds.data(), selCount);
    selectedNodeIds.resize(nodeCount);

    if (nodeCount == 0) return;

    // 清空剪贴板
    m_ClipboardNodes.clear();
    m_ClipboardLinks.clear();
    m_ClipboardCenter = ImVec2(0, 0);

    // 收集选中节点的 ID 集合（用于快速查找）
    std::unordered_set<uintptr_t> selectedSet;
    for (const auto& nid : selectedNodeIds)
        selectedSet.insert(reinterpret_cast<uintptr_t>(nid.AsPointer()));

    // 映射：节点指针 ID -> 剪贴板节点索引
    std::unordered_map<uintptr_t, int> nodeToClipIndex;
    // 映射：引脚指针 ID -> {clipNodeIdx, pinLocalIdx, isOutput}
    struct PinMapping { int nodeIdx; int pinIdx; bool isOutput; };
    std::unordered_map<uintptr_t, PinMapping> pinToClipMapping;

    // 复制节点数据到剪贴板
    for (const auto& node : m_Nodes)
    {
        auto nodeKey = reinterpret_cast<uintptr_t>(node.ID.AsPointer());
        if (selectedSet.find(nodeKey) == selectedSet.end())
            continue;

        int clipIdx = static_cast<int>(m_ClipboardNodes.size());
        nodeToClipIndex[nodeKey] = clipIdx;

        ClipboardNode cn;
        cn.definitionId = node.DefinitionId;
        cn.name = node.Name;
        cn.position = ed::GetNodePosition(node.ID);
        cn.color = node.Color;
        cn.type = node.Type;
        cn.size = node.Size;
        cn.hasDynamicInputs = node.HasDynamicInputs;
        cn.dynamicInputPinType = node.DynamicInputPinType;
        cn.dynamicInputFixedCount = node.DynamicInputFixedCount;

        // 复制输入引脚
        for (int i = 0; i < static_cast<int>(node.Inputs.size()); ++i)
        {
            const auto& pin = node.Inputs[i];
            ClipboardNode::ClipboardPin cp;
            cp.name = pin.Name;
            cp.type = pin.Type;
            cp.kind = PinKind::Input;
            cp.numericValue = pin.BoolValue;
            cp.numericValue = static_cast<int64_t>(pin.IntValue);
            cp.numericValue = static_cast<double>(pin.FloatValue);
            cp.stringValue = pin.StringValue;
            cp.objectValue = pin.ObjectValue;
            cp.hiddenWhen = pin.HiddenWhen;
            cp.originalLocalIndex = i;
            cn.inputs.push_back(cp);

            auto pinKey = reinterpret_cast<uintptr_t>(pin.ID.AsPointer());
            pinToClipMapping[pinKey] = { clipIdx, i, false };
        }

        // 复制输出引脚
        for (int i = 0; i < static_cast<int>(node.Outputs.size()); ++i)
        {
            const auto& pin = node.Outputs[i];
            ClipboardNode::ClipboardPin cp;
            cp.name = pin.Name;
            cp.type = pin.Type;
            cp.kind = PinKind::Output;
            cp.numericValue = pin.BoolValue;
            cp.numericValue = static_cast<int64_t>(pin.IntValue);
            cp.numericValue = static_cast<double>(pin.FloatValue);
            cp.stringValue = pin.StringValue;
            cp.objectValue = pin.ObjectValue;
            cp.hiddenWhen = pin.HiddenWhen;
            cp.originalLocalIndex = i;
            cn.outputs.push_back(cp);

            auto pinKey = reinterpret_cast<uintptr_t>(pin.ID.AsPointer());
            pinToClipMapping[pinKey] = { clipIdx, i, true };
        }

        m_ClipboardNodes.push_back(std::move(cn));
    }

    // 复制选中节点之间的链接
    for (const auto& link : m_Links)
    {
        auto startKey = reinterpret_cast<uintptr_t>(link.StartPinID.AsPointer());
        auto endKey = reinterpret_cast<uintptr_t>(link.EndPinID.AsPointer());

        auto startIt = pinToClipMapping.find(startKey);
        auto endIt = pinToClipMapping.find(endKey);

        if (startIt != pinToClipMapping.end() && endIt != pinToClipMapping.end())
        {
            ClipboardLink cl;
            cl.srcNodeIdx = startIt->second.nodeIdx;
            cl.srcPinIdx = startIt->second.pinIdx;
            cl.dstNodeIdx = endIt->second.nodeIdx;
            cl.dstPinIdx = endIt->second.pinIdx;
            m_ClipboardLinks.push_back(cl);
        }
    }

    // 计算质心
    if (!m_ClipboardNodes.empty())
    {
        ImVec2 sum(0, 0);
        for (const auto& cn : m_ClipboardNodes)
        {
            sum.x += cn.position.x;
            sum.y += cn.position.y;
        }
        m_ClipboardCenter.x = sum.x / m_ClipboardNodes.size();
        m_ClipboardCenter.y = sum.y / m_ClipboardNodes.size();
    }

    // 日志提示
    if (ActiveDoc())
    {
        m_ExecutionLog.push_back("[INFO] Copied " + std::to_string(m_ClipboardNodes.size()) +
                                 " node(s) and " + std::to_string(m_ClipboardLinks.size()) + " link(s)");
        m_ExecutionLogDirty = true;
    }
}

// ============================================================================
// Paste - 粘贴剪贴板节点到画布
// ============================================================================

void BlueprintEditor::PasteNodes(ImVec2 pastePosition)
{
    if (!ActiveDoc()) return;
    if (m_ClipboardNodes.empty()) return;

    // 偏移量：从剪贴板质心到粘贴位置
    ImVec2 offset;
    offset.x = pastePosition.x - m_ClipboardCenter.x;
    offset.y = pastePosition.y - m_ClipboardCenter.y;

    // 映射：剪贴板节点索引 -> 新创建的节点引用
    struct NewNodeInfo
    {
        Node* node;
        std::vector<ed::PinId> inputPinIds;
        std::vector<ed::PinId> outputPinIds;
    };
    std::vector<NewNodeInfo> newNodeInfos;
    newNodeInfos.reserve(m_ClipboardNodes.size());

    // 清除当前选中
    ed::ClearSelection();

    for (const auto& cn : m_ClipboardNodes)
    {
        // 尝试通过定义创建（优先，可获取完整信息）
        auto* def = m_NodeRegistry.getNodeDefinition(cn.definitionId);

        int newNodeId = GetNextId();
        m_Nodes.emplace_back(newNodeId, cn.name.c_str(), cn.color);
        auto& node = m_Nodes.back();
        node.Type = cn.type;
        node.DefinitionId = cn.definitionId;
        node.Size = cn.size;
        node.HasDynamicInputs = cn.hasDynamicInputs;
        node.DynamicInputPinType = cn.dynamicInputPinType;
        node.DynamicInputFixedCount = cn.dynamicInputFixedCount;

        NewNodeInfo info;
        info.node = &node;

        // 创建输入引脚（从剪贴板数据恢复，保留用户编辑的值）
        for (const auto& cp : cn.inputs)
        {
            int newPinId = GetNextId();
            node.Inputs.emplace_back(newPinId, cp.name.c_str(), cp.type);
            auto& pin = node.Inputs.back();
            pin.BoolValue  = cp.type == PinDataType::Boolean ? std::get<bool>(cp.numericValue) : false;
            pin.IntValue   = cp.type == PinDataType::Integer ? static_cast<int>(std::get<int64_t>(cp.numericValue)) : 0;
            pin.FloatValue = cp.type == PinDataType::Float   ? static_cast<float>(std::get<double>(cp.numericValue)) : 0.0f;
            pin.StringValue = cp.stringValue;
            pin.ObjectValue = cp.objectValue;
            pin.HiddenWhen = cp.hiddenWhen;
            info.inputPinIds.push_back(ed::PinId(newPinId));
        }

        // 创建输出引脚
        for (const auto& cp : cn.outputs)
        {
            int newPinId = GetNextId();
            node.Outputs.emplace_back(newPinId, cp.name.c_str(), cp.type);
            auto& pin = node.Outputs.back();
            pin.HiddenWhen = cp.hiddenWhen;
            info.outputPinIds.push_back(ed::PinId(newPinId));
        }

        BuildNode(&node);

        // 应用特殊引脚类型
        if (def)
            FixupSpecialPinTypes(&node, def);

        // 设置节点位置（偏移后）
        ImVec2 newPos;
        newPos.x = cn.position.x + offset.x;
        newPos.y = cn.position.y + offset.y;
        ed::SetNodePosition(node.ID, newPos);

        if (cn.type == NodeType::Comment && cn.size.x > 0)
            ed::SetGroupSize(node.ID, cn.size);

        // 选中新创建的节点
        ed::SelectNode(node.ID, true);

        newNodeInfos.push_back(std::move(info));
    }

    // 重建剪贴板内部的链接
    for (const auto& cl : m_ClipboardLinks)
    {
        if (cl.srcNodeIdx < 0 || cl.srcNodeIdx >= static_cast<int>(newNodeInfos.size()))
            continue;
        if (cl.dstNodeIdx < 0 || cl.dstNodeIdx >= static_cast<int>(newNodeInfos.size()))
            continue;

        const auto& srcInfo = newNodeInfos[cl.srcNodeIdx];
        const auto& dstInfo = newNodeInfos[cl.dstNodeIdx];

        if (cl.srcPinIdx < 0 || cl.srcPinIdx >= static_cast<int>(srcInfo.outputPinIds.size()))
            continue;
        if (cl.dstPinIdx < 0 || cl.dstPinIdx >= static_cast<int>(dstInfo.inputPinIds.size()))
            continue;

        ed::PinId startPinId = srcInfo.outputPinIds[cl.srcPinIdx];
        ed::PinId endPinId = dstInfo.inputPinIds[cl.dstPinIdx];

        m_Links.emplace_back(Link(GetNextId(), startPinId, endPinId));

        // 设置链接颜色（Any 引脚使用对端类型颜色）
        auto* startPin = FindPin(startPinId);
        auto* endPin   = FindPin(endPinId);
        if (startPin)
            m_Links.back().Color = GetIconColor(GetLinkColor(startPin, endPin));
    }

    m_IsDirty = true;

    // 日志提示
    if (ActiveDoc())
    {
        m_ExecutionLog.push_back("[INFO] Pasted " + std::to_string(m_ClipboardNodes.size()) +
                                 " node(s) and " + std::to_string(m_ClipboardLinks.size()) + " link(s)");
        m_ExecutionLogDirty = true;
    }
}

// ============================================================================
// Duplicate - 就地复制选中节点（Ctrl+D）
// ============================================================================

void BlueprintEditor::DuplicateSelectedNodes()
{
    if (!ActiveDoc()) return;

    // 先复制到剪贴板
    CopySelectedNodes();

    if (m_ClipboardNodes.empty()) return;

    // 以质心偏移 (50, 50) 作为粘贴位置
    ImVec2 pastePos;
    pastePos.x = m_ClipboardCenter.x + 50.0f;
    pastePos.y = m_ClipboardCenter.y + 50.0f;

    PasteNodes(pastePos);
}

// ============================================================================
// Cut - 剪切选中节点（Ctrl+X）
// ============================================================================

void BlueprintEditor::CutSelectedNodes()
{
    if (!ActiveDoc()) return;

    // 先复制
    CopySelectedNodes();

    if (m_ClipboardNodes.empty()) return;

    // 然后删除选中的节点
    int selCount = ed::GetSelectedObjectCount();
    std::vector<ed::NodeId> selectedNodeIds;
    selectedNodeIds.resize(selCount);
    int nodeCount = ed::GetSelectedNodes(selectedNodeIds.data(), selCount);
    selectedNodeIds.resize(nodeCount);

    for (const auto& nodeId : selectedNodeIds)
        ed::DeleteNode(nodeId);

    // 日志提示
    if (ActiveDoc())
    {
        m_ExecutionLog.push_back("[INFO] Cut " + std::to_string(m_ClipboardNodes.size()) + " node(s)");
        m_ExecutionLogDirty = true;
    }
}

// ============================================================================
// Align Selected Nodes - 对齐选中的节点
// ============================================================================

void BlueprintEditor::AlignSelectedNodes(AlignMode mode)
{
    if (!ActiveDoc()) return;

    int selCount = ed::GetSelectedObjectCount();
    if (selCount < 2) return;

    std::vector<ed::NodeId> selectedNodeIds;
    selectedNodeIds.resize(selCount);
    int nodeCount = ed::GetSelectedNodes(selectedNodeIds.data(), selCount);
    selectedNodeIds.resize(nodeCount);

    if (nodeCount < 2) return;

    // 收集选中节点的位置和尺寸
    struct NodeRect { ed::NodeId id; ImVec2 pos; ImVec2 size; };
    std::vector<NodeRect> rects;
    for (const auto& nid : selectedNodeIds)
    {
        NodeRect r;
        r.id = nid;
        r.pos = ed::GetNodePosition(nid);
        r.size = ed::GetNodeSize(nid);
        rects.push_back(r);
    }

    switch (mode)
    {
    case AlignMode::Left:
    {
        float minX = rects[0].pos.x;
        for (const auto& r : rects) if (r.pos.x < minX) minX = r.pos.x;
        for (auto& r : rects) ed::SetNodePosition(r.id, ImVec2(minX, r.pos.y));
        break;
    }
    case AlignMode::Right:
    {
        float maxRight = rects[0].pos.x + rects[0].size.x;
        for (const auto& r : rects)
        {
            float right = r.pos.x + r.size.x;
            if (right > maxRight) maxRight = right;
        }
        for (auto& r : rects) ed::SetNodePosition(r.id, ImVec2(maxRight - r.size.x, r.pos.y));
        break;
    }
    case AlignMode::Top:
    {
        float minY = rects[0].pos.y;
        for (const auto& r : rects) if (r.pos.y < minY) minY = r.pos.y;
        for (auto& r : rects) ed::SetNodePosition(r.id, ImVec2(r.pos.x, minY));
        break;
    }
    case AlignMode::Bottom:
    {
        float maxBottom = rects[0].pos.y + rects[0].size.y;
        for (const auto& r : rects)
        {
            float bottom = r.pos.y + r.size.y;
            if (bottom > maxBottom) maxBottom = bottom;
        }
        for (auto& r : rects) ed::SetNodePosition(r.id, ImVec2(r.pos.x, maxBottom - r.size.y));
        break;
    }
    case AlignMode::CenterH:
    {
        float sumCenterX = 0;
        for (const auto& r : rects) sumCenterX += r.pos.x + r.size.x * 0.5f;
        float avgCenterX = sumCenterX / rects.size();
        for (auto& r : rects) ed::SetNodePosition(r.id, ImVec2(avgCenterX - r.size.x * 0.5f, r.pos.y));
        break;
    }
    case AlignMode::CenterV:
    {
        float sumCenterY = 0;
        for (const auto& r : rects) sumCenterY += r.pos.y + r.size.y * 0.5f;
        float avgCenterY = sumCenterY / rects.size();
        for (auto& r : rects) ed::SetNodePosition(r.id, ImVec2(r.pos.x, avgCenterY - r.size.y * 0.5f));
        break;
    }
    }

    m_IsDirty = true;
}
