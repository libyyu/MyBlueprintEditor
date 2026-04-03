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
::   dll      Windows x64 – Runtime-only DLL（无编辑器）
::   wasm     WebAssembly 静态库（需 Emscripten）
::   android  Android ARM64-v8a .so（需 Android NDK）
::
:: 注意: iOS / macOS 只能在 macOS 宿主上构建，请使用 build-all.sh。
::
:: 产物收集到 Unity\Runtime\Plugins\:
::   Windows\x86_64\BlueprintRuntime.dll
::   Android\arm64-v8a\libBlueprintRuntime.so
::   WebGL\libBlueprintRuntime.a
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

:: Skip/Only 列表（用分隔符拼接）
set SKIP_LIST=
set ONLY_LIST=

:: 构建状态 (每个平台一个变量 STATUS_<platform>)
set STATUS_windows=NA
set STATUS_wasm=NA
set STATUS_android=NA
set OVERALL_OK=1

:: ── 参数解析 ──────────────────────────────────────────────────────────────────
:parse_args
if "%~1"=="" goto :banner

if /i "%~1"=="debug"    (set BUILD_TYPE=Debug& shift& goto :parse_args)
if /i "%~1"=="release"  (set BUILD_TYPE=Release& shift& goto :parse_args)
if /i "%~1"=="clean"    (set CLEAN_FLAG=clean& shift& goto :parse_args)

if /i "%~1"=="--ndk"    (set NDK_PATH=%~2& shift& shift& goto :parse_args)
if /i "%~1"=="--api"    (set ANDROID_API=%~2& shift& shift& goto :parse_args)
if /i "%~1"=="--emsdk"  (set EMSDK_PATH=%~2& shift& shift& goto :parse_args)
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
echo   debug              Build Debug (default: Release)
echo   clean              Wipe build dirs before building
echo   --ndk ^<path^>       Android NDK root
echo   --api ^<N^>          Android min API level (default: 21)
echo   --emsdk ^<path^>     Emscripten SDK root
echo   --unity ^<path^>     Unity Plugins output directory
echo   --skip ^<platform^>  Skip a platform (windows/wasm/android)
echo   --only ^<platform^>  Build only this platform (repeatable)
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

:: ── 平台过滤辅助（通过返回 errorlevel 判断）─────────────────────────────────
:: should_build <platform>: 若应构建则 errorlevel=0，否则 errorlevel=1
goto :after_should_build
:should_build
    set _P=%~1
    :: --only 白名单优先
    if not "%ONLY_LIST%"=="" (
        echo %ONLY_LIST% | findstr /i /w "%_P%" >nul 2>&1
        exit /b %ERRORLEVEL%
    )
    :: --skip 黑名单
    if not "%SKIP_LIST%"=="" (
        echo %SKIP_LIST% | findstr /i /w "%_P%" >nul 2>&1
        if not errorlevel 1 exit /b 1
    )
    exit /b 0
:after_should_build

:: ── 构建辅助函数 ──────────────────────────────────────────────────────────────
goto :after_run_build
:run_build
    :: %1=平台标识  %2=build.bat平台参数  剩余=额外参数
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
    set _EXTRA_ARGS=%~3 %~4 %~5 %~6 %~7 %~8 %~9
    set _CMD=call "%BUILD_BAT%" %_BAT_PLAT% %BUILD_TYPE% %CLEAN_FLAG% %_EXTRA_ARGS%
    %_CMD%
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

:: ── 收集辅助函数 ──────────────────────────────────────────────────────────────
goto :after_collect
:collect
    :: %1=平台  %2=源文件  %3=目标目录  %4=目标文件名
    set _CP=%~1
    set _SRC=%~2
    set _DST_DIR=%~3
    set _DST_FILE=%~4
    set _STATUS_VAR=STATUS_%_CP%
    if "!%_STATUS_VAR%!"=="OK" (
        if exist "%_SRC%" (
            if not exist "%_DST_DIR%" mkdir "%_DST_DIR%"
            copy /y "%_SRC%" "%_DST_DIR%\%_DST_FILE%" >nul
            echo [OK]   Collected [%_CP%]: %_DST_DIR%\%_DST_FILE%
        ) else (
            echo [WARN] Expected artifact not found: %_SRC%
            set %_STATUS_VAR%=FAIL
            set OVERALL_OK=0
        )
    )
    exit /b 0
:after_collect

:: ── 1. Windows x64（完整编辑器 + DLL）────────────────────────────────────────
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
    echo [--] emcmake not found - skipping WASM build.
    echo      Install Emscripten or pass --emsdk ^<path^> to enable.
)
echo.

:: ── 3. Android ────────────────────────────────────────────────────────────────
set ANDROID_ARGS=
if "%NDK_PATH%"=="" (
    if not "%ANDROID_NDK%"==""      set NDK_PATH=%ANDROID_NDK%
    if not "%ANDROID_NDK_HOME%"=="" set NDK_PATH=%ANDROID_NDK_HOME%
)
if not "%NDK_PATH%"=="" (
    set ANDROID_ARGS=--ndk "%NDK_PATH%" --api %ANDROID_API%
    call :run_build android android %ANDROID_ARGS%
) else (
    set STATUS_android=NA
    echo [--] Android NDK not found - skipping Android build.
    echo      Pass --ndk ^<path^> or set %%ANDROID_NDK%% to enable.
)
echo.

:: ── 收集产物 ──────────────────────────────────────────────────────────────────
echo.
echo Collecting artifacts to: %UNITY_PLUGINS_DIR%

:: Windows DLL
call :collect windows ^
    "%PROJECT_DIR%\build-windows\bin\%BUILD_TYPE%\BlueprintRuntime.dll" ^
    "%UNITY_PLUGINS_DIR%\Windows\x86_64" ^
    "BlueprintRuntime.dll"

:: Android
call :collect android ^
    "%PROJECT_DIR%\build-android\Runtime\libBlueprintRuntime.so" ^
    "%UNITY_PLUGINS_DIR%\Android\arm64-v8a" ^
    "libBlueprintRuntime.so"

:: WebGL
call :collect wasm ^
    "%PROJECT_DIR%\build-wasm\Runtime\libBlueprintRuntime.a" ^
    "%UNITY_PLUGINS_DIR%\WebGL" ^
    "libBlueprintRuntime.a"

:: ── 汇总报告 ──────────────────────────────────────────────────────────────────
echo.
echo ============================================
echo  Build All - Summary
echo ============================================

for %%P in (windows wasm android) do (
    set _S=!STATUS_%%P!
    if "!_S!"=="OK"   echo   [OK]   %%P
    if "!_S!"=="SKIP" echo   [--]   %%P (skipped)
    if "!_S!"=="NA"   echo   [--]   %%P (not available / tools missing)
    if "!_S!"=="FAIL" echo   [FAIL] %%P
)

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
