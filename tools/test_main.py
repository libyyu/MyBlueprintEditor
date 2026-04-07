"""Test execute_blueprint with Main.bjson (depends on CommonLib.bjson)."""
import sys, os, pathlib, json

os.environ['BLUEPRINT_DLL']  = str(pathlib.Path(__file__).parent.parent / 'build/bin/Release/BlueprintRuntime.dll')
os.environ['BLUEPRINT_ROOT'] = str(pathlib.Path(__file__).parent.parent)

sys.path.insert(0, str(pathlib.Path(__file__).parent))
from mcp_server import _get_lib, BLUEPRINT_ROOT

lib = _get_lib()
main_path = BLUEPRINT_ROOT / "tests" / "assets" / "Main.bjson"

print("=== execute_blueprint tests/assets/Main.bjson ===\n")
result = lib.execute("", dispatch_beginplay=True, file_path=str(main_path))

print("error:", result["error"])
print("\n--- output ---")
for line in result["output"]:
    print(" ", line)
print("\n--- warnings ---")
for w in result["warnings"]:
    print(" ", w)
