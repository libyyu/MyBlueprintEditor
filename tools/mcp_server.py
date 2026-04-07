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
          "BLUEPRINT_DLL": "C:/Users/maxweili/MyProjects/MyBlueprintEditor/build/bin/Release/BlueprintRuntime.dll",
          "BLUEPRINT_ROOT": "C:/Users/maxweili/MyProjects/MyBlueprintEditor"
        }
      }
    }
  }

Available tools:
  list_blueprints       - List all .bjson files in the project
  get_blueprint_content - Read a blueprint file as JSON text
  execute_blueprint     - Load and execute a blueprint, return print output
  get_variable          - Read a variable value after execution
  set_variable          - Set a variable before execution
  create_blueprint      - Write a new blueprint from JSON text
  list_node_defs        - (future) Query registered node type definitions
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
    str(BLUEPRINT_ROOT / "build" / "bin" / "Release" / "BlueprintRuntime.dll")
)

# ---------------------------------------------------------------------------
# C API wrapper
# ---------------------------------------------------------------------------
class BlueprintLib:
    """Thin ctypes wrapper around BlueprintRuntime.dll C API."""

    def __init__(self, dll_path: str):
        self._lib = ctypes.CDLL(dll_path)
        self._setup_signatures()
        self._print_lines: list[str] = []
        self._log_lines:   list[str] = []

        # 注册默认 HTTP client（幂等，已注册则跳过）
        self._lib.BP_InitDefaultHttpClient()

        # keep callback alive (ctypes GC protection)
        LogCb = ctypes.CFUNCTYPE(None, ctypes.c_int, ctypes.c_char_p)
        self._print_cb = LogCb(self._on_print)
        self._log_cb   = LogCb(self._on_log)

    def _setup_signatures(self):
        lib = self._lib

        lib.BP_InitDefaultHttpClient.restype  = None
        lib.BP_InitDefaultHttpClient.argtypes = []

        lib.BP_CreateRunner.restype  = ctypes.c_void_p
        lib.BP_CreateRunner.argtypes = []

        lib.BP_DestroyRunner.restype  = None
        lib.BP_DestroyRunner.argtypes = [ctypes.c_void_p]

        lib.BP_LoadFromJson.restype  = ctypes.c_int
        lib.BP_LoadFromJson.argtypes = [ctypes.c_void_p, ctypes.c_char_p]

        lib.BP_LoadFromJsonWithBaseDir.restype  = ctypes.c_int
        lib.BP_LoadFromJsonWithBaseDir.argtypes = [ctypes.c_void_p, ctypes.c_char_p, ctypes.c_char_p]

        lib.BP_LoadFromFile.restype  = ctypes.c_int
        lib.BP_LoadFromFile.argtypes = [ctypes.c_void_p, ctypes.c_char_p]

        lib.BP_Execute.restype  = ctypes.c_int
        lib.BP_Execute.argtypes = [ctypes.c_void_p]

        lib.BP_DispatchEvent.restype  = ctypes.c_int
        lib.BP_DispatchEvent.argtypes = [ctypes.c_void_p, ctypes.c_char_p]

        lib.BP_SetVariableString.restype  = None
        lib.BP_SetVariableString.argtypes = [ctypes.c_void_p, ctypes.c_char_p, ctypes.c_char_p]

        lib.BP_SetVariableInt.restype  = None
        lib.BP_SetVariableInt.argtypes = [ctypes.c_void_p, ctypes.c_char_p, ctypes.c_int64]

        lib.BP_SetVariableFloat.restype  = None
        lib.BP_SetVariableFloat.argtypes = [ctypes.c_void_p, ctypes.c_char_p, ctypes.c_double]

        lib.BP_GetVariableString.restype  = ctypes.c_int
        lib.BP_GetVariableString.argtypes = [ctypes.c_void_p, ctypes.c_char_p,
                                             ctypes.c_char_p, ctypes.c_int]

        lib.BP_GetVariableInt.restype  = ctypes.c_int64
        lib.BP_GetVariableInt.argtypes = [ctypes.c_void_p, ctypes.c_char_p]

        lib.BP_GetVariableFloat.restype  = ctypes.c_double
        lib.BP_GetVariableFloat.argtypes = [ctypes.c_void_p, ctypes.c_char_p]

        lib.BP_GetLastError.restype  = ctypes.c_int
        lib.BP_GetLastError.argtypes = [ctypes.c_void_p, ctypes.c_char_p, ctypes.c_int]

        lib.BP_IsLoaded.restype  = ctypes.c_int
        lib.BP_IsLoaded.argtypes = [ctypes.c_void_p]

        lib.BP_Tick.restype  = None
        lib.BP_Tick.argtypes = [ctypes.c_void_p, ctypes.c_float]

        lib.BP_GetActiveTimerCount.restype  = ctypes.c_int
        lib.BP_GetActiveTimerCount.argtypes = [ctypes.c_void_p]

        lib.BP_HasPendingWork.restype  = ctypes.c_int
        lib.BP_HasPendingWork.argtypes = [ctypes.c_void_p]

        lib.BP_DrainQueue.restype  = None
        lib.BP_DrainQueue.argtypes = []

        lib.BP_SetBasePath.restype  = None
        lib.BP_SetBasePath.argtypes = [ctypes.c_void_p, ctypes.c_char_p]

        LogCbType = ctypes.CFUNCTYPE(None, ctypes.c_int, ctypes.c_char_p)
        lib.BP_SetPrintCallback.restype  = None
        lib.BP_SetPrintCallback.argtypes = [ctypes.c_void_p, LogCbType]

        lib.BP_SetLogCallback.restype  = None
        lib.BP_SetLogCallback.argtypes = [ctypes.c_void_p, LogCbType]

    def _on_print(self, level: int, msg: bytes):
        self._print_lines.append(msg.decode("utf-8", errors="replace"))

    def _on_log(self, level: int, msg: bytes):
        if level >= 2:  # Warning or above
            self._log_lines.append(f"[{['V','I','W','E'][min(level,3)]}] {msg.decode('utf-8', errors='replace')}")

    def _get_last_error(self, runner) -> str:
        buf = ctypes.create_string_buffer(1024)
        self._lib.BP_GetLastError(runner, buf, 1024)
        return buf.value.decode("utf-8", errors="replace")

    def execute(self, bjson_content: str,
                variables: Optional[dict] = None,
                dispatch_beginplay: bool = True,
                file_path: Optional[str] = None) -> dict:
        """
        Load and run a blueprint.
        - file_path: if provided, use BP_LoadFromFile (auto-loads dependencies like CommonLib).
        - bjson_content: fallback JSON string, used when file_path is None.
        Returns {"output": [...], "warnings": [...], "error": str|None}
        """
        self._print_lines.clear()
        self._log_lines.clear()

        runner = self._lib.BP_CreateRunner()
        if not runner:
            return {"output": [], "warnings": [], "error": "BP_CreateRunner returned NULL"}

        try:
            # Register callbacks (use pre-created callbacks stored on self to prevent GC)
            self._lib.BP_SetPrintCallback(runner, self._print_cb)
            self._lib.BP_SetLogCallback(runner,   self._log_cb)

            # Load — prefer BP_LoadFromFile when a path is given (auto-resolves dependencies)
            if file_path:
                rc = self._lib.BP_LoadFromFile(runner, file_path.encode("utf-8"))
                if rc != 0:
                    err = self._get_last_error(runner)
                    return {"output": [], "warnings": self._log_lines[:],
                            "error": f"Load failed: {err}"}
            else:
                rc = self._lib.BP_LoadFromJsonWithBaseDir(
                    runner,
                    bjson_content.encode("utf-8"),
                    str(BLUEPRINT_ROOT).encode("utf-8")
                )
                if rc != 0:
                    err = self._get_last_error(runner)
                    return {"output": [], "warnings": self._log_lines[:],
                            "error": f"Load failed: {err}"}

            # Inject variables
            if variables:
                for k, v in variables.items():
                    kb = k.encode("utf-8")
                    if isinstance(v, bool):
                        self._lib.BP_SetVariableInt(runner, kb, ctypes.c_int64(int(v)))
                    elif isinstance(v, int):
                        self._lib.BP_SetVariableInt(runner, kb, ctypes.c_int64(v))
                    elif isinstance(v, float):
                        self._lib.BP_SetVariableFloat(runner, kb, ctypes.c_double(v))
                    else:
                        self._lib.BP_SetVariableString(runner, kb, str(v).encode("utf-8"))

            # Execute data-flow nodes
            rc = self._lib.BP_Execute(runner)
            if rc != 0:
                err = self._get_last_error(runner)
                return {"output": self._print_lines[:], "warnings": self._log_lines[:],
                        "error": f"Execute failed: {err}"}

            # Dispatch BeginPlay event chain
            if dispatch_beginplay:
                rc = self._lib.BP_DispatchEvent(runner, b"OnBeginPlay")
                if rc != 0:
                    err = self._get_last_error(runner)
                    return {"output": self._print_lines[:], "warnings": self._log_lines[:],
                            "error": f"DispatchEvent(OnBeginPlay) failed: {err}"}

            # ── Tick 循环：推进异步 timer 和 FireEvent/HTTP 回调 ──────────────
            # BP_HasPendingWork = timers > 0 OR pending async > 0（统一判断）
            import time as _time
            max_time_sec  = 30.0
            tick_rate_sec = 0.016
            elapsed = 0.0
            while self._lib.BP_HasPendingWork(runner) > 0:
                if elapsed >= max_time_sec:
                    self._log_lines.append(
                        f"[W] Tick loop timed out after {max_time_sec}s"
                    )
                    break
                # 先 Drain（消费 FireEvent callback），再 Tick（推进 timer）
                self._lib.BP_DrainQueue()
                self._lib.BP_Tick(runner, ctypes.c_float(tick_rate_sec))
                _time.sleep(tick_rate_sec)
                elapsed += tick_rate_sec

            return {"output": self._print_lines[:],
                    "warnings": self._log_lines[:],
                    "error": None}
        finally:
            self._lib.BP_DestroyRunner(runner)

    def get_variable_after_execute(self, bjson_content: str,
                                   var_name: str,
                                   variables: Optional[dict] = None,
                                   file_path: Optional[str] = None) -> dict:
        """Execute blueprint and return the value of a specific variable."""
        self._print_lines.clear()
        self._log_lines.clear()

        runner = self._lib.BP_CreateRunner()
        if not runner:
            return {"value": None, "output": [], "error": "BP_CreateRunner returned NULL"}

        try:
            self._lib.BP_SetPrintCallback(runner, self._print_cb)
            self._lib.BP_SetLogCallback(runner,   self._log_cb)

            if file_path:
                rc = self._lib.BP_LoadFromFile(runner, file_path.encode("utf-8"))
            else:
                rc = self._lib.BP_LoadFromJsonWithBaseDir(
                    runner,
                    bjson_content.encode("utf-8"),
                    str(BLUEPRINT_ROOT).encode("utf-8")
                )
            if rc != 0:
                return {"value": None, "output": [],
                        "error": f"Load failed: {self._get_last_error(runner)}"}

            if variables:
                for k, v in variables.items():
                    kb = k.encode("utf-8")
                    if isinstance(v, (int, bool)):
                        self._lib.BP_SetVariableInt(runner, kb, ctypes.c_int64(int(v)))
                    elif isinstance(v, float):
                        self._lib.BP_SetVariableFloat(runner, kb, ctypes.c_double(v))
                    else:
                        self._lib.BP_SetVariableString(runner, kb, str(v).encode("utf-8"))

            self._lib.BP_Execute(runner)
            self._lib.BP_DispatchEvent(runner, b"OnBeginPlay")

            # Tick 循环：推进异步 timer 和 FireEvent/HTTP 回调
            import time as _time
            max_time_sec = 30.0; tick_rate_sec = 0.016; elapsed = 0.0
            while elapsed < max_time_sec and self._lib.BP_HasPendingWork(runner) > 0:
                self._lib.BP_DrainQueue()
                self._lib.BP_Tick(runner, ctypes.c_float(tick_rate_sec))
                _time.sleep(tick_rate_sec)
                elapsed += tick_rate_sec

            buf = ctypes.create_string_buffer(4096)
            n = self._lib.BP_GetVariableString(runner, var_name.encode("utf-8"), buf, 4096)
            if n >= 0:
                value = buf.value.decode("utf-8", errors="replace")
            else:
                # Try int/float
                vi = self._lib.BP_GetVariableInt(runner, var_name.encode("utf-8"))
                vf = self._lib.BP_GetVariableFloat(runner, var_name.encode("utf-8"))
                value = vi if vi != 0 else vf

            return {"value": value,
                    "output": self._print_lines[:],
                    "error": None}
        finally:
            self._lib.BP_DestroyRunner(runner)


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
    """Recursively find all .bjson files, return relative paths + metadata."""
    results = []
    skip = {"build", "build-windows", "build-static", ".git", "external", "logs"}
    for p in root.rglob("*.bjson"):
        # Skip build directories
        if any(part in skip for part in p.parts):
            continue
        rel = p.relative_to(root)
        results.append({
            "path": str(rel).replace("\\", "/"),
            "name": p.stem,
            "size_bytes": p.stat().st_size,
        })
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
            lib = _get_lib()
            result = lib.execute(content, variables, dispatch_beginplay=True)
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
                    "definitionId": "Node type identifier (see common_node_types)",
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
                "link_structure": {
                    "id": "Unique integer ID",
                    "startPinId": "Output pin ID",
                    "endPinId": "Input pin ID",
                    "isEnabled": "true"
                },
                "data_types": {
                    "0": "Unknown/Any",
                    "1": "Boolean",
                    "2": "Integer",
                    "3": "Float",
                    "4": "String",
                    "6": "Array",
                    "9": "Any"
                },
                "common_node_types": {
                    "OnBeginPlay": "Event source - entry point, exec output pin only",
                    "PrintString": "Print to console. Pins: 'In String'(in,4), exec in/out",
                    "FormatString": "Format string. Pins: 'Format'(in,4), named args(in,4), 'Result'(out,4)",
                    "ForLoop": "Loop. Pins: exec-in, 'First Index'(in,2), 'Last Index'(in,2); exec-out 'LoopBody'+'Completed', 'Index'(out,2)",
                    "Branch": "If/else. Pins: exec-in, 'Condition'(in,1); exec-out 'True'+'False'",
                    "SetVariable": "Set variable. Pins: exec-in, 'Name'(in,4), 'Value'(in,any); exec-out",
                    "GetVariable": "Get variable. Pins: 'Name'(in,4), 'Value'(out,any)",
                    "AppendString": "Concatenate. Pins: 'A'(in,4), 'B'(in,4), 'Result'(out,4)",
                    "LLM.Chat": "Call LLM API. Pins: BaseURL,ApiKey,Model,Messages,SystemPrompt,MaxTokens,Temperature,Tools(in); onReply,onToolCall,onError(exec-out); Reply,ToolCallsJSON,FullResponse,ErrorMessage(out,4)",
                    "JSON.MakeMessage": "Build chat message. Pins: Role,Content,ToolCallId(in,4); Message(out,4)",
                    "JSON.ArrayPush": "Push to JSON array. Pins: exec-in, JSON(in,4), Element(in,any); exec-out, JSON(out,4), Length(out,2)",
                    "JSON.Extract": "Extract field. Pins: JSON,Path(in,4); Value(out,4)",
                    "String.Template": "Template substitution. Pins: Template(in,4),Keys(in,6),Values(in,6); Result(out,4)"
                },
                "tips": [
                    "Always use unique IDs for nodes and pins across the whole file.",
                    "Exec pins connect the execution flow; data pins connect values.",
                    "OnBeginPlay is the standard entry point for Actor blueprints.",
                    "Use PrintString to output results - they appear in 'output' after execution.",
                    "Link startPinId must be an Output pin, endPinId must be an Input pin."
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


if __name__ == "__main__":
    import asyncio
    asyncio.run(main())
