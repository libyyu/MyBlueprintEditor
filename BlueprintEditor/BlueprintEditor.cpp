// BlueprintEditor.cpp -- 蓝图编辑器核心逻辑
#include "BlueprintEditor.h"
#include "ThemeManager.h"
#include <map>
#include <functional>
#include <algorithm>
#include "../Runtime/ScriptNodeLoader.h"

// ============================================================================
// ID 管理
// ============================================================================

int BlueprintEditor::GetNextId()
{
    return ActiveDoc()->nextId++;
}

ed::LinkId BlueprintEditor::GetNextLinkId()
{
    return ed::LinkId(GetNextId());
}

// ============================================================================
// 触摸追踪
// ============================================================================

void BlueprintEditor::TouchNode(ed::NodeId id)
{
    ActiveDoc()->nodeTouchTime[id] = m_TouchTime;
}

float BlueprintEditor::GetTouchProgress(ed::NodeId id)
{
    auto it = ActiveDoc()->nodeTouchTime.find(id);
    if (it != ActiveDoc()->nodeTouchTime.end() && it->second > 0.0f)
        return (m_TouchTime - it->second) / m_TouchTime;

    return 0.0f;
}

void BlueprintEditor::UpdateTouch()
{
    const auto deltaTime = ImGui::GetIO().DeltaTime;
    for (auto& entry : ActiveDoc()->nodeTouchTime)
    {
        if (entry.second > 0.0f)
            entry.second -= deltaTime;
    }
}

// ============================================================================
// 查找（O(1) 哈希索引加速）
// ============================================================================

Node* BlueprintEditor::FindNode(ed::NodeId id)
{
    auto* doc = ActiveDoc();
    if (!doc) return nullptr;
    doc->ensureEditorIndices();
    uint64_t nid = reinterpret_cast<uintptr_t>(id.AsPointer());
    auto it = doc->nodeIdIndex.find(nid);
    if (it != doc->nodeIdIndex.end() && it->second < doc->nodes.size())
        return &doc->nodes[it->second];
    return nullptr;
}

Link* BlueprintEditor::FindLink(ed::LinkId id)
{
    auto* doc = ActiveDoc();
    if (!doc) return nullptr;
    doc->ensureEditorIndices();
    uint64_t lid = reinterpret_cast<uintptr_t>(id.AsPointer());
    auto it = doc->linkIdIndex.find(lid);
    if (it != doc->linkIdIndex.end() && it->second < doc->links.size())
        return &doc->links[it->second];
    return nullptr;
}

Pin* BlueprintEditor::FindPin(ed::PinId id)
{
    if (!id)
        return nullptr;
    auto* doc = ActiveDoc();
    if (!doc) return nullptr;
    doc->ensureEditorIndices();
    uint64_t pid = reinterpret_cast<uintptr_t>(id.AsPointer());
    auto it = doc->pinIdIndex.find(pid);
    // pinIdIndex stores const Pin* to avoid const_cast in the (const) rebuild method.
    // FindPin is called from non-const contexts only, so casting back is safe.
    return (it != doc->pinIdIndex.end()) ? const_cast<Pin*>(it->second) : nullptr;
}

bool BlueprintEditor::IsPinLinked(ed::PinId id)
{
    if (!id)
        return false;
    auto* doc = ActiveDoc();
    if (!doc) return false;
    doc->ensureEditorIndices();
    uint64_t pid = reinterpret_cast<uintptr_t>(id.AsPointer());
    return doc->pinLinkedCache.count(pid) > 0;
}

bool BlueprintEditor::CanUndo()
{
    auto* doc = ActiveDoc();
    return doc && !doc->undoStack.empty();
}

bool BlueprintEditor::CanRedo()
{
    auto* doc = ActiveDoc();
    return doc && !doc->redoStack.empty();
}

void BlueprintEditor::PushUndoState()
{
    auto* doc = ActiveDoc();
    if (!doc) return;

    UndoState state;
    state.nodes     = doc->nodes;
    state.links     = doc->links;
    state.variables = doc->variables;
    state.nextId    = doc->nextId;

    // 捕获所有节点的当前位置（必须在 ed::Begin/End 之间调用才有效；
    // 如果在 Begin/End 外调用，ed::GetNodePosition 返回 (0,0)，此时位置已记录在
    // lastNodePositions 里，用它作为 fallback）
    for (const auto& node : doc->nodes)
    {
        ImVec2 pos = ed::GetNodePosition(node.ID);
        if (pos.x == 0.0f && pos.y == 0.0f)
        {
            // fallback: 用上一帧追踪到的位置
            auto it = doc->lastNodePositions.find(node.ID);
            if (it != doc->lastNodePositions.end())
                pos = it->second;
        }
        state.nodePositions[node.ID] = pos;
    }

    doc->undoStack.push_back(std::move(state));
    if ((int)doc->undoStack.size() > kMaxUndoSteps)
        doc->undoStack.pop_front();

    // 新操作清空 redo 栈
    doc->redoStack.clear();
}

