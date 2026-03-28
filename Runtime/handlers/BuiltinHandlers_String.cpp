// Runtime/BuiltinHandlers_String.cpp -- String 字符串节点处理器
#include "BuiltinHandlers_String.h"
#include <algorithm>
#include <cctype>

namespace NodeEditor {
namespace Runtime {

void RegisterHandlers_String(std::unordered_map<std::string, NodeHandler>& handlers)
{
    handlers["MakeString"] = [](ExecutionContext& ctx) {
        auto v = ctx.GetInputValue("Value").asString();
        ctx.SetOutputValue("String", Variant(v));
        return true;
    };

    handlers["AppendString"] = [](ExecutionContext& ctx) {
        std::string result;
        const auto* node = ctx.GetCurrentNode();
        if (node)
        {
            for (const auto& pin : node->pins)
            {
                if (pin.kind == PinKind::Input && !pin.isExec)
                    result += ctx.GetInputValue(pin.id).asString();
            }
        }
        else
        {
            result = ctx.GetInputValue("A").asString() + ctx.GetInputValue("B").asString();
        }
        ctx.SetOutputValue("Result", Variant(result));
        return true;
    };

    handlers["StringLength"] = [](ExecutionContext& ctx) {
        auto s = ctx.GetInputValue("String").asString();
        ctx.SetOutputValue("Length", Variant(static_cast<int64_t>(s.size())));
        return true;
    };

    handlers["StringEquals"] = [](ExecutionContext& ctx) {
        auto a = ctx.GetInputValue("A").asString();
        auto b = ctx.GetInputValue("B").asString();
        ctx.SetOutputValue("Result", Variant(a == b));
        return true;
    };

    handlers["StringEqualsIgnoreCase"] = [](ExecutionContext& ctx) {
        auto a = ctx.GetInputValue("A").asString();
        auto b = ctx.GetInputValue("B").asString();
        bool equal = false;
        if (a.size() == b.size())
        {
            equal = std::equal(a.begin(), a.end(), b.begin(),
                [](char ca, char cb) { return std::tolower(static_cast<unsigned char>(ca)) == std::tolower(static_cast<unsigned char>(cb)); });
        }
        ctx.SetOutputValue("Result", Variant(equal));
        return true;
    };

    handlers["StringContains"] = [](ExecutionContext& ctx) {
        auto str = ctx.GetInputValue("String").asString();
        auto sub = ctx.GetInputValue("Substring").asString();
        bool found = !sub.empty() && str.find(sub) != std::string::npos;
        ctx.SetOutputValue("Result", Variant(found));
        return true;
    };

    handlers["StringStartsWith"] = [](ExecutionContext& ctx) {
        auto str = ctx.GetInputValue("String").asString();
        auto prefix = ctx.GetInputValue("Prefix").asString();
        bool result = false;
        if (prefix.size() <= str.size())
            result = str.compare(0, prefix.size(), prefix) == 0;
        ctx.SetOutputValue("Result", Variant(result));
        return true;
    };

    handlers["StringEndsWith"] = [](ExecutionContext& ctx) {
        auto str = ctx.GetInputValue("String").asString();
        auto suffix = ctx.GetInputValue("Suffix").asString();
        bool result = false;
        if (suffix.size() <= str.size())
            result = str.compare(str.size() - suffix.size(), suffix.size(), suffix) == 0;
        ctx.SetOutputValue("Result", Variant(result));
        return true;
    };

    handlers["StringReplace"] = [](ExecutionContext& ctx) {
        auto str = ctx.GetInputValue("String").asString();
        auto from = ctx.GetInputValue("From").asString();
        auto to = ctx.GetInputValue("To").asString();
        if (!from.empty())
        {
            size_t pos = 0;
            while ((pos = str.find(from, pos)) != std::string::npos)
            {
                str.replace(pos, from.size(), to);
                pos += to.size();
            }
        }
        ctx.SetOutputValue("Result", Variant(str));
        return true;
    };

    handlers["StringToUpper"] = [](ExecutionContext& ctx) {
        auto str = ctx.GetInputValue("String").asString();
        std::transform(str.begin(), str.end(), str.begin(),
            [](unsigned char c) { return static_cast<char>(std::toupper(c)); });
        ctx.SetOutputValue("Result", Variant(str));
        return true;
    };

    handlers["StringToLower"] = [](ExecutionContext& ctx) {
        auto str = ctx.GetInputValue("String").asString();
        std::transform(str.begin(), str.end(), str.begin(),
            [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
        ctx.SetOutputValue("Result", Variant(str));
        return true;
    };

    handlers["StringSubstring"] = [](ExecutionContext& ctx) {
        auto str = ctx.GetInputValue("String").asString();
        int64_t start = ctx.GetInputValue("Start").asInt();
        int64_t count = ctx.GetInputValue("Count").asInt();
        std::string result;
        if (start >= 0 && static_cast<size_t>(start) < str.size())
        {
            if (count < 0)
                result = str.substr(static_cast<size_t>(start));
            else
                result = str.substr(static_cast<size_t>(start), static_cast<size_t>(count));
        }
        ctx.SetOutputValue("Result", Variant(result));
        return true;
    };

    handlers["StringFind"] = [](ExecutionContext& ctx) {
        auto str = ctx.GetInputValue("String").asString();
        auto sub = ctx.GetInputValue("Substring").asString();
        auto pos = str.find(sub);
        bool found = (pos != std::string::npos);
        ctx.SetOutputValue("Index", Variant(found ? static_cast<int64_t>(pos) : static_cast<int64_t>(-1)));
        ctx.SetOutputValue("Found", Variant(found));
        return true;
    };

    handlers["StringSplit"] = [](ExecutionContext& ctx) {
        auto str = ctx.GetInputValue("String").asString();
        auto delim = ctx.GetInputValue("Delimiter").asString();
        std::vector<Variant> parts;
        if (delim.empty())
        {
            for (size_t i = 0; i < str.size(); ++i)
                parts.push_back(Variant(std::string(1, str[i])));
        }
        else
        {
            size_t pos = 0;
            size_t found;
            while ((found = str.find(delim, pos)) != std::string::npos)
            {
                parts.push_back(Variant(str.substr(pos, found - pos)));
                pos = found + delim.size();
            }
            parts.push_back(Variant(str.substr(pos)));
        }
        ctx.SetOutputValue("Count", Variant(static_cast<int64_t>(parts.size())));
        ctx.SetOutputValue("Array", Variant(std::move(parts)));
        return true;
    };

    handlers["StringTrimmed"] = [](ExecutionContext& ctx) {
        auto str = ctx.GetInputValue("String").asString();
        size_t start = 0;
        while (start < str.size() && std::isspace(static_cast<unsigned char>(str[start])))
            ++start;
        size_t end = str.size();
        while (end > start && std::isspace(static_cast<unsigned char>(str[end - 1])))
            --end;
        ctx.SetOutputValue("Result", Variant(str.substr(start, end - start)));
        return true;
    };

    // FormatString — 用 {0}, {1}, ... 替换参数
    handlers["FormatString"] = [](ExecutionContext& ctx) {
        std::string fmt = ctx.GetInputValue("Format").asString();
        const auto* node = ctx.GetCurrentNode();
        if (node)
        {
            // 收集除 "Format" 外的所有输入引脚值
            std::vector<std::string> args;
            bool skipFirst = true;
            for (const auto& pin : node->pins)
            {
                if (pin.kind == PinKind::Input && !pin.isExec)
                {
                    if (skipFirst) { skipFirst = false; continue; } // 跳过 "Format"
                    args.push_back(ctx.GetInputValue(pin.id).asString());
                }
            }

            // 替换 {0}, {1}, ...
            for (size_t i = 0; i < args.size(); ++i)
            {
                std::string placeholder = "{" + std::to_string(i) + "}";
                size_t pos = 0;
                while ((pos = fmt.find(placeholder, pos)) != std::string::npos)
                {
                    fmt.replace(pos, placeholder.size(), args[i]);
                    pos += args[i].size();
                }
            }
        }
        ctx.SetOutputValue("Result", Variant(fmt));
        return true;
    };

    // StringJoin — 用分隔符连接数组
    handlers["StringJoin"] = [](ExecutionContext& ctx) {
        auto arr = ctx.GetInputValue("Array");
        auto sep = ctx.GetInputValue("Separator").asString();
        std::string result;
        for (size_t i = 0; i < arr.arraySize(); ++i)
        {
            if (i > 0) result += sep;
            result += arr.arrayGet(i).asString();
        }
        ctx.SetOutputValue("Result", Variant(result));
        return true;
    };

    // StringRepeat — 重复字符串 N 次
    handlers["StringRepeat"] = [](ExecutionContext& ctx) {
        auto str = ctx.GetInputValue("String").asString();
        int64_t count = ctx.GetInputValue("Count").asInt();
        if (count < 0) count = 0;
        if (count > 10000) count = 10000; // 安全限制
        std::string result;
        result.reserve(str.size() * static_cast<size_t>(count));
        for (int64_t i = 0; i < count; ++i)
            result += str;
        ctx.SetOutputValue("Result", Variant(result));
        return true;
    };

    // StringPadLeft — 左侧填充
    handlers["StringPadLeft"] = [](ExecutionContext& ctx) {
        auto str = ctx.GetInputValue("String").asString();
        int64_t totalWidth = ctx.GetInputValue("TotalWidth").asInt();
        auto padChar = ctx.GetInputValue("PadChar").asString();
        char pad = padChar.empty() ? ' ' : padChar[0];
        while (static_cast<int64_t>(str.size()) < totalWidth)
            str.insert(str.begin(), pad);
        ctx.SetOutputValue("Result", Variant(str));
        return true;
    };

    // StringPadRight — 右侧填充
    handlers["StringPadRight"] = [](ExecutionContext& ctx) {
        auto str = ctx.GetInputValue("String").asString();
        int64_t totalWidth = ctx.GetInputValue("TotalWidth").asInt();
        auto padChar = ctx.GetInputValue("PadChar").asString();
        char pad = padChar.empty() ? ' ' : padChar[0];
        while (static_cast<int64_t>(str.size()) < totalWidth)
            str.push_back(pad);
        ctx.SetOutputValue("Result", Variant(str));
        return true;
    };

    // CharAt — 获取指定位置字符
    handlers["CharAt"] = [](ExecutionContext& ctx) {
        auto str = ctx.GetInputValue("String").asString();
        int64_t index = ctx.GetInputValue("Index").asInt();
        bool valid = (index >= 0 && static_cast<size_t>(index) < str.size());
        if (valid)
            ctx.SetOutputValue("Char", Variant(std::string(1, str[static_cast<size_t>(index)])));
        else
            ctx.SetOutputValue("Char", Variant(std::string()));
        ctx.SetOutputValue("Valid", Variant(valid));
        return true;
    };

    // StringReverse — 反转字符串
    handlers["StringReverse"] = [](ExecutionContext& ctx) {
        auto str = ctx.GetInputValue("String").asString();
        std::reverse(str.begin(), str.end());
        ctx.SetOutputValue("Result", Variant(str));
        return true;
    };

    // ── 新增字符串节点 ─────────────────────────────────────────────────────
    handlers["StringCount"] = [](ExecutionContext& ctx) {
        auto hay    = ctx.GetInputValue("String").asString();
        auto needle = ctx.GetInputValue("Substring").asString();
        if (needle.empty()) { ctx.SetOutputValue("Count", Variant(static_cast<int64_t>(0))); return true; }
        int64_t count = 0;
        for (size_t pos = 0; (pos = hay.find(needle, pos)) != std::string::npos; pos += needle.size())
            ++count;
        ctx.SetOutputValue("Count", Variant(count));
        return true;
    };

    handlers["StringIsEmpty"] = [](ExecutionContext& ctx) {
        bool empty = ctx.GetInputValue("String").asString().empty();
        ctx.SetOutputValue("Result", Variant(empty));
        return true;
    };

    handlers["StringInsert"] = [](ExecutionContext& ctx) {
        auto str   = ctx.GetInputValue("String").asString();
        int64_t pos = ctx.GetInputValue("Position").asInt();
        auto ins   = ctx.GetInputValue("Insert").asString();
        if (pos < 0) pos = 0;
        if (static_cast<size_t>(pos) > str.size()) pos = static_cast<int64_t>(str.size());
        str.insert(static_cast<size_t>(pos), ins);
        ctx.SetOutputValue("Result", Variant(str));
        return true;
    };

    handlers["StringRemove"] = [](ExecutionContext& ctx) {
        auto str   = ctx.GetInputValue("String").asString();
        int64_t pos = ctx.GetInputValue("Position").asInt();
        int64_t len = ctx.GetInputValue("Length").asInt();
        if (pos < 0) pos = 0;
        if (static_cast<size_t>(pos) < str.size())
            str.erase(static_cast<size_t>(pos),
                      len < 0 ? std::string::npos : static_cast<size_t>(len));
        ctx.SetOutputValue("Result", Variant(str));
        return true;
    };
}

} // namespace Runtime
} // namespace NodeEditor
