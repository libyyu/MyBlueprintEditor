@echo off
setlocal enabledelayedexpansion
:: ==============================================================================
:: build-all.bat - Build all platforms and collect artifacts to Unity Plugins dir
::
:: Usage:
::   build-all.bat [options]
::
:: Options:
::   debug              Build Debug (default: Release)
::   clean              Clean build dirs before each platform build
::   --ndk <path>       Android NDK root (required for android)
::   --api <N>          Android min API level (default: 21)
::   --emsdk <path>     Emscripten SDK root (default: %EMSDK% env var)
::   --unity <path>     Unity Plugins output dir (default: Unity\Runtime\Plugins)
::   --skip <platform>  Skip a platform (repeatable)
::   --only <platform>  Build only this platform (repeatable)
::   /?  /help
::
:: Platforms (Windows host):
::   windows  Windows x64 MSVC - full editor + BlueprintRuntime.dll
::   wasm     WebAssembly static lib (requires Emscripten)
::   android  Android ARM64-v8a .so (requires Android NDK)
::
:: Artifacts collected to Unity\Runtime\Plugins\:
::   Windows\x86_64\
::     BlueprintRuntime.dll
::     liblua54.dll
::   Android\arm64-v8a\
::     libBlueprintRuntime.so
::     liblua54.so
::   WebGL\
::     libBlueprintBundle.a  (Runtime + crude_json + lua54 + protobuf, all merged)
::
:: Note: iOS/macOS requires a macOS host - use build-all.sh instead.
::
:: Examples:
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

:: ── Argument parsing ──────────────────────────────────────────────────────────
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

:: ── Platform filter ───────────────────────────────────────────────────────────
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

:: ── Build helper ──────────────────────────────────────────────────────────────
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

:: ── BlueprintBundle helper ────────────────────────────────────────────────────
:: %1=build_dir  %2=src_dir  %3=BUILD_TYPE  %4=BUNDLE_LUA(ON|OFF)  %5=BUNDLE_PROTOBUF(ON|OFF)
goto :after_build_bundle
:build_bundle
    set _BD=%~1
    set _SD=%~2
    set _BT=%~3
    set _BL=%~4
    set _BP=%~5
    if "%_BP%"=="" set _BP=OFF
    if not exist "%_BD%" exit /b 0
    echo   Configuring BlueprintBundle (BUNDLE_LUA=%_BL%, BUNDLE_PROTOBUF=%_BP%) in %_BD% ...
    cmake -S "%_SD%" -B "%_BD%" -DBUILD_BUNDLE=ON -DBUNDLE_LUA=%_BL% -DBUNDLE_PROTOBUF=%_BP% -DCMAKE_BUILD_TYPE=%_BT% >nul
    cmake --build "%_BD%" --target BlueprintBundle --config %_BT% --parallel
    if %ERRORLEVEL% neq 0 (
        echo [WARN] BlueprintBundle ^(LUA=%_BL% PROTO=%_BP%^) failed in %_BD%
    ) else (
        echo [OK]   BlueprintBundle ^(LUA=%_BL% PROTO=%_BP%^) built in %_BD%
    )
    exit /b 0
:after_build_bundle

:: ── NoLua variant helper ──────────────────────────────────────────────────────
:: Re-configure with BLUEPRINT_LUA=OFF, build, then restore LUA=ON.
goto :after_build_nolua
:build_nolua
    set _BD=%~1
    set _SD=%~2
    set _BT=%~3
    if not exist "%_BD%" exit /b 0
    echo   Building BlueprintRuntimeNoLua in %_BD% ...
    cmake -S "%_SD%" -B "%_BD%" -DBLUEPRINT_LUA=OFF -DCMAKE_BUILD_TYPE=%_BT% >nul
    cmake --build "%_BD%" --target BlueprintRuntime --config %_BT% --parallel
    if %ERRORLEVEL% neq 0 (
        echo [WARN] BlueprintRuntimeNoLua failed in %_BD%
    ) else (
        echo [OK]   BlueprintRuntimeNoLua built in %_BD%
    )
    :: Restore Lua=ON
    cmake -S "%_SD%" -B "%_BD%" -DBLUEPRINT_LUA=ON -DCMAKE_BUILD_TYPE=%_BT% >nul
    cmake --build "%_BD%" --target BlueprintRuntime --config %_BT% --parallel
    if %ERRORLEVEL% neq 0 (
        echo [WARN] BlueprintRuntime ^(restore with Lua^) failed in %_BD%
    ) else (
        echo [OK]   BlueprintRuntime ^(with Lua^) restored in %_BD%
    )
    exit /b 0
:after_build_nolua

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

:: ── 1. Windows x64 (full editor + shared DLL) ────────────────────────────────
call :run_build windows windows
if "!STATUS_windows!"=="OK" (
    echo.
    echo [1b] Building BlueprintRuntimeNoLua (windows)...
    call :build_nolua "%PROJECT_DIR%\build-windows" "%PROJECT_DIR%" %BUILD_TYPE%
)
echo.

:: ── 2. WebAssembly ────────────────────────────────────────────────────────────
set WASM_ARGS=
if not "%EMSDK_PATH%"=="" set WASM_ARGS=--emsdk "%EMSDK_PATH%"

