"""
BlueprintEditor MCP Server
--------------------------
Exposes BlueprintRuntime as MCP tools for Claude Desktop / Cursor / any MCP client.

Usage:
  python tools/mcp_server.py

Claude Desktop config (~/.claude/claude_desktop_config.json):
  {
    "mcpServers": {
      "blueprint": {
        "command": "py",
        "args": ["C:/Users/maxweili/MyProjects/MyBlueprintEditor/tools/mcp_server.py"],
        "env": {
          "BLUEPRINT_DLL": "C:/Users/maxweili/MyProjects/MyBlueprintEditor/build-windows/bin/Release/BlueprintRuntime.dll",
          "BLUEPRINT_ROOT": "C:/Users/maxweili/MyProjects/MyBlueprintEditor"
        }
      }
    }
  }

Variable injection (pass via execute_blueprint "variables"):
  {"ApiKey": "sk-xxx", "BaseURL": "https://api.openai.com/v1", "Model": "gpt-4o"}

Available tools:
  list_blueprints       - List all .bjson files in the project
  get_blueprint_content - Read a blueprint file as JSON text
  execute_blueprint     - Load and execute a blueprint, return print output
  get_variable          - Read a variable value after execution
  set_variable          - Set a variable before execution
  create_blueprint      - Write a new blueprint from JSON text
  get_blueprint_schema  - Return .bjson format reference
"""

import os
import sys
import json
import ctypes
import ctypes.util
import pathlib
import traceback
from typing import Optional

import mcp.server.stdio
import mcp.types as types
from mcp.server import Server

# ---------------------------------------------------------------------------
# Config from environment
# ---------------------------------------------------------------------------
BLUEPRINT_ROOT = pathlib.Path(
    os.environ.get("BLUEPRINT_ROOT",
                   pathlib.Path(__file__).parent.parent)
).resolve()

BLUEPRINT_DLL = os.environ.get(
    "BLUEPRINT_DLL",
    str(BLUEPRINT_ROOT / "build-windows" / "bin" / "Release" / "BlueprintRuntime.dll")
)

# ---------------------------------------------------------------------------
# C API wrapper (imported from blueprint_mcp)
# ---------------------------------------------------------------------------
from blueprint_mcp import BlueprintLib
# ---------------------------------------------------------------------------
# Load DLL (lazy, once)
# ---------------------------------------------------------------------------
_bp_lib: Optional[BlueprintLib] = None
_bp_lib_error: Optional[str] = None

def _get_lib() -> BlueprintLib:
    global _bp_lib, _bp_lib_error
    if _bp_lib is not None:
        return _bp_lib
    if _bp_lib_error is not None:
        raise RuntimeError(_bp_lib_error)
    try:
        _bp_lib = BlueprintLib(BLUEPRINT_DLL)
        return _bp_lib
    except Exception as e:
        _bp_lib_error = f"Failed to load BlueprintRuntime.dll from '{BLUEPRINT_DLL}': {e}"
        raise RuntimeError(_bp_lib_error)


# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------
def _find_bjson_files(root: pathlib.Path) -> list[dict]:
    """Recursively find all .bjson files, return relative paths + metadata + variables."""
    results = []
    skip = {"build", "build-windows", "build-static", ".git", "external", "logs"}
    for p in root.rglob("*.bjson"):
        if any(part in skip for part in p.parts):
            continue
        rel = p.relative_to(root)
        entry: dict = {
            "path": str(rel).replace("\\", "/"),
            "name": p.stem,
            "size_bytes": p.stat().st_size,
        }
        # 尝试解析变量列表，帮助 AI 知道执行前需要注入哪些变量
        try:
            data = json.loads(p.read_text(encoding="utf-8"))
            vars_list = data.get("runtime", {}).get("variables", [])
            # 只返回非内部变量（不以 __ 开头）
            entry["variables"] = [
                {"name": v["name"], "dataType": v.get("dataType", 0),
                 "defaultValue": v.get("defaultValue", "")}
                for v in vars_list
                if not v.get("name", "").startswith("__")
            ]
        except Exception:
            entry["variables"] = []
        results.append(entry)
    results.sort(key=lambda x: x["path"])
    return results


