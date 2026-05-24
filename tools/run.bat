@echo off
setlocal EnableExtensions EnableDelayedExpansion

set "SCRIPT_DIR=%~dp0"
set "PROJECT_ROOT=%SCRIPT_DIR%.."

call "%SCRIPT_DIR%build.bat"
if errorlevel 1 exit /b 1
"%PROJECT_ROOT%\build\project.exe"
