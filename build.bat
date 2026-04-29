@echo off
setlocal enabledelayedexpansion

:: ==============================================================================
:: build.bat – MyBlueprintEditor cross-platform build script (Windows)
::
:: Usage:
::   build.bat [platform] [options...]
::
:: Platforms:
::   windows       Windows x64, Win32+DX11, full editor – shared runtime (default)
::   windows-dll   Windows x64, full editor + BlueprintRuntime as shared DLL (alias for windows)
::   windows-static Windows x64, full editor + BlueprintRuntime as static lib
::   dll           Windows x64, Runtime-only as shared DLL (no Editor)
::   wasm          WebAssembly via Emscripten (Runtime only, static)
::   android       Android ARM64-v8a (Runtime only)
::   runtime       Windows host, Runtime-only build, static (no Editor)
::
:: Options:
::   debug         Build Debug configuration (default: Release)
::   release       Build Release configuration
::   clean         Wipe build directory before building
::   noexamples    Skip building examples
::   shared        Force BlueprintRuntime as shared DLL (overrides platform default)
::   --ndk <path>  Android NDK root (required for android platform)
::   --api <N>     Android min API level (default: 21)
::   --emsdk <path> Emscripten SDK root (default: %EMSDK% env var)
::
:: Examples:
::   build.bat                          Windows Release, shared runtime + full editor
::   build.bat windows-dll              Alias for build.bat (same as default)
::   build.bat windows-static           Windows Release, static runtime + full editor
::   build.bat dll                      Windows Release, Runtime DLL only
::   build.bat dll debug                Windows Debug, Runtime DLL only
::   build.bat debug                    Windows Debug, shared runtime + full editor
::   build.bat wasm                     WASM Release (needs Emscripten)
::   build.bat android --ndk C:\ndk     Android Release
::   build.bat runtime shared           Runtime-only, shared DLL
:: ==============================================================================

set PROJECT_DIR=%~dp0
if "%PROJECT_DIR:~-1%"=="\" set PROJECT_DIR=%PROJECT_DIR:~0,-1%

:: ── Defaults ─────────────────────────────────────────────────────────────────
set PLATFORM=windows
set BUILD_TYPE=Release
set CLEAN_BUILD=0
set BUILD_EXAMPLES=ON
set BUILD_SHARED=ON
set RUNTIME_ONLY=0
set NDK_PATH=
set ANDROID_API=21
set EMSDK_PATH=%EMSDK%

:: ── Parse arguments ───────────────────────────────────────────────────────────
:parse_args
if "%~1"=="" goto :validate

if /i "%~1"=="windows"     (set PLATFORM=windows&      shift & goto :parse_args)
if /i "%~1"=="windows-dll" (set PLATFORM=windows&      shift & goto :parse_args)
if /i "%~1"=="windows-static" (set PLATFORM=windows-static& shift & goto :parse_args)
if /i "%~1"=="dll"         (set PLATFORM=dll&           shift & goto :parse_args)
if /i "%~1"=="wasm"        (set PLATFORM=wasm&          shift & goto :parse_args)
if /i "%~1"=="android"     (set PLATFORM=android&       shift & goto :parse_args)
if /i "%~1"=="runtime"     (set PLATFORM=runtime&       shift & goto :parse_args)

if /i "%~1"=="debug"      (set BUILD_TYPE=Debug&     shift & goto :parse_args)
if /i "%~1"=="release"    (set BUILD_TYPE=Release&   shift & goto :parse_args)
if /i "%~1"=="clean"      (set CLEAN_BUILD=1&        shift & goto :parse_args)
if /i "%~1"=="noexamples" (set BUILD_EXAMPLES=OFF&   shift & goto :parse_args)
if /i "%~1"=="shared"     (set BUILD_SHARED=ON&      shift & goto :parse_args)

if /i "%~1"=="--ndk"      (set NDK_PATH=%~2&         shift & shift & goto :parse_args)
if /i "%~1"=="--api"      (set ANDROID_API=%~2&      shift & shift & goto :parse_args)
if /i "%~1"=="--emsdk"    (set EMSDK_PATH=%~2&       shift & shift & goto :parse_args)

