// BlueprintEditor/ThemeManager.cpp -- 主题管理器实现
#include "ThemeManager.h"
#include "BlueprintEditor.h"
#include <imgui.h>
#include <imgui_node_editor.h>
#include <fstream>
#include <sstream>

// crude_json for persistence
#include "../Utils/Json/crude_json.h"

namespace ed = ax::NodeEditor;

ThemeManager& ThemeManager::Get()
{
    static ThemeManager instance;
    return instance;
}

ThemeManager::ThemeManager()
{
    RegisterBuiltinThemes();
}

void ThemeManager::RegisterBuiltinThemes()
{
    // ── Dark（默认暗色 — VS 2022 风格）──────────────────────────────────
    {
        EditorTheme t;
        t.name           = "Dark";
        // VS 2022 的基调：#1E1E1E（纯灰）带微蓝偏移
        t.windowBg       = {0.118f, 0.118f, 0.145f, 1.00f};  // #1E1E25
        t.childBg        = {0.110f, 0.110f, 0.135f, 1.00f};  // #1C1C22
        t.frameBg        = {0.165f, 0.165f, 0.200f, 1.00f};  // #2A2A33
        t.frameHovered   = {0.220f, 0.230f, 0.300f, 1.00f};  // #383A4D
        t.frameActive    = {0.265f, 0.290f, 0.420f, 1.00f};  // #444A6B
        t.header         = {0.180f, 0.200f, 0.280f, 1.00f};  // #2E3347
        t.headerHovered  = {0.230f, 0.265f, 0.400f, 1.00f};  // #3B4466
        t.button         = {0.200f, 0.220f, 0.320f, 1.00f};  // #333852
        t.buttonHovered  = {0.260f, 0.300f, 0.460f, 1.00f};  // #424D75
        t.text           = {0.870f, 0.880f, 0.920f, 1.00f};  // #DEE0EB
        t.textDisabled   = {0.430f, 0.440f, 0.500f, 1.00f};  // #6E7080
        t.scrollbarBg    = {0.080f, 0.080f, 0.100f, 0.85f};
        t.scrollbarGrab  = {0.250f, 0.260f, 0.340f, 1.00f};
        t.tabActive      = {0.180f, 0.200f, 0.280f, 1.00f};  // 与 header 同色（VS 风格）
        t.tabHovered     = {0.230f, 0.265f, 0.400f, 1.00f};
        t.tab            = {0.130f, 0.133f, 0.168f, 1.00f};  // 非活跃标签微暗
        t.separator      = {0.200f, 0.210f, 0.270f, 1.00f};  // #333645
        t.popupBg        = {0.128f, 0.128f, 0.160f, 0.97f};
        t.editorBg       = {0.118f, 0.118f, 0.145f, 1.00f};
        t.editorGrid     = {0.180f, 0.185f, 0.225f, 0.50f};
        t.editorGridLine = {0.220f, 0.230f, 0.280f, 0.25f};
        t.nodeBackground = {0.150f, 0.155f, 0.195f, 0.97f};
        t.nodeBorder     = {0.240f, 0.260f, 0.360f, 0.70f};
        t.nodeSelection  = {0.260f, 0.590f, 0.980f, 0.80f};  // VS 蓝色选中
        t.linkFlow       = {0.880f, 0.900f, 1.000f, 0.80f};
        m_themes["Dark"] = t;
        m_themeOrder.push_back("Dark");
    }

    // ── Blueprint（UE4 风格深蓝）────────────────────────────────────────
    {
        EditorTheme t;
        t.name           = "Blueprint";
        t.windowBg       = {0.10f, 0.12f, 0.18f, 1.00f};
        t.childBg        = {0.08f, 0.10f, 0.16f, 1.00f};
        t.frameBg        = {0.14f, 0.18f, 0.28f, 1.00f};
        t.frameHovered   = {0.20f, 0.26f, 0.40f, 1.00f};
        t.frameActive    = {0.25f, 0.35f, 0.55f, 1.00f};
        t.header         = {0.15f, 0.22f, 0.38f, 1.00f};
        t.headerHovered  = {0.20f, 0.30f, 0.50f, 1.00f};
        t.button         = {0.18f, 0.28f, 0.48f, 1.00f};
        t.buttonHovered  = {0.25f, 0.38f, 0.62f, 1.00f};
        t.text           = {0.82f, 0.88f, 1.00f, 1.00f};
        t.textDisabled   = {0.35f, 0.42f, 0.58f, 1.00f};
        t.scrollbarBg    = {0.05f, 0.08f, 0.14f, 0.80f};
        t.scrollbarGrab  = {0.22f, 0.32f, 0.52f, 1.00f};
        t.tabActive      = {0.22f, 0.36f, 0.65f, 1.00f};
        t.tabHovered     = {0.18f, 0.30f, 0.52f, 1.00f};
        t.tab            = {0.12f, 0.18f, 0.32f, 1.00f};
        t.separator      = {0.18f, 0.26f, 0.42f, 1.00f};
        t.popupBg        = {0.08f, 0.11f, 0.18f, 0.96f};
        t.editorBg       = {0.10f, 0.13f, 0.18f, 1.00f};
        t.editorGrid     = {0.14f, 0.18f, 0.28f, 0.70f};
        t.editorGridLine = {0.20f, 0.26f, 0.40f, 0.35f};
        t.nodeBackground = {0.12f, 0.16f, 0.24f, 0.96f};
        t.nodeBorder     = {0.22f, 0.32f, 0.55f, 0.80f};
        t.nodeSelection  = {0.16f, 0.48f, 0.88f, 0.90f};
        t.linkFlow       = {0.40f, 0.70f, 1.00f, 0.90f};
        m_themes["Blueprint"] = t;
        m_themeOrder.push_back("Blueprint");
    }

    // ── Light（浅色）────────────────────────────────────────────────────
    {
        EditorTheme t;
        t.name           = "Light";
        t.windowBg       = {0.91f, 0.91f, 0.93f, 1.00f};
        t.childBg        = {0.88f, 0.88f, 0.90f, 1.00f};
        t.frameBg        = {0.80f, 0.82f, 0.86f, 1.00f};
        t.frameHovered   = {0.72f, 0.76f, 0.84f, 1.00f};
        t.frameActive    = {0.62f, 0.68f, 0.80f, 1.00f};
        t.header         = {0.70f, 0.75f, 0.88f, 1.00f};
        t.headerHovered  = {0.62f, 0.68f, 0.82f, 1.00f};
        t.button         = {0.68f, 0.74f, 0.88f, 1.00f};
        t.buttonHovered  = {0.58f, 0.66f, 0.82f, 1.00f};
        t.text           = {0.10f, 0.10f, 0.12f, 1.00f};
        t.textDisabled   = {0.45f, 0.48f, 0.55f, 1.00f};
        t.scrollbarBg    = {0.82f, 0.83f, 0.86f, 0.80f};
        t.scrollbarGrab  = {0.58f, 0.64f, 0.78f, 1.00f};
        t.tabActive      = {0.60f, 0.70f, 0.90f, 1.00f};
        t.tabHovered     = {0.65f, 0.74f, 0.90f, 1.00f};
        t.tab            = {0.74f, 0.78f, 0.88f, 1.00f};
        t.separator      = {0.68f, 0.72f, 0.80f, 1.00f};
        t.popupBg        = {0.94f, 0.94f, 0.96f, 0.96f};
        t.editorBg       = {0.88f, 0.89f, 0.91f, 1.00f};
        t.editorGrid     = {0.76f, 0.78f, 0.84f, 0.60f};
        t.editorGridLine = {0.68f, 0.70f, 0.78f, 0.40f};
        t.nodeBackground = {0.95f, 0.95f, 0.97f, 0.96f};
        t.nodeBorder     = {0.60f, 0.65f, 0.78f, 0.80f};
        t.nodeSelection  = {0.20f, 0.50f, 0.90f, 0.70f};
        t.linkFlow       = {0.25f, 0.45f, 0.80f, 0.90f};
        m_themes["Light"] = t;
        m_themeOrder.push_back("Light");
    }
}

