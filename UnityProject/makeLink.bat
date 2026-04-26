@echo off
::get current directory
@set self_path=%~dp0
@set self_path=%self_path:~,-1%
@echo current dir: %self_path%
mklink /J %self_path%\Assets\Plugins %self_path%\..\Unity\Runtime\Plugins
mklink /J %self_path%\Assets\Samples %self_path%\..\Unity\Samples
mklink /J %self_path%\Assets\Scripts %self_path%\..\Unity\Runtime\Scripts
mklink /J %self_path%\Assets\Data %self_path%\..\data
pause