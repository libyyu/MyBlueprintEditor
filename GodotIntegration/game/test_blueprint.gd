extends Node

# Minimal verification that the BlueprintRuntime GDExtension works end to end.
# Mirrors the Python ctypes probe, but inside a real Godot game loop:
#   create runner (auto) -> load .bjson -> Execute -> Tick every frame.

@onready var bp: BlueprintNode = $BlueprintNode

func _ready() -> void:
	print("=== Blueprint GDExtension test ===")

	if bp == null:
		push_error("BlueprintNode child not found")
		return

	# BlueprintNode auto-loads its blueprint_path + auto-executes in its own _ready()
	# because we set autorun = true in the scene. Here we just report status and
	# demonstrate the GDScript-facing API.
	await get_tree().process_frame

	# Show the dependency graph declared by the blueprint (resolved via Godot VFS).
	var deps := bp.get_dependencies("res://blueprints/blueprint_1_1.bjson")
	print("declared dependencies (", deps.size(), "): ", deps)

	print("is_loaded: ", bp.is_loaded())
	if not bp.is_loaded():
		push_error("blueprint failed to load: " + bp.get_last_error())
		return

	# Demonstrate variable bridge (works whether or not the bp defines these).
	bp.set_var_int("Health", 100)
	print("Health var round-trip: ", bp.get_var_int("Health"))

	# Fire a named event into the blueprint (no-op if the bp has no such node).
	bp.dispatch_event("OnBeginPlay")

	print("=== test running; BP_Tick is called every frame by BlueprintNode ===")


func _process(_delta: float) -> void:
	# Press SPACE to manually dispatch a test event.
	if Input.is_action_just_pressed("ui_accept") and bp and bp.is_loaded():
		print("dispatch on_win -> ok=", bp.dispatch_event("on_win"))
