// ZoomBar.cpp -- 右下角浮动缩放条
// 显示当前缩放比例，支持拖动调整，提供 -/+ 按钮和 "100%" 快速复位
#include "BlueprintEditor.h"

// ── 常量 ─────────────────────────────────────────────────────────────────────
static constexpr float kZoomMin     = 0.25f;   // 最小缩放 25%（与 kZoomMax=4.0 关于 1.0 对数对称，使 100% 在滑轨中间）
static constexpr float kZoomMax     = 4.00f;   // 最大缩放 400%
static constexpr float kZoomDefault = 1.00f;   // 双击复位目标
// 注意：kBarW/kBarH/kBtnW/kPctW 现在在运行时根据字体大小动态计算
static constexpr float kTrackW      = 110.0f;  // 滑轨宽度（可适当固定）
static constexpr float kMarginR     = 12.0f;   // 距右边距
static constexpr float kMarginB     = 28.0f;   // 距底部（在状态栏上方）
static constexpr float kThumbR      = 5.0f;    // 滑块圆半径
static constexpr float kTrackH      = 3.0f;    // 滑轨高度

// ── 缓存的对数常量（避免每帧重复计算 log） ─────────────────────────────────
static const float kLogMin  = std::log(kZoomMin);
static const float kLogMax  = std::log(kZoomMax);
static const float kLogSpan = kLogMax - kLogMin;   // logMax - logMin

// ── 辅助：zoom → 滑轨 t（对数映射，中间=100%） ────────────────────────────
static float ZoomToT(float zoom)
{
    float logVal = std::log(ImClamp(zoom, kZoomMin, kZoomMax));
    return (logVal - kLogMin) / kLogSpan;
}

static float TToZoom(float t)
{
    return std::exp(kLogMin + t * kLogSpan);
}

// ── 核心实现 ─────────────────────────────────────────────────────────────────
// 通过 NavigateToRect(center ± halfSize / newZoom) 改变缩放
static void ApplyZoom(float newZoom, ImVec2 editorMin, ImVec2 editorMax)
{
    newZoom = ImClamp(newZoom, kZoomMin, kZoomMax);

    float  curZoom   = ed::GetCurrentZoom();
    ImVec2 screenCtr = ImVec2((editorMin.x + editorMax.x) * 0.5f,
                               (editorMin.y + editorMax.y) * 0.5f);
    // 把屏幕中心转成 canvas 坐标
    ImVec2 canvasCtr = ed::ScreenToCanvas(screenCtr);

    // 以 canvas 中心为锚点计算新 VisibleRect
    ImVec2 viewSize  = editorMax - editorMin;  // 屏幕像素尺寸
    // canvas 可视区半大小 = screen 半大小 / zoom
    ImVec2 halfCanvas = viewSize * (0.5f / newZoom);

    ImVec2 rMin = canvasCtr - halfCanvas;
    ImVec2 rMax = canvasCtr + halfCanvas;

    ed::NavigateToRectExact(rMin, rMax, 0.0f);
}

// ─────────────────────────────────────────────────────────────────────────────

