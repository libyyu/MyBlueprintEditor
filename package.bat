@echo off
chcp 65001 >nul
setlocal enabledelayedexpansion
:: ==============================================================================
:: package.bat — 一键构建所有平台 + 打包到 dist/ 目录
::
:: Usage:
::   package.bat                  交互模式（推荐）
::   package.bat full             全功能（Runtime + Lua + Protobuf）
::   package.bat nolua            无内置 Lua（搭配 xLua/tolua）
::   package.bat min              最小包（仅核心蓝图引擎）
::   package.bat --skip-build     跳过编译，直接打包（用已有产物）
::
:: 产出目录：
::   dist/
::   ├── UnityPlugins/           → 拷到 Unity Assets/Plugins/
::   │   ├── Windows/x86_64/
::   │   ├── Android/arm64-v8a/
::   │   └── WebGL/
::   ├── Editor/                 → 桌面编辑器（Windows only）
::   └── README.txt              → 使用说明
:: ==============================================================================

set PROJECT_DIR=%~dp0
if "%PROJECT_DIR:~-1%"=="\" set PROJECT_DIR=%PROJECT_DIR:~0,-1%

set DIST_DIR=%PROJECT_DIR%\dist
set SKIP_BUILD=0
set PRESET=

:: ── Parse args ──────────────────────────────────────────────────────────────
:parse
if "%~1"=="" goto :choose
if /i "%~1"=="full"          (set PRESET=full&   shift& goto :parse)
if /i "%~1"=="nolua"         (set PRESET=nolua&  shift& goto :parse)
if /i "%~1"=="min"           (set PRESET=min&    shift& goto :parse)
if /i "%~1"=="--skip-build"  (set SKIP_BUILD=1&  shift& goto :parse)
if /i "%~1"=="--help" goto :help
if /i "%~1"=="/?" goto :help
echo [ERROR] Unknown option: %~1
goto :help

:help
echo.
echo Usage: package.bat [preset] [options]
echo.
echo Presets:
echo   full    All features (Lua VM + Protobuf nodes)
echo   nolua   No built-in Lua (for xLua/tolua integration)
echo   min     Minimal (core blueprint engine only, no Lua, no Protobuf)
echo.
echo Options:
echo   --skip-build  Skip compilation, use existing build artifacts
echo.
exit /b 0

:: ── Interactive mode ────────────────────────────────────────────────────────
:choose
if not "%PRESET%"=="" goto :resolve

echo.
echo  ╔══════════════════════════════════════════════╗
echo  ║   MyBlueprintEditor — 一键打包工具           ║
echo  ╚══════════════════════════════════════════════╝
echo.
echo  选择你需要的配置：
echo.
echo    [1] 全功能（推荐）
echo        Runtime + 内置 Lua VM + Protobuf 节点
echo        适合：独立使用蓝图系统，不搭配 xLua
echo.
echo    [2] 无内置 Lua
echo        Runtime + Protobuf 节点，Lua VM 由外部提供
echo        适合：搭配 xLua / tolua / 自定义 Lua
echo.
echo    [3] 最小包
echo        仅核心蓝图引擎 + JSON，无 Lua 无 Protobuf
echo        适合：只用蓝图不用 Lua/Proto，追求最小包体
echo.
set /p CHOICE="  请输入 [1/2/3]（默认 1）: "
if "%CHOICE%"=="" set CHOICE=1
if "%CHOICE%"=="1" set PRESET=full
if "%CHOICE%"=="2" set PRESET=nolua
if "%CHOICE%"=="3" set PRESET=min

if "%PRESET%"=="" (
    echo [ERROR] Invalid choice: %CHOICE%
    goto :choose
)

