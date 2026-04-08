-- examples/BlueprintEntry.lua
-- 自动加载入口（与 LuaReActAgent.bjson 同目录）
--
-- runtime 加载 examples/ 下的蓝图时自动执行本文件。
-- 本文件加载 lua/agent_tools.lua 并注册 OnBeginPlay handler。

-- ── 确定本文件所在目录 ─────────────────────────────────────────────────────
local my_dir = ""
local info = debug.getinfo(1, "S")
if info and info.source and info.source:sub(1,1) == "@" then
    my_dir = info.source:sub(2):match("^(.*[/\\])") or ""
end

-- ── 加载工具库 ─────────────────────────────────────────────────────────────
local tools_path = my_dir .. "lua/agent_tools.lua"
local ok, err = pcall(dofile, tools_path)
if not ok then
    print("[BlueprintEntry] Warning: could not load lua/agent_tools.lua: " .. tostring(err))
    -- 定义 stub，避免后续调用出错
    function run_react_agent(q, k, u, m, cb) cb("agent_tools.lua not loaded: " .. tostring(err)) end
end

-- ── 注册 OnBeginPlay handler ──────────────────────────────────────────────
Blueprint.RegisterHandler("OnBeginPlay", function(ctx)
    local api_key    = ctx:GetVariable("ApiKey")    or ""
    local base_url   = ctx:GetVariable("BaseURL")   or "https://api.openai.com/v1"
    local model      = ctx:GetVariable("Model")     or "gpt-4o"
    local user_query = ctx:GetVariable("UserQuery") or
        "What's the weather in Beijing and Shanghai today, and any recent AI news?"

    if api_key == "" or api_key == "sk-your-api-key-here" then
        print("[LuaAgent] ERROR: ApiKey variable is not set.")
        print("[LuaAgent] Usage: runtime-example LuaReActAgent.bjson")
        print("[LuaAgent]   or via MCP: execute_blueprint with variables={\"ApiKey\":\"sk-xxx\"}")
        return true
    end

    print("[LuaAgent] Starting ReAct Agent (Lua implementation)")
    print("[LuaAgent] Query : " .. user_query)
    print("[LuaAgent] Model : " .. model .. " @ " .. base_url)
    print("")

    run_react_agent(user_query, api_key, base_url, model, function(result)
        print("")
        print("═══════════════════════════════════")
        print("[LuaAgent] Final Answer:")
        print(result)
        print("═══════════════════════════════════")
    end)

    return true
end)

print("[BlueprintEntry] LuaReActAgent: OnBeginPlay handler registered.")
