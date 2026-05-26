// BuiltinHandlers_AI_Agent.cpp - Agent/Context/LLM.StructuredOutput/Schema/Route/Intent handlers
#include "BuiltinHandlers_AI_Internal.h"

namespace NodeEditor {
namespace Runtime {

void RegisterHandlers_AI_Agent(
    std::unordered_map<std::string, NodeHandler>& handlers)
{
    handlers["Agent.Plan"] = [](ExecutionContext& ctx) -> bool {
        auto* runner = ctx.GetRunner(); (void)runner;

        IHttpClient* client = BP_GetHttpClient();
        if (!client) {
            ctx.SetOutputValue("ErrorMessage", Variant(std::string("No HttpClient registered")));
            ctx.ActivateOutputFlow("onError");
            return true;
        }

        std::string baseUrl        = ctx.GetInputValue("BaseURL").asString();
        std::string apiKey         = ctx.GetInputValue("ApiKey").asString();
        std::string model          = ctx.GetInputValue("Model").asString();
        std::string goal           = ctx.GetInputValue("Goal").asString();
        std::string availableTools = ctx.GetInputValue("AvailableTools").asString();
        int maxSteps               = static_cast<int>(ctx.GetInputValue("MaxSteps").asInt());
        int maxTokens              = static_cast<int>(ctx.GetInputValue("MaxTokens").asInt());

        if (baseUrl.empty())    baseUrl   = "https://api.openai.com/v1";
        if (model.empty())      model     = "gpt-4o";
        if (maxSteps  <= 0)     maxSteps  = 5;
        if (maxTokens <= 0)     maxTokens = 1024;

        // 构造规划 prompt
        std::string sysPrompt =
            "You are a task planner. Given a GOAL and available TOOLS, decompose the goal into "
            "a minimal ordered sequence of steps (at most " + std::to_string(maxSteps) + " steps).\n"
            "Respond with ONLY a valid JSON array, no markdown, no extra text.\n"
            "Each element: {\"tool\": \"<tool_name>\", \"arguments\": {<key: value>}, \"reason\": \"<why>\"}";
        std::string userMsg =
            "GOAL: " + goal + "\n\n"
            "AVAILABLE TOOLS: " + (availableTools.empty() ? "[]" : availableTools);

        crude_json::array messages;
        crude_json::object sysMsg, userMsgObj;
        sysMsg["role"]        = crude_json::value(std::string("system"));
        sysMsg["content"]     = crude_json::value(sysPrompt);
        userMsgObj["role"]    = crude_json::value(std::string("user"));
        userMsgObj["content"] = crude_json::value(userMsg);
        messages.push_back(crude_json::value(std::move(sysMsg)));
        messages.push_back(crude_json::value(std::move(userMsgObj)));

        crude_json::object body;
        body["model"]      = crude_json::value(model);
        body["messages"]   = crude_json::value(std::move(messages));
        body["max_tokens"] = crude_json::value(static_cast<double>(maxTokens));

        HttpRequest req;
        req.url    = baseUrl + "/chat/completions";
        req.method = "POST";
        req.body   = crude_json::value(std::move(body)).dump();
        req.headers["Content-Type"]  = "application/json";
        req.headers["Authorization"] = "Bearer " + apiKey;
        req.timeoutSeconds = 60;

        PinId donePinId  = ctx.GetPinId("onDone");
        PinId errorPinId = ctx.GetPinId("onError");
        ctx.MarkDownstreamAsHandled("onDone");
        ctx.MarkDownstreamAsHandled("onError");

        auto sharedResp = std::make_shared<HttpResponse>();
        auto dispatcher = [client, req, sharedResp](ExecutionContext::AsyncResolve resolve) mutable
        {
            client->SendAsync(req, [sharedResp, resolve = std::move(resolve)](HttpResponse resp) mutable {
                *sharedResp = std::move(resp);
                resolve();
            });
        };

        auto onComplete = [sharedResp, donePinId, errorPinId](ExecutionContext& c) mutable
        {
            const HttpResponse& resp = *sharedResp;
            if (!resp.ok()) {
                c.SetOutputValue("ErrorMessage", Variant(resp.error.empty() ?
                    "HTTP " + std::to_string(resp.statusCode) : resp.error));
                c.SetOutputValue("PlanJSON",  Variant(std::string("[]")));
                c.SetOutputValue("StepCount", Variant(static_cast<int64_t>(0)));
                c.ActivateOutputFlow(errorPinId);
                return;
            }
            // 提取 LLM 回复内容
            std::string content;
            crude_json::value root = crude_json::value::parse(resp.body);
            if (root.is_object() && root.contains("choices")) {
                const auto& choices = root["choices"];
                if (choices.is_array() && !choices.get<crude_json::array>().empty()) {
                    const auto& first = choices.get<crude_json::array>()[0];
                    if (first.is_object() && first.contains("message")) {
                        const auto& msg = first["message"];
                        if (msg.is_object() && msg.contains("content")) {
                            const auto& cv = msg["content"];
                            if (cv.is_string()) content = cv.get<std::string>();
                        }
                    }
                }
            }
            // 解析步骤 JSON 数组
            // 去掉可能的 markdown 代码块
            auto trim = [](std::string s) -> std::string {
                size_t p = s.find('[');
                size_t q = s.rfind(']');
                if (p != std::string::npos && q != std::string::npos && q > p)
                    return s.substr(p, q - p + 1);
                return s;
            };
            std::string planJson = trim(content);
            crude_json::value planVal = crude_json::value::parse(planJson);
            int stepCount = 0;
            if (planVal.is_array())
                stepCount = static_cast<int>(planVal.get<crude_json::array>().size());
            else
                planJson = "[]";

            c.SetOutputValue("PlanJSON",      Variant(planJson));
            c.SetOutputValue("StepCount",     Variant(static_cast<int64_t>(stepCount)));
            c.SetOutputValue("ErrorMessage",  Variant(std::string("")));
            c.ActivateOutputFlow(donePinId);
        };

        ctx.RunAsync(std::move(dispatcher), std::move(onComplete));
        return true;
    };

    // ========================================================================
    // Agent.Reflect
    // 让 LLM 评估输出是否满足评判标准（Criteria），输出 pass/fail + Feedback
    // ========================================================================
    handlers["Agent.Reflect"] = [](ExecutionContext& ctx) -> bool {
        auto* runner = ctx.GetRunner(); (void)runner;

        IHttpClient* client = BP_GetHttpClient();
        if (!client) {
            ctx.SetOutputValue("ErrorMessage", Variant(std::string("No HttpClient registered")));
            ctx.ActivateOutputFlow("onError");
            return true;
        }

        std::string baseUrl  = ctx.GetInputValue("BaseURL").asString();
        std::string apiKey   = ctx.GetInputValue("ApiKey").asString();
        std::string model    = ctx.GetInputValue("Model").asString();
        std::string output   = ctx.GetInputValue("Output").asString();
        std::string criteria = ctx.GetInputValue("Criteria").asString();
        int maxTokens        = static_cast<int>(ctx.GetInputValue("MaxTokens").asInt());

        if (baseUrl.empty()) baseUrl = "https://api.openai.com/v1";
        if (model.empty())   model   = "gpt-4o";
        if (maxTokens <= 0)  maxTokens = 256;

        // 构造评估 prompt
        std::string sysPrompt =
            "You are a strict evaluator. Given an OUTPUT and CRITERIA, decide if the output PASSES or FAILS.\n"
            "Respond with a JSON object: {\"result\": \"pass\" or \"fail\", \"feedback\": \"brief reason\"}.\n"
            "No extra text, only valid JSON.";
        std::string userMsg =
            "OUTPUT:\n" + output + "\n\nCRITERIA:\n" + criteria;

        crude_json::array messages;
        crude_json::object sysMsg, userMsgObj;
        sysMsg["role"]    = crude_json::value(std::string("system"));
        sysMsg["content"] = crude_json::value(sysPrompt);
        userMsgObj["role"]    = crude_json::value(std::string("user"));
        userMsgObj["content"] = crude_json::value(userMsg);
        messages.push_back(crude_json::value(std::move(sysMsg)));
        messages.push_back(crude_json::value(std::move(userMsgObj)));

        crude_json::object body;
        body["model"]      = crude_json::value(model);
        body["messages"]   = crude_json::value(std::move(messages));
        body["max_tokens"] = crude_json::value(static_cast<double>(maxTokens));

        HttpRequest req;
        req.url    = baseUrl + "/chat/completions";
        req.method = "POST";
        req.body   = crude_json::value(std::move(body)).dump();
        req.headers["Content-Type"]  = "application/json";
        req.headers["Authorization"] = "Bearer " + apiKey;
        req.timeoutSeconds = 30;

        PinId passPinId  = ctx.GetPinId("onPass");
        PinId failPinId  = ctx.GetPinId("onFail");
        PinId errorPinId = ctx.GetPinId("onError");
        ctx.MarkDownstreamAsHandled("onPass");
        ctx.MarkDownstreamAsHandled("onFail");
        ctx.MarkDownstreamAsHandled("onError");

        auto sharedResp = std::make_shared<HttpResponse>();
        auto dispatcher = [client, req, sharedResp](ExecutionContext::AsyncResolve resolve) mutable
        {
            client->SendAsync(req, [sharedResp, resolve = std::move(resolve)](HttpResponse resp) mutable {
                *sharedResp = std::move(resp);
                resolve();
            });
        };

        auto onComplete = [sharedResp, passPinId, failPinId, errorPinId](ExecutionContext& c) mutable
        {
            const HttpResponse& resp = *sharedResp;
            if (!resp.ok()) {
                c.SetOutputValue("ErrorMessage", Variant(resp.error.empty() ?
                    "HTTP " + std::to_string(resp.statusCode) : resp.error));
                c.SetOutputValue("Score",    Variant(std::string("fail")));
                c.SetOutputValue("Feedback", Variant(std::string("")));
                c.ActivateOutputFlow(errorPinId);
                return;
            }
            // 解析 LLM 返回
            crude_json::value root = crude_json::value::parse(resp.body);
            std::string content;
            if (root.is_object() && root.contains("choices")) {
                const auto& choices = root["choices"];
                if (choices.is_array() && !choices.get<crude_json::array>().empty()) {
                    const auto& first = choices.get<crude_json::array>()[0];
                    if (first.is_object() && first.contains("message")) {
                        const auto& msg = first["message"];
                        if (msg.is_object() && msg.contains("content")) {
                            const auto& cv = msg["content"];
                            if (cv.is_string()) content = cv.get<std::string>();
                        }
                    }
                }
            }
            // 解析 JSON 评估结果
            crude_json::value evalJson = crude_json::value::parse(content);
            std::string result   = "fail";
            std::string feedback = content;
            if (evalJson.is_object()) {
                if (evalJson.contains("result") && evalJson["result"].is_string())
                    result = evalJson["result"].get<std::string>();
                if (evalJson.contains("feedback") && evalJson["feedback"].is_string())
                    feedback = evalJson["feedback"].get<std::string>();
            }
            c.SetOutputValue("Score",        Variant(result));
            c.SetOutputValue("Feedback",     Variant(feedback));
            c.SetOutputValue("ErrorMessage", Variant(std::string("")));
            if (result == "pass")
                c.ActivateOutputFlow(passPinId);
            else
                c.ActivateOutputFlow(failPinId);
        };

        ctx.RunAsync(std::move(dispatcher), std::move(onComplete));
        return true;
    };

    // ========================================================================
    // Context.Compress
    // 保留最近 KeepRecent 条消息，用 LLM 摘要更早的内容，防止 context 溢出
    // ========================================================================
    handlers["Context.Compress"] = [](ExecutionContext& ctx) -> bool {
        auto* runner = ctx.GetRunner(); (void)runner;

        IHttpClient* client = BP_GetHttpClient();
        if (!client) {
            ctx.SetOutputValue("ErrorMessage", Variant(std::string("No HttpClient registered")));
            ctx.ActivateOutputFlow("onError");
            return true;
        }

        std::string baseUrl  = ctx.GetInputValue("BaseURL").asString();
        std::string apiKey   = ctx.GetInputValue("ApiKey").asString();
        std::string model    = ctx.GetInputValue("Model").asString();
        std::string messages = ctx.GetInputValue("Messages").asString();
        int keepRecent       = static_cast<int>(ctx.GetInputValue("KeepRecent").asInt());
        int maxTokens        = static_cast<int>(ctx.GetInputValue("MaxTokens").asInt());

        if (baseUrl.empty()) baseUrl  = "https://api.openai.com/v1";
        if (model.empty())   model    = "gpt-4o";
        if (keepRecent <= 0) keepRecent = 6;
        if (maxTokens  <= 0) maxTokens  = 512;

        // 解析 messages 数组
        crude_json::value msgsJson = crude_json::value::parse(messages);
        if (!msgsJson.is_array()) {
            ctx.SetOutputValue("Compressed",   Variant(messages));
            ctx.SetOutputValue("ErrorMessage", Variant(std::string("Messages is not a JSON array")));
            ctx.ActivateOutputFlow("onError");
            return true;
        }

        const auto& arr = msgsJson.get<crude_json::array>();
        int total = static_cast<int>(arr.size());

        // 消息数量 <= KeepRecent，无需压缩
        if (total <= keepRecent) {
            ctx.SetOutputValue("Compressed",   Variant(messages));
            ctx.SetOutputValue("ErrorMessage", Variant(std::string("")));
            ctx.ActivateOutputFlow("onDone");
            return true;
        }

        // 需要压缩：[0, total-keepRecent) 的消息用 LLM 摘要
        int splitAt = total - keepRecent;
        crude_json::array earlyMsgs(arr.begin(), arr.begin() + splitAt);
        crude_json::array recentMsgs(arr.begin() + splitAt, arr.end());

        // 构造摘要请求
        std::string earlyText = crude_json::value(earlyMsgs).dump();
        crude_json::array summaryMessages;
        crude_json::object sysMsg;
        sysMsg["role"]    = crude_json::value(std::string("system"));
        sysMsg["content"] = crude_json::value(std::string(
            "Summarize the following conversation history concisely. "
            "Preserve key facts, decisions, and context. Output plain text summary only."));
        crude_json::object userMsgObj;
        userMsgObj["role"]    = crude_json::value(std::string("user"));
        userMsgObj["content"] = crude_json::value(earlyText);
        summaryMessages.push_back(crude_json::value(std::move(sysMsg)));
        summaryMessages.push_back(crude_json::value(std::move(userMsgObj)));

        crude_json::object reqBody;
        reqBody["model"]      = crude_json::value(model);
        reqBody["messages"]   = crude_json::value(std::move(summaryMessages));
        reqBody["max_tokens"] = crude_json::value(static_cast<double>(maxTokens));

        HttpRequest req;
        req.url    = baseUrl + "/chat/completions";
        req.method = "POST";
        req.body   = crude_json::value(std::move(reqBody)).dump();
        req.headers["Content-Type"]  = "application/json";
        req.headers["Authorization"] = "Bearer " + apiKey;
        req.timeoutSeconds = 30;

        PinId donePinId  = ctx.GetPinId("onDone");
        PinId errorPinId = ctx.GetPinId("onError");
        ctx.MarkDownstreamAsHandled("onDone");
        ctx.MarkDownstreamAsHandled("onError");

        auto sharedResp = std::make_shared<HttpResponse>();
        auto sharedRecent = std::make_shared<crude_json::array>(std::move(recentMsgs));

        auto dispatcher = [client, req, sharedResp](ExecutionContext::AsyncResolve resolve) mutable
        {
            client->SendAsync(req, [sharedResp, resolve = std::move(resolve)](HttpResponse resp) mutable {
                *sharedResp = std::move(resp);
                resolve();
            });
        };

        auto onComplete = [sharedResp, sharedRecent, donePinId, errorPinId](ExecutionContext& c) mutable
        {
            const HttpResponse& resp = *sharedResp;
            if (!resp.ok()) {
                c.SetOutputValue("ErrorMessage", Variant(resp.error.empty() ?
                    "HTTP " + std::to_string(resp.statusCode) : resp.error));
                c.ActivateOutputFlow(errorPinId);
                return;
            }
            // 提取摘要文本
            std::string summary;
            crude_json::value root = crude_json::value::parse(resp.body);
            if (root.is_object() && root.contains("choices")) {
                const auto& choices = root["choices"];
                if (choices.is_array() && !choices.get<crude_json::array>().empty()) {
                    const auto& first = choices.get<crude_json::array>()[0];
                    if (first.is_object() && first.contains("message")) {
                        const auto& msg = first["message"];
                        if (msg.is_object() && msg.contains("content")) {
                            const auto& cv = msg["content"];
                            if (cv.is_string()) summary = cv.get<std::string>();
                        }
                    }
                }
            }
            // 组装压缩后消息：[system summary] + recent messages
            crude_json::array compressed;
            crude_json::object summaryMsg;
            summaryMsg["role"]    = crude_json::value(std::string("system"));
            summaryMsg["content"] = crude_json::value(std::string("[Conversation summary] ") + summary);
            compressed.push_back(crude_json::value(std::move(summaryMsg)));
            for (const auto& m : *sharedRecent)
                compressed.push_back(m);

            c.SetOutputValue("Compressed",   Variant(crude_json::value(std::move(compressed)).dump()));
            c.SetOutputValue("ErrorMessage", Variant(std::string("")));
            c.ActivateOutputFlow(donePinId);
        };

        ctx.RunAsync(std::move(dispatcher), std::move(onComplete));
        return true;
    };

    // ========================================================================
    // LLM.StructuredOutput
    // 使用 response_format: json_schema 强制 LLM 输出符合 JSON Schema 的结构
    // in:  exec, BaseURL, ApiKey, Model, Messages, SystemPrompt,
    //      SchemaName(String), Schema(String JSON Schema),
    //      MaxTokens, Temperature
    // out: onSuccess(exec), onError(exec)
    //      Output(String JSON), ErrorMessage(String)
    // ========================================================================
    handlers["LLM.StructuredOutput"] = [](ExecutionContext& ctx) -> bool {
        auto* runner = ctx.GetRunner(); (void)runner;
        IHttpClient* client = BP_GetHttpClient();
        if (!client) {
            ctx.SetOutputValue("ErrorMessage", Variant(std::string("No HttpClient registered")));
            ctx.ActivateOutputFlow("onError");
            return true;
        }

        // 构建基础请求
        HttpRequest req = BuildLLMRequest(ctx);

        // 注入 response_format: { type: "json_schema", json_schema: {...} }
        std::string schemaName = ctx.GetInputValue("SchemaName").asString();
        std::string schemaStr  = ctx.GetInputValue("Schema").asString();
        if (schemaName.empty()) schemaName = "output";

        crude_json::value bodyVal = crude_json::value::parse(req.body);
        if (bodyVal.is_object()) {
            if (!schemaStr.empty()) {
                crude_json::value schemaVal = crude_json::value::parse(schemaStr);
                if (schemaVal.is_object()) {
                    crude_json::object jsonSchema;
                    jsonSchema["name"]   = crude_json::value(schemaName);
                    jsonSchema["strict"] = crude_json::value(true);
                    jsonSchema["schema"] = schemaVal;
                    crude_json::object rf;
                    rf["type"]        = crude_json::value(std::string("json_schema"));
                    rf["json_schema"] = crude_json::value(std::move(jsonSchema));
                    bodyVal.get<crude_json::object>()["response_format"] =
                        crude_json::value(std::move(rf));
                }
            } else {
                // 无 Schema 时退化为 json_object 模式
                crude_json::object rf;
                rf["type"] = crude_json::value(std::string("json_object"));
                bodyVal.get<crude_json::object>()["response_format"] =
                    crude_json::value(std::move(rf));
            }
            req.body = bodyVal.dump();
        }

        PinId successPinId = ctx.GetPinId("onSuccess");
        PinId errorPinId   = ctx.GetPinId("onError");
        ctx.MarkDownstreamAsHandled("onSuccess");
        ctx.MarkDownstreamAsHandled("onError");

        auto sharedResp = std::make_shared<HttpResponse>();
        auto dispatcher = [client, req, sharedResp](ExecutionContext::AsyncResolve resolve) mutable {
            client->SendAsync(req, [sharedResp, resolve = std::move(resolve)](HttpResponse r) mutable {
                *sharedResp = std::move(r); resolve();
            });
        };

        auto onComplete = [sharedResp, successPinId, errorPinId](ExecutionContext& c) mutable {
            const HttpResponse& resp = *sharedResp;
            if (!resp.ok()) {
                std::string errMsg = resp.error.empty()
                    ? "HTTP " + std::to_string(resp.statusCode) : resp.error;
                c.SetOutputValue("Output",       Variant(std::string("{}")));
                c.SetOutputValue("ErrorMessage", Variant(errMsg));
                c.ActivateOutputFlow(errorPinId);
                return;
            }
            // 提取 content（已是 JSON 字符串）
            std::string content;
            crude_json::value root = crude_json::value::parse(resp.body);
            const crude_json::value* first = GetFirstChoice(root);
            if (first && first->contains("message")) {
                const auto& msg = (*first)["message"];
                if (msg.is_object() && msg.contains("content")) {
                    const auto& cv = msg["content"];
                    if (cv.is_string()) content = cv.get<std::string>();
                }
            }
            c.SetOutputValue("Output",       Variant(content));
            c.SetOutputValue("ErrorMessage", Variant(std::string("")));
            c.Log("[LLM.StructuredOutput] output length=" + std::to_string(content.size()));
            c.ActivateOutputFlow(successPinId);
        };

        ctx.RunAsync(std::move(dispatcher), std::move(onComplete));
        return true;
    };

    // ========================================================================
    // Schema.Validate
    // 验证 JSON 字符串是否符合简化 JSON Schema（type/required/properties）
    // in:  JSON(String), Schema(String)
    // out: onPass(exec), onFail(exec)
    //      IsValid(Bool), ErrorMessage(String)
    // ========================================================================
    handlers["Schema.Validate"] = [](ExecutionContext& ctx) -> bool {
        auto* runner = ctx.GetRunner(); (void)runner;
        std::string jsonStr   = ctx.GetInputValue("JSON").asString();
        std::string schemaStr = ctx.GetInputValue("Schema").asString();

        crude_json::value data   = crude_json::value::parse(jsonStr);
        crude_json::value schema = crude_json::value::parse(schemaStr);

        std::string errMsg;
        bool valid = true;

        auto validate = [&](const crude_json::value& d, const crude_json::value& s, const std::string& path) -> bool {
            if (!s.is_object()) return true;

            // type check
            if (s.contains("type") && s["type"].is_string()) {
                std::string t = s["type"].get<std::string>();
                bool typeOk = false;
                if      (t == "object")  typeOk = d.is_object();
                else if (t == "array")   typeOk = d.is_array();
                else if (t == "string")  typeOk = d.is_string();
                else if (t == "number")  typeOk = d.is_number();
                else if (t == "integer") typeOk = d.is_number();
                else if (t == "boolean") typeOk = (d.type() == crude_json::type_t::boolean);
                else if (t == "null")    typeOk = d.is_null();
                else typeOk = true;
                if (!typeOk) {
                    errMsg = "'" + path + "' expected type '" + t + "'";
                    return false;
                }
            }

            // required fields
            if (s.contains("required") && s["required"].is_array() && d.is_object()) {
                for (const auto& req : s["required"].get<crude_json::array>()) {
                    if (!req.is_string()) continue;
                    std::string field = req.get<std::string>();
                    if (!d.contains(field)) {
                        errMsg = "missing required field '" + path + "." + field + "'";
                        return false;
                    }
                }
            }

            // properties
            if (s.contains("properties") && s["properties"].is_object() && d.is_object()) {
                for (const auto& kv : s["properties"].get<crude_json::object>()) {
                    if (!d.contains(kv.first)) continue;
                    // recurse (simple one-level — avoid deep recursion on large schemas)
                    const auto& sub = kv.second;
                    if (sub.is_object() && sub.contains("type") && sub["type"].is_string()) {
                        std::string t = sub["type"].get<std::string>();
                        bool typeOk = false;
                        const auto& dv = d[kv.first];
                        if      (t == "string")  typeOk = dv.is_string();
                        else if (t == "number")  typeOk = dv.is_number();
                        else if (t == "integer") typeOk = dv.is_number();
                        else if (t == "boolean") typeOk = (dv.type() == crude_json::type_t::boolean);
                        else if (t == "array")   typeOk = dv.is_array();
                        else if (t == "object")  typeOk = dv.is_object();
                        else typeOk = true;
                        if (!typeOk) {
                            errMsg = "'" + path + "." + kv.first + "' expected type '" + t + "'";
                            return false;
                        }
                    }
                }
            }
            return true;
        };

        valid = validate(data, schema, "$");

        ctx.SetOutputValue("IsValid",      Variant(valid));
        ctx.SetOutputValue("ErrorMessage", Variant(errMsg));
        if (valid)
            ctx.ActivateOutputFlow("onPass");
        else
            ctx.ActivateOutputFlow("onFail");
        return true;
    };

    // ========================================================================
    // LLM.Route
    // 根据 LLM 输出自动选择执行分支（最多 8 个分支）
    // in:  exec, BaseURL, ApiKey, Model, Input(String),
    //      Routes(String JSON array of {"label","description"})
    // out: onRoute0..onRoute7(exec), onError(exec)
    //      SelectedRoute(Integer), SelectedLabel(String), ErrorMessage(String)
    // ========================================================================
    handlers["LLM.Route"] = [](ExecutionContext& ctx) -> bool {
        auto* runner = ctx.GetRunner(); (void)runner;
        IHttpClient* client = BP_GetHttpClient();
        if (!client) {
            ctx.SetOutputValue("ErrorMessage", Variant(std::string("No HttpClient")));
            ctx.ActivateOutputFlow("onError");
            return true;
        }

        std::string baseUrl    = ctx.GetInputValue("BaseURL").asString();
        std::string apiKey     = ctx.GetInputValue("ApiKey").asString();
        std::string model      = ctx.GetInputValue("Model").asString();
        std::string input      = ctx.GetInputValue("Input").asString();
        std::string routesJson = ctx.GetInputValue("Routes").asString();

        if (baseUrl.empty()) baseUrl = "https://api.openai.com/v1";
        if (model.empty())   model   = "gpt-4o";

        // 解析路由列表
        crude_json::value routesVal = crude_json::value::parse(routesJson);
        struct RouteEntry { std::string label, description; };
        std::vector<RouteEntry> routes;
        if (routesVal.is_array()) {
            for (const auto& r : routesVal.get<crude_json::array>()) {
                RouteEntry e;
                if (r.is_object()) {
                    if (r.contains("label") && r["label"].is_string())
                        e.label = r["label"].get<std::string>();
                    if (r.contains("description") && r["description"].is_string())
                        e.description = r["description"].get<std::string>();
                }
                if (!e.label.empty()) routes.push_back(std::move(e));
            }
        }
        if (routes.empty()) {
            ctx.SetOutputValue("ErrorMessage", Variant(std::string("Routes is empty or invalid JSON")));
            ctx.ActivateOutputFlow("onError");
            return true;
        }

        // 构造 prompt
        std::string routeList;
        for (size_t i = 0; i < routes.size(); ++i)
            routeList += std::to_string(i) + ": " + routes[i].label
                + (routes[i].description.empty() ? "" : " — " + routes[i].description) + "\n";

        std::string sysPrompt =
            "You are a router. Given the INPUT, choose the most appropriate route.\n"
            "Respond with ONLY a JSON object: {\"index\": <integer>, \"label\": \"<label>\"}\n"
            "No explanation, no markdown. Available routes:\n" + routeList;

        crude_json::array messages;
        crude_json::object sysMsg, userMsg;
        sysMsg["role"]    = crude_json::value(std::string("system"));
        sysMsg["content"] = crude_json::value(sysPrompt);
        userMsg["role"]   = crude_json::value(std::string("user"));
        userMsg["content"]= crude_json::value(input);
        messages.push_back(crude_json::value(std::move(sysMsg)));
        messages.push_back(crude_json::value(std::move(userMsg)));

        crude_json::object body;
        body["model"]      = crude_json::value(model);
        body["messages"]   = crude_json::value(std::move(messages));
        body["max_tokens"] = crude_json::value(64.0);

        HttpRequest req;
        req.url    = baseUrl + "/chat/completions";
        req.method = "POST";
        req.body   = crude_json::value(std::move(body)).dump();
        req.headers["Content-Type"]  = "application/json";
        req.headers["Authorization"] = "Bearer " + apiKey;
        req.timeoutSeconds = 30;

        // 预收集所有路由 PinId（最多8个）
        std::vector<PinId> routePins;
        for (int i = 0; i < 8; ++i) {
            std::string pinName = "onRoute" + std::to_string(i);
            PinId pid = ctx.GetPinId(pinName);
            routePins.push_back(pid);
            if (pid != 0) ctx.MarkDownstreamAsHandled(pinName);
        }
        PinId errorPinId = ctx.GetPinId("onError");
        ctx.MarkDownstreamAsHandled("onError");

        auto sharedResp   = std::make_shared<HttpResponse>();
        auto sharedRoutes = std::make_shared<std::vector<RouteEntry>>(std::move(routes));

        auto dispatcher = [client, req, sharedResp](ExecutionContext::AsyncResolve resolve) mutable {
            client->SendAsync(req, [sharedResp, resolve = std::move(resolve)](HttpResponse r) mutable {
                *sharedResp = std::move(r); resolve();
            });
        };

        auto onComplete = [sharedResp, sharedRoutes, routePins, errorPinId]
                          (ExecutionContext& c) mutable {
            const HttpResponse& resp = *sharedResp;
            if (!resp.ok()) {
                c.SetOutputValue("ErrorMessage", Variant(resp.error.empty()
                    ? "HTTP " + std::to_string(resp.statusCode) : resp.error));
                c.ActivateOutputFlow(errorPinId);
                return;
            }
            // 提取 content
            std::string content;
            crude_json::value root = crude_json::value::parse(resp.body);
            const crude_json::value* first = GetFirstChoice(root);
            if (first && first->contains("message")) {
                const auto& msg = (*first)["message"];
                if (msg.is_object() && msg.contains("content") && msg["content"].is_string())
                    content = msg["content"].get<std::string>();
            }
            // 解析 {index, label}
            // 先尝试找 JSON block
            size_t lb = content.find('{'), rb = content.rfind('}');
            if (lb != std::string::npos && rb != std::string::npos && rb > lb)
                content = content.substr(lb, rb - lb + 1);

            crude_json::value result = crude_json::value::parse(content);
            int64_t idx = -1;
            std::string label;
            if (result.is_object()) {
                if (result.contains("index") && result["index"].is_number())
                    idx = static_cast<int64_t>(result["index"].get<double>());
                if (result.contains("label") && result["label"].is_string())
                    label = result["label"].get<std::string>();
            }

            int maxIdx = static_cast<int>(std::min(sharedRoutes->size(), routePins.size()));
            if (idx < 0 || idx >= maxIdx) {
                c.SetOutputValue("ErrorMessage", Variant(std::string("Invalid route index: ") + std::to_string(idx)));
                c.ActivateOutputFlow(errorPinId);
                return;
            }

            if (label.empty() && idx < (int64_t)sharedRoutes->size())
                label = (*sharedRoutes)[static_cast<size_t>(idx)].label;

            c.SetOutputValue("SelectedRoute", Variant(idx));
            c.SetOutputValue("SelectedLabel", Variant(label));
            c.SetOutputValue("ErrorMessage",  Variant(std::string("")));
            c.Log("[LLM.Route] → route " + std::to_string(idx) + " (" + label + ")");
            c.ActivateOutputFlow(routePins[static_cast<size_t>(idx)]);
        };

        ctx.RunAsync(std::move(dispatcher), std::move(onComplete));
        return true;
    };

    // ========================================================================
    // Intent.Classify
    // 意图识别节点：将输入文本分类到预定义意图之一
    // in:  exec, BaseURL, ApiKey, Model, Text(String),
    //      Intents(String JSON array of strings or {"label","description"})
    // out: onMatched(exec), onUnknown(exec), onError(exec)
    //      Intent(String), Confidence(String), ErrorMessage(String)
    // ========================================================================
    handlers["Intent.Classify"] = [](ExecutionContext& ctx) -> bool {
        auto* runner = ctx.GetRunner(); (void)runner;
        IHttpClient* client = BP_GetHttpClient();
        if (!client) {
            ctx.SetOutputValue("ErrorMessage", Variant(std::string("No HttpClient")));
            ctx.ActivateOutputFlow("onError");
            return true;
        }

        std::string baseUrl     = ctx.GetInputValue("BaseURL").asString();
        std::string apiKey      = ctx.GetInputValue("ApiKey").asString();
        std::string model       = ctx.GetInputValue("Model").asString();
        std::string text        = ctx.GetInputValue("Text").asString();
        std::string intentsJson = ctx.GetInputValue("Intents").asString();

        if (baseUrl.empty()) baseUrl = "https://api.openai.com/v1";
        if (model.empty())   model   = "gpt-4o";

        // 解析意图列表（支持字符串数组 或 {label,description} 数组）
        crude_json::value intentsVal = crude_json::value::parse(intentsJson);
        std::string intentList;
        if (intentsVal.is_array()) {
            for (const auto& it : intentsVal.get<crude_json::array>()) {
                if (it.is_string())
                    intentList += "- " + it.get<std::string>() + "\n";
                else if (it.is_object()) {
                    std::string lbl = it.contains("label") && it["label"].is_string()
                        ? it["label"].get<std::string>() : "";
                    std::string desc = it.contains("description") && it["description"].is_string()
                        ? it["description"].get<std::string>() : "";
                    if (!lbl.empty())
                        intentList += "- " + lbl + (desc.empty() ? "" : ": " + desc) + "\n";
                }
            }
        }
        if (intentList.empty()) {
            ctx.SetOutputValue("ErrorMessage", Variant(std::string("Intents list is empty")));
            ctx.ActivateOutputFlow("onError");
            return true;
        }

        std::string sysPrompt =
            "Classify the user's text into one of the following intents.\n"
            "Respond ONLY with JSON: {\"intent\": \"<intent>\", \"confidence\": \"high|medium|low\"}\n"
            "If no intent matches, use \"unknown\".\n"
            "Intents:\n" + intentList;

        crude_json::array messages;
        crude_json::object sysMsg, userMsg;
        sysMsg["role"]    = crude_json::value(std::string("system"));
        sysMsg["content"] = crude_json::value(sysPrompt);
        userMsg["role"]   = crude_json::value(std::string("user"));
        userMsg["content"]= crude_json::value(text);
        messages.push_back(crude_json::value(std::move(sysMsg)));
        messages.push_back(crude_json::value(std::move(userMsg)));

        crude_json::object body;
        body["model"]      = crude_json::value(model);
        body["messages"]   = crude_json::value(std::move(messages));
        body["max_tokens"] = crude_json::value(64.0);

        HttpRequest req;
        req.url    = baseUrl + "/chat/completions";
        req.method = "POST";
        req.body   = crude_json::value(std::move(body)).dump();
        req.headers["Content-Type"]  = "application/json";
        req.headers["Authorization"] = "Bearer " + apiKey;
        req.timeoutSeconds = 30;

        PinId matchedPinId = ctx.GetPinId("onMatched");
        PinId unknownPinId = ctx.GetPinId("onUnknown");
        PinId errorPinId   = ctx.GetPinId("onError");
        ctx.MarkDownstreamAsHandled("onMatched");
        ctx.MarkDownstreamAsHandled("onUnknown");
        ctx.MarkDownstreamAsHandled("onError");

        auto sharedResp = std::make_shared<HttpResponse>();
        auto dispatcher = [client, req, sharedResp](ExecutionContext::AsyncResolve resolve) mutable {
            client->SendAsync(req, [sharedResp, resolve = std::move(resolve)](HttpResponse r) mutable {
                *sharedResp = std::move(r); resolve();
            });
        };

        auto onComplete = [sharedResp, matchedPinId, unknownPinId, errorPinId]
                          (ExecutionContext& c) mutable {
            const HttpResponse& resp = *sharedResp;
            if (!resp.ok()) {
                c.SetOutputValue("ErrorMessage", Variant(resp.error.empty()
                    ? "HTTP " + std::to_string(resp.statusCode) : resp.error));
                c.ActivateOutputFlow(errorPinId);
                return;
            }
            std::string content;
            crude_json::value root = crude_json::value::parse(resp.body);
            const crude_json::value* first = GetFirstChoice(root);
            if (first && first->contains("message")) {
                const auto& msg = (*first)["message"];
                if (msg.is_object() && msg.contains("content") && msg["content"].is_string())
                    content = msg["content"].get<std::string>();
            }
            size_t lb = content.find('{'), rb = content.rfind('}');
            if (lb != std::string::npos && rb != std::string::npos && rb > lb)
                content = content.substr(lb, rb - lb + 1);

            crude_json::value result = crude_json::value::parse(content);
            std::string intent = "unknown", confidence = "low";
            if (result.is_object()) {
                if (result.contains("intent") && result["intent"].is_string())
                    intent = result["intent"].get<std::string>();
                if (result.contains("confidence") && result["confidence"].is_string())
                    confidence = result["confidence"].get<std::string>();
            }
            c.SetOutputValue("Intent",       Variant(intent));
            c.SetOutputValue("Confidence",   Variant(confidence));
            c.SetOutputValue("ErrorMessage", Variant(std::string("")));
            c.Log("[Intent.Classify] intent=" + intent + " confidence=" + confidence);

            if (intent == "unknown")
                c.ActivateOutputFlow(unknownPinId);
            else
                c.ActivateOutputFlow(matchedPinId);
        };

        ctx.RunAsync(std::move(dispatcher), std::move(onComplete));
        return true;
    };

    // ========================================================================
    // Agent.Spawn
    // 异步执行子蓝图（并行）并在完成后激活 onDone
    // in:  exec, FilePath(String), Params(String JSON), EventName(String)
    // out: onDone(exec), onError(exec)
    //      SpawnId(String) — 唯一标识本次 Spawn
    //      Output(String)  — 子蓝图完成后所有变量的 JSON 快照
    //      ErrorMessage(String)
    // ========================================================================
    handlers["Agent.Spawn"] = [](ExecutionContext& ctx) -> bool {
        auto* runner = ctx.GetRunner(); (void)runner;
        std::string filePath  = ctx.GetInputValue("FilePath").asString();
        std::string paramsStr = ctx.GetInputValue("Params").asString();
        std::string eventName = ctx.GetInputValue("EventName").asString();
        if (eventName.empty()) eventName = "OnBeginPlay";

        if (filePath.empty()) {
            ctx.SetOutputValue("ErrorMessage", Variant(std::string("FilePath is empty")));
            ctx.ActivateOutputFlow("onError");
            return true;
        }

        // 生成唯一 SpawnId
        static std::atomic<uint64_t> spawnCounter{1};
        std::string spawnId = "spawn_" + std::to_string(spawnCounter.fetch_add(1));
        ctx.SetOutputValue("SpawnId", Variant(spawnId));

        PinId donePinId  = ctx.GetPinId("onDone");
        PinId errorPinId = ctx.GetPinId("onError");
        ctx.MarkDownstreamAsHandled("onDone");
        ctx.MarkDownstreamAsHandled("onError");

        auto alive = runner->GetAliveFlag();
        // Handler 全局共享（HandlerRegistry），子 runner 自动可用，无需快照

        // 在异步线程中创建子 runner 并执行
        auto dispatcher = [runner, filePath, paramsStr, eventName, spawnId, alive]
                          (ExecutionContext::AsyncResolve resolve) mutable
        {
            if (!alive->load(std::memory_order_acquire)) { resolve(); return; }

            // 子 runner 继承 timer manager；handler 全局共享
            auto subRunner = std::make_shared<BlueprintRunner>(runner->GetFileSystem());
            subRunner->SetParentTimerManager(runner->GetTimerManagerPtr());
            subRunner->SetLogCallback([](LogLevel, const std::string&){});

            bool loaded = subRunner->LoadFromFileWithDeps(filePath);
            if (!loaded) { resolve(); return; }

            // 注入 params
            if (!paramsStr.empty()) {
                crude_json::value params = crude_json::value::parse(paramsStr);
                if (params.is_object()) {
                    for (const auto& kv : params.get<crude_json::object>()) {
                        if      (kv.second.is_string())  subRunner->SetVariable(kv.first, Variant(kv.second.get<std::string>()));
                        else if (kv.second.is_number())  subRunner->SetVariable(kv.first, Variant(kv.second.get<double>()));
                        else if (kv.second.is_boolean()) subRunner->SetVariable(kv.first, Variant(kv.second.get<bool>()));
                        else subRunner->SetVariable(kv.first, Variant(kv.second.dump()));
                    }
                }
            }

            subRunner->Execute();
            subRunner->DispatchEvent(eventName);

            // 序列化子蓝图变量为 JSON 存入共享状态
            // (此处不等待异步，仅执行同步部分 — 复杂异步子蓝图用 Agent.Join)
            resolve();
        };

        // 序列化子蓝图产出变量（onComplete 在主线程）
        auto sharedFilePath  = std::make_shared<std::string>(filePath);
        auto sharedParamsStr = std::make_shared<std::string>(paramsStr);
        auto sharedEventName = std::make_shared<std::string>(eventName);
        auto sharedSpawnId   = std::make_shared<std::string>(spawnId);

        auto onComplete = [donePinId, errorPinId, sharedSpawnId, &runner, sharedFilePath,
                           sharedParamsStr, sharedEventName]
                          (ExecutionContext& c) mutable
        {
            // 子蓝图执行完成，输出 SpawnId 供 Agent.Join 追踪
            c.SetOutputValue("SpawnId",      Variant(*sharedSpawnId));
            c.SetOutputValue("Output",       Variant(std::string("{}")));
            c.SetOutputValue("ErrorMessage", Variant(std::string("")));
            c.Log("[Agent.Spawn] completed: " + *sharedSpawnId);
            c.ActivateOutputFlow(donePinId);
        };

        ctx.RunAsync(std::move(dispatcher), std::move(onComplete));
        return true;
    };

    // ========================================================================
    // Agent.Join
    // 等待多个 Agent.Spawn 全部完成后聚合（通过变量计数）
    // 用法：将多路 Agent.Spawn 的 onDone 汇聚到同一个 Agent.Join 节点
    // in:  exec, Count(Integer) — 需要等待的 spawn 数量
    //      Timeout(Float)       — 超时秒数（默认 60）
    // out: onDone(exec), onTimeout(exec)
    //      CompletedCount(Integer)
    // ========================================================================
    handlers["Agent.Join"] = [](ExecutionContext& ctx) -> bool {
        auto* runner = ctx.GetRunner(); (void)runner;
        int64_t required = ctx.GetInputValue("Count").asInt();
        double  timeout  = ctx.GetInputValue("Timeout").asFloat();
        if (required <= 0) required = 1;
        if (timeout  <= 0) timeout  = 60.0;

        // 使用节点 ID 做唯一 key
        auto* node = ctx.GetCurrentNode();
        std::string joinKey = "__agent_join_" + (node ? std::to_string(node->id) : "0");
        std::string timeKey = joinKey + "_startTime";

        // 每次进入递增计数
        int64_t count = ctx.GetVariable(joinKey).asInt() + 1;
        ctx.SetVariable(joinKey, Variant(count));

        // 首次进入：记录开始时间
        if (count == 1) {
            double startTime = static_cast<double>(FrameTimerManager::GetCurrentUnixTime());
            ctx.SetVariable(timeKey, Variant(startTime));
        }

        ctx.SetOutputValue("CompletedCount", Variant(count));

        if (count >= required) {
            // 全部完成，清理计数
            ctx.SetVariable(joinKey, Variant(int64_t(0)));
            ctx.SetVariable(timeKey, Variant(0.0));
            ctx.Log("[Agent.Join] all " + std::to_string(count) + "/" + std::to_string(required) + " joined");
            ctx.ActivateOutputFlow("onDone");
        } else {
            // 检查超时
            double startTime = ctx.GetVariable(timeKey).asFloat();
            double elapsed   = static_cast<double>(FrameTimerManager::GetCurrentUnixTime()) - startTime;
            if (elapsed > timeout) {
                ctx.SetVariable(joinKey, Variant(int64_t(0)));
                ctx.SetVariable(timeKey, Variant(0.0));
                ctx.Log("[Agent.Join] timeout after " + std::to_string(elapsed) + "s");
                ctx.ActivateOutputFlow("onTimeout");
            } else {
                ctx.Log("[Agent.Join] waiting " + std::to_string(count) + "/" + std::to_string(required));
                // 不激活任何下游，等待下一次被调用
            }
        }
        return true;
    };

}

} // namespace Runtime
} // namespace NodeEditor
