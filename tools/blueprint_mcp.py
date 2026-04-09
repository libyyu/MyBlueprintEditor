"""
blueprint_mcp.py — BlueprintRuntime Python binding & MCP helper
---------------------------------------------------------------
Provides:
  BlueprintLib: ctypes wrapper around BlueprintRuntime DLL/so C API
  register_tool(): decorator to register Python function as Blueprint node
  set_userinput_mock(): inject mock input for UserInput.Wait

Usage:
  from blueprint_mcp import BlueprintLib, register_tool

  lib = BlueprintLib("/path/to/BlueprintRuntime.so")

  @register_tool(lib, runner_ptr,
      name="web_search",
      description="Search the web",
      params=[("query", "Search query")])
  def web_search(query: str) -> str:
      return "result"
"""

import ctypes
import time
from typing import Optional, Callable

BP_PIN_UNKNOWN = 0
BP_PIN_BOOLEAN = 1
BP_PIN_INTEGER = 2
BP_PIN_FLOAT   = 3
BP_PIN_STRING  = 4
BP_PIN_ARRAY   = 6
BP_PIN_ANY     = 9

class BP_PinDef(ctypes.Structure):
    _fields_ = [
        ("name",     ctypes.c_char_p),
        ("dataType", ctypes.c_int),
        ("isInput",  ctypes.c_int),
        ("isExec",   ctypes.c_int),
        ("tooltip",  ctypes.c_char_p),
    ]

BP_HandlerFn = ctypes.CFUNCTYPE(ctypes.c_int, ctypes.c_void_p, ctypes.c_void_p)
LogCbType    = ctypes.CFUNCTYPE(None, ctypes.c_int, ctypes.c_char_p)


