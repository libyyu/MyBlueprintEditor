@echo off
setlocal enabledelayedexpansion
:: ==============================================================================
:: build-all.bat – 一键构建所有平台产物并收集到 Unity 插件目录（Windows 宿主）
::
:: 用法:
::   build-all.bat [options]
::
:: 选项:
::   debug              构建 Debug（默认 Release）
::   clean              每个平台构建前清理 build 目录
::   --ndk <path>       Android NDK 路径（android 平台必填）
::   --api <N>          Android 最低 API Level（默认 21）
::   --emsdk <path>     Emscripten SDK 根目录（默认 %EMSDK% 环境变量）
::   --unity <path>     Unity Plugins 目标目录（默认 Unity\Runtime\Plugins）
::   --skip <platform>  跳过指定平台（可多次使用）
::   --only <platform>  只构建指定平台（可多次使用）
::   /?  /help
::
:: 构建的平台（Windows 宿主）:
::   windows  Windows x64 MSVC – 完整编辑器 + BlueprintRuntime.dll（默认启用）
::   wasm     WebAssembly 静态库（需 Emscripten）
::   android  Android ARM64-v8a .so（需 Android NDK）
::
:: 产物收集到 Unity\Runtime\Plugins\:
::   Windows\x86_64\
::     BlueprintRuntime.dll          ← 运行时主体
::     liblua54.dll                  ← Lua VM（动态链接时）
::   Android\arm64-v8a\
::     libBlueprintRuntime.so
::     liblua54.so
::   WebGL\
::     libBlueprintBundle.a          ← Runtime + crude_json + lua54 全合并包
::
:: 注: iOS/macOS 需要 macOS 宿主，请使用 build-all.sh。
::     iOS/WebGL 使用 BlueprintBundle（Runtime+JSON+Lua 全合并静态库）。
::
:: 示例:
::   build-all.bat
::   build-all.bat debug
::   build-all.bat --skip android
::   build-all.bat --only windows --only wasm
::   build-all.bat --ndk C:\android-ndk-r25c --emsdk C:\emsdk
:: ==============================================================================

set PROJECT_DIR=%~dp0
if "%PROJECT_DIR:~-1%"=="\" set PROJECT_DIR=%PROJECT_DIR:~0,-1%

set BUILD_BAT=%PROJECT_DIR%\build.bat
set BUILD_TYPE=Release
set CLEAN_FLAG=
set NDK_PATH=
set ANDROID_API=21
set EMSDK_PATH=%EMSDK%
set UNITY_PLUGINS_DIR=%PROJECT_DIR%\Unity\Runtime\Plugins

set SKIP_LIST=
set ONLY_LIST=

set STATUS_windows=NA
set STATUS_wasm=NA
set STATUS_android=NA
set OVERALL_OK=1

:: ── 参数解析 ──────────────────────────────────────────────────────────────────
:parse_args
if "%~1"=="" goto :banner

if /i "%~1"=="debug"    (set BUILD_TYPE=Debug&   shift& goto :parse_args)
if /i "%~1"=="release"  (set BUILD_TYPE=Release& shift& goto :parse_args)
if /i "%~1"=="clean"    (set CLEAN_FLAG=clean&   shift& goto :parse_args)
if /i "%~1"=="--ndk"    (set NDK_PATH=%~2&       shift& shift& goto :parse_args)
if /i "%~1"=="--api"    (set ANDROID_API=%~2&    shift& shift& goto :parse_args)
if /i "%~1"=="--emsdk"  (set EMSDK_PATH=%~2&     shift& shift& goto :parse_args)
if /i "%~1"=="--unity"  (set UNITY_PLUGINS_DIR=%~2& shift& shift& goto :parse_args)
if /i "%~1"=="--skip"   (set SKIP_LIST=%SKIP_LIST% %~2& shift& shift& goto :parse_args)
if /i "%~1"=="--only"   (set ONLY_LIST=%ONLY_LIST% %~2& shift& shift& goto :parse_args)
if /i "%~1"=="/?" goto :show_help
if /i "%~1"=="--help" goto :show_help
if /i "%~1"=="/help"  goto :show_help
echo [ERROR] Unknown option: %~1
goto :show_help

:show_help
echo.
echo Usage: build-all.bat [options]
echo.
echo Options:
echo   debug / release        Build configuration (default: Release)
echo   clean                  Wipe build dirs before building
echo   --ndk ^<path^>           Android NDK root
echo   --api ^<N^>              Android min API level (default: 21)
echo   --emsdk ^<path^>         Emscripten SDK root
echo   --unity ^<path^>         Unity Plugins output directory
echo   --skip ^<platform^>      Skip a platform (windows/wasm/android)
echo   --only ^<platform^>      Build only this platform (repeatable)
exit /b 1

