// Auto-generated from BuiltinHandlers_AI.cpp split
#include "BuiltinHandlers_AI_Internal.h"

namespace NodeEditor {
namespace Runtime {

void RegisterHandlers_AI_LLM(
    std::unordered_map<std::string, NodeHandler>& handlers,
    BlueprintRunner& runner)
{

    // ========================================================================
    // LLM.Chat
    // 输入：
    //   BaseURL(String)   — API 基础 URL，默认 "https://api.openai.com/v1"
    //   ApiKey(String)    — Bearer token
    //   Model(String)     — 模型名，如 "gpt-4o"
    //   Messages(String)  — JSON 数组字符串 [{"role":"user","content":"..."}]
    //   MaxTokens(Integer)— 最大输出 token 数，默认 1024
    //   Temperature(Float)— 温度，默认 0.7
    //   SystemPrompt(String) — 可选，自动拼到 messages 第一条 system 消息
    // 输出：
    //   → onReply        — 成功时走这条线
    //   → onError        — 失败时走这条线
    //   Reply(String)    — 模型回复文本（choices[0].message.content）
    //   FullResponse(String) — 完整响应 JSON
    //   ErrorMessage(String) — 错误描述
    //
    // 异步：通过 RunAsync(dispatcher, onComplete) 实现跨平台异步。
    // ========================================================================
    handlers["LLM.Chat"] = [&runner](ExecutionContext& ctx) -> bool {

        IHttpClient* client = BP_GetHttpClient();
        if (!client) {
            ctx.LogError("[LLM.Chat] No HttpClient registered. Call BP_SetHttpClient() first.");
            ctx.SetOutputValue("Reply",        Variant(std::string("")));
            ctx.SetOutputValue("FullResponse", Variant(std::string("")));
            ctx.SetOutputValue("ErrorMessage", Variant(std::string("No HttpClient registered")));
            ctx.ActivateOutputFlow("onError");
            return true;
        }

        HttpRequest req = BuildLLMRequest(ctx);

        PinId replyPinId    = ctx.GetPinId("onReply");
        PinId toolPinId     = ctx.GetPinId("onToolCall");
        PinId errorPinId    = ctx.GetPinId("onError");

        ctx.MarkDownstreamAsHandled("onReply");
        ctx.MarkDownstreamAsHandled("onToolCall");
        ctx.MarkDownstreamAsHandled("onError");

        auto sharedResp = std::make_shared<HttpResponse>();

        // dispatcher：发起请求，完成后 resolve
        auto dispatcher = [client, req, sharedResp](ExecutionContext::AsyncResolve resolve) mutable {
            client->SendAsync(req,
                [sharedResp, resolve = std::move(resolve)](HttpResponse resp) mutable {
                    *sharedResp = std::move(resp);
                    resolve();
                });
        };

        // onComplete：主线程 Tick 上下文，解析回复并激活下游
        auto onComplete = [sharedResp, replyPinId, toolPinId, errorPinId](ExecutionContext& c) mutable {
            const HttpResponse& resp = *sharedResp;

            c.SetOutputValue("FullResponse", Variant(resp.body));

            if (!resp.ok()) {
                std::string errMsg = resp.error.empty()
                    ? ("HTTP " + std::to_string(resp.statusCode))
                    : resp.error;
                if (!resp.body.empty()) {
                    crude_json::value j = crude_json::value::parse(resp.body);
                    if (j.is_object() && j.contains("error")) {
                        const auto& e = j["error"];
                        if (e.is_object() && e.contains("message"))
                            errMsg = e["message"].get<std::string>();
                    }
                }
                c.SetOutputValue("Reply",         Variant(std::string("")));
                c.SetOutputValue("ToolCallsJSON", Variant(std::string("[]")));
                c.SetOutputValue("FinishReason",  Variant(std::string("error")));
                c.SetOutputValue("ErrorMessage",  Variant(errMsg));
                c.LogError("[LLM.Chat] " + errMsg);
                c.ActivateOutputFlow(errorPinId);
                return;
            }

            crude_json::value j = crude_json::value::parse(resp.body);

            // finish_reason
            std::string finishReason;
            const crude_json::value* first = GetFirstChoice(j);
            if (first && first->contains("finish_reason")) {
                const auto& fr = (*first)["finish_reason"];
                if (fr.is_string()) finishReason = fr.get<std::string>();
            }
            c.SetOutputValue("FinishReason", Variant(finishReason));

            // ── tool_calls 分支 ──────────────────────────────────────
            if (finishReason == "tool_calls") {
                std::string toolCallsJson = "[]";
                if (first && first->contains("message")) {
                    const auto& msg = (*first)["message"];
                    if (msg.is_object() && msg.contains("tool_calls"))
                        toolCallsJson = msg["tool_calls"].dump();
                }
                c.SetOutputValue("Reply",         Variant(std::string("")));
                c.SetOutputValue("ToolCallsJSON", Variant(toolCallsJson));
                c.SetOutputValue("ErrorMessage",  Variant(std::string("")));
                c.Log("[LLM.Chat] tool_calls: " + toolCallsJson.substr(0, 120));
                c.ActivateOutputFlow(toolPinId);
                return;
            }

            // ── 普通文本回复 ─────────────────────────────────────────
            std::string reply;
            if (first && first->contains("message")) {
                const auto& msg = (*first)["message"];
                if (msg.is_object() && msg.contains("content")) {
                    const auto& content = msg["content"];
                    if (content.is_string()) reply = content.get<std::string>();
                }
            }
            c.SetOutputValue("Reply",         Variant(reply));
            c.SetOutputValue("ToolCallsJSON", Variant(std::string("[]")));
            c.SetOutputValue("ErrorMessage",  Variant(std::string("")));
            c.Log("[LLM.Chat] reply length=" + std::to_string(reply.size()));
            c.ActivateOutputFlow(replyPinId);
        };

        ctx.RunAsync(std::move(dispatcher), std::move(onComplete));
        return true;
    };

    // ================================================================
    // LLM.Auto
    //   从配置文件读取多个 provider，按 priority 依次尝试，第一个成功的返回结果。
    //   配置文件格式（JSON）：
    //   {
    //     "providers": [
    //       { "name":"GLM","baseURL":"...","apiKey":"...","model":"...","priority":1 },
    //       { "name":"DeepSeek","baseURL":"...","apiKey":"...","model":"...","priority":2 }
    //     ]
    //   }
    //   in:  exec, ConfigFile(String), Messages(String), SystemPrompt(String),
    //        MaxTokens(Integer), Temperature(Float), Tools(String)
    //   out: onReply, onToolCall, onError
    //        Reply(String), ToolCallsJSON(String), FinishReason(String),
    //        Provider(String), ErrorMessage(String)
    // ================================================================
    handlers["LLM.Auto"] = [](ExecutionContext& ctx) -> bool {
        IHttpClient* client = BP_GetHttpClient();
        if (!client) {
            ctx.SetOutputValue("ErrorMessage", Variant(std::string("No HttpClient registered")));
            ctx.ActivateOutputFlow("onError");
            return true;
        }

        // 读取配置文件
        std::string configFile = ctx.GetInputValue("ConfigFile").asString();
        if (configFile.empty()) {
            ctx.SetOutputValue("ErrorMessage", Variant(std::string("ConfigFile is empty")));
            ctx.ActivateOutputFlow("onError");
            return true;
        }

#ifdef __EMSCRIPTEN__
        ctx.SetOutputValue("ErrorMessage", Variant(std::string("LLM.Auto: file I/O not supported on WebGL")));
        ctx.ActivateOutputFlow("onError");
        return true;
#else
        auto* fs = GetDefaultFileSystem().get();
        std::string cfgContent, cfgErr;
        if (!fs || !fs->ReadFile(configFile, cfgContent, cfgErr)) {
            ctx.SetOutputValue("ErrorMessage", Variant(std::string("Cannot open config file: " + configFile + (cfgErr.empty() ? "" : " (" + cfgErr + ")"))));
            ctx.ActivateOutputFlow("onError");
            return true;
        }
        crude_json::value cfg = crude_json::value::parse(cfgContent);

        // 解析 providers 列表
        struct Provider { std::string name, baseURL, apiKey, model; int priority = 99; };
        std::vector<Provider> providers;

        if (cfg.is_object() && cfg.contains("providers")) {
            const auto& arr = cfg["providers"];
            if (arr.is_array()) {
                for (const auto& p : arr.get<crude_json::array>()) {
                    if (!p.is_object()) continue;
                    Provider pv;
                    if (p.contains("name")     && p["name"].is_string())     pv.name     = p["name"].get<std::string>();
                    if (p.contains("baseURL")  && p["baseURL"].is_string())  pv.baseURL  = p["baseURL"].get<std::string>();
                    if (p.contains("apiKey")   && p["apiKey"].is_string())   pv.apiKey   = p["apiKey"].get<std::string>();
                    if (p.contains("model")    && p["model"].is_string())    pv.model    = p["model"].get<std::string>();
                    if (p.contains("priority") && p["priority"].is_number()) pv.priority = (int)p["priority"].get<double>();
                    if (!pv.baseURL.empty() && !pv.model.empty())
                        providers.push_back(std::move(pv));
                }
            }
        }

        if (providers.empty()) {
            ctx.SetOutputValue("ErrorMessage", Variant(std::string("No valid providers in config file")));
            ctx.ActivateOutputFlow("onError");
            return true;
        }

        // 如果 Provider 引脚指定了名字（非空且非 "auto"），只保留该 provider
        std::string providerHint = ctx.GetInputValue("Provider").asString();
        if (!providerHint.empty() && providerHint != "auto") {
            std::vector<Provider> filtered;
            for (auto& p : providers)
                if (p.name == providerHint) { filtered.push_back(std::move(p)); break; }
            if (filtered.empty()) {
                ctx.SetOutputValue("ErrorMessage", Variant(std::string("Provider not found: " + providerHint)));
                ctx.ActivateOutputFlow("onError");
                return true;
            }
            providers = std::move(filtered);
        } else {
            // auto 模式：按 priority 排序
            std::sort(providers.begin(), providers.end(),
                [](const Provider& a, const Provider& b) { return a.priority < b.priority; });
        }

        // 共享状态
        struct AutoState {
            std::vector<Provider> providers;
            size_t index = 0;
            HttpResponse resp;
            std::string successProvider;
        };
        auto state = std::make_shared<AutoState>();
        state->providers = std::move(providers);

        PinId replyPinId    = ctx.GetPinId("onReply");
        PinId toolPinId     = ctx.GetPinId("onToolCall");
        PinId errorPinId    = ctx.GetPinId("onError");

        ctx.MarkDownstreamAsHandled("onReply");
        ctx.MarkDownstreamAsHandled("onToolCall");
        ctx.MarkDownstreamAsHandled("onError");

        // dispatcher：依次尝试每个 provider，直到成功
        auto dispatcher = [client, state, &ctx](ExecutionContext::AsyncResolve resolve) mutable {
            // 递归 lambda：尝试 state->index 对应的 provider
            struct Retry {
                static void attempt(
                    IHttpClient* client,
                    std::shared_ptr<AutoState> state,
                    ExecutionContext& ctx,
                    ExecutionContext::AsyncResolve resolve)
                {
                    if (state->index >= state->providers.size()) {
                        resolve(); // 全部失败，进入 onComplete
                        return;
                    }
                    const auto& pv = state->providers[state->index];
                    HttpRequest req = BuildLLMRequest(ctx, pv.baseURL, pv.apiKey, pv.model);

                    client->SendAsync(req,
                        [client, state, &ctx, resolve = std::move(resolve)](HttpResponse resp) mutable {
                            if (resp.ok()) {
                                // 成功：记录结果
                                state->successProvider = state->providers[state->index].name;
                                state->resp = std::move(resp);
                                resolve();
                            } else {
                                // 失败：尝试下一个
                                ctx.Log("[LLM.Auto] provider '" + state->providers[state->index].name
                                    + "' failed (" + (resp.error.empty()
                                        ? "HTTP " + std::to_string(resp.statusCode)
                                        : resp.error) + "), trying next...",
                                    ::NodeEditor::Runtime::LogLevel::Warning);
                                ++state->index;
                                Retry::attempt(client, state, ctx, std::move(resolve));
                            }
                        });
                }
            };
            Retry::attempt(client, state, ctx, std::move(resolve));
        };

        // onComplete：解析回复（与 LLM.Chat 相同逻辑）
        auto onComplete = [state, replyPinId, toolPinId, errorPinId](ExecutionContext& c) mutable {
            if (state->successProvider.empty()) {
                // 全部失败
                c.SetOutputValue("Reply",         Variant(std::string("")));
                c.SetOutputValue("ToolCallsJSON", Variant(std::string("[]")));
                c.SetOutputValue("FinishReason",  Variant(std::string("error")));
                c.SetOutputValue("UsedProvider",  Variant(std::string("")));
                c.SetOutputValue("ErrorMessage",  Variant(std::string("All providers failed")));
                c.LogError("[LLM.Auto] All providers failed");
                c.ActivateOutputFlow(errorPinId);
                return;
            }

            const HttpResponse& resp = state->resp;
            c.SetOutputValue("UsedProvider", Variant(state->successProvider));

            crude_json::value j = crude_json::value::parse(resp.body);

            // finish_reason
            std::string finishReason;
            const crude_json::value* first = GetFirstChoice(j);
            if (first && first->contains("finish_reason")) {
                const auto& fr = (*first)["finish_reason"];
                if (fr.is_string()) finishReason = fr.get<std::string>();
            }
            c.SetOutputValue("FinishReason", Variant(finishReason));

            // tool_calls 分支
            if (finishReason == "tool_calls") {
                std::string toolCallsJson = "[]";
                if (first && first->contains("message")) {
                    const auto& msg = (*first)["message"];
                    if (msg.is_object() && msg.contains("tool_calls"))
                        toolCallsJson = msg["tool_calls"].dump();
                }
                c.SetOutputValue("Reply",         Variant(std::string("")));
                c.SetOutputValue("ToolCallsJSON", Variant(toolCallsJson));
                c.SetOutputValue("ErrorMessage",  Variant(std::string("")));
                c.ActivateOutputFlow(toolPinId);
                return;
            }

            // 普通文本回复
            std::string reply;
            if (first && first->contains("message")) {
                const auto& msg = (*first)["message"];
                if (msg.is_object() && msg.contains("content")) {
                    const auto& content = msg["content"];
                    if (content.is_string()) reply = content.get<std::string>();
                }
            }
            c.SetOutputValue("Reply",         Variant(reply));
            c.SetOutputValue("ToolCallsJSON", Variant(std::string("[]")));
            c.SetOutputValue("ErrorMessage",  Variant(std::string("")));
            c.Log("[LLM.Auto] used provider='" + state->successProvider
                  + "', reply length=" + std::to_string(reply.size()));
            c.ActivateOutputFlow(replyPinId);
        };

        ctx.RunAsync(std::move(dispatcher), std::move(onComplete));
        return true;
#endif
    };

    // ================================================================
    // LLM.AutoStream
    //   流式版 LLM.Auto：多 provider fallback + SSE 逐 token 推送
    //   策略：依次尝试每个 provider 的流式请求；某个 provider 开始
    //         收到 chunk 即视为"成功"，后续不再 fallback。
    //   in:  ConfigFile, Provider, Messages, SystemPrompt,
    //        MaxTokens, Temperature, Tools
    //   exec out:
    //     onChunk(Token)     — 每个 token
    //     onToolCall         — finish_reason=tool_calls
    //     onDone(FullText)   — 全部完成
    //     onError(ErrorMessage)
    //   out: Token / FullText / ToolCallsJSON / UsedProvider / ErrorMessage
    // ================================================================
    handlers["LLM.AutoStream"] = [](ExecutionContext& ctx) -> bool {
        auto* client = BP_GetHttpClient();
        if (!client) {
            ctx.SetOutputValue("ErrorMessage", Variant(std::string("No HttpClient registered")));
            ctx.ActivateOutputFlow("onError");
            return true;
        }

        // ── 读取并解析配置文件（同 LLM.Auto）──────────────────────────
        std::string configFile = ctx.GetInputValue("ConfigFile").asString();
        if (configFile.empty()) {
            ctx.SetOutputValue("ErrorMessage", Variant(std::string("ConfigFile is empty")));
            ctx.ActivateOutputFlow("onError");
            return true;
        }

#ifdef __EMSCRIPTEN__
        ctx.SetOutputValue("ErrorMessage", Variant(std::string("LLM.AutoStream: file I/O not supported on WebGL")));
        ctx.ActivateOutputFlow("onError");
        return true;
#else
        auto* fs = GetDefaultFileSystem().get();
        std::string cfgContent, cfgErr;
        if (!fs || !fs->ReadFile(configFile, cfgContent, cfgErr)) {
            ctx.SetOutputValue("ErrorMessage", Variant(std::string("Cannot open config file: " + configFile + (cfgErr.empty() ? "" : " (" + cfgErr + ")"))));
            ctx.ActivateOutputFlow("onError");
            return true;
        }
        crude_json::value cfg = crude_json::value::parse(cfgContent);

        struct Provider { std::string name, baseURL, apiKey, model; int priority = 99; };
        std::vector<Provider> providers;
        if (cfg.is_object() && cfg.contains("providers")) {
            const auto& arr = cfg["providers"];
            if (arr.is_array()) {
                for (const auto& p : arr.get<crude_json::array>()) {
                    if (!p.is_object()) continue;
                    Provider pv;
                    if (p.contains("name")     && p["name"].is_string())     pv.name     = p["name"].get<std::string>();
                    if (p.contains("baseURL")  && p["baseURL"].is_string())  pv.baseURL  = p["baseURL"].get<std::string>();
                    if (p.contains("apiKey")   && p["apiKey"].is_string())   pv.apiKey   = p["apiKey"].get<std::string>();
                    if (p.contains("model")    && p["model"].is_string())    pv.model    = p["model"].get<std::string>();
                    if (p.contains("priority") && p["priority"].is_number()) pv.priority = (int)p["priority"].get<double>();
                    if (!pv.baseURL.empty() && !pv.model.empty())
                        providers.push_back(std::move(pv));
                }
            }
        }
        if (providers.empty()) {
            ctx.SetOutputValue("ErrorMessage", Variant(std::string("No valid providers in config file")));
            ctx.ActivateOutputFlow("onError");
            return true;
        }

        // 过滤指定 provider / 排序
        std::string providerHint = ctx.GetInputValue("Provider").asString();
        if (!providerHint.empty() && providerHint != "auto") {
            std::vector<Provider> filtered;
            for (auto& p : providers)
                if (p.name == providerHint) { filtered.push_back(std::move(p)); break; }
            if (filtered.empty()) {
                ctx.SetOutputValue("ErrorMessage", Variant(std::string("Provider not found: " + providerHint)));
                ctx.ActivateOutputFlow("onError");
                return true;
            }
            providers = std::move(filtered);
        } else {
            std::sort(providers.begin(), providers.end(),
                [](const Provider& a, const Provider& b){ return a.priority < b.priority; });
        }

        // ── Pin IDs ──────────────────────────────────────────────────────
        PinId chunkPinId    = ctx.GetPinId("onChunk");
        PinId toolCallPinId = ctx.GetPinId("onToolCall");
        PinId donePinId     = ctx.GetPinId("onDone");
        PinId errorPinId    = ctx.GetPinId("onError");
        ctx.MarkDownstreamAsHandled("onChunk");
        ctx.MarkDownstreamAsHandled("onToolCall");
        ctx.MarkDownstreamAsHandled("onDone");
        ctx.MarkDownstreamAsHandled("onError");

        // ── 共享状态 ──────────────────────────────────────────────────────
        struct AutoStreamState {
            std::vector<Provider> providers;
            size_t index = 0;
            // 流结果（复用 LLM.StreamChat 的 StreamResult 结构）
            std::vector<std::string> chunks;
            std::string finishReason;
            std::string errorMsg;
            std::string successProvider;
            struct ToolCallAccum { std::string id, name, arguments; };
            std::vector<ToolCallAccum> toolCalls;
        };
        auto state = std::make_shared<AutoStreamState>();
        state->providers = std::move(providers);

        // ── SSE chunk 解析 lambda（与 LLM.StreamChat 完全一致）────────────
        auto parseSSEChunk = [](std::shared_ptr<AutoStreamState> st, const std::string& sseData) {
            if (sseData == "[DONE]") return;
            auto v = crude_json::value::parse(sseData);
            if (!v.is_object() || !v.contains("choices")) return;
            const auto& choices = v["choices"];
            if (!choices.is_array() || choices.get<crude_json::array>().empty()) return;
            const auto& choice = choices.get<crude_json::array>()[0];
            if (!choice.is_object()) return;

            if (choice.contains("finish_reason")) {
                const auto& fr = choice["finish_reason"];
                if (fr.is_string() && !fr.get<std::string>().empty()
                    && fr.get<std::string>() != "null")
                    st->finishReason = fr.get<std::string>();
            }
            // Emscripten 降级：message
            if (choice.contains("message")) {
                const auto& msg = choice["message"];
                if (msg.is_object()) {
                    if (msg.contains("content") && msg["content"].is_string())
                        st->chunks.push_back(msg["content"].get<std::string>());
                    if (msg.contains("tool_calls") && msg["tool_calls"].is_array()) {
                        size_t idx = 0;
                        for (const auto& tc : msg["tool_calls"].get<crude_json::array>()) {
                            if (!tc.is_object()) { ++idx; continue; }
                            while (st->toolCalls.size() <= idx) st->toolCalls.push_back({});
                            auto& ac = st->toolCalls[idx];
                            if (tc.contains("id") && tc["id"].is_string()) ac.id = tc["id"].get<std::string>();
                            if (tc.contains("function") && tc["function"].is_object()) {
                                const auto& fn = tc["function"];
                                if (fn.contains("name") && fn["name"].is_string()) ac.name = fn["name"].get<std::string>();
                                if (fn.contains("arguments") && fn["arguments"].is_string()) ac.arguments = fn["arguments"].get<std::string>();
                            }
                            ++idx;
                        }
                    }
                }
                return;
            }
            // Native SSE delta
            if (choice.contains("delta")) {
                const auto& delta = choice["delta"];
                if (!delta.is_object()) return;
                if (delta.contains("content") && delta["content"].is_string()) {
                    const auto& s = delta["content"].get<std::string>();
                    if (!s.empty()) st->chunks.push_back(s);
                }
                if (delta.contains("tool_calls") && delta["tool_calls"].is_array()) {
                    for (const auto& tc : delta["tool_calls"].get<crude_json::array>()) {
                        if (!tc.is_object()) continue;
                        size_t idx = 0;
                        if (tc.contains("index") && tc["index"].is_number())
                            idx = (size_t)tc["index"].get<double>();
                        while (st->toolCalls.size() <= idx) st->toolCalls.push_back({});
                        auto& ac = st->toolCalls[idx];
                        if (tc.contains("id") && tc["id"].is_string() && ac.id.empty()) ac.id = tc["id"].get<std::string>();
                        if (tc.contains("function") && tc["function"].is_object()) {
                            const auto& fn = tc["function"];
                            if (fn.contains("name") && fn["name"].is_string()) ac.name += fn["name"].get<std::string>();
                            if (fn.contains("arguments") && fn["arguments"].is_string()) ac.arguments += fn["arguments"].get<std::string>();
                        }
                    }
                }
            }
        };

        // ── dispatcher：依次尝试每个 provider 的 StreamAsync ─────────────
        auto dispatcher = [client, state, &ctx, parseSSEChunk](ExecutionContext::AsyncResolve resolve) mutable {
            struct Retry {
                static void attempt(
                    IHttpClient* client,
                    std::shared_ptr<AutoStreamState> state,
                    ExecutionContext& ctx,
                    ExecutionContext::AsyncResolve resolve,
                    std::function<void(std::shared_ptr<AutoStreamState>, const std::string&)> parseChunk)
                {
                    if (state->index >= state->providers.size()) {
                        resolve();
                        return;
                    }
                    const auto& pv = state->providers[state->index];
                    HttpRequest req = BuildLLMRequest(ctx, pv.baseURL, pv.apiKey, pv.model);
                    // 清空上次可能残留的状态
                    state->chunks.clear();
                    state->toolCalls.clear();
                    state->finishReason.clear();
                    state->errorMsg.clear();

                    client->StreamAsync(req,
                        [state, parseChunk](const std::string& sseData) {
                            parseChunk(state, sseData);
                        },
                        [client, state, &ctx, resolve = std::move(resolve), parseChunk]
                        (const std::string& error) mutable {
                            if (error.empty()) {
                                // 流成功结束
                                state->successProvider = state->providers[state->index].name;
                                resolve();
                            } else {
                                // 流出错，尝试下一个
                                ctx.Log("[LLM.AutoStream] provider '" + state->providers[state->index].name
                                    + "' failed: " + error + ", trying next...",
                                    ::NodeEditor::Runtime::LogLevel::Warning);
                                ++state->index;
                                Retry::attempt(client, state, ctx, std::move(resolve), parseChunk);
                            }
                        });
                }
            };
            Retry::attempt(client, state, ctx, std::move(resolve), parseSSEChunk);
        };

        // ── onComplete：批量激活（与 LLM.StreamChat 的 onComplete 一致）──
        auto onComplete = [state, chunkPinId, toolCallPinId, donePinId, errorPinId]
            (ExecutionContext& c) mutable
        {
            if (state->successProvider.empty()) {
                c.SetOutputValue("Token",         Variant(std::string("")));
                c.SetOutputValue("FullText",      Variant(std::string("")));
                c.SetOutputValue("ToolCallsJSON", Variant(std::string("[]")));
                c.SetOutputValue("UsedProvider",  Variant(std::string("")));
                c.SetOutputValue("ErrorMessage",  Variant(std::string("All providers failed")));
                c.LogError("[LLM.AutoStream] All providers failed");
                c.ActivateOutputFlow(errorPinId);
                return;
            }

            c.SetOutputValue("UsedProvider",  Variant(state->successProvider));
            c.SetOutputValue("ErrorMessage",  Variant(std::string("")));

            // tool_calls 路径
            if (!state->toolCalls.empty() || state->finishReason == "tool_calls") {
                crude_json::array tcArr;
                for (const auto& tc : state->toolCalls) {
                    crude_json::object fn;
                    fn["name"]      = crude_json::value(tc.name);
                    fn["arguments"] = crude_json::value(tc.arguments);
                    crude_json::object obj;
                    obj["id"]       = crude_json::value(tc.id);
                    obj["type"]     = crude_json::value(std::string("function"));
                    obj["function"] = crude_json::value(std::move(fn));
                    tcArr.push_back(crude_json::value(std::move(obj)));
                }
                std::string tcJson = crude_json::value(std::move(tcArr)).dump();
                c.SetOutputValue("ToolCallsJSON", Variant(tcJson));
                c.SetOutputValue("FullText",      Variant(std::string("")));
                c.SetOutputValue("Token",         Variant(std::string("")));
                c.ActivateOutputFlow(toolCallPinId);
                return;
            }

            // 文本流路径：逐 token 激活 onChunk，最后激活 onDone
            std::string fullText;
            for (const auto& t : state->chunks) fullText += t;
            c.SetOutputValue("FullText",      Variant(fullText));
            c.SetOutputValue("ToolCallsJSON", Variant(std::string("")));
            c.Log("[LLM.AutoStream] provider='" + state->successProvider
                  + "', " + std::to_string(state->chunks.size())
                  + " chunks, total=" + std::to_string(fullText.size()) + " chars");

            for (const auto& tok : state->chunks) {
                c.SetOutputValue("Token", Variant(tok));
                c.ActivateOutputFlow(chunkPinId);
            }
            c.SetOutputValue("Token", Variant(std::string("")));
            c.ActivateOutputFlow(donePinId);
        };

        ctx.RunAsync(std::move(dispatcher), std::move(onComplete));
        return true;
#endif
    };

    // ================================================================
    // LLM.StreamChat
    //   流式 OpenAI 兼容 Chat（SSE，批量聚合模式）
    //   in:  BaseURL / ApiKey / Model / Messages / SystemPrompt /
    //        MaxTokens / Temperature / Tools（同 LLM.Chat）
    //   exec out:
    //     onChunk(Token)  — 后台流结束后在主线程逐 token 批量激活
    //     onDone          — 全部 chunk 激活完毕后触发
    //     onError(ErrorMessage)
    //   out: Token(String)    — 当前 chunk 文本（onChunk 时有效）
    //        FullText(String) — 完整拼合文本（onDone 时有效）
    //        ErrorMessage(String)
    //
    // 实现说明：
    //   用 RunAsync dispatcher/onComplete 模型，后台线程通过 StreamAsync
    //   收集所有 SSE chunks，流结束后 resolve()，onComplete（主线程）批量
    //   逐 token 激活 onChunk，最后激活 onDone。完全复用已有 RunAsync 机制。
    // ================================================================
    handlers["LLM.StreamChat"] = [](ExecutionContext& ctx) {
        auto* client = BP_GetHttpClient();
        if (!client) {
            ctx.SetOutputValue("Token",        Variant(std::string("")));
            ctx.SetOutputValue("FullText",     Variant(std::string("")));
            ctx.SetOutputValue("ErrorMessage", Variant(std::string("No HTTP client registered")));
            ctx.ActivateOutputFlow("onError");
            return true;
        }

        HttpRequest req = BuildLLMRequest(ctx);

        PinId chunkPinId    = ctx.GetPinId("onChunk");
        PinId toolCallPinId = ctx.GetPinId("onToolCall");
        PinId donePinId     = ctx.GetPinId("onDone");
        PinId errorPinId    = ctx.GetPinId("onError");

        ctx.MarkDownstreamAsHandled("onChunk");
        ctx.MarkDownstreamAsHandled("onToolCall");
        ctx.MarkDownstreamAsHandled("onDone");
        ctx.MarkDownstreamAsHandled("onError");

        // 共享状态：后台线程收集 chunks 和 SSE 原始行（用于解析 tool_calls delta）
        struct StreamResult {
            std::vector<std::string> chunks;    // delta.content 文本片段
            std::string              errorMsg;
            std::string              finishReason;
            // 流式 tool_calls 拼合：按 index 存储 {name, arguments} 累积字符串
            struct ToolCallAccum { std::string id; std::string name; std::string arguments; };
            std::vector<ToolCallAccum> toolCalls;
        };
        auto shared = std::make_shared<StreamResult>();

        // dispatcher：StreamAsync 收集所有 chunk，结束后 resolve
        auto dispatcher = [client, req, shared](ExecutionContext::AsyncResolve resolve) mutable {
            client->StreamAsync(req,
                // onChunk — 接收原始 SSE data JSON 字符串（Native），
                //           或完整响应 JSON（Emscripten 降级）
                [shared](const std::string& sseData) {
                    if (sseData == "[DONE]") return;
                    auto v = crude_json::value::parse(sseData);
                    if (!v.is_object()) return;

                    // 提取 finish_reason
                    if (v.contains("choices")) {
                        const auto& choices = v["choices"];
                        if (choices.is_array() && !choices.get<crude_json::array>().empty()) {
                            const auto& choice = choices.get<crude_json::array>()[0];
                            if (choice.is_object()) {
                                if (choice.contains("finish_reason")) {
                                    const auto& fr = choice["finish_reason"];
                                    if (fr.is_string() && fr.get<std::string>() != "null"
                                        && !fr.get<std::string>().empty())
                                        shared->finishReason = fr.get<std::string>();
                                }

                                // Emscripten 降级：choices[0].message（非流式完整响应）
                                if (choice.contains("message")) {
                                    const auto& msg = choice["message"];
                                    if (msg.is_object()) {
                                        if (msg.contains("content")) {
                                            const auto& c = msg["content"];
                                            if (c.is_string()) shared->chunks.push_back(c.get<std::string>());
                                        }
                                        if (msg.contains("tool_calls")) {
                                            const auto& tcArr = msg["tool_calls"];
                                            if (tcArr.is_array()) {
                                                size_t idx = 0;
                                                for (const auto& tc : tcArr.get<crude_json::array>()) {
                                                    if (!tc.is_object()) { ++idx; continue; }
                                                    while (shared->toolCalls.size() <= idx)
                                                        shared->toolCalls.push_back({});
                                                    auto& accum = shared->toolCalls[idx];
                                                    if (tc.contains("id")) {
                                                        const auto& iv = tc["id"];
                                                        if (iv.is_string()) accum.id = iv.get<std::string>();
                                                    }
                                                    if (tc.contains("function")) {
                                                        const auto& fn = tc["function"];
                                                        if (fn.is_object()) {
                                                            if (fn.contains("name")) {
                                                                const auto& nv = fn["name"];
                                                                if (nv.is_string()) accum.name = nv.get<std::string>();
                                                            }
                                                            if (fn.contains("arguments")) {
                                                                const auto& av = fn["arguments"];
                                                                if (av.is_string()) accum.arguments = av.get<std::string>();
                                                            }
                                                        }
                                                    }
                                                    ++idx;
                                                }
                                            }
                                        }
                                    }
                                    return; // Emscripten 降级，整块已处理完
                                }

                                // Native SSE 流式：choices[0].delta
                                if (choice.contains("delta")) {
                                    const auto& delta = choice["delta"];
                                    if (!delta.is_object()) return;

                                    // delta.content — 普通文本 token
                                    if (delta.contains("content")) {
                                        const auto& c = delta["content"];
                                        if (c.is_string()) {
                                            const auto& s = c.get<std::string>();
                                            if (!s.empty()) shared->chunks.push_back(s);
                                        }
                                    }

                                    // delta.tool_calls — 工具调用增量拼合
                                    if (delta.contains("tool_calls")) {
                                        const auto& tcArr = delta["tool_calls"];
                                        if (!tcArr.is_array()) return;
                                        for (const auto& tc : tcArr.get<crude_json::array>()) {
                                            if (!tc.is_object()) continue;
                                            size_t idx = 0;
                                            if (tc.contains("index")) {
                                                const auto& iv = tc["index"];
                                                if (iv.is_number())
                                                    idx = static_cast<size_t>(iv.get<double>());
                                            }
                                            while (shared->toolCalls.size() <= idx)
                                                shared->toolCalls.push_back({});
                                            auto& accum = shared->toolCalls[idx];

                                            if (tc.contains("id")) {
                                                const auto& idv = tc["id"];
                                                if (idv.is_string() && accum.id.empty())
                                                    accum.id = idv.get<std::string>();
                                            }
                                            if (tc.contains("function")) {
                                                const auto& fn = tc["function"];
                                                if (fn.is_object()) {
                                                    if (fn.contains("name")) {
                                                        const auto& nv = fn["name"];
                                                        if (nv.is_string()) accum.name += nv.get<std::string>();
                                                    }
                                                    if (fn.contains("arguments")) {
                                                        const auto& av = fn["arguments"];
                                                        if (av.is_string()) accum.arguments += av.get<std::string>();
                                                    }
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                },
                [shared, resolve = std::move(resolve)](const std::string& error) mutable {
                    shared->errorMsg = error;
                    resolve();
                });
        };

        // onComplete：主线程，批量激活
        auto onComplete = [shared, chunkPinId, toolCallPinId, donePinId, errorPinId](ExecutionContext& c) mutable {
            if (!shared->errorMsg.empty()) {
                c.SetOutputValue("Token",        Variant(std::string("")));
                c.SetOutputValue("FullText",     Variant(std::string("")));
                c.SetOutputValue("ToolCallsJSON",Variant(std::string("")));
                c.SetOutputValue("FinishReason", Variant(std::string("")));
                c.SetOutputValue("ErrorMessage", Variant(shared->errorMsg));
                c.LogError("[LLM.StreamChat] " + shared->errorMsg);
                c.ActivateOutputFlow(errorPinId);
                return;
            }

            c.SetOutputValue("FinishReason", Variant(shared->finishReason));
            c.SetOutputValue("ErrorMessage", Variant(std::string("")));

            // ── 工具调用路径 ─────────────────────────────────────────────
            if (!shared->toolCalls.empty() || shared->finishReason == "tool_calls") {
                // 将拼合好的 toolCalls 序列化为标准 tool_calls JSON 数组
                crude_json::array tcArr;
                for (const auto& tc : shared->toolCalls) {
                    crude_json::object fn;
                    fn["name"]      = crude_json::value(tc.name);
                    fn["arguments"] = crude_json::value(tc.arguments);

                    crude_json::object obj;
                    obj["id"]       = crude_json::value(tc.id);
                    obj["type"]     = crude_json::value(std::string("function"));
                    obj["function"] = crude_json::value(std::move(fn));
                    tcArr.push_back(crude_json::value(std::move(obj)));
                }
                std::string tcJson = crude_json::value(std::move(tcArr)).dump();
                c.SetOutputValue("ToolCallsJSON", Variant(tcJson));
                c.SetOutputValue("FullText",      Variant(std::string("")));
                c.SetOutputValue("Token",         Variant(std::string("")));
                c.Log("[LLM.StreamChat] tool_calls: " + tcJson);
                c.ActivateOutputFlow(toolCallPinId);
                return;
            }

            // ── 文本流路径 ───────────────────────────────────────────────
            std::string fullText;
            for (const auto& t : shared->chunks) fullText += t;

            c.SetOutputValue("FullText",      Variant(fullText));
            c.SetOutputValue("ToolCallsJSON", Variant(std::string("")));
            c.Log("[LLM.StreamChat] " + std::to_string(shared->chunks.size())
                  + " chunks, total=" + std::to_string(fullText.size()) + " chars");

            for (const auto& tok : shared->chunks) {
                c.SetOutputValue("Token", Variant(tok));
                c.ActivateOutputFlow(chunkPinId);
            }
            c.SetOutputValue("Token", Variant(std::string("")));
            c.ActivateOutputFlow(donePinId);
        };

        ctx.RunAsync(std::move(dispatcher), std::move(onComplete));
        return true;
    };

    // ================================================================
}

} // namespace Runtime
} // namespace NodeEditor
