@echo off
setlocal EnableExtensions EnableDelayedExpansion
for /r src %%f in (*.c *.cpp) do set FILES=!FILES! "%%f"
clang-tidy -p .\build\ %FILES%