// 从 from 恢复到 doc，并把当前状态推入 to
static void ApplyUndoRedo(BlueprintDocument* doc,
                          std::deque<UndoState>& from,
                          std::deque<UndoState>& to)
{
    if (!doc || from.empty()) return;

    // 保存当前状态到目标栈（undo 时推入 redo，redo 时推入 undo）
    UndoState cur;
    cur.nodes     = doc->nodes;
    cur.links     = doc->links;
    cur.variables = doc->variables;
    cur.nextId    = doc->nextId;
    for (const auto& node : doc->nodes)
    {
        ImVec2 pos = ed::GetNodePosition(node.ID);
        if (pos.x == 0.0f && pos.y == 0.0f)
        {
            auto it = doc->lastNodePositions.find(node.ID);
            if (it != doc->lastNodePositions.end())
                pos = it->second;
        }
        cur.nodePositions[node.ID] = pos;
    }
    to.push_back(std::move(cur));
    if ((int)to.size() > kMaxUndoSteps)
        to.pop_front();

    // 恢复目标快照
    UndoState& target = from.back();
    doc->nodes        = target.nodes;
    doc->links        = target.links;
    doc->variables    = target.variables;
    doc->nextId       = target.nextId;

    // 节点位置延迟恢复（需要在 ed::Begin/End 内调用 SetNodePosition）
    doc->pendingRestorePositions = true;
    doc->pendingRestoreNodePos   = target.nodePositions;

    from.pop_back();

    // 关键：快照恢复后重建 Pin::Node 指针（deque 重新分配，旧指针全部失效）
    for (auto& node : doc->nodes)
    {
        for (auto& pin : node.Inputs)  { pin.Node = &node; pin.Kind = PinKind::Input; }
        for (auto& pin : node.Outputs) { pin.Node = &node; pin.Kind = PinKind::Output; }
    }

    doc->isDirty = true;
    doc->invalidateEditorIndices();
}

void BlueprintEditor::Undo()
{
    auto* doc = ActiveDoc();
    if (!doc || doc->undoStack.empty()) return;
    ApplyUndoRedo(doc, doc->undoStack, doc->redoStack);
}

void BlueprintEditor::Redo()
{
    auto* doc = ActiveDoc();
    if (!doc || doc->redoStack.empty()) return;
    ApplyUndoRedo(doc, doc->redoStack, doc->undoStack);
}

bool BlueprintEditor::CanCreateLink(Pin* a, Pin* b)
{
    if (!a || !b || a == b) return false;
    if (!a->Node || !b->Node) return false;  // 防止悬空 Node 指针
    if (a->Kind == b->Kind || a->Node == b->Node)
        return false;

    // Any 类型可以与任何非 Flow 的数据类型连接
    if (a->Type == PinType::Any && b->Type != PinType::Flow)
        return true;
    if (b->Type == PinType::Any && a->Type != PinType::Flow)
        return true;

    // 完全匹配
    if (a->Type == b->Type)
        return true;

    // 兼容类型之间允许隐式连接
    // Int ↔ Float (自动转换)
    if ((a->Type == PinType::Int && b->Type == PinType::Float) ||
        (a->Type == PinType::Float && b->Type == PinType::Int))
        return true;

    // Bool ↔ Int (0/1)
    if ((a->Type == PinType::Bool && b->Type == PinType::Int) ||
        (a->Type == PinType::Int && b->Type == PinType::Bool))
        return true;

    // Bool ↔ Float (0.0/1.0)
    if ((a->Type == PinType::Bool && b->Type == PinType::Float) ||
        (a->Type == PinType::Float && b->Type == PinType::Bool))
        return true;

    return false;
}

// ============================================================================
// 自动类型转换辅助
// ============================================================================

std::string BlueprintEditor::GetConversionNode(PinType from, PinType to)
{
    // 转换表：(from, to) → definitionId
    // 覆盖所有已注册的 Conversion 节点
    static const struct { PinType from; PinType to; const char* defId; } kTable[] = {
        { PinType::Int,    PinType::Float,  "IntToFloat"    },
        { PinType::Float,  PinType::Int,    "FloatToInt"    },
        { PinType::Float,  PinType::Bool,   "FloatToBool"   },
        { PinType::Int,    PinType::String, "IntToString"   },
        { PinType::Float,  PinType::String, "FloatToString" },
        { PinType::Bool,   PinType::String, "BoolToString"  },
        { PinType::String, PinType::Int,    "StringToInt"   },
        { PinType::String, PinType::Float,  "StringToFloat" },
        // 新增：Bool ↔ Int / Bool → Float（本批新增节点）
        { PinType::Bool,   PinType::Int,    "BoolToInt"     },
        { PinType::Int,    PinType::Bool,   "IntToBool"     },
        { PinType::Bool,   PinType::Float,  "BoolToFloat"   },
    };

    for (const auto& entry : kTable)
        if (entry.from == from && entry.to == to)
            return entry.defId;
    return {};
}

