@echo off
:: ============================================================
:: register_file_associations.bat
:: 注册 BlueprintEditor 文件关联（.bjson / .bjson.editor / .bproj）
:: 需要以【管理员身份】运行
:: ============================================================
setlocal EnableDelayedExpansion

:: ── 定位 exe ────────────────────────────────────────────────
:: 优先取脚本所在目录的 build-windows\bin\Release\BlueprintEditor.exe
set "SCRIPT_DIR=%~dp0"
set "EXE_PATH=%SCRIPT_DIR%build-windows\bin\Release\BlueprintEditor.exe"

if not exist "%EXE_PATH%" (
    :: fallback: 和脚本同目录
    set "EXE_PATH=%SCRIPT_DIR%BlueprintEditor.exe"
)
if not exist "%EXE_PATH%" (
    echo [ERROR] BlueprintEditor.exe not found.
    echo         Searched:
    echo           %SCRIPT_DIR%build-windows\bin\Release\BlueprintEditor.exe
    echo           %SCRIPT_DIR%BlueprintEditor.exe
    echo.
    echo         Please build the project first, or copy the exe next to this script.
    pause
    exit /b 1
)

echo [INFO] Using: %EXE_PATH%
echo.

:: ── 检查管理员权限 ───────────────────────────────────────────
net session >nul 2>&1
if %errorlevel% neq 0 (
    echo [ERROR] This script requires Administrator privileges.
    echo         Right-click the script and choose "Run as administrator".
    pause
    exit /b 1
)

:: ============================================================
:: 注册 .bjson（蓝图运行时文件）
:: ============================================================
echo [1/3] Registering .bjson ...

:: 文件扩展名 -> ProgID
reg add "HKEY_CLASSES_ROOT\.bjson"                          /ve /d "BlueprintEditor.bjson" /f >nul
reg add "HKEY_CLASSES_ROOT\.bjson"                          /v "Content Type" /d "application/json" /f >nul
reg add "HKEY_CLASSES_ROOT\.bjson\OpenWithProgids"          /v "BlueprintEditor.bjson" /d "" /f >nul

:: ProgID 描述
reg add "HKEY_CLASSES_ROOT\BlueprintEditor.bjson"           /ve /d "Blueprint File" /f >nul
reg add "HKEY_CLASSES_ROOT\BlueprintEditor.bjson\DefaultIcon" /ve /d "\"%EXE_PATH%\",0" /f >nul
reg add "HKEY_CLASSES_ROOT\BlueprintEditor.bjson\shell\open" /ve /d "Open" /f >nul
reg add "HKEY_CLASSES_ROOT\BlueprintEditor.bjson\shell\open\command" /ve /d "\"%EXE_PATH%\" \"%%1\"" /f >nul
reg add "HKEY_CLASSES_ROOT\BlueprintEditor.bjson\shell\open\ddeexec" /ve /d "" /f >nul

echo     OK

:: ============================================================
:: 注册 .bjson.editor（蓝图编辑器辅助文件）
:: ============================================================
echo [2/3] Registering .bjson.editor ...

reg add "HKEY_CLASSES_ROOT\.bjson.editor"                          /ve /d "BlueprintEditor.bjson.editor" /f >nul
reg add "HKEY_CLASSES_ROOT\.bjson.editor"                          /v "Content Type" /d "application/json" /f >nul
reg add "HKEY_CLASSES_ROOT\.bjson.editor\OpenWithProgids"          /v "BlueprintEditor.bjson.editor" /d "" /f >nul

reg add "HKEY_CLASSES_ROOT\BlueprintEditor.bjson.editor"           /ve /d "Blueprint Editor File" /f >nul
reg add "HKEY_CLASSES_ROOT\BlueprintEditor.bjson.editor\DefaultIcon" /ve /d "\"%EXE_PATH%\",0" /f >nul
reg add "HKEY_CLASSES_ROOT\BlueprintEditor.bjson.editor\shell\open" /ve /d "Open" /f >nul
reg add "HKEY_CLASSES_ROOT\BlueprintEditor.bjson.editor\shell\open\command" /ve /d "\"%EXE_PATH%\" \"%%1\"" /f >nul

echo     OK

:: ============================================================
:: 注册 .bproj（蓝图工程文件）
:: ============================================================
echo [3/3] Registering .bproj ...

reg add "HKEY_CLASSES_ROOT\.bproj"                          /ve /d "BlueprintEditor.bproj" /f >nul
reg add "HKEY_CLASSES_ROOT\.bproj\OpenWithProgids"          /v "BlueprintEditor.bproj" /d "" /f >nul

reg add "HKEY_CLASSES_ROOT\BlueprintEditor.bproj"           /ve /d "Blueprint Project" /f >nul
reg add "HKEY_CLASSES_ROOT\BlueprintEditor.bproj\DefaultIcon" /ve /d "\"%EXE_PATH%\",0" /f >nul
reg add "HKEY_CLASSES_ROOT\BlueprintEditor.bproj\shell\open" /ve /d "Open" /f >nul
reg add "HKEY_CLASSES_ROOT\BlueprintEditor.bproj\shell\open\command" /ve /d "\"%EXE_PATH%\" \"%%1\"" /f >nul

echo     OK
echo.

:: ── 刷新文件图标缓存 ─────────────────────────────────────────
echo [INFO] Refreshing shell icon cache...
ie4uinit.exe -show >nul 2>&1
rundll32.exe user32.dll,UpdatePerUserSystemParameters 1 >nul 2>&1

echo.
echo ============================================================
echo  File associations registered successfully!
echo.
echo  .bjson        -> Blueprint File        (open with BlueprintEditor)
echo  .bjson.editor -> Blueprint Editor File (open with BlueprintEditor)
echo  .bproj        -> Blueprint Project     (open with BlueprintEditor)
echo.
echo  You can now double-click or right-click these files to open them.
echo ============================================================
echo.
pause