# ---------------------------------------------------------------------------
# MCP Server
# ---------------------------------------------------------------------------
server = Server("blueprint-editor")


@server.list_tools()
async def list_tools() -> list[types.Tool]:
    return [
        types.Tool(
            name="list_blueprints",
            description=(
                "List all .bjson blueprint files in the project. "
                "Returns file paths, names and sizes. "
                "Use this to discover available blueprints before executing them."
            ),
            inputSchema={
                "type": "object",
                "properties": {
                    "subdirectory": {
                        "type": "string",
                        "description": "Optional subdirectory to search (relative to project root). "
                                       "If omitted, searches the whole project."
                    }
                },
                "required": []
            }
        ),
        types.Tool(
            name="get_blueprint_content",
            description=(
                "Read the JSON content of a blueprint file. "
                "Use this to inspect nodes, links, and variables before modifying or executing."
            ),
            inputSchema={
                "type": "object",
                "properties": {
                    "path": {
                        "type": "string",
                        "description": "Relative path to the .bjson file (e.g. 'examples/AgentDemo.bjson')"
                    }
                },
                "required": ["path"]
            }
        ),
        types.Tool(
            name="execute_blueprint",
            description=(
                "Load and execute a blueprint file. "
                "Runs BP_Execute (data-flow) then BP_DispatchEvent('OnBeginPlay') (event chain). "
                "Returns all PrintString output lines, warnings, and any error. "
                "Use this to test blueprints or run Agent blueprints."
            ),
            inputSchema={
                "type": "object",
                "properties": {
                    "path": {
                        "type": "string",
                        "description": "Relative path to the .bjson file"
                    },
                    "variables": {
                        "type": "object",
                        "description": "Optional variables to inject before execution (name -> value). "
                                       "Useful for passing API keys, user queries, etc.",
                        "additionalProperties": True
                    },
                    "dispatch_beginplay": {
                        "type": "boolean",
                        "description": "Whether to dispatch OnBeginPlay event after Execute. Default: true.",
                        "default": True
                    }
                },
                "required": ["path"]
            }
        ),
        types.Tool(
            name="execute_blueprint_json",
            description=(
                "Load and execute a blueprint from a JSON string (without saving to disk). "
                "Useful for testing a blueprint you just created with create_blueprint."
            ),
            inputSchema={
                "type": "object",
                "properties": {
                    "content": {
                        "type": "string",
                        "description": "Full blueprint JSON string (.bjson format)"
                    },
                    "variables": {
                        "type": "object",
                        "description": "Optional variables to inject before execution",
                        "additionalProperties": True
                    },
                    "dispatch_beginplay": {
                        "type": "boolean",
                        "description": "Whether to dispatch OnBeginPlay event after Execute. Default: true.",
                        "default": True
                    }
                },
                "required": ["content"]
            }
        ),
        types.Tool(
            name="get_variable",
            description=(
                "Execute a blueprint and retrieve a specific variable value after execution. "
                "Use this to get computed results from a blueprint."
            ),
            inputSchema={
                "type": "object",
                "properties": {
                    "path": {
                        "type": "string",
                        "description": "Relative path to the .bjson file"
                    },
                    "variable_name": {
                        "type": "string",
                        "description": "Name of the variable to retrieve"
                    },
                    "variables": {
                        "type": "object",
                        "description": "Optional input variables to inject",
                        "additionalProperties": True
                    }
                },
                "required": ["path", "variable_name"]
            }
        ),
        types.Tool(
            name="create_blueprint",
            description=(
                "Save a blueprint JSON string to a .bjson file. "
                "Use this to create new blueprints programmatically. "
                "The content must be valid blueprint JSON (see get_blueprint_content for format reference). "
                "Minimum required structure: runtime.metadata, runtime.nodes, runtime.links."
            ),
            inputSchema={
                "type": "object",
                "properties": {
                    "path": {
                        "type": "string",
                        "description": "Relative path to save the .bjson file (e.g. 'examples/MyBlueprint.bjson')"
                    },
                    "content": {
                        "type": "string",
                        "description": "Blueprint JSON content string"
                    },
                    "overwrite": {
                        "type": "boolean",
                        "description": "Whether to overwrite if file already exists. Default: false.",
                        "default": False
                    }
                },
                "required": ["path", "content"]
            }
        ),
        types.Tool(
            name="get_blueprint_schema",
            description=(
                "Return a brief description of the .bjson blueprint format, "
                "common node types, and pin data types. "
                "Use this as a reference when creating new blueprints."
            ),
            inputSchema={
                "type": "object",
                "properties": {},
                "required": []
            }
        ),
    ]