Node* BlueprintEditor::InsertConversionNode(
    Pin* startPin, ed::PinId startPinId,
    Pin* endPin,   ed::PinId endPinId)
{
    PushUndoState();   // 插入转换节点前保存快照

    std::string defId = GetConversionNode(startPin->Type, endPin->Type);
    if (defId.empty()) return nullptr;

    // 计算插入位置（两端节点中点）
    ImVec2 startPos = ed::GetNodePosition(startPin->Node->ID);
    ImVec2 endPos   = ed::GetNodePosition(endPin->Node->ID);
    ImVec2 midPos   = ImVec2((startPos.x + endPos.x) * 0.5f,
                             (startPos.y + endPos.y) * 0.5f);

    // 生成转换节点
    Node* conv = SpawnNodeByDef(defId);
    if (!conv) return nullptr;

    BuildNodes();
    ActiveDoc()->isDirty = true;
    ed::SetNodePosition(conv->ID, midPos);

    // 找转换节点的首个 input 和首个 output
    Pin* convIn  = conv->Inputs.empty()  ? nullptr : &conv->Inputs.front();
    Pin* convOut = conv->Outputs.empty() ? nullptr : &conv->Outputs.front();
    if (!convIn || !convOut) return conv;

    // 先断开 endPin 上已有的同类型链接（保持 input 单一来源语义）
    {
        auto& links = ActiveDoc()->links;
        links.erase(std::remove_if(links.begin(), links.end(), [&](const Link& l) {
            return l.EndPinID == endPinId;
        }), links.end());
    }

    // startPin → convIn
    ActiveDoc()->links.emplace_back(Link(GetNextId(), startPinId, convIn->ID));
    ActiveDoc()->links.back().Color = GetIconColor(GetLinkColor(startPin, convIn));

    // convOut → endPin
    ActiveDoc()->links.emplace_back(Link(GetNextId(), convOut->ID, endPinId));
    ActiveDoc()->links.back().Color = GetIconColor(GetLinkColor(convOut, endPin));

    return conv;
}

// ============================================================================
// 节点构建
// ============================================================================

void BlueprintEditor::BuildNode(Node* node)
{
    for (auto& input : node->Inputs)
    {
        input.Node = node;
        input.Kind = PinKind::Input;
    }

    for (auto& output : node->Outputs)
    {
        output.Node = node;
        output.Kind = PinKind::Output;
    }
}

void BlueprintEditor::BuildNodes()
{
    for (auto& node : ActiveDoc()->nodes)
        BuildNode(&node);
}

// ============================================================================
// 类型映射
// ============================================================================

PinType BlueprintEditor::MapRTPinDataType(RTPinDataType dt, bool isExec)
{
    if (isExec) return PinType::Flow;
    switch (dt)
    {
    case RTPinDataType::Boolean: return PinType::Bool;
    case RTPinDataType::Integer: return PinType::Int;
    case RTPinDataType::Float:   return PinType::Float;
    case RTPinDataType::String:  return PinType::String;
    case RTPinDataType::Object:  return PinType::Object;
    case RTPinDataType::Array:   return PinType::Array;
    case RTPinDataType::Map:     return PinType::Map;
    case RTPinDataType::Any:     return PinType::Any;
    default:                     return PinType::Flow;
    }
}

RTPinDataType BlueprintEditor::MapPinType(PinType type)
{
    switch (type)
    {
    case PinType::Flow:     return RTPinDataType::Unknown; // Flow 是执行引脚
    case PinType::Bool:     return RTPinDataType::Boolean;
    case PinType::Int:      return RTPinDataType::Integer;
    case PinType::Float:    return RTPinDataType::Float;
    case PinType::String:   return RTPinDataType::String;
    case PinType::Object:   return RTPinDataType::Object;
    case PinType::Array:    return RTPinDataType::Array;
    case PinType::Map:      return RTPinDataType::Map;
    case PinType::Any:      return RTPinDataType::Any;
    case PinType::Function: return RTPinDataType::Custom;
    case PinType::Delegate: return RTPinDataType::Custom;
    default:                return RTPinDataType::Unknown;
    }
}

// ============================================================================
// 辅助：解析颜色 / 节点类型
// ============================================================================

static ImColor ParseHexColor(const std::string& hex, ImColor fallback = ImColor(255, 255, 255))
{
    std::string s = hex;
    if (!s.empty() && s[0] == '#') s = s.substr(1);
    if (s.size() != 6) return fallback;
    unsigned int r = 0, g = 0, b = 0;
    for (int i = 0; i < 6; ++i)
    {
        char c = s[i];
        unsigned int v = 0;
        if (c >= '0' && c <= '9') v = c - '0';
        else if (c >= 'a' && c <= 'f') v = 10 + c - 'a';
        else if (c >= 'A' && c <= 'F') v = 10 + c - 'A';
        if (i < 2) r = r * 16 + v;
        else if (i < 4) g = g * 16 + v;
        else b = b * 16 + v;
    }
    return ImColor((int)r, (int)g, (int)b);
}