if /i "%~1"=="/?" goto :show_help
if /i "%~1"=="--help" goto :show_help

echo [ERROR] Unknown option: %~1
echo.
goto :show_help

:show_help
echo Usage: build.bat [platform] [options...]
echo.
echo Platforms:
echo   windows      Windows x64, full editor, shared runtime (default)
echo   windows-dll  Alias for 'windows' (shared runtime)
echo   windows-static Windows x64, full editor, static runtime
echo   dll          Windows x64, Runtime-only shared DLL (no Editor)
echo   wasm         WebAssembly via Emscripten (Runtime only)
echo   android      Android ARM64-v8a (Runtime only)
echo   runtime      Windows host, Runtime-only static (no Editor)
echo.
echo Options:
echo   debug / release     Build configuration (default: Release)
echo   clean               Wipe build directory before building
echo   noexamples          Skip building examples
echo   shared              Force shared DLL (overrides platform default)
echo   --ndk ^<path^>        Android NDK root
echo   --api ^<N^>           Android min API level (default: 21)
echo   --emsdk ^<path^>      Emscripten SDK root
exit /b 1

:: ── Validate & resolve platform-specific settings ────────────────────────────
:validate
set BUILD_DIR=%PROJECT_DIR%\build-%PLATFORM%

if /i "%PLATFORM%"=="windows" (
    set RUNTIME_ONLY=0
    set CMAKE_EXTRA=-A x64 -DBUILD_EXAMPLES=%BUILD_EXAMPLES% -DBUILD_SHARED_LIBS=%BUILD_SHARED%
    goto :banner
)

if /i "%PLATFORM%"=="windows-static" (
    :: Full editor + static lib
    set RUNTIME_ONLY=0
    set BUILD_SHARED=OFF
    set CMAKE_EXTRA=-A x64 -DBUILD_EXAMPLES=%BUILD_EXAMPLES% -DBUILD_SHARED_LIBS=OFF
    goto :banner
)

if /i "%PLATFORM%"=="dll" (
    :: Runtime-only shared DLL, no Editor
    set RUNTIME_ONLY=1
    set BUILD_SHARED=ON
    set CMAKE_EXTRA=-A x64 -DBUILD_RUNTIME_ONLY=ON -DBUILD_SHARED_LIBS=ON -DBUILD_EXAMPLES=%BUILD_EXAMPLES%
    goto :banner
)

if /i "%PLATFORM%"=="runtime" (
    set RUNTIME_ONLY=1
    set CMAKE_EXTRA=-A x64 -DBUILD_RUNTIME_ONLY=ON -DBUILD_SHARED_LIBS=%BUILD_SHARED% -DBUILD_EXAMPLES=%BUILD_EXAMPLES%
    goto :banner
)

if /i "%PLATFORM%"=="wasm" (
    set RUNTIME_ONLY=1
    :: Locate emcmake
    if "%EMSDK_PATH%"=="" (
        where emcmake >nul 2>&1
        if errorlevel 1 (
            echo [ERROR] Emscripten not found. Install the Emscripten SDK or pass --emsdk ^<path^>.
            echo         See: https://emscripten.org/docs/getting_started/downloads.html
            exit /b 1
        )
        set EMCMAKE=emcmake
    ) else (
        :: Try activating emsdk
        if exist "%EMSDK_PATH%\emsdk_env.bat" call "%EMSDK_PATH%\emsdk_env.bat" >nul 2>&1
        set EMCMAKE=emcmake
    )
    set CMAKE_EXTRA=-DBUILD_RUNTIME_ONLY=ON -DCMAKE_BUILD_TYPE=%BUILD_TYPE%
    goto :banner
)

