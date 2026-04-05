In random order:
* Documentation: Make one

Done:
* ~~ImGui: Factor out changes to ImGui to use vanilla version.~~
* ~~Editor: Fix variable naming (mainly add `m_` prefix)~~
* ~~Editor: Split NodeEditorImpl.cpp to multiple files, file has grown too big.~~
* ~~Editor: Factor out use of `picojson.h`~~
* ~~Editor: Move use of `<optional>` to optional code extensions~~



#57 - join `ax::NodeEditor::EditorContext` with `struct EditorContext` and remove `reinterpret_cast<>`

---

# 性能优化 TODO（2026-04-05 分析）

> 分析背景：当前编辑器日常使用（节点 <50、循环 <100 次）无性能问题。
> 以下优化针对**批量处理 / 大循环 / 高频 FuncLib 调用**等压测场景。

---

## 优先级 1 🔴：`executedHere` 全量扫描 → O(N²) 风险

**位置**：`Runtime/BlueprintRunner.cpp` — `executeDownstreamFromPin()` 内部循环

**问题**：
ForLoop 每次迭代调用 `executeDownstreamFromPin`，执行完每个节点后检测 `m_flowExecutedNodes`
新增条目，并全量遍历合并到局部 `executedHere`：
```cpp
for (auto flowId : m_flowExecutedNodes)   // ← 全量遍历，随迭代线性增长
    executedHere.insert(flowId);
```
当 `Last Index = 10000`，且循环体有多个节点时，整体接近 O(N²)。

**为什么这样设计**：
`executedHere` 是为了解决真实 bug ——  ForLoop 循环体中 Branch 等控制流节点通过
`ActivateOutputFlow` 递归执行了下游节点，加入全局 `m_flowExecutedNodes`；下次迭代
不能用全局集合跳过（否则第二次迭代所有节点被误跳过），必须用局部 `executedHere`
记录"本次 executeDownstreamFromPin 调用内已执行过的节点"。设计正确，只是合并方式低效。

**优化方案**：引入 `flowInsertLog`（局部 vector）追踪本次调用期间 `m_flowExecutedNodes`
的新增顺序，用范围遍历替代全量合并：
```cpp
// executeDownstreamFromPin 开头
std::vector<NodeId> flowInsertLog;

// 替换原来的 m_flowExecutedNodes.insert(id)
if (m_flowExecutedNodes.insert(id).second)
    flowInsertLog.push_back(id);

// 替换 snapshotSize 对比逻辑
size_t prevLogSize = flowInsertLog.size();  // 执行节点前记录
executeNodeInternal(*node);
for (size_t k = prevLogSize; k < flowInsertLog.size(); ++k)  // 只遍历新增部分
    if (flowInsertLog[k] != id)
        executedHere.insert(flowInsertLog[k]);
```
复杂度从 O(N×总量) 降到 O(N×每次新增)，接近 O(N)。局部变量，无状态副作用，安全。

---

## 优先级 2 🟡：`BuildFuncSubGraph` BFS 每步线性扫描全部 links

**位置**：`Runtime/BlueprintRunner.cpp` — `BuildFuncSubGraph()` 静态函数

**问题**：
BFS 中每个节点的每个引脚都遍历 `libData.links` 全表（O(L)），整体 O(N×P×L)。
每次 `FuncLib.xxx` 节点执行都重新构建子图，无缓存，ForLoop 内调用则重复 N 次构建。

**为什么这样设计**：
`libData` 在运行时理论上可变（编辑器支持热更新），未做缓存避免失效问题；
子图作为值类型独立 Load 进 subRunner，每次新副本天然隔离。设计合理，只是可以加速。

**优化方案分两层**：

### 层 1（低风险）：BuildFuncSubGraph 内部建局部索引（函数内，单次有效）

```cpp
// BuildFuncSubGraph 开头一次性建立
std::unordered_map<PinId, std::vector<size_t>> startPinToLinks;
std::unordered_map<PinId, std::vector<size_t>> endPinToLinks;
for (size_t i = 0; i < libData.links.size(); ++i) {
    if (!libData.links[i].isEnabled) continue;
    startPinToLinks[libData.links[i].startPinId].push_back(i);
    endPinToLinks[libData.links[i].endPinId].push_back(i);
}
// BFS 中用索引替代 for(lk : libData.links) 全扫
```
复杂度 O(N×P×L) → O(N×P)，局部变量无副作用，完全安全。

