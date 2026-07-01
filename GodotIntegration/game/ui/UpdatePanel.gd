extends Control
## 更新阶段 UI 面板：标题 + 进度条 + 状态文字。
## 对应 Unity 的 FPanelUpdateUI。由更新阶段（独立 VM 或 Boot.gd）驱动 set_progress。

@onready var bar: ProgressBar = $Center/VBox/Bar
@onready var status: Label = $Center/VBox/Status

func set_progress(v: float) -> void:
	if bar: bar.value = clampf(v, 0.0, 1.0) * 100.0

func set_status(text: String) -> void:
	if status: status.text = text
