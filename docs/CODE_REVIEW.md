# BlueprintRuntime 代码审查报告

> 审查范围：`Runtime/` 目录全部源文件  
> 日期：2026-03-25  

---

## 总览

| 优先级 | 问题数 |
|--------|--------|
| 🔴 高（影响正确性/稳定性） | 3 |
| 🟡 中（架构/可维护性） | 4 |
| 🟢 低（体验/性能） | 5 |

---

## 🔴 高优先级

### 1. `Variant` — union active member 切换存在未定义行为

**文件：** `Runtime/Types.h`

**问题：**  
`Variant` 使用 `union { bool boolValue; int64_t intValue; double floatValue; }`，但在赋值/移动时直接读写 union 成员，没有经过 placement new/destroy 流程。C++ 标准要求切换 union active member 时需要先 destroy 旧成员再 construct 新成员（尽管 trivial 类型无所谓，但若将来加入非 trivial 成员就会出问题）。更根本的问题是：目前的写法在某些编译器的高优化等级下会产生非预期行为。

**建议：**  
迁移到 `std::variant`（C++17），或在 C++14 下改用规范的 tagged union 写法（explicit placement new）。迁移到 C++17 的好处是整个 `asXxx()` 系列可以用 `std::visit` 简化，消除大量 switch：

```cpp
// C++17 方案
using VariantStorage = std::variant<
    std::monostate,   // Unknown
    bool,             // Boolean
    int64_t,          // Integer
    double,           // Float
    std::string,      // String / Object
    std::vector<Variant> // Array
>;
```

---

### 2. `DefaultNodeRegistry` — `unordered_map` rehash 导致指针失效

**文件：** `Runtime/NodeDefinition.h`

**问题：**  
```cpp
bool registerNode(const NodeDefinition& definition) override
{
    bool isNew = (m_nodeDefinitions.find(definition.id) == m_nodeDefinitions.end());
    m_nodeDefinitions[definition.id] = definition;   // ← 可能触发 rehash

    if (isNew)
        m_allDefsCache.push_back(&m_nodeDefinitions[definition.id]); // ← 指针存入缓存
    else
        rebuildAllDefsCache();  // ← rehash 后重建，但 isNew=true 时没有重建！
}
```

当 `isNew == true` 且 `unordered_map` 触发 rehash 时，之前缓存里的所有 `const NodeDefinition*` 指针全部失效，导致野指针。

**建议：**  
将 `if (isNew)` 的分支也改成 `rebuildAllDefsCache()`，保证每次写操作后缓存都重建：

```cpp
m_nodeDefinitions[definition.id] = definition;
rebuildAllDefsCache();   // 统一重建，简单安全
return true;
```

或者改用不会 rehash 的稳定容器（如 `std::list<NodeDefinition>` + `unordered_map<id, iterator>`）彻底消除问题。

---

### 3. `Variant::asInt()` / `asFloat()` — `std::stoll`/`std::stod` 在 `-fno-exceptions` 下编译失败

**文件：** `Runtime/Types.h`

**问题：**  
```cpp
int64_t asInt() const {
    case PinDataType::String:
        try { return std::stoll(stringValue); }
        catch (...) { return 0; }
}
```

Emscripten（Unity WebGL）构建时传入了 `-fno-exceptions`，`std::stoll`/`std::stod` 在异常被禁用时行为未定义，且 `try/catch` 语句本身在某些 `-fno-exceptions` 实现下会编译报错。

**建议：**  
用 C 函数替代：

```cpp
int64_t asInt() const {
    case PinDataType::String: {
        char* end = nullptr;
        int64_t v = std::strtoll(stringValue.c_str(), &end, 10);
        return (end != stringValue.c_str()) ? v : 0;
    }
}

double asFloat() const {
    case PinDataType::String: {
        char* end = nullptr;
        double v = std::strtod(stringValue.c_str(), &end);
        return (end != stringValue.c_str()) ? v : 0.0;
    }
}
```

---

## 🟡 中优先级

### 4. `ExecutionContext` 职责过重

**文件：** `Runtime/BlueprintRunner.h`

