// Handlers_Array.cpp -- Misc/Array 节点处理器注册
#include "../BlueprintEditor.h"

void BlueprintEditor::RegisterHandlers_Array()
{
    // ArrayLength — 获取数组长度
    m_HandlerRegistry["ArrayLength"] = [](RTContext& ctx) {
        auto arr = ctx.GetInputValue("Array");
        ctx.SetOutputValue("Length", RTVariant(static_cast<int64_t>(arr.arraySize())));
        return true;
    };

    // ArrayGet — 按索引获取数组元素（保留原始数据类型）
    m_HandlerRegistry["ArrayGet"] = [](RTContext& ctx) {
        auto arr = ctx.GetInputValue("Array");
        int64_t index = ctx.GetInputValue("Index").asInt();
        bool valid = (index >= 0 && static_cast<size_t>(index) < arr.arraySize());
        if (valid)
            ctx.SetOutputValue("Element", arr.arrayGet(static_cast<size_t>(index)));
        else
            ctx.SetOutputValue("Element", RTVariant());
        ctx.SetOutputValue("Valid", RTVariant(valid));
        return true;
    };

    // ForEachLoop — 遍历数组（类似 UE 的 ForEachLoop）
    m_HandlerRegistry["ForEachLoop"] = [](RTContext& ctx) {
        auto arr = ctx.GetInputValue("Array");
        size_t count = arr.arraySize();
        ctx.Log("  [ForEachLoop] Array size = " + std::to_string(count));
        for (size_t i = 0; i < count; ++i)
        {
            ctx.SetOutputValue("Array Element", arr.arrayGet(i));
            ctx.SetOutputValue("Array Index", RTVariant(static_cast<int64_t>(i)));
            if (!ctx.ActivateOutputFlow("Loop Body"))
                return false;
        }
        ctx.ActivateOutputFlow("Completed");
        return true;
    };
}
