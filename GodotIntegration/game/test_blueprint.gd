extends Node

# End-to-end verification with the correct architecture:
#   - GameLauncher (process-level) installs the file reader + Lua searcher ONCE.
#   - BlueprintNode(s) just load & run blueprints; loading N blueprints never
#     re-registers the searcher.

@onready var launcher: GameLauncher = $GameLauncher
@onready var bp: BlueprintNode = $BlueprintNode

const BLUEPRINT := "res://blueprints/blueprint_1_1.bjson"

func _ready() -> void:
	print("=== Blueprint GDExtension test ===")
	# GameLauncher.auto_setup = true runs setup() in its own _ready(); ensure it.
	if launcher and not launcher.is_ready():
		launcher.setup()

	if bp == null:
		push_error("BlueprintNode child not found")
		return

	if not bp.load_blueprint(BLUEPRINT):
		push_error("blueprint failed to load: " + bp.get_last_error())
		return
	bp.execute()

	var deps := bp.get_dependencies(BLUEPRINT)
	print("declared dependencies (", deps.size(), "): ", deps)
	print("is_loaded: ", bp.is_loaded())

	bp.set_var_int("Health", 100)
	print("Health var round-trip: ", bp.get_var_int("Health"))

	print("=== test running; BP_Tick is called every frame by BlueprintNode ===")


func _process(_delta: float) -> void:
	if Input.is_action_just_pressed("ui_accept") and bp and bp.is_loaded():
		print("dispatch on_win -> ok=", bp.dispatch_event("on_win"))
