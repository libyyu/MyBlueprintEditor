// BuiltinHandlers_AI_Tool.cpp - Tool/MCP/Memory/UserInput handlers
#include "BuiltinHandlers_AI_Internal.h"

namespace NodeEditor {
namespace Runtime {

void RegisterHandlers_AI_Tool(
    std::unordered_map<std::string, NodeHandler>& handlers,
    BlueprintRunner& runner)
{
    // JSON.ParseToolCall
    //   从 LLM 返回的 tool_calls 数组中取出指定位置的调用信息
    //   in:  ToolCallsJSON(String), Index(Integer, default=0)
    //   out: Name(String), ArgumentsJSON(String), ID(String)
    //   纯数据节点（无 exec flow）
    // ================================================================
    handlers["JSON.ParseToolCall"] = [](ExecutionContext& ctx) {
        auto* runner = ctx.GetRunner(); (void)runner;
        std::string json = ctx.GetInputValue("ToolCallsJSON").asString();
        int index = (int)ctx.GetInputValue("Index").asInt();

        auto v = crude_json::value::parse(json);
        if (!v.is_array()) {
            ctx.SetOutputValue("Name",          Variant(std::string("")));
            ctx.SetOutputValue("ArgumentsJSON", Variant(std::string("{}")));
            ctx.SetOutputValue("ID",            Variant(std::string("")));
            return true;
        }
        const auto& arr = v.get<crude_json::array>();
        if (index < 0 || index >= (int)arr.size()) {
            ctx.SetOutputValue("Name",          Variant(std::string("")));
            ctx.SetOutputValue("ArgumentsJSON", Variant(std::string("{}")));
            ctx.SetOutputValue("ID",            Variant(std::string("")));
            return true;
        }
        const auto& tc = arr[index];
        auto info = ExtractToolCall(tc);
        ctx.SetOutputValue("Name",          Variant(info.name));
        ctx.SetOutputValue("ArgumentsJSON", Variant(info.arguments.empty() ? std::string("{}") : info.arguments));
        ctx.SetOutputValue("ID",            Variant(info.id));
        return true;
    };

    // ================================================================
    // JSON.MakeToolResult
    //   构造一条 tool role 的消息，用于把工具执行结果返回给 LLM
    //   in:  ToolCallID(String), Content(String)
    //   out: JSON(String)  — {"role":"tool","tool_call_id":"...","content":"..."}
    //   纯数据节点（无 exec flow）
    // ================================================================
    handlers["JSON.MakeToolResult"] = [](ExecutionContext& ctx) {
        auto* runner = ctx.GetRunner(); (void)runner;
        std::string toolCallId = ctx.GetInputValue("ToolCallID").asString();
        std::string content    = ctx.GetInputValue("Content").asString();

        crude_json::object obj;
        obj["role"]         = crude_json::value(std::string("tool"));
        obj["tool_call_id"] = crude_json::value(toolCallId);
        obj["content"]      = crude_json::value(content);
        ctx.SetOutputValue("JSON", Variant(crude_json::value(std::move(obj)).dump()));
        return true;
    };

    // ================================================================
    //   从文件加载对话历史（JSON 数组字符串）
    //   exec in → onSuccess / onError / onNew（文件不存在时）
    //   in:  Path(String)        — 历史文件路径
    //        MaxMessages(Integer)— 最多保留最近 N 条，0=不限制
    //   out: Messages(String)    — JSON 数组字符串
    //        Count(Integer)      — 实际消息条数
    //        ErrorMessage(String)
    //
    // WebGL：直接走 onNew，返回空数组（无磁盘可用）
    // ================================================================
    handlers["Memory.LoadHistory"] = [](ExecutionContext& ctx) {
        auto* runner = ctx.GetRunner(); (void)runner;
        std::string path = ctx.GetInputValue("Path").asString();
        int maxMsg = (int)ctx.GetInputValue("MaxMessages").asInt();

        auto* fs = GetDefaultFileSystem().get();

        if (path.empty()) {
            ctx.SetOutputValue("Messages",     Variant(std::string("[]")));
            ctx.SetOutputValue("Count",        Variant((int64_t)0));
            ctx.SetOutputValue("ErrorMessage", Variant(std::string("Path is empty")));
            ctx.ActivateOutputFlow("onError");
            return true;
        }

        // 文件不存在 → onNew（正常首次启动）
        if (!fs || !fs->FileExists(path)) {
            ctx.SetOutputValue("Messages",     Variant(std::string("[]")));
            ctx.SetOutputValue("Count",        Variant((int64_t)0));
            ctx.SetOutputValue("ErrorMessage", Variant(std::string("")));
            ctx.ActivateOutputFlow("onNew");
            return true;
        }

        // 读取文件
        std::string raw, readErr;
        if (!fs->ReadFile(path, raw, readErr)) {
            ctx.SetOutputValue("Messages",     Variant(std::string("[]")));
            ctx.SetOutputValue("Count",        Variant((int64_t)0));
            ctx.SetOutputValue("ErrorMessage", Variant(std::string(
                "Cannot open history file: " + path + (readErr.empty() ? "" : " (" + readErr + ")"))));
            ctx.ActivateOutputFlow("onError");
            return true;
        }

        // 解析 JSON 数组
        auto v = crude_json::value::parse(raw);
        if (!v.is_array()) {
            // 文件损坏，当新建处理
            ctx.SetOutputValue("Messages",     Variant(std::string("[]")));
            ctx.SetOutputValue("Count",        Variant((int64_t)0));
            ctx.SetOutputValue("ErrorMessage", Variant(std::string(
                "History file is not a JSON array, starting fresh")));
            ctx.ActivateOutputFlow("onNew");
            return true;
        }

        auto& arr = v.get<crude_json::array>();

        // MaxMessages 截断（保留最近 N 条）
        if (maxMsg > 0 && (int)arr.size() > maxMsg) {
            crude_json::array trimmed(arr.end() - maxMsg, arr.end());
            arr = std::move(trimmed);
        }

        std::string result = v.dump();
        int64_t count = (int64_t)arr.size();
        ctx.SetOutputValue("Messages",     Variant(result));
        ctx.SetOutputValue("Count",        Variant(count));
        ctx.SetOutputValue("ErrorMessage", Variant(std::string("")));
        ctx.ActivateOutputFlow("onSuccess");
        return true;
    };

    // ================================================================
    // Memory.SaveHistory
    //   把 messages JSON 数组写回文件
    //   exec in → onSuccess / onError
    //   in:  Path(String)           — 文件路径
    //        Messages(String)       — JSON 数组字符串
    //        MaxMessages(Integer)   — 写入前截断，0=不限
    //   out: ErrorMessage(String)
    //
    // WebGL：直接走 onSuccess（no-op）
    // ================================================================
    handlers["Memory.SaveHistory"] = [](ExecutionContext& ctx) {
        auto* runner = ctx.GetRunner(); (void)runner;
        std::string path     = ctx.GetInputValue("Path").asString();
        std::string messages = ctx.GetInputValue("Messages").asString();
        int maxMsg = (int)ctx.GetInputValue("MaxMessages").asInt();

        auto* fs = GetDefaultFileSystem().get();

        if (path.empty()) {
            ctx.SetOutputValue("ErrorMessage", Variant(std::string("Path is empty")));
            ctx.ActivateOutputFlow("onError");
            return true;
        }

        // 可选截断
        std::string toWrite = messages;
        if (maxMsg > 0) {
            auto v = crude_json::value::parse(messages);
            if (v.is_array()) {
                auto& arr = v.get<crude_json::array>();
                if ((int)arr.size() > maxMsg) {
                    crude_json::array trimmed(arr.end() - maxMsg, arr.end());
                    arr = std::move(trimmed);
                }
                toWrite = v.dump();
            }
        }

        // 自动建父目录（通过 FileSystem.MakeDir）
        if (fs) {
            std::string dirName = fs->GetDirName(path);
            if (!dirName.empty()) {
                std::string mkErr;
                fs->MakeDir(dirName, mkErr); // 忽略错误（目录可能已存在）
            }
        }

        std::string writeErr;
        if (!fs || !fs->WriteFile(path, toWrite, writeErr)) {
            ctx.SetOutputValue("ErrorMessage", Variant(std::string(
                "Cannot write history file: " + path + (writeErr.empty() ? "" : " (" + writeErr + ")"))));
            ctx.ActivateOutputFlow("onError");
            return true;
        }
        ctx.SetOutputValue("ErrorMessage", Variant(std::string("")));
        ctx.ActivateOutputFlow("onSuccess");
        return true;
    };

    // ========================================================================
    // Tool.ForEach — 遍历 tool_calls JSON 数组，逐个激活 onTool
    // 输入：  ToolCallsJSON (string)  — LLM.Chat 输出的 tool_calls JSON 数组
    // 输出：  onTool   exec           — 每个 tool call 触发一次
    //         onDone   exec           — 全部遍历完毕
    //         ToolName  string        — 当前工具名
    //         Arguments string        — 当前工具参数 JSON
    //         ToolCallId string       — 当前 tool_call id
    //         Index     integer       — 0-based 索引
    // ========================================================================
    handlers["Tool.ForEach"] = [](ExecutionContext& ctx) {
        auto* runner = ctx.GetRunner(); (void)runner;
        auto tcJson = ctx.GetInputValue("ToolCallsJSON").asString();
        if (tcJson.empty()) {
            ctx.ActivateOutputFlow("onDone");
            return true;
        }

        crude_json::value root = crude_json::value::parse(tcJson);
        if (!root.is_array()) {
            // 可能是完整 response，尝试提取 choices[0].message.tool_calls
            if (root.is_object() && root.contains("choices")) {
                const auto& choices = root["choices"];
                if (choices.is_array() && !choices.get<crude_json::array>().empty()) {
                    const auto& msg = choices.get<crude_json::array>()[0];
                    if (msg.is_object() && msg.contains("message")) {
                        const auto& m = msg["message"];
                        if (m.is_object() && m.contains("tool_calls"))
                            root = m["tool_calls"];
                    }
                }
            }
        }

        if (!root.is_array()) {
            ctx.ActivateOutputFlow("onDone");
            return true;
        }

        const auto& arr = root.get<crude_json::array>();
        for (int i = 0; i < (int)arr.size(); ++i) {
            auto info = ExtractToolCall(arr[i]);
            ctx.SetOutputValue("ToolName",   Variant(info.name));
            ctx.SetOutputValue("Arguments",  Variant(info.arguments));
            ctx.SetOutputValue("ToolCallId", Variant(info.id));
            ctx.SetOutputValue("Index",      Variant((int64_t)i));
            ctx.ActivateOutputFlow("onTool");
        }
        ctx.ActivateOutputFlow("onDone");
        return true;
    };

    // ========================================================================
    // Tool.ForEachParallel — 并发执行所有 tool_calls，全部完成后激活 onDone
    //
    // 与 Tool.ForEach 的区别：
    //   - Tool.ForEach：串行，每个工具完成后才触发下一个 onTool
    //   - Tool.ForEachParallel：并发，所有工具同时发起（适合多个独立 HTTP 工具调用）
    //
    // 工作原理：
    //   对每个 tool_call，构造一个 HTTP.POST 请求发送到 ToolServerURL，
    //   所有请求并发发起，全部完成后主线程统一激活 onDone，输出 ResultsJSON。
    //
    // 输入：  ToolCallsJSON (string)   — tool_calls JSON 数组
    //         ToolServerURL (string)   — 工具 HTTP 服务地址（可选，为空则退化为串行）
    // 输出：  onDone   exec             — 全部完成
    //         onError  exec             — HTTP 客户端未注册
    //         ResultsJSON (string)      — JSON 数组 [{id, name, result}, ...]
    //
    // 注意：若工具不是 HTTP 服务而是 FuncLib 函数，请使用 Tool.ForEach +
    //       Tool.CallByName（后者已支持 FuncLib 动态路由）。
    // ========================================================================
    handlers["Tool.ForEachParallel"] = [](ExecutionContext& ctx) {
        auto* runner = ctx.GetRunner(); (void)runner;
        auto* client = BP_GetHttpClient();
        if (!client) {
            ctx.SetOutputValue("ResultsJSON", Variant(std::string("[]")));
            ctx.ActivateOutputFlow("onError");
            return true;
        }

        auto tcJson     = ctx.GetInputValue("ToolCallsJSON").asString();
        auto serverURL  = ctx.GetInputValue("ToolServerURL").asString();

        // 解析 tool_calls 数组
        crude_json::value root = crude_json::value::parse(tcJson);
        if (!root.is_array() || root.get<crude_json::array>().empty()) {
            ctx.SetOutputValue("ResultsJSON", Variant(std::string("[]")));
            ctx.ActivateOutputFlow("onDone");
            return true;
        }
        const auto& arr = root.get<crude_json::array>();
        size_t total = arr.size();

        // 共享状态：收集各工具结果
        struct ToolResult { std::string id; std::string name; std::string result; };
        auto results  = std::make_shared<std::vector<ToolResult>>(total);
        auto counter  = std::make_shared<std::atomic<size_t>>(0);

        ctx.MarkDownstreamAsHandled("onDone");
        ctx.MarkDownstreamAsHandled("onError");
        PinId donePinId  = ctx.GetPinId("onDone");

        // dispatcher 模式：在 RunAsync 内并发发起所有请求
        auto dispatcher = [client, arr, serverURL, results, counter, total](
            ExecutionContext::AsyncResolve resolve) mutable
        {
            for (size_t i = 0; i < total; ++i) {
                auto info = ExtractToolCall(arr[i]);
                const std::string& id   = info.id;
                const std::string& name = info.name;
                const std::string& args = info.arguments;

                // 构造 JSON-RPC 风格请求体
                crude_json::object reqBody;
                reqBody["tool_name"] = crude_json::value(name);
                reqBody["arguments"] = crude_json::value(crude_json::value::parse(args.empty() ? "{}" : args));
                reqBody["id"]        = crude_json::value(id);
                std::string bodyStr  = crude_json::value(std::move(reqBody)).dump();

                HttpRequest req;
                req.url    = serverURL.empty() ? "http://localhost:7788/tool/call" : serverURL;
                req.method = "POST";
                req.body   = bodyStr;
                req.headers["Content-Type"] = "application/json";

                (*results)[i] = { id, name, "" };  // 预填

                client->SendAsync(req,
                    [results, counter, total, idx = i, resolve](HttpResponse resp) mutable {
                        (*results)[idx].result = resp.ok() ? resp.body : ("Error: " + resp.error);
                        if (++(*counter) == total)
                            resolve();  // 全部完成，触发 onComplete
                    });
            }
        };

        // onComplete：序列化结果并激活 onDone
        auto onComplete = [results, donePinId](ExecutionContext& c) mutable {
            crude_json::array out;
            for (const auto& r : *results) {
                crude_json::object obj;
                obj["id"]     = crude_json::value(r.id);
                obj["name"]   = crude_json::value(r.name);
                obj["result"] = crude_json::value(r.result);
                out.push_back(crude_json::value(std::move(obj)));
            }
            c.SetOutputValue("ResultsJSON", Variant(crude_json::value(std::move(out)).dump()));
            c.ActivateOutputFlow(donePinId);
        };

        ctx.RunAsync(std::move(dispatcher), std::move(onComplete));
        return true;
    };
    // 输入：  ToolCallsJSON(String)
    // 输出：  Count(Integer)
    // 纯数据节点（无 exec flow）
    // ========================================================================
    handlers["JSON.ToolCallCount"] = [](ExecutionContext& ctx) {
        auto* runner = ctx.GetRunner(); (void)runner;
        auto json = ctx.GetInputValue("ToolCallsJSON").asString();
        int64_t count = 0;
        if (!json.empty()) {
            auto v = crude_json::value::parse(json);
            if (v.is_array())
                count = static_cast<int64_t>(v.get<crude_json::array>().size());
            // 兼容完整 response 格式：尝试提取 choices[0].message.tool_calls
            else if (v.is_object() && v.contains("choices")) {
                const auto& choices = v["choices"];
                if (choices.is_array() && !choices.get<crude_json::array>().empty()) {
                    const auto& msg = choices.get<crude_json::array>()[0];
                    if (msg.is_object() && msg.contains("message")) {
                        const auto& m = msg["message"];
                        if (m.is_object() && m.contains("tool_calls")) {
                            const auto& tc = m["tool_calls"];
                            if (tc.is_array())
                                count = static_cast<int64_t>(tc.get<crude_json::array>().size());
                        }
                    }
                }
            }
        }
        ctx.SetOutputValue("Count", Variant(count));
        return true;
    };

    // ========================================================================
    // Tool.CallByName — 按工具名动态路由到同名 FuncLib 函数
    //
    // 功能：根据 ToolName 在当前蓝图的外部函数库中查找同名函数并调用，
    //       无需 Tool.Match 枚举，实现无上限的动态工具路由。
    //
    // 输入：  exec in
    //         ToolName(String)    — 工具名，与 FuncLib 中的函数名一致
    //         Arguments(String)   — 工具参数 JSON 字符串（传给函数的变量 args）
    //         ToolCallId(String)  — tool_call id（用于组装 MakeToolResult）
    // 输出：  onSuccess exec      — 调用成功
    //         onNotFound exec     — 找不到同名函数
    //         Result(String)      — 函数返回的 "Result" 变量值
    //         ToolCallId(String)  — 透传输入的 ToolCallId（便于直接接 MakeToolResult）
    //
    // 约定：被调用的 FuncLib 函数需有一个名为 "Result" 的输出变量；
    //       Arguments JSON 对象的 key 会作为同名变量注入函数。
    // ========================================================================
    handlers["Tool.CallByName"] = [](ExecutionContext& ctx) {
        auto* runner = ctx.GetRunner(); (void)runner;
        std::string toolName   = ctx.GetInputValue("ToolName").asString();
        std::string argsJson   = ctx.GetInputValue("Arguments").asString();
        std::string toolCallId = ctx.GetInputValue("ToolCallId").asString();

        ctx.SetOutputValue("ToolCallId", Variant(toolCallId));

        if (toolName.empty()) {
            ctx.SetOutputValue("Result", Variant(std::string("")));
            ctx.ActivateOutputFlow("onNotFound");
            return true;
        }

        // 在已注册的外部函数中查找同名函数
        // GetExternalFunctions() 返回 vector<FunctionDefinition>
        auto extFuncs = runner->GetExternalFunctions();
        const ::NodeEditor::Runtime::FunctionDefinition* funcDefPtr = nullptr;
        for (const auto& fd : extFuncs) {
            if (fd.name == toolName || fd.id == toolName) {
                funcDefPtr = &fd;
                break;
            }
        }

        if (!funcDefPtr) {
            ctx.Log("[Tool.CallByName] function '" + toolName + "' not found in libraries",
                    ::NodeEditor::Runtime::LogLevel::Warning);
            ctx.SetOutputValue("Result", Variant(std::string("")));
            ctx.ActivateOutputFlow("onNotFound");
            return true;
        }

        // 构建函数子图
        const auto& extLibs = runner->GetExternalLibraries();
        auto libIt = extLibs.find(funcDefPtr->id);
        if (libIt == extLibs.end() || !libIt->second) {
            ctx.Log("[Tool.CallByName] library data not found for '" + toolName + "'",
                    ::NodeEditor::Runtime::LogLevel::Warning);
            ctx.SetOutputValue("Result", Variant(std::string("")));
            ctx.ActivateOutputFlow("onNotFound");
            return true;
        }

        // 解析 Arguments JSON（稍后作为 FuncLib 节点输入引脚默认值注入）
        std::vector<std::pair<std::string, Variant>> argVars;
        if (!argsJson.empty()) {
            auto argsVal = crude_json::value::parse(argsJson);
            if (argsVal.is_object()) {
                for (const auto& kv : argsVal.get<crude_json::object>()) {
                    const auto& v = kv.second;
                    Variant var;
                    if (v.is_string())
                        var = Variant(v.get<std::string>());
                    else if (v.is_number()) {
                        double d = v.get<double>();
                        var = (d == static_cast<double>(static_cast<int64_t>(d)))
                            ? Variant(static_cast<int64_t>(d)) : Variant(d);
                    } else if (v.is_boolean())
                        var = Variant(v.get<bool>());
                    else
                        var = Variant(v.dump());
                    argVars.emplace_back(kv.first, std::move(var));
                }
            }
        }

        // 构造一个微型 Actor 蓝图，用 FuncLib.<funcId> 节点调用目标函数。
        // FuncLib.* handler 内部创建子 runner 时，会遍历节点的数据输入引脚，
        // 将引脚值（或默认值）通过 SetVariable 注入函数子 runner。
        // 因此，Arguments 中的参数需要以数据输入引脚的形式挂在 FuncLib 节点上。

        BlueprintData miniActor;
        miniActor.metadata.blueprintClass = BlueprintClass::Actor;
        miniActor.metadata.name = "ToolCallByName_" + toolName;

        // OnBeginPlay 节点（id=1，exec 输出 pin=10）
        {
            NodeInstance n;
            n.id = 1; n.definitionId = "OnBeginPlay"; n.name = "OnBeginPlay";
            PinInfo ep; ep.id=10; ep.kind=PinKind::Output; ep.isExec=true; ep.name="";
            n.pins.push_back(ep);
            miniActor.nodes.push_back(n);
        }

        // FuncLib.<funcId> 节点（id=2）
        // 为每个 Argument 添加数据输入引脚（带默认值），FuncLib handler 会将其传入函数
        std::string funcLibDefId = "FuncLib." + libIt->second->metadata.name + "." + funcDefPtr->id;
        {
            NodeInstance n;
            n.id = 2; n.definitionId = funcLibDefId; n.name = toolName;
            // exec in/out
            PinInfo ei; ei.id=20; ei.kind=PinKind::Input; ei.isExec=true; ei.name="";
            n.pins.push_back(ei);
            PinInfo eo; eo.id=29; eo.kind=PinKind::Output; eo.isExec=true; eo.name="";
            n.pins.push_back(eo);
            // 每个参数作为数据输入引脚（pin id 从 200 开始）
            PinId argPinId = 200;
            for (const auto& av : argVars) {
                PinInfo ap;
                ap.id = argPinId++;
                ap.kind = PinKind::Input;
                ap.isExec = false;
                ap.dataType = PinDataType::String;  // 统一 String，FuncLib handler 用名称匹配
                ap.name = av.first;
                ap.defaultValue = av.second;
                n.pins.push_back(ap);
            }
            // Result 输出引脚
            PinInfo rp; rp.id=22; rp.kind=PinKind::Output; rp.isExec=false;
            rp.dataType=PinDataType::String; rp.name="Result";
            n.pins.push_back(rp);
            miniActor.nodes.push_back(n);
        }

        // 连线 OnBeginPlay.exec → FuncLib.exec
        {
            LinkInstance lnk; lnk.id=100;
            lnk.startPinId=10; lnk.endPinId=20;
            miniActor.links.push_back(lnk);
        }

        // 构造子 runner，继承父 runner 的全部配置（含 SetParentTimerManager + SetNodePreExecuteCallback）
        auto subRunner = runner->CreateChildRunner();

        // 加载并执行
        if (subRunner->Load(miniActor)) {
            subRunner->Execute();
            subRunner->DispatchEvent("OnBeginPlay");
            auto result = subRunner->GetVariable("Result");
            if (result.type == ::NodeEditor::Runtime::PinDataType::Unknown)
                result = subRunner->GetPinValue(22);  // fallback：从输出引脚读
            ctx.SetOutputValue("Result",
                result.type != ::NodeEditor::Runtime::PinDataType::Unknown
                    ? result : Variant(std::string("")));
        } else {
            ctx.SetOutputValue("Result", Variant(std::string("")));
        }

        ctx.ActivateOutputFlow("onSuccess");
        return true;
    };

    // ========================================================================
    // Tool.Match — 按工具名路由到对应 exec 分支（最多 8 个 Case）
    // 输入：  ToolName string
    //         Case0..Case7 string（空 = 不使用）
    // 输出：  Match0..Match7 exec — 对应 Case 匹配时激活
    //         Default exec        — 无匹配时激活
    //         MatchedIndex integer — 匹配到的索引，-1=无匹配
    // ========================================================================
    handlers["Tool.Match"] = [](ExecutionContext& ctx) {
        auto* runner = ctx.GetRunner(); (void)runner;
        auto toolName = ctx.GetInputValue("ToolName").asString();
        int matched = -1;
        for (int i = 0; i < 8; ++i) {
            std::string caseKey = "Case" + std::to_string(i);
            auto caseVal = ctx.GetInputValue(caseKey).asString();
            if (!caseVal.empty() && caseVal == toolName) {
                matched = i;
                break;
            }
        }
        ctx.SetOutputValue("MatchedIndex", Variant((int64_t)matched));
        if (matched >= 0) {
            ctx.ActivateOutputFlow("Match" + std::to_string(matched));
        } else {
            ctx.ActivateOutputFlow("Default");
        }
        return true;
    };

    // ========================================================================
    // JSON.Extract — 从文本中提取 ```json ... ``` 代码块内容
    // 输入：  Text string
    // 输出：  JSON string   — 提取到的 JSON（若无代码块则尝试直接 parse）
    //         Found boolean — 是否找到代码块
    // ========================================================================
    handlers["JSON.Extract"] = [](ExecutionContext& ctx) {
        auto* runner = ctx.GetRunner(); (void)runner;
        auto text = ctx.GetInputValue("Text").asString();

        // 尝试提取 ```json ... ``` 或 ``` ... ```
        auto tryExtract = [&](const std::string& fence) -> std::string {
            auto pos = text.find(fence);
            if (pos == std::string::npos) return "";
            pos += fence.size();
            // skip newline
            if (pos < text.size() && (text[pos] == '\n' || text[pos] == '\r')) pos++;
            auto end = text.find("```", pos);
            if (end == std::string::npos) return "";
            // trim trailing whitespace
            while (end > pos && (text[end-1] == '\n' || text[end-1] == '\r' || text[end-1] == ' '))
                --end;
            return text.substr(pos, end - pos);
        };

        std::string extracted = tryExtract("```json");
        if (extracted.empty()) extracted = tryExtract("```JSON");
        if (extracted.empty()) extracted = tryExtract("```");

        if (!extracted.empty()) {
            // 验证是合法 JSON
            auto v = crude_json::value::parse(extracted);
            if (!v.is_null() || extracted.find("null") != std::string::npos) {
                ctx.SetOutputValue("JSON",  Variant(extracted));
                ctx.SetOutputValue("Found", Variant(true));
                return true;
            }
        }

        // 没有代码块，尝试直接把整段文本当 JSON parse
        auto trimmed = text;
        while (!trimmed.empty() && (trimmed.front() == ' ' || trimmed.front() == '\n' || trimmed.front() == '\r'))
            trimmed.erase(trimmed.begin());
        while (!trimmed.empty() && (trimmed.back() == ' ' || trimmed.back() == '\n' || trimmed.back() == '\r'))
            trimmed.pop_back();

        auto v2 = crude_json::value::parse(trimmed);
        bool looksJson = !trimmed.empty() && (trimmed.front() == '{' || trimmed.front() == '[');
        if (looksJson && !v2.is_null()) {
            ctx.SetOutputValue("JSON",  Variant(trimmed));
            ctx.SetOutputValue("Found", Variant(true));
        } else {
            ctx.SetOutputValue("JSON",  Variant(std::string("")));
            ctx.SetOutputValue("Found", Variant(false));
        }
        return true;
    };

    // ========================================================================
    // JSON.Validate — 验证 JSON 字符串是否合法，可选按 key 列表校验必填字段
    // 输入：  JSON string
    //         RequiredKeys string  — 逗号分隔，如 "name,age"（空=只验证格式）
    // 输出：  onValid  exec
    //         onInvalid exec
    //         IsValid  boolean
    //         ErrorMessage string
    // ========================================================================
    handlers["JSON.Validate"] = [](ExecutionContext& ctx) {
        auto* runner = ctx.GetRunner(); (void)runner;
        auto jsonStr     = ctx.GetInputValue("JSON").asString();
        auto requiredRaw = ctx.GetInputValue("RequiredKeys").asString();

        if (jsonStr.empty()) {
            ctx.SetOutputValue("IsValid",      Variant(false));
            ctx.SetOutputValue("ErrorMessage", Variant(std::string("Empty JSON string")));
            ctx.ActivateOutputFlow("onInvalid");
            return true;
        }

        auto v = crude_json::value::parse(jsonStr);
        bool isNull = v.is_null();

        if (isNull && jsonStr.find("null") == std::string::npos) {
            ctx.SetOutputValue("IsValid",      Variant(false));
            ctx.SetOutputValue("ErrorMessage", Variant(std::string("JSON parse error")));
            ctx.ActivateOutputFlow("onInvalid");
            return true;
        }

        // 检查必填 keys（仅对 object 有意义）
        if (!requiredRaw.empty() && v.is_object()) {
            std::vector<std::string> missing;
            size_t pos = 0;
            while (pos < requiredRaw.size()) {
                auto comma = requiredRaw.find(',', pos);
                auto key = requiredRaw.substr(pos, comma == std::string::npos ? std::string::npos : comma - pos);
                // trim
                while (!key.empty() && key.front() == ' ') key.erase(key.begin());
                while (!key.empty() && key.back()  == ' ') key.pop_back();
                if (!key.empty() && !v.contains(key))
                    missing.push_back(key);
                if (comma == std::string::npos) break;
                pos = comma + 1;
            }
            if (!missing.empty()) {
                std::string errMsg = "Missing required keys: ";
                for (size_t i = 0; i < missing.size(); ++i) {
                    if (i) errMsg += ", ";
                    errMsg += missing[i];
                }
                ctx.SetOutputValue("IsValid",      Variant(false));
                ctx.SetOutputValue("ErrorMessage", Variant(errMsg));
                ctx.ActivateOutputFlow("onInvalid");
                return true;
            }
        }

        ctx.SetOutputValue("IsValid",      Variant(true));
        ctx.SetOutputValue("ErrorMessage", Variant(std::string("")));
        ctx.ActivateOutputFlow("onValid");
        return true;
    };

    // ========================================================================
    // UserInput.Wait
    // Mock 模式：runner 变量 __userinput_mock 非空时直接返回
    // 正常模式：设置 pending 标志，通过 SetTimer 每帧轮询等待编辑器写入结果
    //           完全不使用 std::thread，WebGL/Emscripten 兼容。
    //
    // 机制：
    //   ctx.SetTimer(interval, -1, callback) 内部通过 wrapCallbackWithContextRestore
    //   自动调用 AcquireAsync()，callback 返回 false 时自动 ReleaseAsync()。
    //   FrameTimerManager 由 BlueprintRunner::Tick() 驱动，每帧在主线程执行，
    //   与 ImGui UserInput 弹框的 OK 逻辑完全在同一线程，无竞争问题。
    // ========================================================================
    handlers["UserInput.Wait"] = [](ExecutionContext& ctx) -> bool {
        auto* runner = ctx.GetRunner(); (void)runner;
        std::string prompt = ctx.GetInputValue("Prompt").asString();
        std::string defVal = ctx.GetInputValue("DefaultValue").asString();
        if (prompt.empty()) prompt = "Input:";

        // Mock 模式：变量 __userinput_mock 非空时立即返回
        Variant mockVal = ctx.GetVariable("__userinput_mock");
        if (!mockVal.asString().empty()) {
            ctx.SetOutputValue("Input", mockVal);
            ctx.ActivateOutputFlow("");
            return true;
        }

        // 设置 pending 标志，供编辑器渲染弹框
        ctx.SetVariable("__userinput_prompt",  Variant(prompt));
        ctx.SetVariable("__userinput_default", Variant(defVal));
        ctx.SetVariable("__userinput_result",  Variant(std::string("")));
        ctx.SetVariable("__userinput_pending", Variant(std::string("1")));

        ctx.MarkDownstreamAsHandled("");

        ExecutionContext* pCtx = &ctx;

        // 每 50ms 轮询一次 pending 标志（与原后台线程间隔相同）。
        // wrapCallbackWithContextRestore 自动管理 AcquireAsync / ReleaseAsync，
        // callback 返回 true = 继续等待，返回 false = 完成并释放。
        ctx.SetTimer(0.05f, -1, [pCtx]() -> bool {
            std::string pending = pCtx->GetVariable("__userinput_pending").asString();
            if (pending == "1") {
                return true; // 用户尚未确认，继续轮询
            }
            // 编辑器已写入结果（pending == "0"）
            std::string result = pCtx->GetVariable("__userinput_result").asString();
            pCtx->SetOutputValue("Input", Variant(result));
            pCtx->ActivateOutputFlow("");
            return false; // 取消 timer，触发 ReleaseAsync
        });

        return true;
    };

    // ========================================================================
    // Tool.Define — 构建单个 OpenAI function calling tool JSON
    // ========================================================================
    handlers["Tool.Define"] = [](ExecutionContext& ctx) -> bool {
        auto* runner = ctx.GetRunner(); (void)runner;
        std::string name  = ctx.GetInputValue("Name").asString();
        std::string desc  = ctx.GetInputValue("Description").asString();
        Variant paramNames = ctx.GetInputValue("ParamNames");
        Variant paramDescs = ctx.GetInputValue("ParamDescs");
        Variant required   = ctx.GetInputValue("Required");

        crude_json::object props;
        size_t n = paramNames.arraySize();
        for (size_t i = 0; i < n; ++i) {
            std::string pname = paramNames.arrayGet(i).asString();
            std::string pdesc = (i < paramDescs.arraySize())
                                ? paramDescs.arrayGet(i).asString() : "";
            if (pname.empty()) continue;
            crude_json::object prop;
            prop["type"] = crude_json::value(std::string("string"));
            if (!pdesc.empty()) prop["description"] = crude_json::value(pdesc);
            props[pname] = crude_json::value(prop);
        }

        crude_json::array reqArr;
        for (size_t i = 0; i < required.arraySize(); ++i) {
            std::string r = required.arrayGet(i).asString();
            if (!r.empty()) reqArr.push_back(crude_json::value(r));
        }

        crude_json::object params;
        params["type"]       = crude_json::value(std::string("object"));
        params["properties"] = crude_json::value(props);
        if (!reqArr.empty()) params["required"] = crude_json::value(reqArr);

        crude_json::object func;
        func["name"]        = crude_json::value(name);
        func["description"] = crude_json::value(desc);
        func["parameters"]  = crude_json::value(params);

        crude_json::object tool;
        tool["type"]     = crude_json::value(std::string("function"));
        tool["function"] = crude_json::value(func);

        ctx.SetOutputValue("ToolJSON", Variant(crude_json::value(tool).dump()));
        return true;
    };

    // ========================================================================
    // MCP.Call — 调用 MCP Server 工具（标准 JSON-RPC 2.0 + 简单模式）
    // ========================================================================
    // Protocol = "jsonrpc2"（默认）：标准 MCP JSON-RPC 2.0
    //   请求：POST {ServerURL}/  body={"jsonrpc":"2.0","id":N,"method":"tools/call","params":{"name":ToolName,"arguments":{...}}}
    //   响应解析：result.content[0].text → Result
    //            error.message → ErrorMessage
    //
    // Protocol = "simple"：简单 POST 模式（兼容旧格式）
    //   请求：POST {ServerURL}  body={"tool_name":ToolName,"arguments":{...}}
    //   响应：直接把 body 作为 Result
    // ========================================================================
    handlers["MCP.Call"] = [](ExecutionContext& ctx) -> bool {
        auto* runner = ctx.GetRunner(); (void)runner;
        IHttpClient* client = BP_GetHttpClient();
        if (!client) {
            ctx.SetOutputValue("Result",       Variant(std::string("")));
            ctx.SetOutputValue("RawResult",    Variant(std::string("")));
            ctx.SetOutputValue("ErrorMessage", Variant(std::string("No HttpClient registered")));
            ctx.ActivateOutputFlow("onError");
            return true;
        }

        std::string serverURL  = ctx.GetInputValue("ServerURL").asString();
        std::string toolName   = ctx.GetInputValue("ToolName").asString();
        std::string arguments  = ctx.GetInputValue("Arguments").asString();
        std::string protocol   = ctx.GetInputValue("Protocol").asString();
        int         timeoutSec = static_cast<int>(ctx.GetInputValue("TimeoutSec").asInt());

        if (serverURL.empty())  serverURL = "http://localhost:7788";
        if (protocol.empty())   protocol  = "jsonrpc2";
        if (toolName.empty()) {
            ctx.SetOutputValue("Result",       Variant(std::string("")));
            ctx.SetOutputValue("RawResult",    Variant(std::string("")));
            ctx.SetOutputValue("ErrorMessage", Variant(std::string("ToolName is empty")));
            ctx.ActivateOutputFlow("onError");
            return true;
        }
        if (arguments.empty()) arguments = "{}";

        // 解析 arguments JSON
        crude_json::value argsVal = crude_json::value::parse(arguments);
        if (argsVal.is_discarded() || argsVal.is_null()) {
            argsVal = crude_json::value(crude_json::object{});
        }

        // 构造请求体
        std::string bodyStr;
        std::string requestURL = serverURL;

        // 全局自增 ID（线程不敏感，仅用于请求区分）
        static std::atomic<int> s_reqId{1};

        if (protocol == "simple") {
            // 简单模式：{tool_name, arguments}
            crude_json::object req;
            req["tool_name"]  = crude_json::value(toolName);
            req["arguments"]  = argsVal;
            bodyStr = crude_json::value(std::move(req)).dump();
            // URL 使用默认 /tool/call 后缀
            if (requestURL.back() != '/')
                requestURL += "/tool/call";
            else
                requestURL += "tool/call";
        } else {
            // JSON-RPC 2.0 模式（标准 MCP）
            crude_json::object params;
            params["name"]      = crude_json::value(toolName);
            params["arguments"] = argsVal;

            crude_json::object req;
            req["jsonrpc"] = crude_json::value(std::string("2.0"));
            req["id"]      = crude_json::value(static_cast<double>(s_reqId.fetch_add(1)));
            req["method"]  = crude_json::value(std::string("tools/call"));
            req["params"]  = crude_json::value(std::move(params));
            bodyStr = crude_json::value(std::move(req)).dump();
            // JSON-RPC 直接 POST 到 ServerURL
        }

        HttpRequest req;
        req.url    = requestURL;
        req.method = "POST";
        req.body   = bodyStr;
        req.headers["Content-Type"] = "application/json";
        if (timeoutSec > 0) req.timeoutSeconds = timeoutSec;

        ctx.MarkDownstreamAsHandled("onSuccess");
        ctx.MarkDownstreamAsHandled("onError");
        PinId successPin = ctx.GetPinId("onSuccess");
        PinId errorPin   = ctx.GetPinId("onError");

        auto sharedResp = std::make_shared<HttpResponse>();
        bool isJsonRpc   = (protocol != "simple");

        auto dispatcher = [client, req, sharedResp](ExecutionContext::AsyncResolve resolve) mutable {
            client->SendAsync(req,
                [sharedResp, resolve = std::move(resolve)](HttpResponse resp) mutable {
                    *sharedResp = std::move(resp);
                    resolve();
                });
        };

        auto onComplete = [sharedResp, successPin, errorPin, isJsonRpc](ExecutionContext& c) mutable {
            const HttpResponse& resp = *sharedResp;
            if (!resp.ok()) {
                std::string errMsg = resp.error.empty()
                    ? ("HTTP " + std::to_string(resp.statusCode))
                    : resp.error;
                c.SetOutputValue("Result",       Variant(std::string("")));
                c.SetOutputValue("RawResult",    Variant(resp.body));
                c.SetOutputValue("ErrorMessage", Variant(errMsg));
                c.ActivateOutputFlow(errorPin);
                return;
            }

            c.SetOutputValue("RawResult", Variant(resp.body));

            if (!isJsonRpc) {
                // 简单模式：直接返回 body
                c.SetOutputValue("Result",       Variant(resp.body));
                c.SetOutputValue("ErrorMessage", Variant(std::string("")));
                c.ActivateOutputFlow(successPin);
                return;
            }

            // JSON-RPC 2.0 响应解析
            crude_json::value jresp = crude_json::value::parse(resp.body);

            // 检查 error 字段
            if (jresp.is_object() && jresp.contains("error")) {
                const auto& errObj = jresp["error"];
                std::string errMsg;
                if (errObj.is_object() && errObj.contains("message")
                    && errObj["message"].is_string())
                    errMsg = errObj["message"].get<std::string>();
                else
                    errMsg = resp.body.substr(0, 200);
                c.SetOutputValue("Result",       Variant(std::string("")));
                c.SetOutputValue("ErrorMessage", Variant(errMsg));
                c.ActivateOutputFlow(errorPin);
                return;
            }

            // 解析 result.content[0].text
            // 标准 MCP 响应：{"result":{"content":[{"type":"text","text":"..."}],...}}
            std::string resultText;
            bool parsed = false;

            if (jresp.is_object() && jresp.contains("result")) {
                const auto& result = jresp["result"];
                if (result.is_object() && result.contains("content")) {
                    const auto& content = result["content"];
                    if (content.is_array()) {
                        const auto& arr = content.get<crude_json::array>();
                        for (const auto& item : arr) {
                            if (item.is_object()) {
                                // 拼接所有 type=text 的文本
                                bool isText = !item.contains("type") ||
                                    (item["type"].is_string() &&
                                     item["type"].get<std::string>() == "text");
                                if (isText && item.contains("text") && item["text"].is_string()) {
                                    if (!resultText.empty()) resultText += "\n";
                                    resultText += item["text"].get<std::string>();
                                    parsed = true;
                                }
                            }
                        }
                    }
                }
                // 若 content 解析失败，尝试 result.text 或直接 dump
                if (!parsed) {
                    if (result.is_string()) {
                        resultText = result.get<std::string>();
                        parsed = true;
                    } else if (result.is_object() || result.is_array()) {
                        resultText = result.dump();
                        parsed = true;
                    }
                }
            }

            // 最终降级：返回整个 body
            if (!parsed) resultText = resp.body;

            c.SetOutputValue("Result",       Variant(resultText));
            c.SetOutputValue("ErrorMessage", Variant(std::string("")));
            c.Log("[MCP.Call] result length=" + std::to_string(resultText.size()));
            c.ActivateOutputFlow(successPin);
        };

        ctx.RunAsync(std::move(dispatcher), std::move(onComplete));
        return true;
    };

}

} // namespace Runtime
} // namespace NodeEditor
