// Utils/Math/Bezier.h - 贝塞尔曲线数学库
// 包装原有的 imgui_bezier_math，提供统一的接口

#pragma once

// 包含原有的贝塞尔曲线工具
#include "../../Editor/Core/imgui_bezier_math.h"

namespace NodeEditor {
namespace Utils {
namespace Math {

// 使用原有命名空间中的类型和函数
using ::ImCubicBezierPoints;
using ::ImCubicBezier;
using ::ImCubicBezierDt;
using ::ImCubicBezierLength;
using ::ImCubicBezierSplit;
using ::ImCubicBezierBoundingRect;
using ::ImProjectOnCubicBezier;
using ::ImCubicBezierLineIntersect;
using ::ImCubicBezierSubdivide;
using ::ImCubicBezierFixedStep;

// 结果结构
using ::ImProjectResult;
using ::ImCubicBezierSplitResult;
using ::ImCubicBezierIntersectResult;

// 回调类型
using ::ImCubicBezierSubdivideCallback;
using ::ImCubicBezierFixedStepCallback;

} // namespace Math
} // namespace Utils
} // namespace NodeEditor
