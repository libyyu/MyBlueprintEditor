@echo off
setlocal

set PROJECT_DIR=%~dp0
:: 去掉末尾的反斜杠
if "%PROJECT_DIR:~-1%"=="\" set PROJECT_DIR=%PROJECT_DIR:~0,-1%
set BUILD_DIR=%PROJECT_DIR%\build

:: 默认参数
set BUILD_TYPE=Release
set BUILD_EXAMPLES=ON
set CLEAN_BUILD=0

:: 解析命令行参数
:parse_args
if "%~1"=="" goto :start_build
if /i "%~1"=="debug"   (set BUILD_TYPE=Debug& shift & goto :parse_args)
if /i "%~1"=="release" (set BUILD_TYPE=Release& shift & goto :parse_args)
if /i "%~1"=="clean"   (set CLEAN_BUILD=1& shift & goto :parse_args)
if /i "%~1"=="noexamples" (set BUILD_EXAMPLES=OFF& shift & goto :parse_args)
echo Unknown option: %~1
echo.
echo Usage: build.bat [debug^|release] [clean] [noexamples]
echo   debug       - Build Debug configuration (default: Release)
echo   release     - Build Release configuration
echo   clean       - Clean build directory before building
echo   noexamples  - Skip building examples
exit /b 1

:start_build
echo ============================================
echo  Blueprint Editor - Build Script
echo  Configuration: %BUILD_TYPE%
echo  Examples: %BUILD_EXAMPLES%
echo ============================================
echo.

:: 清理构建目录（如果指定了 clean）
if %CLEAN_BUILD%==1 (
    echo [1/3] Cleaning build directory...
    if exist "%BUILD_DIR%" rmdir /s /q "%BUILD_DIR%"
    echo       Done.
) else (
    echo [1/3] Skipping clean (use 'clean' option to force)
)

:: 创建构建目录
if not exist "%BUILD_DIR%" mkdir "%BUILD_DIR%"

:: CMake 配置
echo.
echo [2/3] Running CMake configure...
cmake -S "%PROJECT_DIR%" -B "%BUILD_DIR%" -A x64 -DCMAKE_BUILD_TYPE=%BUILD_TYPE% -DBUILD_EXAMPLES=%BUILD_EXAMPLES%

if %ERRORLEVEL% neq 0 (
    echo.
    echo [ERROR] CMake configure failed!
    exit /b %ERRORLEVEL%
)

:: 编译
echo.
echo [3/3] Building...
cmake --build "%BUILD_DIR%" --config %BUILD_TYPE% --parallel

if %ERRORLEVEL% neq 0 (
    echo.
    echo [ERROR] Build failed!
    exit /b %ERRORLEVEL%
)

echo.
echo ============================================
echo  Build succeeded! (%BUILD_TYPE%)
echo  Output: %BUILD_DIR%\bin
echo ============================================

endlocal
