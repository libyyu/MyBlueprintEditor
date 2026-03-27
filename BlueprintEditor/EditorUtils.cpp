// EditorUtils.cpp -- 编辑器辅助函数实现
// 从 BlueprintEditor.h 迁出，避免 static 函数在每个 TU 中生成独立副本
#include "BlueprintEditor.h"

// ============================================================================
// 分割条
// ============================================================================

bool Splitter(const char* str_id, bool split_vertically, float thickness, float* size1, float* size2, float min_size1, float min_size2, float splitter_long_axis_size)
{
    using namespace ImGui;
    ImGuiContext& g = *GImGui;
    ImGuiWindow* window = g.CurrentWindow;
    ImGuiID id = window->GetID(str_id);
    ImRect bb;
    bb.Min = window->DC.CursorPos + (split_vertically ? ImVec2(*size1, 0.0f) : ImVec2(0.0f, *size1));
    bb.Max = bb.Min + CalcItemSize(split_vertically ? ImVec2(thickness, splitter_long_axis_size) : ImVec2(splitter_long_axis_size, thickness), 0.0f, 0.0f);
    bool result = SplitterBehavior(bb, id, split_vertically ? ImGuiAxis_X : ImGuiAxis_Y, size1, size2, min_size1, min_size2, 4.0f);

    // 视觉反馈：悬停/拖动时绘制强调色线条
    bool hovered = g.HoveredId == id;
    bool active  = g.ActiveId  == id;
    if (hovered || active)
    {
        ImU32 lineCol = active  ? IM_COL32(75, 140, 190, 220)
                                : IM_COL32(60, 110, 160, 140);
        auto* dl = GetWindowDrawList();
        if (split_vertically)
        {
            float cx = (bb.Min.x + bb.Max.x) * 0.5f;
            dl->AddLine(ImVec2(cx, bb.Min.y), ImVec2(cx, bb.Max.y), lineCol, 2.0f);
        }
        else
        {
            float cy = (bb.Min.y + bb.Max.y) * 0.5f;
            dl->AddLine(ImVec2(bb.Min.x, cy), ImVec2(bb.Max.x, cy), lineCol, 2.0f);
        }
    }
    return result;
}

// ============================================================================
// 彩色日志行渲染
// ============================================================================

void DrawColoredLogLine(const std::string& line)
{
    static const ImVec4 colError   (0.95f, 0.32f, 0.32f, 1.00f);
    static const ImVec4 colWarn    (0.95f, 0.72f, 0.28f, 1.00f);
    static const ImVec4 colInfo    (0.38f, 0.68f, 0.95f, 1.00f);
    static const ImVec4 colSuccess (0.35f, 0.88f, 0.42f, 1.00f);
    static const ImVec4 colSeparator(0.45f, 0.48f, 0.56f, 0.70f);
    static const ImVec4 colDim     (0.50f, 0.52f, 0.58f, 0.90f);
    static const ImVec4 colDefault (0.82f, 0.84f, 0.90f, 1.00f);

    const ImVec4* color = &colDefault;

    if (line.find("[ERROR]") != std::string::npos)
        color = &colError;
    else if (line.find("[WARN]") != std::string::npos)
        color = &colWarn;
    else if (line.find("[INFO]") != std::string::npos || line.find("[Timer:") != std::string::npos)
        color = &colInfo;
    else if (line.find("Completed Successfully") != std::string::npos)
        color = &colSuccess;
    else if (line.find("FAILED") != std::string::npos || line.find("ABORTED") != std::string::npos)
        color = &colError;
    else if (line.find("========") != std::string::npos)
        color = &colSeparator;
    else if (line.find("Nodes:") == 0 || line.find("Links:") == 0 ||
             line.find("Validation:") == 0 || line.find("Elapsed:") != std::string::npos)
        color = &colDim;

    ImGui::PushStyleColor(ImGuiCol_Text, *color);
    ImGui::TextUnformatted(line.c_str());
    ImGui::PopStyleColor();
}