:: ── Resolve preset to file names ────────────────────────────────────────────
:resolve
if /i "%PRESET%"=="full" (
    set DLL_NAME=BlueprintRuntime
    set SO_NAME=libBlueprintRuntime
    set BUNDLE_NAME=libBlueprintBundle
    set BUNDLE_LUA=ON
    set BUNDLE_PROTO=ON
    set PRESET_DESC=全功能（Lua + Protobuf）
)
if /i "%PRESET%"=="nolua" (
    set DLL_NAME=BlueprintRuntimeNoLua
    set SO_NAME=libBlueprintRuntimeNoLua
    set BUNDLE_NAME=libBlueprintBundleNoLua
    set BUNDLE_LUA=OFF
    set BUNDLE_PROTO=ON
    set PRESET_DESC=无内置 Lua（搭配 xLua）
)
if /i "%PRESET%"=="min" (
    set DLL_NAME=BlueprintRuntimeNoLua
    set SO_NAME=libBlueprintRuntimeNoLua
    set BUNDLE_NAME=libBlueprintBundleMin
    set BUNDLE_LUA=OFF
    set BUNDLE_PROTO=OFF
    set PRESET_DESC=最小包（仅核心引擎）
)

echo.
echo  配置: %PRESET_DESC%
echo  DLL:  %DLL_NAME%.dll
echo  SO:   %SO_NAME%.so
echo  WASM: %BUNDLE_NAME%.a
echo.

:: ── Step 1: Build ───────────────────────────────────────────────────────────
if %SKIP_BUILD%==1 (
    echo [1/3] 跳过编译（使用已有产物）
    goto :collect
)

echo [1/3] 编译所有平台...
echo.

:: 检查工具链
set HAS_EMSDK=0
set HAS_NDK=0

if not "%EMSDK%"=="" set HAS_EMSDK=1
where emcmake >nul 2>&1
if not errorlevel 1 set HAS_EMSDK=1

if not "%ANDROID_NDK%"=="" set HAS_NDK=1
if not "%ANDROID_NDK_HOME%"=="" set HAS_NDK=1

:: Windows（总是编）
echo  [Windows] 编译中...
call "%PROJECT_DIR%\build.bat" windows release >nul 2>&1
if %ERRORLEVEL% neq 0 (
    echo  [Windows] ❌ 编译失败！
    exit /b 1
)
echo  [Windows] ✅ 完成

:: Windows NoLua 变体（如果需要）
if /i not "%PRESET%"=="full" (
    echo  [Windows] 编译 NoLua 变体...
    cmake -S "%PROJECT_DIR%" -B "%PROJECT_DIR%\build-windows" -DBLUEPRINT_LUA=OFF >nul 2>&1
    cmake --build "%PROJECT_DIR%\build-windows" --target BlueprintRuntime --config Release --parallel >nul 2>&1
    :: 拷贝产物为 NoLua 名字
    if exist "%PROJECT_DIR%\build-windows\bin\Release\BlueprintRuntime.dll" (
        copy /y "%PROJECT_DIR%\build-windows\bin\Release\BlueprintRuntime.dll" "%PROJECT_DIR%\build-windows\bin\Release\BlueprintRuntimeNoLua.dll" >nul
    )
    :: 恢复 Lua=ON
    cmake -S "%PROJECT_DIR%" -B "%PROJECT_DIR%\build-windows" -DBLUEPRINT_LUA=ON >nul 2>&1
    cmake --build "%PROJECT_DIR%\build-windows" --target BlueprintRuntime --config Release --parallel >nul 2>&1
    echo  [Windows] ✅ NoLua 变体完成
)