static NodeType ParseNodeType(const std::string& s)
{
    if (s == "Simple")  return NodeType::Simple;
    if (s == "Tree")    return NodeType::Tree;
    if (s == "Comment") return NodeType::Comment;
    if (s == "Houdini") return NodeType::Houdini;
    return NodeType::Blueprint;
}

// ============================================================================
// 节点创建
// ============================================================================

Node* BlueprintEditor::SpawnNodeByDef(const std::string& defId)
{
    auto* def = m_NodeRegistry.getNodeDefinition(defId);
    if (!def) return nullptr;

    // Determine color & node type from definition
    ImColor color = ImColor(255, 255, 255);
    NodeType ntype = NodeType::Blueprint;
    if (!def->color.empty())
        color = ParseHexColor(def->color);
    else
    {
        // 按分类映射默认颜色（category 前缀匹配）
        const std::string& cat = def->category;
        auto startsWith = [&cat](const char* prefix) {
            return cat.find(prefix) == 0;
        };
        if      (startsWith("Flow"))        color = ImColor(50,  100, 220);
        else if (startsWith("Math"))        color = ImColor(60,  180, 80);
        else if (startsWith("String"))      color = ImColor(180, 80,  200);
        else if (startsWith("Debug"))       color = ImColor(200, 60,  60);
        else if (startsWith("Action") ||
                 startsWith("Event"))       color = ImColor(220, 100, 40);
        else if (startsWith("Conversion"))  color = ImColor(80,  160, 220);
        else if (startsWith("Array"))       color = ImColor(200, 150, 30);
        else if (startsWith("Misc/Map") ||
                 startsWith("Map"))         color = ImColor(40,  180, 200);
        else if (startsWith("Custom"))      color = ImColor(60,  180, 130);
        else                                color = ImColor(100, 100, 120);
    }

    auto it = def->customProperties.find("editorType");
    if (it != def->customProperties.end())
        ntype = ParseNodeType(it->second);

    ActiveDoc()->nodes.emplace_back(GetNextId(), def->name.c_str(), color);
    auto& node = ActiveDoc()->nodes.back();
    node.Type = ntype;
    node.DefinitionId = defId;

    if (ntype == NodeType::Comment && def->defaultSize.width > 0)
        node.Size = ImVec2(def->defaultSize.width, def->defaultSize.height);

    // Input pins
    for (const auto& pd : def->inputPins)
    {
        PinType pt = MapRTPinDataType(pd.dataType, pd.isExec);
        node.Inputs.emplace_back(GetNextId(), pd.name.c_str(), pt);

        // Apply default values
        auto& pin = node.Inputs.back();
        if (pd.dataType == RTPinDataType::Boolean)
            pin.BoolValue = pd.defaultValue.asBool();
        else if (pd.dataType == RTPinDataType::Integer)
            pin.IntValue = pd.defaultValue.asInt();
        else if (pd.dataType == RTPinDataType::Float)
            pin.FloatValue = static_cast<float>(pd.defaultValue.asFloat());
        else if (pd.dataType == RTPinDataType::String)
            pin.StringValue = pd.defaultValue.asString();
        else if (pd.dataType == RTPinDataType::Object)
            pin.ObjectValue = pd.defaultValue.asObjectId();

        // 传递声明式 hiddenWhen 规则
        auto hwIt = pd.customProperties.find("hiddenWhen");
        if (hwIt != pd.customProperties.end())
            pin.HiddenWhen = hwIt->second;
    }

    // Output pins
    for (const auto& pd : def->outputPins)
    {
        PinType pt = MapRTPinDataType(pd.dataType, pd.isExec);
        node.Outputs.emplace_back(GetNextId(), pd.name.c_str(), pt);

        // 传递声明式 hiddenWhen 规则
        auto hwIt = pd.customProperties.find("hiddenWhen");
        if (hwIt != pd.customProperties.end())
            node.Outputs.back().HiddenWhen = hwIt->second;
    }

    BuildNode(&node);

    // Check for dynamic input pins support
    auto dynIt = def->customProperties.find("dynamicInputs");
    if (dynIt != def->customProperties.end() && !dynIt->second.empty())
    {
        node.HasDynamicInputs = true;
        node.DynamicInputFixedCount = static_cast<int>(node.Inputs.size());
        // Parse pin type from the property value (e.g. "String", "Float", "Int", "Bool")
        const std::string& dynType = dynIt->second;
        if (dynType == "String")       node.DynamicInputPinType = PinType::String;
        else if (dynType == "Float")   node.DynamicInputPinType = PinType::Float;
        else if (dynType == "Int")     node.DynamicInputPinType = PinType::Int;
        else if (dynType == "Bool")    node.DynamicInputPinType = PinType::Bool;
        else if (dynType == "Object")  node.DynamicInputPinType = PinType::Object;
        else if (dynType == "Any")     node.DynamicInputPinType = PinType::Any;
        else if (dynType == "Array")   node.DynamicInputPinType = PinType::Array;
        else if (dynType == "Map")     node.DynamicInputPinType = PinType::Map;
        else                           node.DynamicInputPinType = PinType::String;
    }

    return &node;
}

