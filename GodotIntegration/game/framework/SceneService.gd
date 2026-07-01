extends Node
## SceneService —— 场景 / 资源加载能力（Godot 原生实现）
##
## 替代 Unity 侧 game/scene_manager.lua + SceneLoader(C#) + YooAsset 的角色。
## 全部走 Godot 引擎原生：SceneTree 换场景、add_child 叠加、ResourceLoader 预载。
## 注册为 Autoload 单例（名为 SceneService），全局可访问。
##
## 与 BlueprintRuntime 无耦合——它是纯宿主能力，供 GDScript / 蓝图节点调用。

## 场景加载完成信号（对应 Unity 的 on_scene_loaded 钩子）
signal scene_loaded(scene_path: String)
signal scene_unloaded(scene_path: String)

## 叠加加载的场景实例：path -> Node
var _additive: Dictionary = {}
var _current_main: String = ""

# ---------------------------------------------------------------------------
# 单场景切换（Single 模式，替换当前主场景）
# ---------------------------------------------------------------------------
func change_scene(scene_path: String) -> bool:
	if not ResourceLoader.exists(scene_path):
		push_error("[SceneService] scene not found: " + scene_path)
		return false
	var err := get_tree().change_scene_to_file(scene_path)
	if err != OK:
		push_error("[SceneService] change_scene failed(%d): %s" % [err, scene_path])
		return false
	_current_main = scene_path
	# change_scene 是延迟的，下一帧场景才就绪，这里发信号仅表示已请求
	call_deferred("_emit_loaded", scene_path)
	return true

func _emit_loaded(p: String) -> void:
	scene_loaded.emit(p)

func current_scene() -> String:
	return _current_main

# ---------------------------------------------------------------------------
# 叠加加载（Additive，用于 HUD / 弹窗 / UI 层，不替换主场景）
# ---------------------------------------------------------------------------
func load_additive(scene_path: String, parent: Node = null) -> Node:
	if _additive.has(scene_path):
		return _additive[scene_path]           # 已加载，返回现有实例
	var packed := load(scene_path) as PackedScene
	if packed == null:
		push_error("[SceneService] load_additive failed: " + scene_path)
		return null
	var inst := packed.instantiate()
	var host := parent if parent != null else get_tree().current_scene
	if host == null:
		host = get_tree().root
	host.add_child(inst)
	_additive[scene_path] = inst
	scene_loaded.emit(scene_path)
	return inst

func unload(scene_path: String) -> void:
	if _additive.has(scene_path):
		var inst: Node = _additive[scene_path]
		if is_instance_valid(inst):
			inst.queue_free()
		_additive.erase(scene_path)
		scene_unloaded.emit(scene_path)

# ---------------------------------------------------------------------------
# 资源加载（对应 YooAsset 的 AsyncLoad，用 Godot ResourceLoader）
# ---------------------------------------------------------------------------
func preload_resource(res_path: String) -> Resource:
	if not ResourceLoader.exists(res_path):
		push_error("[SceneService] resource not found: " + res_path)
		return null
	return ResourceLoader.load(res_path)

## 异步预载（后台线程），用 poll 查询进度。返回是否成功发起。
func request_load_async(res_path: String) -> bool:
	if not ResourceLoader.exists(res_path):
		return false
	return ResourceLoader.load_threaded_request(res_path) == OK

## 查询异步加载进度 [0,1]；完成后用 take_load_async 取资源。
func load_async_progress(res_path: String) -> float:
	var arr := []
	var status := ResourceLoader.load_threaded_get_status(res_path, arr)
	if status == ResourceLoader.THREAD_LOAD_LOADED:
		return 1.0
	if arr.size() > 0:
		return float(arr[0])
	return 0.0

func take_load_async(res_path: String) -> Resource:
	return ResourceLoader.load_threaded_get(res_path)

## 实例化一个场景资源为节点（不加入树，交调用方处置）
func instance_scene(scene_path: String) -> Node:
	var packed := load(scene_path) as PackedScene
	return packed.instantiate() if packed != null else null
