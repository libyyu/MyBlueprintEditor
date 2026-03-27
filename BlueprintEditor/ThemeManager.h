// BlueprintEditor/ThemeManager.h -- 主题管理器
// 支持 Dark / Blueprint / Light 三套内置主题，可通过 JSON 持久化
#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include <imgui.h>
#include <imgui_node_editor.h>

namespace ax { namespace NodeEditor { struct Style; } }

struct EditorTheme
{
    std::string name;

    // ImGui 窗口/面板颜色
    ImVec4 windowBg         = {0.13f, 0.14f, 0.18f, 1.00f};
    ImVec4 childBg          = {0.11f, 0.12f, 0.15f, 1.00f};
    ImVec4 frameBg          = {0.20f, 0.22f, 0.28f, 1.00f};
    ImVec4 frameHovered     = {0.28f, 0.32f, 0.42f, 1.00f};
    ImVec4 frameActive      = {0.34f, 0.40f, 0.55f, 1.00f};
    ImVec4 header           = {0.25f, 0.30f, 0.42f, 1.00f};
    ImVec4 headerHovered    = {0.32f, 0.38f, 0.55f, 1.00f};
    ImVec4 button           = {0.25f, 0.30f, 0.45f, 1.00f};
    ImVec4 buttonHovered    = {0.34f, 0.42f, 0.60f, 1.00f};
    ImVec4 text             = {0.85f, 0.87f, 0.92f, 1.00f};
    ImVec4 textDisabled     = {0.40f, 0.44f, 0.52f, 1.00f};
    ImVec4 scrollbarBg      = {0.08f, 0.09f, 0.12f, 0.80f};
    ImVec4 scrollbarGrab    = {0.30f, 0.35f, 0.48f, 1.00f};
    ImVec4 tabActive        = {0.32f, 0.42f, 0.65f, 1.00f};
    ImVec4 tabHovered       = {0.28f, 0.36f, 0.55f, 1.00f};
    ImVec4 tab              = {0.18f, 0.22f, 0.32f, 1.00f};
    ImVec4 separator        = {0.25f, 0.30f, 0.42f, 1.00f};
    ImVec4 popupBg          = {0.12f, 0.14f, 0.18f, 0.96f};

    // 节点编辑器颜色
    ImVec4 editorBg         = {0.13f, 0.14f, 0.18f, 1.00f};
    ImVec4 editorGrid       = {0.20f, 0.22f, 0.28f, 0.60f};
    ImVec4 editorGridLine   = {0.28f, 0.30f, 0.38f, 0.30f};
    ImVec4 nodeBackground   = {0.17f, 0.19f, 0.25f, 0.96f};
    ImVec4 nodeBorder       = {0.30f, 0.34f, 0.46f, 0.80f};
    ImVec4 nodeSelection    = {0.26f, 0.59f, 0.98f, 0.80f};
    ImVec4 linkFlow         = {0.90f, 0.92f, 1.00f, 0.80f};
};

class ThemeManager
{
public:
    static ThemeManager& Get();

    void Apply(const std::string& themeName);
    void ApplyImGuiStyle(const EditorTheme& theme);
    void ApplyNodeEditorStyle(const EditorTheme& theme);

    const std::string& GetCurrentTheme() const { return m_currentTheme; }
    const std::vector<std::string>& GetThemeNames() const { return m_themeOrder; }

    void SaveToFile(const std::string& path);
    bool LoadFromFile(const std::string& path);

    const EditorTheme* GetTheme(const std::string& name) const;

private:
    ThemeManager();
    void RegisterBuiltinThemes();

    std::string m_currentTheme = "Dark";
    std::unordered_map<std::string, EditorTheme> m_themes;
    std::vector<std::string> m_themeOrder;
};