**问题：**  
`ExecutionContext` 同时承担了：引脚读写、变量读写、日志输出、Timer API（`Delay/SetTimer/SetTimerByName`）、控制流 API（`ActivateOutputFlow/MarkDownstreamAsHandled`）、节点触发（`FireConnectedNode`）。这使得 handler 的签名 `bool(ExecutionContext&)` 后面难以单独测试或替换其中某一部分。

**建议（渐进式重构）：**  
短期：将 Timer 相关方法抽成一个独立的 `TimerContext` 接口，`ExecutionContext` 持有它的引用；  
长期：`ExecutionContext` 只保留引脚/变量读写，`FlowContext` 负责控制流，两者作为参数分别传给 handler。

```cpp
// 目标签名
using NodeHandler = std::function<bool(DataContext& data, FlowContext& flow)>;
```

---

### 5. `m_parentTimerManager` 裸指针生命周期不安全

**文件：** `Runtime/BlueprintRunner.h`

**问题：**  
```cpp
FrameTimerManager* m_parentTimerManager = nullptr;
void SetParentTimerManager(FrameTimerManager* parent) { m_parentTimerManager = parent; }
```

子蓝图 Runner 持有父 Runner 的 `FrameTimerManager*` 裸指针。如果父 Runner 先被销毁，子 Runner 中还有 pending timer 时回调触发，会访问已释放的内存（UAF）。

**建议：**  
改为 `std::weak_ptr<FrameTimerManager>`，并在 `GetTimerManager()` 里 `lock()` 判空：

```cpp
std::weak_ptr<FrameTimerManager> m_parentTimerManager;

FrameTimerManager& GetTimerManager() {
    if (auto p = m_parentTimerManager.lock())
        return *p;
    return m_timerManager;
}
```

父 Runner 的 `m_timerManager` 需要包成 `shared_ptr` 或通过外部托管。

---

### 6. `m_keepAliveRunners` 永不清理，存在内存泄漏

**文件：** `Runtime/BlueprintRunner.h` / `BlueprintRunner.cpp`

**问题：**  
```cpp
std::vector<std::shared_ptr<BlueprintRunner>> m_keepAliveRunners;
void KeepAlive(std::shared_ptr<BlueprintRunner> subRunner) {
    m_keepAliveRunners.push_back(std::move(subRunner));
}
```

子蓝图 Runner push 进去后没有清理逻辑。随着子蓝图的执行，`m_keepAliveRunners` 会持续增长，所有 timer 完成后也不会自动释放。

**建议：**  
在 `Tick()` 里周期性扫描，将已完成（timer 全部 fired、`m_keepAliveRunners` 自身也无 pending timer）的子 runner 移除：

```cpp
void Tick(float deltaTime) {
    m_timerManager.Tick(deltaTime);
    // 清理已完成的子 runner
    m_keepAliveRunners.erase(
        std::remove_if(m_keepAliveRunners.begin(), m_keepAliveRunners.end(),
            [](const std::shared_ptr<BlueprintRunner>& r) {
                return r->GetTimerManager().IsEmpty();  // 需要 FrameTimerManager 暴露此方法
            }),
        m_keepAliveRunners.end()
    );
}
```

---

### 7. `BlueprintEditor/` 与 `Runtime/` 下各有一套 `BuiltinHandlers` / `BuiltinNodeDefs`

**文件：** `BlueprintEditor/BuiltinHandlers.cpp`、`Runtime/BuiltinHandlers.cpp`

**问题：**  
两个目录下同名文件的关系不明确——是 Runtime 版为纯逻辑、Editor 版附加了 UI？还是彼此独立？目前 CMake 里 Editor 目录的这两个文件参与了 `BlueprintEditor` 可执行程序的编译，但 Runtime 目录的也参与了 `BlueprintRuntime` 库的编译，如果内容有重叠，同一段逻辑会被编译两次。

**建议：**  
- 明确注释说明各自的分工，文件头加 `// Editor-only: depends on ImGui` 或 `// Runtime-only: no ImGui dependency`
- 如果 Editor 版只是 Runtime 版的超集，考虑 Editor 版直接 `#include` Runtime 版，消除重复

