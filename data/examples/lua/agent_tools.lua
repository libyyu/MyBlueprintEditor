-- examples/lua/agent_tools.lua
-- ReAct Agent 工具库（Lua 实现）
--
-- 工具列表：
--   get_weather(location, callback)   使用 wttr.in（无需 Key）
--   web_search(query, callback)       多后端搜索（自动选择可用后端）
--
-- 搜索后端优先级（由蓝图变量控制，未设置则按顺序自动降级）：
--   SearchProvider = "tavily"   → Tavily API（需 TavilyKey，免费 1000次/月，推荐）
--   SearchProvider = "brave"    → Brave Search API（需 BraveKey，免费 2000次/月）
--   SearchProvider = "serpapi"  → SerpApi（需 SerpApiKey，支持 Google）
--   SearchProvider = "bing"     → Bing HTML scraping（无 Key，不稳定）
--   SearchProvider = "ddg"      → DuckDuckGo Lite HTML scraping（无 Key，默认兜底）
--
-- 蓝图变量配置示例：
--   SearchProvider = "tavily"
--   TavilyKey      = "tvly-xxxxxxxx"
--   BraveKey       = "BSA..."
--   SerpApiKey     = "..."

-- ─────────────────────────────────────────────────────────────────────────────
-- async_http_get / async_http_post
-- 封装 http.get/post，配对调用 AcquireAsync/ReleaseAsync 驱动 Tick 循环
-- ─────────────────────────────────────────────────────────────────────────────
local function async_http_get(url, headers, callback)
    Blueprint.AcquireAsync()
    if type(headers) == "function" then
        local cb = headers
        http.get(url, function(body, status, err)
            Blueprint.ReleaseAsync()
            cb(body, status, err)
        end)
    else
        http.get(url, headers, function(body, status, err)
            Blueprint.ReleaseAsync()
            callback(body, status, err)
        end)
    end
end

local function async_http_post(url, body, headers, callback)
    Blueprint.AcquireAsync()
    http.post(url, body, headers, function(resp_body, status, err)
        Blueprint.ReleaseAsync()
        callback(resp_body, status, err)
    end)
end

-- ─────────────────────────────────────────────────────────────────────────────
-- URL 编码工具
-- ─────────────────────────────────────────────────────────────────────────────
local function url_encode(s)
    return s:gsub("([^%w%-%.%_%~])", function(c)
        return string.format("%%%02X", string.byte(c))
    end)
end

-- ─────────────────────────────────────────────────────────────────────────────
-- get_weather(location, callback) → 天气字符串
-- 使用 wttr.in，无需 API Key
-- ─────────────────────────────────────────────────────────────────────────────
function get_weather(location, callback)
    local url = "https://wttr.in/" .. url_encode(location) .. "?format=3"
    async_http_get(url, function(body, status, err)
        if err ~= "" or status < 200 or status >= 300 then
            callback("Weather fetch failed for " .. location .. ": " .. (err ~= "" and err or tostring(status)))
        else
            callback(body)
        end
    end)
end

-- ─────────────────────────────────────────────────────────────────────────────
-- 各搜索后端实现
-- ─────────────────────────────────────────────────────────────────────────────

-- Tavily Search API（专为 AI Agent 设计，结果质量最佳）
-- 文档：https://docs.tavily.com/docs/tavily-api/rest_api
local function search_tavily(query, api_key, callback)
    local req_body = json.stringify({
        api_key        = api_key,
        query          = query,
        search_depth   = "basic",
        max_results    = 5,
        include_answer = true,
    })
    local headers = { ["Content-Type"] = "application/json" }
    async_http_post("https://api.tavily.com/search", req_body, headers, function(body, status, err)
        if err ~= "" or status < 200 or status >= 300 then
            callback(nil, "Tavily error: " .. (err ~= "" and err or "HTTP " .. status))
            return
        end
        local resp = json.parse(body)
        if not resp then callback(nil, "Tavily: failed to parse response") return end

        local text = ""
        -- 如果有 AI 直接回答，优先展示
        local answer = json.get(body, "answer")
        if answer and answer ~= "" and answer ~= "null" then
            text = "Answer: " .. answer .. "\n\n"
        end
        -- 搜索结果列表
        local results = json.get(body, "results")
        if type(results) == "table" then
            for i, r in ipairs(results) do
                if i > 5 then break end
                local title   = json.get(json.stringify(r), "title")   or ""
                local url_val = json.get(json.stringify(r), "url")     or ""
                local content = json.get(json.stringify(r), "content") or ""
                text = text .. i .. ". " .. title .. "\n"
                             .. "   " .. url_val .. "\n"
                             .. "   " .. content:sub(1, 200) .. "\n\n"
            end
        end
        if text == "" then text = "No results from Tavily for: " .. query end
        callback(text, nil)
    end)