:: ── Banner ────────────────────────────────────────────────────────────────────
:banner
echo.
echo ============================================
echo  Blueprint Editor - Build All Platforms
echo  Host OS       : Windows
echo  Configuration : %BUILD_TYPE%
echo  Unity Plugins : %UNITY_PLUGINS_DIR%
if not "%SKIP_LIST%"=="" echo  Skip          : %SKIP_LIST%
if not "%ONLY_LIST%"=="" echo  Only          : %ONLY_LIST%
echo ============================================

:: ── 平台过滤 ──────────────────────────────────────────────────────────────────
goto :after_should_build
:should_build
    set _P=%~1
    if not "%ONLY_LIST%"=="" (
        echo %ONLY_LIST% | findstr /i /w "%_P%" >nul 2>&1
        exit /b %ERRORLEVEL%
    )
    if not "%SKIP_LIST%"=="" (
        echo %SKIP_LIST% | findstr /i /w "%_P%" >nul 2>&1
        if not errorlevel 1 exit /b 1
    )
    exit /b 0
:after_should_build

:: ── 构建辅助 ──────────────────────────────────────────────────────────────────
goto :after_run_build
:run_build
    set _PLAT=%~1
    set _BAT_PLAT=%~2
    call :should_build %_PLAT%
    if errorlevel 1 (
        set STATUS_%_PLAT%=SKIP
        echo [--] Skipping platform: %_PLAT%
        exit /b 0
    )
    echo.
    echo ==========================================
    echo   Building: %_PLAT% ^(%BUILD_TYPE%^)
    echo ==========================================
    set _EXTRA=%~3 %~4 %~5 %~6 %~7 %~8
    call "%BUILD_BAT%" %_BAT_PLAT% %BUILD_TYPE% %CLEAN_FLAG% %_EXTRA%
    if %ERRORLEVEL% neq 0 (
        echo [WARN] Platform %_PLAT% build FAILED, continuing...
        set STATUS_%_PLAT%=FAIL
        set OVERALL_OK=0
    ) else (
        set STATUS_%_PLAT%=OK
        echo [OK]   Platform %_PLAT% built successfully.
    )
    exit /b 0
:after_run_build

:: ── BlueprintBundle 构建辅助 ──────────────────────────────────────────────────
goto :after_build_bundle
:build_bundle
    :: %1=build目录  %2=cmake源码目录(PROJECT_DIR)  %3=BUILD_TYPE
    set _BD=%~1
    set _SD=%~2
    set _BT=%~3
    if not exist "%_BD%" exit /b 0
    echo   Configuring BlueprintBundle in %_BD% ...
    cmake -S "%_SD%" -B "%_BD%" -DBUILD_BUNDLE=ON -DBUNDLE_LUA=ON -DCMAKE_BUILD_TYPE=%_BT% >nul
    cmake --build "%_BD%" --target BlueprintBundle --config %_BT% --parallel
    if %ERRORLEVEL% neq 0 (
        echo [WARN] BlueprintBundle build failed in %_BD%
    ) else (
        echo [OK]   BlueprintBundle built in %_BD%
    )
    exit /b 0
:after_build_bundle

:: ── 收集辅助 ──────────────────────────────────────────────────────────────────
goto :after_collect
:collect
    set _CP=%~1
    set _SRC=%~2
    set _DST_DIR=%~3
    set _DST_FILE=%~4
    set _SV=STATUS_%_CP%
    if "!%_SV%!"=="OK" (
        if exist "%_SRC%" (
            if not exist "%_DST_DIR%" mkdir "%_DST_DIR%"
            copy /y "%_SRC%" "%_DST_DIR%\%_DST_FILE%" >nul
            echo [OK]   [%_CP%] -^> %_DST_DIR%\%_DST_FILE%
        ) else (
            echo [WARN] Artifact not found (optional): %_SRC%
        )
    )
    exit /b 0
:after_collect

goto :after_collect_req
:collect_req
    set _CP=%~1
    set _SRC=%~2
    set _DST_DIR=%~3
    set _DST_FILE=%~4
    set _SV=STATUS_%_CP%
    if "!%_SV%!"=="OK" (
        if exist "%_SRC%" (
            if not exist "%_DST_DIR%" mkdir "%_DST_DIR%"
            copy /y "%_SRC%" "%_DST_DIR%\%_DST_FILE%" >nul
            echo [OK]   [%_CP%] -^> %_DST_DIR%\%_DST_FILE%
        ) else (
            echo [WARN] Required artifact not found: %_SRC%
            set %_SV%=FAIL
            set OVERALL_OK=0
        )
    )
    exit /b 0
:after_collect_req

:: ── 1. Windows x64（完整编辑器 + 共享 DLL） ──────────────────────────────────
call :run_build windows windows
echo.