@server.call_tool()
async def call_tool(name: str, arguments: dict) -> list[types.TextContent]:

    # ------------------------------------------------------------------
    def _text(s: str) -> list[types.TextContent]:
        return [types.TextContent(type="text", text=s)]

    def _json(obj) -> list[types.TextContent]:
        return [types.TextContent(type="text", text=json.dumps(obj, ensure_ascii=False, indent=2))]

    # ------------------------------------------------------------------
    try:
        if name == "list_blueprints":
            subdir = arguments.get("subdirectory", "")
            search_root = BLUEPRINT_ROOT / subdir if subdir else BLUEPRINT_ROOT
            files = _find_bjson_files(search_root)
            return _json({"blueprints": files, "count": len(files),
                          "project_root": str(BLUEPRINT_ROOT)})

        elif name == "get_blueprint_content":
            path = arguments["path"]
            full = BLUEPRINT_ROOT / path
            if not full.exists():
                return _text(f"ERROR: File not found: {path}")
            content = full.read_text(encoding="utf-8")
            return _text(content)

        elif name == "execute_blueprint":
            path = arguments["path"]
            full = BLUEPRINT_ROOT / path
            if not full.exists():
                return _text(f"ERROR: File not found: {path}")
            variables = arguments.get("variables")
            dispatch  = arguments.get("dispatch_beginplay", True)
            lib = _get_lib()
            # 传入 file_path 使用 BP_LoadFromFile，自动加载 dependencies 声明的函数库
            result = lib.execute("", variables, dispatch, file_path=str(full))
            return _json(result)

        elif name == "execute_blueprint_json":
            content   = arguments["content"]
            variables = arguments.get("variables")
            dispatch  = arguments.get("dispatch_beginplay", True)
            lib = _get_lib()
            result = lib.execute(content, variables, dispatch_beginplay=dispatch)
            return _json(result)

        elif name == "get_variable":
            path = arguments["path"]
            var_name = arguments["variable_name"]
            full = BLUEPRINT_ROOT / path
            if not full.exists():
                return _text(f"ERROR: File not found: {path}")
            variables = arguments.get("variables")
            lib = _get_lib()
            result = lib.get_variable_after_execute("", var_name, variables,
                                                    file_path=str(full))
            return _json(result)

        elif name == "create_blueprint":
            path      = arguments["path"]
            content   = arguments["content"]
            overwrite = arguments.get("overwrite", False)
            full = BLUEPRINT_ROOT / path
            if full.exists() and not overwrite:
                return _text(f"ERROR: File already exists: {path}. Pass overwrite=true to replace.")
            # Validate JSON
            try:
                json.loads(content)
            except json.JSONDecodeError as e:
                return _text(f"ERROR: Invalid JSON: {e}")
            full.parent.mkdir(parents=True, exist_ok=True)
            full.write_text(content, encoding="utf-8")
            return _text(f"OK: Blueprint saved to {path}")

        elif name == "get_blueprint_schema":
            schema = {
                "overview": "A .bjson file has two top-level keys: 'runtime' (required) and 'editor' (optional).",
                "runtime_structure": {
                    "metadata": {
                        "blueprintClass": "0=Actor(has OnBeginPlay), 1=FunctionLibrary",
                        "name": "Blueprint name",
                        "schemaVersion": 2
                    },
                    "nodes": "Array of NodeInstance objects",
                    "links": "Array of LinkInstance objects",
                    "variables": "Array of VariableDefinition objects"
                },
                "node_structure": {
                    "id": "Unique integer ID",
                    "definitionId": "Node type identifier (see node_types)",
                    "name": "Display name",
                    "pins": "Array of PinInfo objects"
                },
                "pin_structure": {
                    "id": "Unique integer ID",
                    "kind": "0=Input, 1=Output",
                    "dataType": "See data_types",
                    "isExec": "true if this is an execution flow pin",
                    "name": "Pin name (empty string for unnamed exec pins)",
                    "defaultValue": "Default value (string/number/bool/array)"
                },
                "variable_structure": {
                    "name": "Variable name",
                    "dataType": "See data_types (4=String most common)",
                    "defaultValue": "Default value"
                },
                "link_structure": {
                    "id": "Unique integer ID",
                    "startPinId": "Output pin ID",
                    "endPinId": "Input pin ID",
                    "isEnabled": "true"
                },
                "data_types": {
                    "0": "Unknown/Flow (exec pins)",
                    "1": "Boolean",
                    "2": "Integer",
                    "3": "Float",
                    "4": "String",
                    "6": "Array (JSON array string)",
                    "9": "Any"
                },
                "node_types": {
                    "control_flow": {
                        "OnBeginPlay": "Entry point. Out: exec",
                        "Branch": "If/else. In: exec,'Condition'(bool). Out: exec 'True','False'",
                        "ForLoop": "In: exec,'First Index'(int),'Last Index'(int). Out: exec 'LoopBody','Completed'; 'Index'(int)",
                        "WhileLoop": "In: exec,'Condition'(bool). Out: exec 'Loop Body','Completed'",
                        "FireEvent": "Fire a named event. In: exec,'EventName'(str). Out: exec",
                        "CustomEventNode": "Receives a named event. nodeData:{EventName}. Out: exec,'EventName'(str)"
                    },
                    "variables": {
                        "SetVariable": "In: exec,'Name'(str),'Value'(any). Out: exec",
                        "GetVariable": "In: 'Name'(str). Out: 'Value'(any)"
                    },
                    "string_ops": {
                        "PrintString": "In: exec,'In String'(str). Out: exec",
                        "FormatString": "In: 'Format'(str), named args(str). Out: 'Result'(str). Use {0},{1}... in Format",
                        "AppendString": "In: 'A','B'(str). Out: 'Result'(str)",
                        "String.Template": "In: 'Template'(str),'Keys'(array),'Values'(array). Out: 'Result'(str). Use {{key}} in Template"
                    },
                    "json_ops": {
                        "JSON.MakeMessage": "Build chat message. In: 'Role','Content','ToolCallId'(str). Out: 'Message'(str)",
                        "JSON.ArrayPush": "In: exec,'JSON'(str),'Element'(any). Out: exec,'JSON'(str),'Length'(int)",
                        "JSON.GetPath": "In: 'JSON','Path'(str). Out: 'Value'(str),'Found'(bool). Path: 'key', '[0]', '[0].key'",
                        "JSON.Extract": "In: 'JSON','Path'(str). Out: 'Value'(str)"
                    },
                    "network": {
                        "HTTP.Get": "In: exec,'URL','Headers'(str),'TimeoutSeconds'(int). Out: exec 'onSuccess','onError'; 'StatusCode'(int),'ResponseBody','ErrorMessage'(str)",
                        "HTTP.Post": "In: exec,'URL','Body','ContentType','Headers'(str),'TimeoutSeconds'(int). Out: exec 'onSuccess','onError'; 'StatusCode'(int),'ResponseBody','ErrorMessage'(str)",
                        "HTTP.Download": "In: exec,'URL'(str). Out: exec 'onSuccess','onError'; 'Content','ErrorMessage'(str)",
                        "HTTP.Retry": "Auto-retry HTTP request. In: exec,'URL','Method','Body','Headers'(str),'MaxRetries'(int),'RetryDelaySec'(float),'RetryOnStatus'(str e.g.'429,503'),'TimeoutSeconds'(int). Out: exec 'onSuccess','onError'; 'StatusCode'(int),'ResponseBody','RetryCount'(int),'ErrorMessage'(str)",
                        "Web.Search": "DuckDuckGo (no API key). In: exec,'Query'(str),'MaxResults'(int). Out: exec 'onSuccess','onError'; 'Results'(JSON),'ResultText','ErrorMessage'(str)",
                        "Code.Run": "Run subprocess. In: exec,'Command','WorkDir'(str),'TimeoutSeconds'(int). Out: exec 'onSuccess','onError'; 'Stdout','Stderr','ExitCode'(str)"
                    },
                    "ai_llm": {
                        "LLM.Chat": "OpenAI-compat chat. In: exec,'BaseURL','ApiKey','Model','Messages','SystemPrompt'(str),'MaxTokens'(int),'Temperature'(float),'Tools'(str). Out: exec 'onReply','onToolCall','onError'; 'Reply','ToolCallsJSON','FinishReason','FullResponse','ErrorMessage'(str)",
                        "LLM.StreamChat": "Streaming chat. Same inputs as LLM.Chat plus 'Stream'(bool). Out: exec 'onChunk','onDone','onToolCall','onError'; 'Chunk','Reply','ToolCallsJSON','ErrorMessage'(str)"
                    },
                    "ai_tools": {
                        "Tool.ForEach": "Iterate tool calls. In: exec,'ToolCallsJSON'(str). Out: exec 'onTool','onDone'; 'ToolName','Arguments','ToolCallId'(str),'Index'(int)",
                        "Tool.ForEachParallel": "Parallel tool calls. Same as Tool.ForEach but fires all onTool in parallel",
                        "Tool.Match": "Route by tool name. In: exec,'ToolName'(str),'Case0'..'Case7'(str). Out: exec 'Match0'..'Match7','Default'; 'MatchedIndex'(int)",
                        "Tool.CallByName": "Dynamic tool dispatch via FuncLib. In: exec,'ToolName','Arguments'(str). Out: exec 'onSuccess','onError'; 'Result','ErrorMessage'(str)",
                        "Tool.Define": "Define a tool for LLM. In: 'Name','Description'(str), param pins. Out: 'ToolJSON'(str)",
                        "MCP.Call": "Call MCP server. In: exec,'ServerURL','ToolName','Arguments'(str),'Protocol'(str). Out: exec 'onSuccess','onError'; 'Result','RawResult','ErrorMessage'(str)"
                    },
                    "ai_memory": {
                        "Memory.LoadHistory": "Load chat history. In: exec,'HistoryId'(str). Out: exec 'onFound','onEmpty'; 'Messages'(str)",
                        "Memory.SaveHistory": "Save chat history. In: exec,'HistoryId','Messages'(str). Out: exec"
                    },
                    "ai_agent": {
                        "Agent.Plan": "Decompose a goal into steps. In: exec,'BaseURL','ApiKey','Model','Goal','AvailableTools'(str),'MaxSteps'(int),'MaxTokens'(int). Out: exec 'onDone','onError'; 'PlanJSON'(str JSON array of {tool,arguments,reason}),'StepCount'(int),'ErrorMessage'(str)",
                        "Agent.Reflect": "Evaluate output against criteria. In: exec,'BaseURL','ApiKey','Model','Output','Criteria'(str),'MaxTokens'(int). Out: exec 'onPass','onFail','onError'; 'Feedback','Score'(str pass|fail),'ErrorMessage'(str)",
                        "Context.Compress": "Compress old messages with LLM summary. In: exec,'BaseURL','ApiKey','Model','Messages'(str),'KeepRecent'(int),'MaxTokens'(int). Out: exec 'onDone','onError'; 'Compressed'(str JSON array),'ErrorMessage'(str)"
                    },
                    "ai_misc": {
                        "UserInput.Wait": "Wait for text input. In: exec,'Prompt'(str). Out: exec,'Input'(str)"
                    }
                },
                "best_practices": [
                    "Always add blueprint variables for ApiKey/BaseURL/Model so they can be injected via MCP variables parameter.",
                    "Use GetVariable nodes to read variables and connect to LLM.Chat pins instead of hardcoding in defaultValue.",
                    "Always use unique IDs for nodes and pins across the whole file.",
                    "Exec pins connect execution flow; data pins connect values.",
                    "OnBeginPlay is the standard entry point for Actor blueprints (blueprintClass=0).",
                    "Use PrintString to output results - they appear in 'output' after execute_blueprint.",
                    "Link startPinId must be an Output pin (kind=1), endPinId must be an Input pin (kind=0).",
                    "For multi-turn agents, use FireEvent+CustomEventNode to avoid recursive call stack overflow.",
                    "Tool.CallByName requires the tool handler to be registered as a FuncLib function.",
                    "HTTP.Get/Post/Web.Search are async - connect onSuccess/onError exec pins."
                ]
            }
            return _json(schema)

        else:
            return _text(f"ERROR: Unknown tool '{name}'")

    except RuntimeError as e:
        return _text(f"ERROR (DLL): {e}")
    except Exception as e:
        return _text(f"ERROR: {e}\n{traceback.format_exc()}")