end

-- Brave Search API
-- 文档：https://api.search.brave.com/app/documentation/web-search
local function search_brave(query, api_key, callback)
    local url = "https://api.search.brave.com/res/v1/web/search?q=" .. url_encode(query) .. "&count=5"
    local headers = {
        ["Accept"]              = "application/json",
        ["Accept-Encoding"]     = "gzip",
        ["X-Subscription-Token"] = api_key,
    }
    async_http_get(url, headers, function(body, status, err)
        if err ~= "" or status < 200 or status >= 300 then
            callback(nil, "Brave error: " .. (err ~= "" and err or "HTTP " .. status))
            return
        end
        local text = ""
        local results = json.get(body, "web.results")
        if type(results) == "table" then
            for i, r in ipairs(results) do
                if i > 5 then break end
                local rs = json.stringify(r)
                local title       = json.get(rs, "title")       or ""
                local url_val     = json.get(rs, "url")         or ""
                local description = json.get(rs, "description") or ""
                text = text .. i .. ". " .. title .. "\n"
                             .. "   " .. url_val .. "\n"
                             .. "   " .. description:sub(1, 200) .. "\n\n"
            end
        end
        if text == "" then text = "No results from Brave for: " .. query end
        callback(text, nil)
    end)
end

-- SerpApi（Google 搜索，付费）
-- 文档：https://serpapi.com/search-api
local function search_serpapi(query, api_key, callback)
    local url = "https://serpapi.com/search.json?q=" .. url_encode(query)
                .. "&api_key=" .. api_key .. "&num=5&engine=google"
    async_http_get(url, function(body, status, err)
        if err ~= "" or status < 200 or status >= 300 then
            callback(nil, "SerpApi error: " .. (err ~= "" and err or "HTTP " .. status))
            return
        end
        local text = ""
        local results = json.get(body, "organic_results")
        if type(results) == "table" then
            for i, r in ipairs(results) do
                if i > 5 then break end
                local rs      = json.stringify(r)
                local title   = json.get(rs, "title")   or ""
                local link    = json.get(rs, "link")    or ""
                local snippet = json.get(rs, "snippet") or ""
                text = text .. i .. ". " .. title .. "\n"
                             .. "   " .. link .. "\n"
                             .. "   " .. snippet:sub(1, 200) .. "\n\n"
            end
        end
        if text == "" then text = "No results from SerpApi for: " .. query end
        callback(text, nil)
    end)
end

-- Bing HTML scraping（无 Key，不稳定，备用）
local function search_bing(query, callback)
    local url = "https://www.bing.com/search?q=" .. url_encode(query) .. "&count=5"
    local headers = { ["User-Agent"] = "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36" }
    async_http_get(url, headers, function(body, status, err)
        if err ~= "" or status < 200 or status >= 300 then
            callback(nil, "Bing error: " .. (err ~= "" and err or "HTTP " .. status))
            return
        end
        local results = {}
        -- Bing 结果在 <li class="b_algo"> 里
        for block in body:gmatch('<li class="b_algo">(.-)</li>') do
            if #results >= 5 then break end
            local title = block:match('<h2[^>]*><a[^>]*>([^<]+)') or ""
            local href  = block:match('<h2[^>]*><a href="([^"]+)"') or ""
            local snippet = block:match('<p[^>]*>([^<]+)') or ""
            if title ~= "" then
                table.insert(results, { title = title, url = href, snippet = snippet })
            end
        end
        if #results == 0 then
            callback(nil, "No Bing results for: " .. query)
            return
        end
        local text = ""
        for i, r in ipairs(results) do
            text = text .. i .. ". " .. r.title .. "\n"
                       .. "   " .. r.url .. "\n"
                       .. "   " .. r.snippet:sub(1, 200) .. "\n\n"
        end
        callback(text, nil)
    end)
end

-- DuckDuckGo Lite HTML scraping（无 Key，默认兜底）
local function search_ddg(query, callback)
    local url = "https://lite.duckduckgo.com/lite/?q=" .. url_encode(query)
    local headers = { ["User-Agent"] = "Mozilla/5.0 (compatible; BlueprintLua/1.0)" }
    async_http_get(url, headers, function(body, status, err)
        if err ~= "" or status < 200 or status >= 300 then
            callback(nil, "DDG error: " .. (err ~= "" and err or "HTTP " .. status))
            return
        end
        local results = {}
        for href, title in body:gmatch('class="result%-link"[^>]*href="([^"]+)"[^>]*>([^<]+)') do
            if #results >= 5 then break end
            local snippet = body:match('class="result%-snippet">([^<]+)', body:find(href, 1, true) or 1)
            snippet = snippet and snippet:match("^%s*(.-)%s*$") or ""
            table.insert(results, {
                title   = title:match("^%s*(.-)%s*$"),
                url     = href,
                snippet = snippet,
            })
        end
        if #results == 0 then
            callback(nil, "No DDG results for: " .. query)
            return
        end
        local text = ""
        for i, r in ipairs(results) do
            text = text .. i .. ". " .. r.title .. "\n"
                       .. "   " .. r.url .. "\n"
                       .. "   " .. r.snippet .. "\n\n"
        end
        callback(text, nil)
    end)
