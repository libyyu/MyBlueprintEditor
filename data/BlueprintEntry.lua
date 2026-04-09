-- examples/BlueprintEntry.lua
-- 蓝图扩展入口：注册 Lua 自定义节点和工具库
--
-- 加载时机：runtime 加载 examples/ 目录下任意蓝图时自动执行。
-- 职责：
--   1. 加载 lua/agent_tools.lua（get_weather / web_search / run_react_agent）
--   2. 注册自定义节点 "LuaAgent.RunReAct"

-- ── 确定本文件所在目录 ─────────────────────────────────────────────────────
-- local my_dir = ""
-- local info = debug.getinfo(1, "S")
-- if info and info.source and info.source:sub(1,1) == "@" then
--     my_dir = info.source:sub(2):match("^(.*[/\\])") or ""
-- end

-- -- ── 加载工具库 ─────────────────────────────────────────────────────────────
-- local tools_path = my_dir .. "lua/agent_tools.lua"
-- local ok, err = pcall(dofile, tools_path)
-- if not ok then
--     print("[BlueprintEntry] Warning: could not load lua/agent_tools.lua: " .. tostring(err))
--     function run_react_agent(q, k, u, m, cb)
--         cb("[Error] agent_tools.lua not loaded: " .. tostring(err))
--     end
-- end

-- ── 注册自定义节点：LuaAgent.RunReAct ─────────────────────────────────────
-- 输入：Flow In, UserQuery(String), ApiKey(String), BaseURL(String), Model(String)
-- 输出：Flow Out
-- 行为：handler 同步读取输入值后立即启动异步 agent，然后返回 true。
--       agent 完成时通过 print() 输出结果（不持有 ctx，避免悬空指针）。
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

-- Blueprint.RegisterHandler("LuaAgent.RunReAct", function(ctx)
--     -- 在 handler 内同步读取所有输入，不在异步回调里访问 ctx
--     local user_query = ctx:GetInput("UserQuery"):asString()
--     local api_key    = ctx:GetInput("ApiKey")   :asString()
--     local base_url   = ctx:GetInput("BaseURL")  :asString()
--     local model      = ctx:GetInput("Model")    :asString()

--     if base_url == "" then base_url = "https://api.openai.com/v1" end
--     if model    == "" then model    = "gpt-4o" end

--     if api_key == "" or api_key == "sk-your-api-key-here" then
--         print("[LuaAgent] ERROR: ApiKey is not set.")
--         return true
--     end

--     print("[LuaAgent] Starting ReAct Agent...")
--     print("[LuaAgent] Query: " .. user_query)
--     print("[LuaAgent] Model: " .. model .. " @ " .. base_url)

--     -- 启动异步 agent（回调在 Tick/DrainQueue 上下文执行，不持有 ctx）
--     run_react_agent(user_query, api_key, base_url, model, function(result)
--         print("")
--         print("═══════════════════════════════════")
--         print("[LuaAgent] Final Answer:")
--         print(result)
--         print("═══════════════════════════════════")
--     end)

--     return true
-- end)

print("[BlueprintEntry] LuaAgent.RunReAct node registered.")