:: WebGL
if %HAS_EMSDK%==1 (
    echo  [WebGL]   编译中...（需要约 2 分钟）
    call "%PROJECT_DIR%\build.bat" wasm release >nul 2>&1
    if %ERRORLEVEL% neq 0 (
        echo  [WebGL]   ❌ 编译失败
    ) else (
        echo  [WebGL]   ✅ Runtime 完成
        :: Build Bundle
        echo  [WebGL]   合并 Bundle...
        cmake -S "%PROJECT_DIR%" -B "%PROJECT_DIR%\build-wasm" -UBUILD_BUNDLE -UBUNDLE_LUA -UBUNDLE_PROTOBUF -UBUNDLE_NAME -DBUILD_BUNDLE=ON -DBUNDLE_LUA=%BUNDLE_LUA% -DBUNDLE_PROTOBUF=%BUNDLE_PROTO% -DCMAKE_BUILD_TYPE=Release >nul 2>&1
        cmake --build "%PROJECT_DIR%\build-wasm" --target BlueprintBundle --config Release >nul 2>&1
        echo  [WebGL]   ✅ Bundle 完成
    )
) else (
    echo  [WebGL]   ⏭ 跳过（未找到 Emscripten SDK，设置 %%EMSDK%% 或安装 emsdk）
)

:: Android
if %HAS_NDK%==1 (
    echo  [Android] 编译中...
    set _NDK=%ANDROID_NDK%
    if "!_NDK!"=="" set _NDK=%ANDROID_NDK_HOME%
    call "%PROJECT_DIR%\build.bat" android release --ndk "!_NDK!" >nul 2>&1
    if %ERRORLEVEL% neq 0 (
        echo  [Android] ❌ 编译失败
    ) else (
        echo  [Android] ✅ 完成
    )
) else (
    echo  [Android] ⏭ 跳过（未找到 Android NDK，设置 %%ANDROID_NDK%%）
)

:: ── Step 2: Collect ─────────────────────────────────────────────────────────
:collect
echo.
echo [2/3] 收集产物到 dist/...

if exist "%DIST_DIR%" rmdir /s /q "%DIST_DIR%"
mkdir "%DIST_DIR%\UnityPlugins\Windows\x86_64" 2>nul
mkdir "%DIST_DIR%\UnityPlugins\Android\arm64-v8a" 2>nul
mkdir "%DIST_DIR%\UnityPlugins\WebGL" 2>nul
mkdir "%DIST_DIR%\Editor" 2>nul

set COLLECTED=0

:: Windows DLL
set WIN_BIN=%PROJECT_DIR%\build-windows\bin\Release
if exist "%WIN_BIN%\%DLL_NAME%.dll" (
    copy /y "%WIN_BIN%\%DLL_NAME%.dll" "%DIST_DIR%\UnityPlugins\Windows\x86_64\BlueprintRuntime.dll" >nul
    echo   ✅ Windows: %DLL_NAME%.dll
    set /a COLLECTED+=1
) else (
    echo   ❌ Windows: %DLL_NAME%.dll 未找到
)
:: Lua DLL（full 模式需要）
if /i "%PRESET%"=="full" (
    if exist "%WIN_BIN%\liblua54.dll" (
        copy /y "%WIN_BIN%\liblua54.dll" "%DIST_DIR%\UnityPlugins\Windows\x86_64\liblua54.dll" >nul
        echo   ✅ Windows: liblua54.dll
    )
)

:: Editor exe
if exist "%WIN_BIN%\BlueprintEditor.exe" (
    copy /y "%WIN_BIN%\BlueprintEditor.exe" "%DIST_DIR%\Editor\BlueprintEditor.exe" >nul
    if exist "%WIN_BIN%\BlueprintRuntime.dll" copy /y "%WIN_BIN%\BlueprintRuntime.dll" "%DIST_DIR%\Editor\BlueprintRuntime.dll" >nul
    if exist "%WIN_BIN%\liblua54.dll" copy /y "%WIN_BIN%\liblua54.dll" "%DIST_DIR%\Editor\liblua54.dll" >nul
    echo   ✅ Editor: BlueprintEditor.exe
    set /a COLLECTED+=1
)

