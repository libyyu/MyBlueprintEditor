// Utils/Math/Math.h - 数学工具库
// 包装原有的 imgui_extra_math，提供统一的接口

#pragma once

// 包含原有的数学工具
#include "../../Editor/Core/imgui_extra_math.h"

namespace NodeEditor {
namespace Utils {
namespace Math {

// 使用原有命名空间中的类型
using ::ImLine;
using ::ImRect_IsEmpty;
using ::ImRect_ClosestPoint;
using ::ImRect_ClosestLine;
using ::ImLength;
using ::ImLengthSq;
using ::ImNormalized;
using ::ImLerp;

namespace Easing = ::ImEasing;

} // namespace Math
} // namespace Utils
} // namespace NodeEditor
