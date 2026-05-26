-- data/BlueprintEntry.lua
-- 编辑器全局 Lua 入口（编辑器启动时自动加载）
--
-- 职责：
--   1. 加载 data/examples/lua/agent_tools.lua
--   2. 注册自定义节点 "LuaAgent.RunReAct" 到编辑器节点注册表
--   3. 注册对应 handler 到编辑器 handler 注册表（执行蓝图时使用）

-- ── 确定本文件所在目录 ─────────────────────────────────────────────────────
local my_dir = ""
local info = debug.getinfo(1, "S")
if info and info.source and info.source:sub(1,1) == "@" then
    my_dir = info.source:sub(2):match("^(.*[/\\])") or ""
end

-- ── 加载工具库 ─────────────────────────────────────────────────────────────
local tools_path = my_dir .. "examples/lua/agent_tools.lua"
local ok, err = pcall(dofile, tools_path)
if not ok then
    print("[BlueprintEntry] Warning: could not load examples/lua/agent_tools.lua: " .. tostring(err))
    function run_react_agent(q, k, u, m, cb)
        cb("[Error] agent_tools.lua not loaded: " .. tostring(err))
    end
end

-- my_dir 是 data/ 的绝对路径（末尾有 /），往上一级就是项目根
-- 同时兼容 Windows 反斜杠和 Unix 正斜杠
local sep = package.config:sub(1,1)  -- OS 路径分隔符（\ 或 /）
local proj_root = my_dir:match("^(.*[/\\])data[/\\]?$") or (my_dir .. ".." .. sep)
-- Lua require 用 '.' 作模块分隔符，loader 需要把 '.' 替换成 '/'（或 sep）
-- 为了兼容，同时添加正斜杠和反斜杠两种 pattern
local lua_root = proj_root .. "UnityProj" .. sep .. "Assets" .. sep .. "Lua" .. sep
package.path = package.path
    .. ";" .. lua_root .. "?.lua"
    .. ";" .. lua_root .. "?" .. sep .. "init.lua"
print("[BlueprintEntry] Added Lua path: " .. lua_root)
local ok, err = pcall(require, "blueprints.BlueprintEntry")
if not ok then
    print('[BlueprintEntry] Warning: blueprints.BlueprintEntry load failed: ' .. tostring(err))
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
