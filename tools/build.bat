@echo off
setlocal EnableExtensions EnableDelayedExpansion

set "SCRIPT_DIR=%~dp0"
set "PROJECT_ROOT=%SCRIPT_DIR%.."

if not exist "%PROJECT_ROOT%\build\build.ninja" (
    cmake --preset default
    if errorlevel 1 exit /b 1
)
cmake --build --preset default