where emcmake >nul 2>&1
if not errorlevel 1 (
    call :run_build wasm wasm %WASM_ARGS%
) else (
    if not "%EMSDK_PATH%"=="" (
        call :run_build wasm wasm %WASM_ARGS%
    ) else (
        set STATUS_wasm=NA
        echo [--] emcmake not found - skipping WASM. Pass --emsdk ^<path^> to enable.
    )
)
:: Build 4 BlueprintBundle variants
if "!STATUS_wasm!"=="OK" (
    echo.
    echo [2b] Building BlueprintBundle +Lua +Proto (wasm^)...
    call :build_bundle "%PROJECT_DIR%\build-wasm" "%PROJECT_DIR%" %BUILD_TYPE% ON ON
    echo [2c] Building BlueprintBundleNoLua +Proto (wasm^)...
    call :build_bundle "%PROJECT_DIR%\build-wasm" "%PROJECT_DIR%" %BUILD_TYPE% OFF ON
    echo [2d] Building BlueprintBundleNoProto +Lua (wasm^)...
    call :build_bundle "%PROJECT_DIR%\build-wasm" "%PROJECT_DIR%" %BUILD_TYPE% ON OFF
    echo [2e] Building BlueprintBundleMin -Lua -Proto (wasm^)...
    call :build_bundle "%PROJECT_DIR%\build-wasm" "%PROJECT_DIR%" %BUILD_TYPE% OFF OFF
)
echo.

:: ── 3. Android ────────────────────────────────────────────────────────────────
if "%NDK_PATH%"=="" (
    if not "%ANDROID_NDK%"==""      set NDK_PATH=%ANDROID_NDK%
    if not "%ANDROID_NDK_HOME%"=="" set NDK_PATH=%ANDROID_NDK_HOME%
)
if not "%NDK_PATH%"=="" (
    call :run_build android android --ndk "%NDK_PATH%" --api %ANDROID_API%
    if "!STATUS_android!"=="OK" (
        echo.
        echo [3b] Building BlueprintRuntimeNoLua (android)...
        call :build_nolua "%PROJECT_DIR%\build-android" "%PROJECT_DIR%" %BUILD_TYPE%
    )
) else (
    set STATUS_android=NA
    echo [--] Android NDK not found - skipping. Pass --ndk ^<path^> or set %%ANDROID_NDK%%.
)
echo.

:: ── Collect artifacts ─────────────────────────────────────────────────────────
echo.
echo Collecting artifacts to: %UNITY_PLUGINS_DIR%
echo.

:: Windows
set WIN_BIN=%PROJECT_DIR%\build-windows\bin\%BUILD_TYPE%
call :collect_req windows "%WIN_BIN%\BlueprintRuntime.dll" ^
    "%UNITY_PLUGINS_DIR%\Windows\x86_64" "BlueprintRuntime.dll"
call :collect windows "%WIN_BIN%\liblua54.dll" ^
    "%UNITY_PLUGINS_DIR%\Windows\x86_64" "liblua54.dll"
call :collect windows "%WIN_BIN%\BlueprintRuntimeNoLua.dll" ^
    "%UNITY_PLUGINS_DIR%\Windows\x86_64" "BlueprintRuntimeNoLua.dll"

:: Android
set AND_BIN=%PROJECT_DIR%\build-android\bin\%BUILD_TYPE%
call :collect_req android "%AND_BIN%\libBlueprintRuntime.so" ^
    "%UNITY_PLUGINS_DIR%\Android\arm64-v8a" "libBlueprintRuntime.so"
call :collect android "%AND_BIN%\liblua54.so" ^
    "%UNITY_PLUGINS_DIR%\Android\arm64-v8a" "liblua54.so"
call :collect android "%AND_BIN%\libBlueprintRuntimeNoLua.so" ^
    "%UNITY_PLUGINS_DIR%\Android\arm64-v8a" "libBlueprintRuntimeNoLua.so"

:: WebGL - collect all Bundle variants
set WASM_BIN=%PROJECT_DIR%\build-wasm\bin\%BUILD_TYPE%
for %%B in (libBlueprintBundle.a libBlueprintBundleNoLua.a libBlueprintBundleNoProto.a libBlueprintBundleMin.a) do (
    if exist "%WASM_BIN%\%%B" (
        call :collect wasm "%WASM_BIN%\%%B" "%UNITY_PLUGINS_DIR%\WebGL" "%%B"
    ) else (
        if exist "%PROJECT_DIR%\build-wasm\Runtime\%%B" (
            call :collect wasm "%PROJECT_DIR%\build-wasm\Runtime\%%B" "%UNITY_PLUGINS_DIR%\WebGL" "%%B"
        )
    )
)
:: Fallback to bare Runtime if no Bundle found
if not exist "%UNITY_PLUGINS_DIR%\WebGL\libBlueprintBundle.a" ^
if not exist "%UNITY_PLUGINS_DIR%\WebGL\libBlueprintBundleMin.a" (
    call :collect_req wasm "%PROJECT_DIR%\build-wasm\Runtime\libBlueprintRuntime.a" ^
        "%UNITY_PLUGINS_DIR%\WebGL" "libBlueprintRuntime.a"
)

:: ── Summary ───────────────────────────────────────────────────────────────────
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
echo    Windows\x86_64\  BlueprintRuntime.dll        (with Lua VM)
echo                     BlueprintRuntimeNoLua.dll   (no Lua, for xLua/tolua)
echo                     liblua54.dll
echo    Android\arm64\   libBlueprintRuntime.so      (with Lua VM)
echo                     libBlueprintRuntimeNoLua.so (no Lua)
echo                     liblua54.so
echo    WebGL\           libBlueprintBundle.a          (Runtime+JSON+Lua+Proto)
echo    WebGL\           libBlueprintBundleNoLua.a     (Runtime+JSON+Proto, no Lua)
echo    WebGL\           libBlueprintBundleNoProto.a   (Runtime+JSON+Lua, no Proto)
echo    WebGL\           libBlueprintBundleMin.a       (Runtime+JSON only)
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
