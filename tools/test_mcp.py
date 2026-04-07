"""Quick smoke test for mcp_server.py (run standalone, no MCP client needed)."""
import sys, os, pathlib, json

os.environ['BLUEPRINT_DLL']  = str(pathlib.Path(__file__).parent.parent / 'build/bin/Release/BlueprintRuntime.dll')
os.environ['BLUEPRINT_ROOT'] = str(pathlib.Path(__file__).parent.parent)

sys.path.insert(0, str(pathlib.Path(__file__).parent))
from mcp_server import _get_lib, _find_bjson_files, BLUEPRINT_ROOT

# ------ Test 1: list blueprints ------
print("=== Test 1: list_blueprints ===")
files = _find_bjson_files(BLUEPRINT_ROOT)
print(f"Found {len(files)} .bjson files:")
for f in files:
    print(f"  {f['path']}  ({f['size_bytes']} bytes)")

# ------ Test 2: execute a minimal blueprint ------
print("\n=== Test 2: execute simple PrintString blueprint ===")
simple_bp = json.dumps({
    "runtime": {
        "metadata": {"blueprintClass": 0, "name": "MCPTest", "schemaVersion": 2},
        "nodes": [
            {"id": 1, "definitionId": "OnBeginPlay", "name": "OnBeginPlay",
             "pins": [{"id": 2, "kind": 1, "dataType": 0, "isExec": True}]},
            {"id": 3, "definitionId": "PrintString", "name": "Print",
             "pins": [
                 {"id": 4, "kind": 0, "dataType": 0, "isExec": True},
                 {"id": 5, "kind": 0, "dataType": 4, "name": "In String",
                  "defaultValue": "Hello from MCP Server!"},
                 {"id": 6, "kind": 1, "dataType": 0, "isExec": True}
             ]}
        ],
        "links": [{"id": 100, "startPinId": 2, "endPinId": 4, "isEnabled": True}],
        "variables": []
    }
})

lib = _get_lib()
result = lib.execute(simple_bp)
print("Result:", json.dumps(result, ensure_ascii=False, indent=2))

# ------ Test 3: variable injection + retrieval ------
print("\n=== Test 3: variable injection + SetVariable/GetVariable ===")
var_bp = json.dumps({
    "runtime": {
        "metadata": {"blueprintClass": 0, "name": "VarTest", "schemaVersion": 2},
        "nodes": [
            {"id": 1, "definitionId": "OnBeginPlay", "name": "OnBeginPlay",
             "pins": [{"id": 2, "kind": 1, "dataType": 0, "isExec": True}]},
            {"id": 10, "definitionId": "GetVariable", "name": "Get UserQuery",
             "pins": [
                 {"id": 11, "kind": 0, "dataType": 4, "name": "Name", "defaultValue": "UserQuery"},
                 {"id": 12, "kind": 1, "dataType": 4, "name": "Value"}
             ]},
            {"id": 20, "definitionId": "PrintString", "name": "Print Query",
             "pins": [
                 {"id": 21, "kind": 0, "dataType": 0, "isExec": True},
                 {"id": 22, "kind": 0, "dataType": 4, "name": "In String", "defaultValue": ""},
                 {"id": 23, "kind": 1, "dataType": 0, "isExec": True}
             ]}
        ],
        "links": [
            {"id": 101, "startPinId": 2,  "endPinId": 21, "isEnabled": True},
            {"id": 102, "startPinId": 12, "endPinId": 22, "isEnabled": True},
        ],
        "variables": [
            {"name": "UserQuery", "dataType": 4, "defaultValue": "default query"}
        ]
    }
})

result2 = lib.execute(var_bp, variables={"UserQuery": "What is 2+2?"})
print("Result:", json.dumps(result2, ensure_ascii=False, indent=2))

# ------ Test 4: execute existing AgentDemo (just load, no API call expected) ------
print("\n=== Test 4: load AgentDemo.bjson ===")
agent_path = BLUEPRINT_ROOT / "examples" / "AgentDemo.bjson"
if agent_path.exists():
    content = agent_path.read_text(encoding="utf-8")
    # Note: will fail at LLM.Chat because no real API key - but should load OK
    result3 = lib.execute(content, dispatch_beginplay=False)  # skip BeginPlay to avoid LLM call
    print("Load+Execute(no BeginPlay):", json.dumps(result3, ensure_ascii=False, indent=2))
else:
    print("AgentDemo.bjson not found, skipping")

print("\n=== All tests done ===")