# ---------------------------------------------------------------------------
# Entry point
# ---------------------------------------------------------------------------
async def main():
    async with mcp.server.stdio.stdio_server() as (read_stream, write_stream):
        await server.run(
            read_stream,
            write_stream,
            server.create_initialization_options()
        )


def run_http(host: str = "0.0.0.0", port: int = 7799):
    """HTTP REST 模式：任意程序可通过 POST /tool 调用工具，无需 MCP 客户端。

    请求格式：
      POST /tool
      Content-Type: application/json
      {"tool": "execute_blueprint", "arguments": {"path": "...", "variables": {...}}}

    响应格式：
      {"result": [...TextContent...], "error": null}
    """
    import asyncio
    from http.server import BaseHTTPRequestHandler, HTTPServer

    class Handler(BaseHTTPRequestHandler):
        def log_message(self, fmt, *args):
            pass  # 静默访问日志

        def do_POST(self):
            if self.path != "/tool":
                self.send_response(404)
                self.end_headers()
                return
            length = int(self.headers.get("Content-Length", 0))
            body = self.rfile.read(length)
            try:
                req = json.loads(body)
                tool_name = req.get("tool", "")
                arguments = req.get("arguments", {})
                result = asyncio.run(call_tool(tool_name, arguments))
                resp = json.dumps({
                    "result": [{"type": r.type, "text": r.text} for r in result],
                    "error": None
                }, ensure_ascii=False)
                self.send_response(200)
                self.send_header("Content-Type", "application/json; charset=utf-8")
                self.end_headers()
                self.wfile.write(resp.encode("utf-8"))
            except Exception as e:
                err = json.dumps({"result": [], "error": str(e)})
                self.send_response(500)
                self.send_header("Content-Type", "application/json; charset=utf-8")
                self.end_headers()
                self.wfile.write(err.encode("utf-8"))

        def do_GET(self):
            if self.path == "/health":
                self.send_response(200)
                self.send_header("Content-Type", "application/json")
                self.end_headers()
                self.wfile.write(b'{"status":"ok","server":"blueprint-mcp"}')
            else:
                self.send_response(404)
                self.end_headers()

    httpd = HTTPServer((host, port), Handler)
    print(f"[Blueprint MCP] HTTP mode listening on http://{host}:{port}/tool", flush=True)
    print(f"[Blueprint MCP] Health check: http://{host}:{port}/health", flush=True)
    httpd.serve_forever()


if __name__ == "__main__":
    import sys
    import asyncio

    if "--http" in sys.argv:
        host = "0.0.0.0"
        port = 7799
        for arg in sys.argv:
            if arg.startswith("--port="):
                port = int(arg.split("=", 1)[1])
            if arg.startswith("--host="):
                host = arg.split("=", 1)[1]
        run_http(host, port)
    else:
        asyncio.run(main())
