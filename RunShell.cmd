@echo off

@set proj_root_path=%~dp0
@set proj_root_path=%proj_root_path:~,-1%

sh %*
