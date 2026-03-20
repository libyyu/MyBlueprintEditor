// BlueprintEditor.cpp -- 蓝图编辑器核心逻辑
#include "BlueprintEditor.h"

// ============================================================================
// ID 管理
// ============================================================================

int BlueprintEditor::GetNextId()
{
    return m_NextId++;
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
    m_NodeTouchTime[id] = m_TouchTime;
}

float BlueprintEditor::GetTouchProgress(ed::NodeId id)
{
    auto it = m_NodeTouchTime.find(id);
    if (it != m_NodeTouchTime.end() && it->second > 0.0f)
        return (m_TouchTime - it->second) / m_TouchTime;

    return 0.0f;
}

void BlueprintEditor::UpdateTouch()
{
    const auto deltaTime = ImGui::GetIO().DeltaTime;
    for (auto& entry : m_NodeTouchTime)
    {
        if (entry.second > 0.0f)
            entry.second -= deltaTime;
    }
}

// ============================================================================
// 查找
// ============================================================================

Node* BlueprintEditor::FindNode(ed::NodeId id)
{
    for (auto& node : m_Nodes)
        if (node.ID == id)
            return &node;

    return nullptr;
}

Link* BlueprintEditor::FindLink(ed::LinkId id)
{
    for (auto& link : m_Links)
        if (link.ID == id)
            return &link;

    return nullptr;
}

Pin* BlueprintEditor::FindPin(ed::PinId id)
{
    if (!id)
        return nullptr;

    for (auto& node : m_Nodes)
    {
        for (auto& pin : node.Inputs)
            if (pin.ID == id)
                return &pin;

        for (auto& pin : node.Outputs)
            if (pin.ID == id)
                return &pin;
    }

    return nullptr;
}

bool BlueprintEditor::IsPinLinked(ed::PinId id)
{
    if (!id)
        return false;

    for (auto& link : m_Links)
        if (link.StartPinID == id || link.EndPinID == id)
            return true;

    return false;
}

