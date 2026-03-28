# include "application.h"
# include "setup.h"
# include "platform.h"
# include "renderer.h"

#ifdef _WIN32
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shlwapi.h>
#pragma comment(lib, "shlwapi.lib")
#endif

extern "C" {
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_STATIC
#include "stb_image.h"
}

// 确保工作目录为 exe 所在目录，这样 "data/xxx" 等相对路径能正确找到资源文件
static void EnsureWorkingDirectoryIsExeDir()
{
#ifdef _WIN32
    wchar_t exePath[MAX_PATH] = {};
    GetModuleFileNameW(NULL, exePath, MAX_PATH);
    PathRemoveFileSpecW(exePath);
    SetCurrentDirectoryW(exePath);
#endif
}

Application::Application(const char* name)
    : Application(name, 0, nullptr)
{
}

Application::Application(const char* name, int argc, char** argv)
    : m_Name(name)
    , m_Platform(CreatePlatform(*this))
    , m_Renderer(CreateRenderer())
{
    EnsureWorkingDirectoryIsExeDir();
    m_Platform->ApplicationStart(argc, argv);
}

Application::~Application()
{
    m_Renderer->Destroy();

    m_Platform->ApplicationStop();

    if (m_Context)
    {
        ImGui::DestroyContext(m_Context);
        m_Context= nullptr;
    }
}