class BlueprintLib:
    """Thin ctypes wrapper around BlueprintRuntime DLL/so C API."""

    def __init__(self, dll_path: str):
        self._lib = ctypes.CDLL(dll_path)
        self._setup_signatures()
        self._print_lines: list[str] = []
        self._log_lines:   list[str] = []
        self._registered_handlers: list = []  # GC protection

        try:
            self._lib.BP_InitDefaultHttpClient()
        except AttributeError:
            pass

        self._print_cb = LogCbType(self._on_print)
        self._log_cb   = LogCbType(self._on_log)

    def _setup_signatures(self):
        lib = self._lib

        def sig(fn, res, *args):
            f = getattr(lib, fn, None)
            if f is None:
                return
            f.restype  = res
            f.argtypes = list(args)

        sig("BP_InitDefaultHttpClient", None)
        sig("BP_CreateRunner",  ctypes.c_void_p)
        sig("BP_DestroyRunner", None, ctypes.c_void_p)
        sig("BP_LoadFromJson",  ctypes.c_int, ctypes.c_void_p, ctypes.c_char_p)
        sig("BP_LoadFromJsonWithBaseDir", ctypes.c_int,
            ctypes.c_void_p, ctypes.c_char_p, ctypes.c_char_p)
        sig("BP_LoadFromFile",  ctypes.c_int, ctypes.c_void_p, ctypes.c_char_p)
        sig("BP_Execute",       ctypes.c_int, ctypes.c_void_p)
        sig("BP_DispatchEvent", ctypes.c_int, ctypes.c_void_p, ctypes.c_char_p)
        sig("BP_SetVariableString", None,
            ctypes.c_void_p, ctypes.c_char_p, ctypes.c_char_p)
        sig("BP_SetVariableInt", None,
            ctypes.c_void_p, ctypes.c_char_p, ctypes.c_int64)
        sig("BP_SetVariableFloat", None,
            ctypes.c_void_p, ctypes.c_char_p, ctypes.c_double)
        sig("BP_GetVariableString", ctypes.c_int,
            ctypes.c_void_p, ctypes.c_char_p, ctypes.c_char_p, ctypes.c_int)
        sig("BP_GetVariableInt",   ctypes.c_int64,  ctypes.c_void_p, ctypes.c_char_p)
        sig("BP_GetVariableFloat", ctypes.c_double, ctypes.c_void_p, ctypes.c_char_p)
        sig("BP_GetLastError", ctypes.c_int,
            ctypes.c_void_p, ctypes.c_char_p, ctypes.c_int)
        sig("BP_IsLoaded",       ctypes.c_int,  ctypes.c_void_p)
        sig("BP_Tick",           None, ctypes.c_void_p, ctypes.c_float)
        sig("BP_HasPendingWork", ctypes.c_int, ctypes.c_void_p)
        sig("BP_DrainQueue",     None)
        sig("BP_SetPrintCallback", None, ctypes.c_void_p, LogCbType)
        sig("BP_SetLogCallback",   None, ctypes.c_void_p, LogCbType)
        sig("BP_RegisterNodeDef", ctypes.c_int,
            ctypes.c_void_p, ctypes.c_char_p, ctypes.c_char_p,
            ctypes.c_char_p, ctypes.c_char_p,
            ctypes.c_void_p, ctypes.c_int)
        sig("BP_RegisterHandler", None,
            ctypes.c_void_p, ctypes.c_char_p, BP_HandlerFn, ctypes.c_void_p)
        sig("BP_GetInputString",  ctypes.c_int,
            ctypes.c_void_p, ctypes.c_char_p, ctypes.c_char_p, ctypes.c_int)
        sig("BP_GetInputInt",    ctypes.c_int64,  ctypes.c_void_p, ctypes.c_char_p)
        sig("BP_GetInputFloat",  ctypes.c_double, ctypes.c_void_p, ctypes.c_char_p)
        sig("BP_SetOutputString", None,
            ctypes.c_void_p, ctypes.c_char_p, ctypes.c_char_p)
        sig("BP_SetOutputInt",   None, ctypes.c_void_p, ctypes.c_char_p, ctypes.c_int64)
        sig("BP_SetOutputFloat", None, ctypes.c_void_p, ctypes.c_char_p, ctypes.c_double)
        sig("BP_ActivateOutputFlow", ctypes.c_int, ctypes.c_void_p, ctypes.c_char_p)

    def _on_print(self, level: int, msg: bytes):
        self._print_lines.append(msg.decode("utf-8", errors="replace"))

    def _on_log(self, level: int, msg: bytes):
        if level >= 2:
            labels = ['V','I','W','E']
            self._log_lines.append(
                f"[{labels[min(level,3)]}] {msg.decode('utf-8', errors='replace')}")

    def _get_last_error(self, runner) -> str:
        buf = ctypes.create_string_buffer(1024)
        self._lib.BP_GetLastError(runner, buf, 1024)
        return buf.value.decode("utf-8", errors="replace")

    # ------------------------------------------------------------------
    # register_node_tool
    # ------------------------------------------------------------------
    def register_node_tool(self,
                           runner_ptr,
                           node_id: str,
                           node_name: str,
                           description: str,
                           params: list[tuple[str, str]],
                           handler_fn: Callable) -> None:
        """Register handler_fn as a Blueprint node with string params."""
        total_pins = 1 + len(params) + 1 + 1  # exec-in + params + exec-out + Result
        PinArray = BP_PinDef * total_pins
        pins = PinArray()

        pins[0].name = b""; pins[0].dataType = 0
        pins[0].isInput = 1; pins[0].isExec = 1; pins[0].tooltip = None

        for i, (pname, pdesc) in enumerate(params):
            pins[1+i].name     = pname.encode()
            pins[1+i].dataType = BP_PIN_STRING
            pins[1+i].isInput  = 1
            pins[1+i].isExec   = 0
            pins[1+i].tooltip  = pdesc.encode() if pdesc else None

        eo = 1 + len(params)
        pins[eo].name = b""; pins[eo].dataType = 0
        pins[eo].isInput = 0; pins[eo].isExec = 1; pins[eo].tooltip = None

        ro = eo + 1
        pins[ro].name     = b"Result"
        pins[ro].dataType = BP_PIN_STRING
        pins[ro].isInput  = 0; pins[ro].isExec = 0
        pins[ro].tooltip  = description.encode() if description else None

        self._lib.BP_RegisterNodeDef(
            runner_ptr,
            node_id.encode(), node_name.encode(),
            b"Custom/Python", None,
            pins, total_pins
        )

        lib = self._lib
        param_names = [p[0] for p in params]

        def _handler(ctx, userdata):
            try:
                kwargs = {}
                for pn in param_names:
                    buf = ctypes.create_string_buffer(8192)
                    lib.BP_GetInputString(ctx, pn.encode(), buf, 8192)
                    kwargs[pn] = buf.value.decode("utf-8", errors="replace")
                result = handler_fn(**kwargs)
                lib.BP_SetOutputString(
                    ctx, b"Result",
                    str(result).encode("utf-8") if result is not None else b"")
                lib.BP_ActivateOutputFlow(ctx, b"")
                return 1
            except Exception as e:
                lib.BP_SetOutputString(ctx, b"Result", str(e).encode("utf-8"))
                lib.BP_ActivateOutputFlow(ctx, b"")
                return 1

        cb = BP_HandlerFn(_handler)
        self._registered_handlers.append(cb)
        lib.BP_RegisterHandler(runner_ptr, node_id.encode(), cb, None)

    # ------------------------------------------------------------------
    # execute
    # ------------------------------------------------------------------
    def execute(self,
                bjson_content: str,
                variables: Optional[dict] = None,
                dispatch_beginplay: bool = True,
                file_path: Optional[str] = None,
                runner_setup_fn=None) -> dict:
        self._print_lines.clear()
        self._log_lines.clear()
        runner = self._lib.BP_CreateRunner()
        if not runner:
            return {"output": [], "warnings": [], "error": "BP_CreateRunner returned NULL"}
        try:
            self._lib.BP_SetPrintCallback(runner, self._print_cb)
            self._lib.BP_SetLogCallback(runner,   self._log_cb)
            if runner_setup_fn:
                runner_setup_fn(runner)
            if file_path:
                rc = self._lib.BP_LoadFromFile(runner, file_path.encode("utf-8"))
            else:
                rc = self._lib.BP_LoadFromJsonWithBaseDir(
                    runner, bjson_content.encode("utf-8"), b".")
            if rc != 0:
                return {"output": [], "warnings": self._log_lines[:],
                        "error": f"Load failed: {self._get_last_error(runner)}"}
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
            rc = self._lib.BP_Execute(runner)
            if rc != 0:
                return {"output": self._print_lines[:], "warnings": self._log_lines[:],
                        "error": f"Execute failed: {self._get_last_error(runner)}"}
            if dispatch_beginplay:
                self._lib.BP_DispatchEvent(runner, b"OnBeginPlay")
            max_sec = 30.0; tick = 0.016; elapsed = 0.0
            while self._lib.BP_HasPendingWork(runner) > 0:
                if elapsed >= max_sec:
                    self._log_lines.append(f"[W] Tick loop timed out after {max_sec}s")
                    break
                self._lib.BP_DrainQueue()
                self._lib.BP_Tick(runner, ctypes.c_float(tick))
                time.sleep(tick)
                elapsed += tick
            return {"output": self._print_lines[:],
                    "warnings": self._log_lines[:], "error": None}
        finally:
            self._lib.BP_DestroyRunner(runner)

    def get_variable_after_execute(self,
                                   bjson_content: str,
                                   var_name: str,
                                   variables: Optional[dict] = None,
                                   file_path: Optional[str] = None) -> dict:
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
                    runner, bjson_content.encode("utf-8"), b".")
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
            max_sec = 30.0; tick = 0.016; elapsed = 0.0
            while elapsed < max_sec and self._lib.BP_HasPendingWork(runner) > 0:
                self._lib.BP_DrainQueue()
                self._lib.BP_Tick(runner, ctypes.c_float(tick))
                time.sleep(tick)
                elapsed += tick
            # 先尝试读 String（最通用；int/float 也能转成字符串）
            buf = ctypes.create_string_buffer(4096)
            n = self._lib.BP_GetVariableString(
                runner, var_name.encode("utf-8"), buf, 4096)
            if n >= 0:
                raw = buf.value.decode("utf-8", errors="replace")
                # 尝试还原为原生 int / float 类型（避免返回 "42" 而非 42）
                try:
                    as_int = int(raw)
                    # 只有整数字符串才转 int（避免 "3.14" → 3）
                    if str(as_int) == raw:
                        value = as_int
                    else:
                        value = float(raw)
                except (ValueError, TypeError):
                    value = raw
            else:
                # 变量不存在或读取失败
                err_buf = ctypes.create_string_buffer(256)
                self._lib.BP_GetLastError(runner, err_buf, 256)
                return {"value": None, "output": self._print_lines[:],
                        "error": f"Variable '{var_name}' not found: "
                                 f"{err_buf.value.decode('utf-8', errors='replace')}"}
            return {"value": value, "output": self._print_lines[:], "error": None}
        finally:
            self._lib.BP_DestroyRunner(runner)


# ---------------------------------------------------------------------------
# Decorator helper
# ---------------------------------------------------------------------------
def register_tool(lib: "BlueprintLib",
                  runner_ptr,
                  *,
                  name: str,
                  description: str = "",
                  params: list[tuple[str, str]] = None):
    def decorator(fn: Callable):
        lib.register_node_tool(
            runner_ptr,
            node_id=name,
            node_name=name.replace("_", " ").title(),
            description=description,
            params=params or [],
            handler_fn=fn,
        )
        return fn
    return decorator


# ---------------------------------------------------------------------------
# UserInput.Wait mock helper
# ---------------------------------------------------------------------------
def set_userinput_mock(lib: "BlueprintLib", runner_ptr, text: str):
    """Set __userinput_mock so UserInput.Wait returns immediately."""
    lib._lib.BP_SetVariableString(
        runner_ptr,
        b"__userinput_mock",
        text.encode("utf-8")
    )
