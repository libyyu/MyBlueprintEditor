@echo off
chcp 65001 >nul
setlocal enabledelayedexpansion

set PROJECT_DIR=%~dp0
if "%PROJECT_DIR:~-1%"=="\" set PROJECT_DIR=%PROJECT_DIR:~0,-1%
set DIST_DIR=%PROJECT_DIR%\dist
set SKIP_BUILD=0
set PRESET=

:parse
if "%~1"=="" goto :choose
if /i "%~1"=="full"          set PRESET=full&   shift& goto :parse
if /i "%~1"=="nolua"         set PRESET=nolua&  shift& goto :parse
if /i "%~1"=="min"           set PRESET=min&    shift& goto :parse
if /i "%~1"=="--skip-build"  set SKIP_BUILD=1&  shift& goto :parse
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

:choose
if not "%PRESET%"=="" goto :resolve

echo.
echo ============================================
echo   MyBlueprintEditor - Package Tool
echo ============================================
echo.
echo   [1] Full (recommended)
echo       Runtime + Lua VM + Protobuf
echo.
echo   [2] No Lua
echo       Runtime + Protobuf, for xLua/tolua
echo.
echo   [3] Minimal
echo       Core engine only, smallest size
echo.
set /p CHOICE="  Select [1/2/3] (default 1): "
if "%CHOICE%"=="" set CHOICE=1
if "%CHOICE%"=="1" set PRESET=full
if "%CHOICE%"=="2" set PRESET=nolua
if "%CHOICE%"=="3" set PRESET=min
if "%PRESET%"=="" echo [ERROR] Invalid choice& goto :choose

:resolve
if /i "%PRESET%"=="full" (
    set DLL_NAME=BlueprintRuntime
    set SO_NAME=libBlueprintRuntime
    set BUNDLE_NAME=libBlueprintBundle
    set BUNDLE_LUA=ON
    set BUNDLE_PROTO=ON
    set PRESET_DESC=Full
)
if /i "%PRESET%"=="nolua" (
    set DLL_NAME=BlueprintRuntimeNoLua
    set SO_NAME=libBlueprintRuntimeNoLua
    set BUNDLE_NAME=libBlueprintBundleNoLua
    set BUNDLE_LUA=OFF
    set BUNDLE_PROTO=ON
    set PRESET_DESC=NoLua
)
if /i "%PRESET%"=="min" (
    set DLL_NAME=BlueprintRuntimeNoLua
    set SO_NAME=libBlueprintRuntimeNoLua
    set BUNDLE_NAME=libBlueprintBundleMin
    set BUNDLE_LUA=OFF
    set BUNDLE_PROTO=OFF
    set PRESET_DESC=Minimal
)

echo.
echo   Preset : %PRESET_DESC%
echo   DLL    : %DLL_NAME%.dll
echo   SO     : %SO_NAME%.so
echo   WASM   : %BUNDLE_NAME%.a
echo.

:: ── Build ───────────────────────────────────────────────────────────────────
if %SKIP_BUILD%==1 (
    echo [1/3] Skip build, using existing artifacts
    goto :collect
)

echo [1/3] Building all platforms...
echo.

:: Check toolchains
set HAS_EMSDK=0
set HAS_NDK=0
if not "%EMSDK%"=="" set HAS_EMSDK=1
where emcmake >nul 2>&1
if not errorlevel 1 set HAS_EMSDK=1
if not "%ANDROID_NDK%"=="" set HAS_NDK=1
if not "%ANDROID_NDK_HOME%"=="" set HAS_NDK=1

:: Windows
echo   [Windows] Building...
call "%PROJECT_DIR%\build.bat" windows release >nul 2>&1
if %ERRORLEVEL% neq 0 (
    echo   [Windows] FAILED
    exit /b 1
)
echo   [Windows] OK

:: Windows NoLua variant
if /i not "%PRESET%"=="full" (
    echo   [Windows] Building NoLua variant...
    cmake -S "%PROJECT_DIR%" -B "%PROJECT_DIR%\build-windows" -DBLUEPRINT_LUA=OFF >nul 2>&1
    cmake --build "%PROJECT_DIR%\build-windows" --target BlueprintRuntime --config Release --parallel >nul 2>&1
    if exist "%PROJECT_DIR%\build-windows\bin\Release\BlueprintRuntime.dll" (
        copy /y "%PROJECT_DIR%\build-windows\bin\Release\BlueprintRuntime.dll" "%PROJECT_DIR%\build-windows\bin\Release\BlueprintRuntimeNoLua.dll" >nul
    )
    cmake -S "%PROJECT_DIR%" -B "%PROJECT_DIR%\build-windows" -DBLUEPRINT_LUA=ON >nul 2>&1
    cmake --build "%PROJECT_DIR%\build-windows" --target BlueprintRuntime --config Release --parallel >nul 2>&1
    echo   [Windows] NoLua OK
)