:: Android
set AND_BIN=%PROJECT_DIR%\build-android\bin\Release
if exist "%AND_BIN%\%SO_NAME%.so" (
    copy /y "%AND_BIN%\%SO_NAME%.so" "%DIST_DIR%\UnityPlugins\Android\arm64-v8a\libBlueprintRuntime.so" >nul
    echo   ✅ Android: %SO_NAME%.so
    set /a COLLECTED+=1
) else if exist "%AND_BIN%\libBlueprintRuntime.so" (
    copy /y "%AND_BIN%\libBlueprintRuntime.so" "%DIST_DIR%\UnityPlugins\Android\arm64-v8a\libBlueprintRuntime.so" >nul
    echo   ✅ Android: libBlueprintRuntime.so
    set /a COLLECTED+=1
) else (
    echo   ⏭ Android: 无产物（未编译或编译失败）
)

:: WebGL
set WASM_BIN=%PROJECT_DIR%\build-wasm\bin\Release
if exist "%WASM_BIN%\%BUNDLE_NAME%.a" (
    copy /y "%WASM_BIN%\%BUNDLE_NAME%.a" "%DIST_DIR%\UnityPlugins\WebGL\libBlueprintRuntime.a" >nul
    echo   ✅ WebGL: %BUNDLE_NAME%.a
    set /a COLLECTED+=1
) else (
    :: Fallback: 用单独的 Runtime .a
    if exist "%PROJECT_DIR%\build-wasm\Runtime\libBlueprintRuntime.a" (
        copy /y "%PROJECT_DIR%\build-wasm\Runtime\libBlueprintRuntime.a" "%DIST_DIR%\UnityPlugins\WebGL\libBlueprintRuntime.a" >nul
        echo   ⚠ WebGL: 使用未合并的 libBlueprintRuntime.a（可能缺少 Lua/Proto 符号）
        set /a COLLECTED+=1
    ) else (
        echo   ⏭ WebGL: 无产物（未编译或编译失败）
    )
)

:: ── Step 3: README ──────────────────────────────────────────────────────────
echo.
echo [3/3] 生成说明文件...

(
    echo ═══════════════════════════════════════════════
    echo  MyBlueprintEditor — 构建产物
    echo  配置: %PRESET_DESC%
    echo  日期: %DATE% %TIME%
    echo ═══════════════════════════════════════════════
    echo.
    echo 使用方法:
    echo.
    echo   1. 把 UnityPlugins/ 下的文件拷到你的 Unity 工程:
    echo      UnityPlugins/  →  Assets/Plugins/
    echo.
    echo   2. 把 C# 脚本拷到你的 Unity 工程:
    echo      Unity/Assets/Scripts/BlueprintRuntime.cs  →  Assets/Scripts/
    echo.
    echo   3. 编辑器（可选）:
    echo      Editor/BlueprintEditor.exe  双击即可运行
    echo.
    echo ─── 产物说明 ───
    echo.
    echo   Windows/x86_64/BlueprintRuntime.dll
    echo     Unity Editor + Windows Standalone 使用
    echo.
    echo   Android/arm64-v8a/libBlueprintRuntime.so
    echo     Unity Android 构建使用
    echo.
    echo   WebGL/libBlueprintRuntime.a
    echo     Unity WebGL 构建使用（静态库，含所有依赖）
    echo.
    echo ─── 包体大小参考 ───
    echo.
    echo   Windows DLL:  约 2-3 MB
    echo   Android .so:  约 2-3 MB
    echo   WebGL .wasm:  链接后约 300-500 KB（gzip 后约 150-250 KB）
    echo.
) > "%DIST_DIR%\README.txt"

:: ── Done ────────────────────────────────────────────────────────────────────
echo.
echo ╔══════════════════════════════════════════════╗
echo ║  打包完成！                                  ║
echo ║  配置: %PRESET_DESC%
echo ║  产物: %DIST_DIR%
echo ║  收集: %COLLECTED% 个平台产物
echo ╚══════════════════════════════════════════════╝
echo.
echo  dist/
echo  ├── UnityPlugins/           → 拷到 Unity Assets/Plugins/
echo  │   ├── Windows/x86_64/
echo  │   ├── Android/arm64-v8a/
echo  │   └── WebGL/
echo  ├── Editor/                 → 蓝图编辑器
echo  └── README.txt              → 使用说明
echo.

endlocal