### 层 2（中等风险）：Runner 级别缓存 `funcId → BlueprintData`

```cpp
// BlueprintRunner 新增成员
std::unordered_map<std::string, BlueprintData> m_funcSubGraphCache;

// invalidateTopoCache() 时同步清空
m_funcSubGraphCache.clear();
```
在 FuncLib 分支先查缓存，miss 时构建并存入。ForLoop 内重复调用同一函数只构建一次。
**注意**：`m_reachableDirty` / `m_topoCacheDirty` 触发时必须同步清空，否则热更新蓝图后
使用旧子图。和现有 dirty 机制对齐，可安全实施。

---

## 优先级 3 🟢：Timer 注册时深拷贝 `savedPinNameToId`

**位置**：`Runtime/BlueprintRunner.cpp` — `wrapCallbackWithContextRestore()` 和 `RunAsync()`

**问题**：
每次注册 Timer / RunAsync 都深拷贝整个 `pinNameToId` unordered_map（含堆内存分配）：
```cpp
auto savedPinNameToId = m_state->pinNameToId;  // ← unordered_map 深拷贝
```
低频 Timer 无感；大量并发异步节点时有累积开销。

**为什么这样设计**：
Timer 回调异步触发时 `m_state->pinNameToId` 已被覆盖，必须快照恢复。设计正确。

**优化方案**：只保存 `savedNode` 指针，回调触发时从节点 pins 重建（O(引脚数)，无堆分配）：
```cpp
// 回调触发时替代 map 拷贝恢复
state->currentNode = savedNode;
if (savedNode) {
    state->pinNameToId.clear();
    for (const auto& pin : savedNode->pins)
        state->pinNameToId[pin.name] = pin.id;
}
```
**前提**：`savedNode` 指向 `m_blueprint.nodes` 中的元素，runner 运行期间 nodes 不重新分配，
指针稳定。编辑器"运行中不修改蓝图"保证此条件，安全。

---

## 暂缓 ⏸：`prepareNodeContext` 中 3 个 map → vector

**位置**：`Runtime/BlueprintRunner.cpp` — `prepareNodeContext()`

**问题**：每次执行节点都 clear + rebuild 3 个 `unordered_map`（`pinNameToId` /
`inputPinNameToId` / `outputPinNameToId`），高频 ForLoop 有轻微累积。

**为什么不动**：
- 3 个 map 的拆分是为了解决**同名输入/输出引脚冲突 bug**，不能合并回 1 个。
- 用 `vector<pair<string,PinId>>` + 线性查找替代，改动面大（所有 `find()` 调用点）。
- 引脚数通常 2~8，实际收益极小，风险不低。**等其他优化稳定后再评估。**

---

## 暂缓 ⏸：StepNext 每步 BFS 数据依赖

**位置**：`Runtime/BlueprintRunner.cpp` — `StepNextNode()` 的 `executeOneNode` lambda

**问题**：每次 StepNext 都重新 BFS 收集数据上游依赖并遍历全 topo order。

**为什么不动**：
- 调试专用（手动点击），每步 <1ms，用户感知不到。
- 代码清晰优于微优化。
- 未来若有需要，可在 `m_stepPendingNodes` 填充时同步缓存数据依赖集合。

---

## 实施顺序建议

| 顺序 | 项目 | 改动范围 | 风险 | 预期收益 |
|------|------|----------|------|----------|
| 1 | executedHere 范围遍历 | 1个函数，局部变量 | 低 | 高（ForLoop大循环） |
| 2 | BuildFuncSubGraph 局部索引（层1） | 1个函数，局部变量 | 低 | 中 |
| 3 | FuncLib 子图结果缓存（层2） | 新增 map + 缓存失效 | 中 | 高（循环内FuncLib） |
| 4 | Timer savedNode 替代 map 拷贝 | 2处，逻辑简单 | 低 | 低 |
| ⏸ | prepareNodeContext map→vector | 全量替换 | 中 | 极低 |
| ⏸ | StepNext BFS 缓存 | — | — | 无需 |
