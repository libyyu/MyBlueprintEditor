// ZoomBar.cpp -- 右下角浮动缩放条
// 显示当前缩放比例，支持拖动调整，提供 -/+ 按钮和 "100%" 快速复位
#include "BlueprintEditor.h"

// ── 常量 ─────────────────────────────────────────────────────────────────────
static constexpr float kZoomMin     = 0.10f;   // 最小缩放 10%
static constexpr float kZoomMax     = 4.00f;   // 最大缩放 400%
static constexpr float kZoomDefault = 1.00f;   // 双击复位目标
static constexpr float kBarW        = 220.0f;  // 整个 widget 宽度
static constexpr float kBarH        = 26.0f;   // widget 高度
static constexpr float kTrackW      = 110.0f;  // 滑轨宽度
static constexpr float kBtnW        = 22.0f;   // -/+ 按钮宽
static constexpr float kPctW        = 46.0f;   // 百分比文字区宽
static constexpr float kMarginR     = 12.0f;   // 距右边距
static constexpr float kMarginB     = 28.0f;   // 距底部（在状态栏上方）
static constexpr float kThumbR      = 5.0f;    // 滑块圆半径
static constexpr float kTrackH      = 3.0f;    // 滑轨高度

// ── 辅助：zoom → 滑轨 t（对数映射，中间=100%） ────────────────────────────
static float ZoomToT(float zoom)
{
    // log 映射让 100% 在中间，小值段和大值段各占一半
    float logMin = std::log(kZoomMin);
    float logMax = std::log(kZoomMax);
    float logVal = std::log(ImClamp(zoom, kZoomMin, kZoomMax));
    return (logVal - logMin) / (logMax - logMin);
}

static float TToZoom(float t)
{
    float logMin = std::log(kZoomMin);
    float logMax = std::log(kZoomMax);
    return std::exp(logMin + t * (logMax - logMin));
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

    ed::NavigateToRect(rMin, rMax, false, 0.0f);
}

// ─────────────────────────────────────────────────────────────────────────────

