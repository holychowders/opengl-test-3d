@echo off
setlocal EnableExtensions EnableDelayedExpansion

for /r src %%f in (*.c *.cpp *.h *.hpp) do set FILES=!FILES! "%%f"
clang-format -i %FILES%