void ThemeManager::Apply(const std::string& themeName)
{
    auto it = m_themes.find(themeName);
    if (it == m_themes.end()) return;
    m_currentTheme = themeName;
    ApplyImGuiStyle(it->second);
    // NodeEditor style 需要 ed::SetCurrentEditor 后才能调用，
    // 延迟到 OnFrame 里通过 ApplyPendingNodeEditorStyle() 应用
    m_nodeEditorStyleDirty = true;
}

void ThemeManager::ApplyPendingNodeEditorStyle()
{
    if (!m_nodeEditorStyleDirty) return;
    auto it = m_themes.find(m_currentTheme);
    if (it == m_themes.end()) return;
    ApplyNodeEditorStyle(it->second);
    m_nodeEditorStyleDirty = false;
}

void ThemeManager::ApplyImGuiStyle(const EditorTheme& t)
{
    auto& style = ImGui::GetStyle();

    // ── VS 2022 风格几何参数 ─────────────────────────────────────────────
    style.WindowPadding     = ImVec2(8.0f, 8.0f);
    style.FramePadding      = ImVec2(6.0f, 4.0f);
    style.CellPadding       = ImVec2(4.0f, 2.0f);
    style.ItemSpacing       = ImVec2(8.0f, 4.0f);
    style.ItemInnerSpacing  = ImVec2(4.0f, 4.0f);
    style.IndentSpacing     = 18.0f;
    style.ScrollbarSize     = 12.0f;
    style.GrabMinSize       = 8.0f;

    style.WindowBorderSize  = 1.0f;
    style.ChildBorderSize   = 1.0f;
    style.PopupBorderSize   = 1.0f;
    style.FrameBorderSize   = 0.0f;   // VS 风格：输入框不加边框
    style.TabBorderSize     = 0.0f;

    style.WindowRounding    = 0.0f;   // VS 风格：窗口直角
    style.ChildRounding     = 0.0f;
    style.FrameRounding     = 2.0f;   // 微圆角输入框
    style.PopupRounding     = 2.0f;
    style.ScrollbarRounding = 2.0f;
    style.GrabRounding      = 2.0f;
    style.TabRounding       = 2.0f;   // VS 2022 标签页微圆角

    style.WindowMenuButtonPosition = ImGuiDir_None;  // 不显示窗口菜单按钮

    // ── 颜色 ─────────────────────────────────────────────────────────────
    style.Colors[ImGuiCol_WindowBg]             = t.windowBg;
    style.Colors[ImGuiCol_ChildBg]              = t.childBg;
    style.Colors[ImGuiCol_FrameBg]              = t.frameBg;
    style.Colors[ImGuiCol_FrameBgHovered]       = t.frameHovered;
    style.Colors[ImGuiCol_FrameBgActive]        = t.frameActive;
    style.Colors[ImGuiCol_Header]               = t.header;
    style.Colors[ImGuiCol_HeaderHovered]        = t.headerHovered;
    style.Colors[ImGuiCol_HeaderActive]         = ImVec4(t.headerHovered.x * 0.95f, t.headerHovered.y * 0.95f, t.headerHovered.z * 0.95f, 1.0f);
    style.Colors[ImGuiCol_Button]               = t.button;
    style.Colors[ImGuiCol_ButtonHovered]        = t.buttonHovered;
    style.Colors[ImGuiCol_ButtonActive]         = ImVec4(t.buttonHovered.x * 0.85f, t.buttonHovered.y * 0.85f, t.buttonHovered.z * 0.85f, 1.0f);
    style.Colors[ImGuiCol_Text]                 = t.text;
    style.Colors[ImGuiCol_TextDisabled]         = t.textDisabled;
    style.Colors[ImGuiCol_ScrollbarBg]          = t.scrollbarBg;
    style.Colors[ImGuiCol_ScrollbarGrab]        = t.scrollbarGrab;
    style.Colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(t.scrollbarGrab.x + 0.08f, t.scrollbarGrab.y + 0.08f, t.scrollbarGrab.z + 0.08f, 1.0f);
    style.Colors[ImGuiCol_ScrollbarGrabActive]  = t.frameActive;
    style.Colors[ImGuiCol_Tab]                  = t.tab;
    style.Colors[ImGuiCol_TabHovered]           = t.tabHovered;
    style.Colors[ImGuiCol_TabActive]            = t.tabActive;
    style.Colors[ImGuiCol_TabUnfocused]         = t.tab;
    style.Colors[ImGuiCol_TabUnfocusedActive]   = t.tabActive;
    style.Colors[ImGuiCol_Separator]            = t.separator;
    style.Colors[ImGuiCol_SeparatorHovered]     = t.headerHovered;
    style.Colors[ImGuiCol_SeparatorActive]      = t.frameActive;
    style.Colors[ImGuiCol_PopupBg]              = t.popupBg;
    style.Colors[ImGuiCol_TitleBg]              = t.header;
    style.Colors[ImGuiCol_TitleBgActive]        = t.headerHovered;
    style.Colors[ImGuiCol_TitleBgCollapsed]     = t.windowBg;
    style.Colors[ImGuiCol_MenuBarBg]            = t.childBg;
    style.Colors[ImGuiCol_CheckMark]            = ImVec4(0.40f, 0.70f, 1.00f, 1.00f);  // VS 蓝色勾选
    style.Colors[ImGuiCol_SliderGrab]           = t.scrollbarGrab;
    style.Colors[ImGuiCol_SliderGrabActive]     = t.frameActive;
    style.Colors[ImGuiCol_ResizeGrip]           = ImVec4(t.separator.x, t.separator.y, t.separator.z, 0.25f);
    style.Colors[ImGuiCol_ResizeGripHovered]    = t.headerHovered;
    style.Colors[ImGuiCol_ResizeGripActive]     = t.frameActive;
    style.Colors[ImGuiCol_Border]               = t.separator;
    style.Colors[ImGuiCol_BorderShadow]         = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
    style.Colors[ImGuiCol_NavHighlight]         = ImVec4(0.26f, 0.59f, 0.98f, 0.80f);
    style.Colors[ImGuiCol_TableHeaderBg]        = t.header;
    style.Colors[ImGuiCol_TableBorderStrong]    = t.separator;
    style.Colors[ImGuiCol_TableBorderLight]     = ImVec4(t.separator.x, t.separator.y, t.separator.z, 0.50f);
    style.Colors[ImGuiCol_TableRowBg]           = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
    style.Colors[ImGuiCol_TableRowBgAlt]        = ImVec4(1.0f, 1.0f, 1.0f, 0.03f);
    style.Colors[ImGuiCol_DragDropTarget]       = ImVec4(0.26f, 0.59f, 0.98f, 0.70f);
    style.Colors[ImGuiCol_ModalWindowDimBg]     = ImVec4(0.0f, 0.0f, 0.0f, 0.55f);
}

