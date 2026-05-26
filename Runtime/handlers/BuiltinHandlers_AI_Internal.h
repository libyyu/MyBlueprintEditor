// Runtime/handlers/BuiltinHandlers_AI_Internal.h
// 内部头文件：AI 子模块共享的辅助函数和类型声明
#pragma once

#include "../BlueprintRunner.h"
#include "../BlueprintExporter.h"
#include "../Http/IHttpClient.h"
#include "../../Utils/Json/crude_json.h"
#include "../FileSystem.h"
#include <sstream>
#include <regex>
#include <atomic>
#include <filesystem>

namespace NodeEditor {
namespace Runtime {

// Variant ↔ crude_json 转换
crude_json::value variantToJson_AI(const Variant& v);
Variant jsonToVariant_AI(const crude_json::value& j);

// JSON 路径工具
std::vector<std::string> parsePath(const std::string& path);
void setAtPath(crude_json::value& node, const std::vector<std::string>& segs,
               size_t idx, const crude_json::value& val);

// LLM 辅助
const crude_json::value* GetFirstChoice(const crude_json::value& resp);

struct ToolCallInfo { std::string id, name, arguments; };
ToolCallInfo ExtractToolCall(const crude_json::value& tc);

HttpRequest BuildLLMRequest(ExecutionContext& ctx,
    const std::string& baseURL, const std::string& apiKey, const std::string& model);
HttpRequest BuildLLMRequest(ExecutionContext& ctx);

// 子注册函数
void RegisterHandlers_AI_JSON(std::unordered_map<std::string, NodeHandler>& handlers);
void RegisterHandlers_AI_LLM(std::unordered_map<std::string, NodeHandler>& handlers);
void RegisterHandlers_AI_Tool(std::unordered_map<std::string, NodeHandler>& handlers);
void RegisterHandlers_AI_Agent(std::unordered_map<std::string, NodeHandler>& handlers);

} // namespace Runtime
} // namespace NodeEditor
