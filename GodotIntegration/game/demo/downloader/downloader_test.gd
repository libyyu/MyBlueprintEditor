extends Node

@onready var downloader: Downloader

func _ready() -> void:
	print("[DownloaderTest] ready")
	downloader = Downloader.new()
	add_child(downloader)
	downloader.download_finished.connect(_on_download_complete)
	downloader.download_progress.connect(_on_download_progress)
	
	downloader.download(
		"https://curl.haxx.se/download/curl-7.61.1.tar.gz",
		"user://test_downloader_curl-7.61.1.tar.gz",
	)
	
func _on_download_complete(result: Downloader.DownloadResult, error: String) -> void:
	print("[DownloaderTest] download finished:" + str(result))
	print("[DownloaderTest] download finished:" + error)
	downloader.queue_free()
	
func _on_download_progress(downloaded: int, total: int, speed: String) -> void:
	print("[DownloaderTest] progress:", "downloaded:", downloaded, " total:", total, " speed:", speed)


	