void BlueprintEditor::FixupSpecialPinTypes(Node* node, const RTNodeDef* def)
{
    if (!node || !def) return;
    // Fix input pins
    for (size_t i = 0; i < node->Inputs.size() && i < def->inputPins.size(); ++i)
    {
        auto it = def->inputPins[i].customProperties.find("pinType");
        if (it != def->inputPins[i].customProperties.end())
        {
            if (it->second == "Delegate") node->Inputs[i].Type = PinType::Delegate;
            else if (it->second == "Function") node->Inputs[i].Type = PinType::Function;
        }
    }
    // Fix output pins
    for (size_t i = 0; i < node->Outputs.size() && i < def->outputPins.size(); ++i)
    {
        auto it = def->outputPins[i].customProperties.find("pinType");
        if (it != def->outputPins[i].customProperties.end())
        {
            if (it->second == "Delegate") node->Outputs[i].Type = PinType::Delegate;
            else if (it->second == "Function") node->Outputs[i].Type = PinType::Function;
        }
    }
}

// ============================================================================
// 右键菜单
// ============================================================================

// 辅助：递归构建多级分类菜单
// categoryTree 结构: map<子分类名, pair<子树, 该层直属节点列表>>
// ============================================================================
// Create Node 菜单（带缓存，性能优化版）
// ============================================================================

// 辅助：将 allDefs 按 category 路径填入缓存树
static void RebuildCategoryTree(
    const std::vector<const RTNodeDef*>& allDefs,
    const std::vector<RTNodeCategory>& categories,
    std::map<std::string, BlueprintEditor::CategoryMenuNode>& rootChildren,
    std::vector<std::string>& rootOrder,
    std::unordered_map<std::string, std::string>& catIdToName)
{
    rootChildren.clear();
    rootOrder.clear();
    catIdToName.clear();

    for (const auto& cat : categories)
    {
        catIdToName[cat.id] = cat.name;
        rootOrder.push_back(cat.id);
    }

    for (const auto* d : allDefs)
    {
        if (d->category.empty()) continue;

        std::vector<std::string> parts;
        std::string seg;
        for (char c : d->category)
        {
            if (c == '/')
            {
                if (!seg.empty()) parts.push_back(seg);
                seg.clear();
            }
            else
                seg += c;
        }
        if (!seg.empty()) parts.push_back(seg);
        if (parts.empty()) continue;

        bool found = false;
        for (const auto& r : rootOrder)
            if (r == parts[0]) { found = true; break; }
        if (!found)
            rootOrder.push_back(parts[0]);

        BlueprintEditor::CategoryMenuNode* node = &rootChildren[parts[0]];
        for (size_t i = 1; i < parts.size(); ++i)
            node = &node->children[parts[i]];

        node->directNodes.push_back(d);
    }
}