end

-- ─────────────────────────────────────────────────────────────────────────────
-- web_search(query, callback) → 搜索结果文本
--
-- 根据蓝图变量 SearchProvider 自动选择后端，失败时降级到下一个。
-- 优先级：tavily > brave > serpapi > bing > ddg
-- ─────────────────────────────────────────────────────────────────────────────

-- 全局搜索配置（由蓝图变量注入，或直接在此处填写 Key 进行测试）
-- 在蓝图 Variables 面板设置对应变量即可，无需修改此文件。
SEARCH_CONFIG = SEARCH_CONFIG or {
    provider   = "",    -- "tavily"/"brave"/"serpapi"/"bing"/"ddg"，空=自动
    tavily_key = "",    -- Blueprint 变量 TavilyKey
    brave_key  = "",    -- Blueprint 变量 BraveKey
    serpapi_key = "",   -- Blueprint 变量 SerpApiKey
}

-- 初始化时从蓝图变量读取配置（BlueprintEntry.lua 加载时执行一次）
do
    local function getvar(name)
        -- Blueprint.GetVariable 由 LuaBindings 注册
        if Blueprint.GetVariable then
            return Blueprint.GetVariable(name) or ""
        end
        return ""
    end
    local p = getvar("SearchProvider")
    if p ~= "" then SEARCH_CONFIG.provider    = p end
    local tk = getvar("TavilyKey")
    if tk ~= "" then SEARCH_CONFIG.tavily_key = tk end
    local bk = getvar("BraveKey")
    if bk ~= "" then SEARCH_CONFIG.brave_key  = bk end
    local sk = getvar("SerpApiKey")
    if sk ~= "" then SEARCH_CONFIG.serpapi_key = sk end
end

function web_search(query, callback)
    local provider = SEARCH_CONFIG.provider

    -- 自动选择：优先使用有 Key 的后端
    if provider == "" then
        if SEARCH_CONFIG.tavily_key  ~= "" then provider = "tavily"
        elseif SEARCH_CONFIG.brave_key  ~= "" then provider = "brave"
        elseif SEARCH_CONFIG.serpapi_key ~= "" then provider = "serpapi"
        else provider = "ddg" end
    end

    print("[Search] Using provider: " .. provider)

    local function on_result(text, err)
        if err then
            -- 失败自动降级到 DDG
            print("[Search] " .. provider .. " failed: " .. err .. ", falling back to DDG")
            search_ddg(query, function(t2, e2)
                if e2 then callback("Search failed: " .. e2)
                else        callback(t2) end
            end)
        else
            callback(text)
        end
    end

    if provider == "tavily" then
        if SEARCH_CONFIG.tavily_key == "" then
            print("[Search] TavilyKey not set, falling back to DDG")
            search_ddg(query, function(t, e) if e then callback("Search failed: " .. e) else callback(t) end end)
        else
            search_tavily(query, SEARCH_CONFIG.tavily_key, on_result)
        end
    elseif provider == "brave" then
        if SEARCH_CONFIG.brave_key == "" then
            print("[Search] BraveKey not set, falling back to DDG")
            search_ddg(query, function(t, e) if e then callback("Search failed: " .. e) else callback(t) end end)
        else
            search_brave(query, SEARCH_CONFIG.brave_key, on_result)
        end
    elseif provider == "serpapi" then
        if SEARCH_CONFIG.serpapi_key == "" then
            print("[Search] SerpApiKey not set, falling back to DDG")
            search_ddg(query, function(t, e) if e then callback("Search failed: " .. e) else callback(t) end end)
        else
            search_serpapi(query, SEARCH_CONFIG.serpapi_key, on_result)
        end
    elseif provider == "bing" then
        search_bing(query, on_result)
    else
        -- ddg 或未知
        search_ddg(query, function(t, e) if e then callback("Search failed: " .. e) else callback(t) end end)
    end
end