if /i "%PLATFORM%"=="android" (
    set RUNTIME_ONLY=1
    :: Resolve NDK path
    if "%NDK_PATH%"=="" (
        if not "%ANDROID_NDK%"==""      set NDK_PATH=%ANDROID_NDK%
        if not "%ANDROID_NDK_HOME%"=="" set NDK_PATH=%ANDROID_NDK_HOME%
    )
    if "%NDK_PATH%"=="" (
        echo [ERROR] Android NDK not found. Pass --ndk ^<path^> or set %%ANDROID_NDK%%.
        exit /b 1
    )
    set CMAKE_EXTRA=-DCMAKE_TOOLCHAIN_FILE="%NDK_PATH%\build\cmake\android.toolchain.cmake" -DANDROID_ABI=arm64-v8a -DANDROID_PLATFORM=android-%ANDROID_API% -DBUILD_RUNTIME_ONLY=ON -DBUILD_SHARED_LIBS=%BUILD_SHARED% -DBUILD_EXAMPLES=OFF -DCMAKE_BUILD_TYPE=%BUILD_TYPE%
    goto :banner
)

echo [ERROR] Unknown platform: %PLATFORM%
exit /b 1

:: ── Banner ────────────────────────────────────────────────────────────────────
:banner
echo.
echo ============================================
echo  Blueprint Editor - Build Script
echo  Platform      : %PLATFORM%
echo  Configuration : %BUILD_TYPE%
echo  Runtime only  : %RUNTIME_ONLY%
echo  Shared libs   : %BUILD_SHARED%
echo  Examples      : %BUILD_EXAMPLES%
echo  Build dir     : %BUILD_DIR%
echo ============================================
echo.

:: ── Step 1 – Clean ────────────────────────────────────────────────────────────
if %CLEAN_BUILD%==1 (
    echo [1/3] Cleaning build directory...
    if exist "%BUILD_DIR%" rmdir /s /q "%BUILD_DIR%"
    echo       Done.
) else (
    echo [1/3] Skipping clean (pass 'clean' to force^)
)

if not exist "%BUILD_DIR%" mkdir "%BUILD_DIR%"

:: ── Step 2 – CMake configure ─────────────────────────────────────────────────
echo.
echo [2/3] Running CMake configure...

if /i "%PLATFORM%"=="wasm" (
    %EMCMAKE% cmake -G "Ninja" -S "%PROJECT_DIR%" -B "%BUILD_DIR%" %CMAKE_EXTRA%
) else (
    cmake -S "%PROJECT_DIR%" -B "%BUILD_DIR%" %CMAKE_EXTRA%
)

if %ERRORLEVEL% neq 0 (
    echo.
    echo [ERROR] CMake configure failed!
    exit /b %ERRORLEVEL%
)

:: ── Step 3 – Build ───────────────────────────────────────────────────────────
echo.
echo [3/3] Building...
cmake --build "%BUILD_DIR%" --config %BUILD_TYPE% --parallel

if %ERRORLEVEL% neq 0 (
    echo.
    echo [ERROR] Build failed!
    exit /b %ERRORLEVEL%
)

:: ── Done ──────────────────────────────────────────────────────────────────────
echo.
echo ============================================
echo  Build succeeded! (%PLATFORM% / %BUILD_TYPE%)
echo  Output: %BUILD_DIR%\bin\%BUILD_TYPE%
if /i "%BUILD_SHARED%"=="ON" (
    echo  DLL:     %BUILD_DIR%\bin\%BUILD_TYPE%\BlueprintRuntime.dll
    echo  Import:  %BUILD_DIR%\Runtime\%BUILD_TYPE%\BlueprintRuntime.lib
)
if /i "%PLATFORM%"=="dll" (
    echo  Header:  Runtime\BlueprintRuntime.h
)
if /i "%PLATFORM%"=="wasm" (
    echo  WASM lib: %BUILD_DIR%\Runtime\libBlueprintRuntime.a
    echo  ^> Drop into Assets/Plugins/WebGL/ for Unity
)
if /i "%PLATFORM%"=="android" (
    echo  Android lib: %BUILD_DIR%\Runtime\
)
echo ============================================

endlocal