void ThemeManager::ApplyNodeEditorStyle(const EditorTheme& t)
{
    auto& edStyle = ed::GetStyle();
    edStyle.Colors[ed::StyleColor_Bg]             = t.editorBg;
    edStyle.Colors[ed::StyleColor_Grid]           = t.editorGrid;
    edStyle.Colors[ed::StyleColor_NodeBg]         = t.nodeBackground;
    edStyle.Colors[ed::StyleColor_NodeBorder]     = t.nodeBorder;
    edStyle.Colors[ed::StyleColor_HovNodeBorder]  = t.nodeSelection;
    edStyle.Colors[ed::StyleColor_SelNodeBorder]  = t.nodeSelection;
    edStyle.Colors[ed::StyleColor_Flow]           = t.linkFlow;
    edStyle.Colors[ed::StyleColor_FlowMarker]     = t.linkFlow;
}

const EditorTheme* ThemeManager::GetTheme(const std::string& name) const
{
    auto it = m_themes.find(name);
    return (it != m_themes.end()) ? &it->second : nullptr;
}

void ThemeManager::SaveToFile(const std::string& path)
{
    crude_json::value root = crude_json::object();
    root["theme"] = crude_json::value(m_currentTheme);

    std::ofstream f(path);
    if (f.is_open())
        f << root.dump();
}

bool ThemeManager::LoadFromFile(const std::string& path)
{
    std::ifstream f(path);
    if (!f.is_open()) return false;

    std::string content((std::istreambuf_iterator<char>(f)),
                         std::istreambuf_iterator<char>());
    crude_json::value root = crude_json::value::parse(content);
    if (root.is_null() && content.find('{') != std::string::npos) return false;

    auto& themeVal = root["theme"];
    if (themeVal.is_string())
    {
        const std::string& name = themeVal.get<std::string>();
        if (m_themes.count(name))
        {
            Apply(name);
            return true;
        }
    }
    return false;
}
