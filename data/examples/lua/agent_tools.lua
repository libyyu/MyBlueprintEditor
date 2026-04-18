-- examples/lua/agent_tools.lua
-- ReAct Agent 工具库（Lua 实现）
--
-- 提供两个工具：
--   get_weather(location) → string   使用 wttr.in（无需 API Key）
--   web_search(query)     → string   使用 DuckDuckGo Lite HTML 解析
--
-- 用法：
--   1. 蓝图变量中设置 ApiKey / BaseURL / Model / UserQuery
--   2. ExecuteBlueprint 运行 examples/LuaReActAgent.bjson
--   3. 该蓝图通过 Lua 脚本节点调用本文件中的函数

-- ─────────────────────────────────────────────────────────────────────────────
-- async_http_get / async_http_post
-- 封装 http.get/post，在发起请求前调用 Blueprint.AcquireAsync()，
-- 在回调执行后调用 Blueprint.ReleaseAsync()，
-- 使 runner.HasPendingWork() 在请求期间返回 true，驱动 Tick 循环。
-- ─────────────────────────────────────────────────────────────────────────────
local function async_http_get(url, headers, callback)
    Blueprint.AcquireAsync()
    if type(headers) == "function" then
        -- 无 headers 版本：async_http_get(url, callback)
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
-- get_weather(location, callback) → 天气字符串
-- ─────────────────────────────────────────────────────────────────────────────
function get_weather(location, callback)
    local url = "https://wttr.in/" .. location .. "?format=3"
    async_http_get(url, function(body, status, err)
        if err ~= "" or status < 200 or status >= 300 then
            callback("Weather fetch failed for " .. location .. ": " .. (err ~= "" and err or tostring(status)))
        else
            -- wttr.in format=3 返回类似: "Beijing: 🌤  +18°C"
            callback(body)
        end
    end)
end

-- ─────────────────────────────────────────────────────────────────────────────
-- web_search(query, callback) → 搜索结果文本
-- ─────────────────────────────────────────────────────────────────────────────
function web_search(query, callback)
    local encoded = query:gsub("([^%w%-%.%_%~])", function(c)
        return string.format("%%%02X", string.byte(c))
    end)
    local url = "https://lite.duckduckgo.com/lite/?q=" .. encoded
    local headers = { ["User-Agent"] = "Mozilla/5.0 (compatible; BlueprintLua/1.0)" }

    async_http_get(url, headers, function(body, status, err)
        if err ~= "" or status < 200 or status >= 300 then
            callback("Search failed: " .. (err ~= "" and err or tostring(status)))
            return
        end

        local results = {}
        local max = 5
        for href, title in body:gmatch('class="result%-link"[^>]*href="([^"]+)"[^>]*>([^<]+)') do
            if #results >= max then break end
            local snippet = body:match('class="result%-snippet">([^<]+)', body:find(href, 1, true) or 1)
            snippet = snippet and snippet:match("^%s*(.-)%s*$") or ""
            table.insert(results, {
                title   = title:match("^%s*(.-)%s*$"),
                url     = href,
                snippet = snippet,
            })
        end

        if #results == 0 then
            callback("No results found for: " .. query)
            return
        end

        local text = ""
        for i, r in ipairs(results) do
            text = text .. i .. ". " .. r.title .. "\n"
                       .. "   " .. r.url .. "\n"
                       .. "   " .. r.snippet .. "\n\n"
        end
        callback(text)
    end)
end

-- ─────────────────────────────────────────────────────────────────────────────
-- run_react_agent(query, api_key, base_url, model, on_done)
--
-- ReAct 循环：
--   1. 构建初始 messages = [user: query]
--   2. 调用 LLM，传入 tools 定义
--   3. finish_reason == "stop" → 最终回答，调用 on_done
--   4. tool_calls → 并发执行工具，追加结果，继续循环
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
            description = "Search the web for recent information",
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

            -- finish_reason == "stop" 或无 tool_calls → 最终回答
            if finish_reason == "stop" or not tool_calls_raw then
                print("[Agent] Final answer:\n" .. content)
                on_done(content)
                return
            end

            -- 有工具调用 → 并发执行所有 tool_calls
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

                -- 所有工具完成，追加 tool 消息，进入下一轮
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
