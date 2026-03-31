@echo off
:: ============================================================
:: unregister_file_associations.bat
:: 移除 BlueprintEditor 文件关联（.bjson / .bjson.editor / .bproj）
:: 需要以【管理员身份】运行
:: ============================================================
setlocal

:: ── 检查管理员权限 ───────────────────────────────────────────
net session >nul 2>&1
if %errorlevel% neq 0 (
    echo [ERROR] This script requires Administrator privileges.
    echo         Right-click the script and choose "Run as administrator".
    pause
    exit /b 1
)

echo Removing file associations...
echo.

:: .bjson
echo [1/3] Removing .bjson ...
reg delete "HKEY_CLASSES_ROOT\.bjson"                  /f >nul 2>&1
reg delete "HKEY_CLASSES_ROOT\BlueprintEditor.bjson"   /f >nul 2>&1
echo     OK

:: .bjson.editor
echo [2/3] Removing .bjson.editor ...
reg delete "HKEY_CLASSES_ROOT\.bjson.editor"                   /f >nul 2>&1
reg delete "HKEY_CLASSES_ROOT\BlueprintEditor.bjson.editor"    /f >nul 2>&1
echo     OK

:: .bproj
echo [3/3] Removing .bproj ...
reg delete "HKEY_CLASSES_ROOT\.bproj"                  /f >nul 2>&1
reg delete "HKEY_CLASSES_ROOT\BlueprintEditor.bproj"   /f >nul 2>&1
echo     OK

echo.
ie4uinit.exe -show >nul 2>&1
rundll32.exe user32.dll,UpdatePerUserSystemParameters 1 >nul 2>&1

echo ============================================================
echo  File associations removed.
echo ============================================================
echo.
pause
