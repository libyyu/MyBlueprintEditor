// Runtime/BuiltinHandlers_Array.cpp -- Array 节点处理器
#include "BuiltinHandlers_Array.h"

namespace NodeEditor {
namespace Runtime {

void RegisterHandlers_Array(std::unordered_map<std::string, NodeHandler>& handlers)
{
    handlers["ArrayLength"] = [](ExecutionContext& ctx) {
        auto arr = ctx.GetInputValue("Array");
        ctx.SetOutputValue("Length", Variant(static_cast<int64_t>(arr.arraySize())));
        return true;
    };

    handlers["ArrayGet"] = [](ExecutionContext& ctx) {
        auto arr = ctx.GetInputValue("Array");
        int64_t index = ctx.GetInputValue("Index").asInt();
        bool valid = (index >= 0 && static_cast<size_t>(index) < arr.arraySize());
        if (valid)
            ctx.SetOutputValue("Element", arr.arrayGet(static_cast<size_t>(index)));
        else
            ctx.SetOutputValue("Element", Variant());
        ctx.SetOutputValue("Valid", Variant(valid));
        return true;
    };

    handlers["ArraySet"] = [](ExecutionContext& ctx) {
        auto arr = ctx.GetInputValue("Array");
        int64_t index = ctx.GetInputValue("Index").asInt();
        auto element = ctx.GetInputValue("Element");

        if (index < 0)
        {
            ctx.Log("  [ArraySet] Error: negative index " + std::to_string(index));
            ctx.SetOutputValue("Array", arr);
            return true;
        }

        arr.arraySet(static_cast<size_t>(index), element);
        ctx.SetOutputValue("Array", arr);
        ctx.Log("  [ArraySet] Set index " + std::to_string(index) + " -> " + element.asString());
        return true;
    };

    handlers["ArrayRemoveAt"] = [](ExecutionContext& ctx) {
        auto arr = ctx.GetInputValue("Array");
        int64_t index = ctx.GetInputValue("Index").asInt();

        bool valid = (index >= 0 && static_cast<size_t>(index) < arr.arraySize());
        if (valid)
        {
            arr.arrayRemoveAt(static_cast<size_t>(index));
            ctx.Log("  [ArrayRemoveAt] Removed index " + std::to_string(index) + ", new size = " + std::to_string(arr.arraySize()));
        }
        else
        {
            ctx.Log("  [ArrayRemoveAt] Invalid index " + std::to_string(index) + " (size=" + std::to_string(arr.arraySize()) + ")");
        }
        ctx.SetOutputValue("Array", arr);
        ctx.SetOutputValue("Valid", Variant(valid));
        return true;
    };

    handlers["ArrayClear"] = [](ExecutionContext& ctx) {
        auto arr = ctx.GetInputValue("Array");
        ctx.Log("  [ArrayClear] Cleared array (was size " + std::to_string(arr.arraySize()) + ")");
        arr.arrayClear();
        ctx.SetOutputValue("Array", arr);
        return true;
    };

    handlers["ForEachLoop"] = [](ExecutionContext& ctx) {
        auto arr = ctx.GetInputValue("Array");
        size_t count = arr.arraySize();
        ctx.Log("  [ForEachLoop] Array size = " + std::to_string(count));
        for (size_t i = 0; i < count; ++i)
        {
            ctx.SetOutputValue("Array Element", arr.arrayGet(i));
            ctx.SetOutputValue("Array Index", Variant(static_cast<int64_t>(i)));
            if (!ctx.ActivateOutputFlow("Loop Body"))
                return false;
        }
        ctx.ActivateOutputFlow("Completed");
        return true;
    };

    handlers["ArrayAdd"] = [](ExecutionContext& ctx) {
        auto arr = ctx.GetInputValue("Array");
        auto element = ctx.GetInputValue("Element");
        // 确保是数组类型
        if (arr.type != PinDataType::Array)
        {
            arr.type = PinDataType::Array;
            arr.arrayValue.clear();
        }
        arr.arrayValue.push_back(element);
        ctx.SetOutputValue("Array", arr);
        ctx.SetOutputValue("New Length", Variant(static_cast<int64_t>(arr.arraySize())));
        ctx.Log("  [ArrayAdd] New size = " + std::to_string(arr.arraySize()));
        return true;
    };

    handlers["ArrayInsert"] = [](ExecutionContext& ctx) {
        auto arr = ctx.GetInputValue("Array");
        int64_t index = ctx.GetInputValue("Index").asInt();
        auto element = ctx.GetInputValue("Element");

        if (arr.type != PinDataType::Array)
        {
            arr.type = PinDataType::Array;
            arr.arrayValue.clear();
        }

        if (index < 0) index = 0;
        if (static_cast<size_t>(index) > arr.arrayValue.size())
            index = static_cast<int64_t>(arr.arrayValue.size());

        arr.arrayValue.insert(arr.arrayValue.begin() + static_cast<std::ptrdiff_t>(index), element);
        ctx.SetOutputValue("Array", arr);
        ctx.Log("  [ArrayInsert] Inserted at " + std::to_string(index) + ", new size = " + std::to_string(arr.arraySize()));
        return true;
    };

    handlers["ArrayContains"] = [](ExecutionContext& ctx) {
        auto arr = ctx.GetInputValue("Array");
        auto element = ctx.GetInputValue("Element");
        std::string searchStr = element.asString();
        bool found = false;
        int64_t foundIndex = -1;
        for (size_t i = 0; i < arr.arraySize(); ++i)
        {
            if (arr.arrayGet(i).asString() == searchStr)
            {
                found = true;
                foundIndex = static_cast<int64_t>(i);
                break;
            }
        }
        ctx.SetOutputValue("Found", Variant(found));
        ctx.SetOutputValue("Index", Variant(foundIndex));
        return true;
    };

    handlers["ArrayReverse"] = [](ExecutionContext& ctx) {
        auto arr = ctx.GetInputValue("Array");
        if (arr.type == PinDataType::Array)
            std::reverse(arr.arrayValue.begin(), arr.arrayValue.end());
        ctx.SetOutputValue("Array", arr);
        ctx.Log("  [ArrayReverse] Reversed array of size " + std::to_string(arr.arraySize()));
        return true;
    };

    handlers["MakeArray"] = [](ExecutionContext& ctx) {
        const auto* node = ctx.GetCurrentNode();
        std::vector<Variant> elements;
        if (node)
        {
            for (const auto& pin : node->pins)
            {
                if (pin.kind == PinKind::Input && !pin.isExec)
                    elements.push_back(ctx.GetInputValue(pin.id));
            }
        }
        ctx.SetOutputValue("Array", Variant(std::move(elements)));
        return true;
    };
}

// ============================================================================
// Tree 处理器
// ============================================================================

} // namespace Runtime
} // namespace NodeEditor