:: ── 2. WebAssembly ────────────────────────────────────────────────────────────
set WASM_ARGS=
if not "%EMSDK_PATH%"=="" set WASM_ARGS=--emsdk "%EMSDK_PATH%"

where emcmake >nul 2>&1
if not errorlevel 1 (
    call :run_build wasm wasm %WASM_ARGS%
) else if not "%EMSDK_PATH%"=="" (
    call :run_build wasm wasm %WASM_ARGS%
) else (
    set STATUS_wasm=NA
    echo [--] emcmake not found - skipping WASM. Pass --emsdk ^<path^> to enable.
)
:: 构建 BlueprintBundle（静态全合并包）
if "!STATUS_wasm!"=="OK" (
    echo.
    echo [2b] Building BlueprintBundle (wasm)...
    call :build_bundle "%PROJECT_DIR%\build-wasm" "%PROJECT_DIR%" %BUILD_TYPE%
)
echo.

:: ── 3. Android ────────────────────────────────────────────────────────────────
if "%NDK_PATH%"=="" (
    if not "%ANDROID_NDK%"==""      set NDK_PATH=%ANDROID_NDK%
    if not "%ANDROID_NDK_HOME%"=="" set NDK_PATH=%ANDROID_NDK_HOME%
)
if not "%NDK_PATH%"=="" (
    call :run_build android android --ndk "%NDK_PATH%" --api %ANDROID_API%
) else (
    set STATUS_android=NA
    echo [--] Android NDK not found - skipping. Pass --ndk ^<path^> or set %%ANDROID_NDK%%.
)
echo.

:: ── 收集产物 ──────────────────────────────────────────────────────────────────
echo.
echo Collecting artifacts to: %UNITY_PLUGINS_DIR%
echo.

:: Windows
set WIN_BIN=%PROJECT_DIR%\build-windows\bin\%BUILD_TYPE%
call :collect_req windows "%WIN_BIN%\BlueprintRuntime.dll" ^
    "%UNITY_PLUGINS_DIR%\Windows\x86_64" "BlueprintRuntime.dll"
call :collect windows "%WIN_BIN%\liblua54.dll" ^
    "%UNITY_PLUGINS_DIR%\Windows\x86_64" "liblua54.dll"

:: Android
set AND_BIN=%PROJECT_DIR%\build-android\bin\%BUILD_TYPE%
call :collect_req android "%AND_BIN%\libBlueprintRuntime.so" ^
    "%UNITY_PLUGINS_DIR%\Android\arm64-v8a" "libBlueprintRuntime.so"
call :collect android "%AND_BIN%\liblua54.so" ^
    "%UNITY_PLUGINS_DIR%\Android\arm64-v8a" "liblua54.so"

:: WebGL – 优先 BlueprintBundle，回退到 libBlueprintRuntime.a
set WASM_BIN=%PROJECT_DIR%\build-wasm\bin\%BUILD_TYPE%
if exist "%WASM_BIN%\libBlueprintBundle.a" (
    call :collect_req wasm "%WASM_BIN%\libBlueprintBundle.a" ^
        "%UNITY_PLUGINS_DIR%\WebGL" "libBlueprintBundle.a"
) else if exist "%PROJECT_DIR%\build-wasm\Runtime\libBlueprintBundle.a" (
    call :collect_req wasm "%PROJECT_DIR%\build-wasm\Runtime\libBlueprintBundle.a" ^
        "%UNITY_PLUGINS_DIR%\WebGL" "libBlueprintBundle.a"
) else (
    call :collect_req wasm "%PROJECT_DIR%\build-wasm\Runtime\libBlueprintRuntime.a" ^
        "%UNITY_PLUGINS_DIR%\WebGL" "libBlueprintRuntime.a"
)

:: ── 汇总报告 ──────────────────────────────────────────────────────────────────
echo.
echo ============================================
echo  Build All - Summary
echo ============================================
for %%P in (windows wasm android) do (
    set _S=!STATUS_%%P!
    if "!_S!"=="OK"   echo   [OK]   %%P
    if "!_S!"=="SKIP" echo   [--]   %%P (skipped)
    if "!_S!"=="NA"   echo   [--]   %%P (tools missing / not available)
    if "!_S!"=="FAIL" echo   [FAIL] %%P
)
echo.
echo  Collected artifacts:
echo    Windows\x86_64\  BlueprintRuntime.dll  liblua54.dll
echo    Android\arm64\   libBlueprintRuntime.so  liblua54.so
echo    WebGL\           libBlueprintBundle.a  (Runtime+JSON+Lua merged)
echo.
echo  iOS / macOS: run build-all.sh on a macOS host
echo  Unity Plugins: %UNITY_PLUGINS_DIR%
echo ============================================

if %OVERALL_OK%==1 (
    echo  All available platforms built successfully!
) else (
    echo  Some platforms FAILED. Check output above.
    exit /b 1
)

endlocal
