@echo off
chcp 65001 >nul
@set proj_root_path=%~dp0
@set proj_root_path=%proj_root_path:~,-1%
@set USE_BUSYBOX=1
%proj_root_path%\..\..\tools\busybox.exe sh %*
