"""Test execute_blueprint with Main.bjson (depends on CommonLib.bjson)."""
import sys, os, pathlib, json, ctypes, time

os.environ['BLUEPRINT_DLL']  = str(pathlib.Path(__file__).parent.parent / 'build/bin/Release/BlueprintRuntime.dll')
os.environ['BLUEPRINT_ROOT'] = str(pathlib.Path(__file__).parent.parent)

sys.path.insert(0, str(pathlib.Path(__file__).parent))
from mcp_server import _get_lib, BLUEPRINT_ROOT

# 直接用底层 ctypes 测试 timer count
lib_raw = ctypes.CDLL(str(BLUEPRINT_ROOT / 'build/bin/Release/BlueprintRuntime.dll'))
lib_raw.BP_CreateRunner.restype = ctypes.c_void_p
lib_raw.BP_DestroyRunner.restype = None; lib_raw.BP_DestroyRunner.argtypes = [ctypes.c_void_p]
lib_raw.BP_LoadFromFile.restype = ctypes.c_int; lib_raw.BP_LoadFromFile.argtypes = [ctypes.c_void_p, ctypes.c_char_p]
lib_raw.BP_Execute.restype = ctypes.c_int; lib_raw.BP_Execute.argtypes = [ctypes.c_void_p]
lib_raw.BP_DispatchEvent.restype = ctypes.c_int; lib_raw.BP_DispatchEvent.argtypes = [ctypes.c_void_p, ctypes.c_char_p]
lib_raw.BP_Tick.restype = None; lib_raw.BP_Tick.argtypes = [ctypes.c_void_p, ctypes.c_float]
lib_raw.BP_GetActiveTimerCount.restype = ctypes.c_int; lib_raw.BP_GetActiveTimerCount.argtypes = [ctypes.c_void_p]
LogCb = ctypes.CFUNCTYPE(None, ctypes.c_int, ctypes.c_char_p)
lib_raw.BP_SetPrintCallback.restype = None; lib_raw.BP_SetPrintCallback.argtypes = [ctypes.c_void_p, LogCb]

prints = []
def on_print(lv, msg): 
    s = msg.decode('utf-8', errors='replace')
    prints.append(s)
    print(f"  [PRINT] {s}")

pcb = LogCb(on_print)
r = lib_raw.BP_CreateRunner()
lib_raw.BP_SetPrintCallback(r, pcb)
lib_raw.BP_LoadFromFile(r, str(BLUEPRINT_ROOT / 'tests/assets/Main.bjson').encode())
lib_raw.BP_Execute(r)
lib_raw.BP_DispatchEvent(r, b"OnBeginPlay")

print(f"\nAfter DispatchEvent: active timers = {lib_raw.BP_GetActiveTimerCount(r)}")

max_time = 5.0; tick = 0.016; elapsed = 0.0
while lib_raw.BP_GetActiveTimerCount(r) > 0 and elapsed < max_time:
    time.sleep(tick)
    lib_raw.BP_Tick(r, ctypes.c_float(tick))
    elapsed += tick
    tc = lib_raw.BP_GetActiveTimerCount(r)
    if tc > 0 and int(elapsed * 10) % 10 == 0:
        print(f"  tick {elapsed:.2f}s, timers={tc}")

print(f"\nAfter Tick loop ({elapsed:.2f}s): active timers = {lib_raw.BP_GetActiveTimerCount(r)}")
lib_raw.BP_DestroyRunner(r)
print("\n--- all prints ---")
for p in prints: print(" ", p)

