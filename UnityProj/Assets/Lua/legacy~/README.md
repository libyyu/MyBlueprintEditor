# legacy~

Unity 通过目录名后缀 `~` 自动忽略（不导入、不打包、不生成 .meta）。
本目录用于归档**已确认无外部引用、但暂不直接删除**的历史 Lua 代码，便于日后参考或迁移。

## 当前归档清单

来自 P2 重构（接口对齐死代码清理）：

| 路径                                     | 行数 | 归档原因                                                                  |
| ---------------------------------------- | ---: | ------------------------------------------------------------------------- |
| `ui/FPanelStartUI.lua`                   |  ~70 | 旧启动面板，无任何文件 require，字符串引用 0                              |
| `ui/FPanelMainUI.lua`                    | ~218 | 仅被 `FPanelStartUI` require，随之归档                                    |
| `ui/FPanelConsoleUI.lua`                 |  ~95 | 旧调试控制台面板，孤儿                                                    |
| `ui/FFixedList.lua`                      | ~250 | 依赖的 `ui.FFixedListRootProxy` 文件**根本不存在**，require 时即报错     |
| `ui/FScrollList.lua`                     | ~260 | 仅被 `FPanelMainUI` require，随之归档                                     |
| `ui/widgets/ActivityItemView.lua`        |  ~44 | 孤儿 widget，无任何引用                                                   |
| `utility/FGUITools.lua`                  |  ~23 | 仅被 `FPanelStartUI` require，随之归档；功能已被 `IUIPanelBridge` 覆盖     |

合计 ~960 行。

## 验证方法

迁移前已通过以下检查确认全部为孤立死代码：

```bash
# Lua 端（含路径式与点式 require）
rg -t lua 'require\s*[("'\'']\s*(ui[./])?(FPanelStartUI|FPanelMainUI|FPanelConsoleUI|FFixedList|FScrollList)' UnityProj/Assets/Lua
rg -t lua 'require\s*[("'\'']\s*utility[./]FGUITools'                                                          UnityProj/Assets/Lua
rg -t lua 'require\s*[("'\'']\s*ui/widgets/ActivityItemView|widgets\.ActivityItemView'                         UnityProj/Assets/Lua

# C# 端
rg -t cs '(FPanelStartUI|FPanelMainUI|FPanelConsoleUI|FFixedList|FScrollList|FGUITools|ActivityItemView)' UnityProj/Assets/Scripts
```

迁移前 Lua 端只有这些文件之间的相互 require（FPanelStartUI→FPanelMainUI→FScrollList、FPanelStartUI→FGUITools），
没有任何活跃模块持有它们的引用；C# 端 0 命中。

## 复活步骤

如需恢复某个文件，移回原路径并让 Unity 重新生成 `.meta` 即可：

```bash
git mv UnityProj/Assets/Lua/legacy~/ui/FScrollList.lua UnityProj/Assets/Lua/ui/FScrollList.lua
# Unity 重新打开后会自动生成 .meta
```