---

## 🟢 低优先级

### 8. `getOutputNodes` / `getInputNodes` 结果可能包含重复节点

**文件：** `Runtime/BlueprintData.h`

**问题：**  
一个节点有多个输出引脚时，同一下游节点会被多次 push 进 `result`。调用方如果直接用于拓扑排序，可能导致节点被重复执行。

**建议：**  
返回前去重，或改为返回 `std::unordered_set<NodeId>`：

```cpp
std::vector<NodeId> result;
std::unordered_set<NodeId> seen;
// ... push_back 前检查 seen.insert(id).second
```

---

### 9. `crude_json` 未独立成 CMake target

**文件：** `Runtime/CMakeLists.txt`

**问题：**  
```cmake
set(RUNTIME_SOURCES
    ...
    ${BLUEPRINT_ROOT_DIR}/Utils/Json/crude_json.cpp  # 直接列入 SOURCES
)
```

直接把第三方库的 `.cpp` 加进 Runtime 的 source list，导致：
1. 无法单独控制 crude_json 的编译选项（如关掉它的 warning）
2. 将来替换 JSON 库（nlohmann/json、simdjson）时改动大

**建议：**  
```cmake
add_library(crude_json STATIC Utils/Json/crude_json.cpp)
target_include_directories(crude_json PUBLIC Utils/Json)
target_compile_options(crude_json PRIVATE -w)  # 屏蔽第三方警告

target_link_libraries(BlueprintRuntime PRIVATE crude_json)
```

---

### 10. `BlueprintMetadata` / `NodeDefinition` 缺少 `schemaVersion` 字段

**文件：** `Runtime/BlueprintData.h`、`Runtime/NodeDefinition.h`

**问题：**  
蓝图 JSON 文件里没有版本号。一旦节点定义发生字段变更（如新增必填引脚），旧文件加载时不会报错，而是静默使用默认值，运行时才出现难以定位的 bug。

**建议：**  
```cpp
struct BlueprintMetadata {
    int         schemaVersion = 1;   // 新增
    std::string name;
    // ...
};

// 加载时检查
if (metadata.schemaVersion > CURRENT_SCHEMA_VERSION) {
    outError = "Blueprint schema version " + std::to_string(metadata.schemaVersion)
             + " is newer than supported version " + std::to_string(CURRENT_SCHEMA_VERSION);
    return false;
}
```

---

### 11. `BlueprintData` 索引无线程安全保护

**文件：** `Runtime/BlueprintData.h`

**问题：**  
`ensureIndices()` 是 `mutable` 懒更新，没有互斥锁保护。多线程场景下（如后台加载蓝图 + 主线程执行）会产生 data race。

**建议：**  
短期：在 `ensureIndices()` 加 `std::call_once` 或 `std::mutex`；  
长期：将 `BlueprintData` 设计为加载后不可变（immutable），运行时状态（引脚值、变量）全部移到 `ExecutionContext`，彻底消除并发修改的可能。

---

## 附：快速修复清单（按投入产出排序）

| # | 修复项 | 改动量 | 收益 |
|---|--------|--------|------|
| 1 | `stoll/stod` → `strtoll/strtod` | 10 行 | Emscripten 构建通过 |
| 2 | `registerNode` 统一走 `rebuildAllDefsCache()` | 3 行 | 消除野指针 |
| 3 | `getOutputNodes/getInputNodes` 去重 | 10 行 | 拓扑排序正确 |
| 4 | `crude_json` 独立 CMake target | 5 行 CMake | 第三方 warning 隔离 |
| 5 | `m_keepAliveRunners` Tick 清理 | 15 行 | 消除内存泄漏 |
| 6 | `schemaVersion` 字段 + 加载检查 | 20 行 | 版本兼容性 |
| 7 | `Variant` 迁移到 `std::variant` | 较大重构 | 彻底消除 UB |
| 8 | `m_parentTimerManager` 改 weak_ptr | 中等重构 | 消除 UAF |
