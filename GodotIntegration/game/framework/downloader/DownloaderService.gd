extends Node
class_name DownloaderService

enum DownloaderTaskState { WAITING, DOWNLOADING, FINISHED, FAILED}

# 任务数据结构
class DownloaderTask:
	var id: int
	var url: String
	var save_path: String
	var priority: int # 优先级，数字越小优先级越高
	var downloader: Downloader
	var state: DownloaderTaskState
	
	func _init(p_id: int, p_url: String, p_path: String, p_priority: int = 0):
		id = p_id
		url = p_url
		save_path = p_path
		priority = p_priority
		state = DownloaderTaskState.WAITING

# 配置参数
var max_concurrent_downloads: int = 3 # 最大并发数

# 内部状态
var _task_queue: Array[DownloaderTask] = [] # 等待队列
var _active_tasks: Dictionary[int, DownloaderTask] = {} # 正在下载的任务 {id: DownloaderTask}
var _task_id_counter: int = 0
var _is_processing: bool = false

# 信号定义
signal task_added(task_id: int, url: String)
signal task_started(task_id: int, url: String)
signal task_progress(task_id: int, current_bytes: int, total_bytes: int)
signal task_finished(task_id: int, success: bool, error_code: Downloader.DownloadResult, error_msg: String)
signal queue_updated(active_count: int, waiting_count: int)

func _ready():
	set_process(false)

# 1. 对外接口：添加下载任务
func add_download(url: String, save_path: String, priority: int = 0) -> int:
	var taskFound := _find_task_by_url(url)
	if taskFound:
		push_warning("[DownloadManager]: 任务 URL %s, TaskId %d 已存在" % [url, taskFound.id])
		return taskFound.id

	_task_id_counter += 1
	var task := DownloaderTask.new(_task_id_counter, url, save_path, priority)
	
	_task_queue.append(task)
	_sort_queue() # 按优先级排序
	
	emit_signal("task_added", task.id, url)
	_schedule_next() # 尝试调度
	
	return task.id

# 2. 对外接口：取消任务
func cancel_task(task_id: int):
	if _active_tasks.has(task_id):
		var task = _active_tasks[task_id]
		task.downloader.cancel()
		_active_tasks.erase(task_id)
		_notify_queue_updated()
	elif _find_and_remove_from_queue(task_id):
		pass # 从等待队列中移除成功
	else:
		push_warning("[DownloadManager]: 找不到任务 ID %d" % task_id)

# 3. 对外接口：获取队列统计
func get_queue_stats() -> Dictionary:
	return {
		"active": _active_tasks.size(),
		"waiting": _task_queue.size(),
		"max_concurrent": max_concurrent_downloads
	}

# 4. 核心调度逻辑
func _schedule_next():
	# 如果当前活跃任务数小于最大并发数，且队列不为空
	while _active_tasks.size() < max_concurrent_downloads and _task_queue.size() > 0:
		var task = _task_queue.pop_front()
		_start_task(task)
		
	_notify_queue_updated()

# 5. 启动单个任务
func _start_task(task: DownloaderTask):
	task.state = DownloaderTaskState.DOWNLOADING
	_active_tasks[task.id] = task
	
	# 动态创建下载器实例
	var downloader = Downloader.new()
	add_child(downloader)
	task.downloader = downloader
	
	# 绑定下载器的信号到管理器
	downloader.download_progress.connect(_on_task_progress.bind(task.id))
	downloader.download_finished.connect(_on_task_finished.bind(task.id))
	
	downloader.download(task.url, task.save_path)
	emit_signal("task_started", task.id, task.url)

# 6. 任务进度回调
func _on_task_progress(current: int, total: int, speed: String, task_id: int):
	emit_signal("task_progress", task_id, current, total)

# 7. 任务完成回调
func _on_task_finished(result: Downloader.DownloadResult, reason: String, task_id: int):
	if _active_tasks.has(task_id):
		var task = _active_tasks[task_id]
		task.state = DownloaderTaskState.FINISHED if result == Downloader.DownloadResult.OK else DownloaderTaskState.FAILED
		
		# 清理下载器节点
		if task.downloader:
			task.downloader.queue_free()
			
		_active_tasks.erase(task_id)
		emit_signal("task_finished", task_id, result == Downloader.DownloadResult.OK, result, reason)
		
		# 调度下一个任务
		_schedule_next()

# 8. 辅助方法：按优先级排序队列
func _sort_queue():
	_task_queue.sort_custom(func(a, b): return a.priority < b.priority)

# 9. 辅助方法：从等待队列中移除任务
func _find_and_remove_from_queue(task_id: int) -> bool:
	for i in range(_task_queue.size()):
		if _task_queue[i].id == task_id:
			_task_queue.remove_at(i)
			_notify_queue_updated()
			return true
	return false

# 10. 辅助方法：根据 URL 查找任务
func _find_task_by_url(url: String) -> DownloaderTask:
	for id in _active_tasks:
		var task = _active_tasks[id]
		if task.url == url:
			return task

	for task in _task_queue:
		if task.url == url:
			return task
	return null

# 11. 更新队列状态信号
func _notify_queue_updated():
	emit_signal("queue_updated", _active_tasks.size(), _task_queue.size())
