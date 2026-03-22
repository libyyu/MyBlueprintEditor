// Handlers_String.cpp -- Misc/String 节点处理器注册
#include "../BlueprintEditor.h"
#include <algorithm>
#include <cctype>

void BlueprintEditor::RegisterHandlers_String()
{
    m_HandlerRegistry["MakeString"] = [](RTContext& ctx) {
        auto v = ctx.GetInputValue("Value").asString();
        ctx.SetOutputValue("String", RTVariant(v));
        return true;
    };

    m_HandlerRegistry["AppendString"] = [](RTContext& ctx) {
        // Iterate all input pins of the current node and concatenate their string values.
        // This supports dynamic pins (A, B, C, D, ...) added at edit time.
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
            // Fallback: read A and B only
            result = ctx.GetInputValue("A").asString() + ctx.GetInputValue("B").asString();
        }
        ctx.SetOutputValue("Result", RTVariant(result));
        return true;
    };

    m_HandlerRegistry["StringLength"] = [](RTContext& ctx) {
        auto s = ctx.GetInputValue("String").asString();
        ctx.SetOutputValue("Length", RTVariant(static_cast<int64_t>(s.size())));
        return true;
    };

    // ==================================================================
    // String 比较节点
    // ==================================================================

    // StringEquals — 区分大小写的字符串比较
    m_HandlerRegistry["StringEquals"] = [](RTContext& ctx) {
        auto a = ctx.GetInputValue("A").asString();
        auto b = ctx.GetInputValue("B").asString();
        ctx.SetOutputValue("Result", RTVariant(a == b));
        return true;
    };

    // StringEqualsIgnoreCase — 不区分大小写的字符串比较
    m_HandlerRegistry["StringEqualsIgnoreCase"] = [](RTContext& ctx) {
        auto a = ctx.GetInputValue("A").asString();
        auto b = ctx.GetInputValue("B").asString();
        bool equal = false;
        if (a.size() == b.size())
        {
            equal = std::equal(a.begin(), a.end(), b.begin(),
                [](char ca, char cb) { return std::tolower(static_cast<unsigned char>(ca)) == std::tolower(static_cast<unsigned char>(cb)); });
        }
        ctx.SetOutputValue("Result", RTVariant(equal));
        return true;
    };

    // StringContains — 是否包含子串
    m_HandlerRegistry["StringContains"] = [](RTContext& ctx) {
        auto str = ctx.GetInputValue("String").asString();
        auto sub = ctx.GetInputValue("Substring").asString();
        bool found = !sub.empty() && str.find(sub) != std::string::npos;
        ctx.SetOutputValue("Result", RTVariant(found));
        return true;
    };

    // StringStartsWith — 是否以前缀开头
    m_HandlerRegistry["StringStartsWith"] = [](RTContext& ctx) {
        auto str = ctx.GetInputValue("String").asString();
        auto prefix = ctx.GetInputValue("Prefix").asString();
        bool result = false;
        if (prefix.size() <= str.size())
            result = str.compare(0, prefix.size(), prefix) == 0;
        ctx.SetOutputValue("Result", RTVariant(result));
        return true;
    };

    // StringEndsWith — 是否以后缀结尾
    m_HandlerRegistry["StringEndsWith"] = [](RTContext& ctx) {
        auto str = ctx.GetInputValue("String").asString();
        auto suffix = ctx.GetInputValue("Suffix").asString();
        bool result = false;
        if (suffix.size() <= str.size())
            result = str.compare(str.size() - suffix.size(), suffix.size(), suffix) == 0;
        ctx.SetOutputValue("Result", RTVariant(result));
        return true;
    };

    // ==================================================================
    // String 操作节点
    // ==================================================================

    // StringReplace — 替换所有匹配的子串
    m_HandlerRegistry["StringReplace"] = [](RTContext& ctx) {
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
        ctx.SetOutputValue("Result", RTVariant(str));
        return true;
    };

    // StringToUpper — 转大写
    m_HandlerRegistry["StringToUpper"] = [](RTContext& ctx) {
        auto str = ctx.GetInputValue("String").asString();
        std::transform(str.begin(), str.end(), str.begin(),
            [](unsigned char c) { return static_cast<char>(std::toupper(c)); });
        ctx.SetOutputValue("Result", RTVariant(str));
        return true;
    };

    // StringToLower — 转小写
    m_HandlerRegistry["StringToLower"] = [](RTContext& ctx) {
        auto str = ctx.GetInputValue("String").asString();
        std::transform(str.begin(), str.end(), str.begin(),
            [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
        ctx.SetOutputValue("Result", RTVariant(str));
        return true;
    };

    // StringSubstring — 截取子串
    m_HandlerRegistry["StringSubstring"] = [](RTContext& ctx) {
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
        ctx.SetOutputValue("Result", RTVariant(result));
        return true;
    };

    // StringFind — 查找子串位置
    m_HandlerRegistry["StringFind"] = [](RTContext& ctx) {
        auto str = ctx.GetInputValue("String").asString();
        auto sub = ctx.GetInputValue("Substring").asString();
        auto pos = str.find(sub);
        bool found = (pos != std::string::npos);
        ctx.SetOutputValue("Index", RTVariant(found ? static_cast<int64_t>(pos) : static_cast<int64_t>(-1)));
        ctx.SetOutputValue("Found", RTVariant(found));
        return true;
    };

    // StringSplit — 按分隔符拆分为字符串数组
    m_HandlerRegistry["StringSplit"] = [](RTContext& ctx) {
        auto str = ctx.GetInputValue("String").asString();
        auto delim = ctx.GetInputValue("Delimiter").asString();
        std::vector<RTVariant> parts;
        if (delim.empty())
        {
            // 分隔符为空时，将每个字符作为一个元素
            for (size_t i = 0; i < str.size(); ++i)
                parts.push_back(RTVariant(std::string(1, str[i])));
        }
        else
        {
            size_t pos = 0;
            size_t found;
            while ((found = str.find(delim, pos)) != std::string::npos)
            {
                parts.push_back(RTVariant(str.substr(pos, found - pos)));
                pos = found + delim.size();
            }
            parts.push_back(RTVariant(str.substr(pos)));
        }
        ctx.SetOutputValue("Count", RTVariant(static_cast<int64_t>(parts.size())));
        ctx.SetOutputValue("Array", RTVariant(std::move(parts)));
        return true;
    };

    // StringTrimmed — 去除首尾空白字符
    m_HandlerRegistry["StringTrimmed"] = [](RTContext& ctx) {
        auto str = ctx.GetInputValue("String").asString();
        // trim leading
        size_t start = 0;
        while (start < str.size() && std::isspace(static_cast<unsigned char>(str[start])))
            ++start;
        // trim trailing
        size_t end = str.size();
        while (end > start && std::isspace(static_cast<unsigned char>(str[end - 1])))
            --end;
        ctx.SetOutputValue("Result", RTVariant(str.substr(start, end - start)));
        return true;
    };
}