Node* BlueprintEditor::ShowCreateNodeMenu()
{
    Node* result = nullptr;

    // FunctionLibrary 蓝图过滤：禁止添加事件驱动节点
    // （Event.* / OnBeginPlay / OnTick / CustomEvent 等）
    bool isLibBP = ActiveDoc() && ActiveDoc()->blueprintClass == RTBlueprintClass::FunctionLibrary;
    auto isEventCategory = [](const std::string& cat) -> bool {
        // 分类前缀 "Event" 或就叫 "Events"
        return cat == "Event" || cat == "Events" ||
               cat.rfind("Event", 0) == 0;
    };
    auto isLibraryFilteredDef = [&](const RTNodeDef* d) -> bool {
        if (!isLibBP) return false;
        return isEventCategory(d->category);
    };

    const auto& allDefsRef = m_NodeRegistry.getAllNodeDefinitions();
    size_t defCount = allDefsRef.size();

    // ── 懒加载分类树（仅 registry 变化时重建） ──────────────────────────
    if (defCount != m_CachedDefCount)
    {
        std::vector<const RTNodeDef*> allDefs(allDefsRef.begin(), allDefsRef.end());
        auto categories = m_NodeRegistry.getAllCategories();
        RebuildCategoryTree(allDefs, categories,
                            m_CachedRootChildren, m_CachedRootOrder, m_CachedCatIdToName);
        m_CachedDefCount = defCount;
        // 搜索缓存也失效
        m_CachedSearchFilter = "\xFF";  // 强制下次重算
    }

    // ── 搜索框 ────────────────────────────────────────────────────────────
    // 搜索框（static 可接受：右键菜单是瞬态 UI，不关联特定文档）
    static char searchBuf[128] = "";
    // 菜单刚打开时自动清空并聚焦搜索框
    static bool s_justOpened = false;
    if (ImGui::IsWindowAppearing())
    {
        searchBuf[0] = '\0';
        m_CachedSearchFilter.clear();
        s_justOpened = true;
    }
    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
    if (s_justOpened)
    {
        ImGui::SetKeyboardFocusHere();
        s_justOpened = false;
    }
    bool searchChanged = ImGui::InputTextWithHint(
        "##search", ICON_FA_MAGNIFYING_GLASS " Search nodes...", searchBuf, sizeof(searchBuf));

    std::string filter(searchBuf);

    if (!filter.empty())
    {
        ImGui::Separator();

        // ── 缓存搜索结果（仅 filter 变化时重算） ─────────────────────────
        if (filter != m_CachedSearchFilter)
        {
            m_CachedSearchFilter = filter;
            m_CachedSearchResults.clear();

            std::string lower_filter = filter;
            for (auto& c : lower_filter) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));

            for (const auto* d : allDefsRef)
            {
                // 匹配名字
                std::string lower_name = d->name;
                for (auto& c : lower_name) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
                bool hit = (lower_name.find(lower_filter) != std::string::npos);

                // 也匹配 id（如 "BoolToInt"）
                if (!hit)
                {
                    std::string lower_id = d->id;
                    for (auto& c : lower_id) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
                    hit = (lower_id.find(lower_filter) != std::string::npos);
                }

                // 匹配 category
                if (!hit)
                {
                    std::string lower_cat = d->category;
                    for (auto& c : lower_cat) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
                    hit = (lower_cat.find(lower_filter) != std::string::npos);
                }

                if (hit)
                    m_CachedSearchResults.push_back(d);
            }

            // 排序：名字字母序
            std::sort(m_CachedSearchResults.begin(), m_CachedSearchResults.end(),
                [](const RTNodeDef* a, const RTNodeDef* b) { return a->name < b->name; });
        }

        // ── Enter 快捷：创建第一个搜索结果 ─────────────────────────────────
        if ((ImGui::IsKeyPressed(ImGuiKey_Enter) || ImGui::IsKeyPressed(ImGuiKey_KeypadEnter))
            && !m_CachedSearchResults.empty())
        {
            for (const auto* d : m_CachedSearchResults)
            {
                if (isLibraryFilteredDef(d)) continue;
                PushUndoState();
                result = SpawnNodeByDef(d->id);
                if (result)
                    FixupSpecialPinTypes(result, m_NodeRegistry.getNodeDefinition(d->id));
                searchBuf[0] = '\0';
                m_CachedSearchFilter.clear();
                ImGui::CloseCurrentPopup();
                break;
            }
        }

        // ── 渲染搜索结果（直接遍历缓存，无 tolower） ──────────────────────
        for (const auto* d : m_CachedSearchResults)
        {
            if (isLibraryFilteredDef(d)) continue;  // FunctionLibrary 过滤事件节点
            if (ImGui::MenuItem(d->name.c_str()))
            {
                PushUndoState();
                result = SpawnNodeByDef(d->id);
                if (result)
                    FixupSpecialPinTypes(result, m_NodeRegistry.getNodeDefinition(d->id));
                searchBuf[0] = '\0';
                m_CachedSearchFilter.clear();
            }
            if (!d->category.empty() && ImGui::IsItemHovered())
                ImGui::SetTooltip("Category: %s", d->category.c_str());
        }

        if (m_CachedSearchResults.empty())
            ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "No matching nodes");

        return result;
    }
    else
    {
        // filter 清空时重置缓存
        if (!m_CachedSearchFilter.empty())
            m_CachedSearchFilter.clear();
    }

    ImGui::Separator();

    // ── 按分类树渲染（直接用缓存） ────────────────────────────────────────
    std::function<void(const CategoryMenuNode&)> renderMenu;
    renderMenu = [&](const CategoryMenuNode& menuNode)
    {
        for (const auto& child : menuNode.children)
        {
            if (ImGui::BeginMenu(child.first.c_str()))
            {
                renderMenu(child.second);
                ImGui::EndMenu();
            }
        }

        if (!menuNode.children.empty() && !menuNode.directNodes.empty())
            ImGui::Separator();

        for (const auto* d : menuNode.directNodes)
        {
            if (ImGui::MenuItem(d->name.c_str()))
            {
                PushUndoState();
                result = SpawnNodeByDef(d->id);
                if (result)
                    FixupSpecialPinTypes(result, m_NodeRegistry.getNodeDefinition(d->id));
            }
        }
    };

    for (const auto& rootId : m_CachedRootOrder)
    {
        auto it = m_CachedRootChildren.find(rootId);
        if (it == m_CachedRootChildren.end()) continue;

        auto nameIt = m_CachedCatIdToName.find(rootId);
        const char* displayName = (nameIt != m_CachedCatIdToName.end()) ?
            nameIt->second.c_str() : rootId.c_str();

        // FunctionLibrary 蓝图过滤：跳过 Event 分类顶级菜单
        if (isLibBP && isEventCategory(rootId))
            continue;

        if (ImGui::BeginMenu(displayName))
        {
            renderMenu(it->second);
            ImGui::EndMenu();
        }
    }

    return result;
}

