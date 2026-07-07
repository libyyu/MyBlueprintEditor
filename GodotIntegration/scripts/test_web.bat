@echo off
chcp 65001 >nul
@set proj_root_path=%~dp0
@set proj_root_path=%proj_root_path:~,-1%
@set USE_BUSYBOX=1
rem Web 调试：保留符号名，便于浏览器 stack trace 定位（发布再改回 0）
@set PACK_EXPORT_DEBUG=1
cd /d "%proj_root_path%"
"%proj_root_path%\..\..\tools\busybox.exe" sh pack.sh web
pause