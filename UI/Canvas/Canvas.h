// UI/Canvas/Canvas.h - 画布组件
// 包装原有的 imgui_canvas，提供统一的接口

#pragma once

// 包含原有的画布实现
#include "../../Editor/Core/imgui_canvas.h"

namespace NodeEditor {
namespace UI {

// 使用原有的画布类型
using Canvas = ::ImGuiEx::Canvas;
using CanvasView = ::ImGuiEx::CanvasView;

} // namespace UI
} // namespace NodeEditor