bool Application::Create(int width /*= -1*/, int height /*= -1*/)
{
    m_Context = ImGui::CreateContext();
    ImGui::SetCurrentContext(m_Context);

    if (!m_Platform->OpenMainWindow("Application", width, height))
        return false;

    if (!m_Renderer->Create(*m_Platform))
        return false;

    m_IniFilename = m_Name + ".ini";

    ImGuiIO& io = ImGui::GetIO();
    //io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;  // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;        // Enable Docking (drag windows into tabs)
    io.IniFilename = m_IniFilename.c_str();
    io.LogFilename = nullptr;

    // ================================================================
    // 自定义现代暗色主题（替代默认 StyleColorsDark）
    // ================================================================
    {
        ImGui::StyleColorsDark();
        auto& style = ImGui::GetStyle();
        auto& colors = style.Colors;

        // ---- 基底色板 ----
        // bg0: 最深背景 (主窗口)    bg1: 次级背景 (面板)    bg2: 控件背景
        // accent: 强调蓝 (#4B8BBE)   accentDim: 暗调蓝       surface: 悬浮层
        const ImVec4 bg0       (0.098f, 0.098f, 0.118f, 1.00f);  // #191920
        const ImVec4 bg1       (0.118f, 0.122f, 0.149f, 1.00f);  // #1E1F26
        const ImVec4 bg2       (0.153f, 0.157f, 0.192f, 1.00f);  // #272831
        const ImVec4 surface   (0.176f, 0.180f, 0.220f, 1.00f);  // #2D2E38
        const ImVec4 border    (0.220f, 0.228f, 0.278f, 0.50f);  // subtle edge
        const ImVec4 accent    (0.294f, 0.545f, 0.745f, 1.00f);  // #4B8BBE
        const ImVec4 accentDim (0.220f, 0.400f, 0.600f, 1.00f);
        const ImVec4 accentLit (0.380f, 0.640f, 0.900f, 1.00f);
        const ImVec4 textPri   (0.906f, 0.914f, 0.945f, 1.00f);  // #E7E9F1
        const ImVec4 textSec   (0.550f, 0.565f, 0.620f, 1.00f);  // #8C9090
        const ImVec4 green     (0.310f, 0.720f, 0.440f, 1.00f);  // 成功/执行

        // 窗口 & 背景
        colors[ImGuiCol_WindowBg]               = bg0;
        colors[ImGuiCol_ChildBg]                = ImVec4(0.106f, 0.110f, 0.133f, 1.00f);
        colors[ImGuiCol_PopupBg]                = ImVec4(0.137f, 0.141f, 0.173f, 0.97f);

        // 边框 — 极细微的分隔感
        colors[ImGuiCol_Border]                 = border;
        colors[ImGuiCol_BorderShadow]           = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);

        // 帧（输入框、下拉框等背景）
        colors[ImGuiCol_FrameBg]                = bg2;
        colors[ImGuiCol_FrameBgHovered]         = surface;
        colors[ImGuiCol_FrameBgActive]          = ImVec4(0.200f, 0.210f, 0.260f, 1.00f);

        // 标题栏
        colors[ImGuiCol_TitleBg]                = ImVec4(0.078f, 0.078f, 0.098f, 1.00f);
        colors[ImGuiCol_TitleBgActive]          = ImVec4(0.110f, 0.114f, 0.145f, 1.00f);
        colors[ImGuiCol_TitleBgCollapsed]       = ImVec4(0.078f, 0.078f, 0.098f, 0.75f);

        // 菜单栏 — 略深于窗口背景，形成层次
        colors[ImGuiCol_MenuBarBg]              = ImVec4(0.110f, 0.114f, 0.141f, 1.00f);

        // 滚动条 — 纤细、低调
        colors[ImGuiCol_ScrollbarBg]            = ImVec4(0.098f, 0.098f, 0.118f, 0.60f);
        colors[ImGuiCol_ScrollbarGrab]          = ImVec4(0.280f, 0.290f, 0.340f, 0.80f);
        colors[ImGuiCol_ScrollbarGrabHovered]   = ImVec4(0.380f, 0.400f, 0.460f, 0.90f);
        colors[ImGuiCol_ScrollbarGrabActive]    = ImVec4(0.480f, 0.500f, 0.560f, 1.00f);

        // 选中 & 交互（accent 蓝）
        colors[ImGuiCol_CheckMark]              = accentLit;
        colors[ImGuiCol_SliderGrab]             = accent;
        colors[ImGuiCol_SliderGrabActive]       = accentLit;

        // 按钮 — accent 蓝调，hover 时提亮
        colors[ImGuiCol_Button]                 = ImVec4(0.200f, 0.340f, 0.520f, 0.80f);
        colors[ImGuiCol_ButtonHovered]          = ImVec4(0.260f, 0.440f, 0.660f, 0.95f);
        colors[ImGuiCol_ButtonActive]           = ImVec4(0.300f, 0.500f, 0.740f, 1.00f);

        // 头部（TreeNode, Collapsing Header, Table Header）
        colors[ImGuiCol_Header]                 = ImVec4(0.180f, 0.190f, 0.240f, 1.00f);
        colors[ImGuiCol_HeaderHovered]          = ImVec4(0.240f, 0.260f, 0.340f, 1.00f);
        colors[ImGuiCol_HeaderActive]           = ImVec4(0.220f, 0.360f, 0.540f, 1.00f);

        // 分隔线 — 低调
        colors[ImGuiCol_Separator]              = ImVec4(0.200f, 0.210f, 0.260f, 0.70f);
        colors[ImGuiCol_SeparatorHovered]       = accent;
        colors[ImGuiCol_SeparatorActive]        = accentLit;

        // 调整大小手柄
        colors[ImGuiCol_ResizeGrip]             = ImVec4(accent.x, accent.y, accent.z, 0.15f);
        colors[ImGuiCol_ResizeGripHovered]      = ImVec4(accent.x, accent.y, accent.z, 0.55f);
        colors[ImGuiCol_ResizeGripActive]       = ImVec4(accent.x, accent.y, accent.z, 0.90f);

        // 标签栏 — 活跃标签用强调色底部指示条风格
        colors[ImGuiCol_Tab]                    = ImVec4(0.130f, 0.136f, 0.168f, 1.00f);
        colors[ImGuiCol_TabHovered]             = ImVec4(0.200f, 0.340f, 0.520f, 0.85f);
        colors[ImGuiCol_TabSelected]            = ImVec4(0.180f, 0.300f, 0.480f, 1.00f);
        colors[ImGuiCol_TabDimmed]              = ImVec4(0.100f, 0.104f, 0.130f, 0.97f);
        colors[ImGuiCol_TabDimmedSelected]      = ImVec4(0.160f, 0.260f, 0.420f, 1.00f);

        // 文本选中
        colors[ImGuiCol_TextSelectedBg]         = ImVec4(accent.x, accent.y, accent.z, 0.30f);

        // 拖拽指示
        colors[ImGuiCol_DragDropTarget]         = ImVec4(accentLit.x, accentLit.y, accentLit.z, 0.90f);

        // 导航高亮
        colors[ImGuiCol_NavHighlight]           = accent;

        // 表格
        colors[ImGuiCol_TableHeaderBg]          = ImVec4(0.140f, 0.148f, 0.185f, 1.00f);
        colors[ImGuiCol_TableBorderStrong]      = ImVec4(0.200f, 0.210f, 0.260f, 0.70f);
        colors[ImGuiCol_TableBorderLight]       = ImVec4(0.170f, 0.175f, 0.215f, 0.50f);
        colors[ImGuiCol_TableRowBg]             = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
        colors[ImGuiCol_TableRowBgAlt]          = ImVec4(0.15f, 0.155f, 0.190f, 0.30f);

        // 文字颜色
        colors[ImGuiCol_Text]                   = textPri;
        colors[ImGuiCol_TextDisabled]           = textSec;

        // 圆角 — 统一 5px 基准，更现代
        style.WindowRounding    = 6.0f;
        style.FrameRounding     = 4.0f;
        style.PopupRounding     = 6.0f;
        style.ScrollbarRounding = 8.0f;
        style.GrabRounding      = 4.0f;
        style.TabRounding       = 5.0f;
        style.ChildRounding     = 4.0f;

        // 间距 — 略微紧凑
        style.WindowPadding     = ImVec2(10, 10);
        style.FramePadding      = ImVec2(8, 4);
        style.ItemSpacing       = ImVec2(8, 5);
        style.ItemInnerSpacing  = ImVec2(6, 4);
        style.IndentSpacing     = 20.0f;
        style.ScrollbarSize     = 12.0f;
        style.GrabMinSize       = 10.0f;

        // 边框 — 仅在必要处使用
        style.WindowBorderSize  = 1.0f;
        style.ChildBorderSize   = 1.0f;
        style.FrameBorderSize   = 0.0f;
        style.PopupBorderSize   = 1.0f;
        style.TabBorderSize     = 0.0f;

        // 对齐
        style.TabBarBorderSize  = 1.0f;
        style.SeparatorTextBorderSize = 2.0f;
    }

    RecreateFontAtlas();

    m_Platform->AcknowledgeWindowScaleChanged();
    m_Platform->AcknowledgeFramebufferScaleChanged();

    OnStart();

    Frame();

    return true;
}