// ============================================================================
// Runtime 数据构建
// ============================================================================

RTBlueprintData BlueprintEditor::BuildRuntimeData()
{
    RTBlueprintData bp;
    bp.metadata.name = ActiveDoc()->untitledName.empty() ? "EditorBlueprint" : ActiveDoc()->untitledName;
    bp.metadata.description = "Built from editor state";
    // 同步蓝图类型（确保 FunctionLibrary 保护条件正确触发）
    bp.metadata.blueprintClass = ActiveDoc()->blueprintClass;

    // 转换节点
    for (const auto& node : ActiveDoc()->nodes)
    {
        RTNodeInstance ni;
        ni.id = static_cast<uint64_t>(reinterpret_cast<uintptr_t>(node.ID.AsPointer()));
        ni.definitionId = node.DefinitionId.empty() ? node.Name : node.DefinitionId;
        ni.name = node.Name;
        ni.isEnabled = true;
        // 保存折叠状态到顶层字段（v2+），不再写 customProperties["__collapsed"]
        ni.isCollapsed = node.isCollapsed;

        // Comment 节点颜色：序列化为 customProperties["__color"] = "RRGGBB"
        if (node.Type == NodeType::Comment)
        {
            int r = static_cast<int>(node.Color.Value.x * 255.0f);
            int g = static_cast<int>(node.Color.Value.y * 255.0f);
            int b = static_cast<int>(node.Color.Value.z * 255.0f);
            char colorBuf[8];
            snprintf(colorBuf, sizeof(colorBuf), "%02X%02X%02X", r, g, b);
            ni.customProperties["__color"] = colorBuf;
        }

        // 输入引脚
        for (const auto& pin : node.Inputs)
        {
            RTPinInfo pi;
            pi.id = static_cast<uint64_t>(reinterpret_cast<uintptr_t>(pin.ID.AsPointer()));
            pi.name = pin.Name;
            pi.kind = PinKind::Input;
            pi.dataType = MapPinType(pin.Type);
            pi.isExec = (pin.Type == PinType::Flow);

            switch (pin.Type)
            {
            case PinType::Bool:   pi.defaultValue = RTVariant(pin.BoolValue); break;
            case PinType::Int:    pi.defaultValue = RTVariant(pin.IntValue); break;
            case PinType::Float:  pi.defaultValue = RTVariant(static_cast<double>(pin.FloatValue)); break;
            case PinType::String: pi.defaultValue = RTVariant(pin.StringValue); break;
            case PinType::Object: pi.defaultValue = RTVariant::MakeObject(pin.ObjectValue); break;
            default: break;
            }

            ni.pins.push_back(pi);
        }

        // 输出引脚
        for (const auto& pin : node.Outputs)
        {
            RTPinInfo pi;
            pi.id = static_cast<uint64_t>(reinterpret_cast<uintptr_t>(pin.ID.AsPointer()));
            pi.name = pin.Name;
            pi.kind = PinKind::Output;
            pi.dataType = MapPinType(pin.Type);
            pi.isExec = (pin.Type == PinType::Flow);

            ni.pins.push_back(pi);
        }

        bp.nodes.push_back(std::move(ni));
    }

    // 转换链接
    for (const auto& link : ActiveDoc()->links)
    {
        RTLinkInstance li;
        li.id = static_cast<uint64_t>(reinterpret_cast<uintptr_t>(link.ID.AsPointer()));
        li.startPinId = static_cast<uint64_t>(reinterpret_cast<uintptr_t>(link.StartPinID.AsPointer()));
        li.endPinId = static_cast<uint64_t>(reinterpret_cast<uintptr_t>(link.EndPinID.AsPointer()));
        li.isEnabled = true;
        bp.links.push_back(li);
    }

    // 转换变量
    bp.variables = ActiveDoc()->variables;

    // 转换函数定义
    bp.functions = ActiveDoc()->functions;

    return bp;
}

// ============================================================================
// 多文档管理
// ============================================================================

