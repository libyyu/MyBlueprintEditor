# MyBlueprintEditor TODO

## ✅ 已完成

- [x] 打通 MCP Server → Claude Desktop
- [x] ReActAgent 接真实工具（wttr.in + DuckDuckGo）
- [x] Lua Agent 示例（LuaReActAgent.bjson）
- [x] Web.Search 示例 + FileAgent 示例
- [x] MCP.Call 节点（JSON-RPC 2.0）
- [x] Code.Run 超时强制终止（Windows/Linux）
- [x] 游戏节点（BT/FSM/Save/Vec2/Vec3/Physics/Stat/Cooldown）
- [x] GameExtra 节点（Random/Easing/GameTimer/Entity+Tag/Inventory）
- [x] Lua 统一到 Runtime（编辑器不再感知 lua_State）
- [x] Blueprint.GetVariable/SetVariable（复杂类型支持）
- [x] 多搜索后端（Tavily/Brave/SerpApi/Bing/DDG）
- [x] Lua print() 重定向到编辑器日志窗口
- [x] m_keepAliveRunners Tick 清理（内存泄漏修复）
- [x] Legacy Houdini/Tree 节点加 [Legacy] 警告
- [x] GameExtra 节点单元测试（24个用例）

---

## 🔴 高优先级

- [ ] **Variant union UB**：`Runtime/Types.h` union 切换 active member 存在 UB，
  建议迁移到 `std::variant<std::monostate, bool, int64_t, double>` 处理基本类型，
  string/array/map 仍单独存储。影响：高优化级别下可能出现非预期行为。

---

## 🟡 中优先级

- [ ] **LLM.AutoStream 测试缺失**：流式多 provider fallback 无单元测试，
  参考 `test_agent_demo.cpp` 的 `StreamChatTest` 模式补充。

- [ ] **Agent.Plan / Agent.Reflect / Context.Compress 无直接测试**：
  目前只通过 `ReActAgent` 端到端测试间接覆盖，建议各自加 MockHttpClient 测试。

- [ ] **LuaNodeRegistrar.ReloadFile 全量重载**：
  `(void)filePath; return ReloadAll();` 应优化为精确单文件重载，
  避免大规模 Lua 节点库的性能退化。

- [ ] **游戏节点示例蓝图**：`data/examples/` 全部是 AI 示例，
  缺少 Game/Random、Game/Easing、Game/Inventory 等的演示蓝图。

---

## 🟢 低优先级

- [ ] **docs/TODO.md 与根目录 TODO.md 合并**：内容有重叠，考虑统一维护。

- [ ] **C API 参考文档**：`BlueprintCAPI.h` 有大量接口，缺少独立的参数说明文档页。

- [ ] **CHANGELOG.md**：无版本变更记录，建议从当前节点开始维护。

- [ ] **游戏节点 README**：`data/game_extensions.lua` 的 Lua 游戏扩展节点
  （GameEvent/Dialogue/Tween）缺少文档说明使用方式。

- [ ] **docs/TODO.md P3 优化**：Timer 注册时深拷贝 `savedPinNameToId`，
  可改为只保存 `savedNode` 指针，回调时重建（见 docs/TODO.md 详细说明）。