-- ─────────────────────────────────────────────────────────────────────────────
-- run_react_agent(query, api_key, base_url, model, on_done)
-- ─────────────────────────────────────────────────────────────────────────────
local TOOLS_DEF = json.stringify({
    {
        type = "function",
        ["function"] = {
            name = "get_weather",
            description = "Get current weather for a city",
            parameters = {
                type = "object",
                properties = {
                    location = { type = "string", description = "City name in English" }
                },
                required = { "location" }
            }
        }
    },
    {
        type = "function",
        ["function"] = {
            name = "web_search",
            description = "Search the web for recent information, news, or facts",
            parameters = {
                type = "object",
                properties = {
                    query = { type = "string", description = "Search query" }
                },
                required = { "query" }
            }
        }
    }
})

function run_react_agent(user_query, api_key, base_url, model, on_done)
    base_url = base_url or "https://api.openai.com/v1"
    model    = model    or "gpt-4o"

    local messages = json.stringify({
        { role = "user", content = user_query }
    })

    local max_rounds = 6
    local round = 0

    local function agent_loop()
        round = round + 1
        if round > max_rounds then
            on_done("[Agent] Max rounds reached without final answer.")
            return
        end

        print("[Agent] Round " .. round .. " / calling LLM...")

        local req_body = json.stringify({
            model       = model,
            messages    = json.parse(messages),
            tools       = json.parse(TOOLS_DEF),
            max_tokens  = 1024,
            temperature = 0.7,
        })

        local req_headers = {
            ["Authorization"] = "Bearer " .. api_key,
            ["Content-Type"]  = "application/json",
        }
        local llm_url = (base_url:gsub("/$", "")) .. "/chat/completions"

        async_http_post(llm_url, req_body, req_headers, function(body, status, err)
            if err ~= "" or status < 200 or status >= 300 then
                on_done("[Agent] LLM error: " .. (err ~= "" and err or ("HTTP " .. status)))
                return
            end

            local resp = json.parse(body)
            if not resp then
                on_done("[Agent] Failed to parse LLM response")
                return
            end

            local finish_reason  = json.get(body, "choices[0].finish_reason") or ""
            local content        = json.get(body, "choices[0].message.content") or ""
            local tool_calls_raw = json.get(body, "choices[0].message.tool_calls")

            -- 追加 assistant 消息到 messages
            local asst_msg
            if tool_calls_raw then
                asst_msg = json.stringify({
                    role       = "assistant",
                    content    = content,
                    tool_calls = json.parse(json.stringify(tool_calls_raw)),
                })
            else
                asst_msg = json.stringify({ role = "assistant", content = content })
            end
            local msgs = json.parse(messages)
            table.insert(msgs, json.parse(asst_msg))
            messages = json.stringify(msgs)

            -- 无 tool_calls → 最终回答
            if not tool_calls_raw then
                local answer = (content ~= "") and content or "[Agent] Empty response from LLM"
                print("[Agent] Final answer:\n" .. answer)
                on_done(answer)
                return
            end
            if finish_reason == "stop" then
                print("[Agent] Final answer:\n" .. content)
                on_done(content)
                return
            end

            -- 有工具调用 → 并发执行
            local tool_calls = json.parse(json.stringify(tool_calls_raw))
            if type(tool_calls) ~= "table" or #tool_calls == 0 then
                on_done("[Agent] Empty tool_calls")
                return
            end

            local pending = #tool_calls
            local tool_results = {}
            for i = 1, #tool_calls do tool_results[i] = nil end

            local function on_tool_done(idx)
                pending = pending - 1
                if pending > 0 then return end

                local msgs2 = json.parse(messages)
                for i = 1, #tool_calls do
                    local tc_id = json.get(json.stringify(tool_calls[i]), "id") or ""
                    table.insert(msgs2, {
                        role         = "tool",
                        tool_call_id = tc_id,
                        content      = tool_results[i] or "",
                    })
                end
                messages = json.stringify(msgs2)
                agent_loop()
            end

            for i, tc in ipairs(tool_calls) do
                local tc_str    = json.stringify(tc)
                local func_name = json.get(tc_str, "function.name") or ""
                local args_str  = json.get(tc_str, "function.arguments") or "{}"
                local args      = json.parse(args_str) or {}

                print("[Agent] Tool call: " .. func_name .. "(" .. args_str .. ")")

                if func_name == "get_weather" then
                    local loc = (type(args) == "table" and args.location) or ""
                    get_weather(loc, function(result)
                        tool_results[i] = result
                        on_tool_done(i)
                    end)
                elseif func_name == "web_search" then
                    local q = (type(args) == "table" and args.query) or ""
                    web_search(q, function(result)
                        tool_results[i] = result
                        on_tool_done(i)
                    end)
                else
                    tool_results[i] = "Unknown tool: " .. func_name
                    on_tool_done(i)
                end
            end
        end)
    end

    agent_loop()
end