BlueprintDocument* BlueprintEditor::CreateNewDocument()
{
    auto doc = std::make_unique<BlueprintDocument>();

    // 为每个文档创建独立的节点编辑器上下文
    ed::Config config;
    config.SettingsFile = nullptr;  // 内存模式，不写文件

    config.UserPointer = this;

    config.LoadNodeSettings = [](ed::NodeId nodeId, char* data, void* userPointer) -> size_t
    {
        auto self = static_cast<BlueprintEditor*>(userPointer);
        auto node = self->FindNode(nodeId);
        if (!node)
            return 0;
        if (data != nullptr)
            memcpy(data, node->State.data(), node->State.size());
        return node->State.size();
    };

    config.SaveNodeSettings = [](ed::NodeId nodeId, const char* data, size_t size, ed::SaveReasonFlags reason, void* userPointer) -> bool
    {
        auto self = static_cast<BlueprintEditor*>(userPointer);
        auto node = self->FindNode(nodeId);
        if (!node)
            return false;
        node->State.assign(data, size);
        self->TouchNode(nodeId);
        return true;
    };

    doc->editorContext = ed::CreateEditor(&config);

    m_Documents.push_back(std::move(doc));
    m_ActiveDocIndex = static_cast<int>(m_Documents.size()) - 1;

    return m_Documents.back().get();
}

void BlueprintEditor::CloseDocument(int index)
{
    if (index < 0 || index >= (int)m_Documents.size())
        return;

    // 销毁该文档的编辑器上下文
    auto& doc = m_Documents[index];
    if (doc->editorContext)
    {
        ed::DestroyEditor(doc->editorContext);
        doc->editorContext = nullptr;
    }

    m_Documents.erase(m_Documents.begin() + index);

    // 调整活跃索引
    if (m_ActiveDocIndex >= (int)m_Documents.size())
        m_ActiveDocIndex = (int)m_Documents.size() - 1;
    if (m_ActiveDocIndex < 0)
        m_ActiveDocIndex = 0;

    // 无文档时清空编辑器上下文，回到欢迎页
    if (m_Documents.empty())
    {
        m_ActiveDocIndex = -1;
        ed::SetCurrentEditor(nullptr);
    }
    else
    {
        ed::SetCurrentEditor(ActiveDoc()->editorContext);
    }
}

// ============================================================================
// Application 生命周期
// ============================================================================

void BlueprintEditor::OnStart()
{
    // ── 日志系统（最先启动，后续所有模块都可用 BPLOG）─────────────────────
    BpLogger::Get().Start("logs");
    BPLOG("BlueprintEditor starting up");

    // Initialize node definition registry (全局共享)
    RegisterBuiltinNodeDefinitions();
    RegisterBuiltinHandlers();

    // 不自动创建空白文档——无工程时右侧画布留空，由 Project 面板引导用户

    // 加载纹理资源
    m_HeaderBackground = LoadTexture("data/BlueprintBackground.png");
    m_SaveIcon         = LoadTexture("data/ic_save_white_24dp.png");
    m_RestoreIcon      = LoadTexture("data/ic_restore_white_24dp.png");

    // 加载最近文件列表
    LoadRecentFiles();
    LoadRecentProjects();

    // 加载外部自定义节点定义（文件不存在时静默跳过）
    ::NodeEditor::Runtime::LoadCustomNodesFromFile(m_NodeRegistry, "data/custom_nodes.json");

    // 加载公共函数库（目录不存在时静默返回 0）
    // 若工程系统启用后，此处可改为 SyncProjectLibrariesToRegistry()
    ::NodeEditor::Runtime::LoadFunctionLibrary(m_NodeRegistry, "data/function_library");

    // 加载主题（不存在时默认 Dark）
    if (!ThemeManager::Get().LoadFromFile("data/theme.json"))
        ThemeManager::Get().Apply("Dark");

    // 设置初始标题
    SetTitle("Blueprint Editor - [No Project]");
    BPLOG("BlueprintEditor started");
}

void BlueprintEditor::OnStop()
{
    BPLOG("BlueprintEditor shutting down");
    auto releaseTexture = [this](ImTextureID& id)
    {
        if (id != ImTextureID_Invalid)
        {
            DestroyTexture(id);
            id = ImTextureID_Invalid;
        }
    };

    releaseTexture(m_RestoreIcon);
    releaseTexture(m_SaveIcon);
    releaseTexture(m_HeaderBackground);

    // 保存主题设置
    ThemeManager::Get().SaveToFile("data/theme.json");

    // 销毁所有文档的编辑器上下文
    for (auto& doc : m_Documents)
    {
        if (doc->editorContext)
        {
            ed::DestroyEditor(doc->editorContext);
            doc->editorContext = nullptr;
        }
    }
    m_Documents.clear();

    // Logger 最后停止（确保所有日志写完）
    BpLogger::Get().Stop();
}

ImGuiWindowFlags BlueprintEditor::GetWindowFlags() const
{
    return
        ImGuiWindowFlags_NoTitleBar |
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoScrollbar |
        ImGuiWindowFlags_NoScrollWithMouse |
        ImGuiWindowFlags_NoSavedSettings |
        ImGuiWindowFlags_NoBringToFrontOnFocus |
        ImGuiWindowFlags_NoDocking |
        ImGuiWindowFlags_MenuBar;
}
