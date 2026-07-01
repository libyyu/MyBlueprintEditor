extends Control
## 主菜单面板：证明"更新完成→进入主逻辑"后 UI 框架的展示能力。
## 对应 Unity 的 FPanelMainMenu / FPanelUITestLauncher。

signal start_pressed

@onready var start_btn: Button = $Center/VBox/StartButton

func _ready() -> void:
	if start_btn:
		start_btn.pressed.connect(func(): start_pressed.emit())