int Application::Run()
{
    m_Platform->ShowMainWindow();

    while (m_Platform->ProcessMainWindowEvents())
    {
        if (!m_Platform->IsMainWindowVisible())
            continue;

        Frame();
    }

    OnStop();

    return 0;
}

void Application::RecreateFontAtlas()
{
    ImGuiIO& io = ImGui::GetIO();

    // In newer ImGui (1.92+), the font atlas is registered with the context.
    // We must NOT delete and recreate it. Instead, clear and re-add fonts.
    io.Fonts->Clear();

    // ── DPI 感知：根据平台 ContentScale 缩放字体大小 ──────────────────────
    // Windows/高DPI笔记本：glfwGetWindowContentScale 返回 1.25/1.5/2.0 等
    // macOS Retina：framebuffer scale = 2.0，window scale = 1.0（系统已处理）
    // Linux：通常 1.0
    float dpiScale = m_Platform ? m_Platform->GetWindowScale() : 1.0f;
    if (dpiScale < 0.5f) dpiScale = 1.0f;   // 安全下限

    const float baseFontSize   = 18.0f;
    const float headerFontSize = 20.0f;
    const float iconFontSize   = 22.0f;
    const float fontSize   = std::round(baseFontSize   * dpiScale);
    const float hFontSize  = std::round(headerFontSize * dpiScale);
    const float iFontSize  = std::round(iconFontSize   * dpiScale);

    // 同步 ImGui Style 缩放（每次 DPI 变化时重新应用）
    ImGui::GetStyle().ScaleAllSizes(dpiScale / (m_LastDpiScale > 0.f ? m_LastDpiScale : 1.0f));
    m_LastDpiScale = dpiScale;

    ImFontConfig config;
    config.OversampleH = 4;
    config.OversampleV = 4;
    config.PixelSnapH = false;

    // 1. 加载默认字体（Play-Regular）
    m_DefaultFont = io.Fonts->AddFontFromFileTTF("data/Play-Regular.ttf", fontSize, &config);

    // 2. 合并 FontAwesome 6 图标字体到默认字体
    {
        static const ImWchar icon_ranges[] = { 0xe005, 0xf8ff, 0 };
        ImFontConfig iconConfig;
        iconConfig.MergeMode = true;
        iconConfig.PixelSnapH = true;
        iconConfig.GlyphMinAdvanceX = fontSize;
        iconConfig.GlyphOffset.y = 2.0f;
        io.Fonts->AddFontFromFileTTF("data/fa-solid-900.ttf", fontSize - 2.0f, &iconConfig, icon_ranges);
    }

    // 3. 加载标题字体（Cuprum-Bold）
    m_HeaderFont = io.Fonts->AddFontFromFileTTF("data/Cuprum-Bold.ttf", hFontSize, &config);

    // 4. 合并 FontAwesome 到标题字体
    {
        static const ImWchar icon_ranges[] = { 0xe005, 0xf8ff, 0 };
        ImFontConfig iconConfig;
        iconConfig.MergeMode = true;
        iconConfig.PixelSnapH = true;
        iconConfig.GlyphMinAdvanceX = hFontSize;
        iconConfig.GlyphOffset.y = 2.0f;
        io.Fonts->AddFontFromFileTTF("data/fa-solid-900.ttf", hFontSize - 2.0f, &iconConfig, icon_ranges);
    }

    // 5. 独立图标字体（大号，用于面板标题图标等）
    {
        static const ImWchar icon_ranges[] = { 0xe005, 0xf8ff, 0 };
        ImFontConfig iconOnlyConfig;
        iconOnlyConfig.OversampleH = 2;
        iconOnlyConfig.OversampleV = 2;
        iconOnlyConfig.GlyphMinAdvanceX = iFontSize;
        m_IconFont = io.Fonts->AddFontFromFileTTF("data/fa-solid-900.ttf", iFontSize, &iconOnlyConfig, icon_ranges);
    }

    io.Fonts->Build();
}

