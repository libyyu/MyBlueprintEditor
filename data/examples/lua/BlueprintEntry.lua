-- examples/lua/BlueprintEntry.lua
-- 蓝图扩展入口（从 lua/ 子目录加载时使用）
--
-- 职责：
--   1. 加载 agent_tools.lua（get_weather / web_search / run_react_agent）
--   2. 注册自定义节点 "LuaAgent.RunReAct"

-- ── 加载工具库 ──────────────────────────────────────────────────────────────
local script_dir = ""
local info = debug.getinfo(1, "S")
if info and info.source and info.source:sub(1,1) == "@" then
    local src = info.source:sub(2)
    script_dir = src:match("^(.*[/\\])") or ""
end

if script_dir ~= "" then
    local ok, err = pcall(dofile, script_dir .. "agent_tools.lua")
    if not ok then
        print("[BlueprintEntry] Warning: could not load agent_tools.lua: " .. tostring(err))
        function run_react_agent(q, k, u, m, cb)
            cb("[Error] agent_tools.lua not loaded: " .. tostring(err))
        end
    end
else
    print("[BlueprintEntry] Warning: could not determine script directory")
end

-- ── 注册自定义节点：LuaAgent.RunReAct ─────────────────────────────────────
Blueprint.RegisterNodeDef({
    id          = "LuaAgent.RunReAct",
    name        = "Run ReAct Agent",
    category    = "LuaAgent",
    color       = "3A7BD5",
    description = "Start a ReAct Agent loop. Result is printed when done.",
    inputs  = {
        { name = "In",        type = "Flow"   },
        { name = "UserQuery", type = "String" },
        { name = "ApiKey",    type = "String" },
        { name = "BaseURL",   type = "String" },
        { name = "Model",     type = "String" },
    },
    outputs = {
        { name = "Out", type = "Flow" },
    },
})

Blueprint.RegisterHandler("LuaAgent.RunReAct", function(ctx)
    local user_query = ctx:GetInput("UserQuery"):asString()
    local api_key    = ctx:GetInput("ApiKey")   :asString()
    local base_url   = ctx:GetInput("BaseURL")  :asString()
    local model      = ctx:GetInput("Model")    :asString()

    if base_url == "" then base_url = "https://api.openai.com/v1" end
    if model    == "" then model    = "gpt-4o" end

    if api_key == "" or api_key == "sk-your-api-key-here" then
        print("[LuaAgent] ERROR: ApiKey is not set.")
        return true
    end

    print("[LuaAgent] Starting ReAct Agent...")
    print("[LuaAgent] Query: " .. user_query)
    print("[LuaAgent] Model: " .. model .. " @ " .. base_url)

    run_react_agent(user_query, api_key, base_url, model, function(result)
        print("")
        print("═══════════════════════════════════")
        print("[LuaAgent] Final Answer:")
        print(result)
        print("═══════════════════════════════════")
    end)

    return true
end)

print("[BlueprintEntry] LuaAgent.RunReAct node registered.")
