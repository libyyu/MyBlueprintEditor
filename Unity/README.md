# Unity Integration

## 文件结构

```
Unity/
└── Runtime/
    └── Scripts/
        └── BlueprintRuntime.cs   ← 拷贝到你的 Unity 项目 Assets/ 下
```

## 平台支持

| 平台         | 库类型   | DLL 名称      | 说明                          |
|------------|---------|-------------|------------------------------|
| Windows    | 动态库   | BlueprintRuntime.dll | 放入 `Assets/Plugins/x86_64/` |
| macOS      | 动态库   | libBlueprintRuntime.dylib | 放入 `Assets/Plugins/` |
| Linux      | 动态库   | libBlueprintRuntime.so | 放入 `Assets/Plugins/` |
| Android    | 动态库   | libBlueprintRuntime.so | 放入 `Assets/Plugins/Android/` |
| iOS        | **静态库** | libBlueprintRuntime.a | 放入 `Assets/Plugins/iOS/`；DllImport 自动用 `__Internal` |
| WebGL      | **静态库** | libBlueprintRuntime.a | 放入 `Assets/Plugins/WebGL/`；DllImport 自动用 `__Internal` |

> iOS 和 WebGL 不支持动态库，必须使用静态库（.a）。  
> Emscripten 构建时所有 BP_* 符号通过 `EMSCRIPTEN_KEEPALIVE` 保留。

## 编译库

### 动态库（Windows / macOS / Linux / Android）

```bash
cmake -B build -DBUILD_RUNTIME_ONLY=ON -DBUILD_SHARED_LIBS=ON -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
# 产物：build/Runtime/libBlueprintRuntime.so (或 .dll / .dylib)
```

### 静态库（iOS）

```bash
# 交叉编译需要 iOS toolchain（例如 ios-cmake）
cmake -B build-ios \
  -DBUILD_RUNTIME_ONLY=ON \
  -DBUILD_SHARED_LIBS=OFF \
  -DCMAKE_TOOLCHAIN_FILE=path/to/ios.toolchain.cmake \
  -DPLATFORM=OS64
cmake --build build-ios --parallel
# 产物：build-ios/Runtime/libBlueprintRuntime.a
```

### 静态库（WebGL / Unity WebGL）

```bash
# 需要 Emscripten SDK（emcmake）
emcmake cmake -B build-wasm \
  -DBUILD_RUNTIME_ONLY=ON \
  -DCMAKE_BUILD_TYPE=Release
cmake --build build-wasm --parallel
# 产物：build-wasm/Runtime/libBlueprintRuntime.a
```

## 快速上手

### 方式一：BlueprintBehaviour 组件（最简单）

1. 把 `BlueprintRuntime.cs` 放到 `Assets/Scripts/`
2. 把对应平台的库文件放到 `Assets/Plugins/` 相应子目录
3. 新建 GameObject，挂上 `BlueprintBehaviour` 组件
4. 在 Inspector 把蓝图 JSON 文件拖到 **Blueprint Json** 字段
5. Play — 蓝图自动加载、执行，每帧 Tick

### 方式二：代码控制

```csharp
using BlueprintRuntime;

public class MyGame : MonoBehaviour
{
    BPRunner _runner;

    void Awake()
    {
        _runner = new BPRunner();
        _runner.OnLog += msg => Debug.Log("[BP] " + msg);

        // 从 StreamingAssets 加载（iOS / WebGL 友好）
        string path = System.IO.Path.Combine(
            Application.streamingAssetsPath, "combat.json");
        _runner.LoadFromFile(path);
    }

    void Start()
    {
        _runner.SetVariable("PlayerHP", 100L);
        _runner.SetVariable("EnemyCount", 3L);
        _runner.Execute();
    }

    void Update()
    {
        _runner.Tick(Time.deltaTime);

        // 读取蓝图输出变量
        long score = _runner.GetVariableInt("Score");
    }

    void OnDestroy() => _runner?.Dispose();
}
```

### 变量类型映射

| C# 方法                         | Blueprint 类型 |
|-------------------------------|--------------|
| `SetVariable(name, long)`     | Integer      |
| `SetVariable(name, double/float)` | Float    |
| `SetVariable(name, bool)`     | Boolean      |
| `SetVariable(name, string)`   | String       |
| `GetVariableInt(name)`        | → long       |
| `GetVariableFloat(name)`      | → double     |
| `GetVariableBool(name)`       | → bool       |
| `GetVariableString(name)`     | → string     |

## 注意事项

- `BPRunner` 实现了 `IDisposable`，请用 `using` 块或在 `OnDestroy` 中调用 `Dispose()`
- `OnLog` 回调在调用 `Execute()` / `Tick()` 的线程触发（Unity 主线程）
- iOS/WebGL 静态库必须在 Plugin Inspector 里正确设置目标平台，否则链接失败
- WebGL 下 `BP_LoadFromFile` 会通过 `emscripten_wget_data` 发起同步 XHR，
  文件需放在 `StreamingAssets/` 目录，Unity 构建时会自动打包
