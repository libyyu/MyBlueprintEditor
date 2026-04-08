"""
mcp_test_server.py — 极简本地 MCP Server（用于测试 MCP.Call 节点）
----------------------------------------------------------------
实现标准 MCP JSON-RPC 2.0 协议子集：
  POST /   body={"jsonrpc":"2.0","id":N,"method":"tools/call","params":{"name":...,"arguments":{...}}}

内置工具：
  read_file(path)          — 读取本地文件内容
  write_file(path, content)— 写入本地文件
  list_dir(path)           — 列出目录内容
  web_fetch(url)           — HTTP GET 获取 URL 内容（需要 requests 库）
  echo(message)            — 回显消息（测试用）

启动：
  python tools/mcp_test_server.py [port]   默认端口 7788

然后用 MCP.Call 节点：
  ServerURL = http://localhost:7788
  ToolName  = read_file
  Arguments = {"path": "README.md"}
  Protocol  = jsonrpc2
"""

import sys
import os
import json
import pathlib
from http.server import BaseHTTPRequestHandler, HTTPServer

PORT = int(sys.argv[1]) if len(sys.argv) > 1 else 7788

# ─── 工具实现 ────────────────────────────────────────────────────────────────

def tool_echo(args: dict) -> str:
    return args.get("message", "")

def tool_read_file(args: dict) -> str:
    path = args.get("path", "")
    if not path:
        raise ValueError("path is required")
    p = pathlib.Path(path)
    if not p.exists():
        raise FileNotFoundError(f"File not found: {path}")
    return p.read_text(encoding="utf-8", errors="replace")

def tool_write_file(args: dict) -> str:
    path    = args.get("path", "")
    content = args.get("content", "")
    if not path:
        raise ValueError("path is required")
    p = pathlib.Path(path)
    p.parent.mkdir(parents=True, exist_ok=True)
    p.write_text(content, encoding="utf-8")
    return f"Written {len(content)} bytes to {path}"

def tool_list_dir(args: dict) -> str:
    path = args.get("path", ".")
    p = pathlib.Path(path)
    if not p.is_dir():
        raise NotADirectoryError(f"Not a directory: {path}")
    entries = sorted(
        [f"{e.name}{'/' if e.is_dir() else ''}" for e in p.iterdir()]
    )
    return "\n".join(entries)

def tool_web_fetch(args: dict) -> str:
    url = args.get("url", "")
    if not url:
        raise ValueError("url is required")
    try:
        import urllib.request
        with urllib.request.urlopen(url, timeout=15) as resp:
            return resp.read().decode("utf-8", errors="replace")[:8192]
    except Exception as e:
        raise RuntimeError(f"fetch failed: {e}")

TOOLS = {
    "echo":       tool_echo,
    "read_file":  tool_read_file,
    "write_file": tool_write_file,
    "list_dir":   tool_list_dir,
    "web_fetch":  tool_web_fetch,
}

# ─── JSON-RPC 2.0 响应构造 ────────────────────────────────────────────────────

def make_result(req_id, text: str) -> dict:
    return {
        "jsonrpc": "2.0",
        "id": req_id,
        "result": {
            "content": [{"type": "text", "text": text}],
            "isError": False,
        }
    }

def make_error(req_id, code: int, message: str) -> dict:
    return {
        "jsonrpc": "2.0",
        "id": req_id,
        "error": {"code": code, "message": message}
    }

# ─── HTTP Handler ─────────────────────────────────────────────────────────────

class MCPHandler(BaseHTTPRequestHandler):

    def log_message(self, fmt, *args):
        print(f"[MCP] {self.address_string()} {fmt % args}")

    def do_POST(self):
        length = int(self.headers.get("Content-Length", 0))
        body   = self.rfile.read(length).decode("utf-8")

        try:
            req = json.loads(body)
        except json.JSONDecodeError as e:
            self._respond(400, make_error(None, -32700, f"Parse error: {e}"))
            return

        req_id  = req.get("id")
        method  = req.get("method", "")
        params  = req.get("params", {})

        if method != "tools/call":
            self._respond(200, make_error(req_id, -32601,
                f"Method not found: {method}. Only 'tools/call' is supported."))
            return

        tool_name = params.get("name", "")
        arguments = params.get("arguments", {})

        if tool_name not in TOOLS:
            available = ", ".join(sorted(TOOLS.keys()))
            self._respond(200, make_error(req_id, -32602,
                f"Unknown tool: '{tool_name}'. Available: {available}"))
            return

        try:
            result_text = TOOLS[tool_name](arguments)
            self._respond(200, make_result(req_id, str(result_text)))
        except Exception as e:
            self._respond(200, make_error(req_id, -32603, str(e)))

    def do_GET(self):
        """列出可用工具（方便调试）"""
        tools_info = {
            name: {"description": fn.__doc__ or ""}
            for name, fn in TOOLS.items()
        }
        body = json.dumps({
            "server": "Blueprint MCP Test Server",
            "protocol": "JSON-RPC 2.0",
            "endpoint": f"POST http://localhost:{PORT}/",
            "tools": list(TOOLS.keys()),
        }, ensure_ascii=False, indent=2).encode()
        self.send_response(200)
        self.send_header("Content-Type", "application/json")
        self.send_header("Content-Length", len(body))
        self.end_headers()
        self.wfile.write(body)

    def _respond(self, status: int, obj: dict):
        body = json.dumps(obj, ensure_ascii=False).encode()
        self.send_response(status)
        self.send_header("Content-Type", "application/json")
        self.send_header("Content-Length", len(body))
        self.end_headers()
        self.wfile.write(body)


if __name__ == "__main__":
    os.chdir(pathlib.Path(__file__).parent.parent)  # cd 到项目根目录
    server = HTTPServer(("localhost", PORT), MCPHandler)
    print(f"[MCP Test Server] Listening on http://localhost:{PORT}/")
    print(f"[MCP Test Server] Working dir: {os.getcwd()}")
    print(f"[MCP Test Server] Available tools: {', '.join(sorted(TOOLS.keys()))}")
    print(f"[MCP Test Server] Test with:")
    print(f"  python tools/mcp_test_server.py")
    print(f"  # Then run: runtime-example examples/MCPCallDemo.bjson")
    print(f"  # Or via MCP: execute_blueprint {{\"path\":\"examples/MCPCallDemo.bjson\",")
    print(f"  #   \"variables\":{{\"ServerURL\":\"http://localhost:{PORT}\",")
    print(f"  #   \"ToolName\":\"read_file\",\"Arguments\":\"{{\\\"path\\\":\\\"README.md\\\"}}\"}}}}")
    print()
    try:
        server.serve_forever()
    except KeyboardInterrupt:
        print("\n[MCP Test Server] Stopped.")