bool BlueprintEditor::CanCreateLink(Pin* a, Pin* b)
{
    if (!a || !b || a == b || a->Kind == b->Kind || a->Type != b->Type || a->Node == b->Node)
        return false;

    return true;
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
    for (auto& node : m_Nodes)
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
// 引脚定义辅助
// ============================================================================

RTPinDef BlueprintEditor::MakePin(const char* name, RTPinDataType dt, bool isExec)
{
    RTPinDef p;
    p.name = name;
    p.dataType = dt;
    p.isExec = isExec;
    return p;
}

RTPinDef BlueprintEditor::MakeFlowPin(const char* name)
{
    return MakePin(name, RTPinDataType::Unknown, true);
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

    auto it = def->customProperties.find("editorType");
    if (it != def->customProperties.end())
        ntype = ParseNodeType(it->second);

    m_Nodes.emplace_back(GetNextId(), def->name.c_str(), color);
    auto& node = m_Nodes.back();
    node.Type = ntype;

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
            pin.IntValue = static_cast<int>(pd.defaultValue.asInt());
        else if (pd.dataType == RTPinDataType::Float)
            pin.FloatValue = static_cast<float>(pd.defaultValue.asFloat());
        else if (pd.dataType == RTPinDataType::String)
            pin.StringValue = pd.defaultValue.asString();
    }

    // Output pins
    for (const auto& pd : def->outputPins)
    {
        PinType pt = MapRTPinDataType(pd.dataType, pd.isExec);
        node.Outputs.emplace_back(GetNextId(), pd.name.c_str(), pt);
    }

    BuildNode(&node);
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

Node* BlueprintEditor::ShowCreateNodeMenu()
{
    Node* result = nullptr;

    // Show category sub-menus
    auto categories = m_NodeRegistry.getAllCategories();
    for (const auto& cat : categories)
    {
        if (ImGui::BeginMenu(cat.name.c_str()))
        {
            auto defs = m_NodeRegistry.getNodesByCategory(cat.id);
            for (const auto& d : defs)
            {
                if (ImGui::MenuItem(d.name.c_str()))
                {
                    result = SpawnNodeByDef(d.id);
                    if (result)
                        FixupSpecialPinTypes(result, m_NodeRegistry.getNodeDefinition(d.id));
                }
            }
            ImGui::EndMenu();
        }
    }

    // Search filter — only show flat list when user has typed something
    ImGui::Separator();
    static char searchBuf[128] = "";
    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
    ImGui::InputText("##search", searchBuf, sizeof(searchBuf));
    std::string filter(searchBuf);

    if (!filter.empty())
    {
        auto allDefs = m_NodeRegistry.getAllNodeDefinitions();
        std::sort(allDefs.begin(), allDefs.end(),
            [](const RTNodeDef& a, const RTNodeDef& b) { return a.name < b.name; });

        for (const auto& d : allDefs)
        {
            // Simple case-insensitive substring match
            std::string lower_name = d.name;
            std::string lower_filter = filter;
            for (auto& c : lower_name) c = (char)tolower(c);
            for (auto& c : lower_filter) c = (char)tolower(c);
            if (lower_name.find(lower_filter) == std::string::npos)
                continue;
            if (ImGui::MenuItem(d.name.c_str()))
            {
                result = SpawnNodeByDef(d.id);
                if (result)
                    FixupSpecialPinTypes(result, m_NodeRegistry.getNodeDefinition(d.id));
            }
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
    bp.metadata.name = "EditorBlueprint";
    bp.metadata.description = "Built from editor state";

    // 转换节点
    for (const auto& node : m_Nodes)
    {
        RTNodeInstance ni;
        ni.id = static_cast<uint64_t>(reinterpret_cast<uintptr_t>(node.ID.AsPointer()));
        ni.definitionId = node.Name;
        ni.name = node.Name;
        ni.isEnabled = true;

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
            case PinType::Int:    pi.defaultValue = RTVariant(static_cast<int64_t>(pin.IntValue)); break;
            case PinType::Float:  pi.defaultValue = RTVariant(static_cast<double>(pin.FloatValue)); break;
            case PinType::String: pi.defaultValue = RTVariant(pin.StringValue); break;
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
    for (const auto& link : m_Links)
    {
        RTLinkInstance li;
        li.id = static_cast<uint64_t>(reinterpret_cast<uintptr_t>(link.ID.AsPointer()));
        li.startPinId = static_cast<uint64_t>(reinterpret_cast<uintptr_t>(link.StartPinID.AsPointer()));
        li.endPinId = static_cast<uint64_t>(reinterpret_cast<uintptr_t>(link.EndPinID.AsPointer()));
        li.isEnabled = true;
        bp.links.push_back(li);
    }

    return bp;
}

// ============================================================================
// Application 生命周期
// ============================================================================

ed::EditorContext* s_Editor = nullptr;

void BlueprintEditor::OnStart()
{
    ed::Config config;

    config.SettingsFile = "Blueprints.json";

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

    s_Editor = ed::CreateEditor(&config);
    ed::SetCurrentEditor(s_Editor);

    // Initialize node definition registry
    RegisterBuiltinNodeDefinitions();
    RegisterBuiltinHandlers();
    /*
    // Spawn some initial nodes for demo
    Node* node;
    node = SpawnNodeByDef("InputActionFire");
    if (node) { FixupSpecialPinTypes(node, m_NodeRegistry.getNodeDefinition("InputActionFire")); ed::SetNodePosition(node->ID, ImVec2(-252, 220)); }

    node = SpawnNodeByDef("Branch");
    if (node) ed::SetNodePosition(node->ID, ImVec2(-300, 351));

    node = SpawnNodeByDef("DoN");
    if (node) ed::SetNodePosition(node->ID, ImVec2(-238, 504));

    node = SpawnNodeByDef("OutputAction");
    if (node) { FixupSpecialPinTypes(node, m_NodeRegistry.getNodeDefinition("OutputAction")); ed::SetNodePosition(node->ID, ImVec2(71, 80)); }

    node = SpawnNodeByDef("SetTimer");
    if (node) { FixupSpecialPinTypes(node, m_NodeRegistry.getNodeDefinition("SetTimer")); ed::SetNodePosition(node->ID, ImVec2(168, 316)); }

    node = SpawnNodeByDef("Sequence");
    if (node) ed::SetNodePosition(node->ID, ImVec2(1028, 329));

    node = SpawnNodeByDef("MoveTo");
    if (node) ed::SetNodePosition(node->ID, ImVec2(1204, 458));

    node = SpawnNodeByDef("RandomWait");
    if (node) ed::SetNodePosition(node->ID, ImVec2(868, 538));

    node = SpawnNodeByDef("Comment");
    if (node) { ed::SetNodePosition(node->ID, ImVec2(112, 576)); ed::SetGroupSize(node->ID, ImVec2(384, 154)); }

    node = SpawnNodeByDef("Comment");
    if (node) { ed::SetNodePosition(node->ID, ImVec2(800, 224)); ed::SetGroupSize(node->ID, ImVec2(640, 400)); }

    node = SpawnNodeByDef("Less");
    if (node) ed::SetNodePosition(node->ID, ImVec2(366, 652));

    node = SpawnNodeByDef("Weird");
    if (node) ed::SetNodePosition(node->ID, ImVec2(144, 652));

    node = SpawnNodeByDef("Message");
    if (node) ed::SetNodePosition(node->ID, ImVec2(-348, 698));

    node = SpawnNodeByDef("PrintString");
    if (node) ed::SetNodePosition(node->ID, ImVec2(-69, 652));

    node = SpawnNodeByDef("HoudiniTransform");
    if (node) ed::SetNodePosition(node->ID, ImVec2(500, -70));

    node = SpawnNodeByDef("HoudiniGroup");
    if (node) ed::SetNodePosition(node->ID, ImVec2(500, 42));
    */
    ed::NavigateToContent();

    BuildNodes();

    /*
    // Demo links (require demo nodes above to be spawned first)
    m_Links.push_back(Link(GetNextLinkId(), m_Nodes[5].Outputs[0].ID, m_Nodes[6].Inputs[0].ID));
    m_Links.push_back(Link(GetNextLinkId(), m_Nodes[5].Outputs[0].ID, m_Nodes[7].Inputs[0].ID));

    m_Links.push_back(Link(GetNextLinkId(), m_Nodes[14].Outputs[0].ID, m_Nodes[15].Inputs[0].ID));
    */

    m_HeaderBackground = LoadTexture("data/BlueprintBackground.png");
    m_SaveIcon         = LoadTexture("data/ic_save_white_24dp.png");
    m_RestoreIcon      = LoadTexture("data/ic_restore_white_24dp.png");

    // 设置初始标题
    SetTitle("Blueprint Editor - [New]");
}

void BlueprintEditor::OnStop()
{
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

    if (s_Editor)
    {
        ed::DestroyEditor(s_Editor);
        s_Editor = nullptr;
    }
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
        ImGuiWindowFlags_MenuBar;
}
