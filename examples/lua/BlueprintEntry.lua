-- examples/lua/BlueprintEntry.lua
-- 蓝图自动加载入口
--
-- 加载时机：runtime-example 加载 LuaReActAgent.bjson 时自动执行。
-- 功能：
--   1. 加载 agent_tools.lua（定义 get_weather / web_search / run_react_agent）
--   2. 注册 OnBeginPlay handler，在蓝图执行时启动 Lua ReAct Agent
--
-- 异步说明：
--   http.get/post 回调在 MainThreadDispatcher::DrainQueue 中执行（Tick 循环）。
--   handler 立即返回 true；Agent 异步链在后续 Tick 中逐步推进。
--   最终结果通过 print() 输出，由 mcp_server.py 或 runtime-example 捕获。

-- ── 加载工具库 ──────────────────────────────────────────────────────────────
local script_dir = ""
local info = debug.getinfo(1, "S")
if info and info.source and info.source:sub(1,1) == "@" then
    local src = info.source:sub(2)  -- 去掉 @ 前缀
    script_dir = src:match("^(.*[/\\])") or ""
end

if script_dir ~= "" then
    local ok, err = pcall(dofile, script_dir .. "agent_tools.lua")
    if not ok then
        print("[LuaAgent] Warning: could not load agent_tools.lua: " .. tostring(err))
    end
else
    print("[LuaAgent] Warning: could not determine script directory")
end

-- ── 注册 OnBeginPlay handler ─────────────────────────────────────────────────
-- OnBeginPlay 节点的 definitionId 是 "OnBeginPlay"，handler 名同 definitionId。
-- 注意：Blueprint 会先执行 Execute()（数据流），再 DispatchEvent("OnBeginPlay")。
-- DispatchEvent 触发此 handler。
Blueprint.RegisterHandler("OnBeginPlay", function(ctx)
    -- 从蓝图变量读取配置
    local api_key    = ctx:GetVariable("ApiKey")   :asString()
    local base_url   = ctx:GetVariable("BaseURL")  :asString()
    local model      = ctx:GetVariable("Model")    :asString()
    local user_query = ctx:GetVariable("UserQuery"):asString()

    if base_url   == "" then base_url   = "https://api.openai.com/v1" end
    if model      == "" then model      = "gpt-4o" end
    if user_query == "" then user_query = "What's the weather in Beijing and Shanghai today, and any recent AI news?" end

    if api_key == "" or api_key == "sk-your-api-key-here" then
        print("[LuaAgent] ERROR: ApiKey is not set.")
        print("[LuaAgent] Pass variables: {\"ApiKey\": \"sk-xxx\"} when executing.")
        return true
    end

    print("[LuaAgent] Starting ReAct Agent...")
    print("[LuaAgent] Query: " .. user_query)
    print("[LuaAgent] Model: " .. model)
    print("")

    -- 启动异步 Agent（回调在 Tick/DrainQueue 中执行）
    run_react_agent(user_query, api_key, base_url, model, function(result)
        print("")
        print("═══════════════════════════════════")
        print("[LuaAgent] Final Answer:")
        print(result)
        print("═══════════════════════════════════")
    end)

    return true  -- handler 同步返回，异步链继续在 Tick 中推进
end)

print("[BlueprintEntry] LuaReActAgent ready. OnBeginPlay handler registered.")
