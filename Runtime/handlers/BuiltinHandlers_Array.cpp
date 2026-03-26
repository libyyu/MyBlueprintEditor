// Runtime/BuiltinHandlers_Array.cpp -- Array 节点处理器
#include "BuiltinHandlers_Array.h"
#include <algorithm>
#include <unordered_set>

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

    // ── ArrayFind ─────────────────────────────────────────────────────────
    handlers["ArrayFind"] = [](ExecutionContext& ctx) {
        const auto& arr = ctx.GetInputValue("Array").arrayValue;
        const Variant& target = ctx.GetInputValue("Element");
        for (int64_t i = 0; i < (int64_t)arr.size(); ++i)
        {
            if (arr[i].asString() == target.asString())
            {
                ctx.SetOutputValue("Index", Variant(i));
                ctx.SetOutputValue("Found", Variant(true));
                return true;
            }
        }
        ctx.SetOutputValue("Index", Variant(int64_t(-1)));
        ctx.SetOutputValue("Found", Variant(false));
        return true;
    };

    // ── ArraySlice ────────────────────────────────────────────────────────
    handlers["ArraySlice"] = [](ExecutionContext& ctx) {
        const auto& arr = ctx.GetInputValue("Array").arrayValue;
        int64_t sz    = static_cast<int64_t>(arr.size());
        int64_t start = ctx.GetInputValue("Start").asInt();
        int64_t end   = ctx.GetInputValue("End").asInt();
        if (start < 0) start = std::max(int64_t(0), sz + start);
        if (end   < 0) end   = std::max(int64_t(0), sz + end);
        start = std::clamp(start, int64_t(0), sz);
        end   = std::clamp(end,   int64_t(0), sz);
        std::vector<Variant> result(arr.begin() + start, arr.begin() + end);
        ctx.SetOutputValue("Result", Variant(std::move(result)));
        return true;
    };

    // ── ArrayConcat ───────────────────────────────────────────────────────
    handlers["ArrayConcat"] = [](ExecutionContext& ctx) {
        auto a = ctx.GetInputValue("Array A").arrayValue;
        const auto& b = ctx.GetInputValue("Array B").arrayValue;
        a.insert(a.end(), b.begin(), b.end());
        ctx.SetOutputValue("Result", Variant(std::move(a)));
        return true;
    };

    // ── ArrayUnique ───────────────────────────────────────────────────────
    handlers["ArrayUnique"] = [](ExecutionContext& ctx) {
        const auto& arr = ctx.GetInputValue("Array").arrayValue;
        std::vector<Variant> result;
        std::unordered_set<std::string> seen;
        for (const auto& v : arr)
        {
            std::string key = v.asString();
            if (seen.insert(key).second)
                result.push_back(v);
        }
        ctx.SetOutputValue("Result", Variant(std::move(result)));
        return true;
    };

    // ── ArraySort ─────────────────────────────────────────────────────────
    handlers["ArraySort"] = [](ExecutionContext& ctx) {
        auto arr  = ctx.GetInputValue("Array").arrayValue;
        bool desc = ctx.GetInputValue("Descending").asBool();
        std::sort(arr.begin(), arr.end(), [desc](const Variant& a, const Variant& b) {
            // 同质数组：优先数值比较，否则字符串比较
            if (a.type == PinDataType::Float || b.type == PinDataType::Float ||
                a.type == PinDataType::Integer || b.type == PinDataType::Integer)
            {
                double fa = a.asFloat(), fb = b.asFloat();
                return desc ? fa > fb : fa < fb;
            }
            std::string sa = a.asString(), sb = b.asString();
            return desc ? sa > sb : sa < sb;
        });
        ctx.ActivateOutputFlow("");
        ctx.SetOutputValue("Array", Variant(std::move(arr)));
        return true;
    };

    // ── ArrayFirst ────────────────────────────────────────────────────────
    handlers["ArrayFirst"] = [](ExecutionContext& ctx) {
        const auto& arr = ctx.GetInputValue("Array").arrayValue;
        if (!arr.empty())
        {
            ctx.SetOutputValue("Element", arr.front());
            ctx.SetOutputValue("Valid", Variant(true));
        }
        else
        {
            ctx.SetOutputValue("Element", Variant());
            ctx.SetOutputValue("Valid", Variant(false));
        }
        return true;
    };

    // ── ArrayLast ─────────────────────────────────────────────────────────
    handlers["ArrayLast"] = [](ExecutionContext& ctx) {
        const auto& arr = ctx.GetInputValue("Array").arrayValue;
        if (!arr.empty())
        {
            ctx.SetOutputValue("Element", arr.back());
            ctx.SetOutputValue("Valid", Variant(true));
        }
        else
        {
            ctx.SetOutputValue("Element", Variant());
            ctx.SetOutputValue("Valid", Variant(false));
        }
        return true;
    };

    // ── ArrayRemove (by value) ────────────────────────────────────────────
    handlers["ArrayRemove"] = [](ExecutionContext& ctx) {
        auto arr = ctx.GetInputValue("Array").arrayValue;
        const std::string target = ctx.GetInputValue("Element").asString();
        bool removed = false;
        for (auto it = arr.begin(); it != arr.end(); ++it)
        {
            if (it->asString() == target)
            {
                arr.erase(it);
                removed = true;
                break;
            }
        }
        ctx.ActivateOutputFlow("");
        ctx.SetOutputValue("Array",   Variant(std::move(arr)));
        ctx.SetOutputValue("Removed", Variant(removed));
        return true;
    };
}

// ============================================================================
// Tree 处理器
// ============================================================================

} // namespace Runtime
} // namespace NodeEditor