void BlueprintEditor::DrawZoomBar(ImVec2 editorMin, ImVec2 editorMax)
{
    if (!ActiveDoc()) return;

    float curZoom = ed::GetCurrentZoom();

    // ── DPI 自适应尺寸 ────────────────────────────────────────────────────
    float lineH  = ImGui::GetTextLineHeight();
    float padX   = ImGui::GetStyle().FramePadding.x;
    float kBarH  = lineH + 10.0f;                          // widget 高度
    float kBtnW  = lineH + padX * 2.0f + 2.0f;            // -/+ 按钮宽
    float kPctW  = ImGui::CalcTextSize("100%").x + padX * 2.0f + 6.0f;  // 百分比区宽
    float kBarW  = kPctW + 4 + kBtnW + 4 + kTrackW + 4 + kBtnW + 8;    // 总宽

    // ── 位置：底部状态栏上方，水平居中（不与 Minimap 重叠）─────────────────
    float statusBarH = lineH + 8.0f;
    // ZoomBar 放在底部中央，若 Minimap 可见则向左偏移以避开右下角
    float minimapReserve = 0.0f;
    if (m_ShowMinimap && ActiveDoc() && !ActiveDoc()->nodes.empty())
        minimapReserve = (m_MinimapSize + 20.0f) * 0.5f;  // 微调，使 ZoomBar 向左偏
    float centerX = (editorMin.x + editorMax.x) * 0.5f - minimapReserve;
    ImVec2 wPos = ImVec2(centerX - kBarW * 0.5f,
                          editorMax.y - kBarH - statusBarH - 4.0f);

    // 使用独立浮动窗口，确保 InvisibleButton 的鼠标事件不被 editor 子窗口拦截
    ImGui::SetNextWindowPos(wPos, ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(kBarW, kBarH));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding,  ImVec2(0, 0));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowMinSize,  ImVec2(kBarW, kBarH));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 5.0f);
    ImGui::PushStyleColor(ImGuiCol_WindowBg,   ImVec4(22/255.f, 25/255.f, 35/255.f, 0.82f));
    ImGui::PushStyleColor(ImGuiCol_Border,     ImVec4(60/255.f, 75/255.f, 110/255.f, 0.55f));
    bool open = ImGui::Begin("##ZoomBarWnd", nullptr,
        ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse |
        ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoDocking |
        ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav);
    ImGui::PopStyleColor(2);
    ImGui::PopStyleVar(3);

    if (!open) { ImGui::End(); return; }

    // 从现在起绘图坐标相对于此窗口（左上角 = wPos）
    auto* dl = ImGui::GetWindowDrawList();
    ImVec2 wp  = ImGui::GetWindowPos();   // 等于 wPos
    float  cx  = 4.0f;                    // 窗口内 x 偏移
    float  cy  = kBarH * 0.5f;            // 窗口内 y 中线

    // ── 百分比文本（双击复位到 100%） ────────────────────────────────────
    char pctBuf[16];
    snprintf(pctBuf, sizeof(pctBuf), "%.0f%%", curZoom * 100.0f);

    ImVec2 pctMin(cx, 1.0f);
    ImVec2 pctMax(cx + kPctW, kBarH - 1.0f);
    ImGui::SetCursorPos(pctMin);
    ImGui::InvisibleButton("##zoom_pct", pctMax - pctMin);
    if (ImGui::IsItemHovered())
    {
        ImGui::SetTooltip("Double-click to reset to 100%%\nCurrent: %.0f%%", curZoom * 100.0f);
        dl->AddRectFilled(wp + pctMin, wp + pctMax, IM_COL32(60, 80, 120, 80), 3.0f);
    }
    if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0))
        ApplyZoom(kZoomDefault, editorMin, editorMax);

    ImU32 pctCol = IM_COL32(160, 185, 220, 220);
    if (curZoom < 0.3f)      pctCol = IM_COL32(220, 140, 80, 230);
    else if (curZoom > 2.0f) pctCol = IM_COL32(120, 210, 120, 230);
    ImVec2 pctTextSz = ImGui::CalcTextSize(pctBuf);
    dl->AddText(wp + ImVec2(cx + (kPctW - pctTextSz.x) * 0.5f, cy - pctTextSz.y * 0.5f), pctCol, pctBuf);

    cx += kPctW + 4.0f;

    // ── 减号按钮 ─────────────────────────────────────────────────────────
    ImVec2 btnMinusMin(cx, 2.0f);
    ImVec2 btnMinusMax(cx + kBtnW, kBarH - 2.0f);
    ImGui::SetCursorPos(btnMinusMin);
    ImGui::InvisibleButton("##zoom_minus", btnMinusMax - btnMinusMin);
    bool minusHov = ImGui::IsItemHovered();
    bool minusClk = ImGui::IsItemClicked();
    dl->AddRectFilled(wp + btnMinusMin, wp + btnMinusMax,
                      minusHov ? IM_COL32(80, 100, 150, 200) : IM_COL32(50, 60, 90, 120), 3.0f);
    ImVec2 mTxtSz = ImGui::CalcTextSize("-");
    dl->AddText(wp + ImVec2(cx + (kBtnW - mTxtSz.x) * 0.5f, cy - mTxtSz.y * 0.5f),
                IM_COL32(200, 210, 240, 230), "-");
    if (minusClk)
        ApplyZoom(curZoom / 1.25f, editorMin, editorMax);

    cx += kBtnW + 4.0f;

    // ── 滑轨 ─────────────────────────────────────────────────────────────
    float trackLeft  = cx;
    float trackRight = trackLeft + kTrackW;
    float trackY     = cy;

    dl->AddRectFilled(wp + ImVec2(trackLeft, trackY - kTrackH * 0.5f),
                      wp + ImVec2(trackRight, trackY + kTrackH * 0.5f),
                      IM_COL32(45, 55, 80, 200), kTrackH * 0.5f);

    float t      = ZoomToT(curZoom);
    float thumbX = trackLeft + t * kTrackW;
    if (thumbX > trackLeft)
    {
        dl->AddRectFilled(wp + ImVec2(trackLeft, trackY - kTrackH * 0.5f),
                          wp + ImVec2(thumbX,    trackY + kTrackH * 0.5f),
                          IM_COL32(80, 130, 220, 200), kTrackH * 0.5f);
    }

    float midX = trackLeft + ZoomToT(1.0f) * kTrackW;
    dl->AddLine(wp + ImVec2(midX, trackY - 5.0f), wp + ImVec2(midX, trackY + 5.0f),
                IM_COL32(120, 160, 220, 120), 1.0f);

    ImU32 thumbCol = IM_COL32(130, 175, 255, 255);

    ImVec2 trackHitMin(trackLeft - 4.0f, 2.0f);
    ImVec2 trackHitMax(trackRight + 4.0f, kBarH - 2.0f);
    ImGui::SetCursorPos(trackHitMin);
    ImGui::InvisibleButton("##zoom_track", trackHitMax - trackHitMin);

    bool trackHov = ImGui::IsItemHovered();
    bool trackAct = ImGui::IsItemActive();

    if (trackHov || trackAct)
    {
        thumbCol = IM_COL32(180, 210, 255, 255);
        ImGui::SetTooltip("Drag to adjust zoom\n%.0f%%  (%.2fx)", curZoom * 100.0f, curZoom);
    }
    if (trackAct && ImGui::IsMouseDown(0))
    {
        float mx   = ImGui::GetMousePos().x - wp.x;  // 转为窗口内坐标
        float newT = ImClamp((mx - trackLeft) / kTrackW, 0.0f, 1.0f);
        ApplyZoom(TToZoom(newT), editorMin, editorMax);
    }

    dl->AddCircleFilled(wp + ImVec2(thumbX, trackY), kThumbR, thumbCol);
    dl->AddCircleFilled(wp + ImVec2(thumbX, trackY), kThumbR * 0.45f, IM_COL32(220, 235, 255, 200));

    cx = trackRight + 4.0f;

    // ── 加号按钮 ─────────────────────────────────────────────────────────
    ImVec2 btnPlusMin(cx, 2.0f);
    ImVec2 btnPlusMax(cx + kBtnW, kBarH - 2.0f);
    ImGui::SetCursorPos(btnPlusMin);
    ImGui::InvisibleButton("##zoom_plus", btnPlusMax - btnPlusMin);
    bool plusHov = ImGui::IsItemHovered();
    bool plusClk = ImGui::IsItemClicked();
    dl->AddRectFilled(wp + btnPlusMin, wp + btnPlusMax,
                      plusHov ? IM_COL32(80, 100, 150, 200) : IM_COL32(50, 60, 90, 120), 3.0f);
    ImVec2 pTxtSz = ImGui::CalcTextSize("+");
    dl->AddText(wp + ImVec2(cx + (kBtnW - pTxtSz.x) * 0.5f, cy - pTxtSz.y * 0.5f),
                IM_COL32(200, 210, 240, 230), "+");
    if (plusClk)
        ApplyZoom(curZoom * 1.25f, editorMin, editorMax);

    ImGui::End();
}