:: WebGL
if %HAS_EMSDK%==1 (
    echo   [WebGL]   Building...
    call "%PROJECT_DIR%\build.bat" wasm release >nul 2>&1
    if errorlevel 1 (
        echo   [WebGL]   FAILED
    ) else (
        echo   [WebGL]   Runtime OK
        echo   [WebGL]   Merging Bundle...
        cmake -S "%PROJECT_DIR%" -B "%PROJECT_DIR%\build-wasm" -UBUILD_BUNDLE -UBUNDLE_LUA -UBUNDLE_PROTOBUF -UBUNDLE_NAME -DBUILD_BUNDLE=ON -DBUNDLE_LUA=%BUNDLE_LUA% -DBUNDLE_PROTOBUF=%BUNDLE_PROTO% -DCMAKE_BUILD_TYPE=Release >nul 2>&1
        cmake --build "%PROJECT_DIR%\build-wasm" --target BlueprintBundle --config Release >nul 2>&1
        echo   [WebGL]   Bundle OK
    )
) else (
    echo   [WebGL]   SKIP - Emscripten not found. Set %%EMSDK%% to enable.
)

:: Android
if %HAS_NDK%==1 (
    echo   [Android] Building...
    set _NDK=%ANDROID_NDK%
    if "!_NDK!"=="" set _NDK=%ANDROID_NDK_HOME%
    call "%PROJECT_DIR%\build.bat" android release --ndk "!_NDK!" >nul 2>&1
    if errorlevel 1 (
        echo   [Android] FAILED
    ) else (
        echo   [Android] OK
    )
) else (
    echo   [Android] SKIP - NDK not found. Set %%ANDROID_NDK%% to enable.
)

:: ── Collect ─────────────────────────────────────────────────────────────────
:collect
echo.
echo [2/3] Collecting artifacts to dist/...

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
    echo   OK  Windows: %DLL_NAME%.dll
    set /a COLLECTED+=1
) else (
    echo   --  Windows: %DLL_NAME%.dll not found
)
if /i "%PRESET%"=="full" (
    if exist "%WIN_BIN%\liblua54.dll" (
        copy /y "%WIN_BIN%\liblua54.dll" "%DIST_DIR%\UnityPlugins\Windows\x86_64\liblua54.dll" >nul
        echo   OK  Windows: liblua54.dll
    )
)

:: Editor
if exist "%WIN_BIN%\BlueprintEditor.exe" (
    copy /y "%WIN_BIN%\BlueprintEditor.exe" "%DIST_DIR%\Editor\BlueprintEditor.exe" >nul
    if exist "%WIN_BIN%\BlueprintRuntime.dll" copy /y "%WIN_BIN%\BlueprintRuntime.dll" "%DIST_DIR%\Editor\BlueprintRuntime.dll" >nul
    if exist "%WIN_BIN%\liblua54.dll" copy /y "%WIN_BIN%\liblua54.dll" "%DIST_DIR%\Editor\liblua54.dll" >nul
    echo   OK  Editor: BlueprintEditor.exe
    set /a COLLECTED+=1
)

:: Android
set AND_BIN=%PROJECT_DIR%\build-android\bin\Release
if exist "%AND_BIN%\%SO_NAME%.so" (
    copy /y "%AND_BIN%\%SO_NAME%.so" "%DIST_DIR%\UnityPlugins\Android\arm64-v8a\libBlueprintRuntime.so" >nul
    echo   OK  Android: %SO_NAME%.so
    set /a COLLECTED+=1
) else if exist "%AND_BIN%\libBlueprintRuntime.so" (
    copy /y "%AND_BIN%\libBlueprintRuntime.so" "%DIST_DIR%\UnityPlugins\Android\arm64-v8a\libBlueprintRuntime.so" >nul
    echo   OK  Android: libBlueprintRuntime.so
    set /a COLLECTED+=1
) else (
    echo   --  Android: no artifacts
)

:: WebGL
set WASM_BIN=%PROJECT_DIR%\build-wasm\bin\Release
if exist "%WASM_BIN%\%BUNDLE_NAME%.a" (
    copy /y "%WASM_BIN%\%BUNDLE_NAME%.a" "%DIST_DIR%\UnityPlugins\WebGL\libBlueprintRuntime.a" >nul
    echo   OK  WebGL: %BUNDLE_NAME%.a
    set /a COLLECTED+=1
) else if exist "%PROJECT_DIR%\build-wasm\Runtime\libBlueprintRuntime.a" (
    copy /y "%PROJECT_DIR%\build-wasm\Runtime\libBlueprintRuntime.a" "%DIST_DIR%\UnityPlugins\WebGL\libBlueprintRuntime.a" >nul
    echo   OK  WebGL: libBlueprintRuntime.a (unbundled fallback)
    set /a COLLECTED+=1
) else (
    echo   --  WebGL: no artifacts
)

:: ── README ──────────────────────────────────────────────────────────────────
echo.
echo [3/3] Done!

echo Preset: %PRESET_DESC% > "%DIST_DIR%\README.txt"
echo Date: %DATE% %TIME% >> "%DIST_DIR%\README.txt"
echo. >> "%DIST_DIR%\README.txt"
echo Copy UnityPlugins/ to your Unity project Assets/Plugins/ >> "%DIST_DIR%\README.txt"
echo Copy Unity/Assets/Scripts/BlueprintRuntime.cs to Assets/Scripts/ >> "%DIST_DIR%\README.txt"

echo.
echo ============================================
echo   Package complete!
echo   Preset  : %PRESET_DESC%
echo   Output  : %DIST_DIR%
echo   Platforms: %COLLECTED%
echo ============================================
echo.
echo   dist\
echo     UnityPlugins\    -- copy to Unity Assets\Plugins\
echo     Editor\          -- blueprint editor (Windows)
echo     README.txt
echo.

endlocal