void Application::Frame()
{
    auto& io = ImGui::GetIO();

    if (m_Platform->HasWindowScaleChanged())
    {
        // 窗口移到新 DPI 屏幕（或系统缩放改变），需要重建字体 atlas
        RecreateFontAtlas();
        m_Platform->AcknowledgeWindowScaleChanged();
    }

    if (m_Platform->HasFramebufferScaleChanged())
    {
        RecreateFontAtlas();
        m_Platform->AcknowledgeFramebufferScaleChanged();
    }

    // In newer ImGui (1.92+), the Win32 backend handles DPI scaling,
    // display size, and mouse coordinates correctly via the input event
    // queue (AddMousePosEvent). We must NOT manually rescale io.MousePos
    // or io.DisplaySize here — doing so causes coordinate mismatches
    // that lead to flickering, incorrect dragging, and broken child windows.

    m_Platform->NewFrame();

    m_Renderer->NewFrame();

    ImGui::NewFrame();

    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(io.DisplaySize);
    const auto windowBorderSize = ImGui::GetStyle().WindowBorderSize;
    const auto windowRounding   = ImGui::GetStyle().WindowRounding;
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::Begin("Content", nullptr, GetWindowFlags());
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, windowBorderSize);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, windowRounding);

    OnFrame(io.DeltaTime);

    ImGui::PopStyleVar(2);
    ImGui::End();
    ImGui::PopStyleVar(2);

    // Rendering
    m_Renderer->Clear(ImColor(32, 32, 32, 255));
    ImGui::Render();
    m_Renderer->RenderDrawData(ImGui::GetDrawData());

    m_Platform->FinishFrame();
}

void Application::SetTitle(const char* title)
{
    m_Platform->SetMainWindowTitle(title);
}

bool Application::Close()
{
    return m_Platform->CloseMainWindow();
}

void Application::Quit()
{
    m_Platform->Quit();
}

const std::string& Application::GetName() const
{
    return m_Name;
}

ImFont* Application::DefaultFont() const
{
    return m_DefaultFont;
}

ImFont* Application::HeaderFont() const
{
    return m_HeaderFont;
}

ImFont* Application::IconFont() const
{
    return m_IconFont;
}

ImTextureID Application::LoadTexture(const char* path)
{
    int width = 0, height = 0, component = 0;
    if (auto data = stbi_load(path, &width, &height, &component, 4))
    {
        auto texture = CreateTexture(data, width, height);
        stbi_image_free(data);
        return texture;
    }
    else
        return ImTextureID_Invalid;
}

ImTextureID Application::CreateTexture(const void* data, int width, int height)
{
    return m_Renderer->CreateTexture(data, width, height);
}

void Application::DestroyTexture(ImTextureID texture)
{
    m_Renderer->DestroyTexture(texture);
}

int Application::GetTextureWidth(ImTextureID texture)
{
    return m_Renderer->GetTextureWidth(texture);
}

int Application::GetTextureHeight(ImTextureID texture)
{
    return m_Renderer->GetTextureHeight(texture);
}

ImGuiWindowFlags Application::GetWindowFlags() const
{
    return
        ImGuiWindowFlags_NoTitleBar |
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoScrollbar |
        ImGuiWindowFlags_NoScrollWithMouse |
        ImGuiWindowFlags_NoSavedSettings |
        ImGuiWindowFlags_NoBringToFrontOnFocus;
}