void BlueprintEditor::DrawZoomBar(ImVec2 editorMin, ImVec2 editorMax)
{
    if (!ActiveDoc()) return;

    float curZoom = ed::GetCurrentZoom();

    // ── 位置：右下角，状态栏上方 ─────────────────────────────────────────
    ImVec2 wPos = ImVec2(editorMax.x - kBarW - kMarginR,
                          editorMax.y - kBarH - kMarginB);
    ImVec2 wMax = ImVec2(wPos.x + kBarW, wPos.y + kBarH);

    auto* dl = ImGui::GetWindowDrawList();

    // ── 背景 ─────────────────────────────────────────────────────────────
    dl->AddRectFilled(wPos, wMax, IM_COL32(22, 25, 35, 210), 5.0f);
    dl->AddRect(wPos, wMax, IM_COL32(60, 75, 110, 140), 5.0f);

    float cx = wPos.x + 5.0f;
    float cy = wPos.y + kBarH * 0.5f;

    // ── 百分比文本（左侧固定宽度，双击复位到 100%） ─────────────────────
    char pctBuf[16];
    snprintf(pctBuf, sizeof(pctBuf), "%.0f%%", curZoom * 100.0f);

    ImVec2 pctMin(cx, wPos.y + 1.0f);
    ImVec2 pctMax(cx + kPctW, wMax.y - 1.0f);

    ImGui::SetCursorScreenPos(pctMin);
    ImGui::InvisibleButton("##zoom_pct", pctMax - pctMin);
    if (ImGui::IsItemHovered())
    {
        ImGui::SetTooltip("双击复位到 100%%\n当前: %.0f%%", curZoom * 100.0f);
        dl->AddRectFilled(pctMin, pctMax, IM_COL32(60, 80, 120, 80), 3.0f);
    }
    if (ImGui::IsItemActive() && ImGui::IsMouseDoubleClicked(0))
        ApplyZoom(kZoomDefault, editorMin, editorMax);

    // 颜色跟状态栏保持一致
    ImU32 pctCol = IM_COL32(160, 185, 220, 220);
    if (curZoom < 0.3f)      pctCol = IM_COL32(220, 140, 80, 230);
    else if (curZoom > 2.0f) pctCol = IM_COL32(120, 210, 120, 230);

    ImVec2 pctTextSz = ImGui::CalcTextSize(pctBuf);
    dl->AddText(ImVec2(pctMin.x + (kPctW - pctTextSz.x) * 0.5f,
                       cy - pctTextSz.y * 0.5f),
                pctCol, pctBuf);

    cx += kPctW + 4.0f;

    // ── 减号按钮 ─────────────────────────────────────────────────────────
    ImVec2 btnMinusMin(cx, wPos.y + 2.0f);
    ImVec2 btnMinusMax(cx + kBtnW, wMax.y - 2.0f);
    ImGui::SetCursorScreenPos(btnMinusMin);
    ImGui::InvisibleButton("##zoom_minus", btnMinusMax - btnMinusMin);
    bool minusHov = ImGui::IsItemHovered();
    bool minusClk = ImGui::IsItemClicked();
    dl->AddRectFilled(btnMinusMin, btnMinusMax,
                      minusHov ? IM_COL32(80, 100, 150, 160) : IM_COL32(50, 60, 90, 120), 3.0f);
    ImVec2 minusTxtSz = ImGui::CalcTextSize("-");
    dl->AddText(ImVec2(btnMinusMin.x + (kBtnW - minusTxtSz.x) * 0.5f,
                       cy - minusTxtSz.y * 0.5f),
                IM_COL32(200, 210, 240, 230), "-");
    if (minusClk)
    {
        // 按档位缩小（每次缩小约 20%）
        float newZ = curZoom / 1.25f;
        ApplyZoom(newZ, editorMin, editorMax);
    }

    cx += kBtnW + 4.0f;

    // ── 滑轨 ─────────────────────────────────────────────────────────────
    float trackLeft  = cx;
    float trackRight = trackLeft + kTrackW;
    float trackY     = cy;

    // 滑轨背景
    dl->AddRectFilled(ImVec2(trackLeft, trackY - kTrackH * 0.5f),
                      ImVec2(trackRight, trackY + kTrackH * 0.5f),
                      IM_COL32(45, 55, 80, 200), kTrackH * 0.5f);

    // 已填充部分（蓝色）
    float t     = ZoomToT(curZoom);
    float thumbX = trackLeft + t * kTrackW;
    if (thumbX > trackLeft)
    {
        dl->AddRectFilled(ImVec2(trackLeft, trackY - kTrackH * 0.5f),
                          ImVec2(thumbX, trackY + kTrackH * 0.5f),
                          IM_COL32(80, 130, 220, 200), kTrackH * 0.5f);
    }

    // 100% 刻度线（中间）
    float midX = trackLeft + ZoomToT(1.0f) * kTrackW;
    dl->AddLine(ImVec2(midX, trackY - 5.0f), ImVec2(midX, trackY + 5.0f),
                IM_COL32(120, 160, 220, 120), 1.0f);

    // 滑块圆
    ImU32 thumbCol = IM_COL32(130, 175, 255, 255);

    // 可拖动区域（整条轨道 + thumb）
    ImVec2 trackHitMin(trackLeft - 4.0f, wPos.y + 2.0f);
    ImVec2 trackHitMax(trackRight + 4.0f, wMax.y - 2.0f);
    ImGui::SetCursorScreenPos(trackHitMin);
    ImGui::InvisibleButton("##zoom_track", trackHitMax - trackHitMin);

    bool trackHov  = ImGui::IsItemHovered();
    bool trackAct  = ImGui::IsItemActive();

    if (trackHov || trackAct)
    {
        thumbCol = IM_COL32(180, 210, 255, 255);
        ImGui::SetTooltip("拖动调整缩放\n%.0f%%  (%.2fx)", curZoom * 100.0f, curZoom);
    }

    if (trackAct && ImGui::IsMouseDown(0))
    {
        float mx    = ImGui::GetMousePos().x;
        float newT  = ImClamp((mx - trackLeft) / kTrackW, 0.0f, 1.0f);
        float newZ  = TToZoom(newT);
        ApplyZoom(newZ, editorMin, editorMax);
    }

    dl->AddCircleFilled(ImVec2(thumbX, trackY), kThumbR, thumbCol);
    // 圆心高亮
    dl->AddCircleFilled(ImVec2(thumbX, trackY), kThumbR * 0.45f, IM_COL32(220, 235, 255, 200));

    cx = trackRight + 4.0f;

    // ── 加号按钮 ─────────────────────────────────────────────────────────
    ImVec2 btnPlusMin(cx, wPos.y + 2.0f);
    ImVec2 btnPlusMax(cx + kBtnW, wMax.y - 2.0f);
    ImGui::SetCursorScreenPos(btnPlusMin);
    ImGui::InvisibleButton("##zoom_plus", btnPlusMax - btnPlusMin);
    bool plusHov = ImGui::IsItemHovered();
    bool plusClk = ImGui::IsItemClicked();
    dl->AddRectFilled(btnPlusMin, btnPlusMax,
                      plusHov ? IM_COL32(80, 100, 150, 160) : IM_COL32(50, 60, 90, 120), 3.0f);
    ImVec2 plusTxtSz = ImGui::CalcTextSize("+");
    dl->AddText(ImVec2(btnPlusMin.x + (kBtnW - plusTxtSz.x) * 0.5f,
                       cy - plusTxtSz.y * 0.5f),
                IM_COL32(200, 210, 240, 230), "+");
    if (plusClk)
    {
        float newZ = curZoom * 1.25f;
        ApplyZoom(newZ, editorMin, editorMax);
    }
}
